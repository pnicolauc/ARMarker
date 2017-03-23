#include "mvo.h"
#include "mvo_features.h"

#include <jni.h>
#include <string>
#include <android/log.h>

#include "opencv2/opencv.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;


extern "C"
JNIEXPORT jstring JNICALL
Java_com_mavoar_vo_VisualOdometry_processFrame(
        JNIEnv *env,
        jobject /* this */,
        jlong matAddrGray,
        jdouble scale,
        jfloatArray rotation) {


    return returnString;
}


