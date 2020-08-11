////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation.  All Rights Reserved
// Licensed under the MIT License. See License.txt in the project root for license information.
#pragma once

#include <vector>
#include <winrt/base.h>
#include <d3d11.h>
#include <d3d11_2.h>
#include "PbrMaterial.h"
#include <bgfx/bgfx.h>

#include <bgfx/platform.h>

#include <bx/uint32_t.h>

namespace Pbr {
    // A primitive holds a vertex buffer, index buffer, and a pointer to a PBR material.
    struct Primitive final {
        using Collection = std::vector<Primitive>;

        Primitive() = delete;
        Primitive(UINT indexCount,
                  winrt::com_ptr<bgfx::IndexBufferHandle> indexBuffer,
                  winrt::com_ptr<bgfx::VertexBufferHandle> vertexBuffer,
                  std::shared_ptr<Material> material);
        Primitive(Pbr::Resources const& pbrResources,
                  const Pbr::PrimitiveBuilder& primitiveBuilder,
                  std::shared_ptr<Material> material,
                  bool updatableBuffers = false);

        void UpdateBuffers(const Pbr::PrimitiveBuilder& primitiveBuilder);

        // Get the material for the primitive.
        std::shared_ptr<Material>& GetMaterial() {
            return m_material;
        }
        const std::shared_ptr<Material>& GetMaterial() const {
            return m_material;
        }

    protected:
        friend struct Model;
        void Render(const XrRect2Di& imageRect,
                               const float renderTargetClearColor[4],
                               const std::vector<xr::math::ViewProjection>& viewProjections,
                               DXGI_FORMAT colorSwapchainFormat,
                               void* colorTexture,
                               DXGI_FORMAT depthSwapchainFormat,
                               void* depthTexture) const;
        Primitive Clone(Pbr::Resources const& pbrResources) const;

    private:
        UINT m_indexCount;
        winrt::com_ptr<bgfx::IndexBufferHandle> m_indexBuffer;
        winrt::com_ptr<bgfx::VertexBufferHandle> m_vertexBuffer;
        winrt::com_ptr<bgfx::ProgramHandle> m_shaderProgram;
        std::shared_ptr<Material> m_material;
    };
} // namespace Pbr
