/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

/*
 * The layout of a Morph Spec Text File is as follows:
 *
 * - 1 line with an int representing the spec version
 * - 1 line category name per line starting with the '=' character
 * - 1 line with the character E indicating the end of the categories
 * - 1 line animation per remaining line where:
 *         the first character is the type
 *         the following characters form the name
 *         each animation belongs to the latest category line parsed
 *
 * The layout of a Morph Block is as follows:
 *
 * - 236 byte header
 *         1 unknown 32-bit int, TODO: is 0 for hands and 21 for creatures
 *         version of the morph spec file to be parsed
 *         binary version
 *         base mesh name
 *         6 optional variant mesh names (evil, good, thin, fat, weak, strong)
 *
 * ------------------------ start of animation offsets block ------------------
 *
 * - 4 bytes * number of animations * (1 + min(4, number of variants)) from
 *   specs file, each record contains:
 *         offset into animation block (see below)
 * - 4 byte offset to the next block (animation or hair groups when last)
 *
 * ------------------------ start of animation block --------------------------
 *
 * - 44 bytes header containing:
 *         duration in milliseconds
 *         1 when the animation is a cycle (type C), 0 for a pose range (type L)
 *         5 unknown floats, TODO: likely same role as as in ANMHeader
 *         frame count as a 32-bit int
 *         mesh bone count as a 32-bit int
 *         count of joint rotation joints as a 32-bit int
 *         count of joint translation joints as a 32-bit int
 * - 4 bytes * count of joint rotation joints containing:
 *         joint index of the target mesh
 * - 4 bytes * count of joint translation joints containing:
 *         joint index of the target mesh
 * - frame count Animation Frames, each containing:
 *          count of joint rotation joints * 12 byte euler angles
 *          count of joint translation joints * 12 byte vector
 *
 * ------------------------ start of hair group block -------------------------
 *
 * - 4 bytes 1 unknown int used if the binary version is greater than 4
 * - 4 bytes containing the number of hair groups
 * - for each group, a 100 byte header containing:
 *         segments version
 *         hair count
 *         segment count (points per strand)
 *         mapping index (1 when drawn with the hair texture)
 *         neutral, evil and good looks of 28 bytes each: red, green, blue
 *         as ints 0 to 255, then length, damping, stiffness and thickness
 *   followed by its 76 byte hairs, each containing:
 *         flags (bit 0: turned by its angles)
 *         36 byte point on a triangle of the base mesh
 *         neutral, evil and good x, y and z angles
 * - 4 byte offset to the next block (extra)
 *
 * ------------------------ start of extra animation block --------------------
 *
 * - 16 byte structure * number of non-zero offsets in the base animation
 *   block, containing:
 *         4 unknown 32-bit data, TODO
 *
 */

#include "MorphFile.h"

#include <cassert>
#include <cstring>

#include <fstream>
#include <utility>
#include <vector>

using namespace openblack::morph;

namespace
{
// Adapted from https://stackoverflow.com/a/13059195/10604387
//          and https://stackoverflow.com/a/46069245/10604387
struct membuf: std::streambuf
{
	membuf(char const* base, size_t size)
	{
		char* p(const_cast<char*>(base));
		this->setg(p, p, p + size);
	}
	std::streampos seekoff(off_type off, std::ios_base::seekdir way, [[maybe_unused]] std::ios_base::openmode which) override
	{
		if (way == std::ios_base::cur)
		{
			gbump(static_cast<int>(off));
		}
		else if (way == std::ios_base::end)
		{
			setg(eback(), egptr() + off, egptr());
		}
		else if (way == std::ios_base::beg)
		{
			setg(eback(), eback() + off, egptr());
		}
		return gptr() - eback();
	}

	std::streampos seekpos([[maybe_unused]] pos_type pos, [[maybe_unused]] std::ios_base::openmode which) override
	{
		return seekoff(pos - static_cast<off_type>(0), std::ios_base::beg, which);
	}
};
struct imemstream: virtual membuf, std::istream
{
	imemstream(char const* base, size_t size)
	    : membuf(base, size)
	    , std::istream(dynamic_cast<std::streambuf*>(this))
	{
	}
};

// https://stackoverflow.com/questions/6089231/getting-std-ifstream-to-handle-lf-cr-and-crlf
std::istream& safe_getline(std::istream& is, std::string& t)
{
	t.clear();

	// The characters in the stream are read one-by-one using a std::streambuf.
	// That is faster than reading them one-by-one using the std::istream.
	// Code that uses streambuf this way must be guarded by a sentry object.
	// The sentry object performs various tasks,
	// such as thread synchronization and updating the stream state.

	std::istream::sentry se(is, true);
	std::streambuf* sb = is.rdbuf();

	for (;;)
	{
		const int c = sb->sbumpc();
		switch (c)
		{
		case '\n':
			return is;
		case '\r':
			if (sb->sgetc() == '\n')
			{
				sb->sbumpc();
			}
			return is;
		case std::streambuf::traits_type::eof():
			// Also handle the case when the last line has no line ending
			if (t.empty())
			{
				is.setstate(std::ios::eofbit);
			}
			return is;
		default:
			t += static_cast<char>(c);
		}
	}
}
} // namespace

