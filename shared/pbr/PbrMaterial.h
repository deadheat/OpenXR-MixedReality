////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation.  All Rights Reserved
// Licensed under the MIT License. See License.txt in the project root for license information.
#pragma once

#include <vector>
#include <array>
#include <map>
#include <memory>
#include <winrt/base.h>
#include <d3d11.h>
#include <d3d11_2.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include "PbrResources.h"

namespace Pbr {
    // A Material contains the metallic roughness parameters and textures.
    // Primitives specify which Material to use when being rendered.
    struct Material final {
#pragma warning(push)
#pragma warning(disable:4324)
        // Coefficients used by the shader. Each texture is sampled and multiplied by these coefficients.
        struct ConstantBufferData {
            // packoffset(c0)
            alignas(16) RGBAColor BaseColorFactor{1, 1, 1, 1};
            // packoffset(c1.x and c1.y)
            alignas(16) float MetallicFactor{1};
            float RoughnessFactor{1};
            // packoffset(c2)
            alignas(16) RGBColor EmissiveFactor{1, 1, 1};
            // packoffset(c3.x, c3.y and c3.z)
            alignas(16) float NormalScale{1};
            float OcclusionStrength{1};
            float AlphaCutoff{0};
        };
#pragma warning(pop)

        static_assert((sizeof(ConstantBufferData) % 16) == 0, "Constant Buffer must be divisible by 16 bytes");

        // Create a uninitialized material. Textures and shader coefficients must be set.
        Material();

        // Create a clone of this material.
        std::shared_ptr<Material> Clone(Pbr::Resources const& pbrResources) const;

        // Create a flat (no texture) material.
        static std::shared_ptr<Material> CreateFlat(const Resources& pbrResources,
                                                    RGBAColor baseColorFactor,
                                                    float roughnessFactor = 1.0f,
                                                    float metallicFactor = 0.0f,
                                                    RGBColor emissiveFactor = RGB::Black);

        // Set a Metallic-Roughness texture.
        void SetTexture(ShaderSlots::PSMaterial slot,
                        _In_ bgfx::TextureHandle* textureView,
                        _In_opt_ bgfx::UniformHandle* sampler = nullptr);

        void SetDoubleSided(bool doubleSided);
        void SetWireframe(bool wireframeMode);
        void SetAlphaBlended(bool alphaBlended);

        // Bind this material to current context.
        void Bind(const Resources& pbrResources) const;

        ConstantBufferData& Parameters();
        const ConstantBufferData& Parameters() const;

        std::string Name;
        bool Hidden{false};

    private:
        mutable bool m_parametersChanged{true};
        ConstantBufferData m_parameters;

        bool m_alphaBlended{false};
        bool m_doubleSided{false};
        bool m_wireframe{false};

        bgfx::UniformHandle m_baseColorFactor;
        bgfx::UniformHandle m_metallicRoughnessNormalOcclusion;
        bgfx::UniformHandle m_emissiveAlphaCutoff;

        static constexpr size_t TextureCount = ShaderSlots::LastMaterialSlot + 1;
        std::array<winrt::com_ptr<bgfx::TextureHandle>, TextureCount> m_textures;
        std::array<winrt::com_ptr<bgfx::UniformHandle>, TextureCount> m_samplers;
        winrt::com_ptr<bgfx::UniformHandle> m_constantBuffer;
    };
} // namespace Pbr
