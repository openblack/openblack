find_package(bgfx REQUIRED COMPONENTS shaderc)

# shaderc_parse(
#	FILE filename
#	OUTPUT filename
#	FRAGMENT|VERTEX|COMPUTE
#	ANDROID|ASM_JS|IOS|LINUX|NACL|OSX|WINDOWS
#	[PROFILE profile]
#	[O 0|1|2|3]
#	[VARYINGDEF filename]
#	[BIN2C filename]
#	[INCLUDES include;include]
#	[DEFINES include;include]
#	[DEPENDS]
#	[PREPROCESS]
#	[RAW]
#	[VERBOSE]
#	[DEBUG]
#	[DISASM]
#	[WERROR]
# )
function( shaderc_parse ARG_OUT )
	cmake_parse_arguments( ARG "DEPENDS;ANDROID;ASM_JS;IOS;LINUX;NACL;OSX;WINDOWS;PREPROCESS;RAW;FRAGMENT;VERTEX;COMPUTE;VERBOSE;DEBUG;DISASM;WERROR" "FILE;OUTPUT;VARYINGDEF;BIN2C;PROFILE;O" "INCLUDES;DEFINES" ${ARGN} )
	set( CLI "" )

	# -f
	if( ARG_FILE )
		list( APPEND CLI "-f" "${ARG_FILE}" )
	endif()

	# -i
	if( ARG_INCLUDES )
		list( APPEND CLI "-i" )
		set( INCLUDES "" )
		foreach( INCLUDE ${ARG_INCLUDES} )
			if( NOT "${INCLUDES}" STREQUAL "" )
				set( INCLUDES "${INCLUDES}\\\\;${INCLUDE}" )
			else()
				set( INCLUDES "${INCLUDE}" )
			endif()
		endforeach()
		list( APPEND CLI "${INCLUDES}" )
	endif()

	# -o
	if( ARG_OUTPUT )
		list( APPEND CLI "-o" "${ARG_OUTPUT}" )
	endif()

	# --bin2c
	if( ARG_BIN2C )
		list( APPEND CLI "--bin2c" "${ARG_BIN2C}" )
	endif()

	# --depends
	if( ARG_DEPENDS )
		list( APPEND CLI "--depends" )
	endif()

	# --platform
	set( PLATFORM "" )
	set( PLATFORMS "ANDROID;ASM_JS;IOS;LINUX;NACL;OSX;WINDOWS" )
	foreach( P ${PLATFORMS} )
		if( ARG_${P} )
			if( PLATFORM )
				message( SEND_ERROR "Call to shaderc_parse() cannot have both flags ${PLATFORM} and ${P}." )
				return()
			endif()
			set( PLATFORM "${P}" )
		endif()
	endforeach()
	if( "${PLATFORM}" STREQUAL "" )
		message( SEND_ERROR "Call to shaderc_parse() must have a platform flag: ${PLATFORMS}" )
		return()
	elseif( "${PLATFORM}" STREQUAL "ANDROID" )
		list( APPEND CLI "--platform" "android" )
	elseif( "${PLATFORM}" STREQUAL "ASM_JS" )
		list( APPEND CLI "--platform" "asm.js" )
	elseif( "${PLATFORM}" STREQUAL "IOS" )
		list( APPEND CLI "--platform" "ios" )
	elseif( "${PLATFORM}" STREQUAL "LINUX" )
		list( APPEND CLI "--platform" "linux" )
	elseif( "${PLATFORM}" STREQUAL "NACL" )
		list( APPEND CLI "--platform" "nacl" )
	elseif( "${PLATFORM}" STREQUAL "OSX" )
		list( APPEND CLI "--platform" "osx" )
	elseif( "${PLATFORM}" STREQUAL "WINDOWS" )
		list( APPEND CLI "--platform" "windows" )
	endif()

	# --preprocess
	if( ARG_PREPROCESS )
		list( APPEND CLI "--preprocess" )
	endif()

	# --define
	if( ARG_DEFINES )
		list( APPEND CLI "--defines" )
		set( DEFINES "" )
		foreach( DEFINE ${ARG_DEFINES} )
			if( NOT "${DEFINES}" STREQUAL "" )
				set( DEFINES "${DEFINES}\\\\;${DEFINE}" )
			else()
				set( DEFINES "${DEFINE}" )
			endif()
		endforeach()
		list( APPEND CLI "${DEFINES}" )
	endif()

	# --raw
	if( ARG_RAW )
		list( APPEND CLI "--raw" )
	endif()

	# --type
	set( TYPE "" )
	set( TYPES "FRAGMENT;VERTEX;COMPUTE" )
	foreach( T ${TYPES} )
		if( ARG_${T} )
			if( TYPE )
				message( SEND_ERROR "Call to shaderc_parse() cannot have both flags ${TYPE} and ${T}." )
				return()
			endif()
			set( TYPE "${T}" )
		endif()
	endforeach()
	if( "${TYPE}" STREQUAL "" )
		message( SEND_ERROR "Call to shaderc_parse() must have a type flag: ${TYPES}" )
		return()
	elseif( "${TYPE}" STREQUAL "FRAGMENT" )
		list( APPEND CLI "--type" "fragment" )
	elseif( "${TYPE}" STREQUAL "VERTEX" )
		list( APPEND CLI "--type" "vertex" )
	elseif( "${TYPE}" STREQUAL "COMPUTE" )
		list( APPEND CLI "--type" "compute" )
	endif()

	# --varyingdef
	if( ARG_VARYINGDEF )
		list( APPEND CLI "--varyingdef" "${ARG_VARYINGDEF}" )
	endif()

	# --verbose
	if( ARG_VERBOSE )
		list( APPEND CLI "--verbose" )
	endif()

	# --debug
	if( ARG_DEBUG )
		list( APPEND CLI "--debug" )
	endif()

	# --disasm
	if( ARG_DISASM )
		list( APPEND CLI "--disasm" )
	endif()

	# --profile
	if( ARG_PROFILE )
		list( APPEND CLI "--profile" "${ARG_PROFILE}" )
	endif()

	# -O
	if( ARG_O )
		list( APPEND CLI "-O" "${ARG_O}" )
	endif()

	# --Werror
	if( ARG_WERROR )
		list( APPEND CLI "--Werror" )
	endif()

	set( ${ARG_OUT} ${CLI} PARENT_SCOPE )
