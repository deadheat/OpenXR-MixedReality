////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation.  All Rights Reserved
// Licensed under the MIT License. See License.txt in the project root for license information.
#include "pch.h"
#include "PbrCommon.h"
#include "PbrResources.h"
#include "PbrMaterial.h"
#include <PbrPixelShader.h>
#include <PbrVertexShader.h>
#include <HighlightPixelShader.h>
#include <HighlightVertexShader.h>

#include <bx/platform.h>
#include <bx/math.h>
#include <bx/pixelformat.h>

#include <bgfx/platform.h>
#include <bgfx/embedded_shader.h>

using namespace DirectX;

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
        //alignas(16) 
        DirectX::XMFLOAT4X4 ModelToWorld;
    };
} // namespace
const bgfx::EmbeddedShader s_embeddedShaders[] = {BGFX_EMBEDDED_SHADER(g_PbrPixelShader),
                                                  BGFX_EMBEDDED_SHADER(g_HighlightPixelShader),
                                                  BGFX_EMBEDDED_SHADER(g_PbrVertexShader),
                                                  BGFX_EMBEDDED_SHADER(g_HighlightVertexShader),
                                                  BGFX_EMBEDDED_SHADER_END()};
namespace Pbr {
    struct Resources::Impl {
        
        void Initialize() {

            //Internal::ThrowIfFailed(device->CreateInputLayout(Pbr::Vertex::s_vertexDesc,
            //                                                  ARRAYSIZE(Pbr::Vertex::s_vertexDesc),
            //                                                  g_PbrVertexShader,
            //                                                  sizeof(g_PbrVertexShader),
            //                                                  Resources.InputLayout.put()));
            bgfx::VertexLayout vertexLayout;
            vertexLayout.begin()
                .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
                .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
                .add(bgfx::Attrib::Tangent, 4, bgfx::AttribType::Float)
                .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
                .add(bgfx::Attrib::Indices, 1, bgfx::AttribType::Int16)
                .end();
            // Set up pixel shader.
            bgfx::RendererType::Enum type = bgfx::RendererType::Direct3D11;
            Resources.PbrPixelShader = UniqueBgfxHandle(bgfx::createEmbeddedShader(s_embeddedShaders, type, "g_PbrPixelShader"));
            
            Resources.HighlightPixelShader = UniqueBgfxHandle(bgfx::createEmbeddedShader(s_embeddedShaders, type, "g_HighlightPixelShader"));

            Resources.PbrVertexShader = UniqueBgfxHandle(bgfx::createEmbeddedShader(s_embeddedShaders, type, "g_PbrVertexShader"));

            Resources.PbrVertexShader = UniqueBgfxHandle(bgfx::createEmbeddedShader(s_embeddedShaders, type, "g_HighlightVertexShader"));

 
            // Seyi NOTE: since there are no constant buffers in bgfx, will find some type of way to implement without

            /*uniform mat4 u_viewProjection;
            uniform vec4 u_eyePosition;
            uniform mat3 u_highlightPositionLightDirectionLightColor;
            uniform vec4 u_numSpecularMipLevelsAnimationTime;*/

            Resources.AllUniformHandles->ViewProjection = 
                bgfx::createUniform("u_viewProjection", bgfx::UniformType::Mat4, 1);
            Resources.AllUniformHandles->EyePosition =
                bgfx::createUniform("u_eyePosition", bgfx::UniformType::Vec4, 1);
            Resources.AllUniformHandles->HighlightPositionLightDirectionLightColor = 
                bgfx::createUniform("u_highlightPositionLightDirectionLightColor", bgfx::UniformType::Mat3, 1);
            Resources.AllUniformHandles->NumSpecularMipLevelsAnimationTime =
                bgfx::createUniform("u_numSpecularMipLevelsAnimationTime", bgfx::UniformType::Vec4, 1);
            /*static_assert((sizeof(SceneConstantBuffer) % 16) == 0, "Constant Buffer must be divisible by 16 bytes");
            const CD3D11_BUFFER_DESC pbrConstantBufferDesc(sizeof(SceneConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
            Internal::ThrowIfFailed(device->CreateBuffer(&pbrConstantBufferDesc, nullptr, Resources.SceneConstantBuffer.put()));*/

            Resources.AllUniformHandles->ModelToWorld = 
                bgfx::createUniform("u_modelToWorld", bgfx::UniformType::Mat4, 1);
            /*static_assert((sizeof(ModelConstantBuffer) % 16) == 0, "Constant Buffer must be divisible by 16 bytes");
            const CD3D11_BUFFER_DESC modelConstantBufferDesc(sizeof(ModelConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
            Internal::ThrowIfFailed(device->CreateBuffer(&modelConstantBufferDesc, nullptr, Resources.ModelConstantBuffer.put()));*/

            //ID3D11Device* device = (ID3D11Device*)bgfx::getInternalData()->context;
            // Samplers for environment map and BRDF.
            Resources.EnvironmentMapSampler = Texture::CreateSampler("EnvironmentMapSampler");

            Resources.MetallicRoughnessSampler = Texture::CreateSampler("u_metallicRoughnessTexture");
            Resources.NormalSampler = Texture::CreateSampler("u_normalTexture");
            Resources.OcclusionSampler = Texture::CreateSampler("u_occlusionTexture");
            Resources.EmissiveSampler = Texture::CreateSampler("u_emissiveTexture");
            Resources.BRDFSampler = Texture::CreateSampler("u_BRDFTexture");
            Resources.SpecularSampler = Texture::CreateSampler("u_specularTexture");
            Resources.DiffuseSampler = Texture::CreateSampler("u_diffuseTexture");

            uint64_t defaultState = BGFX_STATE_DEFAULT;
            Resources.DefaultBlendState.copy_from(&defaultState);
            // CD3D11_BLEND_DESC blendStateDesc(D3D11_DEFAULT);
            // Internal::ThrowIfFailed(device->CreateBlendState(&blendStateDesc, Resources.DefaultBlendState.put()));

            uint64_t rtBlendMode =
                BGFX_STATE_DEFAULT |
                BGFX_STATE_BLEND_EQUATION_ADD |
                BGFX_STATE_BLEND_FUNC_SEPARATE(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA,BGFX_STATE_BLEND_ZERO,BGFX_STATE_BLEND_ONE) |
                BGFX_STATE_WRITE_RGB | 
                BGFX_STATE_WRITE_A;
            Resources.AlphaBlendState.copy_from(&rtBlendMode);
            // D3D11_RENDER_TARGET_BLEND_DESC rtBlendDesc;
            // rtBlendDesc.BlendEnable = TRUE;
            // rtBlendDesc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
            // rtBlendDesc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            // rtBlendDesc.BlendOp = D3D11_BLEND_OP_ADD;
            // rtBlendDesc.SrcBlendAlpha = D3D11_BLEND_ZERO;
            // rtBlendDesc.DestBlendAlpha = D3D11_BLEND_ONE;
            // rtBlendDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
            // rtBlendDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            // for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
            //    blendStateDesc.RenderTarget[i] = rtBlendDesc;
            //}
            //Internal::ThrowIfFailed(device->CreateBlendState(&blendStateDesc, Resources.AlphaBlendState.put()));

            for (bool doubleSided : {false, true}) {
                for (bool wireframe : {false, true}) {
                    for (bool frontCounterClockwise : {false, true}) {
                        //Seyi NOTE: Wireframe is set in debugging in bgfx and not rasterizer so find a way to implement
                        uint64_t rastState = BGFX_STATE_DEFAULT;
                        rastState = doubleSided ? rastState : rastState | BGFX_STATE_CULL_CCW;
                        Resources.RasterizerStates[doubleSided][wireframe][frontCounterClockwise].copy_from(&rastState);
                        //CD3D11_RASTERIZER_DESC rasterizerDesc(D3D11_DEFAULT);
                        //rasterizerDesc.CullMode = doubleSided ? D3D11_CULL_NONE : D3D11_CULL_BACK;
                        //rasterizerDesc.FillMode = wireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
                        //rasterizerDesc.FrontCounterClockwise = frontCounterClockwise;
                        //Internal::ThrowIfFailed(device->CreateRasterizerState(
                        //    &rasterizerDesc, Resources.RasterizerStates[doubleSided][wireframe][frontCounterClockwise].put()));
                    }
                }
            }

            for (bool reverseZ : {false, true}) {
                for (bool noWrite : {false, true}) {
                    uint64_t rastState = BGFX_STATE_DEFAULT;
                    rastState = rastState | (reverseZ ? BGFX_STATE_DEPTH_TEST_GREATER : BGFX_STATE_DEPTH_TEST_LESS);
                    rastState = rastState | ( noWrite ? rastState : BGFX_STATE_WRITE_R);
                    Resources.DepthStencilStates[reverseZ][noWrite].copy_from(&rastState);
                }
            }
        }

