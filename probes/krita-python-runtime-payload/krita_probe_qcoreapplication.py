"""Probe-only Qt core application context for Krita's Python wrapper."""

from __future__ import annotations

import glob
import os

from PyQt5.QtCore import QCoreApplication


def _platform_plugin_report():
    assets_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    platform_dir = os.path.join(assets_dir, "qt", "plugins", "platforms")
    plugins = sorted(glob.glob(os.path.join(platform_dir, "libplugins_platforms_qtforandroid_*.so")))
    if not plugins:
        return "missing at {}".format(platform_dir)
    return "{} bytes={}".format(os.path.basename(plugins[0]), os.path.getsize(plugins[0]))


APP = QCoreApplication.instance()
CREATED = APP is None
if APP is None:
    APP = QCoreApplication(["krita-android-qcoreapplication-probe"])

PROBE_RESULT = "OK: QCoreApplication active {} type={} qpa_plugin={}".format(
    "(created)" if CREATED else "(reused)", type(APP).__name__, _platform_plugin_report()
)
