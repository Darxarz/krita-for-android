#include <jni.h>

#include <android/log.h>

#include <cerrno>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <signal.h>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ucontext.h>
#include <unistd.h>

extern "C" int krita_android_python_runtime_init_probe(const char *runtimeRoot);
extern "C" int krita_android_python_runtime_init_probe_message(const char *runtimeRoot,
                                                               char *messageBuffer,
                                                               int messageBufferSize);
extern "C" int krita_android_python_runtime_import_probe(const char *runtimeRoot,
                                                         char *messageBuffer,
                                                         int messageBufferSize);
extern "C" int krita_android_python_runtime_import_one_probe(const char *runtimeRoot,
                                                             const char *moduleName,
                                                             char *messageBuffer,
                                                             int messageBufferSize);

namespace
{
constexpr const char *LOG_TAG = "KritaPyRuntimeProbe";
constexpr int CHILD_PROBE_FAILED_EXIT = 100;
int g_childCrashPipeFd = -1;

enum class ChildProbeMode
{
    ImportOne,
    DlopenPyKrita,
    DlopenPath,
};

std::string toString(JNIEnv *env, jstring value)
{
    if (!value) {
        return {};
    }

    const char *chars = env->GetStringUTFChars(value, nullptr);
    if (!chars) {
        return {};
    }

    std::string result(chars);
    env->ReleaseStringUTFChars(value, chars);
    return result;
}

std::string pyKritaExtensionPath(const std::string &runtimeRoot)
{
    return runtimeRoot + "/assets/python/krita-python-libs/PyKrita/krita.so";
}

void writeRaw(int fd, const char *text)
{
    if (fd < 0 || !text) {
        return;
    }
    write(fd, text, std::strlen(text));
}

void writeDecimal(int fd, long long value)
{
    char buffer[32] = {};
    char *cursor = buffer + sizeof(buffer) - 1;
    bool negative = value < 0;
    unsigned long long remaining = negative
            ? static_cast<unsigned long long>(-value)
            : static_cast<unsigned long long>(value);

    do {
        *--cursor = static_cast<char>('0' + (remaining % 10));
        remaining /= 10;
    } while (remaining > 0 && cursor > buffer);

    if (negative && cursor > buffer) {
        *--cursor = '-';
    }

    write(fd, cursor, static_cast<size_t>((buffer + sizeof(buffer) - 1) - cursor));
}

void writeHex(int fd, uintptr_t value)
{
    char buffer[2 + sizeof(uintptr_t) * 2] = {};
    buffer[0] = '0';
    buffer[1] = 'x';
    for (size_t index = 0; index < sizeof(uintptr_t) * 2; ++index) {
        const size_t shift = (sizeof(uintptr_t) * 2 - index - 1) * 4;
        const uintptr_t nibble = (value >> shift) & 0xf;
        buffer[2 + index] = static_cast<char>(nibble < 10 ? '0' + nibble : 'a' + (nibble - 10));
    }
    write(fd, buffer, sizeof(buffer));
}

bool hexValue(char value, uintptr_t *digit)
{
    if (value >= '0' && value <= '9') {
        *digit = static_cast<uintptr_t>(value - '0');
        return true;
    }
    if (value >= 'a' && value <= 'f') {
        *digit = static_cast<uintptr_t>(value - 'a' + 10);
        return true;
    }
    if (value >= 'A' && value <= 'F') {
        *digit = static_cast<uintptr_t>(value - 'A' + 10);
        return true;
    }
    return false;
}

const char *parseHex(const char *cursor, const char *end, uintptr_t *result)
{
    uintptr_t value = 0;
    bool any = false;
    while (cursor < end) {
        uintptr_t digit = 0;
        if (!hexValue(*cursor, &digit)) {
            break;
        }
        value = (value << 4) | digit;
        any = true;
        ++cursor;
    }
    if (!any) {
        return nullptr;
    }
    *result = value;
    return cursor;
}

void writeMapLineForAddress(int fd, const char *label, uintptr_t address)
{
    writeRaw(fd, label);
    writeRaw(fd, "=");
    writeHex(fd, address);
    if (address == 0) {
        writeRaw(fd, " no-map\n");
        return;
    }

    const int mapsFd = open("/proc/self/maps", O_RDONLY | O_CLOEXEC);
    if (mapsFd < 0) {
        writeRaw(fd, " maps_open_errno=");
        writeDecimal(fd, errno);
        writeRaw(fd, "\n");
        return;
    }

    char buffer[1024];
    char line[1024];
    size_t lineLength = 0;
    bool matched = false;

    while (!matched) {
        const ssize_t count = read(mapsFd, buffer, sizeof(buffer));
        if (count <= 0) {
            break;
        }

        for (ssize_t index = 0; index < count && !matched; ++index) {
            const char current = buffer[index];
            if (lineLength < sizeof(line) - 1) {
                line[lineLength++] = current;
            }
            if (current != '\n') {
                continue;
            }

            const char *begin = line;
            const char *end = line + lineLength;
            uintptr_t start = 0;
            uintptr_t finish = 0;
            const char *cursor = parseHex(begin, end, &start);
            if (cursor && cursor < end && *cursor == '-') {
                cursor = parseHex(cursor + 1, end, &finish);
                if (cursor && address >= start && address < finish) {
                    writeRaw(fd, " offset=");
                    writeHex(fd, address - start);
                    writeRaw(fd, " map=");
                    write(fd, line, lineLength);
                    matched = true;
                }
            }
            lineLength = 0;
        }
    }

    close(mapsFd);
    if (!matched) {
        writeRaw(fd, " no-matching-map\n");
    }
}

uintptr_t signalPc(void *context)
{
#if defined(__aarch64__)
    return static_cast<uintptr_t>(reinterpret_cast<ucontext_t *>(context)->uc_mcontext.pc);
#elif defined(__arm__)
    return static_cast<uintptr_t>(reinterpret_cast<ucontext_t *>(context)->uc_mcontext.arm_pc);
#else
    (void)context;
    return 0;
#endif
}

uintptr_t signalLr(void *context)
{
#if defined(__aarch64__)
    return static_cast<uintptr_t>(reinterpret_cast<ucontext_t *>(context)->uc_mcontext.regs[30]);
#elif defined(__arm__)
    return static_cast<uintptr_t>(reinterpret_cast<ucontext_t *>(context)->uc_mcontext.arm_lr);
#else
    (void)context;
    return 0;
#endif
}

uintptr_t signalSp(void *context)
{
#if defined(__aarch64__)
    return static_cast<uintptr_t>(reinterpret_cast<ucontext_t *>(context)->uc_mcontext.sp);
#elif defined(__arm__)
    return static_cast<uintptr_t>(reinterpret_cast<ucontext_t *>(context)->uc_mcontext.arm_sp);
#else
    (void)context;
    return 0;
#endif
}

void writeMapsSnapshot(int fd)
{
    const int mapsFd = open("/proc/self/maps", O_RDONLY | O_CLOEXEC);
    if (mapsFd < 0) {
        writeRaw(fd, "maps_open_errno=");
        writeDecimal(fd, errno);
        writeRaw(fd, "\n");
        return;
    }

    writeRaw(fd, "/proc/self/maps:\n");
    char buffer[1024];
    size_t total = 0;
    while (total < 12000) {
        ssize_t count = read(mapsFd, buffer, sizeof(buffer));
        if (count <= 0) {
            break;
        }
        write(fd, buffer, static_cast<size_t>(count));
        total += static_cast<size_t>(count);
    }
    close(mapsFd);
    writeRaw(fd, "\n/maps_end\n");
}

void childSignalHandler(int signalNumber, siginfo_t *info, void *context)
{
    const int fd = g_childCrashPipeFd;
    if (fd >= 0) {
        const uintptr_t pc = signalPc(context);
        const uintptr_t lr = signalLr(context);
        const uintptr_t sp = signalSp(context);
        const uintptr_t fault = reinterpret_cast<uintptr_t>(info ? info->si_addr : nullptr);

        writeRaw(fd, "\nchild_signal_handler:\nsignal=");
        writeDecimal(fd, signalNumber);
        writeRaw(fd, "\nsi_code=");
        writeDecimal(fd, info ? info->si_code : 0);
        writeRaw(fd, "\nfault_addr=");
        writeHex(fd, fault);
        writeRaw(fd, "\npc=");
        writeHex(fd, pc);
        writeRaw(fd, "\nlr=");
        writeHex(fd, lr);
        writeRaw(fd, "\nsp=");
        writeHex(fd, sp);
        writeRaw(fd, "\n");
        writeMapLineForAddress(fd, "fault_map", fault);
        writeMapLineForAddress(fd, "pc_map", pc);
        writeMapLineForAddress(fd, "lr_map", lr);
        writeMapLineForAddress(fd, "sp_map", sp);
        writeMapsSnapshot(fd);
    }
    _exit(128 + signalNumber);
}

void installChildCrashHandlers(int pipeFd)
{
    g_childCrashPipeFd = pipeFd;

    struct sigaction action = {};
    action.sa_sigaction = childSignalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO | SA_RESETHAND;

    sigaction(SIGSEGV, &action, nullptr);
    sigaction(SIGBUS, &action, nullptr);
    sigaction(SIGABRT, &action, nullptr);
    sigaction(SIGILL, &action, nullptr);
}

std::string describeFile(const std::string &path)
{
    struct stat info = {};
    if (stat(path.c_str(), &info) != 0) {
        return "file=" + path + "\nexists=0\nstat_errno=" + std::to_string(errno)
                + "\nstat_error=" + std::strerror(errno);
    }

    return "file=" + path + "\nexists=1\nsize=" + std::to_string(static_cast<long long>(info.st_size))
            + "\nmode=" + std::to_string(static_cast<unsigned int>(info.st_mode));
}

int dlopenPyKritaExtension(const std::string &runtimeRoot, std::string *message)
{
    const std::string path = pyKritaExtensionPath(runtimeRoot);
    dlerror();
    void *handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        const char *error = dlerror();
        if (message) {
            *message = "FAILED: dlopen PyKrita.krita\n" + describeFile(path) + "\ndlerror="
                    + (error ? error : "unknown");
        }
        return -80;
    }