        struct DeviceResources {
            UniqueBgfxHandle<bgfx::UniformHandle> EnvironmentMapSampler;


            UniqueBgfxHandle<bgfx::UniformHandle> MetallicRoughnessSampler;
            UniqueBgfxHandle<bgfx::UniformHandle> NormalSampler; 
            UniqueBgfxHandle<bgfx::UniformHandle> OcclusionSampler;
            UniqueBgfxHandle<bgfx::UniformHandle> EmissiveSampler; 
            UniqueBgfxHandle<bgfx::UniformHandle> BRDFSampler;
            UniqueBgfxHandle<bgfx::UniformHandle> SpecularSampler; 
            UniqueBgfxHandle<bgfx::UniformHandle> DiffuseSampler;
            UniqueBgfxHandle<bgfx::ProgramHandle> ShaderProgram;
            UniqueBgfxHandle<bgfx::VertexLayout> InputLayout;
            UniqueBgfxHandle<bgfx::EmbeddedShader> PbrVertexShader;
            UniqueBgfxHandle<bgfx::EmbeddedShader> PbrPixelShader;
            UniqueBgfxHandle<bgfx::EmbeddedShader> HighlightVertexShader;
            UniqueBgfxHandle<bgfx::EmbeddedShader> HighlightPixelShader;
            //winrt::com_ptr<ID3D11Buffer> SceneConstantBuffer;
            winrt::com_ptr<SceneUniforms> SceneUniforms;
            winrt::com_ptr<UniformHandles> AllUniformHandles;
            winrt::com_ptr<ModelConstantUniform> ModelConstantUniform;
            UniqueBgfxHandle<bgfx::TextureHandle> BrdfLut;
            UniqueBgfxHandle<bgfx::TextureHandle> SpecularEnvironmentMap;
            UniqueBgfxHandle<bgfx::TextureHandle> DiffuseEnvironmentMap;
            winrt::com_ptr<uint64_t> AlphaBlendState;
            winrt::com_ptr<uint64_t> DefaultBlendState;
            winrt::com_ptr<uint64_t>
                RasterizerStates[2][2][2]; // Three dimensions for [DoubleSide][Wireframe][FrontCounterClockWise]
            uint64_t StateFlags;
            winrt::com_ptr<uint64_t> DepthStencilStates[2][2]; // Two dimensions for [ReverseZ][NoWrite]
            mutable std::map<uint32_t, UniqueBgfxHandle<bgfx::TextureHandle>> SolidColorTextureCache;
        };

