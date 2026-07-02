#include <Python.h>

#include <android/log.h>

#include <cstdio>
#include <string>
#include <vector>

namespace
{
constexpr const char *LOG_TAG = "KritaPythonRuntime";

struct RuntimePaths
{
    std::string root;
    std::string pythonRoot;
    std::string stdlibRoot;
    std::string kritaPythonLibs;
};

RuntimePaths makeRuntimePaths(const char *runtimeRoot)
{
    RuntimePaths paths;
    paths.root = runtimeRoot && runtimeRoot[0] ? runtimeRoot : ".";
    paths.pythonRoot = paths.root + "/assets/python";
    paths.stdlibRoot = paths.pythonRoot + "/lib/python3.14";
    paths.kritaPythonLibs = paths.pythonRoot + "/krita-python-libs";
    return paths;
}

void copyMessage(const std::string &message, char *buffer, int bufferSize)
{
    if (!buffer || bufferSize <= 0) {
        return;
    }

    std::snprintf(buffer, static_cast<size_t>(bufferSize), "%s", message.c_str());
}

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

std::string pythonObjectToUtf8(PyObject *object)
{
    if (!object) {
        return {};
    }

    PyObject *text = PyObject_Str(object);
    if (!text) {
        PyErr_Clear();
        return {};
    }

    const char *utf8 = PyUnicode_AsUTF8(text);
    std::string result = utf8 ? utf8 : "";
    Py_DECREF(text);
    return result;
}

int initializePythonWithPaths(const RuntimePaths &paths)
{
    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    config.isolated = 1;
    config.use_environment = 0;
    config.site_import = 1;
    config.write_bytecode = 0;
    config.module_search_paths_set = 1;

    int result = setConfigString(&config, &config.program_name, "krita-android");
    if (result == 0) {
        result = setConfigString(&config, &config.home, paths.pythonRoot);
    }
    if (result == 0) {
        result = appendModulePath(&config, paths.stdlibRoot);
    }
    if (result == 0) {
        result = appendModulePath(&config, paths.stdlibRoot + "/lib-dynload");
    }
    if (result == 0) {
        result = appendModulePath(&config, paths.stdlibRoot + "/site-packages");
    }
    if (result == 0) {
        result = appendModulePath(&config, paths.kritaPythonLibs);
    }
    if (result == 0) {
        result = appendModulePath(&config, paths.kritaPythonLibs + "/PyKrita");
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

    return 0;
}

int initializePythonFromPayload(const char *runtimeRoot, std::string *message)
{
    const RuntimePaths paths = makeRuntimePaths(runtimeRoot);

    const int initResult = initializePythonWithPaths(paths);
    if (initResult != 0) {
        if (message) {
            *message = "FAILED: Py_InitializeFromConfig returned " + std::to_string(initResult);
        }
        return initResult;
    }

    PyObject *sysModule = PyImport_ImportModule("sys");
    if (!sysModule) {
        PyErr_Print();
        Py_FinalizeEx();
        if (message) {
            *message = "FAILED: import sys";
        }
        return -11;
    }

    PyObject *versionObject = PyObject_GetAttrString(sysModule, "version");
    const std::string version = pythonObjectToUtf8(versionObject);
    Py_XDECREF(versionObject);
    Py_DECREF(sysModule);

    const int finalizeResult = Py_FinalizeEx();
    if (finalizeResult != 0) {
        if (message) {
            *message = "FAILED: Py_FinalizeEx returned " + std::to_string(finalizeResult);
        }
        return -12;
    }

    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Python runtime initialized from payload paths");
    if (message) {
        *message = "OK: Python initialized; sys.version=" + version;
    }
    return 0;
}

int importPythonModulesFromPayload(const char *runtimeRoot, std::string *message)
{
    const RuntimePaths paths = makeRuntimePaths(runtimeRoot);

    const int initResult = initializePythonWithPaths(paths);
    if (initResult != 0) {
        if (message) {
            *message = "FAILED: Py_InitializeFromConfig returned " + std::to_string(initResult);
        }
        return initResult;
    }

    std::string report = "OK imports:";
    const std::vector<const char *> modules = {
        "sys",
        "PyQt5.QtCore",
        "PyKrita.krita",
        "krita",
    };

    int index = 0;
    for (const char *moduleName : modules) {
        PyObject *module = PyImport_ImportModule(moduleName);
        if (!module) {
            PyErr_Print();
            Py_FinalizeEx();
            if (message) {
                *message = "FAILED: import " + std::string(moduleName);
            }
            return -30 - index;
        }

        report += "\n";
        report += moduleName;
        report += " OK";

        if (std::string(moduleName) == "sys") {
            PyObject *versionObject = PyObject_GetAttrString(module, "version");
            const std::string version = pythonObjectToUtf8(versionObject);
            Py_XDECREF(versionObject);
            if (!version.empty()) {
                report += " ";
                report += version.substr(0, 32);
            }
        }

        Py_DECREF(module);
        ++index;
    }

    const int finalizeResult = Py_FinalizeEx();
    if (finalizeResult != 0) {
        if (message) {
            *message = "FAILED: Py_FinalizeEx returned " + std::to_string(finalizeResult);
        }
        return -40;
    }

    if (message) {
        *message = report;
    }

    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Python import probe completed");
    return 0;
}

int importOnePythonModuleFromPayload(const char *runtimeRoot, const char *moduleName, std::string *message)
{
    if (!moduleName || !moduleName[0]) {
        if (message) {
            *message = "FAILED: empty module name";
        }
        return -50;
    }

    const RuntimePaths paths = makeRuntimePaths(runtimeRoot);

    const int initResult = initializePythonWithPaths(paths);
    if (initResult != 0) {
        if (message) {
            *message = "FAILED: Py_InitializeFromConfig returned " + std::to_string(initResult);
        }
        return initResult;
    }

    PyObject *module = PyImport_ImportModule(moduleName);
    if (!module) {
        PyErr_Print();
        Py_FinalizeEx();
        if (message) {
            *message = "FAILED: import " + std::string(moduleName);
        }
        return -51;
    }

    Py_DECREF(module);

    const int finalizeResult = Py_FinalizeEx();
    if (finalizeResult != 0) {
        if (message) {
            *message = "FAILED: Py_FinalizeEx returned " + std::to_string(finalizeResult);
        }
        return -52;
    }

    if (message) {
        *message = "OK: import " + std::string(moduleName);
    }

    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Python single import probe completed for %s", moduleName);
    return 0;
}
} // namespace

extern "C" __attribute__((visibility("default"))) int krita_android_python_runtime_init_probe(const char *runtimeRoot)
{
    return initializePythonFromPayload(runtimeRoot, nullptr);
}

extern "C" __attribute__((visibility("default"))) int krita_android_python_runtime_init_probe_message(
    const char *runtimeRoot, char *messageBuffer, int messageBufferSize)
{
    std::string message;
    const int result = initializePythonFromPayload(runtimeRoot, &message);
    copyMessage(message, messageBuffer, messageBufferSize);
    return result;
}

extern "C" __attribute__((visibility("default"))) int krita_android_python_runtime_import_probe(
    const char *runtimeRoot, char *messageBuffer, int messageBufferSize)
{
    std::string message;
    const int result = importPythonModulesFromPayload(runtimeRoot, &message);
    copyMessage(message, messageBuffer, messageBufferSize);
    return result;
}

extern "C" __attribute__((visibility("default"))) int krita_android_python_runtime_import_one_probe(
    const char *runtimeRoot, const char *moduleName, char *messageBuffer, int messageBufferSize)
{
    std::string message;
    const int result = importOnePythonModuleFromPayload(runtimeRoot, moduleName, &message);
    copyMessage(message, messageBuffer, messageBufferSize);
    return result;
}
