/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "GestureFile.h"

#include <cstring>

#include <algorithm>
#include <bit>
#include <fstream>
#include <iterator>

using namespace openblack::gestures;

namespace
{
constexpr size_t k_PointsBytes = GestureTemplate::k_MaxPoints * sizeof(TemplatePoint);

template <typename T>
T ReadValue(std::span<const uint8_t> bytes, size_t offset)
{
	T value;
	std::memcpy(&value, bytes.data() + offset, sizeof(T));
	return value;
}

template <typename T>
void WriteValue(std::vector<uint8_t>& bytes, const T& value)
{
	const auto* begin = reinterpret_cast<const uint8_t*>(&value);
	bytes.insert(bytes.end(), begin, begin + sizeof(T));
}
} // namespace

std::string_view openblack::gestures::ResultToStr(GestureFileResult result)
{
	switch (result)
	{
	case GestureFileResult::Success:
		return "Success";
	case GestureFileResult::ErrCantOpen:
		return "Can't open the file";
	case GestureFileResult::ErrFileTooSmall:
		return "The file is too small to hold its count of templates";
	case GestureFileResult::ErrSizeMismatch:
		return "The file's size doesn't match its count of templates";
	case GestureFileResult::ErrTooManyPoints:
		return "A template has more points than it has room for";
	}
	return "Unknown";
}

size_t GestureTemplate::PointCount() const
{
	return std::min<size_t>(pointCountField & 0xFFu, k_MaxPoints);
}

std::span<const TemplatePoint> GestureTemplate::Points() const
{
	return std::span(points).first(PointCount());
}

GestureFileResult GestureFile::Open(const std::filesystem::path& path)
{
	std::ifstream stream(path, std::ios::binary);
	if (!stream)
	{
		return GestureFileResult::ErrCantOpen;
	}
	const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
	return Open(bytes);
}

GestureFileResult GestureFile::Open(std::span<const uint8_t> bytes)
{
	_templates.clear();
	if (bytes.size() < sizeof(int32_t))
	{
		return GestureFileResult::ErrFileTooSmall;
	}
	const auto count = ReadValue<int32_t>(bytes, 0);
	if (count < 0 || bytes.size() != sizeof(int32_t) + (static_cast<size_t>(count) * k_RecordSize))
	{
		return GestureFileResult::ErrSizeMismatch;
	}
	_templates.reserve(static_cast<size_t>(count));
	for (size_t i = 0; i < static_cast<size_t>(count); ++i)
	{
		const auto record = bytes.subspan(sizeof(int32_t) + (i * k_RecordSize), k_RecordSize);
		GestureTemplate entry;
		std::memcpy(entry.points.data(), record.data(), k_PointsBytes);
		size_t offset = k_PointsBytes;
		const auto next = [&record, &offset]() {
			const auto value = ReadValue<uint32_t>(record, offset);
			offset += sizeof(uint32_t);
			return value;
		};
		entry.pointCountField = next();
		entry.gestureField = next();
		entry.positionModeField = next();
		entry.checkDirection = next();
		entry.allowMirror = next();
		entry.checkAspectRatio = next();
		entry.aspectRatio = std::bit_cast<float>(next());
		if ((entry.pointCountField & 0xFFu) > GestureTemplate::k_MaxPoints)
		{
			_templates.clear();
			return GestureFileResult::ErrTooManyPoints;
		}
		_templates.push_back(entry);
	}
	return GestureFileResult::Success;
}

std::vector<uint8_t> GestureFile::Write() const
{
	std::vector<uint8_t> bytes;
	bytes.reserve(sizeof(int32_t) + (_templates.size() * k_RecordSize));
	WriteValue(bytes, static_cast<int32_t>(_templates.size()));
	for (const auto& entry : _templates)
	{
		const auto* points = reinterpret_cast<const uint8_t*>(entry.points.data());
		bytes.insert(bytes.end(), points, points + k_PointsBytes);
		WriteValue(bytes, entry.pointCountField);
		WriteValue(bytes, entry.gestureField);
		WriteValue(bytes, entry.positionModeField);
		WriteValue(bytes, entry.checkDirection);
		WriteValue(bytes, entry.allowMirror);
		WriteValue(bytes, entry.checkAspectRatio);
		WriteValue(bytes, entry.aspectRatio);
	}
	return bytes;
}

GestureFileResult GestureFile::Write(const std::filesystem::path& path) const
{
	std::ofstream stream(path, std::ios::binary);
	if (!stream)
	{
		return GestureFileResult::ErrCantOpen;
	}
	const auto bytes = Write();
	stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
	return stream ? GestureFileResult::Success : GestureFileResult::ErrCantOpen;
}
