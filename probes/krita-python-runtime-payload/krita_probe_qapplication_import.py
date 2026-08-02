"""Probe the public Krita package after a Qt application is active."""

from __future__ import annotations

from krita_probe_qapplication import APP, PROBE_RESULT as QAPPLICATION_RESULT

import krita


PROBE_RESULT = "OK: QApplication then import krita; {}".format(QAPPLICATION_RESULT)
