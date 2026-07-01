#include <jni.h>

#include <android/log.h>

#include <cstdio>
#include <string>

extern "C" int krita_android_python_runtime_init_probe(const char *runtimeRoot);

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

    const int result = krita_android_python_runtime_init_probe(root.c_str());

    char message[256];
    std::snprintf(message,
                  sizeof(message),
                  "%s: PyConfig init probe returned %d for %s",
                  result == 0 ? "OK" : "FAILED",
                  result,
                  root.c_str());

    __android_log_print(result == 0 ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR,
                        LOG_TAG,
                        "%s",
                        message);

    return env->NewStringUTF(message);
}
