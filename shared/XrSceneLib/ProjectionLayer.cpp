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

#include <XrUtility/XrMath.h>
#include <XrUtility/XrEnumerate.h>
#include <SampleShared/BgfxUtility.h>
#include <SampleShared/Trace.h>

#include "ProjectionLayer.h"
#include "CompositionLayers.h"
#include "Scene.h"
#include "SceneContext.h"
#include "BgfxRenderer.h"

//using namespace DirectX;

ProjectionLayer::ProjectionLayer(const xr::SessionContext& sessionContext) {
    auto primaryViewConfiguraionType = sessionContext.PrimaryViewConfigurationType;
    auto colorSwapchainFormat = sessionContext.SupportedColorSwapchainFormats[0];
    auto depthSwapchainFormat = sessionContext.SupportedDepthSwapchainFormats[0];

    m_defaultViewConfigurationType = primaryViewConfiguraionType;
    m_viewConfigComponents[primaryViewConfiguraionType].PendingConfig.ColorSwapchainFormat = colorSwapchainFormat;
    m_viewConfigComponents[primaryViewConfiguraionType].PendingConfig.DepthSwapchainFormat = depthSwapchainFormat;

    for (const XrViewConfigurationType type : sessionContext.EnabledSecondaryViewConfigurationTypes) {
        m_viewConfigComponents[type].PendingConfig.ColorSwapchainFormat = colorSwapchainFormat;
        m_viewConfigComponents[type].PendingConfig.DepthSwapchainFormat = depthSwapchainFormat;

        if (type == XR_VIEW_CONFIGURATION_TYPE_SECONDARY_MONO_FIRST_PERSON_OBSERVER_MSFT) {
            m_viewConfigComponents[type].PendingConfig.LayerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
        }
    }
}

