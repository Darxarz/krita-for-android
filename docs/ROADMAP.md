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

Статус: зелёная проверка в GitHub Actions.

Run: https://github.com/Darxarz/krita-for-android/actions/runs/28191101042

Взять `patches/krita-deps-management/0001-ext-python-android-release-package.patch` и
применить его в форке `krita-deps-management`.

Критерий готовности выполнен: `ext_python` перестает падать на Android и устанавливает:

- `include/python3.14`;
- `lib/libpython3.14.so`;
- `lib/python3.14`;
- companion `.so` из Python package.

На этом этапе `pykrita` еще не обязан собираться. Дополнительно добавлен patch-кандидат,
который включает `base/python` в Android dependency seed.

## Phase 2 - SIP/PyQt split для cross-build

Это самый тяжелый этап.

Первый найденный blocker: `krita_initialize_python.cmake` в upstream deps ожидает
`lib/python3.13/site-packages`, а официальный Android Python package сейчас ставит
`lib/python3.14`. Добавлен patch-кандидат, который переключает этот путь на 3.14 только
для Android и добавляет host-version `site-packages` для build tools, установленных через
host `python3`.

CI `Krita deps Python tools Android` теперь зелёный.

Run: https://github.com/Darxarz/krita-for-android/actions/runs/28191619971

Проверено: `ext_python`, `ext_sip` и `ext_pyqt-builder` устанавливаются в один Android
dependency prefix для `arm64-v8a` и `x86_64`; `sipbuild` и `pyqtbuild` импортируются через
host Python, а target `libpython3.14.so` остаётся в этом же prefix.

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
