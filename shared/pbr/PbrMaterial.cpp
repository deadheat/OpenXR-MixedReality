////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation.  All Rights Reserved
// Licensed under the MIT License. See License.txt in the project root for license information.
#pragma once

#include "pch.h"
#include "PbrCommon.h"
#include "PbrResources.h"
#include "PbrMaterial.h"
#include <SampleShared/BgfxUtility.h>
using namespace DirectX;



namespace Pbr {
    Material::Material(Pbr::Resources const& pbrResources) {
        //const CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ConstantBufferData), D3D11_BIND_CONSTANT_BUFFER);
        m_baseColorFactor = bgfx::createUniform("u_baseColorFactor", bgfx::UniformType::Vec4);
        m_metallicRoughnessNormalOcclusion = bgfx::createUniform("u_metallicRoughnessNormalOcclusion", bgfx::UniformType::Vec4);
        m_emissiveAlphaCutoff =  bgfx::createUniform("u_emissiveAlphaCutoff", bgfx::UniformType::Vec4);

        //Internal::ThrowIfFailed(pbrResources.GetDevice()->CreateBuffer(&constantBufferDesc, nullptr, m_constantBuffer.put()));
        //m_constantBuffer = bgfx::createUniform("ConstantBufferData", bgfx::UniformType::Count, sizeof(ConstantBufferData))

        for (size_t i = 0; i < TextureCount; i ++) {
            m_textures.emplace_back(bgfx::kInvalidHandle);
            m_samplers.emplace_back(bgfx::kInvalidHandle);
        }
    }

    std::shared_ptr<Material> Material::Clone(Pbr::Resources const& pbrResources) const {
        auto clone = std::make_shared<Material>(pbrResources);
        clone->Name = Name;
        clone->Hidden = Hidden;
        clone->m_parameters = m_parameters;
        clone->m_textures = m_textures;
        clone->m_samplers = m_samplers;
        clone->m_alphaBlended = m_alphaBlended;
        clone->m_doubleSided = m_doubleSided;
        clone->m_baseColorFactor = m_baseColorFactor;
        clone->m_metallicRoughnessNormalOcclusion = m_metallicRoughnessNormalOcclusion;
        clone->m_emissiveAlphaCutoff = m_emissiveAlphaCutoff;
        return clone;
    }

    /* static */
    std::shared_ptr<Material> Material::CreateFlat(const Resources& pbrResources,
                                                   RGBAColor baseColorFactor,
                                                   float roughnessFactor /* = 1.0f */,
                                                   float metallicFactor /* = 0.0f */,
                                                   RGBColor emissiveFactor /* = XMFLOAT3(0, 0, 0) */) {
        std::shared_ptr<Material> material = std::make_shared<Material>(pbrResources);

        if (baseColorFactor.w < 1.0f) { // Alpha channel
            material->SetAlphaBlended(true);
        }

        Pbr::Material::ConstantBufferData parameters = material->Parameters();
        parameters.BaseColorFactor = baseColorFactor;
        parameters.EmissiveFactor = emissiveFactor;
        parameters.MetallicFactor = metallicFactor;
        parameters.RoughnessFactor = roughnessFactor;


        // Seyi NOTE: I dont think this is putting the right samplers in place, should modify later
        shared_bgfx_handle<bgfx::UniformHandle> EnvironmentMapSampler(Pbr::Texture::CreateSampler("EnvironmentMapSampler"));
        shared_bgfx_handle<bgfx::UniformHandle> MetallicRoughnessSampler(Pbr::Texture::CreateSampler("MetallicRoughnessSampler"));
        shared_bgfx_handle<bgfx::UniformHandle> NormalSampler      (Pbr::Texture::CreateSampler("NormalSampler"));
        shared_bgfx_handle<bgfx::UniformHandle> OcclusionSampler   (Pbr::Texture::CreateSampler("OcclusionSampler"));
        shared_bgfx_handle<bgfx::UniformHandle> EmissiveSampler    (Pbr::Texture::CreateSampler("EmissiveSampler"));
        //shared_bgfx_handle<bgfx::UniformHandle> BRDFSampler        (Pbr::Texture::CreateSampler("BRDFSampler"));
        //shared_bgfx_handle<bgfx::UniformHandle> SpecularSampler    (Pbr::Texture::CreateSampler("SpecularSampler"));
        //shared_bgfx_handle<bgfx::UniformHandle> DiffuseSampler     (Pbr::Texture::CreateSampler("DiffuseSampler"));
        shared_bgfx_handle<bgfx::UniformHandle> DefaultSampler(Pbr::Texture::CreateSampler("defaultSampler"));



        material->SetTexture(ShaderSlots::BaseColor, pbrResources.CreateSolidColorTexture(RGBA::White),DefaultSampler );
        material->SetTexture(ShaderSlots::MetallicRoughness, pbrResources.CreateSolidColorTexture(RGBA::White), MetallicRoughnessSampler);
        // No occlusion.
        material->SetTexture(ShaderSlots::Occlusion, pbrResources.CreateSolidColorTexture(RGBA::White), OcclusionSampler);
        // Flat normal.
        material->SetTexture(ShaderSlots::Normal, pbrResources.CreateSolidColorTexture(RGBA::FlatNormal), NormalSampler);
        material->SetTexture(ShaderSlots::Emissive, pbrResources.CreateSolidColorTexture(RGBA::White), EmissiveSampler);

        return material;
    }

