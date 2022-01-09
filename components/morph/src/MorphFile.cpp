/*******************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************
 *
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
 *         1 unknown int, TODO: possibly duration or offset
 *         1 unknown int, TODO: seem to be 1 when type C and 0 when not
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
 * - 100 bytes header * number of groups, containing:
 *         1 unknown int, TODO:
 *         hair count
 *         1 unknown int, TODO: a count of some sort
 *         1 unknown int, TODO:
 *         3 unknown struct of 28 bytes each, TODO
 * - 76 byte hair structure * hair count per hair group, containing:
 *         1 unknown int, TODO:
 *         36 struct containing intersection data
 *         3 floats representing 3 x values
 *         3 floats representing 3 y values
 *         3 floats representing 3 z values
 * - 4 byte offset to the next block (extra)
 *
 * ------------------------ start of extra animation block --------------------
 *
 * - 16 byte structure * number of non-zero offsets in the base animation
 *   block, containing:
 *         4 unknown 32-bit data, TODO
 *
 */

#include <MorphFile.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
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
	std::streampos seekoff(off_type off, std::ios_base::seekdir way,
	                       [[maybe_unused]] std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override
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

	std::streampos seekpos([[maybe_unused]] pos_type pos,
	                       [[maybe_unused]] std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override
	{
		return seekoff(pos - pos_type(off_type(0)), std::ios_base::beg, which);
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
		int c = sb->sbumpc();
		switch (c)
		{
		case '\n':
			return is;
		case '\r':
			if (sb->sgetc() == '\n')
				sb->sbumpc();
			return is;
		case std::streambuf::traits_type::eof():
			// Also handle the case when the last line has no line ending
			if (t.empty())
				is.setstate(std::ios::eofbit);
			return is;
		default:
			t += (char)c;
		}
	}
}
} // namespace

/// Error handling
void MorphFile::Fail(const std::string& msg)
{
	throw std::runtime_error("MorphFile Error: " + msg + "\nFilename: " + _filename);
}

MorphFile::MorphFile()
    : _isLoaded(false)
{
}

MorphFile::~MorphFile() = default;

void MorphFile::ReadSpecFile(const std::string& spec_file_path)
{
	assert(!_isLoaded);

	_animation_specs = {};
	_animation_specs.path = spec_file_path;
	std::ifstream spec(_animation_specs.path);
	if (!spec.good())
	{
		Fail("Failed to read spec file: " + _animation_specs.path);
	}
	std::string line;
	spec >> _animation_specs.version >> std::ws;
	if (_header.spec_file_version != _animation_specs.version)
	{
		Fail("Spec file version mismatch: " + _animation_specs.path);
	}

	[[maybe_unused]] bool reached_end = false;
	while (!spec.eof())
	{
		char type;
		spec.get(type);
		// End of file
		if (type == 'E')
		{
			reached_end = true;
			break;
		}
		safe_getline(spec, line);
		if (type == '=')
		{
			_animation_specs.animation_sets.emplace_back().name = line;
		}
		else
		{
			if (_animation_specs.animation_sets.empty())
			{
				Fail("Spec file has animations before categories: " + _animation_specs.path);
			}
			_animation_specs.animation_sets.back().animations.emplace_back(
			    AnimationDesc {line, static_cast<AnimationType>(type)});
		}
	}
	assert(reached_end);
}

std::vector<Animation> MorphFile::ReadAnimations(std::istream& stream, const std::vector<uint32_t>& offsets)
{
	assert(!_isLoaded);

	std::vector<Animation> animations;

	uint32_t i = 0;
	for (auto& anim_set : _animation_specs.animation_sets)
	{
		for ([[maybe_unused]] auto& _ : anim_set.animations)
		{
			if (offsets[i] > 0)
			{
				stream.seekg(offsets[i]);
				auto& animation = animations.emplace_back();
				stream.read(reinterpret_cast<char*>(&animation.header), sizeof(animation.header));

				animation.rotated_joint_indices.resize(animation.header.rotated_joint_count);
				stream.read(reinterpret_cast<char*>(animation.rotated_joint_indices.data()),
				            animation.rotated_joint_indices.size() * sizeof(animation.rotated_joint_indices[0]));

				animation.translated_joint_indices.resize(animation.header.translated_joint_count);
				stream.read(reinterpret_cast<char*>(animation.translated_joint_indices.data()),
				            animation.translated_joint_indices.size() * sizeof(animation.translated_joint_indices[0]));

				animation.keyframes.resize(animation.header.frame_count);
				for (auto& frame : animation.keyframes)
				{
					frame.euler_angles.resize(animation.header.rotated_joint_count);
					stream.read(reinterpret_cast<char*>(frame.euler_angles.data()),
					            frame.euler_angles.size() * sizeof(frame.euler_angles[0]));

					frame.translations.resize(animation.header.translated_joint_count);
					stream.read(reinterpret_cast<char*>(frame.translations.data()),
					            frame.translations.size() * sizeof(frame.translations[0]));
				}
			}
			i++;
		}
	}

	return animations;
}

