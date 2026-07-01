#include <Python.h>

#include <android/log.h>

#include <string>

namespace
{
constexpr const char *LOG_TAG = "KritaPythonRuntime";

int setConfigString(PyConfig *config, wchar_t **field, const std::string &value)
{
    wchar_t *wideValue = Py_DecodeLocale(value.c_str(), nullptr);
    if (!wideValue) {
        return -1;
    }

    const PyStatus status = PyConfig_SetString(config, field, wideValue);
    PyMem_RawFree(wideValue);

    return PyStatus_Exception(status) ? -2 : 0;
}

int appendModulePath(PyConfig *config, const std::string &path)
{
    wchar_t *widePath = Py_DecodeLocale(path.c_str(), nullptr);
    if (!widePath) {
        return -1;
    }

    const PyStatus status = PyWideStringList_Append(&config->module_search_paths, widePath);
    PyMem_RawFree(widePath);

    return PyStatus_Exception(status) ? -2 : 0;
}

int initializePythonFromPayload(const char *runtimeRoot)
{
    const std::string root = runtimeRoot && runtimeRoot[0] ? runtimeRoot : ".";
    const std::string pythonRoot = root + "/assets/python";
    const std::string stdlibRoot = pythonRoot + "/lib/python3.14";
    const std::string kritaPythonLibs = pythonRoot + "/krita-python-libs";

    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    config.isolated = 1;
    config.use_environment = 0;
    config.site_import = 1;
    config.write_bytecode = 0;
    config.module_search_paths_set = 1;

    int result = setConfigString(&config, &config.program_name, "krita-android");
    if (result == 0) {
        result = setConfigString(&config, &config.home, pythonRoot);
    }
    if (result == 0) {
        result = appendModulePath(&config, stdlibRoot);
    }
    if (result == 0) {
        result = appendModulePath(&config, stdlibRoot + "/lib-dynload");
    }
    if (result == 0) {
        result = appendModulePath(&config, stdlibRoot + "/site-packages");
    }
    if (result == 0) {
        result = appendModulePath(&config, kritaPythonLibs);
    }
    if (result == 0) {
        result = appendModulePath(&config, kritaPythonLibs + "/PyKrita");
    }

    if (result != 0) {
        PyConfig_Clear(&config);
        return result;
    }

    const PyStatus status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status)) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Py_InitializeFromConfig failed");
        return -10;
    }

    PyObject *sysModule = PyImport_ImportModule("sys");
    if (!sysModule) {
        PyErr_Print();
        Py_FinalizeEx();
        return -11;
    }
    Py_DECREF(sysModule);

    const int finalizeResult = Py_FinalizeEx();
    if (finalizeResult != 0) {
        return -12;
    }

    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Python runtime initialized from payload paths");
    return 0;
}
} // namespace

extern "C" __attribute__((visibility("default"))) int krita_android_python_runtime_init_probe(const char *runtimeRoot)
{
    return initializePythonFromPayload(runtimeRoot);
}