std::string_view openblack::morph::ResultToStr(MorphResult result)
{
	switch (result)
	{
	case MorphResult::Success:
		return "Success";
	case MorphResult::ErrCantOpen:
		return "Could not open file.";
	case MorphResult::ErrFileTooSmall:
		return "File too small to be a valid Morph file.";
	case MorphResult::ErrSpecFileCantOpen:
		return "Could not open file spec file.";
	case MorphResult::ErrSpecFileVersionMismatch:
		return "Spec file version mismatch.";
	case MorphResult::ErrSpecFileAnimationsBeforeCategories:
		return "Spec file has animations before categories.";
	}
	std::unreachable();
}

MorphFile::MorphFile() noexcept = default;

MorphFile::~MorphFile() noexcept = default;

MorphResult MorphFile::ReadSpecFile(const std::filesystem::path& specFilePath) noexcept
{
	assert(!_isLoaded);

	_animationSpecs = {};
	_animationSpecs.path = specFilePath;
	std::ifstream spec(_animationSpecs.path);
	if (!spec.good())
	{
		return MorphResult::ErrSpecFileCantOpen;
	}
	std::string line;
	spec >> _animationSpecs.version >> std::ws;
	if (_header.specFileVersion != _animationSpecs.version)
	{
		return MorphResult::ErrSpecFileVersionMismatch;
	}

	[[maybe_unused]] bool reachedEnd = false;
	while (!spec.eof())
	{
		char type;
		spec.get(type);
		// End of file
		if (type == 'E')
		{
			reachedEnd = true;
			break;
		}
		safe_getline(spec, line);
		if (type == '=')
		{
			_animationSpecs.animationSets.emplace_back().name = line;
		}
		else
		{
			if (_animationSpecs.animationSets.empty())
			{
				return MorphResult::ErrSpecFileAnimationsBeforeCategories;
			}
			_animationSpecs.animationSets.back().animations.emplace_back(
			    AnimationDesc {line, static_cast<AnimationType>(type)});
		}
	}
	assert(reachedEnd);

	return MorphResult::Success;
}

std::vector<Animation> MorphFile::ReadAnimations(std::istream& stream, const std::vector<uint32_t>& offsets) noexcept
{
	assert(!_isLoaded);

	std::vector<Animation> animations;

	uint32_t i = 0;
	for (auto& animSet : _animationSpecs.animationSets)
	{
		for (const auto& animDesc : animSet.animations)
		{
			if (offsets[i] > 0)
			{
				stream.seekg(offsets[i]);
				auto& animation = animations.emplace_back();
				// The spec stores the node type (C/L) as the first character, followed by the name.
				animation.name = static_cast<char>(animDesc.type) + animDesc.name;
				animation.setName = animSet.name;
				stream.read(reinterpret_cast<char*>(&animation.header), sizeof(animation.header));

				animation.rotatedJointIndices.resize(animation.header.rotatedJointCount);
				stream.read(reinterpret_cast<char*>(animation.rotatedJointIndices.data()),
				            animation.rotatedJointIndices.size() * sizeof(animation.rotatedJointIndices[0]));

				animation.translatedJointIndices.resize(animation.header.translatedJointCount);
				stream.read(reinterpret_cast<char*>(animation.translatedJointIndices.data()),
				            animation.translatedJointIndices.size() * sizeof(animation.translatedJointIndices[0]));

				animation.keyframes.resize(animation.header.frameCount);
				for (auto& frame : animation.keyframes)
				{
					frame.eulerAngles.resize(animation.header.rotatedJointCount);
					stream.read(reinterpret_cast<char*>(frame.eulerAngles.data()),
					            frame.eulerAngles.size() * sizeof(frame.eulerAngles[0]));

					frame.translations.resize(animation.header.translatedJointCount);
					stream.read(reinterpret_cast<char*>(frame.translations.data()),
					            frame.translations.size() * sizeof(frame.translations[0]));
				}
			}
			i++;
		}
	}

	return animations;
}

