////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation.  All Rights Reserved
// Licensed under the MIT License. See License.txt in the project root for license information.
//
// This shader is based on the fragment shader at https://github.com/KhronosGroup/glTF-WebGL-PBR
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

#include "PbrShared.sc"

uniform vec4 u_baseColorFactor;
uniform vec4 u_metallicRoughnessNormalOcclusion;
uniform vec4 u_emissiveAlphaCutoff;

#define u_metallicFactor u_metallicRoughnessNormalOcclusion.x;
#define u_roughnessFactor u_metallicRoughnessNormalOcclusion.y;
#define u_normalScale u_metallicRoughnessNormalOcclusion.z;
#define u_occlusionStrength u_metallicRoughnessNormalOcclusion.w;
#define u_emissiveFactor u_emmisiveAlphaCutoff.xyz;
#define u_alphaCutoff u_emmisiveAlphaCutoff.w;

uniform sampler u_baseColorSampler            ;   
uniform sampler u_metallicRoughnessSampler    ;   
uniform sampler u_normalSampler               ;   
uniform sampler u_occlusionSampler            ;   
uniform sampler u_emissiveSampler             ;   
uniform sampler u_BRDFSampler                 ;   
uniform sampler u_IBLSampler                  ;   

cbuffer MaterialConstantBuffer : register(b2)
{
    float4 BaseColorFactor  : packoffset(c0);
    float MetallicFactor    : packoffset(c1.x);
    float RoughnessFactor   : packoffset(c1.y);
    float3 EmissiveFactor   : packoffset(c2);
    float NormalScale       : packoffset(c3.x);
    float OcclusionStrength : packoffset(c3.y);
    float AlphaCutoff       : packoffset(c3.z);
};

// The texture registers must match the order of the MaterialTextures enum.
Texture2D<float4> BaseColorTexture          : register(t0);
Texture2D<float3> MetallicRoughnessTexture  : register(t1); // Green(y)=Roughness, Blue(z)=Metallic
Texture2D<float3> NormalTexture             : register(t2);
Texture2D<float3> OcclusionTexture          : register(t3); // Red(x) channel
Texture2D<float3> EmissiveTexture           : register(t4);
Texture2D<float3> BRDFTexture               : register(t5);
TextureCube<float3> SpecularTexture         : register(t6);
TextureCube<float3> DiffuseTexture          : register(t7);

SamplerState BaseColorSampler               : register(s0);
SamplerState MetallicRoughnessSampler       : register(s1);
SamplerState NormalSampler                  : register(s2);
SamplerState OcclusionSampler               : register(s3);
SamplerState EmissiveSampler                : register(s4);
SamplerState BRDFSampler                    : register(s5);
SamplerState IBLSampler                     : register(s6);

static const vec3 f0 = vec3(0.04, 0.04, 0.04);
static const float MinRoughness = 0.04;
static const float PI = 3.141592653589793;

vec3 getIBLContribution(float perceptualRoughness, float NdotV, vec3 diffuseColor, vec3 specularColor, vec3 n, vec3 reflection)
{
    const float lod = perceptualRoughness * NumSpecularMipLevels;

    const vec3 brdf = BRDFTexture.Sample(BRDFSampler, vec2(NdotV, 1.0 - perceptualRoughness)).rgb;

    const vec3 diffuseLight = DiffuseTexture.Sample(IBLSampler, n).rgb;
    const vec3 specularLight = SpecularTexture.SampleLevel(IBLSampler, reflection, lod).rgb;

    const vec3 diffuse = diffuseLight * diffuseColor;
    const vec3 specular = specularLight * (specularColor * brdf.x + brdf.y);

    return diffuse + specular;
}

vec3 diffuse(vec3 diffuseColor)
{
    return diffuseColor / PI;
}

