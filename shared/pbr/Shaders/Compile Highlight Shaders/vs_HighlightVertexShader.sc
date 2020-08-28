$input a_position, a_normal, a_tangent, a_color0, a_texcoord0, a_indices, i_data0, i_data1, i_data2, i_data3,i_data4 
$output v_positionWorld, v_normalWorld

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
	transform[0] = i_data0;
	transform[1] = i_data1;
	transform[2] = i_data2;
	transform[3] = i_data3;
    mat4 modelTransform = mul(transform, u_modelToWorld);
    vec4 transformedPosWorld = mul(a_position, modelTransform);
    gl_Position = mul(transformedPosWorld, u_viewProjection);
    v_positionWorld = transformedPosWorld.xyz / transformedPosWorld.w;
    #define mm modelTransform
    mat3 modelTransformMat3 = mat3(mm[0][0], mm[0][1], mm[0][2], mm[1][0], mm[1][1], mm[1][2], mm[2][0], mm[2][1], mm[2][2]);
    v_normalWorld = mul(a_normal, modelTransformMat3).xyz;
}
