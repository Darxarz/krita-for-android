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

Следующий узкий слой: конфигурация родительского `plugins/extensions/pykrita` без
desktop-only `kritarunner`. Добавлен patch-кандидат
`patches/krita/0004-skip-kritarunner-for-android-pykrita.patch`: в Android experimental
режиме остаются `sip` и embedded `plugin`, но не создаётся отдельный runner executable.

Добавлен CI probe `Krita PyKrita top-level configure Android`. Он проходит через
родительский `plugins/extensions/pykrita/CMakeLists.txt` с fake Android Python/PyQt
prefix и проверяет, что targets `python_module_PyKrita_krita_sip_generate`, `pykrita`
и `kritapykrita` создаются, а `kritarunner` пропускается.

CI `Krita PyKrita top-level configure Android` зелёный.

Run: https://github.com/Darxarz/krita-for-android/actions/runs/28262441987

Следующий узкий слой: link smoke для обычного target `PyKrita.krita` без compile-only
режима. Добавлен CI probe `Krita PyKrita SIP link smoke Android`. Он берёт зелёный
`krita-deps-pyqt5-minimal-${abi}` artifact, применяет Krita patch-серию, собирает
нормальный `python_module_PyKrita_krita` target и проверяет Android ELF `krita.so`.

CI `Krita PyKrita SIP link smoke Android` зелёный.

Run: https://github.com/Darxarz/krita-for-android/actions/runs/28264084186

Проверено: `krita.so` собирается для `arm64-v8a` и `x86_64`, имеет правильный ELF
machine type и NEEDED-зависимости на `libpython3.14.so`, `libQt5Core_${abi}.so`,
`libQt5Gui_${abi}.so`, `libQt5Xml_${abi}.so` и `libQt5Widgets_${abi}.so`.

Важно: это smoke-пробник, а не финальная runtime-сборка. В workflow пока остаются
header-only/stub include surfaces и fake interface targets для Krita libraries; Android
`--no-undefined` отключён только для этого smoke, поэтому unresolved Krita symbols
ожидаемы до появления настоящих Android-built Krita shared libraries.

Следующий узкий слой: инвентаризация настоящей link surface перед real-link сборкой.
Добавлен CI probe `Krita PyKrita link inventory`. Он берёт sparse checkout upstream
Krita, применяет patch-серию и читает CMake-файлы `PyKrita.krita`, `kritalibkis`,
`kritaui`, `kritaimage` и `kritalibbrush`, чтобы получить Android Qt5 минимум без
полной сборки Krita.

Проверено локально: для первого real-link шага нужны Krita libraries
`kritalibkis`, `kritaui`, `kritaimage`, `kritalibbrush`, `kritacolor`,
`kritacommand`, `kritaglobal`, `kritaimpex`, `kritametadata`, `kritamultiarch`,
`kritapigment`, `kritapsdutils`, `kritaresources`, `kritaversion`,
`kritawidgets` и `kritawidgetutils`, плюс внешняя поверхность Qt5/KF5, PNG,
Eigen, Boost, FFTW, GLESv3 и lager.

CI `Krita PyKrita link inventory` зелёный.

Run: https://github.com/Darxarz/krita-for-android/actions/runs/28293224421

Следующий узкий слой: первые настоящие Krita shared libraries для Android. Добавлен
CI probe `Krita real libs seed Android`. Он берёт уже зелёный Android PyQt5 prefix,
импортирует Android Qt5 Core/Gui/Widgets/Xml/Sql/PrintSupport/AndroidExtras и напрямую собирает upstream
`libs/version/CMakeLists.txt` как реальную `libkritaversion.so`, а также upstream
`libs/global/CMakeLists.txt` как реальную `libkritaglobal.so`.

Для `kritaglobal` probe пока подставляет минимальную generated surface для внешних
зависимостей (`KF5::I18n`, `KF5::ConfigCore`, Boost/Eigen/lager/zug/GSL/unwindstack),
но сама библиотека собирается из upstream `libs/global` и линкуется с real Android
`libkritaversion.so`.

Следующий низовой слой тоже добавлен в этот probe: upstream `libs/koplugin`,
`libs/multiarch` и `libs/color` собираются как реальные Android shared libraries
`libkritaplugin.so`, `libkritamultiarch.so` и `libkritacolor.so`. Для этого seed
добавляет минимальную generated surface для `KF5::CoreAddons/KPluginFactory` и
header-only `xsimd`, а upstream source по-прежнему берётся напрямую из sparse checkout.

