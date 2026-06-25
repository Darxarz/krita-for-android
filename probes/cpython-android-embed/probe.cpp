#include <Python.h>

#include <android/log.h>

constexpr const char *LogTag = "KritaPythonProbe";

extern "C" __attribute__((visibility("default"))) int krita_android_python_probe()
{
    __android_log_print(
        ANDROID_LOG_INFO,
        LogTag,
        "Linked against Python: %s",
        Py_GetVersion());

    return PY_VERSION_HEX;
}