void ProjectionLayer::PrepareRendering(const SceneContext& sceneContext,
                                       XrViewConfigurationType viewConfigType,
                                       const std::vector<XrViewConfigurationView>& viewConfigViews) {
    ViewConfigComponent& viewConfigComponent = m_viewConfigComponents.at(viewConfigType);
    ProjectionLayerConfig& layerPendingConfig = viewConfigComponent.PendingConfig;
    ProjectionLayerConfig& layerCurrentConfig = viewConfigComponent.CurrentConfig;

    bool shouldResetSwapchain = layerPendingConfig.ForceReset || layerCurrentConfig.DoubleWideMode != layerPendingConfig.DoubleWideMode ||
                                layerCurrentConfig.SwapchainSampleCount != layerPendingConfig.SwapchainSampleCount ||
                                layerCurrentConfig.SwapchainSizeScale.width != layerPendingConfig.SwapchainSizeScale.width ||
                                layerCurrentConfig.SwapchainSizeScale.height != layerPendingConfig.SwapchainSizeScale.height ||
                                layerCurrentConfig.ContentProtected != layerPendingConfig.ContentProtected;

    if (!shouldResetSwapchain && layerCurrentConfig.ColorSwapchainFormat != layerPendingConfig.ColorSwapchainFormat) {
        if (!xr::Contains(sceneContext.Session.SupportedColorSwapchainFormats, layerPendingConfig.ColorSwapchainFormat)) {
            throw std::runtime_error(
                fmt::format("Unsupported color swapchain format: {}", layerPendingConfig.ColorSwapchainFormat).c_str());
        }
        shouldResetSwapchain = true;
    }

    if (!shouldResetSwapchain && layerCurrentConfig.DepthSwapchainFormat != layerPendingConfig.DepthSwapchainFormat) {
        if (!xr::Contains(sceneContext.Session.SupportedDepthSwapchainFormats, layerPendingConfig.DepthSwapchainFormat)) {
            throw std::runtime_error(
                fmt::format("Unsupported depth swapchain format: {}", layerPendingConfig.DepthSwapchainFormat).c_str());
        }
        shouldResetSwapchain = true;
    }

    if (!shouldResetSwapchain && !viewConfigComponent.ColorSwapchain.get() || !viewConfigComponent.DepthSwapchain.get()) {
        shouldResetSwapchain = true;
    }

    layerPendingConfig.ForceReset = false;
    layerCurrentConfig = layerPendingConfig;

    // SceneLib only supports identical sized swapchain images for left and right eyes (texture array/double wide).
    // Thus if runtime gives us different image rect sizes for left/right eyes,
    // we use the maximum left/right imageRect extent for recommendedImageRectExtent
    assert(!viewConfigViews.empty());
    uint32_t recommendedImageRectWidth = 0;
    uint32_t recommendedImageRectHeight = 0;
    for (const XrViewConfigurationView& view : viewConfigViews) {
        recommendedImageRectWidth = std::max(recommendedImageRectWidth, view.recommendedImageRectWidth);
        recommendedImageRectHeight = std::max(recommendedImageRectHeight, view.recommendedImageRectHeight);
    }
    assert(recommendedImageRectWidth != 0 && recommendedImageRectHeight != 0);

    const uint32_t swapchainImageWidth =
        static_cast<uint32_t>(std::ceil(recommendedImageRectWidth * layerCurrentConfig.SwapchainSizeScale.width));
    const uint32_t swapchainImageHeight =
        static_cast<uint32_t>(std::ceil(recommendedImageRectHeight * layerCurrentConfig.SwapchainSizeScale.height));

    const uint32_t swapchainSampleCount = layerCurrentConfig.SwapchainSampleCount < 1
                                              ? viewConfigViews[xr::StereoView::Left].recommendedSwapchainSampleCount
                                              : layerCurrentConfig.SwapchainSampleCount;

    viewConfigComponent.Viewports.resize(viewConfigViews.size());

    for (uint32_t viewIndex = 0; viewIndex < (uint32_t)viewConfigViews.size(); viewIndex++) {
        viewConfigComponent.Viewports[viewIndex] = CD3D11_VIEWPORT(
            layerCurrentConfig.DoubleWideMode ? static_cast<float>(swapchainImageWidth * viewIndex + layerCurrentConfig.ViewportOffset.x)
                                              : static_cast<float>(layerCurrentConfig.ViewportOffset.x),
            static_cast<float>(layerCurrentConfig.ViewportOffset.y),
            static_cast<float>(swapchainImageWidth * layerCurrentConfig.ViewportSizeScale.width),
            static_cast<float>(swapchainImageHeight * layerCurrentConfig.ViewportSizeScale.height));

        const int32_t doubleWideOffsetX = static_cast<int32_t>(swapchainImageWidth * viewIndex);
        viewConfigComponent.LayerDepthImageRect[viewIndex] =
            viewConfigComponent.LayerColorImageRect[viewIndex] = {layerCurrentConfig.DoubleWideMode ? doubleWideOffsetX : 0,
                                                                  0,
                                                                  static_cast<int32_t>(std::ceil(swapchainImageWidth)),
                                                                  static_cast<int32_t>(std::ceil(swapchainImageHeight))};
    }

    if (!shouldResetSwapchain) {
        return;
    }

    const uint32_t wideScale = layerCurrentConfig.DoubleWideMode ? 2 : 1;
    const uint32_t arrayLength = layerCurrentConfig.DoubleWideMode ? 1 : (uint32_t)viewConfigViews.size();

    const std::optional<XrViewConfigurationType> viewConfigurationForSwapchain =
        sceneContext.Extensions.SupportsSecondaryViewConfiguration ? std::optional{viewConfigType} : std::nullopt;

    // Create color swapchain with recommended properties.
    viewConfigComponent.ColorSwapchain =
        sample::bg::CreateSwapchain(sceneContext.Session.Handle,
                                         layerCurrentConfig.ColorSwapchainFormat,
                                         swapchainImageWidth * wideScale,
                                         swapchainImageHeight,
                                         arrayLength,
                                         swapchainSampleCount,
                                         layerCurrentConfig.ContentProtected ? XR_SWAPCHAIN_CREATE_PROTECTED_CONTENT_BIT : 0,
                                         XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
                                         viewConfigurationForSwapchain);

    // Create depth swapchain with recommended properties.
    viewConfigComponent.DepthSwapchain =
        sample::bg::CreateSwapchain(sceneContext.Session.Handle,
                                         layerCurrentConfig.DepthSwapchainFormat,
                                         swapchainImageWidth * wideScale,
                                         swapchainImageHeight,
                                         arrayLength,
                                         swapchainSampleCount,
                                         layerCurrentConfig.ContentProtected ? XR_SWAPCHAIN_CREATE_PROTECTED_CONTENT_BIT : 0,
                                         XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                         viewConfigurationForSwapchain);

    {
        CD3D11_DEPTH_STENCIL_DESC depthStencilDesc(CD3D11_DEFAULT{});
        depthStencilDesc.StencilEnable = false;
        depthStencilDesc.DepthEnable = true;
        depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D11_COMPARISON_GREATER;
        m_reversedZDepthNoStencilTest = nullptr;
        CHECK_HRCMD(sceneContext.Device->CreateDepthStencilState(&depthStencilDesc, m_reversedZDepthNoStencilTest.put()));
    }

    viewConfigComponent.ProjectionViews.resize(viewConfigViews.size());
    viewConfigComponent.DepthInfo.resize(viewConfigViews.size());
    //Moved resource binding here so its not binding everytime it renders
    //sceneContext.PbrResources.Bind();
}

