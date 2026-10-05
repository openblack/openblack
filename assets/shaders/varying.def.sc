vec4 a_position          : POSITION;
vec3 a_normal            : NORMAL;
vec4 a_indices           : BLENDINDICES;
vec4 a_color0            : COLOR0;     // time of day
vec2 a_texcoord0         : TEXCOORD0;
vec2 a_texcoord3         : TEXCOORD3;  // lightmap coordinates
vec4 i_data0             : TEXCOORD7;
vec4 i_data1             : TEXCOORD6;
vec4 i_data2             : TEXCOORD5;
vec4 i_data3             : TEXCOORD4;
vec4 i_data4             : COLOR0;     // Wind for instanced draws

vec4 v_position          : TEXCOORD1 = vec4(0.0, 0.0, 0.0, 0.0);
vec4 v_color0            : COLOR0    = vec4(1.0, 0.0, 0.0, 1.0);
vec4 v_texcoord0         : TEXCOORD0 = vec4(0.0, 0.0, 0.0, 1.0);
vec4 v_texcoord1         : TEXCOORD1 = vec4(0.0, 0.0, 0.0, 1.0);
vec3 v_normal            : NORMAL;
float v_lightLevel       : COLOR3;
float v_distToCamera     : DEPTH0;
vec4 v_shadowCoord       : TEXCOORD2 = vec4(0.0, 0.0, -1.0, 1.0);
vec4 v_haze              : TEXCOORD3 = vec4(0.0, 0.0, 0.0, 1.0);
