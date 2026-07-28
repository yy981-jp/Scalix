import cv2
import mediapipe as mp
import socket
from generated.proto import pose_pb2

MODEL_PATH = "models/pose_landmarker_full.task"

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

timestamp = 0

while True:
	ret, image = cap.read()
	if not ret:
		break

	rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

	mp_image = mp.Image(
		image_format=mp.ImageFormat.SRGB,
		data=rgb
	)

	result = landmarker.detect_for_video(
		mp_image,
		timestamp
	)

	timestamp += 33

	packet = pose_pb2.PoseFrame()
	packet.timestamp = timestamp

	h, w = image.shape[:2]

	for pose in result.pose_landmarks:

		# UDP送信
		for lm in pose:
			item = packet.landmarks.add()
			item.x = lm.x
			item.y = lm.y
			item.z = lm.z
			item.visibility = lm.visibility

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
		("127.0.0.1", 51801)
	)

	cv2.imshow("camera", image)

	if cv2.waitKey(1) == 27:
		break

cap.release()
udp.close()
cv2.destroyAllWindows()
