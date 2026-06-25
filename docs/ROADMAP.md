# Roadmap

Цель: получить экспериментальную Krita Android, в которой запускается Python-плагин
`krita-ai-diffusion` и подключается к удаленному AI backend.

## Phase 0 - CPython Android probe

Статус: начато в этом репозитории.

Проверить минимальную базу: Android native library собирается и линкуется с официальным
`libpython3.14.so`.

Готово:

- download script для официального Python Android package;
- CMake probe;
- GitHub Actions workflow.

Критерий готовности: workflow собирает `libkrita_android_python_probe.so` для `arm64-v8a`
и `x86_64`.

## Phase 1 - ext_python для Krita deps

Взять `patches/krita-deps-management/0001-ext-python-android-release-package.patch` и
применить его в форке `krita-deps-management`.

Критерий готовности: `ext_python` перестает падать на Android и устанавливает:

- `include/python3.14`;
- `lib/libpython3.14.so`;
- `lib/python3.14`;
- companion `.so` из Python package.

На этом этапе `pykrita` еще не обязан собираться.

## Phase 2 - SIP/PyQt split для cross-build

Это самый тяжелый этап.

Нужно разделить две роли Python:

- host Python: запускается на Linux runner и генерирует SIP/PyQt metadata;
- target Python: Android `libpython`, headers и extension modules, с которыми линкуется
  Krita и которые попадут в APK.

Критерий готовности: Android deps содержат PyQt5 runtime modules и SIP metadata достаточно
полные, чтобы Krita смогла собрать `plugins/extensions/pykrita/sip`.

## Phase 3 - PyKrita в Android-сборке Krita

Нужно поправить CMake discovery:

- `FindPythonLibrary.cmake` должен принимать target include/lib без target interpreter;
- `FindPyQt5.cmake` не должен пытаться импортировать target Android PyQt на host runner;
- `plugins/extensions/pykrita` должен собираться для Android только в экспериментальном
  режиме, например через `-DENABLE_ANDROID_PYKRITA_EXPERIMENTAL=ON`.

Критерий готовности: `kritapykrita` и `PyKrita.krita` собираются в Android build tree.

## Phase 4 - APK packaging and runtime init

Нужно упаковать:

- `libpython3.14.so` и companion libraries как JNI/native libs;
- Python standard library и site-packages в APK assets или app private storage;
- `krita-ai-diffusion` plugin files.

Нужно инициализировать Python на Android через modern `PyConfig`, а не через запуск
внешнего `python` executable.

Критерий готовности: простой test plugin печатает версию Python в logcat и видит `krita`
module.

## Phase 5 - krita-ai-diffusion MVP

Первый MVP должен отключить все, что требует локального ComfyUI на планшете, и оставить:

- подключение к remote ComfyUI или Interstice;
- UI docker;
- отправку/получение изображений через Qt network/WebSocket stack;
- сохранение настроек в app-private storage.

Критерий готовности: на Android можно открыть документ в Krita, включить docker AI
Diffusion и получить результат от удаленного backend.
