////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation.  All Rights Reserved
// Licensed under the MIT License. See License.txt in the project root for license information.
#include "pch.h"
#include "PbrCommon.h"
#include "PbrResources.h"
#include "PbrPrimitive.h"
#include <windows.h>
#include <XrUtility/XrEnumerate.h>
#include <XrUtility/XrToString.h>
#include <XrUtility/XrViewConfiguration.h>
#include <DirectXMath.h>

#include <SampleShared/FileUtility.h>
#include <SampleShared/BgfxUtility.h>
#include <SampleShared/Trace.h>

#include <bx/platform.h>
#include <bx/math.h>
#include <bx/pixelformat.h>

#include <bgfx/platform.h>
#include <bgfx/embedded_shader.h>

using namespace DirectX;
constexpr uint32_t MaxViewInstance = 2;

namespace {
    UINT GetPbrVertexByteSize(size_t size) {
        return (UINT)(sizeof(decltype(Pbr::PrimitiveBuilder::Vertices)::value_type) * size);
    }

   winrt::com_ptr<bgfx::VertexBufferHandle> CreateVertexBuffer(const Pbr::PrimitiveBuilder& primitiveBuilder,
                                                    bool updatableBuffers) {
        // Create Vertex Buffer BGFX
        bgfx::VertexLayout vertexLayout;
        

        vertexLayout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Tangent, 4, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Indices, 1, bgfx::AttribType::Int16)
            .end();

        // Create Vertex Buffer
        D3D11_BUFFER_DESC desc{};
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.ByteWidth = GetPbrVertexByteSize(primitiveBuilder.Vertices.size());
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        if (updatableBuffers) {
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
        }

        D3D11_SUBRESOURCE_DATA initData{};
        initData.pSysMem = primitiveBuilder.Vertices.data();

        winrt::com_ptr<bgfx::VertexBufferHandle> vertexBuffer;
     
        

        bgfx::VertexBufferHandle rawVertexBuffer = 
            bgfx::createVertexBuffer(bgfx::makeRef(primitiveBuilder.Vertices.data(), sizeof(primitiveBuilder.Vertices.data())), vertexLayout);
        vertexBuffer.copy_from(&rawVertexBuffer);
        return vertexBuffer;
    }

     winrt::com_ptr<bgfx::IndexBufferHandle> CreateIndexBuffer(const Pbr::PrimitiveBuilder& primitiveBuilder,
                                                   bool updatableBuffers) {
        //Create bgfx Index Buffer
        // Create Index Buffer
        D3D11_BUFFER_DESC desc{};
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.ByteWidth = (UINT)(sizeof(decltype(primitiveBuilder.Indices)::value_type) * primitiveBuilder.Indices.size());
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        if (updatableBuffers) {
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
        }

        D3D11_SUBRESOURCE_DATA initData{};
        initData.pSysMem = primitiveBuilder.Indices.data();

        winrt::com_ptr<bgfx::IndexBufferHandle> indexBuffer;
       

        bgfx::IndexBufferHandle rawIndexBuffer = 
            bgfx::createIndexBuffer(bgfx::makeRef(primitiveBuilder.Indices.data(), sizeof(primitiveBuilder.Indices.data())));
        indexBuffer.copy_from(&rawIndexBuffer);
        return indexBuffer;
    }
} // namespace

namespace Pbr {
    const bgfx::RendererType::Enum type = bgfx::getRendererType();

    Primitive::Primitive(UINT indexCount,
                         winrt::com_ptr<bgfx::IndexBufferHandle> indexBuffer,
                         winrt::com_ptr<bgfx::VertexBufferHandle> vertexBuffer,
                         std::shared_ptr<Material> material)
        : m_indexCount(indexCount)
        , m_indexBuffer(std::move(indexBuffer))
        , m_vertexBuffer(std::move(vertexBuffer))
        , m_material(std::move(material)) {
    }

    Primitive::Primitive(Pbr::Resources const& pbrResources,
                         const Pbr::PrimitiveBuilder& primitiveBuilder,
                         std::shared_ptr<Pbr::Material> material,
                         bool updatableBuffers)
        : Primitive((UINT)primitiveBuilder.Indices.size(),
                    CreateIndexBuffer(primitiveBuilder, updatableBuffers),
                    CreateVertexBuffer(primitiveBuilder, updatableBuffers),
                    std::move(material)) {
    }

    Primitive Primitive::Clone(Pbr::Resources const& pbrResources) const {
        return Primitive(m_indexCount, m_indexBuffer, m_vertexBuffer, m_material->Clone(pbrResources));
    }

