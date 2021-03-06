//
// Copyright 2018 Intel
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef HDOSPRAY_RENDER_PASS_H
#define HDOSPRAY_RENDER_PASS_H

#include "pxr/base/gf/matrix4d.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/pxr.h"

#include "ospray/ospray.h"

#if HDOSPRAY_ENABLE_DENOISER
#    include <OpenImageDenoise/oidn.hpp>
#endif

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdOSPRayRenderPass
///
/// HdRenderPass represents a single render iteration, rendering a view of the
/// scene (the HdRprimCollection) for a specific viewer (the camera/viewport
/// parameters in HdRenderPassState) to the current draw target.
///
/// This class does so by raycasting into the OSPRay scene.
///
class HdOSPRayRenderPass final : public HdRenderPass {
public:
    /// Renderpass constructor.
    ///   \param index The render index containing scene data to render.
    ///   \param collection The initial rprim collection for this renderpass.
    ///   \param scene The OSPRay scene to raycast into.
    HdOSPRayRenderPass(HdRenderIndex* index,
                       HdRprimCollection const& collection, OSPModel model,
                       OSPRenderer renderer, std::atomic<int>* sceneVersion);

    /// Renderpass destructor.
    virtual ~HdOSPRayRenderPass();

    // -----------------------------------------------------------------------
    // HdRenderPass API

    /// Clear the sample buffer (when scene or camera changes).
    virtual void ResetImage();

    /// Determine whether the sample buffer has enough samples.
    ///   \return True if the image has enough samples to be considered final.
    virtual bool IsConverged() const override;

protected:
    // -----------------------------------------------------------------------
    // HdRenderPass API

    /// Draw the scene with the bound renderpass state.
    ///   \param renderPassState Input parameters (including viewer parameters)
    ///                          for this renderpass.
    ///   \param renderTags Which rendertags should be drawn this pass.
    virtual void _Execute(HdRenderPassStateSharedPtr const& renderPassState,
                          TfTokenVector const& renderTags) override;

    /// Update internal tracking to reflect a dirty collection.
    virtual void _MarkCollectionDirty() override;

private:
    // -----------------------------------------------------------------------
    // Internal API

    // Specify a new viewport size for the sample buffer. Note: the caller
    // should also call ResetImage().
    void _ResizeSampleBuffer(unsigned int width, unsigned int height);

    // The sample buffer is cleared in Execute(), so this flag records whether
    // ResetImage() has been called since the last Execute().
    bool _pendingResetImage;
    bool _pendingModelUpdate;

    OSPFrameBuffer _frameBuffer;

    OSPRenderer _renderer;

    // A reference to the global scene version.
    std::atomic<int>* _sceneVersion;
    // The last scene version we rendered with.
    int _lastRenderedVersion;

    // The resolved output buffer, in GL_RGBA. This is an intermediate between
    // _sampleBuffer and the GL framebuffer.
    std::vector<osp::vec4f> _colorBuffer;

    // The width of the viewport we're rendering into.
    unsigned int _width;
    // The height of the viewport we're rendering into.
    unsigned int _height;

    // OSPRay model that will hold OSPRay specific geometry
    OSPModel _model;

    OSPCamera _camera;

    // The inverse view matrix: camera space to world space.
    GfMatrix4d _inverseViewMatrix;
    // The inverse projection matrix: NDC space to camera space.
    GfMatrix4d _inverseProjMatrix;

    // The color of a ray miss.
    GfVec3f _clearColor;

#if HDOSPRAY_ENABLE_DENOISER
    oidn::DeviceRef _denoiserDevice;
    oidn::FilterRef _denoiserFilter;
#endif

    bool _denoiserDirty { true };
    std::vector<osp::vec3f> _normalBuffer;
    std::vector<osp::vec3f> _albedoBuffer;
    std::vector<osp::vec4f> _denoisedBuffer;

    int _numSamplesAccumulated { 0 }; // number of rendered frames not cleared
    int _spp { 1 };
    bool _useDenoiser { false };
    int _denoiserSPPThreshold { 3 };

    void Denoise();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDOSPRAY_RENDER_PASS_H
