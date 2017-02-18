/*===============================================================================
Copyright (c) 2016 PTC Inc. All Rights Reserved.

Vuforia is a trademark of PTC Inc., registered in the United States and other
countries.
===============================================================================*/

#include "SampleAppRenderer.h"
#include "SampleUtils.h"

SampleAppRenderer::SampleAppRenderer()
{
    // Setup the mutex used to guard updates to RenderingPrimitives
    pthread_mutex_init(&renderingPrimitivesMutex, 0);
}

SampleAppRenderer::~SampleAppRenderer()
{
    pthread_mutex_destroy(&renderingPrimitivesMutex);
}

float SampleAppRenderer::getSceneScaleFactor()
{
    static const float VIRTUAL_FOV_Y_DEGS = 85.0f;

    // Get the y-dimension of the physical camera field of view
    Vuforia::Vec2F fovVector = Vuforia::CameraDevice::getInstance().getCameraCalibration().getFieldOfViewRads();
    float cameraFovYRads = fovVector.data[1];

    // Get the y-dimension of the virtual camera field of view
    float virtualFovYRads = VIRTUAL_FOV_Y_DEGS * M_PI / 180;

    // The scene-scale factor represents the proportion of the viewport that is filled by
    // the video background when projected onto the same plane.
    // In order to calculate this, let 'd' be the distance between the cameras and the plane.
    // The height of the projected image 'h' on this plane can then be calculated:
    //   tan(fov/2) = h/2d
    // which rearranges to:
    //   2d = h/tan(fov/2)
    // Since 'd' is the same for both cameras, we can combine the equations for the two cameras:
    //   hPhysical/tan(fovPhysical/2) = hVirtual/tan(fovVirtual/2)
    // Which rearranges to:
    //   hPhysical/hVirtual = tan(fovPhysical/2)/tan(fovVirtual/2)
    // ... which is the scene-scale factor
    return tan(cameraFovYRads / 2) / tan(virtualFovYRads / 2);
}


void SampleAppRenderer::renderVideoBackground()
{
    // Use texture unit 0 for the video background - this will hold the camera frame and we want to reuse for all views
    // So need to use a different texture unit for the augmentation
    int vbVideoTextureUnit = 0;

    // Bind the video bg texture and get the Texture ID from Vuforia
    Vuforia::GLTextureUnit tex;

    // Please note that if you want to use a specific GL texture handle for the video background, you should configure it
    // from the initRendering() method and use the Renderer::setVideoBackgroundTexture( GLTextureData ) method.
    // Please refer to API Guide for more information.
    tex.mTextureUnit = vbVideoTextureUnit;

    if (! Vuforia::Renderer::getInstance().updateVideoBackgroundTexture(&tex))
    {
        LOG("Unable to bind video background texture!!");
        return;
    }

    Vuforia::Matrix44F vbProjectionMatrix = Vuforia::Tool::convert2GLMatrix(
                                                                            renderingPrimitives->getVideoBackgroundProjectionMatrix(Vuforia::VIEW_SINGULAR, Vuforia::COORDINATE_SYSTEM_CAMERA));

    // Apply the scene scale on video see-through eyewear, to scale the video background and augmentation
    // so that the display lines up with the real world
    // This should not be applied on optical see-through devices, as there is no video background,
    // and the calibration ensures that the augmentation matches the real world
    if (Vuforia::Device::getInstance().isViewerActive())
    {
        float sceneScaleFactor = getSceneScaleFactor();
        SampleUtils::scalePoseMatrix(sceneScaleFactor, sceneScaleFactor, 1.0f, vbProjectionMatrix.data);
    }

    GLboolean depthTest = false;
    GLboolean cullTest = false;
    GLboolean scissorsTest = false;

    glGetBooleanv(GL_DEPTH_TEST, &depthTest);
    glGetBooleanv(GL_CULL_FACE, &cullTest);
    glGetBooleanv(GL_SCISSOR_TEST, &scissorsTest);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);

    const Vuforia::Mesh& vbMesh = renderingPrimitives->getVideoBackgroundMesh(Vuforia::VIEW_SINGULAR);
    // Load the shader and upload the vertex/texcoord/index data
    glUseProgram(vbShaderProgramID);
    glVertexAttribPointer(vbVertexHandle, 3, GL_FLOAT, false, 0, vbMesh.getPositionCoordinates());
    glVertexAttribPointer(vbTextureCoordHandle, 2, GL_FLOAT, false, 0, vbMesh.getUVCoordinates());

    glUniform1i(vbTexSampler2DHandle, vbVideoTextureUnit);

    // Render the video background with the custom shader
    // First, we enable the vertex arrays
    glEnableVertexAttribArray(vbVertexHandle);
    glEnableVertexAttribArray(vbTextureCoordHandle);

    // Pass the projection matrix to OpenGL
    glUniformMatrix4fv(vbMvpMatrixHandle, 1, GL_FALSE, vbProjectionMatrix.data);

    // Then, we issue the render call
    glDrawElements(GL_TRIANGLES, vbMesh.getNumTriangles() * 3, GL_UNSIGNED_SHORT,
                   vbMesh.getTriangles());

    // Finally, we disable the vertex arrays
    glDisableVertexAttribArray(vbVertexHandle);
    glDisableVertexAttribArray(vbTextureCoordHandle);

    if(depthTest)
        glEnable(GL_DEPTH_TEST);

    if(cullTest)
        glEnable(GL_CULL_FACE);

    if(scissorsTest)
        glEnable(GL_SCISSOR_TEST);

    SampleUtils::checkGlError("Rendering of the video background failed");
}