    dlerror();
    void *initSymbol = dlsym(handle, "PyInit_krita");
    const char *symbolError = dlerror();
    if (!initSymbol || symbolError) {
        dlclose(handle);
        if (message) {
            *message = "FAILED: dlsym PyInit_krita\n" + describeFile(path) + "\ndlerror="
                    + (symbolError ? symbolError : "symbol missing");
        }
        return -81;
    }

    dlclose(handle);
    if (message) {
        *message = "OK: dlopen PyKrita.krita and dlsym PyInit_krita\n" + describeFile(path);
    }
    return 0;
}

int dlopenPath(const std::string &path, std::string *message)
{
    dlerror();
    void *handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        const char *error = dlerror();
        if (message) {
            *message = "FAILED: dlopen native library\n" + describeFile(path) + "\ndlerror="
                    + (error ? error : "unknown");
        }
        return -82;
    }

    dlclose(handle);
    if (message) {
        *message = "OK: dlopen native library\n" + describeFile(path);
    }
    return 0;
}

void writeAll(int fd, const std::string &message)
{
    const char *data = message.c_str();
    size_t remaining = message.size();
    while (remaining > 0) {
        ssize_t written = write(fd, data, remaining);
        if (written <= 0) {
            return;
        }
        data += written;
        remaining -= static_cast<size_t>(written);
    }
}

