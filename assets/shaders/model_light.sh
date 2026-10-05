#ifndef MODEL_LIGHT_SH
#define MODEL_LIGHT_SH

// The game's model light, which src/Graphics/ModelLight.h places: one point light and an ambient level, applied per
// vertex in whole numbers.

uniform vec4 u_modelLight; // xyz: the light's position in the world, w: the ambient, of 255

// The light's direction in the space of a mesh, whose axes and origin in the world are given: from the origin, not from
// the vertex, as the game takes it. Mirrored spaces keep the light on the right side.
vec3 ModelLightLocal(vec3 axisX, vec3 axisY, vec3 axisZ, vec3 origin)
{
	vec3 toLight = u_modelLight.xyz - origin;
	vec3 row0 = cross(axisY, axisZ);
	vec3 row1 = cross(axisZ, axisX);
	vec3 row2 = cross(axisX, axisY);
	float determinant = dot(axisX, row0);
	vec3 local = vec3(dot(row0, toLight), dot(row1, toLight), dot(row2, toLight));
	local *= determinant < 0.0f ? -1.0f : 1.0f;
	float lengthSquared = dot(local, local);
	return lengthSquared > 0.0f ? local / sqrt(lengthSquared) : vec3(0.0f, 0.0f, 0.0f);
}

// What a white vertex becomes, 0 to 1, for its local normal (as the mesh stores it, not normalised) and the light's
// local direction: I = round(255 n.l), halves to even; f = I < 0 ? ambient : ambient + ((255 - ambient) I >> 8); and
// the colour (255 f) >> 8
float ModelLightLevel(vec3 localNormal, vec3 localLight)
{
	float lit = 255.0f * dot(localNormal, localLight);
	float intensity = floor(lit + 0.5f);
	if (intensity - lit == 0.5f && mod(intensity, 2.0f) != 0.0f)
	{
		intensity -= 1.0f;
	}
	float ambient = u_modelLight.w;
	float factor = intensity < 0.0f ? ambient : ambient + floor((255.0f - ambient) * intensity / 256.0f);
	return floor(255.0f * factor / 256.0f) / 255.0f;
}

#endif // MODEL_LIGHT_SH
