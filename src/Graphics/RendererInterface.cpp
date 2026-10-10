/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "RendererInterface.h"

#define LOCATOR_IMPLEMENTATIONS

// #include "Renderer.h"
#include "RendererSdl3Gpu.h"

using namespace openblack::graphics;

std::unique_ptr<RendererInterface> RendererInterface::Create(GraphicsBackend backend, bool vsync) noexcept
{
	// return Renderer::Create(backend, vsync);
	return RendererSdl3Gpu::Create(vsync);
}
