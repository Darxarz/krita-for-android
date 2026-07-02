#include <jni.h>

#include <android/log.h>

#include <cstdio>
#include <string>

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
