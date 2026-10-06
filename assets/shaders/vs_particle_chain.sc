$input a_position, a_normal, a_tangent, a_texcoord0, a_color0, a_texcoord3
$output v_texcoord0, v_color0

#include <bgfx_shader.sh>

// One corner of a particle ribbon, moved to one side of its segment as the camera sees it:
// a_position: the joint, and its half width signed by the corner's side
// a_normal: the segment's other joint
// a_tangent: the other joint of the neighbouring segment that shares this joint
// a_texcoord3: x is 1 at the segment's first joint and -1 at its next; y is 1 when there is a neighbouring segment, whose
// corner here meets this one half way

vec3 SideOf(vec3 joint, vec3 along, vec3 eye)
{
	vec3 side = cross(eye - joint, along);
	float length2 = dot(side, side);
	return length2 > 0.0 ? side * inversesqrt(length2) : vec3_splat(0.0);
}

void main()
{
	vec3 eye = mul(u_invView, vec4(0.0, 0.0, 0.0, 1.0)).xyz;
	vec3 joint = a_position.xyz;
	vec3 side = SideOf(joint, (a_normal - joint) * a_texcoord3.x, eye);
	if (a_texcoord3.y > 0.5)
	{
		side = (side + SideOf(joint, (joint - a_tangent) * a_texcoord3.x, eye)) * 0.5;
	}
	vec3 world = joint + side * a_position.w;
	v_texcoord0 = vec4(a_texcoord0, 0.0, 0.0);
	v_color0 = a_color0;
	gl_Position = mul(u_viewProj, vec4(world, 1.0));
}
