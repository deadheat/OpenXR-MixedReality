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

#include <winrt/base.h> // winrt::com_ptr
#include <d3dcommon.h>  //ID3DBlob
#include <XrUtility/XrHandle.h>
#include <XrUtility/XrExtensionContext.h>
#include <wil/resource.h>
#include <bgfx/bgfx.h>
#include "BgfxHandle.h"
#include <bgfx/platform.h>

#include <bx/uint32_t.h>
#include "bgfx_utils.h"

template <typename BgfxHandleType, typename close_fn_t = void (*)(BgfxHandleType), close_fn_t close_fn = ::bgfx::destroy>
using unique_bgfx_handle = wil::unique_any<BgfxHandleType, close_fn_t, close_fn, wil::details::pointer_access_all, BgfxHandleType, decltype(::bgfx::kInvalidHandle), ::bgfx::kInvalidHandle, BgfxHandleType>;

template <typename BgfxHandleType, typename close_fn_t = void (*)(BgfxHandleType), close_fn_t close_fn = ::bgfx::destroy>
using shared_bgfx_handle = wil::shared_any<unique_bgfx_handle<BgfxHandleType, close_fn_t, close_fn>>;

namespace sample::bg {

    bgfx::TextureFormat::Enum DxgiFormatToBgfxFormat(DXGI_FORMAT format);
    winrt::com_ptr<IDXGIAdapter1> GetAdapter(LUID adapterId);

    std::tuple<XrGraphicsBindingD3D11KHR, winrt::com_ptr<ID3D11Device>, winrt::com_ptr<ID3D11DeviceContext>>
    __stdcall
    BgfxCreateD3D11Binding(XrInstance instance,
                       XrSystemId systemId,
                       const xr::ExtensionContext& extensions,
                       bool singleThreadedD3D11Device,
                       const std::vector<D3D_FEATURE_LEVEL>& appSupportedFeatureLevels);

    struct SwapchainD3D11 {
        xr::SwapchainHandle Handle;
        DXGI_FORMAT Format{DXGI_FORMAT_UNKNOWN};
        int32_t Width{0};
        int32_t Height{0};
        std::vector<XrSwapchainImageD3D11KHR> Images;
    };

    SwapchainD3D11 __stdcall CreateSwapchainD3D11(XrSession session,
                                        DXGI_FORMAT format,
                                        int32_t width,
                                        int32_t height,
                                        uint32_t arrayLength,
                                        uint32_t sampleCount,
                                        XrSwapchainCreateFlags createFlags,
                                        XrSwapchainUsageFlags usageFlags,
                                        std::optional<XrViewConfigurationType> viewConfigurationForSwapchain = std::nullopt);
} // namespace sample::bgfx
