#!/usr/bin/env python3
"""Inventory the upstream Krita link surface needed by PyKrita.

The script intentionally does not configure or build Krita.  It reads the
upstream CMake files that define PyKrita and the first layer of libraries it
links to, then emits a JSON inventory that can drive the next Android real-link
workflows.
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


LINK_KEYWORDS = {
    "PUBLIC",
    "PRIVATE",
    "INTERFACE",
    "LINK_PUBLIC",
    "LINK_PRIVATE",
    "LINK_INTERFACE_LIBRARIES",
    "debug",
    "optimized",
    "general",
}

TARGET_FILES = {
    "pykrita_sip": "plugins/extensions/pykrita/sip/CMakeLists.txt",
    "kritalibkis": "libs/libkis/CMakeLists.txt",
    "kritalibbrush": "libs/brush/CMakeLists.txt",
    "kritaimage": "libs/image/CMakeLists.txt",
    "kritaui": "libs/ui/CMakeLists.txt",
}


def strip_cmake_comment(line: str) -> str:
    in_quote = False
    escaped = False

    for index, char in enumerate(line):
        if escaped:
            escaped = False
            continue

        if char == "\\":
            escaped = True
            continue

        if char == '"':
            in_quote = not in_quote
            continue

        if char == "#" and not in_quote:
            return line[:index]

    return line


def paren_balance(line: str) -> int:
    in_quote = False
    escaped = False
    balance = 0

    for char in line:
        if escaped:
            escaped = False
            continue

        if char == "\\":
            escaped = True
            continue

        if char == '"':
            in_quote = not in_quote
            continue

        if in_quote:
            continue

        if char == "(":
            balance += 1
        elif char == ")":
            balance -= 1

    return balance


def split_cmake_args(body: str) -> list[str]:
    args: list[str] = []
    current: list[str] = []
    in_quote = False
    escaped = False

    for char in body:
        if escaped:
            current.append(char)
            escaped = False
            continue

        if char == "\\":
            escaped = True
            continue

        if char == '"':
            in_quote = not in_quote
            continue

        if char.isspace() and not in_quote:
            if current:
                args.append("".join(current))
                current.clear()
            continue

        current.append(char)

    if current:
        args.append("".join(current))

    return args


def iter_cmake_commands(path: Path):
    active_command: list[str] = []
    active_name = ""
    active_start_line = 0
    balance = 0
    command_start = re.compile(r"^\s*([A-Za-z_][A-Za-z0-9_]*)\s*\(")

    for line_number, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = strip_cmake_comment(raw_line)

        if not active_command:
            match = command_start.match(line)
            if not match:
                continue

            active_name = match.group(1)
            active_start_line = line_number

        active_command.append(line)
        balance += paren_balance(line)

        if active_command and balance <= 0:
            command_text = "\n".join(active_command)
            open_paren = command_text.find("(")
            close_paren = command_text.rfind(")")
            body = command_text[open_paren + 1 : close_paren]

            yield {
                "name": active_name,
                "body": body,
                "args": split_cmake_args(body),
                "line": active_start_line,
            }

            active_command = []
            active_name = ""
            active_start_line = 0
            balance = 0


def normalize_condition(args: list[str]) -> str:
    return " ".join(args).strip()


def resolve_token(token: str, qt_major: str, kf_major: str) -> str:
    return (
        token.replace("${QT_MAJOR_VERSION}", qt_major)
        .replace("${KF_MAJOR}", kf_major)
        .replace("Qt${QT_MAJOR_VERSION}", f"Qt{qt_major}")
        .replace("KF${KF_MAJOR}", f"KF{kf_major}")
    )


def library_tokens(args: list[str], qt_major: str, kf_major: str) -> tuple[list[str], list[str]]:
    raw_libraries: list[str] = []
    libraries: list[str] = []

    for token in args:
        if token in LINK_KEYWORDS:
            continue
        raw_libraries.append(token)
        libraries.append(resolve_token(token, qt_major, kf_major))

    return raw_libraries, libraries


def collect_inventory(krita_source: Path, qt_major: str, kf_major: str) -> dict:
    link_entries: dict[str, list[dict]] = {}
    pykrita_modules: list[dict] = []

    for label, relative_path in TARGET_FILES.items():
        cmake_file = krita_source / relative_path
        if not cmake_file.exists():
            raise FileNotFoundError(f"Missing expected Krita CMake file: {cmake_file}")

        condition_stack: list[str] = []
        for command in iter_cmake_commands(cmake_file):
            name = command["name"].lower()
            args = command["args"]

            if name == "if":
                condition_stack.append(normalize_condition(args))
                continue

            if name == "elseif":
                if condition_stack:
                    condition_stack[-1] = normalize_condition(args)
                continue

            if name == "else":
                if condition_stack:
                    condition_stack[-1] = f"else({condition_stack[-1]})"
                continue

            if name == "endif":
                if condition_stack:
                    condition_stack.pop()
                continue

            if name == "target_link_libraries" and args:
                target = args[0]
                raw_libraries, libraries = library_tokens(args[1:], qt_major, kf_major)
                link_entries.setdefault(target, []).append(
                    {
                        "file": relative_path,
                        "line": command["line"],
                        "conditions": list(condition_stack),
                        "raw_libraries": raw_libraries,
                        "libraries": libraries,
                    }
                )
                continue

            if name in {"add_sip_python_module", "add_sip_python_module_v5"} and len(args) >= 3:
                module_name = args[0]
                module_sip = args[1]
                raw_libraries, libraries = library_tokens(args[2:], qt_major, kf_major)
                pykrita_modules.append(
                    {
                        "macro": command["name"],
                        "module": module_name,
                        "sip_file": module_sip,
                        "file": relative_path,
                        "line": command["line"],
                        "conditions": list(condition_stack),
                        "raw_libraries": raw_libraries,
                        "libraries": libraries,
                    }
                )

    return {
        "krita_source": str(krita_source),
        "qt_major": qt_major,
        "kf_major": kf_major,
        "pykrita_modules": pykrita_modules,
        "link_entries": link_entries,
    }


def condition_matches_android_qt5(condition: str) -> bool:
    normalized = condition.replace('"', "").strip()

    if not normalized:
        return True

    if normalized.startswith("else("):
        return False

    if normalized in {"ANDROID", "QT_MAJOR_VERSION STREQUAL 5"}:
        return True

    if "ANDROID" in normalized and "NOT" not in normalized:
        return True

    return False


def entry_matches_profile(entry: dict, profile: str | None) -> bool:
    if profile is None:
        return True

    if profile == "android_qt5_minimum":
        return all(condition_matches_android_qt5(condition) for condition in entry["conditions"])

    raise ValueError(f"Unknown inventory profile: {profile}")


def flatten_target_libraries(inventory: dict, target: str, profile: str | None = None) -> list[str]:
    libraries: list[str] = []
    for entry in inventory["link_entries"].get(target, []):
        if not entry_matches_profile(entry, profile):
            continue
        libraries.extend(entry["libraries"])
    return libraries


def condition_has(entry: dict, fragment: str) -> bool:
    return any(fragment in condition for condition in entry["conditions"])


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def validate_inventory(inventory: dict) -> dict:
    expected_pykrita = {"kritalibkis", "kritaui", "kritaimage", "kritalibbrush"}
    pykrita_v5 = [
        module
        for module in inventory["pykrita_modules"]
        if module["macro"] == "add_sip_python_module_v5" and module["module"] == "PyKrita.krita"
    ]
    require(pykrita_v5, "PyKrita.krita SIP v5 module was not found")
    require(
        set(pykrita_v5[0]["libraries"]) == expected_pykrita,
        f"Unexpected PyKrita.krita direct libraries: {pykrita_v5[0]['libraries']}",
    )

    expected_target_deps = {
        "kritalibkis": {"kritaui", "kritaimage", "kritaversion"},
        "kritalibbrush": {"kritaimage", "Qt5::Svg", "kritamultiarch", "lager"},
        "kritaimage": {
            "kritaversion",
            "kritawidgets",
            "kritaglobal",
            "kritapsdutils",
            "kritapigment",
            "kritacommand",
            "kritawidgetutils",
            "kritametadata",
            "kritaresources",
            "Eigen3::Eigen",
            "Boost::boost",
            "kritamultiarch",
        },
        "kritaui": {
            "KF5::CoreAddons",
            "KF5::Completion",
            "KF5::I18n",
            "KF5::ItemViews",
            "Qt5::Network",
            "Qt5::Concurrent",
            "Eigen3::Eigen",
            "Boost::boost",
            "${PNG_LIBRARIES}",
            "kritaversion",
            "kritaimpex",
            "kritacolor",
            "kritaimage",
            "kritalibbrush",
            "kritawidgets",
            "kritawidgetutils",
            "kritaresources",
        },
    }

    for target, expected in expected_target_deps.items():
        actual = set(flatten_target_libraries(inventory, target))
        missing = expected - actual
        require(not missing, f"{target} is missing expected dependencies: {sorted(missing)}")

    kritaui_android_entries = [
        entry for entry in inventory["link_entries"].get("kritaui", []) if condition_has(entry, "ANDROID")
    ]
    android_libraries = {
        library
        for entry in kritaui_android_entries
        for library in entry["libraries"]
    }
    require("GLESv3" in android_libraries, "kritaui Android link libraries do not include GLESv3")
    require("Qt5::Gui" in android_libraries, "kritaui Android link libraries do not include Qt5::Gui")
    require(
        "Qt5::AndroidExtras" in android_libraries,
        "kritaui Android link libraries do not include Qt5::AndroidExtras",
    )

    return {
        "pykrita_direct_libraries": sorted(expected_pykrita),
        "android_kritaui_libraries": sorted(android_libraries),
    }


def build_closure(inventory: dict, profile: str | None = None) -> dict:
    pykrita_v5 = next(
        module
        for module in inventory["pykrita_modules"]
        if module["macro"] == "add_sip_python_module_v5" and module["module"] == "PyKrita.krita"
    )

    seen: set[str] = set()
    pending = list(pykrita_v5["libraries"])

    while pending:
        library = pending.pop(0)
        if library in seen:
            continue

        seen.add(library)
        if library in inventory["link_entries"]:
            pending.extend(flatten_target_libraries(inventory, library, profile))

    krita_libraries = sorted(
        library for library in seen if re.match(r"^krita", library)
    )
    external_libraries = sorted(library for library in seen if library not in krita_libraries)

    return {
        "all": sorted(seen),
        "krita_libraries": krita_libraries,
        "external_libraries": external_libraries,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--krita-source", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--qt-major", default="5")
    parser.add_argument("--kf-major", default="5")
    args = parser.parse_args()

    inventory = collect_inventory(args.krita_source, args.qt_major, args.kf_major)
    inventory["validation"] = validate_inventory(inventory)
    inventory["closures"] = {
        "all_observed": build_closure(inventory),
        "android_qt5_minimum": build_closure(inventory, "android_qt5_minimum"),
    }

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(inventory, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print(f"Wrote PyKrita link inventory to {args.output}")
    print("Krita library artifacts needed for the first real-link layer:")
    for library in inventory["closures"]["android_qt5_minimum"]["krita_libraries"]:
        print(f"  - {library}")

    print("External link surface:")
    for library in inventory["closures"]["android_qt5_minimum"]["external_libraries"]:
        print(f"  - {library}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
