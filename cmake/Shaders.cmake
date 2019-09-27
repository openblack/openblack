if(OPENBLACK_USE_SYSTEM_DEPS)
	find_package(bgfx REQUIRED COMPONENTS shaderc)
else()
	add_executable(bgfx::shaderc ALIAS shaderc)
	set(BGFX_SHADER_INCLUDE_PATH ${bgfx_SOURCE_DIR}/bgfx/src)
endif()

function(get_profile_ext PROFILE PROFILE_EXT)
	string(REPLACE spirv spv PROFILE ${PROFILE})
	string(REPLACE metal mtl PROFILE ${PROFILE})
	string(REPLACE s_3 dx9 PROFILE ${PROFILE})
	string(REPLACE s_5 dx11 PROFILE ${PROFILE})

	set(${PROFILE_EXT} ${PROFILE} PARENT_SCOPE)
endfunction()

function(mark_shaders_for_compilation_to_header)
	set(options "")
	set(oneValueArgs TYPE VARYING_DEF OUTPUT_DIR)
	set(multiValueArgs SHADERS)
	cmake_parse_arguments(
		SHADER_COMPILATION
		"${options}"
		"${oneValueArgs}"
		"${multiValueArgs}"
		"${ARGN}")
	foreach(SHADER_FILE ${SHADER_COMPILATION_SHADERS})
		source_group("Shaders" FILES "${SHADER}")
		get_filename_component(SHADER_FILE_BASENAME ${SHADER_FILE} NAME_WE)
		get_filename_component(SHADER_FILE_ABSOLUTE ${SHADER_FILE} ABSOLUTE)
		set (EXTRA_PROFILES spirv)
#		set (EMPTY_PROFILES pssl)
		if (UNIX)
			set (PLATFORM linux)
			list(APPEND EMPTY_PROFILES metal)
			list(APPEND EMPTY_PROFILES s_3)
#			list(APPEND EMPTY_PROFILES s_4)
#			list(APPEND EMPTY_PROFILES s_4_0_level)
			list(APPEND EMPTY_PROFILES s_5)
		elseif (EMSCRIPTEN)
			set (PLATFORM emscripten)
			list(APPEND EMPTY_PROFILES metal)
			list(APPEND EMPTY_PROFILES s_3)
#			list(APPEND EMPTY_PROFILES s_4)
#			list(APPEND EMPTY_PROFILES s_4_0_level)
			list(APPEND EMPTY_PROFILES s_5)
		elseif (APPLE)
			set (PLATFORM osx)
			list(APPEND EXTRA_PROFILES metal)
			list(APPEND EMPTY_PROFILES s_3)
#			list(APPEND EMPTY_PROFILES s_4)
#			list(APPEND EMPTY_PROFILES s_4_0_level)
			list(APPEND EMPTY_PROFILES s_5)
		elseif (WIN32)
			set (PLATFORM windows)
			list(APPEND EMPTY_PROFILES metal)
			list(APPEND EXTRA_PROFILES s_3)
#			list(APPEND EXTRA_PROFILES s_4)
#			list(APPEND EXTRA_PROFILES s_4_0_level)
			list(APPEND EXTRA_PROFILES s_5)
		elseif (MINGW)
			set (PLATFORM windows)
			list(APPEND EMPTY_PROFILES metal)
			list(APPEND EXTRA_PROFILES s_3)
#			list(APPEND EXTRA_PROFILES s_4)
#			list(APPEND EXTRA_PROFILES s_4_0_level)
			list(APPEND EXTRA_PROFILES s_5)
		elseif (MSYS)
			set (PLATFORM windows)
			list(APPEND EMPTY_PROFILES metal)
			list(APPEND EXTRA_PROFILES s_3)
#			list(APPEND EXTRA_PROFILES s_4)
#			list(APPEND EXTRA_PROFILES s_4_0_level)
			list(APPEND EXTRA_PROFILES s_5)
		elseif (CYGWIN)
			set (PLATFORM windows)
			list(APPEND EMPTY_PROFILES metal)
			list(APPEND EXTRA_PROFILES s_3)
