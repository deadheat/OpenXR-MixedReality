//*********************************************************
//    Copyright (c) Microsoft. All rights reserved.
//
//    Apache 2.0 License
//
//    You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
//
//*********************************************************
#pragma once
#include "pch.h"
#include "FileUtility.h"
//#include "DirectXTK/DDSTextureLoader.h"
#include <pbr/GltfLoader.h>
#include <pbr/PbrModel.h>
#include <fstream>
#include <XrUtility/XrString.h>
#include "Trace.h"
#include <SampleShared/BgfxUtility.h>

#define FMT_HEADER_ONLY
#include <fmt/format.h>

using namespace DirectX;

namespace sample {
    std::vector<uint8_t> ReadFileBytes(const std::filesystem::path& path) {
        bool fileExists = false;
        try {
            std::ifstream file;
            file.exceptions(std::ios::failbit | std::ios::badbit);
            file.open(path, std::ios::binary | std::ios::ate);
            fileExists = true;
            // If tellg fails then it will throw an exception instead of returning -1.
            std::vector<uint8_t> data(static_cast<size_t>(file.tellg()));
            file.seekg(0, std::ios::beg);
            file.read(reinterpret_cast<char*>(data.data()), data.size());
            return data;
        } catch (const std::ios::failure&) {
            // The exception only knows that the failbit was set so it doesn't contain anything useful.
            throw std::runtime_error(fmt::format("Failed to {} file: {}", fileExists ? "read" : "open", path.string()));
        }
    }

    std::filesystem::path GetAppFolder() {
        HMODULE thisModule;
#ifdef UWP
        thisModule = nullptr;
#else
        if (!::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCWSTR>(GetAppFolder), &thisModule)) {
            throw std::runtime_error("Unable to get the module handle.");
        }
#endif

        wchar_t moduleFilename[MAX_PATH];
        ::GetModuleFileName(thisModule, moduleFilename, (DWORD)std::size(moduleFilename));
        std::filesystem::path fullPath(moduleFilename);
        return fullPath.remove_filename();
    }

    std::filesystem::path GetPathInAppFolder(const std::filesystem::path& filename) {
        return GetAppFolder() / filename;
    }

    std::filesystem::path FindFileInAppFolder(const std::filesystem::path& filename,
                                              const std::vector<std::filesystem::path>& searchFolders) {
        auto appFolder = GetAppFolder();
        for (auto folder : searchFolders) {
            auto path = appFolder / folder / filename;
            if (std::filesystem::exists(path)) {
                return path;
            }
        }

        sample::Trace(fmt::format("File \"{}\" is not found in app folder \"{}\" and search folders{}",
                                    xr::wide_to_utf8(filename.c_str()),
                                    xr::wide_to_utf8(appFolder.c_str()),
                                    [&searchFolders]() -> std::string {
                                        fmt::memory_buffer buffer;
                                        for (auto& folder : searchFolders) {
                                            fmt::format_to(buffer, " \"{}\"", xr::wide_to_utf8(folder.c_str()));
                                        }
                                        return buffer.data();
                                    }())
                            .c_str());

        assert(false && "The file should be embeded in app folder in debug build.");
        return "";
    }

    Pbr::Resources InitializePbrResources( bool environmentIBL) {
        Pbr::Resources pbrResources = Pbr::Resources();

        // Set up a light source (an image-based lighting environment map will also be loaded and contribute to the scene lighting).
        pbrResources.SetLight({0.0f, 0.7071067811865475f, 0.7071067811865475f}, Pbr::RGB::White);

        // Read the BRDF Lookup Table used by the PBR system into a DirectX texture.
        std::vector<byte> brdfLutFileData = ReadFileBytes(FindFileInAppFolder(L"brdf_lut.png", {"", L"Pbr_uwp"}));
        unique_bgfx_handle<bgfx::TextureHandle> brdLutResourceView;
        brdLutResourceView.reset(Pbr::Texture::LoadTextureImage(brdfLutFileData.data(), (uint32_t)brdfLutFileData.size()));
        pbrResources.SetBrdfLut(std::move(brdLutResourceView));
        unique_bgfx_handle<bgfx::TextureHandle> diffuseTextureView;
        unique_bgfx_handle<bgfx::TextureHandle> specularTextureView;
        std::map<std::string, bgfx::TextureInfo> textureInformation;
        if (environmentIBL) {
            
            diffuseTextureView.reset(loadTexture(FindFileInAppFolder(L"Sample_DiffuseHDR.DDS", {"", "SampleShared_uwp"}).c_str(),
                                                 NULL,
                                                 NULL,
                                                 &textureInformation["diffuseTextureView"]));
            specularTextureView.reset(loadTexture(FindFileInAppFolder(L"Sample_SpecularHDR.DDS", {"", "SampleShared_uwp"}).c_str(),

                                                       NULL,
                                                       NULL,
                                                       &textureInformation["specularTextureView"]));
           /* CHECK_HRCMD(DirectX::CreateDDSTextureFromFile(device,
                                                          FindFileInAppFolder(L"Sample_DiffuseHDR.DDS", {"", "SampleShared_uwp"}).c_str(),
                                                          nullptr,
                                                          diffuseTextureView.put()));
            CHECK_HRCMD(DirectX::CreateDDSTextureFromFile(device,
                                                          FindFileInAppFolder(L"Sample_SpecularHDR.DDS", {"", "SampleShared_uwp"}).c_str(),
                                                          nullptr,
                                                          specularTextureView.put()));*/
        } else {
            diffuseTextureView.reset(Pbr::Texture::CreateFlatCubeTexture(Pbr::RGBA::White));
            specularTextureView.reset(Pbr::Texture::CreateFlatCubeTexture(Pbr::RGBA::White));
        }

        pbrResources.SetEnvironmentMap(std::move(specularTextureView), std::move(diffuseTextureView), textureInformation);

        return pbrResources;
    }

} // namespace sample