void SampleAppRenderer::renderFrame()
{
    pthread_mutex_lock(&renderingPrimitivesMutex);

    Vuforia::Renderer& renderer = Vuforia::Renderer::getInstance();
    const Vuforia::State state = Vuforia::TrackerManager::getInstance().getStateUpdater().updateState();
    renderer.begin(state);

    Vuforia::ViewList& viewList = renderingPrimitives->getRenderingViews();

    // Clear colour and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Iterate over the ViewList
    for (int viewIdx = 0; viewIdx < viewList.getNumViews(); viewIdx++) {
        Vuforia::VIEW vw = viewList.getView(viewIdx);
        // Set up the viewport
        Vuforia::Vec4I viewport;
        // We're writing directly to the screen, so the viewport is relative to the screen
        viewport = renderingPrimitives->getViewport(vw);

        // Set viewport for current view
        glViewport(viewport.data[0], viewport.data[1], viewport.data[2], viewport.data[3]);

        //set scissor
        glScissor(viewport.data[0], viewport.data[1], viewport.data[2], viewport.data[3]);

        Vuforia::Matrix34F projMatrix = renderingPrimitives->getProjectionMatrix(vw,
                                                                Vuforia::COORDINATE_SYSTEM_CAMERA);

        Vuforia::Matrix44F rawProjectionMatrixGL = Vuforia::Tool::convertPerspectiveProjection2GLMatrix(
                                                                                                        projMatrix,
                                                                                                        0.01,
                                                                                                        5);

        // Apply the appropriate eye adjustment to the raw projection matrix, and assign to the global variable
        Vuforia::Matrix44F eyeAdjustmentGL = Vuforia::Tool::convert2GLMatrix(renderingPrimitives->getEyeDisplayAdjustmentMatrix(vw));

        Vuforia::Matrix44F currentProjectionMatrix;
        SampleUtils::multiplyMatrix(&rawProjectionMatrixGL.data[0], &eyeAdjustmentGL.data[0], &currentProjectionMatrix.data[0]);

        // Call renderFrameForView from ImageTargets.cpp
        // This will be called for MONO, LEFT and RIGHT views, POSTPROCESS will not render the
        // frame, in this specific case MONO is the only view to be rendered since we are not
        // in stereo mode
        if (vw != Vuforia::VIEW_POSTPROCESS) {
            renderFrameForView(&state, currentProjectionMatrix);
        }
    }
    renderer.end();

    pthread_mutex_unlock(&renderingPrimitivesMutex);
}

void SampleAppRenderer::initRendering()
{
    vbShaderProgramID     = SampleUtils::createProgramFromBuffer(cubeMeshVertexShader,
                                                            cubeFragmentShader);

    vbVertexHandle        = glGetAttribLocation(vbShaderProgramID,
                                                "vertexPosition");
    vbTextureCoordHandle  = glGetAttribLocation(vbShaderProgramID,
                                                "vertexTexCoord");
    vbMvpMatrixHandle     = glGetUniformLocation(vbShaderProgramID,
                                                "modelViewProjectionMatrix");
    vbTexSampler2DHandle  = glGetUniformLocation(vbShaderProgramID,
                                                "texSampler2D");
}

void SampleAppRenderer::updateRenderingPrimitives()
{
    LOG("SampleAppRenderer_updateRenderingPrimitives");

    pthread_mutex_lock(&renderingPrimitivesMutex);
    if (renderingPrimitives != NULL)
    {
        delete renderingPrimitives;
        renderingPrimitives = NULL;
    }

    // Cache the rendering primitives whenever there is an orientation change or initial setup
    renderingPrimitives = new Vuforia::RenderingPrimitives(Vuforia::Device::getInstance().getRenderingPrimitives());
    pthread_mutex_unlock(&renderingPrimitivesMutex);
}