vec3 specularReflection(vec3 reflectance0, vec3 reflectance90, float VdotH)
{
    return reflectance0 + (reflectance90 - reflectance0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
}

float geometricOcclusion(float NdotL, float NdotV, float alphaRoughness)
{
    const float attenuationL = 2.0 * NdotL / (NdotL + sqrt(alphaRoughness * alphaRoughness + (1.0 - alphaRoughness * alphaRoughness) * (NdotL * NdotL)));
    const float attenuationV = 2.0 * NdotV / (NdotV + sqrt(alphaRoughness * alphaRoughness + (1.0 - alphaRoughness * alphaRoughness) * (NdotV * NdotV)));
    return attenuationL * attenuationV;
}

float microfacetDistribution(float NdotH, float alphaRoughness)
{
    const float roughnessSq = alphaRoughness * alphaRoughness;
    const float f = (NdotH * roughnessSq - NdotH) * NdotH + 1.0;
    return roughnessSq / (PI * f * f);
}

vec4 main(PSInputPbr input) : SV_TARGET
{
    // Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
    // This layout intentionally reserves the 'r' channel for (optional) occlusion map data
    const vec3 mrSample = MetallicRoughnessTexture.Sample(MetallicRoughnessSampler, input.TexCoord0);
    const vec4 baseColor = BaseColorTexture.Sample(BaseColorSampler, input.TexCoord0) * input.Color0 * BaseColorFactor;

    // Discard if below alpha cutoff.
    clip(baseColor.a - AlphaCutoff);

    const float metallic = saturate(mrSample.b * MetallicFactor);
    const float perceptualRoughness = clamp(mrSample.g * RoughnessFactor, MinRoughness, 1.0);

    // Roughness is authored as perceptual roughness; as is convention,
    // convert to material roughness by squaring the perceptual roughness [2].
    const float alphaRoughness = perceptualRoughness * perceptualRoughness;

    const vec3 diffuseColor = (baseColor.rgb * (vec3(1.0, 1.0, 1.0) - f0)) * (1.0 - metallic);
    const vec3 specularColor = lerp(f0, baseColor.rgb, metallic);

    // Compute reflectance.
    const float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

    // For typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical fresnel effect.
    // For very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing reflecance to 0%.
    const float reflectance90 = saturate(reflectance * 25.0);
    const vec3 specularEnvironmentR0 = specularColor.rgb;
    const vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;

    // normal at surface point
    vec3 n = 2.0 * NormalTexture.Sample(NormalSampler, input.TexCoord0) - 1.0;
    n = normalize(mul(n * vec3(NormalScale, NormalScale, 1.0), input.TBN));

    const vec3 v = normalize(EyePosition - input.PositionWorld);   // Vector from surface point to camera
    const vec3 l = normalize(LightDirection);                           // Vector from surface point to light
    const vec3 h = normalize(l + v);                                    // Half vector between both l and v
    const vec3 reflection = -normalize(reflect(v, n));

    const float NdotL = clamp(dot(n, l), 0.001, 1.0);
    const float NdotV = abs(dot(n, v)) + 0.001;
    const float NdotH = saturate(dot(n, h));
    const float LdotH = saturate(dot(l, h));
    const float VdotH = saturate(dot(v, h));

    // Calculate the shading terms for the microfacet specular shading model
    const vec3 F = specularReflection(specularEnvironmentR0, specularEnvironmentR90, VdotH);
    const float G = geometricOcclusion(NdotL, NdotV, alphaRoughness);
    const float D = microfacetDistribution(NdotH, alphaRoughness);

    // Calculation of analytical lighting contribution
    const vec3 diffuseContrib = (1.0 - F) * diffuse(diffuseColor);
    const vec3 specContrib = F * G * D / (4.0 * NdotL * NdotV);
    vec3 color = NdotL * LightColor * (diffuseContrib + specContrib);

    // Calculate lighting contribution from image based lighting source (IBL)
    color += getIBLContribution(perceptualRoughness, NdotV, diffuseColor, specularColor, n, reflection);

    // Apply optional PBR terms for additional (optional) shading
    const float ao = OcclusionTexture.Sample(OcclusionSampler, input.TexCoord0).r;
    color = lerp(color, color * ao, OcclusionStrength);

    const vec3 emissive = EmissiveTexture.Sample(EmissiveSampler, input.TexCoord0) * EmissiveFactor;
    color += emissive;

    return vec4(color, baseColor.a);
}