        DeviceResources Resources;
        SceneUniforms SceneUniformsInstance;
        ModelConstantUniform ModelBuffer;

        Duration HighlightAnimationTimeStart;
        DirectX::XMFLOAT3 HighlightPulseLocation;

        ShadingMode Shading = ShadingMode::Regular;
        FillMode Fill = FillMode::Solid;
        FrontFaceWindingOrder WindingOrder = FrontFaceWindingOrder::ClockWise;
        bool ReverseZ = false;
        mutable std::mutex m_cacheMutex;
    };

    Resources::Resources()
        : m_impl(std::make_unique<Impl>()) {
        m_impl->Initialize();
    }

    Resources::Resources(Resources&& resources) = default;

    Resources::~Resources() = default;

    void Resources::SetBrdfLut(_In_ bgfx::TextureHandle* brdfLut) {
        m_impl->Resources.BrdfLut = UniqueBgfxHandle(*brdfLut);
    }

    void Resources::CreateDeviceDependentResources() {
        m_impl->Initialize();
    }

    void Resources::ReleaseDeviceDependentResources() {
        m_impl->Resources = {};
    }


    winrt::com_ptr<ID3D11Device> Resources::GetDevice() const {
        winrt::com_ptr<ID3D11Device> device;
        //m_impl->Resources.SceneConstantBuffer->GetDevice(device.put());
        return device;
    }
    void Resources::SubmitProgram() const {
        // Need to submit program somehow
        //(*pbrResources.m_impl.get()).Resources
        bgfx::submit(0,m_impl->Resources.ShaderProgram.Get());
        // ;
    }


