find_program(PROTOC_EXECUTABLE protoc REQUIRED)

# Generates C++ (engine) and Python (tracker) bindings from
# shared/proto/${NAME}.proto and exposes the generated C++ sources to the
# caller as ${NAME}_PROTO_SOURCES so they can be added to a target.
function(generate_proto NAME)
	set(PROTO_DIR ${CMAKE_SOURCE_DIR}/shared/proto)
	set(PROTO_FILE ${PROTO_DIR}/${NAME}.proto)

	set(ENGINE_GENERATED_DIR ${CMAKE_SOURCE_DIR}/engine/generated/proto)
	set(TRACKER_GENERATED_DIR ${CMAKE_SOURCE_DIR}/tracker/generated/proto)

	set(GENERATED_CC ${ENGINE_GENERATED_DIR}/${NAME}.pb.cc)
	set(GENERATED_H  ${ENGINE_GENERATED_DIR}/${NAME}.pb.h)

	add_custom_command(
		OUTPUT
			${GENERATED_CC}
			${GENERATED_H}
			${TRACKER_GENERATED_DIR}/${NAME}_pb2.py
			${TRACKER_GENERATED_DIR}/${NAME}_pb2.pyi

		COMMAND ${CMAKE_COMMAND} -E make_directory
			${ENGINE_GENERATED_DIR}
			${TRACKER_GENERATED_DIR}

		COMMAND ${PROTOC_EXECUTABLE}
			-I ${PROTO_DIR}
			--cpp_out=${ENGINE_GENERATED_DIR}
			--python_out=${TRACKER_GENERATED_DIR}
			--pyi_out=${TRACKER_GENERATED_DIR}
			${PROTO_FILE}

		DEPENDS ${PROTO_FILE}

		VERBATIM
	)

	set(${NAME}_PROTO_SOURCES ${GENERATED_CC} ${GENERATED_H} PARENT_SCOPE)
endfunction()