#			list(APPEND EXTRA_PROFILES s_4)
#			list(APPEND EXTRA_PROFILES s_4_0_level)
			list(APPEND EXTRA_PROFILES s_5)
		else ()
			message(error "shaderc: Unsupported platform")
		endif ()

		# Build empty output targets and their commands
		foreach(PROFILE ${EMPTY_PROFILES})
			get_profile_ext(${PROFILE} PROFILE_EXT)
			list(APPEND EMPTY_OUTPUTS ${SHADER_COMPILATION_OUTPUT_DIR}/${SHADER_FILE_BASENAME}.${PROFILE_EXT}.bin.h)
			list(APPEND EMPTY_COMMANDS COMMAND ${CMAKE_COMMAND} -E touch ${SHADER_COMPILATION_OUTPUT_DIR}/${SHADER_FILE_BASENAME}.${PROFILE_EXT}.bin.h)
		endforeach()
		# Build output targets and their commands
		foreach(PROFILE ${EXTRA_PROFILES})
			get_profile_ext(${PROFILE} PROFILE_EXT)
			list(APPEND EXTRA_OUTPUTS ${SHADER_COMPILATION_OUTPUT_DIR}/${SHADER_FILE_BASENAME}.${PROFILE_EXT}.bin.h)
			list(APPEND EXTRA_COMMANDS
				COMMAND
					bgfx::shaderc
					--verbose
					-f ${SHADER_FILE_ABSOLUTE}
					-o ${SHADER_COMPILATION_OUTPUT_DIR}/${SHADER_FILE_BASENAME}.${PROFILE_EXT}.bin.h
					-i ${BGFX_SHADER_INCLUDE_PATH}
					--platform ${PLATFORM}
					--bin2c ${SHADER_FILE_BASENAME}_${PROFILE_EXT}
					--profile ${PROFILE}
					--type ${SHADER_COMPILATION_TYPE}
					--varyingdef ${SHADER_COMPILATION_VARYING_DEF}
					"$<$<CONFIG:debug>:--debug>$<$<CONFIG:relwithdebinfo>:--debug>"
					"$<$<CONFIG:debug>:-O 0>$<$<CONFIG:release>:-O 3>$<$<CONFIG:relwithdebinfo>:-O 3>$<$<CONFIG:minsizerel>:-O 3>"
			)
		endforeach()

		add_custom_command(
			OUTPUT
			${SHADER_COMPILATION_OUTPUT_DIR}/${SHADER_FILE_BASENAME}.essl.bin.h
			${SHADER_COMPILATION_OUTPUT_DIR}/${SHADER_FILE_BASENAME}.glsl.bin.h
			${EMPTY_OUTPUTS}
			${EXTRA_OUTPUTS}
			# OpenGL ES
			COMMAND
				${CMAKE_COMMAND} -E make_directory ${SHADER_COMPILATION_OUTPUT_DIR}
			COMMAND
				bgfx::shaderc
				--verbose
				-f ${SHADER_FILE_ABSOLUTE}
				-o ${SHADER_COMPILATION_OUTPUT_DIR}/${SHADER_FILE_BASENAME}.essl.bin.h
				-i ${BGFX_SHADER_INCLUDE_PATH}
				--platform ${PLATFORM}
				--bin2c ${SHADER_FILE_BASENAME}_essl
				--profile 140
				--type ${SHADER_COMPILATION_TYPE}
				--varyingdef ${SHADER_COMPILATION_VARYING_DEF}
				"$<$<CONFIG:debug>:--debug>$<$<CONFIG:relwithdebinfo>:--debug>"
				"$<$<CONFIG:debug>:-O 0>$<$<CONFIG:release>:-O 3>$<$<CONFIG:relwithdebinfo>:-O 3>$<$<CONFIG:minsizerel>:-O 3>"
			# TODO: try 4.40
			# OpenGL GLSL 1.40
			COMMAND
				bgfx::shaderc
				--verbose
				-f ${SHADER_FILE_ABSOLUTE}
				-o ${SHADER_COMPILATION_OUTPUT_DIR}/${SHADER_FILE_BASENAME}.glsl.bin.h
				-i ${BGFX_SHADER_INCLUDE_PATH}
				--platform ${PLATFORM}
				--bin2c ${SHADER_FILE_BASENAME}_glsl
				--profile 150
				--type ${SHADER_COMPILATION_TYPE}
				--varyingdef ${SHADER_COMPILATION_VARYING_DEF}
				--bin2c
				"$<$<CONFIG:debug>:--debug>$<$<CONFIG:relwithdebinfo>:--debug>"
				"$<$<CONFIG:debug>:-O 0>$<$<CONFIG:release>:-O 3>$<$<CONFIG:relwithdebinfo>:-O 3>$<$<CONFIG:minsizerel>:-O 3>"
			${EMPTY_COMMANDS}
			${EXTRA_COMMANDS}
			MAIN_DEPENDENCY ${SHADER_FILE_ABSOLUTE}
		)
	endforeach()
endfunction()

