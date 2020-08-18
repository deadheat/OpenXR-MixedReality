////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation.  All Rights Reserved
// Licensed under the MIT License. See License.txt in the project root for license information.
//
$input a_position, a_normal, a_tangent, a_color0, a_texCoord0, a_indices, i_data0, i_data1, i_data2, i_data3,i_data4 
$output v_positionProj, v_positionWorld, v_normalWorld

#include "HighlightShared.sc"

//StructuredBuffer<float4x4> Transforms : register(t0);
uniform mat4 u_modelToWorld;

//cbuffer modelconstantbuffer : register(b1)
//{
 //   float4x4 modeltoworld  : packoffset(c0);
//};


//struct VSInputFlat
//{
 //   float4      Position            : POSITION;
 //   float3      Normal              : NORMAL;
 //   float4      Tangent             : TANGENT;
 //   float4      Color0              : COLOR0;
 //   float2      TexCoord0           : TEXCOORD0;
 //   min16uint   ModelTransformIndex : TRANSFORMINDEX;
//};
//struct PSInputFlat
//{
//    float4 PositionProj : SV_POSITION;
//    float3 PositionWorld: POSITION1;
//    nointerpolation float3 NormalWorld : Normal;
//};

#define VSOutputFlat PSInputFlat
VSOutputFlat main(VSInputFlat input)
{
    mat4 transform;
	transform[0] = i_data0;
	transform[1] = i_data1;
	transform[2] = i_data2;
	transform[3] = i_data3;
    const mat4 modelTransform = mul(transform, u_modelToWorld);
    const vec4 transformedPosWorld = mul(input.Position, modelTransform);
    v_positionProj = mul(transformedPosWorld, u_viewProjection);
    //output.PositionProj = mul(transformedPosWorld, ViewProjection);
    v_positionWorld = transformedPosWorld.xyz / transformedPosWorld.w;
    v_normalWorld = mul(input.Normal, (mat3)modelTransform).xyz;
    //return output;
}
