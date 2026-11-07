from pathlib import Path
import re
import shutil

TARGETS = [
	"build/compile_commands.json",
	"sxmp/external/mediapipe/compile_commands.json",
]

# Windowsの絶対パスにある大文字のドライブ文字を検出する。
# C:/... と C:\... の両方に対応。
DRIVE_PATH = re.compile(rb'(?<![A-Za-z0-9_])([A-Z]):(?=[\\/])')


def normalize(path):
	path = Path(path)

	if not path.is_file():
		print(f"[SKIP] Not found: {path}")
		return

	data = path.read_bytes()
	fixed, count = DRIVE_PATH.subn(
		lambda match: match.group(1).lower() + b":",
		data
	)

	if count == 0:
		print(f"[OK] No changes needed: {path}")
		return

	backup = path.with_name(path.name + ".bak")

	if not backup.exists():
		shutil.copy2(path, backup)
		print(f"[BACKUP] {backup}")
	else:
		print(f"[BACKUP] Existing backup preserved: {backup}")

	temp = path.with_name(path.name + ".tmp")

	try:
		temp.write_bytes(fixed)
		temp.replace(path)
	except Exception:
		temp.unlink(missing_ok=True)
		raise

	print(f"[FIXED] {path}")
	print(f"        {count} drive-letter occurrence(s) normalized")


def main():
	for target in TARGETS:
		normalize(target)


if __name__ == "__main__":
	main()
