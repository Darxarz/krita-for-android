#include <jni.h>

#include <android/log.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <string>
#include <sys/wait.h>
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

enum class ChildProbeMode
{
    ImportOne,
    DlopenPyKrita,
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

int dlopenPyKritaExtension(const std::string &runtimeRoot, std::string *message)
{
    const std::string path = pyKritaExtensionPath(runtimeRoot);
    dlerror();
    void *handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        const char *error = dlerror();
        if (message) {
            *message = "FAILED: dlopen PyKrita.krita\npath=" + path + "\ndlerror="
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
            *message = "FAILED: dlsym PyInit_krita\npath=" + path + "\ndlerror="
                    + (symbolError ? symbolError : "symbol missing");
        }
        return -81;
    }

    dlclose(handle);
    if (message) {
        *message = "OK: dlopen PyKrita.krita and dlsym PyInit_krita\npath=" + path;
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

    const std::string label = mode == ChildProbeMode::DlopenPyKrita
            ? "child dlopen PyKrita.krita"
            : "child import " + moduleName;

    if (WIFSIGNALED(status)) {
        return "FAILED: " + label + " crashed\nsignal=" + std::to_string(WTERMSIG(status))
                + "\n" + childMessage;
    }

    if (WIFEXITED(status)) {
        const int exitCode = WEXITSTATUS(status);
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
