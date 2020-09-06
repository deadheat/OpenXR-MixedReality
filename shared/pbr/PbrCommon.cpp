////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation.  All Rights Reserved
// Licensed under the MIT License. See License.txt in the project root for license information.
#include "pch.h"
#include <sstream>
// Implementation is in the Gltf library so this isn't needed: #define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "PbrCommon.h"

#include <bx/platform.h>
#include <bx/math.h>
#include <bx/pixelformat.h>

#include <bgfx/platform.h>
#include <bgfx/embedded_shader.h>
#include "SampleShared/bgfx_utils.h"
#include "SampleShared/BgfxUtility.h"

using namespace DirectX;

#define TRIANGLE_VERTEX_COUNT 3 // #define so it can be used in lambdas without capture

namespace Pbr {
    
    namespace Internal {
        void ThrowIfFailed(HRESULT hr) {
            if (FAILED(hr)) {
                std::stringstream ss;
                ss << std::hex << "Error in PBR renderer: 0x" << hr;
                throw std::exception(ss.str().c_str());
            }
        }
    } // namespace Internal

    /*const D3D11_INPUT_ELEMENT_DESC Vertex::s_vertexDesc[6] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TRANSFORMINDEX", 0, DXGI_FORMAT_R16_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };*/

    RGBAColor XM_CALLCONV FromSRGB(DirectX::XMVECTOR color) {
        RGBAColor linearColor{};
        DirectX::XMStoreFloat4(&linearColor, DirectX::XMColorSRGBToRGB(color));
        return linearColor;
    }

    RGBColor XM_CALLCONV RGBFromSRGB(DirectX::XMVECTOR color) {
        RGBColor linearColor{};
        DirectX::XMStoreFloat3(&linearColor, DirectX::XMColorSRGBToRGB(color));
        return linearColor;
    }