    void Resources::SetLight(DirectX::XMFLOAT3 direction, RGBColor diffuseColor) {
        // Setting light direction first
        m_impl->SceneUniformsInstance.u_highlightPositionLightDirectionLightColor[1][0] = direction.x;
        m_impl->SceneUniformsInstance.u_highlightPositionLightDirectionLightColor[1][1] = direction.y;
        m_impl->SceneUniformsInstance.u_highlightPositionLightDirectionLightColor[1][2] = direction.z;
        // Now setting light color
        m_impl->SceneUniformsInstance.u_highlightPositionLightDirectionLightColor[2][0] = diffuseColor.x;
        m_impl->SceneUniformsInstance.u_highlightPositionLightDirectionLightColor[2][1] = diffuseColor.y;
        m_impl->SceneUniformsInstance.u_highlightPositionLightDirectionLightColor[2][2] = diffuseColor.z;
    }

    void Resources::UpdateAnimationTime(Duration currentTotalTimeElapsed) {
        // Animation time is kept at the end of the vector
        m_impl->SceneUniformsInstance.u_numSpecularMipLevelsAnimationTime[1] =
            std::chrono::duration<float>(currentTotalTimeElapsed - m_impl->HighlightAnimationTimeStart).count();
    }

    void Resources::StartHighlightAnimation(DirectX::XMFLOAT3 location, Duration currentTotalTimeElapsed) {
        m_impl->SceneUniformsInstance.u_numSpecularMipLevelsAnimationTime[3] = 0;
        m_impl->HighlightAnimationTimeStart = currentTotalTimeElapsed;
        // Set highlight position to location
        m_impl->SceneUniformsInstance.u_highlightPositionLightDirectionLightColor[0][0] = location.x;
        m_impl->SceneUniformsInstance.u_highlightPositionLightDirectionLightColor[0][1] = location.y;
        m_impl->SceneUniformsInstance.u_highlightPositionLightDirectionLightColor[0][2] = location.z;
    }

    void XM_CALLCONV Resources::SetModelToWorld(DirectX::FXMMATRIX modelToWorld) const {
        //XMStoreFloat4x4(&m_impl->ModelBuffer.ModelToWorld, XMMatrixTranspose(modelToWorld));
        bgfx::setUniform(m_impl->Resources.AllUniformHandles->ModelToWorld, &modelToWorld, 1);
        //context->UpdateSubresource(m_impl->Resources.ModelConstantBuffer.get(), 0, nullptr, &m_impl->ModelBuffer, 0, 0);
    }
    // DirectX::XMFLOAT4X4 u_viewProjection;
    // float[4] u_eyePosition;
    // float[3][3] u_highlightPositionLightDirectionLightColor;
    // float[4] u_numSpecularMipLevelsAnimationTime;
    void XM_CALLCONV Resources::SetViewProjection(DirectX::FXMMATRIX view, DirectX::CXMMATRIX projection) {
        XMStoreFloat4x4(&m_impl->SceneUniformsInstance.u_viewProjection, XMMatrixTranspose(XMMatrixMultiply(view, projection)));
        XMStoreFloat4(&m_impl->SceneUniformsInstance.u_eyePosition, XMMatrixInverse(nullptr, view).r[3]);
    }

