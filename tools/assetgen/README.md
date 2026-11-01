# AssetGen v1

A small Python CLI that generates WebP files and platform icon assets from one original image.

## Requirements

- Python 3.10+
- Pillow 9.1.0 or newer
- CairoSVG 2.7.0 or newer (only needed for SVG input)

CairoSVG supports Windows, macOS and Linux, but its installation may require the Cairo and FFI native libraries for the target environment.

## Supported input formats

- SVG
- PNG
- JPEG (`.jpg`, `.jpeg`)
- WebP

Animated WebP is treated as a still image; only its first frame is used.

## Usage

```cmd
python assetgen.py webp --original path/to/source.svg --output-dir build/generated
python assetgen.py icon --original path/to/source.svg --output-dir build/generated/icons
```

Optional arguments:

- `--name NAME`: output filename stem (defaults to the source filename without extension)
- `--quality 0-100`: WebP quality (default: `90`)

Existing outputs are overwritten on every run.

## Output profiles

### `webp`

- `<name>.webp`
- Original raster dimensions are retained. SVG is rendered at its intrinsic size.
- WebP quality defaults to 90.

### `icon`

- `<name>.ico`: 16, 32, 48, 64, 128 and 256 px representations
- `<name>.icns`: multi-resolution ICNS with the standard and Retina representations supported by Pillow (up to 1024 physical pixels)
- `<name>-48.png`
- `<name>-128.png`
- `<name>-256.png`
- `<name>.webp`: 256x256, for SDL3

All icon outputs are square. The source aspect ratio is retained and any remaining canvas area is transparent padding; the image is never cropped or stretched.

## Notes

- ImageMagick, FFmpeg and `iconutil` are not used.
- Pillow writes ICNS on non-macOS platforms starting with version 8.3.0; this project requires 9.1.0 or newer for its image resampling API.
- Linux PNGs are generated as files only. Installation into the `hicolor` icon theme and creation of the `.desktop` entry are packaging responsibilities, not AssetGen responsibilities.
- WebP is lossy by default at quality 90. PNG, ICO and ICNS outputs are lossless image containers.