    PrimitiveBuilder& PrimitiveBuilder::AddAxis(float axisLength, float axisThickness, Pbr::NodeIndex_t transformIndex) {
        AddCube(axisThickness + 0.01f, transformIndex, Pbr::FromSRGB(DirectX::Colors::Gray));
        AddCube({axisLength, axisThickness, axisThickness},
                DirectX::XMVECTORF32{axisLength / 2, 0, 0},
                transformIndex,
                Pbr::FromSRGB(DirectX::Colors::Red));
        AddCube({axisThickness, axisLength, axisThickness},
                DirectX::XMVECTORF32{0, axisLength / 2, 0},
                transformIndex,
                Pbr::FromSRGB(DirectX::Colors::Green));
        AddCube({axisThickness, axisThickness, axisLength},
                DirectX::XMVECTORF32{0, 0, axisLength / 2},
                transformIndex,
                Pbr::FromSRGB(DirectX::Colors::Blue));

        return *this;
    }
    bgfx::VertexLayout Vertex::ms_layout;
    // Based on code from DirectXTK
    PrimitiveBuilder&
    PrimitiveBuilder::AddSphere(float diameter, uint32_t tessellation, Pbr::NodeIndex_t transformIndex, RGBAColor vertexColor) {
        if (tessellation < 3) {
            throw std::out_of_range("tesselation parameter out of range");
        }

        const uint32_t verticalSegments = tessellation;
        const uint32_t horizontalSegments = tessellation * 2;

        const float radius = diameter / 2;

        const uint32_t startVertexIndex = (uint32_t)Vertices.size();

        // Create rings of vertices at progressively higher latitudes.
        for (uint32_t i = 0; i <= verticalSegments; i++) {
            const float v = 1 - (float)i / verticalSegments;

            const float latitude = (i * DirectX::XM_PI / verticalSegments) - DirectX::XM_PIDIV2;
            float dy, dxz;
            DirectX::XMScalarSinCos(&dy, &dxz, latitude);

            // Create a single ring of vertices at this latitude.
            for (uint32_t j = 0; j <= horizontalSegments; j++) {
                const float longitude = j * DirectX::XM_2PI / horizontalSegments;
                float dx, dz;
                DirectX::XMScalarSinCos(&dx, &dz, longitude);
                dx *= dxz;
                dz *= dxz;

                // Compute tangent at 90 degrees along longitude.
                float tdx, tdz;
                DirectX::XMScalarSinCos(&tdx, &tdz, longitude + DirectX::XM_PI);
                tdx *= dxz;
                tdz *= dxz;

                const DirectX::XMVECTOR normal = DirectX::XMVectorSet(dx, dy, dz, 0);
                const DirectX::XMVECTOR tangent = DirectX::XMVectorSet(tdx, 0, tdz, 0);

                const float u = (float)j / horizontalSegments;
                const DirectX::XMVECTOR textureCoordinate = DirectX::XMVectorSet(u, v, 0, 0);

                Pbr::Vertex vert;
                //XMStoreFloat4(&vert.Position, normal * radius);
                //XMStoreFloat3(&vert.Normal, normal);
                //XMStoreFloat4(&vert.Tangent, tangent);
                //XMStoreFloat2(&vert.TexCoord0, textureCoordinate);
                //vert.Color0 = vertexColor;

                memcpy(vert.Position, &(normal * radius), sizeof(vert.Position));
                memcpy(vert.Normal, &(normal), sizeof(vert.Normal));
                memcpy(vert.Tangent, &(tangent), sizeof(vert.Tangent));
                memcpy(vert.TexCoord0, &(textureCoordinate), sizeof(vert.TexCoord0));
                memcpy(vert.Color0, &(vertexColor), sizeof(vert.Color0));


                //vert.ModelTransformIndex = transformIndex;
                Vertices.push_back(vert);
            }
        }

        // Fill the index buffer with triangles joining each pair of latitude rings.
        const uint32_t stride = horizontalSegments + 1;
        const uint32_t startIndicesIndex = (uint32_t)Indices.size();
        for (uint32_t i = 0; i < verticalSegments; i++) {
            for (uint32_t j = 0; j <= horizontalSegments; j++) {
                uint32_t nextI = i + 1;
                uint32_t nextJ = (j + 1) % stride;

                Indices.push_back(startVertexIndex + (i * stride + j));
                Indices.push_back(startVertexIndex + (nextI * stride + j));
                Indices.push_back(startVertexIndex + (i * stride + nextJ));

                Indices.push_back(startVertexIndex + (i * stride + nextJ));
                Indices.push_back(startVertexIndex + (nextI * stride + j));
                Indices.push_back(startVertexIndex + (nextI * stride + nextJ));
            }
        }

        return *this;
    }

