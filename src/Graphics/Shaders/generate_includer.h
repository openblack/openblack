// https://stackoverflow.com/questions/40062883/how-to-use-a-macro-in-an-include-directive

// Turn off formatting because it adds spaces which break the stringifying
// clang-format off
#define GENERATED_SHADERS_DIR generated/shaders/

#ifndef SHADER_DIR
#define SHADER_DIR
#endif

#define STRINGIFY_MACRO(x) STR(x)
#define STR(x) #x
#define EXPAND(x) x
#define CONCAT(n1, n2) STRINGIFY_MACRO(EXPAND(n1)EXPAND(n2))
#define CONCAT4(n1, n2, n3, n4) STRINGIFY_MACRO(EXPAND(n1)EXPAND(n2)EXPAND(n3)EXPAND(n4))

// Concatenate the four elements of your path.
#include CONCAT4(GENERATED_SHADERS_DIR, SHADER_DIR, SHADER_NAME,.sc.glsl.bin.h)
#include CONCAT4(GENERATED_SHADERS_DIR, SHADER_DIR, SHADER_NAME,.sc.spv.bin.h)
#include CONCAT4(GENERATED_SHADERS_DIR, SHADER_DIR, SHADER_NAME,.sc.essl.bin.h)
#if defined(_WIN32)
#include CONCAT4(GENERATED_SHADERS_DIR, SHADER_DIR, SHADER_NAME,.sc.dx11.bin.h)
#include CONCAT4(GENERATED_SHADERS_DIR, SHADER_DIR, SHADER_NAME,.sc.dx9.bin.h)
#endif //  defined(_WIN32)
#if __APPLE__
#include CONCAT4(GENERATED_SHADERS_DIR, SHADER_DIR, SHADER_NAME,.sc.mtl.bin.h)
#endif // __APPLE__

#undef SHADER_DIR
#undef SHADER_NAME
// clang-format on
