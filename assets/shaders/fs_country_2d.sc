$input v_texcoord0

/*
 * Copyright 2014 Dario Manesku. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include <bgfx_shader.sh>

// dx9 poop
#if BGFX_SHADER_LANGUAGE_HLSL && (BGFX_SHADER_LANGUAGE_HLSL < 400)
void main()
{
	gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
#else

SAMPLER2DARRAY(s0_materials, 0);
USAMPLER2D(s1_countryMap, 1);

void main()
{
	ivec2 lookup = ivec2(v_texcoord0.x * 256, v_texcoord0.y * 256);

	// lookup country map [country, altitude]
	uint texID = texelFetch(s1_countryMap, lookup, 0).r;

	gl_FragColor = texture2DArray(s0_materials, vec3(v_texcoord0.xy, texID));
}

#endif