    void Resources::SetEnvironmentMap(_In_ bgfx::TextureHandle* specularEnvironmentMap,
                                      _In_ bgfx::TextureHandle* diffuseEnvironmentMap,
                                      std::map<std::string, bgfx::TextureInfo>& textureInformation) {
        // D3D11_SHADER_RESOURCE_VIEW_DESC desc;
        // diffuseEnvironmentMap->GetDesc(&desc);
        // Seyi NOTE: Not checking to see if it is a cube map, could be a source of errors
        
        
        //if (!diffuseEnvironmentMap->cubeMap /*desc.ViewDimension != D3D_SRV_DIMENSION_TEXTURECUBE*/) {
        //    // throw std::exception("Diffuse Resource View Type is not D3D_SRV_DIMENSION_TEXTURECUBE");
        //    throw std::exception("Diffuse Resource View Type is not cubeMap");
        //}

        //// specularEnvironmentMap->GetDesc(&desc);
        //if (!specularEnvironmentMap->cubeMap /*desc.ViewDimension != D3D_SRV_DIMENSION_TEXTURECUBE*/) {
        //    throw std::exception("Specular Resource View Type is not cubeMap");
        //}

        // m_impl->SceneBuffer.NumSpecularMipLevels = desc.TextureCube.MipLevels;
        m_impl->SceneUniformsInstance.u_numSpecularMipLevelsAnimationTime[0] = textureInformation["specularEnvironmentView"].numMips;
        m_impl->Resources.SpecularEnvironmentMap = UniqueBgfxHandle(*specularEnvironmentMap);
        m_impl->Resources.DiffuseEnvironmentMap = UniqueBgfxHandle(*diffuseEnvironmentMap);
    }

    UniqueBgfxHandle<bgfx::TextureHandle> Resources::CreateSolidColorTexture(RGBAColor color) const {
        const std::array<uint8_t, 4> rgba = Texture::LoadRGBAUI4(color);

        // Check cache to see if this flat texture already exists.
        const uint32_t colorKey = *reinterpret_cast<const uint32_t*>(rgba.data());
        {
            std::lock_guard guard(m_impl->m_cacheMutex);
            auto textureIt = m_impl->Resources.SolidColorTextureCache.find(colorKey);
            if (textureIt != m_impl->Resources.SolidColorTextureCache.end()) {
                return std::move(textureIt->second);
            }
        }

        UniqueBgfxHandle<bgfx::TextureHandle> texture =
            Pbr::Texture::CreateTexture(rgba.data(), 1, 1, 1, sample::bg::DxgiFormatToBgfxFormat(DXGI_FORMAT_R8G8B8A8_UNORM));
        std::lock_guard guard(m_impl->m_cacheMutex);
        // If the key already exists then the existing texture will be returned.
        return std::move(m_impl->Resources.SolidColorTextureCache.emplace(colorKey, texture).first->second);
    }
    /*uniform mat4 u_viewProjection;
            uniform vec4 u_eyePosition;
            uniform mat3 u_highlightPositionLightDirectionLightColor;
            uniform vec4 u_numSpecularMipLevelsAnimationTime;*/

    // DirectX::XMFLOAT4X4 u_viewProjection;
    //DirectX::XMFLOAT4 u_eyePosition;
    //float[3][3] u_highlightPositionLightDirectionLightColor;
    //float[4] u_numSpecularMipLevelsAnimationTime;

