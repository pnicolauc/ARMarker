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


#ifdef __cplusplus
extern "C"
{
#endif

struct GPUObjs gpuObjs;
struct ScreenParams screenParams;
struct FileSystem fileSystem;
struct DatasetData datasets;
struct SensorsData sensorData;
struct CameraData cameraData;
struct RotDeviceTracker rotDeviceTracker;
struct ExtTracking extTracking;
struct TrackerParams trackerParams;
struct UserDefTargets userDefTargets;
struct Trajectory trajectory;
struct AuxMat auxMat;

ThreeDModel* model;
Vuforia::Matrix44F modelViewProjection;

int ang=0;

MyJNIHelper * gHelperObject=NULL;

using namespace cv;

Vuforia::Vec3F cameraPos(0.0,0.0,0.0);
Vuforia::Vec3F cameraUp;
Vuforia::Vec3F cameraDir;


// Object to receive update callbacks from Vuforia SDK
class ImageTargets_UpdateCallback : public Vuforia::UpdateCallback
{
    virtual void Vuforia_onUpdate(Vuforia::State& state)
    {
        Vuforia::TrackerManager& trackerManager = Vuforia::TrackerManager::getInstance();
        Vuforia::ObjectTracker* objectTracker = static_cast<Vuforia::ObjectTracker*>(
                        trackerManager.getTracker(Vuforia::ObjectTracker::getClassType()));

       
        if (datasets.switchDataSetAsap)
        {
            datasets.switchDataSetAsap = false;
            // Get the object tracker:

            if (objectTracker == 0)
            {
                LOG("Failed to switch data set.");
                return;
            }
            objectTracker->activateDataSet(datasets.targets);
            if(extTracking.useExtendedTracking)
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
Java_com_mavoar_renderer_GLRenderer_getMat(JNIEnv *, jobject, jlong mat)
{
    Mat* aux = (Mat*) mat;
    aux->create(cameraData.curr_frame.rows, cameraData.curr_frame.cols, cameraData.curr_frame.type());
    memcpy(aux->data, cameraData.curr_frame.data, aux->step * aux->rows);
}

JNIEXPORT void JNICALL
Java_com_mavoar_markers_ImageTargets_setActivityPortraitMode(JNIEnv *, jobject, jboolean isPortrait)
{
    screenParams.isActivityInPortraitMode = isPortrait;
}

JNIEXPORT void JNICALL
Java_com_mavoar_markers_ImageTargets_switchDatasetAsap(JNIEnv* env, jobject, jstring dataset)
{
    datasets.selectedDataset = 0;
    datasets.switchDataSetAsap = true;
}


JNIEXPORT int JNICALL
Java_com_mavoar_markers_ImageTargets_initTracker(
JNIEnv *env, jobject instance,jobject assetManager,jstring pathToInternalDir,
jstring obj,jstring mtl,jstring xml,jstring folder,jfloat sca, jint markerNum,
 jobjectArray markerNames,jfloatArray markerRot,jfloatArray markerTra, jfloatArray markerSca,jboolean jmvo)
{

    trackerParams.sensorRotation.data[3]=0;
    trackerParams.sensorRotation.data[7]=0;
    trackerParams.sensorRotation.data[11]=0;
    trackerParams.sensorRotation.data[12]=0;
    trackerParams.sensorRotation.data[13]=0;
    trackerParams.sensorRotation.data[14]=0;
    trackerParams.sensorRotation.data[15]=1;

    SampleUtils::setRotationMatrix(-90,0,0,1,auxMat.conv.data);
    SampleUtils::setRotationMatrix(180,0,1,0,auxMat.aux.data);

    LOG("Java_com_mavoar_markers_ImageTargets_initTracker");

    datasets.kObjectScale=(float)sca;
    jfloat* rots = env->GetFloatArrayElements( markerRot,0);
    jfloat* trans = env->GetFloatArrayElements( markerTra,0);
    jfloat* scals = env->GetFloatArrayElements( markerSca,0);

    for(unsigned int a=0;a<markerNum;a++){
        Marker* marker= new Marker;

        jstring string = (jstring) (env->GetObjectArrayElement(markerNames, a));
        const char* na= env->GetStringUTFChars(string, NULL );

        marker->name=na;

        marker->rotation[0]=(float)rots[(a*4)];
        marker->rotation[1]=(float)rots[(a*4)+1];
        marker->rotation[2]=(float)rots[(a*4)+2];
        marker->rotation[3]=(float)rots[(a*4)+3];

        marker->translation[0]=(float)trans[(a*3)];
        marker->translation[1]=(float)trans[(a*3)+1];
        marker->translation[2]=(float)trans[(a*3)+2];

        marker->scale[0]=(float)scals[(a*3)];
        marker->scale[1]=(float)scals[(a*3)+1];
        marker->scale[2]=(float)scals[(a*3)+2];

        //std::string str(marker.name);
        datasets.markers[marker->name] = marker;

        datasets.currMarker = marker;
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
    gpuObjs.gAssimpObject = new ModelAssimp();

    gpuObjs.gAssimpObject->PerformGLInits(objCPP,mtlCPP,folderCPP);

    model = gpuObjs.gAssimpObject->getThreeDModel();
    // Initialize the object tracker:
    Vuforia::TrackerManager& trackerManager = Vuforia::TrackerManager::getInstance();
    Vuforia::Tracker* tracker = trackerManager.initTracker(Vuforia::ObjectTracker::getClassType());
    
    if (tracker == NULL)
    {
        LOG("Failed to initialize ObjectTracker.");
        return 0;
    }

    LOG("Successfully initialized ObjectTracker.");

    Vuforia::ObjectTracker* imageTracker = static_cast<Vuforia::ObjectTracker*>(
                        trackerManager.getTracker(Vuforia::ObjectTracker::getClassType()));
    userDefTargets.builder= imageTracker->getImageTargetBuilder();
    userDefTargets.builder->startScan();
    SampleUtils::zeroesFloatVector3(userDefTargets.translationUDT);
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

    rotDeviceTracker.deviceTracker = static_cast<Vuforia::RotationalDeviceTracker*>(
    trackerManager.initTracker(Vuforia:: RotationalDeviceTracker::getClassType()));
    // activate pose prediction
    rotDeviceTracker.deviceTracker->setPosePrediction(false);

    // activate model correction: default handheld model
    rotDeviceTracker.deviceTracker->setModelCorrection((Vuforia::TransformModel*)rotDeviceTracker.deviceTracker->getDefaultHandheldModel());
    // start the tracker
    rotDeviceTracker.deviceTracker->start();

    // Create the data set:
    datasets.targets=objectTracker->createDataSet();
    datasets.udts=objectTracker->createDataSet();
    if (datasets.targets == 0)
    {
        LOG("Failed to create a new tracking data.");
        return 0;
    }
    // Load the data sets:
    if (!datasets.targets->load(xmlFile, Vuforia::STORAGE_ABSOLUTE))
    {
        LOG("Failed to load data set.");
        return 0;
    }
    // Activate the data set:
    if (!objectTracker->activateDataSet(datasets.targets))
    {
        LOG("Failed to activate data set.");
        return 0;
    }
    if (!objectTracker->activateDataSet(datasets.udts))
    {
        LOG("Failed to activate data set.");
        return 0;
    }

    LOG("Successfully loaded and activated data sets.");
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

    if (!objectTracker->deactivateDataSet(datasets.targets))
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
    Vuforia::setHint(Vuforia::HINT_MAX_SIMULTANEOUS_IMAGE_TARGETS, 2);
}


JNIEXPORT void JNICALL
Java_com_mavoar_renderer_GLRenderer_renderFrame(JNIEnv *env, jobject, jdouble sc, jfloat xx,jfloat yy,jfloat zz,
jfloat x1,jfloat x2,jfloat x3,
jfloat y1,jfloat y2,jfloat y3,
jfloat z1,jfloat z2,jfloat z3)
{
    try{
        sensorData.scale = sc;
        SampleUtils::setMatrix(x1,x2,x3,y1,y2,y3,z1,z2,z3,sensorData.rotation);   
        SampleUtils::setRotation33to44(x1,x2,x3,y1,y2,y3,z1,z2,z3,trackerParams.sensorRotation.data);

        SampleUtils::multiplyMatrix(auxMat.conv.data,
                                        trackerParams.sensorRotation.data ,
                                        trackerParams.sensorRotation.data);
        SampleUtils::multiplyMatrix(auxMat.aux.data,
                                                trackerParams.sensorRotation.data ,
                                                trackerParams.sensorRotation.data);
        // Call renderFrame from SampleAppRenderer which will loop through the rendering primitives
        // views and then it will call renderFrameForView per each of the views available,
        // in this case there is only one view since it is not rendering in stereo mode
        gpuObjs.sampleAppRenderer->renderFrame();
    }
    catch(int a){

    }
}

void drawNode(const Node *node, Vuforia::Matrix44F objectMatrix)
{
    std::vector<MeshInfo*> modelMeshes= gpuObjs.gAssimpObject->getMeshes();
    unsigned int numberOfLoadedMeshes = modelMeshes.size();
    for(int imm = 0; imm<node->meshes.size(); ++imm)
    {
        if (modelMeshes[0]->mTextureID) {    

        glActiveTexture(GL_TEXTURE0);

        glUniform1i(gpuObjs.texSampler2DHandle, 0 );

        glBindTexture( GL_TEXTURE_2D, node->meshes[imm]->material->mTextureID);

        }
        glVertexAttribPointer(gpuObjs.vertexHandle, 3, GL_FLOAT, GL_FALSE, 0,
                            (const GLvoid*) node->meshes[imm]->vertex /*(modelMeshes[0]->vertices)*/);
        glVertexAttribPointer(gpuObjs.textureCoordHandle, 2, GL_FLOAT, GL_FALSE, 0,
                            (const GLvoid*) node->meshes[imm]->uv);

        glEnableVertexAttribArray(gpuObjs.vertexHandle);
        glEnableVertexAttribArray(gpuObjs.textureCoordHandle);
        glUniformMatrix4fv(gpuObjs.mvpMatrixHandle, 1, GL_FALSE,
        (GLfloat*)&modelViewProjection.data[0] );

        glDrawElements(GL_TRIANGLES,   node->meshes[imm]->indexCount, GL_UNSIGNED_SHORT,
                    (const GLvoid*)(node->meshes[imm]->indices));

    }

    // Recursively draw this nodes children nodes
    for(int inn = 0; inn<node->nodes.size(); ++inn)
        drawNode(&node->nodes[inn], objectMatrix);
}
// This method will be called from SampleAppRenderer per each rendering primitives view
void renderFrameForView(const Vuforia::State *state, Vuforia::Matrix44F& projectionMatrix)
{
    bool hasMarker=false;
    Vuforia::Matrix44F joinedmv;
     // Did we find any trackables this frame?

    Vuforia::Matrix44F inverseModelView;
    // Explicitly render the Video Background
    gpuObjs.sampleAppRenderer->renderVideoBackground();
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

    for(int tIdx = 0; tIdx < state->getNumTrackableResults(); tIdx++) {
        // Get the trackable:
        const Vuforia::TrackableResult *result = state->getTrackableResult(tIdx);
        const Vuforia::Trackable &trackable = result->getTrackable();
        trackerParams.modelViewMatrix =
                Vuforia::Tool::convertPose2GLMatrix(result->getPose());

        if (result->isOfType(Vuforia::DeviceTrackableResult::getClassType())) {
            const Vuforia::DeviceTrackableResult *deviceTrackableResult =
                    static_cast<const Vuforia::DeviceTrackableResult *>(result);
            // base device matrix that can be used for rendering (will need to be inverted), debug
            //deviceMatrix = modelViewMatrix;
            rotDeviceTracker.deviceMatrix = SampleMath::Matrix44FInverse(trackerParams.modelViewMatrix);
            //deviceMatrix = SampleMath::Matrix44FTranspose(modelViewMatrix);

        } else if(datasets.markers.find(trackable.getName())!=datasets.markers.end()) {

            LOG("trackable: %s",trackable.getName());
            bool istrNewMarker=strcmp(trackable.getName(), datasets.currMarker->name) != 0;            
            trackerParams.markerMatrix = trackerParams.modelViewMatrix;

            hasMarker=true;

            if (istrNewMarker) {
                //mvo_reset();
                
                LOG("New marker %s", trackable.getName());
                datasets.currMarker = datasets.markers[trackable.getName()];
                inverseModelView=SampleMath::Matrix44FTranspose(SampleMath::Matrix44FInverse(trackerParams.markerMatrix));
                // pull the camera position and look at vectors from this matrix
                cameraPos = Vuforia::Vec3F(inverseModelView.data[12], inverseModelView.data[13], inverseModelView.data[14]);
                

            }
        }
    }
    
    
    if(!hasMarker){
        if(trackerParams.lastMarker ){
            Vuforia::Matrix44F inverseSensor= SampleMath::Matrix44FTranspose(trackerParams.lastMat);
            SampleUtils::multiplyMatrix(trackerParams.sensorRotation.data, 
            inverseSensor.data,trackerParams.resMatrix.data);
            joinedmv=trackerParams.markerMatrix;

            SampleUtils::multiplyMatrix(trackerParams.resMatrix.data,
                                        &joinedmv.data[0],
                                        &joinedmv.data[0]);
        }
        
    } else{
        trackerParams.lastMat=trackerParams.sensorRotation;
        trackerParams.lastMarker=hasMarker;
        trackerParams.noTrackerAvailable=false;
        joinedmv=trackerParams.markerMatrix;        
    }

    inverseModelView=SampleMath::Matrix44FTranspose(SampleMath::Matrix44FInverse(joinedmv));
    // pull the camera position and look at vectors from this matrix
    Vuforia::Vec3F camPosNew(inverseModelView.data[12], inverseModelView.data[13], inverseModelView.data[14]);
    
    SampleUtils::rotatePoseMatrix(
                                    180-((int)datasets.currMarker->rotation[0]),
                                    datasets.currMarker->rotation[1],
                                    datasets.currMarker->rotation[2] ,
                                    datasets.currMarker->rotation[3],
                                    &joinedmv.data[0]);


    SampleUtils::translatePoseMatrix(datasets.currMarker->translation[0],
                                      datasets.currMarker->translation[1],
                                      datasets.currMarker->translation[2],
                                        &joinedmv.data[0]);


    SampleUtils::multiplyMatrix(&projectionMatrix.data[0],
                                &joinedmv.data[0] ,
                                &modelViewProjection.data[0]);


    // render all meshes
    drawNode(model->node , Vuforia::Matrix44F());

    SampleUtils::checkGlError("ImageTargets renderFrame");
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

    if (screenParams.isActivityInPortraitMode)
    {
        //LOG("configureVideoBackground PORTRAIT");
        config.mSize.data[0] = videoMode.mHeight
                                * (screenParams.screenHeight / (float)videoMode.mWidth);
        config.mSize.data[1] = screenParams.screenHeight;

        if(config.mSize.data[0] < screenParams.screenWidth)
        {
            LOG("Correcting rendering background size to handle missmatch between screen and video aspect ratios.");
            config.mSize.data[0] = screenParams.screenWidth;
            config.mSize.data[1] = screenParams.screenWidth *
                              (videoMode.mWidth / (float)videoMode.mHeight);
        }
    }
    else
    {
        //LOG("configureVideoBackground LANDSCAPE");
        config.mSize.data[0] = screenParams.screenWidth;
        config.mSize.data[1] = videoMode.mHeight
                            * (screenParams.screenWidth / (float)videoMode.mWidth);

        if(config.mSize.data[1] < screenParams.screenHeight)
        {
            LOG("Correcting rendering background size to handle missmatch between screen and video aspect ratios.");
            config.mSize.data[0] = screenParams.screenHeight
                                * (videoMode.mWidth / (float)videoMode.mHeight);
            config.mSize.data[1] = screenParams.screenHeight;
        }
    }

    LOG("Configure Video Background : Video (%d,%d), Screen (%d,%d), mSize (%d,%d)", videoMode.mWidth, videoMode.mHeight, screenParams.screenWidth, screenParams.screenHeight, config.mSize.data[0], config.mSize.data[1]);

    // Set the config:
    Vuforia::Renderer::getInstance().setVideoBackgroundConfig(config);
}


JNIEXPORT void JNICALL
Java_com_mavoar_markers_ImageTargets_initApplicationNative(
                            JNIEnv* env, jobject obj, jint width, jint height)
{
    LOG("Java_com_mavoar_markers_ImageTargets_initApplicationNative");

    // Store screen dimensions
    screenParams.screenWidth = width;
    screenParams.screenHeight = height;

    gpuObjs.sampleAppRenderer = new SampleAppRenderer();

    // Handle to the activity class:
    jclass activityClass = env->GetObjectClass(obj);

    LOG("Java_com_mavoar_markers_ImageTargets_initApplicationNative finished");
}

JNIEXPORT void JNICALL
Java_com_mavoar_markers_ImageTargets_deinitApplicationNative(
                                                        JNIEnv* env, jobject obj)
{
    LOG("Java_com_mavoar_markers_ImageTargets_deinitApplicationNative");

    extTracking.useExtendedTracking = false;

    delete gpuObjs.sampleAppRenderer;
    gpuObjs.sampleAppRenderer = NULL;
}

JNIEXPORT void JNICALL
Java_com_mavoar_markers_ImageTargets_saveTrajectory(
                                                        JNIEnv* env, jobject obj)
{
    FILE* file = fopen("/sdcard/mavoar_traj.txt","w+");

    int vecSize= trajectory.vec.size();

    LOG("Java_com_mavoar_markers_ImageTargets_saveTrajectory %d points",vecSize);

    std::string str;

    int ind=1;
    for(float i : trajectory.vec){
        std::stringstream ss;
        ss << i;
        str.append(ss.str());
        str.append(" ");
        if(ind%3==0)
            str.append("\n");

        ind++;
    }

    if (file != NULL)
    {
        fputs(str.c_str(), file);
        fflush(file);
        fclose(file);
    }
}

JNIEXPORT void JNICALL
Java_com_mavoar_markers_ImageTargets_startCamera(JNIEnv *,
                                                                         jobject, jint camera)
{
    LOG("Java_com_mavoar_markers_ImageTargets_startCamera");

    cameraData.currentCamera = static_cast<Vuforia::CameraDevice::CAMERA_DIRECTION> (camera);

    // Initialize the camera:
    if (!Vuforia::CameraDevice::getInstance().init(cameraData.currentCamera))
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

    Vuforia::setFrameFormat(Vuforia::GRAYSCALE, true);

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

    gpuObjs.sampleAppRenderer->updateRenderingPrimitives();
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

    extTracking.useExtendedTracking = true;
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

    extTracking.useExtendedTracking = false;
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_mavoar_renderer_GLRenderer_initRendering(
                                                    JNIEnv* env, jobject obj)
{
    LOG("Java_com_mavoar_markers_GLRenderer_initRendering");

    // Define clear color
    glClearColor(0.0f, 0.0f, 0.0f, Vuforia::requiresAlpha() ? 0.0f : 1.0f);

    //std::vector<cv::Mat> textures= gpuObjs.gAssimpObject->getTextures();

    //std::vector<MeshInfo*> modelMeshes= gpuObjs.gAssimpObject->getMeshes();
    //unsigned int numberOfLoadedMeshes = modelMeshes.size();

    // Now generate the OpenGL texture objects and add settings
    for (int i = 0; i < model->m_materials.size(); ++i)
    {
       

        cv::Mat texture= model->m_materials[i]->cvTexture;

        glGenTextures(1, &(model->m_materials[i]->mTextureID));
        glBindTexture(GL_TEXTURE_2D, model->m_materials[i]->mTextureID);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture.cols,
                texture.rows, 0, GL_RGB, GL_UNSIGNED_BYTE,
                  texture.data );

        MyLOGI("Cols:%d; Rows:%d;", texture.cols,texture.rows);

    }
    gpuObjs.shaderProgramID     = SampleUtils::createProgramFromBuffer(cubeMeshVertexShader,
                                                            cubeFragmentShader);
    gpuObjs.vertexHandle        = glGetAttribLocation(gpuObjs.shaderProgramID,
                                                "vertexPosition");
    gpuObjs.textureCoordHandle  = glGetAttribLocation(gpuObjs.shaderProgramID,
                                                "vertexTexCoord");
    gpuObjs.mvpMatrixHandle     = glGetUniformLocation(gpuObjs.shaderProgramID,
                                                "modelViewProjectionMatrix");
    gpuObjs.texSampler2DHandle  = glGetUniformLocation(gpuObjs.shaderProgramID,
                                                "texSampler2D");
    gpuObjs.sampleAppRenderer->initRendering();
}


JNIEXPORT void JNICALL
Java_com_mavoar_renderer_GLRenderer_updateRendering(
                        JNIEnv* env, jobject obj, jint width, jint height)
{
    LOG("Java_com_mavoar_markers_GLRenderer_updateRendering");
    // Update screen dimensions
    screenParams.screenWidth = width;
    screenParams.screenHeight = height;

    // Reconfigure the video background
    configureVideoBackground();
}

#ifdef __cplusplus
}
#endif
