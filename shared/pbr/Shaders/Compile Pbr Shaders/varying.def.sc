vec3 v_positionWorld    : TEXCOORD1 = vec3(0.0, 0.0, 0.0);
mat3 v_TBN              : TANGENT = mat3(0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0);
vec2 v_texcoord0        : TEXCOORD0 = vec2(0.0, 0.0);
vec4 v_color0           : COLOR0 = vec4(1.0, 0.0, 0.0, 1.0);

vec4 a_position  : POSITION;
vec3 a_normal    : NORMAL;
vec4 a_tangent   : TANGENT;
vec4 a_color0    : COLOR0;
vec2 a_texcoord0 : TEXCOORD0;
vec4 i_data0     : TEXCOORD7;
vec4 i_data1     : TEXCOORD6;
vec4 i_data2     : TEXCOORD5;
vec4 i_data3     : TEXCOORD4;