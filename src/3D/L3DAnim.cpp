/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "L3DAnim.h"

#include <ANMFile.h>
#include <glm/gtx/matrix_interpolation.hpp>
#include <spdlog/spdlog.h>

#include "Common/FileSystem.h"
#include "Game.h"

using namespace openblack;

void L3DAnim::Load(const anm::ANMFile& anm)
{
	_name = anm.GetHeader().name;
	_unknown_0x20 = anm.GetHeader().unknown_0x20;
	_unknown_0x24 = anm.GetHeader().unknown_0x24;
	_unknown_0x28 = anm.GetHeader().unknown_0x28;
	_unknown_0x2C = anm.GetHeader().unknown_0x2C;
	_unknown_0x30 = anm.GetHeader().unknown_0x30;
	_unknown_0x34 = anm.GetHeader().unknown_0x34;
	assert(anm.GetHeader().frame_count == anm.GetKeyframes().size());
	_unknown_0x3C = anm.GetHeader().unknown_0x3C;
	_duration = anm.GetHeader().animation_duration;
	_unknown_0x44 = anm.GetHeader().unknown_0x44;
	_unknown_0x48 = anm.GetHeader().unknown_0x48;
	_unknown_0x50 = anm.GetHeader().unknown_0x50;

	_frames.reserve(anm.GetKeyframes().size());
	for (const auto& keyframe : anm.GetKeyframes())
	{
		auto& frame = _frames.emplace_back();
		frame.time = keyframe.time;
		frame.bones.resize(keyframe.bones.size());
		for (uint32_t i = 0; i < keyframe.bones.size(); ++i)
		{
			// clang-format off
			auto matrix = glm::mat4(keyframe.bones[i].matrix[0], keyframe.bones[i].matrix[1], keyframe.bones[i].matrix[2], 0.0f,
			                        keyframe.bones[i].matrix[3], keyframe.bones[i].matrix[4], keyframe.bones[i].matrix[5], 0.0f,
			                        keyframe.bones[i].matrix[6], keyframe.bones[i].matrix[7], keyframe.bones[i].matrix[8], 0.0f,
			                        keyframe.bones[i].matrix[9], keyframe.bones[i].matrix[10], keyframe.bones[i].matrix[11], 1.0f);
			// clang-format on
			frame.bones[i] = matrix;
		}
	}
}

bool L3DAnim::LoadFromFile(const fs::path& path)
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading L3DAnim from file: {}", path.generic_string());
	anm::ANMFile anm;

	try
	{
		anm.Open(Game::instance()->GetFileSystem().FindPath(path).u8string());
	}
	catch (std::runtime_error& err)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to open l3d mesh from filesystem {}: {}", path.generic_string(),
		                    err.what());
		return false;
	}

	Load(anm);

	return true;
}

void L3DAnim::LoadFromBuffer(const std::vector<uint8_t>& data)
{
	anm::ANMFile anm;

	try
	{
		anm.Open(data);
	}
	catch (std::runtime_error& err)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to open l3d animation from buffer: {}", err.what());
		return;
	}

	Load(anm);
}

const std::vector<glm::mat4> L3DAnim::GetBoneMatrices(uint32_t time) const
{
	if (_frames.empty())
	{
		return {};
	}
	if (_duration == 0)
	{
		return _frames[0].bones;
	}
	uint32_t animation_time = time % _duration;
	uint32_t index = 0;
	uint32_t previous_time = 0;
	for (const auto& frame : _frames)
	{
		if (frame.time < animation_time)
		{
			previous_time = frame.time;
			index++;
		}
		else
		{
			break;
		}
	}
	// No interpolation needed
	if (index == 0)
	{
		return _frames[0].bones;
	}
	else if (index >= _frames.size())
	{
		return _frames[_frames.size() - 1].bones;
	}
	float t = static_cast<float>(animation_time - previous_time) / (_frames[index].time - previous_time);

	// Interpolate
	std::vector<glm::mat4> bones;
	bones.resize(_frames[index].bones.size());
	for (uint32_t i = 0; i < bones.size(); ++i)
	{
		// Doing matrix interpolation is not ideal. Would prefer quaternions but
		// extracting a quaternion from a glm::mat4 does not reconstruct the
		// same matrix from the quaternion.
		bones[i] = glm::mat4(glm::mix(_frames[index - 1].bones[i][0], _frames[index].bones[i][0], t),
		                     glm::mix(_frames[index - 1].bones[i][1], _frames[index].bones[i][1], t),
		                     glm::mix(_frames[index - 1].bones[i][2], _frames[index].bones[i][2], t),
		                     glm::mix(_frames[index - 1].bones[i][3], _frames[index].bones[i][3], t));
	}

	// assert(index < _frames.size());
	return bones;
}
