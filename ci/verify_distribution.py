#!/usr/bin/env python3
"""Validate Scalix build and distributable package outputs on native CI runners."""

from __future__ import annotations

import argparse
import os
import platform
import stat
import subprocess
import sys
import tarfile
import tempfile
import zipfile
from pathlib import Path, PurePosixPath
from typing import Callable

SHADERS = (
    "resources/runtime/vs_tex.bin",
    "resources/runtime/fs_tex.bin",
    "resources/runtime/vs_grid.bin",
    "resources/runtime/fs_grid.bin",
)
LICENSES = (
    "share/licenses/Scalix/LICENSE",
    "share/licenses/Scalix/THIRD_PARTY_NOTICES.md",
)


def fail(message: str) -> None:
    print(f"::error::{message}", file=sys.stderr)
    raise SystemExit(1)


def normalize_path(value: str) -> str:
    parts = PurePosixPath(value.replace("\\", "/")).parts
    return "/".join(part for part in parts if part not in ("/", "."))


def operating_system() -> str:
    name = platform.system().lower()
    if name == "windows":
        return "windows"
    if name == "darwin":
        return "macos"
    if name == "linux":
        return "linux"
    fail(f"Unsupported CI operating system: {platform.system()}")
    raise AssertionError("unreachable")


def exe_name(system: str) -> str:
    return "sl.exe" if system == "windows" else "sl"


def runtime_library_name(system: str) -> str:
    suffix = {"windows": "dll", "linux": "so", "macos": "dylib"}[system]
    return f"libsxmp_be.{suffix}"


def validate_executable_bytes(data: bytes, system: str, label: str) -> None:
    if len(data) < 4096:
        fail(f"Executable looks truncated or unexpectedly small: {label} ({len(data)} bytes)")

    if system == "windows":
        valid = data.startswith(b"MZ")
        expected = "PE/MZ"
    elif system == "linux":
        valid = data.startswith(b"\x7fELF")
        expected = "ELF"
    else:
        # Thin/fat Mach-O headers in either byte order.
        mach_o_magics = {
            b"\xfe\xed\xfa\xce", b"\xce\xfa\xed\xfe",
            b"\xfe\xed\xfa\xcf", b"\xcf\xfa\xed\xfe",
            b"\xca\xfe\xba\xbe", b"\xbe\xba\xfe\xca",
            b"\xca\xfe\xba\xbf", b"\xbf\xba\xfe\xca",
        }
        valid = data[:4] in mach_o_magics
        expected = "Mach-O"

    if not valid:
        fail(f"Executable has the wrong file signature (expected {expected}): {label}")


def append_summary(title: str, lines: list[str]) -> None:
    summary_path = os.environ.get("GITHUB_STEP_SUMMARY")
    content = f"\n### {title}\n" + "\n".join(f"- {line}" for line in lines) + "\n"
    if summary_path:
        with open(summary_path, "a", encoding="utf-8") as summary:
            summary.write(content)
    print(content, end="")


def verify_build() -> None:
    system = operating_system()
    root = Path(__file__).resolve().parents[1]
    binary = root / "build" / exe_name(system)
    if not binary.is_file():
        fail(f"Build succeeded without producing the expected executable: {binary}")

    binary_size = binary.stat().st_size
    with binary.open("rb") as stream:
        header = stream.read(4096)
    validate_executable_bytes(header, system, str(binary))
    if system != "windows" and not (binary.stat().st_mode & stat.S_IXUSR):
        fail(f"Native executable is not marked executable: {binary}")

    missing_shaders = []
    for relative in SHADERS:
        shader = root / relative
        if not shader.is_file() or shader.stat().st_size == 0:
            missing_shaders.append(relative)
    if missing_shaders:
        fail("Build did not produce required runtime shaders: " + ", ".join(missing_shaders))

    append_summary(
        f"Build output verified ({system})",
        [
            f'Executable: "{binary.relative_to(root).as_posix()}" ({binary_size:,} bytes; valid native signature)',
            f"Runtime shaders: {len(SHADERS)}/{len(SHADERS)} present and non-empty",
        ],
    )


