"""Probe-only creation of a Qt application before Krita singleton access.

The public ``krita`` package normally runs after Krita has started its Qt
application.  This helper isolates the smaller question of whether a real
``QApplication`` alone is enough for that package's eager bootstrap.
"""

from __future__ import annotations

import os


def _configure_android_platform_plugin():
    assets_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    plugins_dir = os.path.join(assets_dir, "qt", "plugins")
    platform_dir = os.path.join(plugins_dir, "platforms")
    candidates = [
        os.path.join(platform_dir, name)
        for name in sorted(os.listdir(platform_dir))
        if name.startswith("libplugins_platforms_qtforandroid_") and name.endswith(".so")
    ] if os.path.isdir(platform_dir) else []
    if len(candidates) != 1 or not os.path.isfile(candidates[0]):
        raise RuntimeError("expected one Android Qt platform plugin in {}".format(platform_dir))

    os.environ["QT_PLUGIN_PATH"] = plugins_dir
    os.environ["QT_QPA_PLATFORM_PLUGIN_PATH"] = platform_dir
    os.environ["QT_DEBUG_PLUGINS"] = "1"
    return candidates[0], plugins_dir


PLATFORM_PLUGIN, PLUGINS_DIR = _configure_android_platform_plugin()

from PyQt5.QtCore import QCoreApplication
from PyQt5.QtWidgets import QApplication


QCoreApplication.setLibraryPaths([PLUGINS_DIR])
os.environ["QT_QPA_PLATFORM"] = "android"

APP = QApplication.instance()
CREATED = APP is None
if APP is None:
    APP = QApplication(["krita-android-qapplication-probe"])

PROBE_RESULT = "OK: QApplication active {} type={} qpa_plugin={}".format(
    "(created)" if CREATED else "(reused)",
    type(APP).__name__,
    os.path.basename(PLATFORM_PLUGIN),
)