    void Primitive::UpdateBuffers(const Pbr::PrimitiveBuilder& primitiveBuilder) {
        //TODO figure out how to implement updatable logic
        // Update vertex buffer.
        {
            /*D3D11_BUFFER_DESC vertDesc;
            m_vertexBuffer->GetDesc(&vertDesc);*/

            UINT requiredSize = GetPbrVertexByteSize(primitiveBuilder.Vertices.size());
            if (false /*vertDesc.ByteWidth >= requiredSize*/) {
                //context->UpdateSubresource(m_vertexBuffer.get(), 0, nullptr, primitiveBuilder.Vertices.data(), requiredSize, requiredSize);
            } else {
                m_vertexBuffer = CreateVertexBuffer(primitiveBuilder, true);
            }
        }

        // Update index buffer.
        {
            /*D3D11_BUFFER_DESC idxDesc;
            m_indexBuffer->GetDesc(&idxDesc);*/

            UINT requiredSize = (UINT)(primitiveBuilder.Indices.size() * sizeof(decltype(primitiveBuilder.Indices)::value_type));
            if (false /*idxDesc.ByteWidth >= requiredSize*/) {
                //context->UpdateSubresource(m_indexBuffer.get(), 0, nullptr, primitiveBuilder.Indices.data(), requiredSize, requiredSize);
            } else {
                m_indexBuffer = CreateIndexBuffer(primitiveBuilder, true);
            }

            m_indexCount = (UINT)primitiveBuilder.Indices.size();
        }
    }
    
    void Primitive::Render(const XrRect2Di& imageRect,
                           const float renderTargetClearColor[4],
                           const std::vector<xr::math::ViewProjection>& viewProjections,
                           DXGI_FORMAT colorSwapchainFormat,
                           void* colorTexture,
                           DXGI_FORMAT depthSwapchainFormat,
                           void* depthTexture) const {
        const uint32_t viewInstanceCount = (uint32_t)viewProjections.size();
        //for some reason the line below doesnt work
       /* xr::detail::CHECK_MSG(viewInstanceCount <= MaxViewInstance,
                  "Sample shader supports 2 or fewer view instances. Adjust shader to accommodate more.");*/

        const bool reversedZ = viewProjections[0].NearFar.Near > viewProjections[0].NearFar.Far;
        const float depthClearValue = reversedZ ? 0.f : 1.f;
        const uint64_t state =
            BGFX_STATE_WRITE_MASK | (reversedZ ? BGFX_STATE_DEPTH_TEST_GREATER : BGFX_STATE_DEPTH_TEST_LESS) | BGFX_STATE_CULL_CCW;





        uint32_t clearColorRgba;
        uint8_t* dst = reinterpret_cast<uint8_t*>(&clearColorRgba);
        dst[3] = uint8_t(bx::toUnorm(renderTargetClearColor[0], 255.0f));
        dst[2] = uint8_t(bx::toUnorm(renderTargetClearColor[1], 255.0f));
        dst[1] = uint8_t(bx::toUnorm(renderTargetClearColor[2], 255.0f));
        dst[0] = uint8_t(bx::toUnorm(renderTargetClearColor[3], 255.0f));


        // Render each view
        //for (uint32_t k = 0; k < viewInstanceCount; k++) {
            //const DirectX::XMMATRIX spaceToView = xr::math::LoadInvertedXrPose(viewProjections[k].Pose);
            //const DirectX::XMMATRIX projectionMatrix = ComposeProjectionMatrix(viewProjections[k].Fov, viewProjections[k].NearFar);

            DirectX::XMFLOAT4X4 view;
            DirectX::XMFLOAT4X4 proj;
            //DirectX::XMStoreFloat4x4(&view, spaceToView);
            //DirectX::XMStoreFloat4x4(&proj, projectionMatrix);

            const bgfx::ViewId viewId = bgfx::ViewId(m_indexCount);
           // bgfx::setViewFrameBuffer(viewId, frameBuffers[k].Get());
            bgfx::setViewClear(viewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, clearColorRgba, depthClearValue, 0);
            bgfx::setViewRect(viewId,
                              (uint16_t)imageRect.offset.x,
                              (uint16_t)imageRect.offset.y,
                              (uint16_t)imageRect.extent.width,
                              (uint16_t)imageRect.extent.height);
            bgfx::setViewTransform(viewId, view.m, proj.m);
            bgfx::touch(viewId);

            //Render each cube part?
            bgfx::setVertexBuffer(0, *m_vertexBuffer);
            bgfx::setIndexBuffer(*m_indexBuffer);
            bgfx::setState(state);
            // Draw the cube?
            bgfx::submit(viewId, *m_shaderProgram);
        //}
        //context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
        //context->IASetIndexBuffer(m_indexBuffer.get(), DXGI_FORMAT_R32_UINT, 0);
        //context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        //context->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);
    }
} // namespace Pbr
