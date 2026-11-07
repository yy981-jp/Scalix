import requests
from pathlib import Path

REMOTE = "https://storage.googleapis.com/mediapipe-models/"

with open("target.txt", "r", encoding="utf-8") as f:
	for line in f:
		line = line.rstrip("\n")
		url = REMOTE + line
		name = "models/" + Path(line).name
		
		print(url)
		
		r = requests.get(url)
		r.raise_for_status()
		
		with open(name, "wb") as ofs:
			ofs.write(r.content)
