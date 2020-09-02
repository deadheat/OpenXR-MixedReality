////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation.  All Rights Reserved
// Licensed under the MIT License. See License.txt in the project root for license information.
#pragma once

#include <optional>
#include <vector>
#include <memory>
#include <winrt/base.h>
#include <d3d11.h>
#include <d3d11_2.h>
#include <DirectXMath.h>
#include "PbrCommon.h"
#include "PbrResources.h"
#include "PbrPrimitive.h"

namespace Pbr {
    // Node for creating a hierarchy of transforms. These transforms are referenced by vertices in the model's primitives.
    struct Node {
        using Collection = std::vector<Node>;

        Node(DirectX::CXMMATRIX localTransform, std::string name, NodeIndex_t index, NodeIndex_t parentNodeIndex)
            : Name(std::move(name))
            , Index(index)
            , ParentNodeIndex(parentNodeIndex) {
            SetTransform(localTransform);
        }

        // Set the local transform for this node.
        void XM_CALLCONV SetTransform(DirectX::FXMMATRIX transform) {
            DirectX::XMStoreFloat4x4(&m_localTransform, transform);
            InterlockedIncrement(&m_modifyCount);
        }

        // Get the local transform for this node.
        DirectX::XMMATRIX XM_CALLCONV GetTransform() const {
            return DirectX::XMLoadFloat4x4(&m_localTransform);
        }

        const std::string Name;
        const NodeIndex_t Index;
        const NodeIndex_t ParentNodeIndex;

    private:
        friend struct Model;
        uint32_t m_modifyCount{0};
        DirectX::XMFLOAT4X4 m_localTransform;
    };
    struct CachedFrameBuffer {
        std::vector<unique_bgfx_handle<bgfx::FrameBufferHandle>> FrameBuffers;
    };


    // A model is a collection of primitives (which reference a material) and transforms referenced by the primitives' vertices.
    struct Model final {
        std::string Name;

    public:
        Model(bool createRootNode = true);

        // Add a node to the model.
        NodeIndex_t XM_CALLCONV AddNode(DirectX::FXMMATRIX transform, NodeIndex_t parentIndex, std::string name = "");

        // Add a primitive to the model.
        void AddPrimitive(Primitive primitive);

        // Render the model.
        void Render(Pbr::Resources const& pbrResources) const;

        // Remove all primitives.
        void Clear();

        // Create a clone of this model.
        std::shared_ptr<Model> Clone(Pbr::Resources const& pbrResources) const;

        NodeIndex_t GetNodeCount() const {
            return (NodeIndex_t)m_nodes.size();
        }
        Node& GetNode(NodeIndex_t nodeIndex) {
            return m_nodes[nodeIndex];
        }
        const Node& GetNode(NodeIndex_t nodeIndex) const {
            return m_nodes[nodeIndex];
        }

        uint32_t GetPrimitiveCount() const {
            return (uint32_t)m_primitives.size();
        }
        Primitive& GetPrimitive(uint32_t index) {
            return m_primitives[index];
        }
        const Primitive& GetPrimitive(uint32_t index) const {
            return m_primitives[index];
        }

        // Find the first node which matches a given name.
        std::optional<NodeIndex_t> FindFirstNode(std::string_view name, std::optional<NodeIndex_t> const& parentNodeIndex = {}) const;

    private:
        // Compute the transform relative to the root of the model for a given node.
        DirectX::XMMATRIX GetNodeToModelRootTransform(NodeIndex_t nodeIndex) const;

        // Updated the transforms used to render the model. This needs to be called any time a node transform is changed.
        void UpdateTransforms(Pbr::Resources const& pbrResources) const;

    private:
        // A model is made up of one or more Primitives. Each Primitive has a unique material.
        // Ideally primitives with the same material should be merged to reduce draw calls.
        Primitive::Collection m_primitives;

        // A model contains one or more nodes. Each vertex of a primitive references a node to have the
        // node's transform applied.
        Node::Collection m_nodes;

        // Temporary buffer holds the world transforms, computed from the node's local transforms.
        mutable std::vector<DirectX::XMFLOAT4X4> m_modelTransforms;
        mutable bgfx::InstanceDataBuffer m_modelTransformsStructuredBuffer;
        bool m_modelSet = false;
        mutable unique_bgfx_handle<bgfx::TextureHandle> m_modelTransformsResourceView;
        //std::map<std::tuple<void*, void*>, CachedFrameBuffer> m_cachedFrameBuffers;
        mutable uint32_t TotalModifyCount{0};
    };
} // namespace Pbr