    void Resources::Bind() const {
        //context->UpdateSubresource(m_impl->Resources.SceneConstantBuffer.get(), 0, nullptr, &m_impl->SceneBuffer, 0, 0);
        bgfx::setUniform(m_impl->Resources.AllUniformHandles->ViewProjection, &m_impl->SceneUniformsInstance.u_viewProjection);
        bgfx::setUniform(m_impl->Resources.AllUniformHandles->EyePosition, &m_impl->SceneUniformsInstance.u_eyePosition);
        bgfx::setUniform(m_impl->Resources.AllUniformHandles->HighlightPositionLightDirectionLightColor,
                   &m_impl->SceneUniformsInstance.u_highlightPositionLightDirectionLightColor);
        bgfx::setUniform(m_impl->Resources.AllUniformHandles->NumSpecularMipLevelsAnimationTime,
                   &m_impl->SceneUniformsInstance.u_numSpecularMipLevelsAnimationTime);
        bgfx::setUniform(m_impl->Resources.AllUniformHandles->ModelToWorld, &m_impl->ModelBuffer.ModelToWorld);

   /*     if (m_impl->Shading == ShadingMode::Highlight) {
            context->VSSetShader(m_impl->Resources.HighlightVertexShader.get(), nullptr, 0);
            context->PSSetShader(m_impl->Resources.HighlightPixelShader.get(), nullptr, 0);
        } else {
            context->VSSetShader(m_impl->Resources.PbrVertexShader.get(), nullptr, 0);
            context->PSSetShader(m_impl->Resources.PbrPixelShader.get(), nullptr, 0);
        }*/

        const bgfx::ShaderHandle vsh;
        const bgfx::ShaderHandle fsh;

        if (m_impl->Shading == ShadingMode::Highlight) {
            vsh = m_impl->Resources.HighlightVertexShader.Get();
            fsh = m_impl->Resources.HighlightPixelShader.Get();
        } else {
            vsh = m_impl->Resources.PbrVertexShader.Get();
            fsh = m_impl->Resources.PbrPixelShader.Get();
        }

        m_impl->Resources.ShaderProgram =
            UniqueBgfxHandle<bgfx::ProgramHandle>(bgfx::createProgram(vsh, fsh, true /* destroy shaders when program is destroyed */));

        /*ID3D11Buffer* vsBuffers[] = {m_impl->Resources.SceneConstantBuffer.get(), m_impl->Resources.ModelConstantBuffer.get()};
        context->VSSetConstantBuffers(Pbr::ShaderSlots::ConstantBuffers::Scene, _countof(vsBuffers), vsBuffers);
        ID3D11Buffer* psBuffers[] = {m_impl->Resources.SceneConstantBuffer.get()};
        context->PSSetConstantBuffers(Pbr::ShaderSlots::ConstantBuffers::Scene, _countof(psBuffers), psBuffers);
        context->IASetInputLayout(m_impl->Resources.InputLayout.get());*/

        /*static_assert(ShaderSlots::DiffuseTexture == ShaderSlots::SpecularTexture + 1, "Diffuse must follow Specular slot");
        static_assert(ShaderSlots::SpecularTexture == ShaderSlots::Brdf + 1, "Specular must follow BRDF slot");*/

        bgfx::setTexture(5, m_impl->Resources.BRDFSampler.Get(), m_impl->Resources.BrdfLut.Get());
        bgfx::setTexture(6, m_impl->Resources.SpecularSampler.Get(), m_impl->Resources.SpecularEnvironmentMap.Get());
        bgfx::setTexture(7, m_impl->Resources.DiffuseSampler.Get(), m_impl->Resources.DiffuseEnvironmentMap.Get());

   /*     bgfx::TextureHandle* shaderRes ources[] = {
            m_impl->Resources.BrdfLut.get(), m_impl->Resources.SpecularEnvironmentMap.get(), m_impl->Resources.DiffuseEnvironmentMap.get()};
        context->PSSetShaderResources(Pbr::ShaderSlots::Brdf, _countof(shaderResources), shaderResources);
        ID3D11SamplerState* samplers[] = {m_impl->Resources.BrdfSampler.get(), m_impl->Resources.EnvironmentMapSampler.get()};
        context->PSSetSamplers(ShaderSlots::Brdf, _countof(samplers), samplers);*/
    }

