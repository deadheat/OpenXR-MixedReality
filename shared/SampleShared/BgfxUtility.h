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
#include "BgfxUtility.h"
#include "Trace.h"

#pragma comment(lib, "D3DCompiler.lib")

#include <DirectXMath.h>

#include <memory>

#include <winrt/base.h> // winrt::com_ptr
#include <d3dcommon.h>  //ID3DBlob
#include <XrUtility/XrHandle.h>
#include <XrUtility/XrExtensionContext.h>
#include <wil/resource.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include <bx/uint32_t.h>
#include "bgfx_utils.h"

template <typename bgfx_handle_t>
struct bgfx_handle_wrapper_t : public bgfx_handle_t {
    bgfx_handle_wrapper_t(const bgfx_handle_t& handle) {
        this->idx = handle.idx;
    }

    bgfx_handle_wrapper_t(const uint16_t& handle) {
        this->idx = handle;
    }

    operator uint16_t() const {
        return this->idx;
    }
};

template <typename bgfx_handle_t, typename close_fn_t = void (*)(bgfx_handle_t), close_fn_t close_fn = bgfx::destroy>
using unique_bgfx_handle = wil::unique_any<bgfx_handle_wrapper_t<bgfx_handle_t>,
                                           close_fn_t,
                                           close_fn,
                                           wil::details::pointer_access_all,
                                           bgfx_handle_wrapper_t<bgfx_handle_t>,
                                           decltype(bgfx::kInvalidHandle),
                                           bgfx::kInvalidHandle,
                                           bgfx_handle_wrapper_t<bgfx_handle_t>>;

template <typename bgfx_handle_t,
          typename close_fn_t = void (*)(bgfx_handle_t),
          close_fn_t close_fn = bgfx::destroy>
using shared_bgfx_handle = wil::shared_any<unique_bgfx_handle<bgfx_handle_t, close_fn_t, close_fn>>;

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
