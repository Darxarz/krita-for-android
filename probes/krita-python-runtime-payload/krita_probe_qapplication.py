"""Probe-only creation of a Qt application before Krita singleton access.

The public ``krita`` package normally runs after Krita has started its Qt
application.  This helper isolates the smaller question of whether a real
``QApplication`` alone is enough for that package's eager bootstrap.
"""

from __future__ import annotations

import os

from PyQt5.QtWidgets import QApplication


# Android is the platform expected by the packaged Qt runtime.  A missing Qt
# platform plugin remains a useful, explicit diagnostic from this child probe.
os.environ.setdefault("QT_QPA_PLATFORM", "android")

APP = QApplication.instance()
CREATED = APP is None
if APP is None:
    APP = QApplication(["krita-android-qapplication-probe"])

PROBE_RESULT = "OK: QApplication active {} type={}".format(
    "(created)" if CREATED else "(reused)", type(APP).__name__
)