std::string readAll(int fd)
{
    std::string result;
    char buffer[1024];
    while (true) {
        ssize_t count = read(fd, buffer, sizeof(buffer));
        if (count > 0) {
            result.append(buffer, static_cast<size_t>(count));
            continue;
        }
        break;
    }
    return result;
}

int runChildBody(ChildProbeMode mode,
                 const std::string &runtimeRoot,
                 const std::string &moduleName,
                 std::string *message)
{
    if (mode == ChildProbeMode::DlopenPyKrita) {
        return dlopenPyKritaExtension(runtimeRoot, message);
    }
    if (mode == ChildProbeMode::DlopenPath) {
        return dlopenPath(moduleName, message);
    }

    char probeMessage[4096] = {};
    const int result = krita_android_python_runtime_import_one_probe(runtimeRoot.c_str(),
                                                                     moduleName.c_str(),
                                                                     probeMessage,
                                                                     static_cast<int>(sizeof(probeMessage)));
    if (message) {
        *message = probeMessage;
    }
    return result;
}

std::string runChildProbe(ChildProbeMode mode, const std::string &runtimeRoot, const std::string &moduleName)
{
    int pipeFds[2] = {-1, -1};
    if (pipe(pipeFds) != 0) {
        return "FAILED: pipe() returned errno=" + std::to_string(errno);
    }

    const pid_t pid = fork();
    if (pid < 0) {
        close(pipeFds[0]);
        close(pipeFds[1]);
        return "FAILED: fork() returned errno=" + std::to_string(errno);
    }

    if (pid == 0) {
        close(pipeFds[0]);
        installChildCrashHandlers(pipeFds[1]);
        if (dup2(pipeFds[1], STDOUT_FILENO) >= 0) {
            setvbuf(stdout, nullptr, _IONBF, 0);
        }
        if (dup2(pipeFds[1], STDERR_FILENO) >= 0) {
            setvbuf(stderr, nullptr, _IONBF, 0);
        }

        if (mode == ChildProbeMode::DlopenPyKrita) {
            writeAll(pipeFds[1], "preflight:\n" + describeFile(pyKritaExtensionPath(runtimeRoot)) + "\n");
        } else if (mode == ChildProbeMode::DlopenPath) {
            writeAll(pipeFds[1], "preflight:\n" + describeFile(moduleName) + "\n");
        }

        std::string childMessage;
        const int result = runChildBody(mode, runtimeRoot, moduleName, &childMessage);
        childMessage += "\nchild_result=" + std::to_string(result);
        writeAll(pipeFds[1], childMessage);
        close(pipeFds[1]);
        _exit(result == 0 ? 0 : CHILD_PROBE_FAILED_EXIT);
    }

    close(pipeFds[1]);
    const std::string childMessage = readAll(pipeFds[0]);
    close(pipeFds[0]);

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        return "FAILED: waitpid() returned errno=" + std::to_string(errno) + "\n" + childMessage;
    }

    std::string label;
    if (mode == ChildProbeMode::DlopenPyKrita) {
        label = "child dlopen PyKrita.krita";
    } else if (mode == ChildProbeMode::DlopenPath) {
        label = "child dlopen " + moduleName;
    } else {
        label = "child import " + moduleName;
    }

    if (WIFSIGNALED(status)) {
        return "FAILED: " + label + " crashed\nsignal=" + std::to_string(WTERMSIG(status))
                + "\n" + childMessage;
    }

    if (WIFEXITED(status)) {
        const int exitCode = WEXITSTATUS(status);
        if (exitCode >= 128) {
            return "FAILED: " + label + " crashed via handler\nsignal="
                    + std::to_string(exitCode - 128) + "\n" + childMessage;
        }
        return std::string(exitCode == 0 ? "OK: " : "FAILED: ") + label
                + " exited " + std::to_string(exitCode) + "\n" + childMessage;
    }

    return "FAILED: " + label + " ended with unknown wait status=" + std::to_string(status)
            + "\n" + childMessage;
}
} // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_org_krita_android_pythonruntimeprobe_MainActivity_runInitProbe(JNIEnv *env, jclass, jstring runtimeRoot)
{
    const std::string root = toString(env, runtimeRoot);
    if (root.empty()) {
        return env->NewStringUTF("FAILED: runtime root is empty");
    }

    char probeMessage[4096] = {};
    const int result = krita_android_python_runtime_init_probe_message(root.c_str(),
                                                                       probeMessage,
                                                                       static_cast<int>(sizeof(probeMessage)));

    char message[4608];
    std::snprintf(message,
                  sizeof(message),
                  "%s: PyConfig init probe returned %d\n%s\nroot=%s",
                  result == 0 ? "OK" : "FAILED",
                  result,
                  probeMessage,
                  root.c_str());

    __android_log_print(result == 0 ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR,
                        LOG_TAG,
                        "%s",
                        message);

    return env->NewStringUTF(message);
}

