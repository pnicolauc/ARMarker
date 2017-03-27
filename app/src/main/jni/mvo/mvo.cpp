#include "mvo.h"
#include "mvo_features.h"

#include <jni.h>
#include <string>
#include <android/log.h>

#include "opencv2/opencv.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;

Stage stage;
struct Frames frames;
struct Camera camera;
struct Matrices matrices;
struct Sensors sensors;
jstring returnString;

JNIEnv *envgl;

void mvoInit(float focalLength,float ppx,float ppy){
    LOGD("mvo init");
    stage=WAITING_FIRST_FRAME;
    camera.focal = (float) focalLength;
    camera.pp.x= (float) ppx;
    camera.pp.y= (float) ppy;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_mavoar_activities_VOModule_init(
        JNIEnv *env,
        jobject /* this */,jfloat focalLength,jfloat ppx,jfloat ppy) {
    mvoInit(focalLength,ppx,ppy);
}


void setRotationFromSensor(){
    matrices.total_rotation.at<double>(0)= sensors.rotation[0];
    matrices.total_rotation.at<double>(1)= sensors.rotation[1];
    matrices.total_rotation.at<double>(2)= sensors.rotation[2];
}

void initialFix(){
    try
        {
        vector<Point2f> points1, points2; //vectors to store the coordinates of the feature points
        featureDetection(frames.prev_frame, points1); //detect features in img_1

        if(points1.size()>0){
            vector < uchar > status;
            featureTracking(frames.prev_frame, frames.curr_frame, points1, points2, status); //track those features to img_2

            if(points2.size()>0){

                matrices.essential = findEssentialMat(points2, points1, camera.focal, camera.pp, RANSAC, 0.999, 1.0, matrices.mask);
                recoverPose(matrices.essential, points2, points1, matrices.rotation, matrices.translation, camera.focal, camera.pp, matrices.mask);

                frames.prev_features = points2;


                if(matrices.total_translation.size().width ==0){
                    matrices.total_rotation = matrices.rotation.clone();
                    setRotationFromSensor();

                    matrices.total_translation = (matrices.translation.clone()* sensors.scale);
                }else{
                    matrices.total_translation = matrices.total_translation + sensors.scale * (matrices.total_rotation * matrices.translation);
                }


                stage=WAITING_FRAME;

            }
        }

        if(points2.size()==0 | points1.size()==0){
            stage=WAITING_FIRST_FRAME;
        }
    }
    catch( cv::Exception& e )
    {
        LOGD("Exception caught. resetting stages.");
        stage=WAITING_FIRST_FRAME;
    }
}

void mvoDetectAndTrack(){
    try{

        setRotationFromSensor();
        vector < uchar > status;
        featureTracking(frames.prev_frame, frames.curr_frame, frames.prev_features, frames.curr_features, status);

        if(frames.curr_features.size()>0){

            matrices.essential = findEssentialMat(frames.curr_features, frames.prev_features, camera.focal, camera.pp, RANSAC, 0.999,
                    1.0, matrices.mask);
            recoverPose(matrices.essential, frames.curr_features, frames.prev_features, matrices.rotation, matrices.translation, camera.focal, camera.pp, matrices.mask);

            if ((sensors.scale > 0.05) && (matrices.translation.at<double>(2) > matrices.translation.at<double>(0))
                    && (matrices.translation.at<double>(2) > matrices.translation.at<double>(1))) {


                matrices.total_translation = matrices.total_translation + sensors.scale * (matrices.total_rotation * matrices.translation);

            }
            frames.prev_features = frames.curr_features;
        }
        else{
            stage=WAITING_FIRST_FRAME;
        }
    }
    catch( cv::Exception& e )
    {
        LOGD("Exception caught. resetting stages.");
        stage=WAITING_FIRST_FRAME;
    }
}

jstring returnMessage(JNIEnv *env,char str[]){

 char buffer [100];
 sprintf (buffer, "%s - %f %f %f",str,matrices.total_translation.at<double>(0),
 matrices.total_translation.at<double>(1),matrices.total_translation.at<double>(2));

 //sprintf (buffer, "%s - %f %f %f",str,matrices.total_rotation.at<double>(0),
 //matrices.total_rotation.at<double>(1),matrices.total_rotation.at<double>(2));

 LOGD("%s",buffer);

 return env->NewStringUTF(buffer);
}


void mvo_processFrame(jlong matAddrGray,
        jdouble scale,
float* rotation) {
LOGD("Received Frame");

sensors.scale= (double)scale;
sensors.rotation= rotation;

    switch(stage){
        case WAITING_FIRST_FRAME:
                LOGD("First Frame");
            stage=WAITING_SECOND_FRAME;
            frames.prev_frame = ((Mat*)matAddrGray)->clone();
            if(envgl)
                returnString = envgl->NewStringUTF("Fixing");

            break;
        case WAITING_SECOND_FRAME:
            LOGD("Second Frame");
            frames.curr_frame = (*(Mat*)matAddrGray);
            initialFix();
            if(envgl)
                returnString = returnMessage(envgl,"Fixing");
        break;
        case WAITING_FRAME:
                LOGD("Normal Frame");
        frames.prev_frame = frames.curr_frame.clone();
        frames.curr_frame = *(Mat *) matAddrGray;
        mvoDetectAndTrack();
        if(envgl)
            returnString = returnMessage(envgl,"Tracking");


        break;
    }
    LOGD("Received Frame");

}


extern "C"
JNIEXPORT jstring JNICALL
Java_com_mavoar_activities_VOModule_processFrame(
        JNIEnv *env,
        jobject /* this */,
        jlong matAddrGray,
        jdouble scale,
        jfloatArray rotation) {
    LOGD("Received Frame");
    envgl=env;
    mvo_processFrame(matAddrGray, scale,(float*)env->GetFloatArrayElements( rotation,0));


    /*jstring returnString;
    switch(stage){
        case WAITING_FIRST_FRAME:
            LOGD("First Frame");
            stage=WAITING_SECOND_FRAME;

            frames.prev_frame = ((Mat*)matAddrGray)->clone();
            returnString = env->NewStringUTF("Fixing");

            break;
        case WAITING_SECOND_FRAME:
            LOGD("Second Frame");
            frames.curr_frame = (*(Mat*)matAddrGray);
            initialFix();
            returnString = returnMessage(env,"Fixing");

            break;
        case WAITING_FRAME:
            LOGD("Normal Frame");
            frames.prev_frame = frames.curr_frame.clone();
            frames.curr_frame = *(Mat *) matAddrGray;
            mvoDetectAndTrack();
            returnString = returnMessage(env,"Tracking");

            break;
        }*/
        return returnString;
}

