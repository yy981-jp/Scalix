import socket
import time
from generated.proto import pose_pb2

INPUT = "pose.bin"
HOST = "127.0.0.1"
PORT = 51801

with open(INPUT, "rb") as f:
	data = f.read()

frame = pose_pb2.PoseFrame()
frame.ParseFromString(data)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

print(f"Loaded {len(frame.landmarks)} landmarks.")

while True:
	frame.timestamp = int(time.monotonic() * 1000)

	sock.sendto(
		frame.SerializeToString(),
		(HOST, PORT)
	)

	time.sleep(1.0 / 30.0)
