/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace openblack::morph
{

struct MorphHeader
{
	uint32_t unknown0x0; // TODO(#458): is 0 for hands and 21 for creatures
	uint32_t specFileVersion;
	uint32_t binaryVersion;
	std::array<char, 0x20> baseMeshName;
	std::array<std::array<char, 0x20>, 6> variantMeshNames;
};
static_assert(sizeof(MorphHeader) == 0xec);

struct AnimationHeader
{
	uint32_t unknown0x0; // TODO(#459): possibly duration or offset
	uint32_t unknown0x4; // TODO(#459): either 0 or 1. Seem to be 1 when type C and 0 when not
	float unknown0x8;    // TODO(#459):
	float unknown0xc;    // TODO(#459):
	float unknown0x10;   // TODO(#459):
	float unknown0x14;   // TODO(#459):
	float unknown0x18;   // TODO(#459):
	uint32_t frameCount;
	uint32_t meshBoneCount;
	uint32_t rotatedJointCount;
	uint32_t translatedJointCount;
};
static_assert(sizeof(AnimationHeader) == 0x2c);

struct HairHeader
{
	uint32_t unknown0x0; // TODO(#460):
	uint32_t hairGroupCount;
};
static_assert(sizeof(HairHeader) == 0x8);

// TODO(#461): Function unknown
struct HairGroupHeaderMember
{
	uint32_t unknown0x0; // TODO(#461)
	uint32_t unknown0x4; // TODO(#461)
	uint32_t unknown0x8; // TODO(#461)
	float unknown0xc;    // TODO(#461)
	float unknown0x10;   // TODO(#461)
	float unknown0x14;   // TODO(#461)
	float unknown0x18;   // TODO(#461)
};
static_assert(sizeof(HairGroupHeaderMember) == 0x1c);

struct HairGroupHeader
{
	uint32_t unknown0x0; // TODO(#462)
	uint32_t hairCount;
	uint32_t unknown0x8; // TODO(#462): some count
	uint32_t unknown0xc; // TODO(#462)
	std::array<HairGroupHeaderMember, 3> unknown0x10;
};
static_assert(sizeof(HairGroupHeader) == 0x64);

struct HairIntersection
{
	uint32_t unknown0x0;  // TODO(#463)
	uint32_t unknown0x4;  // TODO(#463)
	uint32_t unknown0x8;  // TODO(#463)
	uint32_t unknown0xc;  // TODO(#463)
	uint32_t unknown0x10; // TODO(#463)
	uint32_t unknown0x14; // TODO(#463)
	uint32_t unknown0x18; // TODO(#463)
	float unknown0x1c;    // TODO(#463)
	float unknown0x20;    // TODO(#463)
};
static_assert(sizeof(HairIntersection) == 0x24);

struct Hair
{
	uint32_t unknown0x0; // TODO(#464)
	HairIntersection intersection;
	std::array<float, 3> xs;
	std::array<float, 3> ys;
	std::array<float, 3> zs;
};
static_assert(sizeof(Hair) == 0x4c);

struct ExtraData
{
	uint32_t unknown0x0; // TODO(#465)
	uint32_t unknown0x4; // TODO(#465)
	uint32_t unknown0x8; // TODO(#465)
	uint32_t unknown0xc; // TODO(#465)
};
static_assert(sizeof(ExtraData) == 0x10);

enum class AnimationType : char
{
	// TODO(#466): figure out types and their functions and convert, remove char underlying type
};

struct AnimationDesc
{
	std::string name;
	AnimationType type;
};

struct AnimationSetDesc
{
	std::string name;
	std::vector<AnimationDesc> animations;
};

struct AnimationSpecs
{
	std::filesystem::path path;
	uint32_t version;
	std::vector<AnimationSetDesc> animationSets;
};

struct AnimationFrame
{
	std::vector<std::array<float, 3>> eulerAngles;
	std::vector<std::array<float, 3>> translations;
};

struct Animation
{
	AnimationHeader header;
	std::vector<uint32_t> rotatedJointIndices;
	std::vector<uint32_t> translatedJointIndices;
	std::vector<AnimationFrame> keyframes;
};

struct HairGroup
{
	HairGroupHeader header;
	std::vector<Hair> hairs;
};

/**
  This class is used to read the Creature block of CBN and the Hand block of HBN files.
 */
class MorphFile
{
protected:
	/// True when a file has been loaded
	bool _isLoaded {false};

	std::filesystem::path _filename;

	MorphHeader _header;
	AnimationSpecs _animationSpecs;
	std::vector<Animation> _baseAnimation;
	std::array<std::vector<Animation>, 4> _variantAnimations; // last 2 mesh variants don't have animations
	HairHeader _hairHeader;
	std::vector<HairGroup> _hairGroups;
	std::vector<std::vector<ExtraData>> _extraData; ///< related to \ref _base_animation

	/// Error handling
	void Fail(const std::string& msg);

	/// Read file from the input source
	virtual void ReadFile(std::istream& stream, const std::filesystem::path& specsDirectory);
	void ReadSpecFile(const std::filesystem::path& specFilePath);
	std::vector<Animation> ReadAnimations(std::istream& stream, const std::vector<uint32_t>& offsets);
	HairGroup ReadHairGroup(std::istream& stream);

public:
	MorphFile();
	virtual ~MorphFile();

	/// Read morph file from the filesystem
	void Open(const std::filesystem::path& filepath, const std::filesystem::path& specsDirectory);

	/// Read morph file from a buffer
	void Open(const std::vector<uint8_t>& buffer, const std::filesystem::path& specsDirectory);

	[[nodiscard]] std::string GetFilename() const { return _filename.string(); }
	[[nodiscard]] const MorphHeader& GetHeader() const { return _header; }
	[[nodiscard]] const AnimationSpecs& GetAnimationSpecs() const { return _animationSpecs; }
	[[nodiscard]] const std::vector<Animation>& GetBaseAnimationSet() const { return _baseAnimation; }
	[[nodiscard]] const std::vector<Animation>& GetVariantAnimationSet(uint32_t index) const
	{
		return _variantAnimations.at(index);
	}
	[[nodiscard]] const std::vector<HairGroup>& GetHairGroups() const { return _hairGroups; }
	[[nodiscard]] const std::vector<std::vector<ExtraData>>& GetExtraData() const { return _extraData; }
};

} // namespace openblack::morph
