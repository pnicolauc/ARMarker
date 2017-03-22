//
// Created by al on 15-03-2017.
//

#include "opencv2/video/tracking.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/calib3d/calib3d.hpp"

#ifndef VOMODULE_MVO_FEATURES_H
#define VOMODULE_MVO_FEATURES_H

using namespace std;
using namespace cv;

void featureTracking(Mat img_1, Mat img_2, vector<Point2f>& points1, vector<Point2f>& points2, vector<uchar>& status);

void featureDetection(Mat img_1, vector<Point2f>& points1);

#endif //VOMODULE_MVO_FEATURES_H
