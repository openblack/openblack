/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstdio>
#include <cstdlib>

#include <SDL3/SDL.h>

#include "generated/shaders/sky.slang.spv.h"

static_assert(k_SkyVertexBufferDescriptionByVertex.pitch == 36);
static_assert(k_SkyVertexAttributes[3].offset == 32);

int main()
{
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		std::fprintf(stderr, "SDL init: %s\n", SDL_GetError());
		return 77;
	}
	auto* device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, "vulkan");
	if (device == nullptr)
	{
		std::fprintf(stderr, "GPU create: %s\n", SDL_GetError());
		SDL_Quit();
		return 77;
	}
	auto* vs = SDL_CreateGPUShader(device, &k_SkyVertexShaderCreateInfo);
	auto* fs = SDL_CreateGPUShader(device, &k_SkyFragmentShaderCreateInfo);
	if (vs == nullptr || fs == nullptr)
	{
		std::fprintf(stderr, "Shader create: %s\n", SDL_GetError());
		SDL_DestroyGPUDevice(device);
		SDL_Quit();
		return 1;
	}
	const SDL_GPUColorTargetDescription target = {.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM};
	const SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {
	    .vertex_shader = vs,
	    .fragment_shader = fs,
	    .vertex_input_state = k_SkyVertexInputStateByVertex,
	    .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
	    .rasterizer_state = {.enable_depth_clip = true},
	    .multisample_state = {.sample_count = SDL_GPU_SAMPLECOUNT_1},
	    .target_info = {.color_target_descriptions = &target, .num_color_targets = 1},
	};
	auto* pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
	if (pipeline == nullptr)
	{
		std::fprintf(stderr, "Pipeline create: %s\n", SDL_GetError());
	}
	else
	{
		SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
		std::puts("SDL GPU sky shaders and pipeline: PASS");
	}
	SDL_ReleaseGPUShader(device, vs);
	SDL_ReleaseGPUShader(device, fs);
	SDL_DestroyGPUDevice(device);
	SDL_Quit();
	return pipeline == nullptr ? EXIT_SUCCESS : EXIT_FAILURE;
}
