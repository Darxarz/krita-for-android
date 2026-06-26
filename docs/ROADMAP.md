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

Следующий слой: `ext_pyqt5-sip`. Добавлен patch-кандидат
`0004-build-pyqt5-sip-for-android.patch`, который не запускает host `pip install` для
target module, а собирает `PyQt5/sip.cpython-314-<triplet>.so` через NDK CMake sub-build
и устанавливает `sip.h` в Android Python include-prefix.

CI `Krita deps PyQt5 SIP Android` зелёный.

Run: https://github.com/Darxarz/krita-for-android/actions/runs/28192903905

Проверено: артефакты `krita-deps-pyqt5-sip-arm64-v8a` и
`krita-deps-pyqt5-sip-x86_64` содержат target Android extension module
`PyQt5/sip.cpython-314-<triplet>.so`; workflow проверяет ELF machine type и зависимость
от `libpython3.14.so`.

Следующий слой: минимальный `ext_pyqt5` для Android. Добавлен patch-кандидат
`0005-build-minimal-pyqt5-for-android.patch`, который вводит Android-only recipe для
`ext_pyqt5`: `sip-build` получает target Python 3.14 настройки через `pyproject.toml`,
использует Android `qmake`, явно включает только `QtCore`, `QtNetwork`, `QtGui` и
`QtXml` и `QtWidgets`, а после установки переименовывает host-style extension suffix в Android
suffix `cpython-314-<triplet>`.

Добавлен CI probe `Krita deps PyQt5 minimal Android`. Для скорости он подкладывает готовый
Qt for Android через `aqtinstall`, а затем собирает уже проверенные Python/SIP слои и
пробует минимальный PyQt5 runtime. Это проверка сборочного рецепта; полноценная интеграция
с upstream `ext_qt` остается отдельным шагом.

CI `Krita deps PyQt5 minimal Android` зелёный. После PyKrita SIP import analysis в
минимальный набор добавлен `QtXml`, потому что upstream `kritamod.sip` импортирует
`QtXml/QtXmlmod.sip`.

Run: https://github.com/Darxarz/krita-for-android/actions/runs/28239107004

Проверено: артефакты `krita-deps-pyqt5-minimal-arm64-v8a` и
`krita-deps-pyqt5-minimal-x86_64` содержат Android target modules
`PyQt5/sip.cpython-314-<triplet>.so`, `QtCore`, `QtNetwork`, `QtGui`, `QtXml` и `QtWidgets`.
Workflow проверяет ELF machine type, зависимость от `libpython3.14.so` и наличие SIP
metadata `PyQt5/bindings/QtCore/QtCore.toml`.

Нужно разделить две роли Python:

- host Python: запускается на Linux runner и генерирует SIP/PyQt metadata;
- target Python: Android `libpython`, headers и extension modules, с которыми линкуется
  Krita и которые попадут в APK.

Критерий готовности выполнен для минимального PyQt5 runtime: Android deps содержат PyQt5
runtime modules и SIP metadata достаточно полные, чтобы перейти к сборке
`plugins/extensions/pykrita/sip`.

## Phase 3 - PyKrita в Android-сборке Krita

Статус: начат patch-кандидат для upstream Krita.

Нужно поправить CMake discovery:

- `FindPythonLibrary.cmake` должен принимать target include/lib без target interpreter;
- `FindPyQt5.cmake` не должен пытаться импортировать target Android PyQt на host runner;
- `plugins/extensions/pykrita` должен собираться для Android только в экспериментальном
  режиме, например через `-DENABLE_ANDROID_PYKRITA_EXPERIMENTAL=ON`.

Добавлен patch-кандидат `patches/krita/0001-enable-android-pykrita-discovery.patch`.
Он вводит Android-only option `ENABLE_ANDROID_PYKRITA_EXPERIMENTAL` и ручной discovery
target Python/PyQt5 из Android dependency prefix. Host Python по-прежнему используется
только для запуска SIP tooling, а target Android `PyQt5.QtCore` не импортируется на host.

Добавлен CI probe `Krita PyKrita discovery Android`. Он применяет Krita patch к upstream
CMake-файлам и проверяет на fake Android prefix, что `PythonLibrary`, `SIP` и `PyQt5`
находят target include/lib/site-packages/SIP metadata без запуска target extension modules.

CI `Krita PyKrita discovery Android` зелёный.

Run: https://github.com/Darxarz/krita-for-android/actions/runs/28259537561

Следующий узкий слой: генерация SIP C++ для `PyKrita.krita` без компиляции всего модуля.
Добавлен patch-кандидат `patches/krita/0002-add-android-pykrita-sip-generate-only.patch`,
который вводит Android-only флаг `KRITA_ANDROID_PYKRITA_GENERATE_ONLY`. В этом режиме
`SIPMacros.cmake` создаёт custom target генерации и не переходит к линковке
`PyKrita.krita` с `kritalibkis`, `kritaui` и остальными Krita libraries.

Добавлен CI probe `Krita PyKrita SIP generate Android`. Он берёт upstream Krita sparse
checkout, применяет Krita patch-серию, устанавливает host `sip`/`PyQt5`, копирует PyQt5
SIP bindings в fake Android prefix и запускает target
`python_module_PyKrita_krita_sip_generate`.

CI `Krita PyKrita SIP generate Android` зелёный.

Run: https://github.com/Darxarz/krita-for-android/actions/runs/28259537548

Следующий узкий слой: компиляция generated SIP C++ object files Android clang'ом без
линковки финального Python module. Добавлен patch-кандидат
`patches/krita/0003-add-android-pykrita-sip-compile-only.patch`, который вводит
Android-only флаг `KRITA_ANDROID_PYKRITA_COMPILE_ONLY`. В этом режиме
`SIPMacros.cmake` создаёт object library
`python_module_PyKrita_krita_sip_objects`, подключает target Python headers,
PyQt/Krita usage requirements, `SIP_PROTECTED_IS_PUBLIC` и Android-safe
`-fno-operator-names`.

Добавлен CI probe `Krita PyKrita SIP compile Android`. Он использует уже зелёный
артефакт `krita-deps-pyqt5-minimal-${abi}`, применяет Krita patch-серию, генерирует
SIP sources и компилирует `sipkritapart0.cpp` для `arm64-v8a` и `x86_64`.

CI `Krita PyKrita SIP compile Android` зелёный.

Run: https://github.com/Darxarz/krita-for-android/actions/runs/28261664383

Проверено: Android clang компилирует generated PyKrita SIP C++ против Python 3.14,
PyQt5 (`QtCore`, `QtGui`, `QtXml`, `QtWidgets`) и заголовков Krita. Для narrow probe
в workflow временно добавлены header-only/stub include surfaces для Boost, Eigen,
Krita generated config/export headers и KDE `KLocalizedString`.

Следующий слой: переход от object compile к настоящей линковке `PyKrita.krita`.
Для этого уже недостаточно SIP/PyQt; нужны Android-built Krita libraries
(`kritalibkis`, `kritaui`, `kritaimage`, `kritapigment` и зависимости), чтобы
убрать compile-probe stubs и собрать реальный loadable Python extension.

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
