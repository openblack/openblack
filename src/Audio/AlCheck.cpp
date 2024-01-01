/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "AlCheck.h"

#include <spdlog/spdlog.h>

extern "C" {
#include <AL/al.h>
}

void openblack::audio::AlErrorCheck(std::string message, std::string file, uint64_t line)
{
	std::string errorMessage;
	const ALenum error = alGetError();
	switch (error)
	{
	case AL_NO_ERROR:
		return;
	case AL_INVALID_NAME:
		errorMessage = "AL_INVALID_NAME: a bad name (ID) was passed to an OpenAL function";
		break;
	case AL_INVALID_ENUM:
		errorMessage = "AL_INVALID_ENUM: an invalid enum value was passed to an OpenAL function";
		break;
	case AL_INVALID_VALUE:
		errorMessage = "AL_INVALID_VALUE: an invalid value was passed to an OpenAL function";
		break;
	case AL_INVALID_OPERATION:
		errorMessage = "AL_INVALID_OPERATION: the requested operation is not valid";
		break;
	case AL_OUT_OF_MEMORY:
		errorMessage = "AL_OUT_OF_MEMORY: the requested operation resulted in OpenAL running out of memory";
		break;
	default:
		errorMessage = "UNKNOWN AL ERROR: " + std::to_string(error);
	}

	SPDLOG_LOGGER_ERROR(spdlog::get("audio"), R"(OpenAL error: {} with call "{}" at file "{}" on line {})", errorMessage,
	                    message, file, line);
}
