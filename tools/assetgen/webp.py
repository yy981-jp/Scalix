from __future__ import annotations

from pathlib import Path

from PIL import Image

from raster import load_image


def convert_to_webp(
	original: str | Path,
	output_dir: str | Path,
	*,
	name: str | None = None,
	quality: int = 90,
) -> Path:
	if not 0 <= quality <= 100:
		raise ValueError("WebP quality must be between 0 and 100")

	source_path = Path(original)
	image, metadata = load_image(source_path)
	output_path = Path(output_dir)
	output_path.mkdir(parents=True, exist_ok=True)
	output_name = name or source_path.stem
	destination = output_path / f"{output_name}.webp"

	save_options: dict[str, object] = {
		"format": "WEBP",
		"quality": quality,
		"method": 6,
	}
	if "icc_profile" in metadata:
		save_options["icc_profile"] = metadata["icc_profile"]

	image.save(destination, **save_options)
	return destination
