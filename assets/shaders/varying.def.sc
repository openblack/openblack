vec4 a_position     : POSITION;
vec2 a_texcoord0    : TEXCOORD0;
vec3 a_normal    : NORMAL;
vec4 a_color0       : COLOR0;
vec3 a_weight       : WEIGHTS;
vec3 a_color1       : COLOR1;  // firstMaterialID
vec3 a_color2      : COLOR2;  // secondMaterialID
vec3 a_indices     : INDICES;  // material blend coefficient
float a_color3      : COLOR3;  // water alpha

vec4 v_position    : TEXCOORD1    = vec4(0.0, 0.0, 0.0, 0.0);
vec4 v_color0    : COLOR0    = vec4(1.0, 0.0, 0.0, 1.0);
vec4 v_texcoord0 : TEXCOORD0 = vec4(0.0, 0.0, 0.0, 1.0);
vec3 v_normal    : NORMAL;
vec3 v_weight    : WEIGHTS;
flat uvec3 v_materialID0 : COLOR0;
flat uvec3 v_materialID1 : COLOR1;
vec3 v_materialBlend : COLOR2;
float v_lightLevel : COLOR3;
float v_waterAlpha : COLOR4;
