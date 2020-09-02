////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation.  All Rights Reserved
// Licensed under the MIT License. See License.txt in the project root for license information.
#pragma once

#include "pch.h"
#include "PbrCommon.h"
#include "PbrModel.h"
#include "SampleShared/BgfxUtility.h"

#include <bx/platform.h>
#include <bx/math.h>
#include <bx/pixelformat.h>

#include <bgfx/platform.h>
#include <bgfx/embedded_shader.h>

using namespace DirectX;

#define TRIANGLE_VERTEX_COUNT 3 // #define so it can be used in lambdas without capture

namespace
{
    constexpr Pbr::NodeIndex_t RootParentNodeIndex = -1;
}

namespace Pbr
{
    Model::Model(bool createRootNode /*= true*/)
    {
        if (createRootNode)
        {
            AddNode(XMMatrixIdentity(), RootParentNodeIndex, "root");
        }
    }

    void Model::Render(Pbr::Resources const& pbrResources) const
    {
        UpdateTransforms(pbrResources);
        // bgfx doesnt use shader slots from the looks of it and the modelTransforms are done by some internal bgfx gymnastics
        //Pbr::ShaderSlots::Transforms
        //context->VSSetShaderResources(Pbr::ShaderSlots::Transforms, _countof(vsShaderResources), vsShaderResources);

        //const DirectX::XMMATRIX spaceToView = xr::math::LoadInvertedXrPose(viewProjections[k].Pose);
        //const DirectX::XMMATRIX projectionMatrix = ComposeProjectionMatrix(viewProjections[k].Fov, viewProjections[k].NearFar);


        for (const Pbr::Primitive& primitive : m_primitives)
        {
            if (primitive.GetMaterial()->Hidden) continue;
            primitive.GetMaterial()->SetWireframe(pbrResources.GetFillMode() == FillMode::Wireframe);
            primitive.GetMaterial()->Bind(pbrResources);
            primitive.Render();
        }
        bgfx::frame();
        // Expect the caller to reset other state, but the geometry shader is cleared specially.
        //context->GSSetShader(nullptr, nullptr, 0);
    }

    NodeIndex_t XM_CALLCONV Model::AddNode(FXMMATRIX transform, Pbr::NodeIndex_t parentIndex, std::string name)
    {
        auto newNodeIndex = (Pbr::NodeIndex_t)m_nodes.size();
        if (newNodeIndex != RootNodeIndex && parentIndex == RootParentNodeIndex)
        {
            throw new std::exception("Only the first node can be the root");
        }

        m_nodes.emplace_back(transform, std::move(name), newNodeIndex, parentIndex);
        m_modelSet = false; // Structured buffer will need to be recreated.
        return m_nodes.back().Index;
    }

    void Model::Clear()
    {
        m_primitives.clear();
    }

    std::shared_ptr<Model> Model::Clone(Pbr::Resources const& pbrResources) const
    {
        auto clone = std::make_shared<Model>(false /* createRootNode */);

        for (const Node& node : m_nodes)
        {
            clone->AddNode(node.GetTransform(), node.ParentNodeIndex, node.Name);
        }

        for (const Primitive& primitive : m_primitives)
        {
            clone->AddPrimitive(primitive.Clone(pbrResources));
        }

        return clone;
    }

    std::optional<NodeIndex_t> Model::FindFirstNode(std::string_view name, std::optional<NodeIndex_t> const& parentNodeIndex) const {
        // Children are guaranteed to come after their parents, so start looking after the parent index if one is provided.
        const NodeIndex_t startIndex = parentNodeIndex ? parentNodeIndex.value() + 1 : Pbr::RootNodeIndex;
        for (NodeIndex_t i = startIndex; i < m_nodes.size(); ++i) {
            const Pbr::Node& node = m_nodes[i];
            if ((!parentNodeIndex || node.ParentNodeIndex == parentNodeIndex.value()) && node.Name == name) {
                return node.Index;
            }
        }
        return {};
    }

