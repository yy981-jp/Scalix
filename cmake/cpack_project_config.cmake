# Settings specific to the Debian package generator.
if(CPACK_GENERATOR STREQUAL "DEB")
	set(CPACK_DEBIAN_PACKAGE_MAINTAINER "yy981")
	set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
	set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
	set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${CPACK_PACKAGE_HOMEPAGE_URL}")
	set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")

	# bgfx loads these graphics libraries dynamically, so dpkg-shlibdeps
	# cannot discover them from the ELF DT_NEEDED entries.
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libegl1, libgl1")
	set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "libvulkan1")
	set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

	# Reorganize only the DEB staging tree; TGZ keeps its portable layout.
	set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_CURRENT_LIST_DIR}/cpack_deb_layout.cmake")
endif()
