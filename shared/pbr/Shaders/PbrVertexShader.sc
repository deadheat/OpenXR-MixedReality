////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation.  All Rights Reserved
// Licensed under the MIT License. See License.txt in the project root for license information.
//
// This shader is based on the vertex shader at https://github.com/KhronosGroup/glTF-WebGL-PBR
// with modifications for HLSL and stereoscopic rendering.
//
// The MIT License
// 
// Copyright(c) 2016 - 2017 Mohamad Moneimne and Contributors
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
$input a_position, a_normal, a_tangent, a_color0, a_texCoord0, a_indices, i_data0, i_data1, i_data2, i_data3, i_data4
$output v_positionProj, v_positionWorld, v_TBN, v_texCoord0, v_color0

#include "PbrShared.sc"

//StructuredBuffer<float4x4> Transforms : register(t0);

uniform mat4 u_modelToWorld;

//cbuffer ModelConstantBuffer : register(b1)
//{
//    float4x4 ModelToWorld  : packoffset(c0);
//
//};

//struct VSInputPbr
//{
//    float4      Position            : POSITION;
//    float3      Normal              : NORMAL;
//    float4      Tangent             : TANGENT;
//    float4      Color0              : COLOR0;
//    float2      TexCoord0           : TEXCOORD0;
//    min16uint   ModelTransformIndex : TRANSFORMINDEX;
//};

VSOutputPbr main()
{
    mat4 transform;
	transform[0] = i_data0;
	transform[1] = i_data1;
	transform[2] = i_data2;
	transform[3] = i_data3;
    const mat4 modelTransform = instMul(transform, u_modelToWorld);
    const vec4 transformedPosWorld = mul(a_position, modelTransform);
    v_positionProj = mul(transformedPosWorld, u_viewProjection);
    v_positionWorld = transformedPosWorld.xyz / transformedPosWorld.w;

    const vec3 normalW = normalize(mul(vec4(a_normal, 0.0), modelTransform).xyz);
    const vec3 tangentW = normalize(mul(vec4(a_tangent.xyz, 0.0), modelTransform).xyz);
    const vec3 bitangentW = cross(normalW, tangentW) * a_tangent.w;
    v_TBN = float3x3(tangentW, bitangentW, normalW);

    v_texCoord0 = a_texCoord0;
    v_color0 = a_color0;
}
