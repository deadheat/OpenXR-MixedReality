////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation.  All Rights Reserved
// Licensed under the MIT License. See License.txt in the project root for license information.

uniform mat4 u_viewProjection;
uniform vec4 u_eyePosition;
uniform mat3 u_highlightPositionLightDirectionLightColor;
uniform vec4 u_numSpecularMipLevelsAnimationTime;


// Seyi NOTE: Not sure if you can pack a mat4 like this, if you can, im really smart
#define u_highlightPosition u_highlightPositionLightDirectionLightColor[0];
#define u_lightDirection u_highlightPositionLightDirectionLightColor[1];
#define u_lightColor u_highlightPositionLightDirectionLightColor[2];
#define u_numSpecularMipLevels u_numSpecularMipLevelsAnimationTime.x;
#define u_animationTime u_numSpecularMipLevelsAnimationTime.y;


//cbuffer SceneBuffer : register(b0)
//{
//    float4x4 ViewProjection     : packoffset(c0);
//    float3 EyePosition          : packoffset(c4);
//    float3 LightDirection       : packoffset(c5);
//    float3 LightColor           : packoffset(c6);
//    int NumSpecularMipLevels    : packoffset(c7);
//    float3 HighlightPosition    : packoffset(c8);
//    float AnimationTime         : packoffset(c9);
//};
