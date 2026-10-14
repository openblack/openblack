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
#include <limits>
#include <new>
#include <utility>

#include "RendererInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack::graphics
{

// Keep resource metadata available to the simulation without creating a GPU or window.
class RendererNoOp final: public RendererInterface
{
public:
	[[nodiscard]] void* GetDevice() const noexcept final { return nullptr; }
	void DrawScene(const DrawSceneDesc&) noexcept final {}
	void Frame() noexcept final {}
	void RequestScreenshot(const std::filesystem::path&) noexcept final {}
	[[nodiscard]] bool GetDebug() const noexcept final { return _debug; }
	void SetDebug(bool value) noexcept final { _debug = value; }
	[[nodiscard]] bool GetProfile() const noexcept final { return _profile; }
	void SetProfile(bool value) noexcept final { _profile = value; }
	void DrawMesh(const L3DMesh&, const L3DMeshSubmitDesc&, uint8_t) const noexcept final {}

	[[nodiscard]] VertexBufferUniquePtr CreateVertexBuffer(std::string name, std::span<const uint8_t> memory,
	                                                       VertexDecl decl) noexcept final
	{
		// Match the packed vertex layout used by the GPU renderer.
		static constexpr std::array<std::array<uint32_t, 4>, 3> k_Strides = {{{4, 4, 4, 4}, {4, 4, 8, 8}, {4, 8, 12, 16}}};
		uint32_t stride = 0;
		for (const auto& attrib : decl)
		{
			if (attrib.num < 1 || attrib.num > 4 || static_cast<size_t>(attrib.type) >= k_Strides.size())
			{
				return nullptr;
			}
			stride += k_Strides.at(static_cast<size_t>(attrib.type)).at(attrib.num - 1);
		}
		if (stride == 0 || memory.empty() || memory.size_bytes() > std::numeric_limits<uint32_t>::max() ||
		    memory.size_bytes() % stride != 0)
		{
			return nullptr;
		}
		return {new (std::nothrow)
		            VertexBuffer {std::move(name), static_cast<uint32_t>(memory.size_bytes() / stride), stride, {}},
		        [](VertexBuffer* buffer) { delete buffer; }};
	}

	[[nodiscard]] IndexBufferUniquePtr CreateIndexBuffer(std::string name, std::span<const uint8_t> memory,
	                                                     IndexBuffer::Type type) noexcept final
	{
		const auto stride = IndexBuffer::GetTypeSize(type);
		if (memory.empty() || memory.size_bytes() > std::numeric_limits<uint32_t>::max() || memory.size_bytes() % stride != 0)
		{
			return nullptr;
		}
		return {new (std::nothrow) IndexBuffer {std::move(name), static_cast<uint32_t>(memory.size_bytes() / stride), type, {}},
		        [](IndexBuffer* buffer) { delete buffer; }};
	}

	[[nodiscard]] Texture2dUniquePtr CreateTexture2d(std::string name, std::span<const uint8_t>, glm::u16vec2 resolution,
	                                                 uint16_t layers, TextureFormat format, Wrapping, Filter) noexcept final
	{
		return {new (std::nothrow) Texture2d {std::move(name), {}, {}, resolution, layers, format},
		        [](Texture2d* texture) { delete texture; }};
	}

	[[nodiscard]] std::unique_ptr<FrameBuffer> CreateFrameBuffer(std::string name, glm::u16vec2 resolution,
	                                                             TextureFormat colorFormat,
	                                                             std::optional<TextureFormat> depthStencilFormat) noexcept final
	{
		return std::make_unique<FrameBuffer>(std::move(name), resolution, colorFormat, depthStencilFormat, TextureHandle {},
		                                     SamplerHandle {}, TextureHandle {}, [] {});
	}

private:
	bool _debug {};
	bool _profile {};
};

} // namespace openblack::graphics