    XMMATRIX Model::GetNodeToModelRootTransform(NodeIndex_t nodeIndex) const
    {
        const Pbr::Node& node = GetNode(nodeIndex);

        // Compute the transform recursively.
        const XMMATRIX parentTransform = node.ParentNodeIndex == Pbr::RootNodeIndex ? XMMatrixIdentity() : GetNodeToModelRootTransform(node.ParentNodeIndex);
        return XMMatrixMultiply(node.GetTransform(), parentTransform);
    }

    void Model::AddPrimitive(Pbr::Primitive primitive)
    {
        m_primitives.push_back(std::move(primitive));
    }

    void Model::UpdateTransforms(Pbr::Resources const& pbrResources) const {
        const uint32_t newTotalModifyCount =
            std::accumulate(m_nodes.begin(), m_nodes.end(), 0, [](uint32_t sumChangeCount, const Node& node) {
                return sumChangeCount + node.m_modifyCount;
            });

        // If none of the node transforms have changed, no need to recompute/update the model transform structured buffer.
        if (newTotalModifyCount != TotalModifyCount || !m_modelSet) {
            if (!m_modelSet) // The structured buffer is reset when a Node is added.
            {
                m_modelTransforms.resize(m_nodes.size());

                // Create/recreate the structured buffer and SRV which holds the node transforms.
                // Use Usage=D3D11_USAGE_DYNAMIC and CPUAccessFlags=D3D11_CPU_ACCESS_WRITE with Map/Unmap instead?
                /*D3D11_BUFFER_DESC desc{};
                desc.Usage = D3D11_USAGE_DEFAULT;
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
                desc.StructureByteStride = sizeof(decltype(m_modelTransforms)::value_type);
                desc.ByteWidth = (UINT)(m_modelTransforms.size() * desc.StructureByteStride);*/
                const uint32_t numInstances = 121;
                bgfx::allocInstanceDataBuffer(&m_modelTransformsStructuredBuffer, numInstances, sizeof(decltype(m_modelTransforms)::value_type));



               // Internal::ThrowIfFailed(pbrResources.GetDevice()->CreateBuffer(&desc, nullptr, m_modelTransformsStructuredBuffer.put()));


                /*D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
                bgfx::setInstanceDataBuffer(&m_modelTransformsStructuredBuffer);
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
                srvDesc.Buffer.NumElements = (UINT)m_modelTransforms.size();
                srvDesc.Buffer.ElementWidth = (UINT)m_modelTransforms.size();
                m_modelTransformsResourceView = nullptr;
                Internal::ThrowIfFailed(pbrResources.GetDevice()->CreateShaderResourceView(
                    m_modelTransformsStructuredBuffer.get(), &srvDesc, m_modelTransformsResourceView.put()));*/
            }
            //m_modelTransforms = (std::vector<DirectX::XMFLOAT4X4>)m_modelTransformsStructuredBuffer.data;
            // Nodes are guaranteed to come after their parents, so each node transform can be multiplied by its parent transform in a
            // single pass.
            assert(m_nodes.size() == m_modelTransforms.size());
            for (const auto& node : m_nodes) {
                assert(node.ParentNodeIndex == RootParentNodeIndex || node.ParentNodeIndex < node.Index);
                const XMMATRIX parentTransform = (node.ParentNodeIndex == RootParentNodeIndex)
                                                     ? XMMatrixIdentity()
                                                     : XMLoadFloat4x4(&m_modelTransforms[node.ParentNodeIndex]);
                
                XMStoreFloat4x4(&m_modelTransforms[node.Index], XMMatrixMultiply(parentTransform, XMMatrixTranspose(node.GetTransform())));
            }
            
            bgfx::InstanceDataBuffer x;
            // Seyi NOTE: This casting may not work
            x.data = (uint8_t*) m_modelTransforms.data();
            m_modelTransformsStructuredBuffer = x;
                //.data = (uint8_t*) m_modelTransforms.data();
            // Update node transform structured buffer.
            //context->UpdateSubresource(m_modelTransformsStructuredBuffer.get(), 0, nullptr, this->m_modelTransforms.data(), 0, 0);
            bgfx::setInstanceDataBuffer(&m_modelTransformsStructuredBuffer);
            TotalModifyCount = newTotalModifyCount;
        }
    }
}
