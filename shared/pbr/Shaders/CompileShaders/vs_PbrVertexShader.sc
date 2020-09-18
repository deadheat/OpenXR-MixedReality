$input a_position, a_normal, a_tangent, a_color0, a_texcoord0, i_data0, i_data1, i_data2, i_data3, i_data4
$output v_positionWorld, v_TBN, v_texcoord0, v_color0

#include "common.sh"

uniform mat4 u_viewProjection;
uniform vec4 u_eyePosition;
uniform mat3 u_highlightPositionLightDirectionLightColor;
uniform vec4 u_numSpecularMipLevelsAnimationTime;


// Seyi NOTE: Not sure if you can pack a mat4 like this, if you can, im really smart
#define u_highlightPosition u_highlightPositionLightDirectionLightColor[0]
#define u_lightDirection u_highlightPositionLightDirectionLightColor[1]
#define u_lightColor u_highlightPositionLightDirectionLightColor[2]
#define u_numSpecularMipLevels u_numSpecularMipLevelsAnimationTime.x
#define u_animationTime u_numSpecularMipLevelsAnimationTime.y


uniform mat4 u_modelToWorld;


void main()
{
    mat4 transform;
	transform[0] = vec4(1, 0, 0, 0);//i_data0;
	transform[1] = vec4(0, 1, 0, 0);//i_data1;
	transform[2] = vec4(0, 0, 1, 0);//i_data2;
	transform[3] = vec4(0, 0, 0, 1);//i_data3;

    a_position +=  vec4(0, 0, 1, 0);
    mat4 modelTransform = mul(transform, u_modelToWorld);
    vec4 transformedPosWorld = mul(a_position, modelTransform);
    gl_Position = mul(transformedPosWorld, u_viewProjection);
    v_positionWorld = transformedPosWorld.xyz / transformedPosWorld.w;

    vec3 normalW = normalize(mul(vec4(a_normal, 0.0), modelTransform).xyz);
    vec3 tangentW = normalize(mul(vec4(a_tangent.xyz, 0.0), modelTransform).xyz);
    vec3 bitangentW = cross(normalW, tangentW) * a_tangent.w;
    v_TBN = mtxFromRows(tangentW, bitangentW, normalW);

    v_texcoord0 = a_texcoord0;
    v_color0 = a_color0;
}