HairGroup MorphFile::ReadHairGroup(std::istream& stream)
{
	HairGroup hair_group;
	stream.read(reinterpret_cast<char*>(&hair_group.header), sizeof(hair_group.header));
	hair_group.hairs.resize(hair_group.header.hair_count);
	stream.read(reinterpret_cast<char*>(hair_group.hairs.data()), hair_group.hairs.size() * sizeof(hair_group.hairs[0]));
	return hair_group;
}

void MorphFile::ReadFile(std::istream& stream, const std::string& specs_directory)
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
		Fail("File too small to be a valid MORPH file.");
	}

	// First 236 bytes
	stream.read(reinterpret_cast<char*>(&_header), sizeof(MorphHeader));

	assert(_header.binary_version > 4); // structure is much different below v5

	// Parse spec file (a separate text file) using the version
	std::string spec_name;
	// this field is a good guess for hand or not, but a better choice might be
	// getting the segment name from pack
	if (_header.unknown_0x0)
	{
		spec_name = "ctrspec" + std::to_string(_header.spec_file_version) + ".txt";
	}
	else
	{
		spec_name = "hndspec" + std::to_string(_header.spec_file_version) + ".txt";
	}
	ReadSpecFile(specs_directory + "/" + spec_name);
	size_t num_animations = 0;
	for (auto& anim_set : _animation_specs.animation_sets)
	{
		num_animations += anim_set.animations.size();
	}

	// After the header is the anim set, a variable length array of offsets relative to the section offset
	std::vector<uint32_t> animation_offsets(num_animations);
	stream.read(reinterpret_cast<char*>(animation_offsets.data()), animation_offsets.size() * sizeof(animation_offsets[0]));

	// Following the animation offsets are chained offsets which can lead to extra data
	uint32_t extra_offset = 0;
	stream.read(reinterpret_cast<char*>(&extra_offset), sizeof(extra_offset));

	// Read in the base animations using those offsets
	_base_animation = ReadAnimations(stream, animation_offsets);

	// Creature files have different animations for the morph meshes (evil, good, thin, fat) weak, strong are skipped
	for (uint32_t i = 0; i < 4; ++i)
	{
		if (std::strlen(_header.variant_mesh_names[i].data()) > 0)
		{
			// Set file to next animation set
			stream.seekg(extra_offset);

			std::vector<uint32_t> variant_animation_offsets(num_animations);
			stream.read(reinterpret_cast<char*>(variant_animation_offsets.data()),
			            variant_animation_offsets.size() * sizeof(variant_animation_offsets[0]));

			// Again, the get pointer to the next part
			stream.read(reinterpret_cast<char*>(&extra_offset), sizeof(extra_offset));

			_variant_animations[i] = ReadAnimations(stream, variant_animation_offsets);
		}
	}

	// Once all the animation sets are loaded, the extra offset points to hair groups data (even if there are none)
	stream.seekg(extra_offset);
	stream.read(reinterpret_cast<char*>(&_hair_header), sizeof(_hair_header));
	for (uint32_t i = 0; i < _hair_header.hair_group_count; ++i)
	{
		_hair_groups.emplace_back(ReadHairGroup(stream));
	}

	// The extra data segment is in relation to the number of animations in the base animation set
	_extra_data.resize(num_animations);
	for (size_t i = 0; i < num_animations; ++i)
	{
		if (animation_offsets[i] == 0)
		{
			continue;
		}
		uint32_t has_data; // TODO: unknown if this serves another function
		while (stream.read(reinterpret_cast<char*>(&has_data), sizeof(has_data)).good() && has_data)
		{
			auto& data = _extra_data[i].emplace_back();
			stream.read(reinterpret_cast<char*>(&data), sizeof(data));
		}
	}

	_isLoaded = true;
}

void MorphFile::Open(const std::string& file, const std::string& specs_directory)
{
	assert(!_isLoaded);

	_filename = file;

	std::ifstream stream(_filename, std::ios::binary);

	if (!stream.is_open())
	{
		Fail("Could not open file.");
	}

	ReadFile(stream, specs_directory);
}

void MorphFile::Open(const std::vector<uint8_t>& buffer, const std::string& specs_directory)
{
	assert(!_isLoaded);

	imemstream stream(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(buffer[0]));

	_filename = "buffer";

	ReadFile(stream, specs_directory);
}
