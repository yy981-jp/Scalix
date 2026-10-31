from __future__ import annotations

import argparse
import sys
from pathlib import Path

from icon import generate_icons
from webp import convert_to_webp


def quality_int(value: str) -> int:
	try:
		result = int(value)
	except ValueError as exc:
		raise argparse.ArgumentTypeError("must be an integer") from exc
	if not 0 <= result <= 100:
		raise argparse.ArgumentTypeError("must be between 0 and 100")
	return result


def build_parser() -> argparse.ArgumentParser:
	parser = argparse.ArgumentParser(
		prog="assetgen",
		description="Generate WebP images and platform icon assets from one original image.",
	)
	subparsers = parser.add_subparsers(dest="mode", required=True)

	for mode in ("webp", "icon"):
		mode_parser = subparsers.add_parser(mode)
		mode_parser.add_argument(
			"--original",
			type=Path,
			required=True,
			help="Original SVG, PNG, JPEG or WebP image.",
		)
		mode_parser.add_argument(
			"--output-dir",
			type=Path,
			required=True,
			help="Directory where generated files are written.",
		)
		mode_parser.add_argument(
			"--name",
			default=None,
			help="Output filename stem. Defaults to the original filename without extension.",
		)
		mode_parser.add_argument(
			"--quality",
			type=quality_int,
			default=90,
			metavar="0-100",
			help="WebP quality (default: 90).",
		)

	return parser


def main(argv: list[str] | None = None) -> int:
	parser = build_parser()
	args = parser.parse_args(argv)

	try:
		if args.mode == "webp":
			outputs = [
				convert_to_webp(
					args.original,
					args.output_dir,
					name=args.name,
					quality=args.quality,
				)
			]
		else:
			outputs = generate_icons(
				args.original,
				args.output_dir,
				name=args.name,
				quality=args.quality,
			)
	except (FileNotFoundError, RuntimeError, ValueError, OSError) as exc:
		print(f"assetgen: error: {exc}", file=sys.stderr)
		return 1

	for output in outputs:
		print(output)
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
