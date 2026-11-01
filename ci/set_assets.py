#!/usr/bin/env python3
"""Copy generated/submodule assets into Scalix resources; mirrors setAssets.bat."""

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
	destination.mkdir(parents=True, exist_ok=True)
	shutil.copytree(source, destination, dirs_exist_ok=True)

	print(f"Copied {len(files)} asset file(s):")
	print(f"  {source}")
	print(f"  -> {destination}")


if __name__ == "__main__":
	main()
