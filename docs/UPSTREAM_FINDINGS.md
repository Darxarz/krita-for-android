# Upstream Findings

Дата проверки: 2026-06-25.

Проверенные upstream-репозитории:

- Krita: `https://invent.kde.org/graphics/krita.git`, `master`,
  commit `2bd9710f0c415ae5558df26eb89026134c8608a1`.
- Krita deps management: `https://invent.kde.org/packaging/krita-deps-management.git`.
- AI Diffusion plugin: `https://github.com/Acly/krita-ai-diffusion.git`,
  `main`, commit `657e79c`.

## Что блокирует Python-плагины в Krita Android

В Krita сам `pykrita` есть в исходниках. Он собирается только если одновременно доступны:

- Python development library.
- SIP.
- PyQt для текущей версии Qt.

Это видно в `plugins/extensions/pykrita/CMakeLists.txt`:

```cmake
if (HAVE_PYQT${QT_MAJOR_VERSION} AND HAVE_SIP AND HAVE_PYTHONLIBS)
    add_subdirectory(sip)
    add_subdirectory(plugin)
    add_subdirectory(kritarunner)
endif ()
```

В `krita-deps-management/latest/krita-deps.yml` Python/SIP/PyQt сейчас подключаются
только для `Windows`, `MacOS` и `Linux`, но не для `Android`.

В `krita-deps-management/ext_python/CMakeLists.txt` Android-ветка прямо останавливает
сборку:

```cmake
if (ANDROID)
    message (FATAL_ERROR "TODO: the build of Python on Android is not implemented! It should use --with-openssl=${EXTPREFIX} to use proper openssl")
endif()
```

Значит, реальная точка входа - dependency recipe, а не UI Krita.

## Что поменялось в пользу проекта

Python 3.14 уже публикует официальные Android tarball-пакеты:

- `python-3.14.0-aarch64-linux-android.tar.gz`
- `python-3.14.0-x86_64-linux-android.tar.gz`

Внутри пакета есть `prefix/include/python3.14`, `prefix/lib/libpython3.14.so`,
`prefix/lib/libpython3.so`, стандартная библиотека, `lib-dynload`, OpenSSL и SQLite
companion libraries.

Официальная документация Python для Android описывает именно embedded mode: приложение
упаковывает `libpython`, стандартную библиотеку и свой Python-код внутрь APK и запускает
Python через embedding API.

Документация:

- https://docs.python.org/3/using/android.html
- https://peps.python.org/pep-0738/

## Что известно про krita-ai-diffusion

В `requirements.txt` плагина прямо сказано, что сам плагин работает внутри embedded Python
Krita и в runtime имеет доступ только к стандартной библиотеке Python и Qt5. Остальные
зависимости из `requirements.txt` относятся к разработке, тестам или backend service.

Это хорошая новость: первый Android MVP можно нацелить на удаленный ComfyUI/Interstice,
а не пытаться запускать локальную нейросеть на устройстве.

## Главный риск

Официальный Android Python решает только слой `libpython`.

Следующие сложные слои:

- собрать или упаковать Android-compatible PyQt5;
- получить SIP metadata и target Python extension modules для `PyKrita`;
- научить `FindPythonLibrary.cmake` не путать host Python, который запускается при сборке,
  с target Android Python, с которым линкуется Krita;
- упаковать Python standard library и plugin files в APK assets;
- при запуске Krita Android выставить пути Python через `PyConfig`/`PYTHONHOME`-аналог.

## Текущая стратегия для PyQt5

Полная Android-сборка Qt из `ext_qt` тяжелая для быстрых проверок, поэтому следующий
proof-layer проверяет сам `ext_pyqt5` отдельно: CI ставит готовый Qt 5.15.2 for Android
через `aqtinstall`, кладет его в dependency prefix и запускает Android-only рецепт
`ext_pyqt5` поверх уже собранных `ext_python`, `ext_sip`, `ext_pyqt-builder` и
`ext_pyqt5-sip`.

CI probe `Krita deps PyQt5 minimal Android` теперь зелёный:
https://github.com/Darxarz/krita-for-android/actions/runs/28196914189

Проверенный минимальный набор PyQt5 для Android:

- `PyQt5/sip.cpython-314-<triplet>.so`;
- `PyQt5/QtCore.cpython-314-<triplet>.so`;
- `PyQt5/QtNetwork.cpython-314-<triplet>.so`;
- `PyQt5/QtGui.cpython-314-<triplet>.so`;
- `PyQt5/QtWidgets.cpython-314-<triplet>.so`;
- SIP metadata under `PyQt5/bindings`.

Во время сборки PyQt5 генерирует limited-API модули как `Qt*.abi3.so`; Android-only
install step переименовывает их в target suffix `cpython-314-<triplet>`, чтобы Krita CMake
и packaging layer могли искать обычные Android extension module names.

Это не заменяет будущую интеграцию с Krita `ext_qt`, но уже показывает, что qmake mkspec,
target Python metadata, suffix names, линковка к `libpython3.14.so` и минимальный набор Qt
modules работают для `arm64-v8a` и `x86_64`.
