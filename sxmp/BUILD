cc_binary(
    name = "sxmp",
    srcs = ["main.cc"],
    deps = [
        "@//mediapipe/tasks/cc/vision/pose_landmarker:pose_landmarker",
        "@//third_party:opencv",
    ],
)

cc_library(
    name = "mediapipe_backend_impl",
    srcs = ["sxmp.cpp"],
    hdrs = ["sxmp.h"],
    deps = [
        "@//mediapipe/tasks/cc/vision/pose_landmarker:pose_landmarker",
		"@//third_party:opencv",
    ],
    local_defines = ["SXMP_BACKEND_BUILD"],
)

cc_shared_library(
    name = "sxmp_be",
    deps = [":mediapipe_backend_impl"],
    visibility = ["//visibility:public"],
)