void MorphFile::ReadCreatureBlock(std::istream& stream) noexcept
{
	// The creature block's version is the header's first field. Each field below came in at a version, and the eyes
	// at version 14.
	const auto version = _header.unknown0x0;
	constexpr uint32_t k_EyesVersion = 14;
	if (version < k_EyesVersion)
	{
		return;
	}

	const auto skip = [&stream](std::streamoff bytes) { stream.seekg(bytes, std::ios_base::cur); };
	constexpr std::streamoff k_Field = sizeof(uint32_t);
	// Older morph data ends with a name
	if (_header.binaryVersion <= 5)
	{
		skip(0x20);
	}
	// The creature's bones and sizes that come before the eyes, as many as the version has
	std::streamoff fields = 2 + 1 + 1 + 1 + 1 + 4;
	fields += version > 2 ? 2 : 0;
	fields += version > 11 ? 1 : 0;
	fields += version > 4 ? 2 : 0;
	fields += version > 8 ? 2 : 0;
	fields += version > 9 ? 1 : 0;
	fields += version > 15 ? 1 : 0;
	fields += version > 7 ? 1 : 0;
	skip(fields * k_Field);
	// Then up to two points on the body, each after whether it is there
	const auto skipOptionalPoint = [&stream, &skip]() {
		uint32_t present = 0;
		stream.read(reinterpret_cast<char*>(&present), sizeof(present));
		if (present != 0)
		{
			skip(sizeof(MeshIntersect));
		}
	};
	if (version > 7)
	{
		skipOptionalPoint();
	}
	if (version > 17)
	{
		skipOptionalPoint();
	}

	CreatureEyes eyes {};
	stream.read(reinterpret_cast<char*>(&eyes.scale), sizeof(eyes.scale));
	for (auto& point : eyes.points)
	{
		uint32_t enabled = 0;
		stream.read(reinterpret_cast<char*>(&point.intersect), sizeof(point.intersect));
		stream.read(reinterpret_cast<char*>(&enabled), sizeof(enabled));
		stream.read(reinterpret_cast<char*>(&point.depth), sizeof(point.depth));
		point.enabled = enabled != 0;
	}
	// Stored a column at a time
	for (size_t k = 0; k < 3; ++k)
	{
		for (auto& row : eyes.lidAngles)
		{
			stream.read(reinterpret_cast<char*>(&row.at(k)), sizeof(float));
		}
	}
	if (stream.good())
	{
		_creatureEyes = eyes;
		ReadTattooSites(stream, version);
	}
}

void MorphFile::ReadTattooSites(std::istream& stream, uint32_t version) noexcept
{
	// Each field below came in at a version, and the tattoo sites at version 15
	constexpr uint32_t k_TattooVersion = 15;
	if (version < k_TattooVersion)
	{
		return;
	}
	const auto skip = [&stream](std::streamoff bytes) { stream.seekg(bytes, std::ios_base::cur); };
	constexpr std::streamoff k_Field = sizeof(uint32_t);
	// The sound bank's name, then seven sizes, then twelve pairs or triples of numbers
	if (version > 18)
	{
		skip(0x20);
	}
	skip((version < 11 ? 1 : 7) * k_Field);
	skip(12 * (version > 12 ? 3 : 2) * k_Field);

	TattooSites sites {};
	for (auto& site : sites)
	{
		site = {};
		uint32_t enabled = 0;
		stream.read(reinterpret_cast<char*>(&enabled), sizeof(enabled));
		if (enabled == 0)
		{
			continue;
		}
		// The last byte of the packed place is left over
		std::array<uint8_t, 4> packed {};
		stream.read(reinterpret_cast<char*>(packed.data()), packed.size());
		stream.read(reinterpret_cast<char*>(&site.size), sizeof(site.size));
		site.enabled = true;
		site.u = packed[0];
		site.v = packed[1];
		site.skin = static_cast<uint8_t>(packed[2] >> 6u);
		if (version > 16)
		{
			uint32_t mirror = 0;
			stream.read(reinterpret_cast<char*>(&mirror), sizeof(mirror));
			stream.read(reinterpret_cast<char*>(&site.rotation), sizeof(site.rotation));
			site.mirror = mirror != 0;
			site.rotation &= 3u;
		}
	}
	if (stream.good())
	{
		_tattooSites = sites;
	}
}

HairGroup MorphFile::ReadHairGroup(std::istream& stream) noexcept
{
	HairGroup hairGroup;
	stream.read(reinterpret_cast<char*>(&hairGroup.header), sizeof(hairGroup.header));
	hairGroup.hairs.resize(hairGroup.header.hairCount);
	stream.read(reinterpret_cast<char*>(hairGroup.hairs.data()), hairGroup.hairs.size() * sizeof(hairGroup.hairs[0]));
	return hairGroup;
}