uint32_t AquireAndWaitForSwapchainImage(XrSwapchain handle) {
    uint32_t swapchainImageIndex;
    XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    CHECK_XRCMD(xrAcquireSwapchainImage(handle, &acquireInfo, &swapchainImageIndex));

    XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    waitInfo.timeout = XR_INFINITE_DURATION;
    CHECK_XRCMD(xrWaitSwapchainImage(handle, &waitInfo));

    return swapchainImageIndex;
}

bool ProjectionLayer::Render(SceneContext& sceneContext,
                             const FrameTime& frameTime,
                             XrSpace layerSpace,
                             const std::vector<XrView>& views,
                             const std::vector<std::unique_ptr<Scene>>& activeScenes,
                             XrViewConfigurationType viewConfig) {
    //static int __frameCount = 0;
    //wchar_t text_buffer[2048] = {0};                                               // temporary buffer
    //swprintf(text_buffer, _countof(text_buffer), L"frameIndex: %I64u, frame: %d\n\0", frameTime.FrameIndex, ++__frameCount); // convert
    //OutputDebugString(text_buffer);                                              // print


    ViewConfigComponent& viewConfigComponent = m_viewConfigComponents.at(viewConfig);
    const sample::bg::Swapchain& colorSwapchain = *viewConfigComponent.ColorSwapchain;
    const sample::bg::Swapchain& depthSwapchain = *viewConfigComponent.DepthSwapchain;

    // Use the full range of recommended image size to achieve optimum resolution
    const XrRect2Di imageRect = {{0, 0}, {(int32_t)colorSwapchain.Width, (int32_t)colorSwapchain.Height}};
    CHECK(colorSwapchain.Width == depthSwapchain.Width);
    CHECK(colorSwapchain.Height == depthSwapchain.Height);

    const uint32_t colorSwapchainWait = AquireAndWaitForSwapchainImage(colorSwapchain.Handle.Get());
    const uint32_t depthSwapchainWait = AquireAndWaitForSwapchainImage(depthSwapchain.Handle.Get());

    std::vector<XrCompositionLayerProjectionView>& projectionViews = viewConfigComponent.ProjectionViews;
    std::vector<XrCompositionLayerDepthInfoKHR>& depthInfo = viewConfigComponent.DepthInfo;
    std::vector<D3D11_VIEWPORT>& viewports = viewConfigComponent.Viewports;
    const ProjectionLayerConfig& currentConfig = viewConfigComponent.CurrentConfig;

    bool submitProjectionLayer = false;

    // For Hololens additive display, best to clear render target with transparent black color (0,0,0,0)
    constexpr DirectX::XMVECTORF32 opaqueColor = {0.184313729f, 0.309803933f, 0.309803933f, 1.000000000f};
    constexpr DirectX::XMVECTORF32 transparent = {0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f};
    constexpr DirectX::XMVECTORF32 red = {1.000000000f, 0.000000000f, 0.000000000f, 1.000000000f};
    


    if (false/*(colorSwapchainWait != XR_SUCCESS) || (depthSwapchainWait != XR_SUCCESS)*/) {
        // Swapchain image timeout, don't submit this multi projection layer
        submitProjectionLayer = false;
    } else {
        const uint32_t viewCount = (uint32_t)views.size();
        std::vector<xr::math::ViewProjection> viewProjections(viewCount);
        
        const bool reversedZ = (currentConfig.NearFar.Near > currentConfig.NearFar.Far);
        for (uint32_t viewIndex = 0; viewIndex < viewCount; viewIndex++) {
            
            viewProjections[viewIndex] = {views[viewIndex].pose, views[viewIndex].fov, m_nearFar};
            const XrView& projection = views[viewIndex];
            DirectX::XMMATRIX worldToViewMatrix = xr::math::LoadInvertedXrPose(projectionViews[viewIndex].pose);
            //worldToViewMatrix.r[0].m128_f32[0] = 0.0;
            //worldToViewMatrix.r[0].m128_f32[1] = 20.0;

            const XrFovf fov = projection.fov;
            const DirectX::XMMATRIX projectionMatrix = xr::math::ComposeProjectionMatrix(fov, currentConfig.NearFar);
            
            sceneContext.PbrResources.SetViewProjection(worldToViewMatrix, projectionMatrix);
            // sceneContext.PbrResources.Bind();
            sceneContext.PbrResources.SetDepthFuncReversed(reversedZ);
            const XrPosef viewPose = projection.pose;
            xr::math::NearFar NearFar = currentConfig.NearFar;
            const float normalizedViewportMinDepth = 0;
            const float normalizedViewportMaxDepth = 1;
            const uint32_t colorImageArrayIndex = currentConfig.DoubleWideMode ? 0 : viewIndex;
            const uint32_t depthImageArrayIndex = currentConfig.DoubleWideMode ? 0 : viewIndex;

            viewConfigComponent.LayerSpace = layerSpace;
            projectionViews[viewIndex] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
            projectionViews[viewIndex].pose = viewPose;
            projectionViews[viewIndex].fov.angleLeft = fov.angleLeft * currentConfig.SwapchainFovScale.width;
            projectionViews[viewIndex].fov.angleRight = fov.angleRight * currentConfig.SwapchainFovScale.width;
            projectionViews[viewIndex].fov.angleUp = fov.angleUp * currentConfig.SwapchainFovScale.height;
            projectionViews[viewIndex].fov.angleDown = fov.angleDown * currentConfig.SwapchainFovScale.height;
            projectionViews[viewIndex].subImage.swapchain = colorSwapchain.Handle.Get();
            projectionViews[viewIndex].subImage.imageArrayIndex = colorImageArrayIndex;
            projectionViews[viewIndex].subImage.imageRect = viewConfigComponent.LayerColorImageRect[viewIndex];

            D3D11_VIEWPORT viewport = viewports[viewIndex];
            if (currentConfig.SubmitDepthInfo && sceneContext.Extensions.SupportsDepthInfo) {
                depthInfo[viewIndex] = {XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR};
                depthInfo[viewIndex].minDepth = viewport.MinDepth = normalizedViewportMinDepth;
                depthInfo[viewIndex].maxDepth = viewport.MaxDepth = normalizedViewportMaxDepth;
                depthInfo[viewIndex].nearZ = currentConfig.NearFar.Near;
                depthInfo[viewIndex].farZ = currentConfig.NearFar.Far;
                depthInfo[viewIndex].subImage.swapchain = depthSwapchain.Handle.Get();
                depthInfo[viewIndex].subImage.imageArrayIndex = depthImageArrayIndex;
                depthInfo[viewIndex].subImage.imageRect = viewConfigComponent.LayerDepthImageRect[viewIndex];

                projectionViews[viewIndex].next = &depthInfo[viewIndex];
            } else {
                projectionViews[viewIndex].next = nullptr;
            }
            
            
        }
        const DirectX::XMVECTORF32 renderTargetClearColor = opaqueColor; //(m_environmentBlendMode == XR_ENVIRONMENT_BLEND_MODE_OPAQUE) ? opaqueColor : transparent;
        switch (bgfx::getRendererType()) {
        case bgfx::RendererType::Direct3D11:
            sample::bg::RenderView(imageRect,
                                   renderTargetClearColor,
                                   viewProjections,
                                   colorSwapchain.Format,
                                   ((sample::bg::SwapchainD3D11&)(colorSwapchain)).Images[colorSwapchainWait].texture,
                                   depthSwapchain.Format,
                                   ((sample::bg::SwapchainD3D11&)(depthSwapchain)).Images[depthSwapchainWait].texture
                                   ,activeScenes,
                                   frameTime,
                                   submitProjectionLayer                               
            );
            break;

        case 4: // bgfx::RendererType::Enum::Direct3D12:
            printf("Direct3d12");
            /*sample::bg::RenderView(
                imageRect,
                renderTargetClearColor,
                viewProjections,
                colorSwapchain.Format,
                static_cast<sample::bg::SwapchainD3D12&>(*m_renderResources->ColorSwapchain).Images[colorSwapchainImageIndex].texture,
                depthSwapchain.Format,
                static_cast<sample::bg::SwapchainD3D12&>(*m_renderResources->DepthSwapchain).Images[depthSwapchainImageIndex].texture,
                visibleCubes);*/
            break;

        default:
            CHECK(false);
        }

        // Render for this view pose.
        /*{
            for (const std::unique_ptr<Scene>& scene : activeScenes) {
                if (scene->IsActive() && !std::empty(scene->GetSceneObjects())) {
                    submitProjectionLayer = true;
                    scene->Render(frameTime);
                }
            }
        }*/
    }

    // Now that the scene is done writing to the swapchain, it must be released in order to be made available for
    // xrEndFrame.
    const XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    CHECK_XRCMD(xrReleaseSwapchainImage(colorSwapchain.Handle.Get(), &releaseInfo));
    CHECK_XRCMD(xrReleaseSwapchainImage(depthSwapchain.Handle.Get(), &releaseInfo));

    sceneContext.PbrResources.UpdateAnimationTime(frameTime.TotalElapsed);

    return submitProjectionLayer;
}

void AppendProjectionLayer(CompositionLayers& layers, const ProjectionLayer* layer, XrViewConfigurationType viewConfig) {
    XrCompositionLayerProjection& projectionLayer = layers.AddProjectionLayer(layer->Config(viewConfig).LayerFlags);
    projectionLayer.space = layer->LayerSpace(viewConfig);
    projectionLayer.viewCount = (uint32_t)layer->ProjectionViews(viewConfig).size();
    projectionLayer.views = layer->ProjectionViews(viewConfig).data();
}

