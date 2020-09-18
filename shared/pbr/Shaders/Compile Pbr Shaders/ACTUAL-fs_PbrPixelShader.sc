$input v_positionWorld, v_TBN, v_texcoord0, v_color0
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
// Seyi NOTE: It seems like the original D3D11 code stored a vec3 in a vec4 , could be wrong
#define u_eyePositionVec3 u_eyePosition.xyz

uniform vec4 u_baseColorFactor;
uniform vec4 u_metallicRoughnessNormalOcclusion;
uniform vec4 u_emissiveAlphaCutoff;

#define u_metallicFactor u_metallicRoughnessNormalOcclusion.x
#define u_roughnessFactor u_metallicRoughnessNormalOcclusion.y
#define u_normalScale u_metallicRoughnessNormalOcclusion.z
#define u_occlusionStrength u_metallicRoughnessNormalOcclusion.w
#define u_emissiveFactor u_emissiveAlphaCutoff.xyz
#define u_alphaCutoff u_emissiveAlphaCutoff.w

SAMPLER2D(u_baseColorTexture, 0);
SAMPLER2D(u_metallicRoughnessTexture, 1);
SAMPLER2D(u_normalTexture, 2);
SAMPLER2D(u_occlusionTexture, 3);
SAMPLER2D(u_emissiveTexture, 4);
SAMPLER2D(u_BRDFTexture, 5);
SAMPLERCUBE(u_specularTexture, 6);
SAMPLERCUBE(u_diffuseTexture, 7);

vec3 f0 = vec3(0.04, 0.04, 0.04);
float MinRoughness = 0.04;
float PI = 3.141592653589793;

vec3 getIBLContribution(float perceptualRoughness, float NdotV, vec3 diffuseColor, vec3 specularColor, vec3 n, vec3 reflection)
{
    float lod = perceptualRoughness * u_numSpecularMipLevels;

    vec3 brdf = texture2D(u_BRDFTexture,vec2(NdotV, 1.0 - perceptualRoughness)).rgb;
    vec3 diffuseLight = textureCube(u_diffuseTexture,n).rgb;
    //DiffuseTexture.Sample(IBLSampler, n).rgb;
    vec3 specularLight = textureCubeLod(u_specularTexture,reflection,lod).rgb;
    //SpecularTexture.SampleLevel(IBLSampler, reflection, lod).rgb;
    vec3 diffuse = diffuseLight * diffuseColor;
    vec3 specular = specularLight * (specularColor * brdf.x + brdf.y);

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
    float attenuationL = 2.0 * NdotL / (NdotL + sqrt(alphaRoughness * alphaRoughness + (1.0 - alphaRoughness * alphaRoughness) * (NdotL * NdotL)));
    float attenuationV = 2.0 * NdotV / (NdotV + sqrt(alphaRoughness * alphaRoughness + (1.0 - alphaRoughness * alphaRoughness) * (NdotV * NdotV)));
    return attenuationL * attenuationV;
}

float microfacetDistribution(float NdotH, float alphaRoughness)
{
    float roughnessSq = alphaRoughness * alphaRoughness;
    float f = (NdotH * roughnessSq - NdotH) * NdotH + 1.0;
    return roughnessSq / (PI * f * f);
}

void main()
{
    vec3 mrSample = texture2D(u_metallicRoughnessTexture, v_texcoord0).xyz;
    vec4 baseColor = texture2D(u_baseColorTexture, v_texcoord0) * v_color0 * u_baseColorFactor;
    //clip(baseColor.a - u_alphaCutoff);
    if(baseColor.a  - u_alphaCutoff < 0.0) discard;

    float metallic = saturate(mrSample.b * u_metallicFactor);
    float perceptualRoughness = clamp(mrSample.g * u_roughnessFactor, MinRoughness, 1.0);
    float alphaRoughness = perceptualRoughness * perceptualRoughness;

    vec3 diffuseColor = (baseColor.rgb * (vec3(1.0, 1.0, 1.0) - f0)) * (1.0 - metallic);
    //vec3 specularColor = lerp(f0, baseColor.rgb, metallic);
    vec3 specularColor = mix(f0, baseColor.rgb, metallic);
    // Compute reflectance.
    float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

    // For typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical fresnel effect.
    // For very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing reflecance to 0%.
    float reflectance90 = saturate(reflectance * 25.0);
    vec3 specularEnvironmentR0 = specularColor.rgb;
    vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;

    // normal at surface point
    vec3 n = 2.0 * texture2D(u_normalTexture, v_texcoord0).xyz - 1.0;
    //2.0 * NormalTexture.Sample(NormalSampler, v_texcoord0) - 1.0;
    n = normalize(mul(n * vec3(u_normalScale, u_normalScale, 1.0), v_TBN));

    vec3 v = normalize(u_eyePositionVec3 - v_positionWorld);   // Vector from surface point to camera
    vec3 l = normalize(u_lightDirection);                           // Vector from surface point to light
    vec3 h = normalize(l + v);                                    // Half vector between both l and v
    vec3 reflection = -normalize(reflect(v, n));

    float NdotL = clamp(dot(n, l), 0.001, 1.0);
    float NdotV = abs(dot(n, v)) + 0.001;
    float NdotH = saturate(dot(n, h));
    float LdotH = saturate(dot(l, h));
    float VdotH = saturate(dot(v, h));

    // Calculate the shading terms for the microfacet specular shading model
    vec3 F = specularReflection(specularEnvironmentR0, specularEnvironmentR90, VdotH);
    float G = geometricOcclusion(NdotL, NdotV, alphaRoughness);
    float D = microfacetDistribution(NdotH, alphaRoughness);

    // Calculation of analytical lighting contribution
    vec3 diffuseContrib = (1.0 - F) * diffuse(diffuseColor);
    vec3 specContrib = F * G * D / (4.0 * NdotL * NdotV);
    vec3 color = NdotL * u_lightColor * (diffuseContrib + specContrib);

    // Calculate lighting contribution from image based lighting source (IBL)
    color += getIBLContribution(perceptualRoughness, NdotV, diffuseColor, specularColor, n, reflection);

    // Apply optional PBR terms for additional (optional) shading
    float ao = texture2D(u_occlusionTexture, v_texcoord0).r;
    color = mix(color, color * ao, u_occlusionStrength);

    vec3 emissive = texture2D(u_emissiveTexture, v_texcoord0) * u_emissiveFactor;
    color += emissive;
    gl_FragColor = vec4(color.x,color.y,color.z, baseColor.a);

}
