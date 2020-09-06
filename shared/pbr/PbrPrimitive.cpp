////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation.  All Rights Reserved
// Licensed under the MIT License. See License.txt in the project root for license information.
#pragma once
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

    bgfx::VertexBufferHandle CreateVertexBuffer(const Pbr::PrimitiveBuilder& primitiveBuilder, bool updatableBuffers) {
        Pbr::Vertex::init();
        // Create Vertex Buffer BGFX
        //bgfx::VertexLayout vertexLayout;
        //vertexLayout.begin()
        //    .add(bgfx::Attrib::Position, 4, bgfx::AttribType::Float)
        //    .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
        //    .add(bgfx::Attrib::Tangent, 4, bgfx::AttribType::Float)
        //    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float)
        //    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        //    .end();
        //// Create Vertex Buffer
        // D3D11_BUFFER_DESC desc{};
        // desc.Usage = D3D11_USAGE_DEFAULT;
        // desc.ByteWidth = GetPbrVertexByteSize(primitiveBuilder.Vertices.size());
        // desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        // if (updatableBuffers) {
        //    desc.Usage = D3D11_USAGE_DYNAMIC;
        //    desc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
        //}

        // D3D11_SUBRESOURCE_DATA initData{};
        // initData.pSysMem = primitiveBuilder.Vertices.data();

        // unique_bgfx_handle<bgfx::VertexBufferHandle> vertexBuffer;
        // bgfx::VertexBufferHandle rawVertexBuffer =
        //
        // vertexBuffer.copy_from(&rawVertexBuffer);
        size_t numVertex = primitiveBuilder.Vertices.size();
        size_t sizeOfVertex = sizeof(primitiveBuilder.Vertices[0]);
        return bgfx::createVertexBuffer(
            bgfx::makeRef(&primitiveBuilder.Vertices[0], (uint32_t)(sizeOfVertex*numVertex)),
                                        Pbr::Vertex::ms_layout);

    }

    bgfx::IndexBufferHandle CreateIndexBuffer(const Pbr::PrimitiveBuilder& primitiveBuilder

                                              /*,bool updatableBuffers*/) {
        // Create bgfx Index Buffer
        // Create Index Buffer
        // D3D11_BUFFER_DESC desc{};
        // desc.Usage = D3D11_USAGE_DEFAULT;
        // desc.ByteWidth = (UINT)(sizeof(decltype(primitiveBuilder.Indices)::value_type) * primitiveBuilder.Indices.size());
        // desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        // if (updatableBuffers) {
        //    desc.Usage = D3D11_USAGE_DYNAMIC;
        //    desc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
        //}

        /* D3D11_SUBRESOURCE_DATA initData{};
         initData.pSysMem = primitiveBuilder.Indices.data();*/

        return bgfx::createIndexBuffer(bgfx::makeRef(primitiveBuilder.Indices.data(), sizeof(primitiveBuilder.Indices.data())));
    }
} // namespace

namespace Pbr {
    const bgfx::RendererType::Enum type = bgfx::getRendererType();

    Primitive::Primitive(UINT indexCount,
                         shared_bgfx_handle<bgfx::IndexBufferHandle> indexBuffer,
                         shared_bgfx_handle<bgfx::VertexBufferHandle> vertexBuffer,
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
                    shared_bgfx_handle<bgfx::IndexBufferHandle>(CreateIndexBuffer(primitiveBuilder /*, updatableBuffers*/)),
                    shared_bgfx_handle<bgfx::VertexBufferHandle>(CreateVertexBuffer(primitiveBuilder, updatableBuffers)),
                    std::move(material)) {
        
    }

    Primitive Primitive::Clone(Pbr::Resources const& pbrResources) const {
        return Primitive(m_indexCount, m_indexBuffer, m_vertexBuffer, m_material->Clone(pbrResources));
    }

    void Primitive::UpdateBuffers(const Pbr::PrimitiveBuilder& primitiveBuilder) {
        // TODO figure out how to implement updatable logic
        // Update vertex buffer.
        {
            /*D3D11_BUFFER_DESC vertDesc;
            m_vertexBuffer->GetDesc(&vertDesc);*/

            UINT requiredSize = GetPbrVertexByteSize(primitiveBuilder.Vertices.size());
            if (false /*vertDesc.ByteWidth >= requiredSize*/) {
                // context->UpdateSubresource(m_vertexBuffer.get(), 0, nullptr, primitiveBuilder.Vertices.data(), requiredSize,
                // requiredSize);
            } else {
                m_vertexBuffer.reset(CreateVertexBuffer(primitiveBuilder, true));
            }
        }

        // Update index buffer.
        {
            /*D3D11_BUFFER_DESC idxDesc;
            m_indexBuffer->GetDesc(&idxDesc);*/

            UINT requiredSize = (UINT)(primitiveBuilder.Indices.size() * sizeof(decltype(primitiveBuilder.Indices)::value_type));
            if (false /*idxDesc.ByteWidth >= requiredSize*/) {
                // context->UpdateSubresource(m_indexBuffer.get(), 0, nullptr, primitiveBuilder.Indices.data(), requiredSize, requiredSize);
            } else {
                m_indexBuffer.reset(CreateIndexBuffer(primitiveBuilder));
            }

            m_indexCount = (UINT)primitiveBuilder.Indices.size();
        }
    }

    void Primitive::Render(const Resources& pbrResources) const {
        // const UINT stride = sizeof(Pbr::Vertex);
        // const UINT offset = 0;
        // bgfx::VertexBufferHandle* const vertexBuffers[] = {&m_vertexBuffer.get()};
        bgfx::setVertexBuffer(0, m_vertexBuffer.get());
        bgfx::setIndexBuffer(m_indexBuffer.get());
        
        /*context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
        context->IASetIndexBuffer(m_indexBuffer.get(), DXGI_FORMAT_R32_UINT, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);*/
    }
} // namespace Pbr
