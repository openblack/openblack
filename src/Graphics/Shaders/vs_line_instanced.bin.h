#pragma once

// Generated compiled shaders using shaderc with the bin2c option
#include <generated/shaders/vs_line_instanced.sc.glsl.bin.h>
#include <generated/shaders/vs_line_instanced.sc.spv.bin.h>
#if defined(_WIN32)
#include <generated/shaders/vs_line_instanced.sc.dx9.bin.h>
#include <generated/shaders/vs_line_instanced.sc.dx11.bin.h>
#endif  //  defined(_WIN32)
#if __APPLE__
#include <generated/shaders/vs_line_instanced.sc.mtl.bin.h>
#endif // __APPLE__