    void Material::SetTexture(ShaderSlots::PSMaterial slot,
                              _In_ shared_bgfx_handle<bgfx::TextureHandle> textureView,
                              _In_opt_ shared_bgfx_handle<bgfx::UniformHandle> sampler) {
        m_textures[slot] = std::move(textureView);

        if (sampler) {
            m_samplers[slot] = std::move(sampler);
        }
    }

    void Material::SetDoubleSided(bool doubleSided) {
        m_doubleSided = doubleSided;
    }

    void Material::SetWireframe(bool wireframeMode) {
        m_wireframe = wireframeMode;
    }

    void Material::SetAlphaBlended(bool alphaBlended) {
        m_alphaBlended = alphaBlended;
    }

    void Material::Bind(const Resources& pbrResources) const {
        // If the parameters of the constant buffer have changed, update the constant buffer.
        float* baseColorFactor = (float*) &m_parameters.BaseColorFactor;
        float metallicRoughnessNormalOcclusion[] = {
            m_parameters.MetallicFactor,
            m_parameters.RoughnessFactor,
            m_parameters.NormalScale,
            m_parameters.OcclusionStrength,
        };
        float* emissiveAlphaCutoff = {(float*)&m_parameters.EmissiveFactor};
        emissiveAlphaCutoff[3] = m_parameters.AlphaCutoff;
        // Seyi NOTE: This block of code may be irrelevant because I dont think theres a differnce in updating a uniforma and setting a uniform
        if (m_parametersChanged) {
            m_parametersChanged = false;
            
            //context->UpdateSubresource(m_constantBuffer.get(), 0, nullptr, &m_parameters, 0, 0);
        }

        // In bgfx all the state is set at once and not broken up
        pbrResources.SetState(m_alphaBlended, m_alphaBlended, m_doubleSided, m_wireframe);
        

        //pbrResources.SetBlendState(m_alphaBlended);
        //pbrResources.SetDepthStencilState(m_alphaBlended);
        //pbrResources.SetRasterizerState(m_doubleSided, m_wireframe);

        bgfx::setUniform(m_baseColorFactor, &baseColorFactor);
        bgfx::setUniform(m_metallicRoughnessNormalOcclusion, &metallicRoughnessNormalOcclusion);
        bgfx::setUniform(m_emissiveAlphaCutoff, &emissiveAlphaCutoff);

        //context->PSSetConstantBuffers(Pbr::ShaderSlots::ConstantBuffers::Material, 1, psConstantBuffers);
        //static_assert(Pbr::ShaderSlots::BaseColor == 0, "BaseColor must be the first slot");

        for (unsigned int i = 0; i < m_samplers.size(); i++) {
            bgfx::setTexture(i, m_samplers[i].get(), m_textures[i].get());
        }
        //setUniform(textures[0], textures.data(), (UINT)textures.size());
        //context->PSSetShaderResources(Pbr::ShaderSlots::BaseColor, (UINT)textures.size(), textures.data()  );

        
        //setUniform(samplers[0], samplers.data(), (UINT)samplers.size());
        //context->PSSetSamplers(Pbr::ShaderSlots::BaseColor, (UINT)samplers.size(), samplers.data());
        pbrResources.SubmitProgram();
    }

    Material::ConstantBufferData& Material::Parameters() {
        m_parametersChanged = true;
        return m_parameters;
    }

    const Material::ConstantBufferData& Material::Parameters() const {
        return m_parameters;
    }
} // namespace Pbr