function(mark_shaders_for_compilation)
	set(options "")
	set(oneValueArgs TYPE VARYING_DEF OUTPUT_DIR)
	set(multiValueArgs SHADERS)
	cmake_parse_arguments(
		SHADER_COMPILATION
			"${options}"
			"${oneValueArgs}"
			"${multiValueArgs}"
			"${ARGN}")
	foreach(SHADER_FILE ${SHADER_COMPILATION_SHADERS})
		source_group("Shaders" FILES "${SHADER}")
		get_filename_component(SHADER_FILE_BASENAME ${SHADER_FILE} NAME)
		get_filename_component(SHADER_FILE_ABSOLUTE ${SHADER_FILE} ABSOLUTE)
		set (EXTRA_PROFILES spirv) # pssl
		if (UNIX)
			set (PLATFORM linux)
		elseif (EMSCRIPTEN)
			set (PLATFORM emscripten)
		elseif (APPLE)
			set (PLATFORM osx)
			list(APPEND EXTRA_PROFILES metal)
		elseif (WIN32)
			set (PLATFORM windows)
			list(APPEND EXTRA_PROFILES s_3)
			list(APPEND EXTRA_PROFILES s_4)
			list(APPEND EXTRA_PROFILES s_4_0_level)
			list(APPEND EXTRA_PROFILES s_5)
		elseif (MINGW)
			set (PLATFORM windows)
			list(APPEND EXTRA_PROFILES s_3)
			list(APPEND EXTRA_PROFILES s_4)
			list(APPEND EXTRA_PROFILES s_4_0_level)
		elseif (MSYS)
			set (PLATFORM windows)
			list(APPEND EXTRA_PROFILES s_3)
			list(APPEND EXTRA_PROFILES s_4)
			list(APPEND EXTRA_PROFILES s_4_0_level)
		elseif (CYGWIN)
			set (PLATFORM windows)
			list(APPEND EXTRA_PROFILES s_3)
			list(APPEND EXTRA_PROFILES s_4)
			list(APPEND EXTRA_PROFILES s_4_0_level)
		else ()
			message(error "shaderc: Unsupported platform")
		endif ()

		# Build output targets and their commands
		foreach(PROFILE ${EXTRA_PROFILES})
			list(APPEND EXTRA_OUTPUTS ${SHADER_COMPILATION_OUTPUT_DIR}/${SHADER_FILE_BASENAME}.${PROFILE})
			list(APPEND EXTRA_COMMANDS
				COMMAND
					bgfx::shaderc
					--verbose
					-f ${SHADER_FILE_ABSOLUTE}
					-o ${SHADER_COMPILATION_OUTPUT_DIR}/${SHADER_FILE_BASENAME}.${PROFILE}
					-i ${BGFX_SHADER_INCLUDE_PATH}
					--platform ${PLATFORM}
					--profile ${PROFILE}
					--type ${SHADER_COMPILATION_TYPE}
					--varyingdef ${SHADER_COMPILATION_VARYING_DEF}
					"$<$<CONFIG:debug>:--debug>$<$<CONFIG:relwithdebinfo>:--debug>"
					"$<$<CONFIG:debug>:-O 0>$<$<CONFIG:release>:-O 3>$<$<CONFIG:relwithdebinfo>:-O 3>$<$<CONFIG:minsizerel>:-O 3>"
			)
		endforeach()

		add_custom_command(
			OUTPUT
#				${SHADER_COMPILATION_OUTPUT_DIR}/${SHADER_FILE_BASENAME}.essl
				${SHADER_COMPILATION_OUTPUT_DIR}/${SHADER_FILE_BASENAME}.glsl
				${EXTRA_OUTPUTS}
			# OpenGL ES
			COMMAND
#				bgfx::shaderc
#				--verbose
#				-f ${SHADER_FILE_ABSOLUTE}
#				-o ${SHADER_COMPILATION_OUTPUT_DIR}/${SHADER_FILE_BASENAME}.essl
#				-i ${BGFX_SHADER_INCLUDE_PATH}
#				--platform ${PLATFORM}
#				--profile 140
#				--type ${SHADER_COMPILATION_TYPE}
#				--varyingdef ${SHADER_COMPILATION_VARYING_DEF}
#				"$<$<CONFIG:debug>:--debug>$<$<CONFIG:relwithdebinfo>:--debug>"
#				"$<$<CONFIG:debug>:-O 0>$<$<CONFIG:release>:-O 3>$<$<CONFIG:relwithdebinfo>:-O 3>$<$<CONFIG:minsizerel>:-O 3>"
			# TODO: try 4.40
			# OpenGL GLSL 1.40
		 	COMMAND
			/home/sandy/code/bgfx.cmake/cmake-build-debug/shaderc
		 		--verbose
		 		-f ${SHADER_FILE_ABSOLUTE}
		 		-o ${SHADER_COMPILATION_OUTPUT_DIR}/${SHADER_FILE_BASENAME}.glsl
		 		-i ${BGFX_SHADER_INCLUDE_PATH}
		 		--platform ${PLATFORM}
		 		--profile 120
		 		--type ${SHADER_COMPILATION_TYPE}
		 		--varyingdef ${SHADER_COMPILATION_VARYING_DEF}
		 		"$<$<CONFIG:debug>:--debug>$<$<CONFIG:relwithdebinfo>:--debug>"
		 		"$<$<CONFIG:debug>:-O 0>$<$<CONFIG:release>:-O 3>$<$<CONFIG:relwithdebinfo>:-O 3>$<$<CONFIG:minsizerel>:-O 3>"
			${EXTRA_COMMANDS}
		 	MAIN_DEPENDENCY ${SHADER_FILE_ABSOLUTE}
			DEPENDS ${SHADER_COMPILATION_VARYING_DEF}
		)
	endforeach()
endfunction()
