/*===============================================================================
Copyright (c) 2016 PTC Inc. All Rights Reserved.

Vuforia is a trademark of PTC Inc., registered in the United States and other
countries.
===============================================================================*/

#ifndef _VUFORIA_SAMPLEAPPRENDERER_H_
#define _VUFORIA_SAMPLEAPPRENDERER_H_

// Includes:
#include <stdio.h>
#include <android/log.h>
#include <pthread.h>
#include <math.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <Vuforia/Vuforia.h>
#include <Vuforia/CameraDevice.h>
#include <Vuforia/Renderer.h>
#include <Vuforia/VideoBackgroundConfig.h>
#include <Vuforia/Tool.h>
#include <Vuforia/Device.h>
#include <Vuforia/RenderingPrimitives.h>
#include <Vuforia/GLRenderer.h>
#include <Vuforia/StateUpdater.h>
#include <Vuforia/TrackerManager.h>
#include <Vuforia/ViewList.h>

#include "CubeShaders.h"
#include "ImageTargets.h"

// Utility for logging:
#define LOG_TAG    "Vuforia"
#define LOG(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

/// Class which encapsulates the rendering primitives usage
class SampleAppRenderer
{
private:
    // Pointer to RenderingPrimitives that are currently in use in the rendering loop.
    Vuforia::RenderingPrimitives* renderingPrimitives = NULL;
    // Mutex to manage access to the RenderingPrimitives
    pthread_mutex_t renderingPrimitivesMutex;

    unsigned int vbShaderProgramID    = 0;
    GLint vbVertexHandle              = 0;
    GLint vbTextureCoordHandle        = 0;
    GLint vbMvpMatrixHandle           = 0;
    GLint vbTexSampler2DHandle        = 0;

public:
    SampleAppRenderer();
    ~SampleAppRenderer();

    // Initial state for videobackground rendering
    void initRendering();

    // Renders the camera feed
    void renderVideoBackground();

    // Get scale factor for the video background rendering
    float getSceneScaleFactor();

    // Call render to loop through the available views
    void renderFrame();

    // Updates rendering primitives on start and configuration changes
    void updateRenderingPrimitives();

};

#endif // _VUFORIA_SAMPLEAPPRENDERER_H_
