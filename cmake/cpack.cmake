# Package metadata and output formats.
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/dist")
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${PROJECT_NAME})
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/yy981-jp/Scalix")
set(CPACK_PACKAGE_VENDOR "yy981")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
	"High-performance 3D avatar engine utilizing C++23 and bgfx for maximizing VRChat/Unity asset usage.")

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/cmake/cpack-description.txt")

set(CPACK_THREADS 0)

if(WIN32)
	set(CPACK_GENERATOR "ZIP;NSIS")
elseif(APPLE)
	set(CPACK_GENERATOR "DragNDrop")
elseif(UNIX)
	set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)
