import cv2
import mediapipe as mp

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

timestamp = 0

while True:
	ret, frame = cap.read()

	if not ret:
		break

	rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

	mp_image = mp.Image(
		image_format=mp.ImageFormat.SRGB,
		data=rgb
	)

	result = landmarker.detect_for_video(
		mp_image,
		timestamp
	)

	timestamp += 33

	if result.pose_landmarks:
		landmarks = result.pose_landmarks[0]

		print(
			"nose:",
			landmarks[0].x,
			landmarks[0].y,
			landmarks[0].z
		)

	cv2.imshow("camera", frame)

	if cv2.waitKey(1) == 27:
		break

cap.release()
cv2.destroyAllWindows()