    // Based on code from DirectXTK
    PrimitiveBuilder& PrimitiveBuilder::AddCube(DirectX::XMFLOAT3 sideLengths,
                                                DirectX::CXMVECTOR translation,
                                                Pbr::NodeIndex_t transformIndex,
                                                RGBAColor vertexColor) {
        // A box has six faces, each one pointing in a different direction.
        const int FaceCount = 6;

        static const DirectX::XMVECTORF32 faceNormals[FaceCount] = {
            {{{0, 0, 1, 0}}},
            {{{0, 0, -1, 0}}},
            {{{1, 0, 0, 0}}},
            {{{-1, 0, 0, 0}}},
            {{{0, 1, 0, 0}}},
            {{{0, -1, 0, 0}}},
        };

        static const DirectX::XMVECTORF32 textureCoordinates[4] = {
            {{{1, 0, 0, 0}}},
            {{{1, 1, 0, 0}}},
            {{{0, 1, 0, 0}}},
            {{{0, 0, 0, 0}}},
        };

        // Create each face in turn.
        const DirectX::XMVECTORF32 sideLengthHalfVector = {{{sideLengths.x / 2, sideLengths.y / 2, sideLengths.z / 2}}};

        for (int i = 0; i < FaceCount; i++) {
            DirectX::XMVECTOR normal = faceNormals[i];

            // Get two vectors perpendicular both to the face normal and to each other.
            DirectX::XMVECTOR basis = (i >= 4) ? DirectX::g_XMIdentityR2 : DirectX::g_XMIdentityR1;

            DirectX::XMVECTOR side1 = DirectX::XMVector3Cross(normal, basis);
            DirectX::XMVECTOR side2 = DirectX::XMVector3Cross(normal, side1);

            // Six indices (two triangles) per face.
            size_t vbase = Vertices.size();
            Indices.push_back((uint32_t)vbase + 0);
            Indices.push_back((uint32_t)vbase + 1);
            Indices.push_back((uint32_t)vbase + 2);

            Indices.push_back((uint32_t)vbase + 0);
            Indices.push_back((uint32_t)vbase + 2);
            Indices.push_back((uint32_t)vbase + 3);
            // using namespace DirectX;
            const DirectX::XMVECTOR positions[4] = {{(normal - side1 - side2) * sideLengthHalfVector},
                                                    {(normal - side1 + side2) * sideLengthHalfVector},
                                                    {(normal + side1 + side2) * sideLengthHalfVector},
                                                    {(normal + side1 - side2) * sideLengthHalfVector}};

            for (int j = 0; j < 4; j++) {
                Pbr::Vertex vert;
                //XMStoreFloat4(&vert.Position, positions[j] + translation);
                //XMStoreFloat3(&vert.Normal, normal);
                //XMStoreFloat4(&vert.Tangent, side1); // TODO arbitrarily picked side 1
                //XMStoreFloat2(&vert.TexCoord0, textureCoordinates[j]);
                //vert.Color0 = vertexColor;
                //vert.ModelTransformIndex = transformIndex;
                memcpy(vert.Position, &(positions[j] + translation), sizeof(vert.Position));
                memcpy(vert.Normal, &(normal), sizeof(vert.Normal));
                memcpy(vert.Tangent, &(side1), sizeof(vert.Tangent)); // TODO arbitrarily picked side 1
                memcpy(vert.TexCoord0, &(textureCoordinates[j]), sizeof(vert.TexCoord0));
                memcpy(vert.Color0, &(vertexColor), sizeof(vert.Color0));
                Vertices.push_back(vert);
            }
        }

        return *this;
    }

    PrimitiveBuilder& PrimitiveBuilder::AddCube(DirectX::XMFLOAT3 sideLengths, Pbr::NodeIndex_t transformIndex, RGBAColor vertexColor) {
        return AddCube(sideLengths, DirectX::g_XMZero, transformIndex, vertexColor);
    }

    PrimitiveBuilder& PrimitiveBuilder::AddCube(float sideLength, Pbr::NodeIndex_t transformIndex, RGBAColor vertexColor) {
        return AddCube(DirectX::XMFLOAT3{sideLength, sideLength, sideLength}, transformIndex, vertexColor);
    }

    PrimitiveBuilder& PrimitiveBuilder::AddQuad(DirectX::XMFLOAT2 sideLengths,
                                                DirectX::XMFLOAT2 textureCoord,
                                                Pbr::NodeIndex_t transformIndex,
                                                RGBAColor vertexColor) {
        const DirectX::XMFLOAT2 halfSideLength = {sideLengths.x / 2, sideLengths.y / 2};
        const DirectX::XMFLOAT4 vertices[4] = {{-halfSideLength.x, -halfSideLength.y, 0,0}, // LB
                                               {-halfSideLength.x, halfSideLength.y, 0,0},  // LT
                                               {halfSideLength.x, halfSideLength.y, 0,0},   // RT
                                               {halfSideLength.x, -halfSideLength.y, 0,0}}; // RB
        const DirectX::XMFLOAT2 uvs[4] = {
            {0, textureCoord.y},
            {0, 0},
            {textureCoord.x, 0},
            {textureCoord.x, textureCoord.y},
        };

        // Two triangles.
        auto vbase = static_cast<uint32_t>(Vertices.size());
        Indices.push_back(vbase + 0);
        Indices.push_back(vbase + 1);
        Indices.push_back(vbase + 2);
        Indices.push_back(vbase + 0);
        Indices.push_back(vbase + 2);
        Indices.push_back(vbase + 3);

        Pbr::Vertex vert;
       


        float _normal[3] = {0, 0, 1};
        float _tangent[4] = {1, 0, 0, 0};
        //vert.Color0 = vertexColor;
        memcpy(vert.Normal, &(_normal), sizeof(vert.Normal));
        memcpy(vert.Tangent, &(_tangent), sizeof(vert.Tangent)); // TODO arbitrarily picked side 1
        memcpy(vert.Color0, &(vertexColor), sizeof(vert.Color0));
        //vert.ModelTransformIndex = transformIndex;
        for (size_t j = 0; j < _countof(vertices); j++) {
            //vert.Position = vertices[j];
            //vert.TexCoord0 = uvs[j];
            memcpy(vert.Position, &(vertices[j]), sizeof(vert.Position));
            memcpy(vert.TexCoord0, &(uvs[j]), sizeof(vert.TexCoord0));
            Vertices.push_back(vert);
        }
        return *this;
    }

