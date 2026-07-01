# Krita AI Diffusion on Android - port lab

Это рабочая лаборатория для портирования Python-плагинов Krita на Android, с фокусом на
`krita-ai-diffusion`.

Коротко и честно: готового APK здесь пока нет. Но я уже нашел реальный первый
технический узел и сделал для него воспроизводимый probe. Сейчас Android-сборка Krita
не включает Python-плагины не из-за одной выключенной галочки, а потому что для Android
нет цепочки `Python + SIP + PyQt + PyKrita` в зависимостях Krita.

## Что уже есть

- `probes/cpython-android-embed/` - минимальная Android C++ `.so`, которая линкуется с
  официальным `libpython3.14.so`.
- `scripts/download-python-android.ps1` - скачивает официальный Android Python release
  с python.org для `arm64-v8a` или `x86_64`.
- `scripts/build-python-embed-probe.ps1` - собирает probe через Android NDK.
- `.github/workflows/python-android-embed-probe.yml` - GitHub Actions проверка, что
  probe собирается в облаке.
- `patches/krita-deps-management/0001-ext-python-android-release-package.patch` -
  первый экспериментальный патч для dependency recipe Krita: заменить TODO/FATAL_ERROR
  в `ext_python` на установку официального Android Python package.
- `patches/krita-deps-management/0004-build-pyqt5-sip-for-android.patch` -
  экспериментальный Android cross-build для модуля `PyQt5.sip`.
- `.github/workflows/krita-deps-pyqt5-sip-android.yml` - проверяет, что `PyQt5.sip`
  собирается как Android extension module для `arm64-v8a` и `x86_64`.
- `.github/workflows/krita-real-libs-seed-android.yml` - собирает первые настоящие
  upstream-библиотеки Krita для Android: `libkritaversion.so`, `libkritaglobal.so`,
  `libkritaplugin.so`, `libkritamultiarch.so`, `libkritacolor.so` и
  `libkritastore.so`, `libkritaresources.so`, `libkritawidgetutils.so` и
  `libkritacommand.so`, `libkritapigment.so`, `libkritametadata.so`.
- `.github/workflows/krita-python-runtime-payload-android.yml` - первый packaging layer:
  собирает APK-похожий payload из Android Python/PyQt, real Krita libraries,
  `PyKrita.krita` и built-in PyKrita Python package.
- `.github/workflows/krita-python-runtime-init-probe-android.yml` - собирает Android
  `.so` с `PyConfig`-инициализацией Python из staged payload paths.
- `docs/UPSTREAM_FINDINGS.md` - что найдено в текущих upstream-исходниках.
- `docs/ROADMAP.md` - путь от этого probe до реального `krita-ai-diffusion` в Krita
  Android.

## Быстрая проверка локально

Нужны Android SDK/NDK и CMake. Если они уже стоят:

```powershell
.\scripts\download-python-android.ps1 -Abi arm64-v8a
.\scripts\build-python-embed-probe.ps1 -Abi arm64-v8a -AndroidNdkRoot "C:\path\to\Android\Sdk\ndk\27.3.13750724"
```

На GitHub достаточно запустить workflow `Python Android embed probe` вручную.

Этот probe пока не инициализирует интерпретатор. Он проверяет базовый link/load слой:
Android C++ code + Python headers + `libpython3.14.so`. Настройка `PyConfig` и путей к
stdlib относится к следующему этапу.

## Главная идея

Первый рабочий MVP должен быть не "Stable Diffusion целиком на планшете", а Krita Android
с Python-плагином, который подключается к удаленному или облачному ComfyUI/Interstice
backend. Это гораздо реалистичнее: сам `krita-ai-diffusion` уже рассчитан на работу
внутри встроенного Python Krita и в runtime в основном опирается на стандартную библиотеку
Python и Qt5.

Дальше самый тяжелый кусок - не сам Python, а Android-сборка SIP/PyQt и `PyKrita`.
