/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <bgfx/bgfx.h>
#include <imgui.h>

namespace ImGui
{
#define IMGUI_FLAGS_NONE UINT8_C(0x00)
#define IMGUI_FLAGS_ALPHA_BLEND UINT8_C(0x01)

// Helper function for passing bgfx::TextureHandle to ImGui::Image.
inline void Image(bgfx::TextureHandle handle, uint8_t flags, uint8_t mip, const ImVec2& size, const ImVec2& cuv0 = ImVec2(0, 0),
                  const ImVec2& cuv1 = ImVec2(1, 1))
{
	union
	{
		struct
		{
			bgfx::TextureHandle handle;
			uint8_t flags;
			uint8_t mip;
		} s;
		ImTextureID ptr;
	} texture;
	texture.s.handle = handle;
	texture.s.flags = flags;
	texture.s.mip = mip;

	// Do y-flip
	auto uv0 = cuv0;
	auto uv1 = cuv1;
	if (bgfx::getRendererType() == bgfx::RendererType::OpenGL || bgfx::getRendererType() == bgfx::RendererType::OpenGLES)
	{
		uv0.y = 1.0f - uv0.y;
		uv1.y = 1.0f - uv1.y;
	}

	Image(texture.ptr, size, uv0, uv1);
}

// Helper function for passing bgfx::TextureHandle to ImGui::Image.
inline void Image(bgfx::TextureHandle handle, const ImVec2& size, const ImVec2& uv0 = ImVec2(0.0f, 0.0f),
                  const ImVec2& uv1 = ImVec2(1.0f, 1.0f))
{
	Image(handle, IMGUI_FLAGS_ALPHA_BLEND, 0, size, uv0, uv1);
}

} // namespace ImGui