    namespace Texture {
        std::array<uint8_t, 4> LoadRGBAUI4(RGBAColor color) {
            DirectX::XMFLOAT4 colorf;
            DirectX::XMStoreFloat4(&colorf, DirectX::XMVectorScale(XMLoadFloat4(&color), 255));
            return std::array<uint8_t, 4>{(uint8_t)colorf.x, (uint8_t)colorf.y, (uint8_t)colorf.z, (uint8_t)colorf.w};
        }

        bgfx::TextureHandle LoadTextureImage(_In_reads_bytes_(fileSize) const uint8_t* fileData, uint32_t fileSize) {
            auto freeImageData = [](unsigned char* ptr) { ::free(ptr); };
            using stbi_unique_ptr = std::unique_ptr<unsigned char, decltype(freeImageData)>;

            constexpr uint32_t DesiredComponentCount = 4;

            int w, h, c;
            // If c == 3, a component will be padded with 1.0f
            //bimg::ImageContainer* dmap = imageLoad(m_dmap.pathToFile.getCPtr(), bgfx::TextureFormat::R16);
            //auto file = std::make_pair(fileData, fileSize);


            stbi_unique_ptr rgbadata(stbi_load_from_memory(fileData, fileSize, &w, &h, &c, DesiredComponentCount), freeImageData);
            if (!rgbadata) {
                throw std::exception("failed to load image file data.");
            }

           /* bgfx::TextureInfo ti;
            bimg::imageGetSize((bimg::TextureInfo*)&ti,
                               w,
                               h,
                               1,
                               false,
                               true,
                               1,
                               bimg::TextureFormat::Enum(sample::bg::DxgiFormatToBgfxFormat(DXGI_FORMAT_R8G8B8A8_UNORM)));*/

            return CreateTexture(rgbadata.get(),
                                 w * h * DesiredComponentCount,
                                 w,
                                 h,
                                 sample::bg::DxgiFormatToBgfxFormat(DXGI_FORMAT_R8G8B8A8_UNORM));
        }

        bgfx::TextureHandle CreateFlatCubeTexture(RGBAColor color, bgfx::TextureFormat::Enum format) {
            // Each side is a 1x1 pixel (RGBA) image.
            const std::array<uint8_t, 4> rgbaColor = LoadRGBAUI4(color);
            return bgfx::createTextureCube(1 /*_size*/,
                                           false /*bool _hasMips*/,
                                           6 /*_numLayers*/,
                                           format,
                                           /*uint64_t _flags = */ BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE | BGFX_CAPS_TEXTURE_CUBE_ARRAY,
                                           bgfx::makeRef(&rgbaColor, sizeof(rgbaColor) /*constMemory* _mem = NULL*/));
        }

        bgfx::TextureHandle
        CreateTexture(_In_reads_bytes_(size) const uint8_t* rgba, uint32_t size, int width, int height, bgfx::TextureFormat::Enum format) {
            
            return bgfx::createTexture2D(width,
                                         height,
                                         false /*_hasMips*/,
                                         1 /*_numLayers*/,
                                         format /*TextureFormat::Enum_format*/,
                                         /*uint64_t _flags = */ BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE,
                                         bgfx::makeRef(rgba, size));
        }

        // from what I can tell this is handled internally in bgfx, but I could be wrong
        bgfx::UniformHandle CreateSampler(const char* _uniqueName) {
            return bgfx::createUniform(_uniqueName, bgfx::UniformType::Sampler);
        }
    } // namespace Texture
} // namespace Pbr
