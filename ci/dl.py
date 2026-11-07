import json
import sys
from pathlib import Path
from urllib.parse import urljoin
import urllib.request
import hashlib



def core(manifestPathDir:str):
	manifestPath = Path(manifestPathDir) / "manifest.json"
	with open(manifestPath, "r", encoding="utf-8") as f:
		manifest = json.load(f)

	if manifest["version"] != 1:
		raise RuntimeError("Version mismatch")

	for entry in manifest["files"]:
		# get entry
		url = urljoin(
			manifest["remote"][entry["origin"]].rstrip("/") + "/",
			entry["remote"]
		)
		local = Path(manifestPathDir) / entry["local"]
		local.parent.mkdir(parents=True, exist_ok=True)
		correctHash = entry["sha256"]

		# dl
		urllib.request.urlretrieve(str(url), str(local))

		# verify
		with open(local, "rb") as f:
			hashed = hashlib.file_digest(f, "sha256").hexdigest()
		
		if hashed != correctHash:
			raise RuntimeError(f"sha256 mismatch: {entry['local']}")



def main():
	if len(sys.argv) < 2:
		raise RuntimeError("target dir is required")
	manifestPathDir = sys.argv[1]
	core(manifestPathDir)





main()
