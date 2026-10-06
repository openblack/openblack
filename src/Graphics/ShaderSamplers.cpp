/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ShaderSamplers.h"

#include <algorithm>

namespace openblack::graphics::shader_samplers
{

namespace
{

// The layout of a bgfx shader binary's header, as its shader compiler writes it: a four character code naming the
// shader's kind and format version, hashes of its inputs and outputs, then its uniforms
constexpr uint8_t k_FirstVersionWithOutputHash = 6;
constexpr uint8_t k_FirstVersionWithTextureInfo = 8;
constexpr uint8_t k_FirstVersionWithTextureFormat = 10;
// From this version, a SPIR-V sampler's binding is its stage plus two (past the two uniform buffers); before it the
// stage was offset by 16, and a fragment shader's by another 48
constexpr uint8_t k_FirstVersionWithNewBindings = 11;
constexpr uint16_t k_BindingShift = 2;
constexpr uint16_t k_OldTextureShift = 16;
constexpr uint16_t k_OldFragmentShift = 48;

// A uniform's type is its kind in the low bits with flags above; a sampler is kind 0 with the sampler flag
constexpr uint8_t k_TypeFlags = 0xF0;
constexpr uint8_t k_SamplerFlag = 0x20;

class Reader
{
public:
	explicit Reader(std::span<const uint8_t> data)
	    : _data(data)
	{
	}

	[[nodiscard]] bool Ok() const { return _ok; }

	template <typename T>
	T Read()
	{
		T value {};
		if (!_ok || _data.size() - _position < sizeof(T))
		{
			_ok = false;
			return value;
		}
		// Little endian, as bgfx writes it
		for (size_t i = 0; i < sizeof(T); ++i)
		{
			value = static_cast<T>(value | (static_cast<T>(_data[_position + i]) << (8 * i)));
		}
		_position += sizeof(T);
		return value;
	}

	std::string ReadString(size_t size)
	{
		if (!_ok || _data.size() - _position < size)
		{
			_ok = false;
			return {};
		}
		std::string value(reinterpret_cast<const char*>(_data.data() + _position), size);
		_position += size;
		return value;
	}

private:
	std::span<const uint8_t> _data;
	size_t _position = 0;
	bool _ok = true;
};

Dimension DimensionFromId(uint8_t id)
{
	switch (id)
	{
	case 1:
		return Dimension::Texture1D;
	case 2:
		return Dimension::Texture2D;
	case 3:
		return Dimension::Texture2DArray;
	case 4:
		return Dimension::Cube;
	case 5:
		return Dimension::CubeArray;
	case 6:
		return Dimension::Texture3D;
	default:
		return Dimension::Unknown;
	}
}

} // namespace

std::optional<std::vector<Sampler>> ReadSpirvSamplers(std::span<const uint8_t> binary)
{
	Reader reader(binary);
	const auto kind = static_cast<char>(reader.Read<uint8_t>());
	const auto s = static_cast<char>(reader.Read<uint8_t>());
	const auto h = static_cast<char>(reader.Read<uint8_t>());
	const auto version = reader.Read<uint8_t>();
	if (!reader.Ok() || (kind != 'V' && kind != 'F' && kind != 'C') || s != 'S' || h != 'H')
	{
		return std::nullopt;
	}
	const bool fragment = kind == 'F';

	reader.Read<uint32_t>(); // the hash of its inputs
	if (version >= k_FirstVersionWithOutputHash)
	{
		reader.Read<uint32_t>(); // and of its outputs
	}

	const auto count = reader.Read<uint16_t>();
	std::vector<Sampler> samplers;
	for (uint16_t i = 0; i < count && reader.Ok(); ++i)
	{
		const auto nameSize = reader.Read<uint8_t>();
		auto name = reader.ReadString(nameSize);
		const auto type = reader.Read<uint8_t>();
		reader.Read<uint8_t>(); // its array size
		const auto binding = reader.Read<uint16_t>();
		reader.Read<uint16_t>(); // its register count
		uint8_t dimension = 0;
		if (version >= k_FirstVersionWithTextureInfo)
		{
			reader.Read<uint8_t>(); // the type of its texture's components
			dimension = reader.Read<uint8_t>();
		}
		if (version >= k_FirstVersionWithTextureFormat)
		{
			reader.Read<uint16_t>(); // the format of a storage image
		}

		if ((type & k_SamplerFlag) == 0 || (type & ~k_TypeFlags) != 0)
		{
			continue;
		}
		const uint16_t shift = version >= k_FirstVersionWithNewBindings
		                           ? k_BindingShift
		                           : static_cast<uint16_t>(k_OldTextureShift + (fragment ? k_OldFragmentShift : 0));
		if (binding < shift || binding - shift >= k_MaxStages)
		{
			continue;
		}
		samplers.push_back(Sampler {
		    .name = std::move(name),
		    .stage = static_cast<uint8_t>(binding - shift),
		    .dimension = DimensionFromId(dimension),
		});
	}
	if (!reader.Ok())
	{
		return std::nullopt;
	}
	return samplers;
}

DefaultTexture DefaultTextureFor(Dimension dimension) noexcept
{
	switch (dimension)
	{
	case Dimension::Texture2DArray:
		return DefaultTexture::White2DArray;
	case Dimension::Cube:
	case Dimension::CubeArray:
		return DefaultTexture::WhiteCube;
	case Dimension::Texture3D:
		return DefaultTexture::White3D;
	case Dimension::Unknown:
	case Dimension::Texture1D:
	case Dimension::Texture2D:
		break;
	}
	return DefaultTexture::White2D;
}

std::vector<Sampler> Merge(std::span<const Sampler> vertex, std::span<const Sampler> fragment)
{
	std::vector<Sampler> merged(vertex.begin(), vertex.end());
	for (const auto& sampler : fragment)
	{
		if (std::ranges::none_of(merged, [&sampler](const Sampler& s) { return s.name == sampler.name; }))
		{
			merged.push_back(sampler);
		}
	}
	return merged;
}

std::vector<std::pair<std::string, std::string>> StageCollisions(std::span<const Sampler> samplers)
{
	std::vector<std::pair<std::string, std::string>> collisions;
	for (size_t i = 0; i < samplers.size(); ++i)
	{
		for (size_t j = i + 1; j < samplers.size(); ++j)
		{
			if (samplers[i].stage == samplers[j].stage && samplers[i].name != samplers[j].name)
			{
				collisions.emplace_back(samplers[i].name, samplers[j].name);
			}
		}
	}
	return collisions;
}

} // namespace openblack::graphics::shader_samplers