Следующий storage-слой добавлен туда же: upstream `libs/store` собирается как реальная
Android shared library `libkritastore.so`. Для него seed пока подставляет минимальную
header-only QuaZip surface (`quazip.h`, `quazipfile.h`, `quazipdir.h`,
`quazipnewinfo.h`), но сама `kritastore` собирается из upstream `libs/store` и линкуется
с real `kritaglobal`.

CI проверяет обе ABI, ELF machine type, NEEDED-зависимости на Android Qt5 libraries,
NEEDED-зависимости новых Krita libraries на уже собранные seed libraries и экспортированные
symbols `KritaVersionWrapper::versionString`, `KisUsageLogger::initialize`,
`KoPluginLoader`, `vectorizationConfiguration`, `KisColorManager` и `KoStore`.

CI `Krita real libs seed Android` зелёный.

Run: https://github.com/Darxarz/krita-for-android/actions/runs/28298474948

Следующий dependency cluster тоже закрыт в этом probe: upstream `libs/resources`,
`libs/widgetutils` и `libs/command` собираются как реальные Android shared libraries
`libkritaresources.so`, `libkritawidgetutils.so` и `libkritacommand.so`. Для этого seed
добавляет минимальную generated surface для оставшихся KF5/XMLGUI/lager/QuaZip API, но
исходники трёх библиотек берутся напрямую из upstream Krita.

CI проверяет обе ABI, ELF machine type, NEEDED-зависимости новых библиотек на Android Qt5
и предыдущие real seed libraries, а также exported symbols `KisResourceLocator`,
`KisActionRegistry` и `KUndo2Stack`.

CI `Krita real libs seed Android` зелёный для этого слоя.

Run: https://github.com/Darxarz/krita-for-android/actions/runs/28301018937

Следующий слой real-library цепочки тоже продвинут: upstream `libs/pigment` собирается как
реальная Android shared library `libkritapigment.so`. Seed добавляет per-arch xsimd
копии исходников (`NEON64` на `arm64-v8a`; `SSE2`, `SSSE3`, `SSE4_1`, `AVX`,
`AVX2+FMA` на `x86_64`), включает Krita source root для `KoAlwaysInline.h`, отключает
tests/benchmarks и собирает `kritapigment` с `-fno-operator-names`, как это требуется для
старых Krita-имен методов `xor`/`and`/`or`.

CI проверяет обе ABI, ELF machine type, NEEDED-зависимости `libkritapigment.so` на Android
Qt5 и предыдущие real seed libraries, а также exported symbol `KoColorSpaceRegistry`.

CI `Krita real libs seed Android` зелёный для этого слоя.

Run: https://github.com/Darxarz/krita-for-android/actions/runs/28302430361

Еще один compact prerequisite для `kritaimage` закрыт: upstream `libs/metadata`
собирается как реальная Android shared library `libkritametadata.so`. Seed отключает
tests, подключает upstream target напрямую и проверяет, что библиотека линкуется с уже
собранными real seed libraries.

CI проверяет обе ABI, ELF machine type, NEEDED-зависимости `libkritametadata.so` на
`libkritaglobal.so`, `libkritaplugin.so`, `libkritawidgetutils.so` и Android Qt5 Core, а
также exported symbols из namespace `KisMetaData`.

CI `Krita real libs seed Android` зелёный для этого слоя.

Run: https://github.com/Darxarz/krita-for-android/actions/runs/28303022560

Следующий слой: продолжить real-library цепочку к зависимостям `PyKrita.krita`, начиная с
оставшихся prerequisites для `kritaimage` (`kritawidgets`, `kritapsdutils`), затем
`kritaimage`, `kritalibbrush` и дальше к `kritalibkis` -> `kritaui` -> `PyKrita.krita`.

Следующий dependency cluster закрыт в этом probe: upstream `libs/resourcewidgets`,
`libs/widgets` и `libs/psdutils` собираются как реальные Android shared libraries
`libkritaresourcewidgets.so`, `libkritawidgets.so` и `libkritapsdutils.so`. Для
`kritawidgets` seed добавляет минимальные KDE widget/config shims и точечные generated
UI headers для `KoConfigAuthorPage`, `wdg_file_name_requester`, `WdgDlgInternalColorSelector`
и `WdgPaletteListWidget`; upstream source остается прямым sparse checkout.

CI `Krita real libs seed Android` зеленый для этого слоя.