extern "C" JNIEXPORT jstring JNICALL
Java_org_krita_android_pythonruntimeprobe_MainActivity_runChildImportOneProbe(JNIEnv *env,
                                                                              jclass,
                                                                              jstring runtimeRoot,
                                                                              jstring moduleName)
{
    const std::string root = toString(env, runtimeRoot);
    if (root.empty()) {
        return env->NewStringUTF("FAILED: runtime root is empty");
    }

    const std::string module = toString(env, moduleName);
    if (module.empty()) {
        return env->NewStringUTF("FAILED: module name is empty");
    }

    const std::string message = runChildProbe(ChildProbeMode::ImportOne, root, module);
    __android_log_print(message.rfind("OK:", 0) == 0 ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR,
                        LOG_TAG,
                        "%s",
                        message.c_str());
    return env->NewStringUTF(message.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_org_krita_android_pythonruntimeprobe_MainActivity_runChildDlopenPyKritaProbe(JNIEnv *env,
                                                                                  jclass,
                                                                                  jstring runtimeRoot)
{
    const std::string root = toString(env, runtimeRoot);
    if (root.empty()) {
        return env->NewStringUTF("FAILED: runtime root is empty");
    }

    const std::string message = runChildProbe(ChildProbeMode::DlopenPyKrita, root, "");
    __android_log_print(message.rfind("OK:", 0) == 0 ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR,
                        LOG_TAG,
                        "%s",
                        message.c_str());
    return env->NewStringUTF(message.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_org_krita_android_pythonruntimeprobe_MainActivity_runChildDlopenPathProbe(JNIEnv *env,
                                                                               jclass,
                                                                               jstring libraryPath)
{
    const std::string path = toString(env, libraryPath);
    if (path.empty()) {
        return env->NewStringUTF("FAILED: native library path is empty");
    }

    const std::string message = runChildProbe(ChildProbeMode::DlopenPath, "", path);
    __android_log_print(message.rfind("OK:", 0) == 0 ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR,
                        LOG_TAG,
                        "%s",
                        message.c_str());
    return env->NewStringUTF(message.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_org_krita_android_pythonruntimeprobe_MainActivity_runImportProbe(JNIEnv *env, jclass, jstring runtimeRoot)
{
    const std::string root = toString(env, runtimeRoot);
    if (root.empty()) {
        return env->NewStringUTF("FAILED: runtime root is empty");
    }

    char probeMessage[4096] = {};
    const int result = krita_android_python_runtime_import_probe(root.c_str(),
                                                                 probeMessage,
                                                                 static_cast<int>(sizeof(probeMessage)));

    char message[4608];
    std::snprintf(message,
                  sizeof(message),
                  "%s: Python import probe returned %d\n%s\nroot=%s",
                  result == 0 ? "OK" : "FAILED",
                  result,
                  probeMessage,
                  root.c_str());

    __android_log_print(result == 0 ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR,
                        LOG_TAG,
                        "%s",
                        message);

    return env->NewStringUTF(message);
}

extern "C" JNIEXPORT jstring JNICALL
Java_org_krita_android_pythonruntimeprobe_MainActivity_runImportOneProbe(JNIEnv *env,
                                                                         jclass,
                                                                         jstring runtimeRoot,
                                                                         jstring moduleName)
{
    const std::string root = toString(env, runtimeRoot);
    if (root.empty()) {
        return env->NewStringUTF("FAILED: runtime root is empty");
    }

    const std::string module = toString(env, moduleName);
    if (module.empty()) {
        return env->NewStringUTF("FAILED: module name is empty");
    }

    char probeMessage[4096] = {};
    const int result = krita_android_python_runtime_import_one_probe(root.c_str(),
                                                                     module.c_str(),
                                                                     probeMessage,
                                                                     static_cast<int>(sizeof(probeMessage)));

    char message[4608];
    std::snprintf(message,
                  sizeof(message),
                  "%s: Python import-one probe returned %d\n%s\nmodule=%s\nroot=%s",
                  result == 0 ? "OK" : "FAILED",
                  result,
                  probeMessage,
                  module.c_str(),
                  root.c_str());

    __android_log_print(result == 0 ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR,
                        LOG_TAG,
                        "%s",
                        message);

    return env->NewStringUTF(message);
}