MorphResult MorphFile::ReadFile(std::istream& stream, const std::filesystem::path& specsDirectory) noexcept
{
	assert(!_isLoaded);

	// Total file size
	std::size_t fsize = 0;
	if (stream.seekg(0, std::ios_base::end))
	{
		fsize = static_cast<std::size_t>(stream.tellg());
		stream.seekg(0);
	}

	if (fsize < sizeof(MorphHeader))
	{
		return MorphResult::ErrFileTooSmall;
	}

	// First 236 bytes
	stream.read(reinterpret_cast<char*>(&_header), sizeof(MorphHeader));

	assert(_header.binaryVersion > 4); // structure is much different below v5

	// Parse spec file (a separate text file) using the version
	std::string specName;
	// this field is a good guess for hand or not, but a better choice might be
	// getting the segment name from pack
	if (_header.unknown0x0 != 0u)
	{
		specName = "ctrspec" + std::to_string(_header.specFileVersion) + ".txt";
	}
	else
	{
		specName = "hndspec" + std::to_string(_header.specFileVersion) + ".txt";
	}
	const auto specResult = ReadSpecFile(specsDirectory / specName);
	if (specResult != MorphResult::Success)
	{
		return specResult;
	}
	size_t numAnimations = 0;
	for (auto& animSet : _animationSpecs.animationSets)
	{
		numAnimations += animSet.animations.size();
	}

	// After the header is the anim set, a variable length array of offsets relative to the section offset
	std::vector<uint32_t> animationOffsets(numAnimations);
	stream.read(reinterpret_cast<char*>(animationOffsets.data()), animationOffsets.size() * sizeof(animationOffsets[0]));

	// Following the animation offsets are chained offsets which can lead to extra data
	uint32_t extraOffset = 0;
	stream.read(reinterpret_cast<char*>(&extraOffset), sizeof(extraOffset));

	// Read in the base animations using those offsets
	_baseAnimation = ReadAnimations(stream, animationOffsets);
	_baseAnimationIndices.resize(numAnimations, -1);
	int32_t baseIndex = 0;
	for (size_t i = 0; i < numAnimations; ++i)
	{
		if (animationOffsets[i] > 0)
		{
			_baseAnimationIndices[i] = baseIndex++;
		}
	}

	// Creature files have different animations for the morph meshes (evil, good, thin, fat) weak, strong are skipped
	for (uint32_t i = 0; i < 4; ++i)
	{
		if (std::strlen(_header.variantMeshNames.at(i).data()) > 0)
		{
			// Set file to next animation set
			stream.seekg(extraOffset);

			std::vector<uint32_t> variantAnimationOffsets(numAnimations);
			stream.read(reinterpret_cast<char*>(variantAnimationOffsets.data()),
			            variantAnimationOffsets.size() * sizeof(variantAnimationOffsets[0]));

			// Again, the get pointer to the next part
			stream.read(reinterpret_cast<char*>(&extraOffset), sizeof(extraOffset));

			_variantAnimations.at(i) = ReadAnimations(stream, variantAnimationOffsets);
			auto& indices = _variantAnimationIndices.at(i);
			indices.assign(numAnimations, -1);
			int32_t variantIndex = 0;
			for (size_t j = 0; j < numAnimations; ++j)
			{
				if (variantAnimationOffsets[j] > 0)
				{
					indices[j] = variantIndex++;
				}
			}
		}
	}

	// Once all the animation sets are loaded, the extra offset points to hair groups data (even if there are none)
	stream.seekg(extraOffset);
	stream.read(reinterpret_cast<char*>(&_hairHeader), sizeof(_hairHeader));
	for (uint32_t i = 0; i < _hairHeader.hairGroupCount; ++i)
	{
		_hairGroups.emplace_back(ReadHairGroup(stream));
	}

	// The extra data segment is in relation to the number of animations in the base animation set
	_extraData.resize(numAnimations);
	for (size_t i = 0; i < numAnimations; ++i)
	{
		if (animationOffsets[i] == 0)
		{
			continue;
		}
		uint32_t hasData; // TODO(#467): unknown if this serves another function
		while (stream.read(reinterpret_cast<char*>(&hasData), sizeof(hasData)).good() && hasData != 0u)
		{
			auto& data = _extraData[i].emplace_back();
			stream.read(reinterpret_cast<char*>(&data), sizeof(data));
		}
	}

	// Creature files go on with the creature's own block
	if (_header.unknown0x0 != 0u)
	{
		ReadCreatureBlock(stream);
	}

	_isLoaded = true;

	return MorphResult::Success;
}

MorphResult MorphFile::Open(const std::filesystem::path& filepath, const std::filesystem::path& specsDirectory) noexcept
{
	assert(!_isLoaded);

	std::ifstream stream(filepath, std::ios::binary);

	if (!stream.is_open())
	{
		return MorphResult::ErrCantOpen;
	}

	return ReadFile(stream, specsDirectory);
}

MorphResult MorphFile::Open(const std::vector<uint8_t>& buffer, const std::filesystem::path& specsDirectory) noexcept
{
	assert(!_isLoaded);

	imemstream stream(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(buffer[0]));

	return ReadFile(stream, specsDirectory);
}