Run: https://github.com/Darxarz/krita-for-android/actions/runs/28312002206

Следующие PyKrita prerequisites закрыты в том же real-libs seed:

- upstream `libs/image` собирается как `libkritaimage.so`;
- upstream `libs/brush` собирается как `libkritalibbrush.so`;
- upstream `libs/impex` собирается как `libkritaimpex.so`;
- upstream `libs/ui` собирается как `libkritaui.so`;
- upstream `libs/libkis` собирается как `libkritalibkis.so`.

Для этих слоёв seed добавил минимальные Android/KDE/FFmpeg/SeExpr shims, но сами
библиотеки берутся из upstream Krita. CI проверяет обе ABI, ELF machine type,
NEEDED-зависимости и ключевые exported symbols.

Зелёные runs:

- `kritaimage`: https://github.com/Darxarz/krita-for-android/actions/runs/28323732554
- `kritalibbrush`: https://github.com/Darxarz/krita-for-android/actions/runs/28325085418
- `kritaimpex`: https://github.com/Darxarz/krita-for-android/actions/runs/28325491346
- `kritaui`: https://github.com/Darxarz/krita-for-android/actions/runs/28479640404
- `kritalibkis`: https://github.com/Darxarz/krita-for-android/actions/runs/28481920223
- `PyKrita.krita`: https://github.com/Darxarz/krita-for-android/actions/runs/28483079006
- `kritapykrita`: https://github.com/Darxarz/krita-for-android/actions/runs/28484876691

Настоящий Android `PyKrita.krita` (`krita.so`) теперь собирается против уже зелёных
real seed libraries вместо старого interface-only link smoke. Plugin wrapper
`kritapykrita`, который инициализирует embedded Python plugin manager внутри Krita,
тоже собирается и проверяется на обеих ABI.

Критерий готовности: `kritapykrita` и `PyKrita.krita` собираются в Android build tree.

## Phase 4 - APK packaging and runtime init

Нужно упаковать:

- `libpython3.14.so` и companion libraries как JNI/native libs;
- Python standard library и site-packages в APK assets или app private storage;
- `krita-ai-diffusion` plugin files.

Первый packaging слой начат: `scripts/stage-krita-python-runtime-payload.sh` и CI
`Krita Python runtime payload Android` скачивают зелёные PyQt/Python и real-libs
artifacts, затем раскладывают их в APK-похожую структуру `jniLibs/<abi>` +
`assets/python/...`. Это ещё не APK и ещё без `krita-ai-diffusion`, но это проверяемый
runtime payload для следующего слоя `PyConfig`/Android packaging.

CI `Krita Python runtime payload Android` зелёный для обеих ABI.

Run: https://github.com/Darxarz/krita-for-android/actions/runs/28485870841

Нужно инициализировать Python на Android через modern `PyConfig`, а не через запуск
внешнего `python` executable.

Следующий init layer начат: `probes/krita-python-runtime-init/` собирает Android shared
library, которая на runtime выставляет `PyConfig.home`, `module_search_paths` для staged
payload и вызывает `Py_InitializeFromConfig()`. CI проверяет compile/link,
ELF machine type, NEEDED `libpython3.14.so` и exported probe symbol.

CI `Krita Python runtime init probe Android` зелёный для обеих ABI.

Run: https://github.com/Darxarz/krita-for-android/actions/runs/28486088495

Следующий packaging layer начат: нужно подключить staged payload и init probe к
настоящему Android APK-контейнеру, а затем уже переходить к запуску в app process и
первому тесту на планшете.

CI `Krita Python runtime APK probe Android` зелёный для обеих ABI. Этот layer собирает
минимальный подписанный APK-контейнер с `lib/<abi>` и `assets/python`, проверяет APK
signature, entries и ELF machine type для ключевых native libraries.

Run: https://github.com/Darxarz/krita-for-android/actions/runs/28486382851

Следующий device-facing layer тоже добавлен: `Krita Python runtime launch APK probe
Android` собирает launchable APK с `MainActivity`, Java-copy `assets/python` в
app-private storage, JNI launcher и вызовом `krita_android_python_runtime_init_probe`.
CI проверяет сборку launcher `.so`, NEEDED `libkrita_python_runtime_init_probe.so`,
наличие JNI symbol, `classes.dex`, APK signature и ключевые native/assets entries.

Run: https://github.com/Darxarz/krita-for-android/actions/runs/28486651957

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
