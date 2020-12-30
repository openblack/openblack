#pragma once

// Generated compiled shaders using shaderc with the bin2c option
#include <generated/shaders/fs_country_2d.sc.glsl.bin.h>
#include <generated/shaders/fs_country_2d.sc.spv.bin.h>
#if defined(_WIN32)
#include <generated/shaders/fs_country_2d.sc.dx9.bin.h>
#include <generated/shaders/fs_country_2d.sc.dx11.bin.h>
#endif //  defined(_WIN32)
#if __APPLE__
#include <generated/shaders/fs_country_2d.sc.mtl.bin.h>
#endif // __APPLE__
