from __future__ import annotations

from pathlib import Path

from PIL import Image

from raster import load_image, make_icon_master, resized_square

ICO_SIZES = (16, 32, 48, 64, 128, 256)
ICNS_SIZES = (32, 64, 128, 256, 512, 1024)
LINUX_PNG_SIZES = (48, 128, 256)
SDL_WEBP_SIZE = 256


def generate_icons(
	original: str | Path,
	output_dir: str | Path,
	*,
	name: str | None = None,
	quality: int = 90,
) -> list[Path]:
	if not 0 <= quality <= 100:
		raise ValueError("WebP quality must be between 0 and 100")

	source_path = Path(original)
	image, metadata = load_image(source_path, max_svg_side=1024)
	master = make_icon_master(image, 1024)

	destination_dir = Path(output_dir)
	destination_dir.mkdir(parents=True, exist_ok=True)
	output_name = name or source_path.stem
	outputs: list[Path] = []

	# Windows: one ICO containing all configured resolutions.
	ico_path = destination_dir / f"{output_name}.ico"
	ico_frames = [resized_square(master, size) for size in ICO_SIZES]
	ico_base = resized_square(master, 256)
	ico_base.save(
		ico_path,
		format="ICO",
		sizes=[(size, size) for size in ICO_SIZES],
		append_images=[frame for frame in ico_frames if frame.size != ico_base.size],
	)
	outputs.append(ico_path)

	# macOS: Pillow writes ICNS cross-platform (Pillow >= 8.3.0).
	# The ICNS writer uses 32, 64, 128, 256, 512 and 1024 px representations,
	# including the corresponding Retina resource entries.
	icns_path = destination_dir / f"{output_name}.icns"
	icns_frames = [resized_square(master, size) for size in ICNS_SIZES]
	icns_base = resized_square(master, 1024)
	icns_base.save(
		icns_path,
		format="ICNS",
		append_images=[frame for frame in icns_frames if frame.size != icns_base.size],
	)
	outputs.append(icns_path)

	# Linux: size-specific PNGs, ready to be staged into the hicolor theme.
	for size in LINUX_PNG_SIZES:
		png_path = destination_dir / f"{output_name}-{size}.png"
		resized_square(master, size).save(png_path, format="PNG", optimize=True)
		outputs.append(png_path)

	# SDL3: a square WebP, preserving transparency.
	webp_path = destination_dir / f"{output_name}.webp"
	webp_options: dict[str, object] = {
		"format": "WEBP",
		"quality": quality,
		"method": 6,
	}
	if "icc_profile" in metadata:
		webp_options["icc_profile"] = metadata["icc_profile"]
	resized_square(master, SDL_WEBP_SIZE).save(webp_path, **webp_options)
	outputs.append(webp_path)

	return outputs
