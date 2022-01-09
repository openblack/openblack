/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <array>
#include <string>
#include <vector>

namespace openblack::morph
{

struct MorphHeader
{
	uint32_t unknown_0x0; // TODO: is 0 for hands and 21 for creatures
	uint32_t spec_file_version;
	uint32_t binary_version;
	std::array<char, 0x20> base_mesh_name;
	std::array<std::array<char, 0x20>, 6> variant_mesh_names;
};
static_assert(sizeof(MorphHeader) == 0xec);

struct AnimationHeader
{
	uint32_t unknown_0x0; // TODO: possibly duration or offset
	uint32_t unknown_0x4; // TODO: either 0 or 1. Seem to be 1 when type C and 0 when not
	float unknown_0x8;    // TODO:
	float unknown_0xc;    // TODO:
	float unknown_0x10;   // TODO:
	float unknown_0x14;   // TODO:
	float unknown_0x18;   // TODO:
	uint32_t frame_count;
	uint32_t mesh_bone_count;
	uint32_t rotated_joint_count;
	uint32_t translated_joint_count;
};
static_assert(sizeof(AnimationHeader) == 0x2c);

struct HairHeader
{
	uint32_t unknown_0x0; // TODO:
	uint32_t hair_group_count;
};
static_assert(sizeof(HairHeader) == 0x8);

/// TODO: Function unknown
struct HairGroupHeaderMember
{
	uint32_t unknown_0x0; // TODO
	uint32_t unknown_0x4; // TODO
	uint32_t unknown_0x8; // TODO
	float unknown_0xc;    // TODO
	float unknown_0x10;   // TODO
	float unknown_0x14;   // TODO
	float unknown_0x18;   // TODO
};
static_assert(sizeof(HairGroupHeaderMember) == 0x1c);

struct HairGroupHeader
{
	uint32_t unknown_0x0; // TODO
	uint32_t hair_count;
	uint32_t unknown_0x8; // TODO: some count
	uint32_t unknown_0xc; // TODO
	std::array<HairGroupHeaderMember, 3> unknown_0x10;
};
static_assert(sizeof(HairGroupHeader) == 0x64);

struct HairIntersection
{
	uint32_t unknown_0x0;  // TODO
	uint32_t unknown_0x4;  // TODO
	uint32_t unknown_0x8;  // TODO
	uint32_t unknown_0xc;  // TODO
	uint32_t unknown_0x10; // TODO
	uint32_t unknown_0x14; // TODO
	uint32_t unknown_0x18; // TODO
	float unknown_0x1c;    // TODO
	float unknown_0x20;    // TODO
};
static_assert(sizeof(HairIntersection) == 0x24);

struct Hair
{
	uint32_t unknown_0x0;          // TODO
	HairIntersection intersection; // TODO
	std::array<float, 3> xs;
	std::array<float, 3> ys;
	std::array<float, 3> zs;
};
static_assert(sizeof(Hair) == 0x4c);

struct ExtraData
{
	uint32_t unknown_0x0; // TODO
	uint32_t unknown_0x4; // TODO
	uint32_t unknown_0x8; // TODO
	uint32_t unknown_0xc; // TODO
};
static_assert(sizeof(ExtraData) == 0x10);

enum class AnimationType : char
{
	// TODO: figure out types and their functions and convert, remove char underlying type
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
	std::string path;
	uint32_t version;
	std::vector<AnimationSetDesc> animation_sets;
};

struct AnimationFrame
{
	std::vector<std::array<float, 3>> euler_angles;
	std::vector<std::array<float, 3>> translations;
};

struct Animation
{
	AnimationHeader header;
	std::vector<uint32_t> rotated_joint_indices;
	std::vector<uint32_t> translated_joint_indices;
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
	bool _isLoaded;

	std::string _filename;

	MorphHeader _header;
	AnimationSpecs _animation_specs;
	std::vector<Animation> _base_animation;
	std::array<std::vector<Animation>, 4> _variant_animations; // last 2 mesh variants don't have animations
	HairHeader _hair_header;
	std::vector<HairGroup> _hair_groups;
	std::vector<std::vector<ExtraData>> _extra_data; ///< related to \ref _base_animation

	/// Error handling
	void Fail(const std::string& msg);

	/// Read file from the input source
	virtual void ReadFile(std::istream& stream, const std::string& specs_directory);
	void ReadSpecFile(const std::string& spec_file_path);
	std::vector<Animation> ReadAnimations(std::istream& stream, const std::vector<uint32_t>& offsets);
	HairGroup ReadHairGroup(std::istream& stream);

public:
	MorphFile();

	virtual ~MorphFile();

	/// Read morph file from the filesystem
	void Open(const std::string& file, const std::string& specs_directory);

	/// Read morph file from a buffer
	void Open(const std::vector<uint8_t>& buffer, const std::string& specs_directory);

	[[nodiscard]] const std::string& GetFilename() const { return _filename; }
	[[nodiscard]] const MorphHeader& GetHeader() const { return _header; }
	[[nodiscard]] const AnimationSpecs& GetAnimationSpecs() const { return _animation_specs; }
	[[nodiscard]] const std::vector<Animation>& GetBaseAnimationSet() const { return _base_animation; }
	[[nodiscard]] const std::vector<Animation>& GetVariantAnimationSet(uint32_t index) const
	{
		return _variant_animations[index];
	}
	[[nodiscard]] const std::vector<HairGroup>& GetHairGroups() const { return _hair_groups; }
	[[nodiscard]] const std::vector<std::vector<ExtraData>>& GetExtraData() const { return _extra_data; }
};

} // namespace openblack::morph
