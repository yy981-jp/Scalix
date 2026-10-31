from __future__ import annotations

import io
import re
from defusedxml import ElementTree as ET
from pathlib import Path
from typing import Final

from PIL import Image, ImageOps

SUPPORTED_RASTER_FORMATS: Final[dict[str, str]] = {
	".png": "PNG",
	".jpg": "JPEG",
	".jpeg": "JPEG",
	".webp": "WEBP",
}
SUPPORTED_EXTENSIONS: Final[set[str]] = set(SUPPORTED_RASTER_FORMATS) | {".svg"}
SVG_LENGTH_RE: Final[re.Pattern[str]] = re.compile(
	r"^\s*([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)\s*([a-zA-Z%]*)\s*$"
)


def _svg_length_to_px(value: str | None) -> float | None:
	if not value:
		return None

	match = SVG_LENGTH_RE.match(value)
	if not match:
		return None

	number = float(match.group(1))
	unit = match.group(2).lower()
	unit_scale = {
		"": 1.0,
		"px": 1.0,
		"in": 96.0,
		"cm": 96.0 / 2.54,
		"mm": 96.0 / 25.4,
		"pt": 96.0 / 72.0,
		"pc": 16.0,
		"q": 96.0 / 101.6,
	}

	if unit not in unit_scale:
		return None
	return number * unit_scale[unit]


def _svg_aspect_ratio(path: Path) -> float:
	try:
		root = ET.parse(path).getroot()
	except (ET.ParseError, OSError) as exc:
		raise ValueError(f"Could not parse SVG: {exc}") from exc

	view_box = root.attrib.get("viewBox") or root.attrib.get("viewbox")
	if view_box:
		values = re.split(r"[\s,]+", view_box.strip())
		if len(values) == 4:
			try:
				width = float(values[2])
				height = float(values[3])
				if width > 0 and height > 0:
					return width / height
			except ValueError:
				pass

	width = _svg_length_to_px(root.attrib.get("width"))
	height = _svg_length_to_px(root.attrib.get("height"))
	if width and height and width > 0 and height > 0:
		return width / height

	# SVG's common fallback viewport is 300x150 when intrinsic dimensions
	# are not supplied. Prefer a predictable result over guessing a ratio.
	return 2.0


def _render_svg(path: Path, max_side: int | None = None) -> tuple[Image.Image, dict[str, bytes]]:
	try:
		import cairosvg
	except ImportError as exc:
		raise RuntimeError(
			"SVG input requires CairoSVG. Install the dependencies in requirements.txt."
		) from exc

	kwargs: dict[str, object] = {"url": str(path)}
	if max_side is not None:
		ratio = _svg_aspect_ratio(path)
		if ratio >= 1.0:
			width = max_side
			height = max(1, round(max_side / ratio))
		else:
			height = max_side
			width = max(1, round(max_side * ratio))
		kwargs["output_width"] = width
		kwargs["output_height"] = height

	try:
		png_data = cairosvg.svg2png(**kwargs)
		with Image.open(io.BytesIO(png_data)) as source:
			image = source.convert("RGBA")
	except Exception as exc:
		raise ValueError(f"Could not render SVG '{path}': {exc}") from exc

	return image, {}


def load_image(path: str | Path, *, max_svg_side: int | None = None) -> tuple[Image.Image, dict[str, bytes]]:
	"""Load a supported still image and return an RGBA image plus ICC metadata."""
	source_path = Path(path)
	extension = source_path.suffix.lower()
	if extension not in SUPPORTED_EXTENSIONS:
		allowed = ", ".join(sorted(SUPPORTED_EXTENSIONS))
		raise ValueError(f"Unsupported input extension '{extension}'. Supported: {allowed}")
	if not source_path.is_file():
		raise FileNotFoundError(f"Input file does not exist: {source_path}")

	if extension == ".svg":
		return _render_svg(source_path, max_svg_side)

	try:
		with Image.open(source_path) as source:
			actual_format = (source.format or "").upper()
			expected_format = SUPPORTED_RASTER_FORMATS[extension]
			if actual_format != expected_format:
				raise ValueError(
					f"File extension says {expected_format}, but the file contains {actual_format or 'unknown data'}."
				)

			# This tool handles static assets. For animated WebP, use its first frame.
			source.seek(0)
			icc_profile = source.info.get("icc_profile")
			image = ImageOps.exif_transpose(source).convert("RGBA")
	except (OSError, ValueError) as exc:
		if isinstance(exc, ValueError):
			raise
		raise ValueError(f"Could not read image '{source_path}': {exc}") from exc

	metadata: dict[str, bytes] = {}
	if isinstance(icc_profile, bytes):
		metadata["icc_profile"] = icc_profile
	return image, metadata


def fit_square(image: Image.Image, size: int) -> Image.Image:
	"""Fit an image into a transparent square without cropping or distortion."""
	if size < 1:
		raise ValueError("Image size must be positive")

	image = image.convert("RGBA")
	contained = ImageOps.contain(
		image,
		(size, size),
		method=Image.Resampling.LANCZOS,
	)
	canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
	x = (size - contained.width) // 2
	y = (size - contained.height) // 2
	canvas.alpha_composite(contained, (x, y))
	return canvas


def make_icon_master(image: Image.Image, size: int = 1024) -> Image.Image:
	return fit_square(image, size)


def resized_square(master: Image.Image, size: int) -> Image.Image:
	return master.resize((size, size), Image.Resampling.LANCZOS)
