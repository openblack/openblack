/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace openblack::morph
{

enum class MorphResult : uint8_t
{
	Success = 0,
	ErrCantOpen,
	ErrFileTooSmall,
	ErrSpecFileCantOpen,
	ErrSpecFileVersionMismatch,
	ErrSpecFileAnimationsBeforeCategories,
};

std::string_view ResultToStr(MorphResult result);

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
	/// Length in milliseconds. A cycle (type C) spends it on every frame including the wrap back to the first, a
	/// pose range (type L) spans it from the first frame to the last.
	uint32_t duration;
	/// 1 for cycles (type C), 0 for pose ranges (type L)
	uint32_t looping;
	float unknown0x8;  // TODO(#459):
	float unknown0xc;  // TODO(#459):
	float unknown0x10; // TODO(#459):
	float unknown0x14; // TODO(#459):
	float unknown0x18; // TODO(#459):
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

/// How a hair group looks at one end of the evil to good axis, or in the middle
struct HairGroupVariant
{
	/// The strands' colour, 0 to 255 a channel
	int32_t red;
	int32_t green;
	int32_t blue;
	/// How long a strand is, in units of the creature's hair scale
	float length;
	/// With the length, how much of its speed a strand keeps over a second: (damping * length) squared
	float damping;
	/// How hard each segment springs back in line with the one before it
	float stiffness;
	/// How wide a strand is drawn, and how far its root is sunk into the body, in units of the hair scale
	float thickness;
};
static_assert(sizeof(HairGroupVariant) == 0x1c);

struct HairGroupHeader
{
	uint32_t segmentsVersion;
	uint32_t hairCount;
	/// The points each strand is simulated and drawn with, its root first
	uint32_t segmentCount;
	/// 1 when the strands are drawn with the hair texture, otherwise in their colour alone
	uint32_t mappingIndex;
	/// Neutral, evil and good
	std::array<HairGroupVariant, 3> variants;
};
static_assert(sizeof(HairGroupHeader) == 0x64);

/// A point on a triangle of the base mesh's first submesh, which follows the mesh as it is morphed and posed
struct MeshIntersect
{
	/// The primitive of the submesh the triangle is in
	uint32_t primitive;
	/// The triangle's vertices, counted in the primitive
	std::array<uint32_t, 3> vertices;
	/// The vertex group, counted in the primitive, of each of the triangle's vertices, whose bone moves it
	std::array<uint32_t, 3> vertexGroups;
	/// How far the point is from the first vertex towards the second and towards the third
	float u;
	float v;
};
static_assert(sizeof(MeshIntersect) == 0x24);

/// One strand of a hair group, rooted on a triangle of the body
struct Hair
{
	/// Bit 0: the strand grows out turned from the surface by its angles, rather than straight out of it
	uint32_t flags;
	MeshIntersect intersection;
	/// For neutral, evil and good, the x, y and z angles in radians the strand is turned by, combined y, x then z, in
	/// the space of the bones that move its triangle
	std::array<std::array<float, 3>, 3> angles;
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
	std::string name;    ///< Name from the spec file (e.g. "Cwiggle")
	std::string setName; ///< Animation set from the spec file (e.g. "standard")
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

/// Where a creature's eyes sit on its body, from the creature block of a .cbn file
struct CreatureEyes
{
	struct Point
	{
		MeshIntersect intersect;
		bool enabled;
		/// How far the point is sunk into the body along the triangle's normal
		float depth;
	};
	/// How big the eyes are for the species
	float scale;
	/// The left and right eyes, then the points the left and right eyelids are turned towards
	std::array<Point, 4> points;
	/// The eyelids' angles, in units of pi, as [mode][k]: open, closed, and the base the lid follows the pupil from.
	/// Only k = 1 is used.
	std::array<std::array<float, 3>, 3> lidAngles;
};

/**
  This class is used to read the Creature block of CBN and the Hand block of HBN files.
 */
class MorphFile
{
protected:
	/// True when a file has been loaded
	bool _isLoaded {false};

	MorphHeader _header;
	AnimationSpecs _animationSpecs;
	std::vector<Animation> _baseAnimation;
	/// For every animation of the spec file in order, its index in _baseAnimation or -1 when the file has none
	std::vector<int32_t> _baseAnimationIndices;
	std::array<std::vector<Animation>, 4> _variantAnimations; // last 2 mesh variants don't have animations
	/// For each variant, as _baseAnimationIndices for its animations
	std::array<std::vector<int32_t>, 4> _variantAnimationIndices;
	HairHeader _hairHeader;
	std::vector<HairGroup> _hairGroups;
	std::vector<std::vector<ExtraData>> _extraData; ///< related to \ref _base_animation
	/// The creature's eyes, in creature files from version 14 on
	std::optional<CreatureEyes> _creatureEyes;

	/// Read file from the input source
	MorphResult ReadFile(std::istream& stream, const std::filesystem::path& specsDirectory) noexcept;
	MorphResult ReadSpecFile(const std::filesystem::path& specFilePath) noexcept;
	std::vector<Animation> ReadAnimations(std::istream& stream, const std::vector<uint32_t>& offsets) noexcept;
	HairGroup ReadHairGroup(std::istream& stream) noexcept;
	/// The creature block that follows the morph data in creature files, as far as the eyes
	void ReadCreatureEyes(std::istream& stream) noexcept;

public:
	MorphFile() noexcept;
	virtual ~MorphFile() noexcept;

	/// Read morph file from the filesystem
	MorphResult Open(const std::filesystem::path& filepath, const std::filesystem::path& specsDirectory) noexcept;

	/// Read morph file from a buffer
	MorphResult Open(const std::vector<uint8_t>& buffer, const std::filesystem::path& specsDirectory) noexcept;

	[[nodiscard]] const MorphHeader& GetHeader() const noexcept { return _header; }
	[[nodiscard]] const AnimationSpecs& GetAnimationSpecs() const noexcept { return _animationSpecs; }
	[[nodiscard]] const std::vector<Animation>& GetBaseAnimationSet() const noexcept { return _baseAnimation; }
	/// The base animation for an animation of the spec file, counted across all of its sets, or null when absent
	[[nodiscard]] const Animation* GetBaseAnimation(size_t specIndex) const noexcept
	{
		if (specIndex >= _baseAnimationIndices.size() || _baseAnimationIndices[specIndex] < 0)
		{
			return nullptr;
		}
		return &_baseAnimation[static_cast<size_t>(_baseAnimationIndices[specIndex])];
	}
	[[nodiscard]] const std::vector<Animation>& GetVariantAnimationSet(uint32_t index) const noexcept
	{
		return _variantAnimations.at(index);
	}
	/// A variant's animation (0 evil, 1 good, 2 thin, 3 fat) for an animation of the spec file, or null when absent
	[[nodiscard]] const Animation* GetVariantAnimation(uint32_t variant, size_t specIndex) const noexcept
	{
		const auto& indices = _variantAnimationIndices.at(variant);
		if (specIndex >= indices.size() || indices[specIndex] < 0)
		{
			return nullptr;
		}
		return &_variantAnimations.at(variant)[static_cast<size_t>(indices[specIndex])];
	}
	[[nodiscard]] const std::vector<HairGroup>& GetHairGroups() const noexcept { return _hairGroups; }
	[[nodiscard]] const std::vector<std::vector<ExtraData>>& GetExtraData() const noexcept { return _extraData; }
	[[nodiscard]] const std::optional<CreatureEyes>& GetCreatureEyes() const noexcept { return _creatureEyes; }
};

} // namespace openblack::morph