endfunction()

function(get_profile_ext PROFILE PROFILE_EXT)
	string(REPLACE 120 glsl PROFILE ${PROFILE})
	string(REPLACE spirv spv PROFILE ${PROFILE})
	string(REPLACE metal mtl PROFILE ${PROFILE})
	string(REPLACE vs_3_0 dx9 PROFILE ${PROFILE})
	string(REPLACE vs_4_0 dx10 PROFILE ${PROFILE})
	string(REPLACE vs_5_0 dx11 PROFILE ${PROFILE})
	string(REPLACE ps_3_0 dx9 PROFILE ${PROFILE})
	string(REPLACE ps_4_0 dx10 PROFILE ${PROFILE})
	string(REPLACE ps_5_0 dx11 PROFILE ${PROFILE})
	string(REPLACE cs_4_0 dx10 PROFILE ${PROFILE})
	string(REPLACE cs_5_0 dx11 PROFILE ${PROFILE})

	set(${PROFILE_EXT} ${PROFILE} PARENT_SCOPE)
endfunction()

function(mark_shaders_for_compilation)
	set(options HEADER)
	set(oneValueArgs TYPE VARYING_DEF OUTPUT_DIR)
	set(multiValueArgs SHADERS)
	cmake_parse_arguments(
		ARGS
			"${options}"
			"${oneValueArgs}"
			"${multiValueArgs}"
			"${ARGN}")

	set (PROFILES 120 spirv) # essl pssl
	if (UNIX)
		set (PLATFORM LINUX)
	elseif (EMSCRIPTEN)
		set (PLATFORM ASM_JS)
	elseif (APPLE)
		set (PLATFORM OSX)
		list(APPEND PROFILES metal)
	elseif (WIN32 OR MINGW OR MSYS OR CYGWIN)
		set (PLATFORM WINDOWS)
		if (ARGS_TYPE STREQUAL "VERTEX")
			list(APPEND PROFILES vs_3_0)
			list(APPEND PROFILES vs_4_0)
			list(APPEND PROFILES vs_5_0)
		elseif (ARGS_TYPE STREQUAL "FRAGMENT")
			list(APPEND PROFILES ps_3_0)
			list(APPEND PROFILES ps_4_0)
			list(APPEND PROFILES ps_5_0)
		elseif (ARGS_TYPE STREQUAL "COMPUTE")
			list(APPEND PROFILES cs_4_0)
			list(APPEND PROFILES cs_5_0)
		else ()
			message(error "shaderc: Unsupported type")
		endif ()
	else ()
		message(error "shaderc: Unsupported platform")
	endif ()

	foreach(SHADER_FILE ${ARGS_SHADERS})
		source_group("Shaders" FILES "${SHADER}")
		get_filename_component(SHADER_FILE_BASENAME ${SHADER_FILE} NAME)
		get_filename_component(SHADER_FILE_NAME_WE ${SHADER_FILE} NAME_WE)
		get_filename_component(SHADER_FILE_ABSOLUTE ${SHADER_FILE} ABSOLUTE)

		# Build output targets and their commands
		set(OUTPUTS "")
		set(COMMANDS "")
		foreach(PROFILE ${PROFILES})
			get_profile_ext(${PROFILE} PROFILE_EXT)
			if (ARGS_HEADER)
				set(OUTPUT ${ARGS_OUTPUT_DIR}/${SHADER_FILE_BASENAME}.${PROFILE_EXT}.bin.h)
				set(BIN2C BIN2C ${SHADER_FILE_NAME_WE}_${PROFILE_EXT})
			else()
				set(OUTPUT ${ARGS_OUTPUT_DIR}/${SHADER_FILE_BASENAME}.${PROFILE_EXT})
				set(BIN2C "")
			endif()
			set(PLATFORM_I ${PLATFORM})
			if (PROFILE STREQUAL "spirv")
				set(PLATFORM_I LINUX)
			endif()
			shaderc_parse(
				CLI
				FILE ${SHADER_FILE_ABSOLUTE}
				OUTPUT ${OUTPUT}
				${ARGS_TYPE}
				${PLATFORM_I}
				PROFILE ${PROFILE}
				O "$<$<CONFIG:debug>:0>$<$<CONFIG:release>:3>$<$<CONFIG:relwithdebinfo>:3>$<$<CONFIG:minsizerel>:3>"
				VARYINGDEF ${ARGS_VARYING_DEF}
				INCLUDES ${BGFX_SHADER_INCLUDE_PATH}
				VERBOSE
				${BIN2C}
				WERROR
			)
			list(APPEND OUTPUTS ${OUTPUT})
			list(APPEND COMMANDS COMMAND bgfx::shaderc ${CLI})
		endforeach()

		add_custom_command(
			OUTPUT ${OUTPUTS}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${ARGS_OUTPUT_DIR}
			${COMMANDS}
			MAIN_DEPENDENCY ${SHADER_FILE_ABSOLUTE}
			DEPENDS ${ARGS_VARYING_DEF}
		)
	endforeach()
endfunction()