def find_entry(entries: list[str], suffix: str, system: str, *, required: bool = True) -> str | None:
    expected = normalize_path(suffix)
    casefold = system == "windows"
    wanted = expected.casefold() if casefold else expected
    matches = []
    for entry in entries:
        candidate = normalize_path(entry)
        compared = candidate.casefold() if casefold else candidate
        if compared == wanted or compared.endswith("/" + wanted):
            matches.append(candidate)
    # Avoid ambiguity if a malformed package includes multiple copies.
    if len(matches) > 1:
        fail(f'Package contains more than one file matching "{suffix}": {matches}')
    if not matches:
        if required:
            fail(f"Package is missing required file: {suffix}")
        return None
    return matches[0]


def validate_payload(
    system: str,
    entries: list[str],
    size_for: Callable[[str], int],
    read_prefix: Callable[[str, int], bytes],
    source_label: str,
) -> list[str]:
    executable_path = find_entry(entries, f"bin/{exe_name(system)}", system)
    assert executable_path is not None
    validate_executable_bytes(read_prefix(executable_path, 4096), system, f"{source_label}:{executable_path}")

    library_path = find_entry(entries, f"bin/{runtime_library_name(system)}", system)
    assert library_path is not None
    if size_for(library_path) < 4096:
        fail(f"Runtime library is unexpectedly small: {source_label}:{library_path}")

    verified = [
        f'"{executable_path}" ({size_for(executable_path):,} bytes; valid native signature)',
        f'"{library_path}" ({size_for(library_path):,} bytes)',
    ]

    for relative in SHADERS + LICENSES:
        path = find_entry(entries, relative, system)
        assert path is not None
        size = size_for(path)
        if size == 0:
            fail(f"Required package file is empty: {source_label}:{path}")
        verified.append(f'"{path}" ({size:,} bytes)')

    third_party_dir = "share/licenses/Scalix/third-party/"
    third_party = [
        normalize_path(path) for path in entries
        if normalize_path(path).startswith(third_party_dir)
        and not normalize_path(path).endswith("/")
    ]
    if not third_party:
        fail(f"Package contains no third-party license documents: {source_label}")
    verified.append(f"Third-party license documents: {len(third_party)}")
    return verified


def verify_zip(path: Path, system: str) -> list[str]:
    try:
        with zipfile.ZipFile(path) as archive:
            bad_member = archive.testzip()
            if bad_member is not None:
                fail(f"ZIP CRC check failed for {path.name}:{bad_member}")
            infos = [info for info in archive.infolist() if not info.is_dir()]
            entries = [normalize_path(info.filename) for info in infos]
            if len(set(entries)) != len(entries):
                fail(f"ZIP contains duplicate file paths: {path.name}")
            info_by_name = {normalize_path(info.filename): info for info in infos}

            def size_for(name: str) -> int:
                return info_by_name[name].file_size

            def read_prefix(name: str, size: int) -> bytes:
                with archive.open(info_by_name[name]) as member:
                    return member.read(size)

            return validate_payload(system, entries, size_for, read_prefix, path.name)
    except (OSError, zipfile.BadZipFile, RuntimeError) as error:
        fail(f"Could not inspect ZIP package {path.name}: {error}")
    raise AssertionError("unreachable")


def verify_tar(path: Path, system: str) -> list[str]:
    try:
        with tarfile.open(path, mode="r:gz") as archive:
            members = [member for member in archive.getmembers() if member.isfile()]
            entries = [normalize_path(member.name) for member in members]
            if len(set(entries)) != len(entries):
                fail(f"TAR package contains duplicate file paths: {path.name}")
            member_by_name = {normalize_path(member.name): member for member in members}

            def size_for(name: str) -> int:
                return member_by_name[name].size

            def read_prefix(name: str, size: int) -> bytes:
                stream = archive.extractfile(member_by_name[name])
                if stream is None:
                    fail(f"Unable to read packaged file {name} from {path.name}")
                with stream:
                    return stream.read(size)

            return validate_payload(system, entries, size_for, read_prefix, path.name)
    except (OSError, tarfile.TarError, EOFError) as error:
        fail(f"Could not inspect TGZ package {path.name}: {error}")
    raise AssertionError("unreachable")


