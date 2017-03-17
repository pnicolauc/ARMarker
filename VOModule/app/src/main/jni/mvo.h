//
// Created by al on 15-03-2017.
//
#include "opencv2/opencv.hpp"
using namespace cv;
using namespace std;

#ifndef VOMODULE_MVO_H
#define VOMODULE_MVO_H

#define  LOG_TAG    "VOMODULE"

#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

enum Stage { WAITING_FIRST_FRAME, WAITING_SECOND_FRAME, WAITING_FRAME };
#define MIN_NUM_FEAT 2000


struct Frames {
   Mat prev_frame;
   Mat curr_frame;
   vector<Point2f> prev_features;
};

struct Camera {
   double focal;
   cv::Point2d pp;
};

struct Matrices {
    Mat essential;
    Mat rotation;
    Mat translation;
    Mat mask;
};

#endif //VOMODULE_MVO_H
