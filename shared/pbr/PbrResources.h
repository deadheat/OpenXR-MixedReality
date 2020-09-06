////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation.  All Rights Reserved
// Licensed under the MIT License. See License.txt in the project root for license information.
#pragma once

#include <vector>
#include <map>
#include <memory>
#include <winrt/base.h>
#include <d3d11.h>
#include <d3d11_2.h>
#include <DirectXMath.h>
#include "PbrCommon.h"

namespace Pbr {
    namespace {
        struct UniformHandles {
            bgfx::UniformHandle ViewProjection;
            bgfx::UniformHandle EyePosition;
            bgfx::UniformHandle HighlightPositionLightDirectionLightColor;
            bgfx::UniformHandle NumSpecularMipLevelsAnimationTime;
            bgfx::UniformHandle ModelToWorld;
        };

        struct SceneUniforms {
            DirectX::XMFLOAT4X4 u_viewProjection;
            DirectX::XMFLOAT4 u_eyePosition;
            float u_highlightPositionLightDirectionLightColor[3][3];
            float u_numSpecularMipLevelsAnimationTime[4];
        };

        struct ModelConstantUniform {
            // alignas(16)
            DirectX::XMFLOAT4X4 ModelToWorld;
        };
    } // namespace
    struct DeviceResources {
        //unique_bgfx_handle<bgfx::UniformHandle> EnvironmentMapSampler;

        //unique_bgfx_handle<bgfx::UniformHandle> MetallicRoughnessSampler;
        //unique_bgfx_handle<bgfx::UniformHandle> NormalSampler;
        //unique_bgfx_handle<bgfx::UniformHandle> OcclusionSampler;
        //unique_bgfx_handle<bgfx::UniformHandle> EmissiveSampler;
        unique_bgfx_handle<bgfx::UniformHandle> BRDFSampler;
        unique_bgfx_handle<bgfx::UniformHandle> SpecularSampler;
        unique_bgfx_handle<bgfx::UniformHandle> DiffuseSampler;
        unique_bgfx_handle<bgfx::ProgramHandle> ShaderProgram;
        // winrt::com_ptr<bgfx::VertexLayout> InputLayout;
        unique_bgfx_handle<bgfx::ShaderHandle> PbrVertexShader;
        unique_bgfx_handle<bgfx::ShaderHandle> PbrPixelShader;
        unique_bgfx_handle<bgfx::ShaderHandle> HighlightVertexShader;
        unique_bgfx_handle<bgfx::ShaderHandle> HighlightPixelShader;
        // winrt::com_ptr<ID3D11Buffer> SceneConstantBuffer;
        SceneUniforms SceneUniforms;
        UniformHandles AllUniformHandles;
        ModelConstantUniform ModelConstantUniform;
        unique_bgfx_handle<bgfx::TextureHandle> BrdfLut;
        unique_bgfx_handle<bgfx::TextureHandle> SpecularEnvironmentMap;
        unique_bgfx_handle<bgfx::TextureHandle> DiffuseEnvironmentMap;
        uint64_t AlphaBlendState;
        uint64_t DefaultBlendState;
        uint64_t RasterizerStates[2][2][2]; // Three dimensions for [DoubleSide][Wireframe][FrontCounterClockWise]
        uint64_t StateFlags{0};
        uint64_t DepthStencilStates[2][2]; // Two dimensions for [ReverseZ][NoWrite]
        mutable std::map<uint32_t, shared_bgfx_handle<bgfx::TextureHandle>> SolidColorTextureCache;
    };
    namespace ShaderSlots {
        enum VSResourceViews {
            Transforms = 0,
        };

        enum PSMaterial { // For both samplers and textures.
            BaseColor = 0,
            MetallicRoughness,
            Normal,
            Occlusion,
            Emissive,
            LastMaterialSlot = Emissive
        };

        enum Pbr { // For both samplers and textures.
            Brdf = LastMaterialSlot + 1
        };

        enum EnvironmentMap { // For both samplers and textures.
            SpecularTexture = Brdf + 1,
            DiffuseTexture = SpecularTexture + 1,
            EnvironmentMapSampler = Brdf + 1
        };

        enum ConstantBuffers {
            Scene,    // Used by VS and PS
            Model,    // PS only
            Material, // PS only
        };
    } // namespace ShaderSlots

    enum class ShadingMode : uint32_t {
        Regular,
        Highlight,
    };

    enum class FillMode : uint32_t {
        Solid,
        Wireframe,
    };

    enum class FrontFaceWindingOrder : uint32_t {
        ClockWise,
        CounterClockWise,
    };

    // Global PBR resources required for rendering a scene.
     struct Resources final {
        explicit Resources();
        Resources(Resources&&);

        ~Resources();

        // Submit the program
        void SubmitProgram() const ;
        // Sets the Bidirectional Reflectance Distribution Function Lookup Table texture, required by the shader to compute surface
        // reflectance from the IBL.
        void SetBrdfLut(_In_ unique_bgfx_handle<bgfx::TextureHandle>&& brdfLut);

        // Create device-dependent resources.
        void CreateDeviceDependentResources();

        // Release device-dependent resources.
        void ReleaseDeviceDependentResources();

        // Get the D3D11Device that the PBR resources are associated with.
        //winrt::com_ptr<ID3D11Device> GetDevice() const;

        // Set the directional light.
        void SetLight(DirectX::XMFLOAT3 direction, RGBColor diffuseColor);

        // Calculate the time since the last highlight ripple effect was started.
        using Duration = std::chrono::high_resolution_clock::duration;
        void UpdateAnimationTime(Duration currentTotalTimeElapsed);

        // Start the highlight pulse animation from the given location.
        void StartHighlightAnimation(DirectX::XMFLOAT3 location, Duration currentTotalTimeElapsed);

        // Set the specular and diffuse image-based lighting (IBL) maps. ShaderResourceViews must be TextureCubes.
        void SetEnvironmentMap(_In_ unique_bgfx_handle<bgfx::TextureHandle>&& specularEnvironmentMap,
                               _In_ unique_bgfx_handle<bgfx::TextureHandle>&& diffuseEnvironmentMap,
                               std::map<std::string, bgfx::TextureInfo>& textureInformation);

        // Set the current view and projection matrices.
        void XM_CALLCONV SetViewProjection(DirectX::FXMMATRIX view, DirectX::CXMMATRIX projection);

        // Many 1x1 pixel colored textures are used in the PBR system. This is used to create textures backed by a cache to reduce the
        // number of textures created.
        shared_bgfx_handle<bgfx::TextureHandle> CreateSolidColorTexture(RGBAColor color) const;

        // Bind the the PBR resources to the current context.
        void Bind() const;

        // Set and update the model to world constant buffer value.
        void XM_CALLCONV SetModelToWorld(DirectX::FXMMATRIX modelToWorld) const;

        // Set or get the shading and fill modes.
        void SetShadingMode(ShadingMode mode);
        ShadingMode GetShadingMode() const;
        void SetFillMode(FillMode mode);
        FillMode GetFillMode() const;
        void SetFrontFaceWindingOrder(FrontFaceWindingOrder windingOrder);
        FrontFaceWindingOrder GetFrontFaceWindingOrder() const;

        void SetDepthFuncReversed(bool reverseZ);
        DeviceResources* getResources();

    private:
        void SetState(bool blended, bool doubleSided, bool wireframe, bool disableDepthWrite) const;

        friend struct Material;

        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
} // namespace Pbr
