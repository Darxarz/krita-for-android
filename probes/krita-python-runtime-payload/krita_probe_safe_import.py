"""Probe-only safe bootstrap for Krita's Python package.

This imports the real Krita Python package body while skipping only the eager
Krita.instance() aliases that require a fully started Krita application.
"""

from __future__ import annotations

import os
import sys
import types

PROBE_RESULT = "not-started"


def _package_dir():
    return os.path.join(os.path.dirname(__file__), "krita")


def _patched_source(source):
    replacements = {
        "builtins.Scripter = Krita.instance()": "builtins.Scripter = None  # probe skipped Krita.instance()",
        "builtins.Application = Krita.instance()": "builtins.Application = None  # probe skipped Krita.instance()",
        "builtins.Krita = Krita.instance()": "builtins.Krita = Krita  # probe kept the API class, not the singleton",
    }
    for old, new in replacements.items():
        if old not in source:
            raise RuntimeError("missing expected krita.__init__ line: {}".format(old))
        source = source.replace(old, new)
    return source + "\n_probe_safe_bootstrap_ok = True\n"


def _bootstrap():
    package_dir = _package_dir()
    init_path = os.path.join(package_dir, "__init__.py")
    if not os.path.isfile(init_path):
        raise RuntimeError("missing krita package __init__.py at {}".format(init_path))

    with open(init_path, "r", encoding="utf-8") as handle:
        source = handle.read()

    module = types.ModuleType("krita")
    module.__file__ = init_path
    module.__loader__ = None
    module.__package__ = "krita"
    module.__path__ = [package_dir]

    previous = sys.modules.get("krita")
    sys.modules["krita"] = module
    try:
        exec(compile(_patched_source(source), init_path, "exec"), module.__dict__)
    except BaseException as error:
        if previous is None:
            sys.modules.pop("krita", None)
        else:
            sys.modules["krita"] = previous
        raise RuntimeError(
            "safe krita package bootstrap failed: {}: {}".format(type(error).__name__, error)
        ) from error

    for attribute in ("Krita", "DockWidgetFactory", "pykritaEventHandler"):
        if not hasattr(module, attribute):
            raise RuntimeError("safe krita package bootstrap missing {}".format(attribute))

    return module


_module = _bootstrap()
PROBE_RESULT = "OK: safe krita package bootstrap without Krita.instance"
