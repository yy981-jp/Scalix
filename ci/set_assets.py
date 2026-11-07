#!/usr/bin/env python3
"""Copy runtime assets from assets/dist/assets into resources/assets."""

from __future__ import annotations

import shutil
from pathlib import Path


def main() -> None:
	root = Path(__file__).resolve().parents[1]
	source = root / "assets" / "dist" / "assets"
	destination = root / "resources" / "assets"

	if not source.is_dir():
		raise SystemExit(f"Asset output directory does not exist: {source}")

	files = [path for path in source.rglob("*") if path.is_file()]
	if destination.exists():
		shutil.rmtree(destination)
	shutil.copytree(source, destination)

	print(f"Copied {len(files)} runtime asset file(s):")
	print(f"  {source}")
	print(f"  -> {destination}")


if __name__ == "__main__":
	main()