    void Resources::SetShadingMode(ShadingMode mode) {
        m_impl->Shading = mode;
    }

    ShadingMode Resources::GetShadingMode() const {
        return m_impl->Shading;
    }

    void Resources::SetFillMode(FillMode mode) {
        m_impl->Fill = mode;
    }

    FillMode Resources::GetFillMode() const {
        return m_impl->Fill;
    }

    void Resources::SetFrontFaceWindingOrder(FrontFaceWindingOrder windingOrder) {
        m_impl->WindingOrder = windingOrder;
    }

    FrontFaceWindingOrder Resources::GetFrontFaceWindingOrder() const {
        return m_impl->WindingOrder;
    }

    void Resources::SetDepthFuncReversed(bool reverseZ) {
        m_impl->ReverseZ = reverseZ;
    }

    void Resources::SetState(bool blended, bool doubleSided, bool wireframe, bool disableDepthWrite) const {
        // I dereference because .get returns a pointer to the flag but I need the actual flag to or it
        // Set blend state
        m_impl->Resources.StateFlags =
            m_impl->Resources.StateFlags | (blended ? *m_impl->Resources.AlphaBlendState.get() : *m_impl->Resources.DefaultBlendState.get());
        // Set Rasterizer State
        m_impl->Resources.StateFlags =
            m_impl->Resources.StateFlags | *m_impl->Resources.RasterizerStates[doubleSided ? 1 : 0][wireframe ? 1 : 0]
                                                                 [m_impl->WindingOrder == FrontFaceWindingOrder::CounterClockWise ? 1 : 0].get();
        // Set Depth Stencil State
        m_impl->Resources.StateFlags = disableDepthWrite ? m_impl->Resources.StateFlags : m_impl->Resources.StateFlags | BGFX_STATE_WRITE_R;
        m_impl->Resources.StateFlags = m_impl->Resources.StateFlags | BGFX_STATE_WRITE_MASK |
                                       (m_impl->ReverseZ ? BGFX_STATE_DEPTH_TEST_GREATER : BGFX_STATE_DEPTH_TEST_LESS) |
                                       BGFX_STATE_CULL_CCW;
        bgfx::setState(m_impl->Resources.StateFlags);
    }
    //void Resources::SetBlendState(_In_ ID3D11DeviceContext* context, bool enabled) const {
    //    context->OMSetBlendState(
    //        enabled ? m_impl->Resources.AlphaBlendState.get() : m_impl->Resources.DefaultBlendState.get(), nullptr, 0xFFFFFF);
    //}

    //void Resources::SetRasterizerState(_In_ ID3D11DeviceContext* context, bool doubleSided, bool wireframe) const {
    //    context->RSSetState(m_impl->Resources
    //                            .RasterizerStates[doubleSided ? 1 : 0][wireframe ? 1 : 0]
    //                                             [m_impl->WindingOrder == FrontFaceWindingOrder::CounterClockWise ? 1 : 0]
    //                            .get());
    //}

    //void Resources::SetDepthStencilState(bool disableDepthWrite) const {
    //    m_impl->Resources.StateFlags = disableDepthWrite ? m_impl->Resources.StateFlags : m_impl->Resources.StateFlags | BGFX_STATE_WRITE_R;
    //    m_impl->Resources.StateFlags = m_impl->Resources.StateFlags |
    //        BGFX_STATE_WRITE_MASK | (reversedZ ? BGFX_STATE_DEPTH_TEST_GREATER : BGFX_STATE_DEPTH_TEST_LESS) | BGFX_STATE_CULL_CCW;
    //    //context->OMSetDepthStencilState(m_impl->Resources.DepthStencilStates[m_impl->ReverseZ ? 1 : 0][disableDepthWrite ? 1 : 0].get(), 1);
    //}
} // namespace Pbr