def verify_dmg(path: Path, system: str) -> list[str]:
    with tempfile.TemporaryDirectory(prefix="scalix-dmg-") as temporary:
        mountpoint = Path(temporary) / "mount"
        mountpoint.mkdir()
        attached = False
        try:
            attach = subprocess.run(
                ["hdiutil", "attach", "-readonly", "-nobrowse", "-mountpoint", str(mountpoint), str(path)],
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
            )
            if attach.returncode != 0:
                fail(f"Could not mount macOS package {path.name}:\n{attach.stdout}")
            attached = True

            files = [candidate for candidate in mountpoint.rglob("*") if candidate.is_file()]
            entries = [candidate.relative_to(mountpoint).as_posix() for candidate in files]
            file_by_name = {normalize_path(name): mountpoint / name for name in entries}

            def size_for(name: str) -> int:
                return file_by_name[name].stat().st_size

            def read_prefix(name: str, size: int) -> bytes:
                with file_by_name[name].open("rb") as stream:
                    return stream.read(size)

            return validate_payload(system, entries, size_for, read_prefix, path.name)
        finally:
            if attached:
                subprocess.run(
                    ["hdiutil", "detach", str(mountpoint), "-force"],
                    text=True,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    check=False,
                )


def verify_package() -> None:
    system = operating_system()
    root = Path(__file__).resolve().parents[1]
    dist = root / "build" / "dist"
    if not dist.is_dir():
        fail(f"Package output directory does not exist: {dist}")

    if system == "windows":
        packages = sorted(path for path in dist.glob("*.zip") if path.is_file())
        installers = sorted(path for path in dist.glob("*.exe") if path.is_file())
        if len(packages) != 1:
            fail(f"Expected exactly one Windows ZIP package in {dist}, found {len(packages)}")
        if len(installers) != 1:
            fail(f"Expected exactly one Windows NSIS installer in {dist}, found {len(installers)}")
        package = packages[0]
        installer = installers[0]
        if package.stat().st_size < 4096:
            fail(f"Windows ZIP package is unexpectedly small: {package.name}")
        if installer.stat().st_size < 4096:
            fail(f"Windows NSIS installer is unexpectedly small: {installer.name}")
        with installer.open("rb") as stream:
            if stream.read(2) != b"MZ":
                fail(f"Windows NSIS installer does not have a PE/MZ signature: {installer.name}")
        details = [f'ZIP: "{package.name}" ({package.stat().st_size:,} bytes)']
        details.extend(verify_zip(package, system))
        details.append(f'NSIS installer: "{installer.name}" ({installer.stat().st_size:,} bytes; valid PE/MZ signature)')
    elif system == "linux":
        packages = sorted(path for path in dist.glob("*.tar.gz") if path.is_file())
        if not packages:
            packages = sorted(path for path in dist.glob("*.tgz") if path.is_file())
        if len(packages) != 1:
            fail(f"Expected exactly one Linux TGZ package in {dist}, found {len(packages)}")
        package = packages[0]
        if package.stat().st_size < 4096:
            fail(f"Linux TGZ package is unexpectedly small: {package.name}")
        details = [f'TGZ: "{package.name}" ({package.stat().st_size:,} bytes; archive opened successfully)']
        details.extend(verify_tar(package, system))
    else:
        packages = sorted(path for path in dist.glob("*.dmg") if path.is_file())
        if len(packages) != 1:
            fail(f"Expected exactly one macOS DMG package in {dist}, found {len(packages)}")
        package = packages[0]
        if package.stat().st_size < 4096:
            fail(f"macOS DMG package is unexpectedly small: {package.name}")
        details = [f'DMG: "{package.name}" ({package.stat().st_size:,} bytes; mounted read-only successfully)']
        details.extend(verify_dmg(package, system))

    append_summary(f"Distribution package verified ({system})", details)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("build", "package"), help="which generated output to validate")
    args = parser.parse_args()
    if args.mode == "build":
        verify_build()
    else:
        verify_package()


if __name__ == "__main__":
    main()
