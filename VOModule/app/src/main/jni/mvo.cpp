#include "mvo.h"
#include "mvo_features.h"

#include <jni.h>
#include <string>
#include <android/log.h>

#include "opencv2/opencv.hpp"


using namespace cv;

Stage stage;
struct Frames frames;
struct Camera camera;
struct Matrices matrices;

Mat prev_frame;
Mat curr_frame;
vector<Point2f> prev_features;

Mat R_f, t_f;

//double focal = 718.8560;
//cv::Point2d pp(607.1928, 185.2157);
Mat E, R, t, mask;


extern "C"
JNIEXPORT void JNICALL
Java_com_mavoar_vomodule_vomodule_VisualOdometry_init(
        JNIEnv *env,
        jobject /* this */,jfloat focalLength,jfloat ppx,jfloat ppy) {
    LOGD("init");
    stage=WAITING_FIRST_FRAME;
    camera.focal = (float) focalLength;
    camera.pp.x= (float) ppx;
    camera.pp.y= (float) ppy;
}



void initialFix(){
    try
        {
        vector<Point2f> points1, points2; //vectors to store the coordinates of the feature points
        featureDetection(prev_frame, points1); //detect features in img_1

        if(points1.size()>0){
            vector < uchar > status;
            featureTracking(prev_frame, curr_frame, points1, points2, status); //track those features to img_2

            if(points2.size()>0){

                E = findEssentialMat(points2, points1, camera.focal, camera.pp, RANSAC, 0.999, 1.0, mask);
                recoverPose(E, points2, points1, R, t, camera.focal, camera.pp, mask);

                prev_features = points2;

                R_f = R.clone();
                t_f = t.clone();

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
        vector < Point2f > currFeatures;
        vector < uchar > status;
        featureTracking(prev_frame, curr_frame, prev_features, currFeatures, status);

        if(currFeatures.size()>0){

            E = findEssentialMat(currFeatures, prev_features, camera.focal, camera.pp, RANSAC, 0.999,
                    1.0, mask);
            recoverPose(E, currFeatures, prev_features, R, t, camera.focal, camera.pp, mask);

            //Mat prevPts(2, prev_features.size(), CV_64F), currPts(2, currFeatures.size(),
            //        CV_64F);

            /*for (int i = 0; i < prev_features.size(); i++) { //this (x,y) combination makes sense as observed from the source code of triangulatePoints on GitHub
                prevPts.at<double>(0, i) = prev_features.at(i).x;
                prevPts.at<double>(1, i) = prev_features.at(i).y;

                currPts.at<double>(0, i) = currFeatures.at(i).x;
                currPts.at<double>(1, i) = currFeatures.at(i).y;
            }*/

            //if ((scale > 0.1) /*&& (t.at<double>(2) > t.at<double>(0)*/)
                    //&& (t.at<double>(2) > t.at<double>(1))) {

            /*float scale=1.0;
            t_f = t_f + scale * (R_f * t);
            R_f = R * R_f;

            //}*/
            prev_features = currFeatures;
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

extern "C"
JNIEXPORT void JNICALL
Java_com_mavoar_vomodule_vomodule_VisualOdometry_processFrame(
        JNIEnv *env,
        jobject /* this */,
        jlong matAddrGray) {
    LOGD("Received Frame");

    switch(stage){
        case WAITING_FIRST_FRAME:
            LOGD("First Frame");
            stage=WAITING_SECOND_FRAME;

            prev_frame = ((Mat*)matAddrGray)->clone();
            break;
        case WAITING_SECOND_FRAME:
            LOGD("Second Frame");

            curr_frame = (*(Mat*)matAddrGray);

            initialFix();
            break;
        case WAITING_FRAME:
            LOGD("Normal Frame");
            prev_frame = curr_frame.clone();
            curr_frame = *(Mat *) matAddrGray;
            mvoDetectAndTrack();
            break;
    }
}


