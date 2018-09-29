#pragma once
#ifndef OPENBLACK_SHADERS_H
#define OPENBLACK_SHADERS_H

#define SHADER_VF(Name)                    \
    struct Name {                          \
        static const char* VertexShader;   \
        static const char* FragmentShader; \
}

namespace OpenBlack
{
	namespace Shaders
	{
		SHADER_VF(Terrain);
		SHADER_VF(WorldObject);
	}
}

#endif // OPENBLACK_SHADERS_H
