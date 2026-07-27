import time
import cv2
import mediapipe as mp
import socket
from generated.proto import pose_pb2

MODEL_PATH = "models/pose_landmarker_full.task"

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

	for pose in result.pose_landmarks:
		for lm in pose:
			item = packet.landmarks.add()
			item.x = lm.x
			item.y = lm.y
			item.z = lm.z
			item.visibility = lm.visibility

	udp.sendto(packet.SerializeToString(), ("127.0.0.1", 51801))

	cv2.imshow("camera", image)

	if cv2.waitKey(1) == 27:
		break

cap.release()
udp.close()
cv2.destroyAllWindows()
