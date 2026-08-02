"""Probe-only fallback for Krita's built-in pykrita helper module.

The real desktop module is registered by the Krita PyKrita plugin.  The Android
runtime probe imports the public ``krita`` package outside a full Krita app
startup, so this tiny Python module provides the two helper functions used by
the package bootstrap.
"""

from __future__ import annotations

import sys


def qt_major_version():
    return 5


def qDebug(text):
    print("PYKRITA: {}".format(text), file=sys.stderr)
