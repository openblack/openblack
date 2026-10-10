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
#include "generated/shaders/sprite.slang.spv.h"

static_assert(k_SkyVertexBufferDescriptionByVertex.pitch == 36);
static_assert(k_SkyVertexAttributes[3].offset == 32);
static_assert(k_SpriteVertexBufferDescriptionByVertex.pitch == 8);
static_assert(k_SpriteVertexShaderCreateInfo.num_uniform_buffers == 1);
static_assert(k_SpriteFragmentShaderCreateInfo.num_uniform_buffers == 1);
static_assert(k_SpriteFragmentShaderCreateInfo.num_samplers == 1);

bool CheckPipeline(SDL_GPUDevice* device, const char* name, const SDL_GPUShaderCreateInfo& vertexInfo,
                   const SDL_GPUShaderCreateInfo& fragmentInfo, const SDL_GPUVertexInputState& inputState)
{
	auto* vs = SDL_CreateGPUShader(device, &vertexInfo);
	auto* fs = SDL_CreateGPUShader(device, &fragmentInfo);
	if (vs == nullptr || fs == nullptr)
	{
		std::fprintf(stderr, "%s shader create: %s\n", name, SDL_GetError());
		if (vs != nullptr)
		{
			SDL_ReleaseGPUShader(device, vs);
		}
		if (fs != nullptr)
		{
			SDL_ReleaseGPUShader(device, fs);
		}
		return false;
	}
	const SDL_GPUColorTargetDescription target = {.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM};
	const SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {
	    .vertex_shader = vs,
	    .fragment_shader = fs,
	    .vertex_input_state = inputState,
	    .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
	    .rasterizer_state = {.enable_depth_clip = true},
	    .multisample_state = {.sample_count = SDL_GPU_SAMPLECOUNT_1},
	    .target_info = {.color_target_descriptions = &target, .num_color_targets = 1},
	};
	auto* pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
	if (pipeline == nullptr)
	{
		std::fprintf(stderr, "%s pipeline create: %s\n", name, SDL_GetError());
	}
	else
	{
		SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
		std::printf("SDL GPU %s shaders and pipeline: PASS\n", name);
	}
	SDL_ReleaseGPUShader(device, vs);
	SDL_ReleaseGPUShader(device, fs);
	return pipeline != nullptr;
}

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
	const bool sky =
	    CheckPipeline(device, "sky", k_SkyVertexShaderCreateInfo, k_SkyFragmentShaderCreateInfo, k_SkyVertexInputStateByVertex);
	const bool sprite = CheckPipeline(device, "sprite", k_SpriteVertexShaderCreateInfo, k_SpriteFragmentShaderCreateInfo,
	                                  k_SpriteVertexInputStateByVertex);
	SDL_DestroyGPUDevice(device);
	SDL_Quit();
	return sky && sprite ? EXIT_SUCCESS : EXIT_FAILURE;
}
