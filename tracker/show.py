import socket
import time
import sys
from generated.proto import pose_pb2

INPUT = "./records/" + sys.argv[1] + ".bin"

NAMES = [
	"nose",
	"left eye (inner)",
	"left eye",
	"left eye (outer)",
	"right eye (inner)",
	"right eye",
	"right eye (outer)",
	"left ear",
	"right ear",
	"mouth (left)",
	"mouth (right)",
	"left shoulder",
	"right shoulder",
	"left elbow",
	"right elbow",
	"left wrist",
	"right wrist",
	"left pinky",
	"right pinky",
	"left index",
	"right index",
	"left thumb",
	"right thumb",
	"left hip",
	"right hip",
	"left knee",
	"right knee",
	"left ankle",
	"right ankle",
	"left heel",
	"right heel",
	"left foot index",
	"right foot index",
]

with open(INPUT, "rb") as f:
	data = f.read()

frame = pose_pb2.PoseFrame()
frame.ParseFromString(data)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

print(f"Loaded {len(frame.landmarks)} landmarks.\n")

i = 0
for lm in frame.landmarks:
	print(f"{i}: \tx: {lm.x:.7f}, \ty: {lm.y:.7f}, \tz: {lm.z:.7f}, \tv: {lm.visibility:.7f} \t: {NAMES[i]}")
	i += 1
