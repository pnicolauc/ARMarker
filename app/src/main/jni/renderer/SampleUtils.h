/*===============================================================================
Copyright (c) 2016 PTC Inc. All Rights Reserved.

Copyright (c) 2012-2014 Qualcomm Connected E xperiences, Inc. All Rights Reserved.

Vuforia is a trademark of PTC Inc., registered in the United States and other 
countries.
===============================================================================*/

#ifndef _VUFORIA_SAMPLEUTILS_H_
#define _VUFORIA_SAMPLEUTILS_H_

// Includes:
#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>  


#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <Vuforia/Vuforia.h>
#include <Vuforia/CameraDevice.h>
#include <Vuforia/Renderer.h>
#include <Vuforia/ImageTargetBuilder.h>

#include <Vuforia/VideoBackgroundConfig.h>
#include <Vuforia/Trackable.h>
#include <Vuforia/TrackableResult.h>
#include <Vuforia/DeviceTrackableResult.h>

#include <Vuforia/Tool.h>
#include <Vuforia/Tracker.h>
#include <Vuforia/TrackerManager.h>
#include <Vuforia/ObjectTracker.h>
#include <Vuforia/DeviceTracker.h>
#include <Vuforia/RotationalDeviceTracker.h>

#include <Vuforia/CameraCalibration.h>
#include <Vuforia/UpdateCallback.h>
#include <Vuforia/DataSet.h>
#include <Vuforia/Device.h>
#include <Vuforia/RenderingPrimitives.h>
#include <Vuforia/GLRenderer.h>
#include <Vuforia/StateUpdater.h>
#include <Vuforia/ViewList.h>
#import <Vuforia/Image.h>

#include <renderer/SampleUtils.h>
#include <renderer/CubeShaders.h>
#include <renderer/SampleAppRenderer.h>


#include <assimp/Importer.hpp>
#include <android/asset_manager_jni.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>     // Post processing fla
#include <assimp/DefaultLogger.hpp>
#include <renderer/myJNIHelper.h>
#include <renderer/modelAssimp.h>

#include <utils/SampleMath.h>

#include "opencv2/opencv.hpp"
// Utility for logging:
#define LOG_TAG    "Vuforia"
#define LOG(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)


// info used to render a mesh
struct Marker {
    const char* name;

    float rotation[4];
    float translation[3];
    float scale[3];
};

struct GPUObjs {
    unsigned int shaderProgramID    = 0;
    GLint vertexHandle              = 0;
    GLint textureCoordHandle        = 0;
    GLint mvpMatrixHandle           = 0;
    GLint texSampler2DHandle        = 0;

    SampleAppRenderer* sampleAppRenderer = 0;
    const aiScene* scene;
    ModelAssimp *gAssimpObject =NULL;
};

struct ScreenParams {
    // Screen dimensions:
    int screenWidth                 = 0;
    int screenHeight                = 0;

    // Indicates whether screen is in portrait (true) or landscape (false) mode
    bool isActivityInPortraitMode   = false;
};

struct FileSystem {
    static AAssetManager *mgr;
    // global pointer to instance of MyJNIHelper that is used to read from assets
};

struct DatasetData {
    // Constants:
    float kObjectScale;
    Vuforia::DataSet* targets  = 0;
    Vuforia::DataSet* udts  = 0;
    bool switchDataSetAsap           = false;
    std::map<std::string,Marker*> markers;
    Marker* currMarker;


    const int STONES_AND_CHIPS_DATASET_ID = 0;
    const int TARMAC_DATASET_ID = 1;
    int selectedDataset = STONES_AND_CHIPS_DATASET_ID;

};

struct SensorsData {
    jdouble scale;
    float rotation[9];
};
struct CameraData {
    Vuforia::CameraDevice::CAMERA_DIRECTION currentCamera;
    cv::Mat curr_frame;
};
struct RotDeviceTracker {
    Vuforia::RotationalDeviceTracker* deviceTracker=0;
    Vuforia::Matrix44F deviceMatrix;
    bool useDeviceTracker=false;
};
struct ExtTracking {
    bool useExtendedTracking=false;
};
struct TrackerParams {
    Vuforia::Matrix44F markerMatrix;
    Vuforia::Matrix44F modelViewMatrix;
    bool noTrackerAvailable=false;
    bool lastMarker=false;
    Vuforia::Matrix44F lastMat;
    Vuforia::Matrix44F sensorRotation;
    Vuforia::Matrix44F resMatrix;
};
struct UserDefTargets {
    Vuforia::ImageTargetBuilder* builder;
    bool building = false;
    bool scanning = false;
    Vuforia::Trackable* udt;
    int udtcount=0;
    float lastudttranslation[3];
    float translationUDT[3];
};

struct MVOParams {
    bool mvo;
    float fl;
    bool init = false;
    float* mvoTranslation=new float[3];
};

struct Trajectory{
    std::vector<float> vec = std::vector<float>();

};

struct AuxMat{
    Vuforia::Matrix44F conv;
    Vuforia::Matrix44F aux;
};

/// A utility class used by the Vuforia SDK samples.
class SampleUtils
{
public:

    static void printVector(const float* mat);

    /// Prints a 4x4 matrix.
    static void printMatrix(const float* matrix);
    
    /// Prints a 3x3 matrix.
    static void printMatrix33(const float* mat);

    static void setIDMatrix(float x,float y,float z,float* mat);
    static void zeroesFloatVector3(float* vec);

    /// Prints GL error information.
    static void checkGlError(const char* operation);
    
    static void setMatrix(float x1,float x2,float x3,float y1,float y2,float y3,float z1,float z2,float z3,float* mat);
    /// Set the rotation components of this 4x4 matrix.
    static void setRotationMatrix(float angle, float x, float y, float z, 
        float *nMatrix);
    
    /// Set the translation components of this 4x4 matrix.
    static void translatePoseMatrix(float x, float y, float z,
        float* nMatrix = NULL);

    static void multiplyMatrixForVector(float* mat1,float* mat2,float* out);
    
    static void
    setRotation33to44(float x1,float x2,float x3,
                                float y1,float y2,float y3,
                                float z1,float z2,float z3,
                                float* mat);

    static void setIdentity(float* mat);
    /// Applies a rotation.
    static void rotatePoseMatrix(float angle, float x, float y, float z, 
        float* nMatrix = NULL);

    static float* invertMatrix(float* m);

    static void printMatrix44(float* matrix);
    
    /// Applies a scaling transformation.
    static void scalePoseMatrix(float x, float y, float z, 
        float* nMatrix = NULL);

    /// transposes matrix
    static float* transposeMatrix(float* matrix);


    /// Multiplies the two matrices A and B and writes the result to C.
    static void multiplyMatrix(float *matrixA, float *matrixB, 
        float *matrixC);
    
    /// Initialize a shader.
    static unsigned int initShader(unsigned int shaderType, 
        const char* source);
    
    /// Create a shader program.
    static unsigned int createProgramFromBuffer(const char* vertexShaderBuffer,
        const char* fragmentShaderBuffer);
};

#endif // _VUFORIA_SAMPLEUTILS_H_
