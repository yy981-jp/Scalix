import csv
import socket
import time
import sys
from generated.proto import pose_pb2

HOST = "127.0.0.1"
PORT = 51801

CSV_FILE = "./records/" + sys.argv[1] + ".csv"
PLAY_TIME = float(sys.argv[2])

frames = []

with open(CSV_FILE, newline="", encoding="utf-8") as f:
	for row in csv.reader(f):
		if not row:
			continue

		name = row[0].strip()
		if not name:
			continue

		if not name.endswith(".bin"):
			name += ".bin"

		with open("./records/" + name, "rb") as bf:
			frame = pose_pb2.PoseFrame()
			frame.ParseFromString(bf.read())
			frames.append(frame)

print(f"Loaded {len(frames)} frames.")

if len(frames) == 0:
	sys.exit("No frames.")

interval = PLAY_TIME / len(frames)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

while True:
	for frame in frames:
		end_time = time.monotonic() + PLAY_TIME

		while time.monotonic() < end_time:
			frame.timestamp = int(time.monotonic() * 1000)

			sock.sendto(
				frame.SerializeToString(),
				(HOST, PORT)
			)

			time.sleep(1.0 / 60.0)
