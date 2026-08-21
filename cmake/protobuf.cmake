find_program(PROTOC_EXECUTABLE protoc REQUIRED)

function(generate_proto NAME)
	set(PROTO_FILE
		${CMAKE_SOURCE_DIR}/proto/${NAME}.proto
	)

	add_custom_command(
		OUTPUT
			${CMAKE_SOURCE_DIR}/engine/generated/${NAME}.pb.cc
			${CMAKE_SOURCE_DIR}/engine/generated/${NAME}.pb.h
			${CMAKE_SOURCE_DIR}/tracker/generated/${NAME}_pb2.py
			${CMAKE_SOURCE_DIR}/tracker/generated/${NAME}_pb2.pyi

		COMMAND ${PROTOC_EXECUTABLE}
			--cpp_out=${CMAKE_SOURCE_DIR}/engine/generated
			--python_out=${CMAKE_SOURCE_DIR}/tracker/generated
			--pyi_out=${CMAKE_SOURCE_DIR}/tracker/generated
			${PROTO_FILE}

		DEPENDS ${PROTO_FILE}

		VERBATIM
	)
endfunction()
