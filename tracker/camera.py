import cv2
import mediapipe as mp
import socket
import time
import argparse

from generated.proto import pose_pb2


POSE_CONNECTIONS = [
	(0, 1), (1, 2), (2, 3),
	(0, 4), (4, 5), (5, 6),

	(7, 9),
	(8, 10),
	(9, 10),

	(11, 12),

	(11, 13), (13, 15),
	(15, 17), (15, 19), (15, 21), (17, 19),

	(12, 14), (14, 16),
	(16, 18), (16, 20), (16, 22), (18, 20),

	(11, 23),
	(12, 24),
	(23, 24),

	(23, 25), (25, 27),
	(27, 29), (29, 31), (27, 31),

	(24, 26), (26, 28),
	(28, 30), (30, 32), (28, 32),
]


parser = argparse.ArgumentParser(
	description="sxtr-camera"
)
parser.add_argument("--background", "-b", action='store_true', help="デバッグ表示オフ")
parser.add_argument("--host",		default="127.0.0.1", help="送信先ip")
parser.add_argument("--port",		default=51801, type=int, help="送信先port")
parser.add_argument("--model",		default="full", help="モデル名")

args = parser.parse_args()


debugMode = not args.background
MODEL_PATH = f"models/pose_landmarker_{args.model}.task"

BaseOptions = mp.tasks.BaseOptions
PoseLandmarker = mp.tasks.vision.PoseLandmarker
PoseLandmarkerOptions = mp.tasks.vision.PoseLandmarkerOptions
RunningMode = mp.tasks.vision.RunningMode

options = PoseLandmarkerOptions(
	base_options=BaseOptions(
		model_asset_path=MODEL_PATH
	),
	running_mode=RunningMode.VIDEO
)

landmarker = PoseLandmarker.create_from_options(options)

cap = cv2.VideoCapture(0)

udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

start_time = time.perf_counter()

while True:
	ret, image = cap.read()
	if not ret:
		break

	rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

	mp_image = mp.Image(
		image_format=mp.ImageFormat.SRGB,
		data=rgb
	)

	timestamp = int((time.perf_counter() - start_time) * 1000)

	result = landmarker.detect_for_video(
		mp_image,
		timestamp
	)

	packet = pose_pb2.PoseFrame()
	packet.timestamp = timestamp

	h, w = image.shape[:2]

	for pose, world_pose in zip(
		result.pose_landmarks,
		result.pose_world_landmarks
	):

		# UDP送信（World Landmarks）
		for lm in world_pose:
			item = packet.landmarks.add()
			item.x = lm.x
			item.y = lm.y
			item.z = lm.z
			item.visibility = lm.visibility


		if debugMode:
			# 骨線
			for a, b in POSE_CONNECTIONS:
				la = pose[a]
				lb = pose[b]

				if la.visibility < 0.5 or lb.visibility < 0.5:
					continue

				cv2.line(
					image,
					(int(la.x * w), int(la.y * h)),
					(int(lb.x * w), int(lb.y * h)),
					(255, 255, 255),
					2,
					cv2.LINE_AA
				)

			# 点
			for i, lm in enumerate(pose):
				if lm.visibility < 0.5:
					continue

				x = int(lm.x * w)
				y = int(lm.y * h)

				cv2.circle(
					image,
					(x, y),
					4,
					(0, 255, 0),
					-1,
					cv2.LINE_AA
				)

				# デバッグ用に番号表示
				cv2.putText(
					image,
					str(i),
					(x + 5, y - 5),
					cv2.FONT_HERSHEY_SIMPLEX,
					0.35,
					(0, 255, 255),
					1,
					cv2.LINE_AA
				)

	udp.sendto(
		packet.SerializeToString(),
		(args.host, args.port)
	)

	if debugMode:
		cv2.imshow("camera", image)

		if cv2.waitKey(1) == 27:
			break

cap.release()
udp.close()
if debugMode:
	cv2.destroyAllWindows()
