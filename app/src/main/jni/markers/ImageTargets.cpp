/*===============================================================================
Copyright (c) 2016 PTC Inc. All Rights Reserved.


Copyright (c) 2012-2014 Qualcomm Connected Experiences, Inc. All Rights Reserved.

Vuforia is a trademark of PTC Inc., registered in the United States and other 
countries.
===============================================================================*/

#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <Vuforia/Vuforia.h>
#include <Vuforia/CameraDevice.h>
#include <Vuforia/Renderer.h>
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

#include <mvo/mvo.h>

#include "opencv2/opencv.hpp"


#ifdef __cplusplus
extern "C"
{
#endif


unsigned int shaderProgramID    = 0;
GLint vertexHandle              = 0;
GLint textureCoordHandle        = 0;
GLint mvpMatrixHandle           = 0;
GLint texSampler2DHandle        = 0;

// Screen dimensions:
int screenWidth                 = 0;
int screenHeight                = 0;

// Indicates whether screen is in portrait (true) or landscape (false) mode
bool isActivityInPortraitMode   = false;

// Constants:
float kObjectScale;

Vuforia::DataSet* dataSet  = 0;
Vuforia::RotationalDeviceTracker* deviceTracker=0;

Vuforia::Matrix44F deviceMatrix;
Vuforia::Matrix44F markerMatrix;


SampleAppRenderer* sampleAppRenderer = 0;

bool switchDataSetAsap           = false;

Vuforia::CameraDevice::CAMERA_DIRECTION currentCamera;

const int STONES_AND_CHIPS_DATASET_ID = 0;
const int TARMAC_DATASET_ID = 1;
int selectedDataset = STONES_AND_CHIPS_DATASET_ID;


static AAssetManager *mgr;

const aiScene* scene;

bool bLoaded;

std::map<std::string,Marker*> markers;
Marker* currMarker;


// global pointer to instance of MyJNIHelper that is used to read from assets
MyJNIHelper * gHelperObject=NULL;

// global pointer is used in JNI calls to reference to same object of type Cube
ModelAssimp *gAssimpObject =NULL;


using namespace cv;
Mat curr_frame;
bool mvo;
float fl;
bool init = false;


jdouble scale;
float* rotation;

Vuforia::Matrix44F modelViewMatrix;


bool useDeviceTracker=true;
bool useExtendedTracking=false;

Vuforia::Matrix44F conv;
Vuforia::Matrix44F aux;

// Object to receive update callbacks from Vuforia SDK
class ImageTargets_UpdateCallback : public Vuforia::UpdateCallback
{
    virtual void Vuforia_onUpdate(Vuforia::State& state)
    {
        Vuforia::Frame frame = state.getFrame();
        LOG("images in frame %d.",frame.getNumImages());

        for (int i = 0; i < frame.getNumImages(); ++i) {
            const Vuforia::Image *image = frame.getImage(i);
            if (image->getFormat() == Vuforia::RGB565) {
                //LOG("init %d mvo %d",init, mvo);
                if(!init && mvo){
                    mvoInit(fl,(float)(image->getHeight()/16.0),(float)(image->getWidth()/16.0));
                    init=true;
                }
                curr_frame = Mat(image->getHeight(),image->getWidth(),CV_8UC2,(unsigned char *)image->getPixels());
                resize(curr_frame, curr_frame, Size(image->getHeight()/8, image->getHeight()/8), 0, 0, INTER_CUBIC); // resize to 1024x768 resolution
                cvtColor(curr_frame, curr_frame, CV_BGR5652GRAY);


                LOG("Saving frame as Mat. SIZE: %d %d.",image->getHeight(),image->getWidth());
                //imwrite( "curr_frame.jpg", curr_frame );

            }
        }
        if (switchDataSetAsap)
        {
            switchDataSetAsap = false;

            // Get the object tracker:
            Vuforia::TrackerManager& trackerManager = Vuforia::TrackerManager::getInstance();
            Vuforia::ObjectTracker* objectTracker = static_cast<Vuforia::ObjectTracker*>(
                        trackerManager.getTracker(Vuforia::ObjectTracker::getClassType()));

            if (objectTracker == 0)
            {
                LOG("Failed to switch data set.");
                return;
            }
            objectTracker->activateDataSet(dataSet);


            if(useExtendedTracking)
            {
                Vuforia::DataSet* currentDataSet = objectTracker->getActiveDataSet(0);
                for (int tIdx = 0; tIdx < currentDataSet->getNumTrackables(); tIdx++)
                {
                    Vuforia::Trackable* trackable = currentDataSet->getTrackable(tIdx);
                    trackable->startExtendedTracking();
                }
            }



        }
    }
};

ImageTargets_UpdateCallback updateCallback;


JNIEXPORT void JNICALL
Java_com_mavoar_markers_ImageTargets_setActivityPortraitMode(JNIEnv *, jobject, jboolean isPortrait)
{
    isActivityInPortraitMode = isPortrait;
}

JNIEXPORT void JNICALL
Java_com_mavoar_markers_ImageTargets_switchDatasetAsap(JNIEnv* env, jobject, jstring dataset)
{
    selectedDataset = 0;
    switchDataSetAsap = true;
}


JNIEXPORT int JNICALL
Java_com_mavoar_markers_ImageTargets_initTracker(
JNIEnv *env, jobject instance,jobject assetManager,jstring pathToInternalDir,
jstring obj,jstring mtl,jstring xml,jstring folder,jfloat scale, jint markerNum,
 jobjectArray markerNames,jfloatArray markerRot,jfloatArray markerTra, jfloatArray markerSca,jboolean jmvo)
{

    SampleUtils::setRotationMatrix(-90,0,0,1,conv.data);

    SampleUtils::setRotationMatrix(180,0,1,0,aux.data);

    LOG("Java_com_mavoar_markers_ImageTargets_initTracker");
    mvo = jmvo;

    kObjectScale=(float)scale;
    jfloat* rots = env->GetFloatArrayElements( markerRot,0);
    jfloat* trans = env->GetFloatArrayElements( markerTra,0);
    jfloat* scals = env->GetFloatArrayElements( markerSca,0);

    for(unsigned int a=0;a<markerNum;a++){
        Marker* marker= new Marker;

        jstring string = (jstring) (env->GetObjectArrayElement(markerNames, a));
        const char* na= env->GetStringUTFChars(string, NULL );

        marker->name=na;

        marker->rotation[0]=(float)rots[(a*3)];
        marker->rotation[1]=(float)rots[(a*3)+1];
        marker->rotation[2]=(float)rots[(a*3)+2];
        marker->rotation[3]=(float)rots[(a*3)+3];

        marker->translation[0]=(float)trans[(a*3)];
        marker->translation[1]=(float)trans[(a*3)+1];
        marker->translation[2]=(float)trans[(a*3)+2];

        marker->scale[0]=(float)scals[(a*3)];
        marker->scale[1]=(float)scals[(a*3)+1];
        marker->scale[2]=(float)scals[(a*3)+2];

        //std::string str(marker.name);
        markers[marker->name] = marker;

        currMarker = marker;
    }

    int markerN=(int)markerNum;

    const char *objCPP;
    objCPP = env->GetStringUTFChars(obj, NULL );

    const char *mtlCPP;
    mtlCPP = env->GetStringUTFChars(mtl, NULL );

    const char *xmlCPP;
    xmlCPP = env->GetStringUTFChars(xml, NULL );

    const char *folderCPP;
    folderCPP = env->GetStringUTFChars(folder, NULL ) ;

    gHelperObject = new MyJNIHelper(env, instance, assetManager, pathToInternalDir);
    gAssimpObject = new ModelAssimp();

    gAssimpObject->PerformGLInits(objCPP,mtlCPP,folderCPP);



    // Initialize the object tracker:
    Vuforia::TrackerManager& trackerManager = Vuforia::TrackerManager::getInstance();
    Vuforia::Tracker* tracker = trackerManager.initTracker(Vuforia::ObjectTracker::getClassType());
    if (tracker == NULL)
    {
        LOG("Failed to initialize ObjectTracker.");
        return 0;
    }

    LOG("Successfully initialized ObjectTracker.");



    return 1;
}


JNIEXPORT void JNICALL
Java_com_mavoar_markers_ImageTargets_deinitTracker(JNIEnv *, jobject)
{
    LOG("Java_com_mavoar_markers_ImageTargets_deinitTracker");

    // Deinit the object tracker:
    Vuforia::TrackerManager& trackerManager = Vuforia::TrackerManager::getInstance();
    trackerManager.deinitTracker(Vuforia::ObjectTracker::getClassType());
}

void printMatrix(const float* mat)
{
    for(int r=0; r<4; r++,mat+=4)
        LOG("%7.3f %7.3f %7.3f %7.3f", mat[0], mat[1], mat[2], mat[3]);
}


JNIEXPORT int JNICALL
Java_com_mavoar_markers_ImageTargets_loadTrackerData(JNIEnv *env, jobject,jstring xml)
{
    LOG("Java_com_mavoar_markers_ImageTargets_loadTrackerData");

    const char *xmlFile;
    xmlFile = env->GetStringUTFChars(xml, NULL );

    LOG("XML FILE: %s",xmlFile);

    // Get the object tracker:
    Vuforia::TrackerManager& trackerManager = Vuforia::TrackerManager::getInstance();
    Vuforia::ObjectTracker* objectTracker = static_cast<Vuforia::ObjectTracker*>(
                    trackerManager.getTracker(Vuforia::ObjectTracker::getClassType()));
    if (objectTracker == NULL)
    {
        LOG("Failed to load tracking data set because the ObjectTracker has not"
            " been initialized.");
        return 0;
    }


     deviceTracker = static_cast<Vuforia::RotationalDeviceTracker*>(
     trackerManager.initTracker(Vuforia:: RotationalDeviceTracker::getClassType()));

    // activate pose prediction
    deviceTracker->setPosePrediction(false);

    // activate model correction: default handheld model
    deviceTracker->setModelCorrection((Vuforia::TransformModel*)deviceTracker->getDefaultHandheldModel());
    // start the tracker
    deviceTracker->start();


    // Create the data set:
    dataSet=objectTracker->createDataSet();
    if (dataSet == 0)
    {
        LOG("Failed to create a new tracking data.");
        return 0;
    }
    // Load the data sets:
    if (!dataSet->load(xmlFile, Vuforia::STORAGE_APPRESOURCE))
    {
        LOG("Failed to load data set.");
        return 0;
    }
    // Activate the data set:
    if (!objectTracker->activateDataSet(dataSet))
    {
        LOG("Failed to activate data set.");
        return 0;
    }

    LOG("Successfully loaded and activated data set.");
    return 1;
}


JNIEXPORT int JNICALL
Java_com_mavoar_markers_ImageTargets_destroyTrackerData(JNIEnv *, jobject)
{
    LOG("Java_com_mavoar_markers_ImageTargets_destroyTrackerData");

    // Get the object tracker:
    Vuforia::TrackerManager& trackerManager = Vuforia::TrackerManager::getInstance();
    Vuforia::ObjectTracker* objectTracker = static_cast<Vuforia::ObjectTracker*>(
        trackerManager.getTracker(Vuforia::ObjectTracker::getClassType()));
    if (objectTracker == NULL)
    {
        LOG("Failed to destroy the tracking data set because the ObjectTracker has not"
            " been initialized.");
        return 0;
    }

    if (!objectTracker->deactivateDataSet(dataSet))
    {
        LOG("Failed to destroy the tracking data set StonesAndChips because the data set "
            "could not be deactivated.");
        return 0;
    }


    return 1;
}


JNIEXPORT void JNICALL
Java_com_mavoar_markers_ImageTargets_onVuforiaInitializedNative(JNIEnv *, jobject)
{
    // Register the update callback where we handle the data set swap:
    Vuforia::registerCallback(&updateCallback);

    // Comment in to enable tracking of up to 2 targets simultaneously and
    // split the work over multiple frames:
    // Vuforia::setHint(Vuforia::HINT_MAX_SIMULTANEOUS_IMAGE_TARGETS, 2);
}


JNIEXPORT void JNICALL
Java_com_mavoar_renderer_GLRenderer_renderFrame(JNIEnv *env, jobject, jdouble sc, jfloatArray r)
{
    scale = sc;
    rotation = (float*)env->GetFloatArrayElements( r,0);
    // Call renderFrame from SampleAppRenderer which will loop through the rendering primitives
    // views and then it will call renderFrameForView per each of the views available,
    // in this case there is only one view since it is not rendering in stereo mode
    sampleAppRenderer->renderFrame();
}

// This method will be called from SampleAppRenderer per each rendering primitives view
void renderFrameForView(const Vuforia::State *state, Vuforia::Matrix44F& projectionMatrix)
{

    bool hasMarker=false;
    // Explicitly render the Video Background
    sampleAppRenderer->renderVideoBackground();
    glEnable(GL_DEPTH_TEST);

    // We must detect if background reflection is active and adjust the culling direction.
    // If the reflection is active, this means the post matrix has been reflected as well,
    // therefore standard counter clockwise face culling will result in "inside out" models.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    if(Vuforia::Renderer::getInstance().getVideoBackgroundConfig().mReflection == Vuforia::VIDEO_BACKGROUND_REFLECTION_ON)
        glFrontFace(GL_CCW);  //Front camera
    else
        glFrontFace(GL_CCW);   //Back camera


    LOG("NUmber of trackables: %d",state->getNumTrackableResults());

    // Did we find any trackables this frame?
    if(state->getNumTrackableResults() <=1 && mvo && init){
        mvo_processFrame((long)&curr_frame,scale,rotation);
    }

    for(int tIdx = 0; tIdx < state->getNumTrackableResults(); tIdx++) {
        // Get the trackable:
        const Vuforia::TrackableResult *result = state->getTrackableResult(tIdx);
        const Vuforia::Trackable &trackable = result->getTrackable();
        modelViewMatrix =
                Vuforia::Tool::convertPose2GLMatrix(result->getPose());

        if (result->isOfType(Vuforia::DeviceTrackableResult::getClassType())) {
            const Vuforia::DeviceTrackableResult *deviceTrackableResult =
                    static_cast<const Vuforia::DeviceTrackableResult *>(result);
            // base device matrix that can be used for rendering (will need to be inverted), debug
            deviceMatrix = SampleMath::Matrix44FInverse(modelViewMatrix);
            deviceMatrix = SampleMath::Matrix44FTranspose(deviceMatrix);

        } else {
            hasMarker=true;
            if (strcmp(trackable.getName(), currMarker->name) != 0) {

                LOG("New marker %s", trackable.getName());

                currMarker = markers[trackable.getName()];
            }


            markerMatrix = modelViewMatrix;
        }
    }
    Vuforia::Matrix44F modelViewProjection;

    Vuforia::Matrix44F joinedmv;

        if(deviceMatrix.data[0] && markerMatrix.data[0] && useDeviceTracker && !hasMarker){

            SampleUtils::multiplyMatrix(conv.data,deviceMatrix.data,deviceMatrix.data);


            SampleUtils::multiplyMatrix(aux.data,deviceMatrix.data,deviceMatrix.data);

            //LOG("conversion matrix");
            //SampleUtils::printMatrix(conv.data);

            SampleUtils::multiplyMatrix(&deviceMatrix.data[0],
                                    &markerMatrix.data[0] ,
                                   &joinedmv.data[0]);


            //joinedmv=deviceMatrix;
        } else if(hasMarker){
            //SampleUtils::printMatrix(markerMatrix.data);

            joinedmv=markerMatrix;

        }

        SampleUtils::translatePoseMatrix(currMarker->translation[0],
                                       currMarker->translation[1],
                                        currMarker->translation[2],
                                         &joinedmv.data[0]);

        SampleUtils::rotatePoseMatrix(currMarker->rotation[0],
                                        currMarker->rotation[1],
                                        currMarker->rotation[2] ,
                                        currMarker->rotation[3],
                                        &joinedmv.data[0]);


        SampleUtils::scalePoseMatrix(kObjectScale,
                                    kObjectScale,
                                    kObjectScale,
                                    &joinedmv.data[0]);

        SampleUtils::multiplyMatrix(&projectionMatrix.data[0],
                                    &joinedmv.data[0] ,
                                    &modelViewProjection.data[0]);

        glUseProgram(shaderProgramID);


        std::vector<MeshInfo*> modelMeshes= gAssimpObject->getMeshes();
        unsigned int numberOfLoadedMeshes = modelMeshes.size();

        // render all meshes
         for (unsigned int n = 0; n < numberOfLoadedMeshes; ++n) {
            if (modelMeshes[n]->mTextureID) {
                glActiveTexture(GL_TEXTURE0);

                glUniform1i(texSampler2DHandle, 0 );

                glBindTexture( GL_TEXTURE_2D, modelMeshes[n]->mTextureID);

             }
             glVertexAttribPointer(vertexHandle, 3, GL_FLOAT, GL_FALSE, 0,
                                   (const GLvoid*)  (modelMeshes[n]->vertices));
             glVertexAttribPointer(textureCoordHandle, 2, GL_FLOAT, GL_FALSE, 0,
                                  (const GLvoid*) (modelMeshes[n]->texCoords));

             glEnableVertexAttribArray(vertexHandle);
             glEnableVertexAttribArray(textureCoordHandle);
             glUniformMatrix4fv(mvpMatrixHandle, 1, GL_FALSE,
             (GLfloat*)&modelViewProjection.data[0] );

              glDrawElements(GL_TRIANGLES,  modelMeshes[n]->nIndices, GL_UNSIGNED_SHORT,
                            (const GLvoid*)(modelMeshes[n]->indices));


            }

            SampleUtils::checkGlError("ImageTargets renderFrame");

    //
    // }


    glDisable(GL_DEPTH_TEST);
}


void
configureVideoBackground()
{
    // Get the default video mode:
    Vuforia::CameraDevice& cameraDevice = Vuforia::CameraDevice::getInstance();
    Vuforia::VideoMode videoMode = cameraDevice.
                                getVideoMode(Vuforia::CameraDevice::MODE_DEFAULT);


    // Configure the video background
    Vuforia::VideoBackgroundConfig config;
    config.mEnabled = true;
    config.mPosition.data[0] = 0.0f;
    config.mPosition.data[1] = 0.0f;

    if (isActivityInPortraitMode)
    {
        //LOG("configureVideoBackground PORTRAIT");
        config.mSize.data[0] = videoMode.mHeight
                                * (screenHeight / (float)videoMode.mWidth);
        config.mSize.data[1] = screenHeight;

        if(config.mSize.data[0] < screenWidth)
        {
            LOG("Correcting rendering background size to handle missmatch between screen and video aspect ratios.");
            config.mSize.data[0] = screenWidth;
            config.mSize.data[1] = screenWidth *
                              (videoMode.mWidth / (float)videoMode.mHeight);
        }
    }
    else
    {
        //LOG("configureVideoBackground LANDSCAPE");
        config.mSize.data[0] = screenWidth;
        config.mSize.data[1] = videoMode.mHeight
                            * (screenWidth / (float)videoMode.mWidth);

        if(config.mSize.data[1] < screenHeight)
        {
            LOG("Correcting rendering background size to handle missmatch between screen and video aspect ratios.");
            config.mSize.data[0] = screenHeight
                                * (videoMode.mWidth / (float)videoMode.mHeight);
            config.mSize.data[1] = screenHeight;
        }
    }

    LOG("Configure Video Background : Video (%d,%d), Screen (%d,%d), mSize (%d,%d)", videoMode.mWidth, videoMode.mHeight, screenWidth, screenHeight, config.mSize.data[0], config.mSize.data[1]);

    // Set the config:
    Vuforia::Renderer::getInstance().setVideoBackgroundConfig(config);
}


JNIEXPORT void JNICALL
Java_com_mavoar_markers_ImageTargets_initApplicationNative(
                            JNIEnv* env, jobject obj, jint width, jint height)
{
    LOG("Java_com_mavoar_markers_ImageTargets_initApplicationNative");

    // Store screen dimensions
    screenWidth = width;
    screenHeight = height;

    sampleAppRenderer = new SampleAppRenderer();

    // Handle to the activity class:
    jclass activityClass = env->GetObjectClass(obj);

    LOG("Java_com_mavoar_markers_ImageTargets_initApplicationNative finished");
}


JNIEXPORT void JNICALL
Java_com_mavoar_markers_ImageTargets_deinitApplicationNative(
                                                        JNIEnv* env, jobject obj)
{
    LOG("Java_com_mavoar_markers_ImageTargets_deinitApplicationNative");

    useExtendedTracking = false;

    delete sampleAppRenderer;
    sampleAppRenderer = NULL;
}


JNIEXPORT void JNICALL
Java_com_mavoar_markers_ImageTargets_startCamera(JNIEnv *,
                                                                         jobject, jint camera)
{
    LOG("Java_com_mavoar_markers_ImageTargets_startCamera");

    currentCamera = static_cast<Vuforia::CameraDevice::CAMERA_DIRECTION> (camera);

    // Initialize the camera:
    if (!Vuforia::CameraDevice::getInstance().init(currentCamera))
        return;

    // Select the default camera mode:
    if (!Vuforia::CameraDevice::getInstance().selectVideoMode(
                                Vuforia::CameraDevice::MODE_DEFAULT))
        return;

    // Configure the rendering of the video background
    configureVideoBackground();

    // Start the camera:
    if (!Vuforia::CameraDevice::getInstance().start())
        return;

    Vuforia::setFrameFormat(Vuforia::RGB565, true);

    // Uncomment to enable flash
    //if(Vuforia::CameraDevice::getInstance().setFlashTorchMode(true))
    //    LOG("IMAGE TARGETS : enabled torch");

    // Uncomment to enable infinity focus mode, or any other supported focus mode
    // See CameraDevice.h for supported focus modes
    //if(Vuforia::CameraDevice::getInstance().setFocusMode(Vuforia::CameraDevice::FOCUS_MODE_INFINITY))
    //    LOG("IMAGE TARGETS : enabled infinity focus");

    // Start the tracker:
    Vuforia::TrackerManager& trackerManager = Vuforia::TrackerManager::getInstance();
    Vuforia::Tracker* objectTracker = trackerManager.getTracker(Vuforia::ObjectTracker::getClassType());
    if(objectTracker != 0) {
        objectTracker->start();

        objectTracker->start();

    }
}


JNIEXPORT void JNICALL
Java_com_mavoar_markers_ImageTargets_stopCamera(JNIEnv *, jobject)
{
    LOG("Java_com_mavoar_markers_ImageTargets_stopCamera");

    // Stop the tracker:
    Vuforia::TrackerManager& trackerManager = Vuforia::TrackerManager::getInstance();
    Vuforia::Tracker* objectTracker = trackerManager.getTracker(Vuforia::ObjectTracker::getClassType());
    if(objectTracker != 0)
        objectTracker->stop();

    Vuforia::CameraDevice::getInstance().stop();
    Vuforia::CameraDevice::getInstance().deinit();
}


JNIEXPORT void JNICALL
Java_com_mavoar_renderer_GLRenderer_updateRenderingPrimitives(JNIEnv *, jobject)
{
    LOG("Java_com_mavoar_markers_GLRenderer_updateRenderingPrimitives");

    sampleAppRenderer->updateRenderingPrimitives();
}

// ----------------------------------------------------------------------------
// Activates Camera Flash
// ----------------------------------------------------------------------------
JNIEXPORT jboolean JNICALL
Java_com_mavoar_markers_ImageTargets_activateFlash(JNIEnv*, jobject, jboolean flash)
{
    return Vuforia::CameraDevice::getInstance().setFlashTorchMode((flash==JNI_TRUE)) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_mavoar_markers_ImageTargets_autofocus(JNIEnv*, jobject)
{
    return Vuforia::CameraDevice::getInstance().setFocusMode(Vuforia::CameraDevice::FOCUS_MODE_TRIGGERAUTO) ? JNI_TRUE : JNI_FALSE;
}


JNIEXPORT jboolean JNICALL
Java_com_mavoar_markers_ImageTargets_setFocusMode(JNIEnv*, jobject, jint mode)
{
    int focusMode;

    switch ((int)mode)
    {
        case 0:
            focusMode = Vuforia::CameraDevice::FOCUS_MODE_NORMAL;
            break;

        case 1:
            focusMode = Vuforia::CameraDevice::FOCUS_MODE_CONTINUOUSAUTO;
            break;

        case 2:
            focusMode = Vuforia::CameraDevice::FOCUS_MODE_INFINITY;
            break;

        case 3:
            focusMode = Vuforia::CameraDevice::FOCUS_MODE_MACRO;
            break;

        default:
            return JNI_FALSE;
    }

    return Vuforia::CameraDevice::getInstance().setFocusMode(focusMode) ? JNI_TRUE : JNI_FALSE;
}


JNIEXPORT jboolean JNICALL
Java_com_mavoar_markers_ImageTargets_startExtendedTracking(JNIEnv*, jobject)
{
    Vuforia::TrackerManager& trackerManager = Vuforia::TrackerManager::getInstance();
    Vuforia::ObjectTracker* objectTracker = static_cast<Vuforia::ObjectTracker*>(
          trackerManager.getTracker(Vuforia::ObjectTracker::getClassType()));

    Vuforia::DataSet* currentDataSet = objectTracker->getActiveDataSet(0);
    if (objectTracker == 0 || currentDataSet == 0)
        return JNI_FALSE;

    for (int tIdx = 0; tIdx < currentDataSet->getNumTrackables(); tIdx++)
    {
        Vuforia::Trackable* trackable = currentDataSet->getTrackable(tIdx);
        if(!trackable->startExtendedTracking())
            return JNI_FALSE;
    }

    useExtendedTracking = true;
    //trackerManager = Vuforia::TrackerManager::getInstance();
    //objectTracker = trackerManager.getTracker(Vuforia::ObjectTracker::getClassType());


    objectTracker->persistExtendedTracking(true);

    return JNI_TRUE;
}


JNIEXPORT jboolean JNICALL
Java_com_mavoar_markers_ImageTargets_stopExtendedTracking(JNIEnv*, jobject)
{
    Vuforia::TrackerManager& trackerManager = Vuforia::TrackerManager::getInstance();
    Vuforia::ObjectTracker* objectTracker = static_cast<Vuforia::ObjectTracker*>(
          trackerManager.getTracker(Vuforia::ObjectTracker::getClassType()));

    Vuforia::DataSet* currentDataSet = objectTracker->getActiveDataSet(0);
    if (objectTracker == 0 || currentDataSet == 0)
        return JNI_FALSE;

    for (int tIdx = 0; tIdx < currentDataSet->getNumTrackables(); tIdx++)
    {
        Vuforia::Trackable* trackable = currentDataSet->getTrackable(tIdx);
        if(!trackable->stopExtendedTracking())
            return JNI_FALSE;
    }

    useExtendedTracking = false;
    return JNI_TRUE;
}


JNIEXPORT void JNICALL
Java_com_mavoar_renderer_GLRenderer_initRendering(
                                                    JNIEnv* env, jobject obj)
{
    LOG("Java_com_mavoar_markers_GLRenderer_initRendering");

    // Define clear color
    glClearColor(0.0f, 0.0f, 0.0f, Vuforia::requiresAlpha() ? 0.0f : 1.0f);

    std::vector<cv::Mat> textures= gAssimpObject->getTextures();

    std::vector<MeshInfo*> modelMeshes= gAssimpObject->getMeshes();
    unsigned int numberOfLoadedMeshes = modelMeshes.size();

    // Now generate the OpenGL texture objects and add settings
    for (int i = 0; i < numberOfLoadedMeshes; ++i)
    {
        if(textures.size()>i){
        MeshInfo* meshinfo=modelMeshes.at(i);

        cv::Mat texture= textures.at(i);

        glGenTextures(1, &(meshinfo->mTextureID));
        glBindTexture(GL_TEXTURE_2D, meshinfo->mTextureID);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture.cols,
                texture.rows, 0, GL_RGB, GL_UNSIGNED_BYTE,
                  texture.data );

        MyLOGI("Cols:%d; Rows:%d;", texture.cols,texture.rows);
        }
        else{break;}

    }



    shaderProgramID     = SampleUtils::createProgramFromBuffer(cubeMeshVertexShader,
                                                            cubeFragmentShader);

    vertexHandle        = glGetAttribLocation(shaderProgramID,
                                                "vertexPosition");
    textureCoordHandle  = glGetAttribLocation(shaderProgramID,
                                                "vertexTexCoord");
    mvpMatrixHandle     = glGetUniformLocation(shaderProgramID,
                                                "modelViewProjectionMatrix");
    texSampler2DHandle  = glGetUniformLocation(shaderProgramID,
                                                "texSampler2D");
    sampleAppRenderer->initRendering();
}


JNIEXPORT void JNICALL
Java_com_mavoar_renderer_GLRenderer_updateRendering(
                        JNIEnv* env, jobject obj, jint width, jint height)
{
    LOG("Java_com_mavoar_markers_GLRenderer_updateRendering");

    // Update screen dimensions
    screenWidth = width;
    screenHeight = height;

    // Reconfigure the video background
    configureVideoBackground();
}


#ifdef __cplusplus
}
#endif
