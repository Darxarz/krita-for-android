"""Probe the public Krita package after a Qt core application is active."""

from __future__ import annotations

from krita_probe_qcoreapplication import APP, PROBE_RESULT as QCOREAPPLICATION_RESULT

import krita


PROBE_RESULT = "OK: QCoreApplication then import krita; {}".format(QCOREAPPLICATION_RESULT)
