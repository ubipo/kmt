
#pragma once

// Internal
#include "KinectWrapper.h"

// OpenCV
#include <opencv2/opencv.hpp>
using namespace cv;

class Kmt;

// Typedef
using tByte = unsigned char; // Random prefix t to avoid conflict
using tWord = unsigned short;
using matSource = Mat(Kmt::*)();
using matOutput = void(Kmt::*)(Mat(Kmt::*)());

struct findPosOutput {
	Mat frame;
	tWord x;
	tWord y;
};

class Kmt {
public:
	Kmt();
	Mat blur(Mat frame, int blurSize);
	Mat diffThreshold(Mat frame, int thresholdValue);
	findPosOutput findPos(Mat frame, float minimumSize);
	Mat getDepthMat();
	Mat getColorMat();
	void setBg(Mat bg);

private:
	int frameUpdateTimeout = 5000;
	KinectWrapper kinect;
	Mat colorFrameBufToGrayscaleMat(tByte* buf);
	Mat depthBufToGrayscaleMat(tWord* buf);
	Mat bg;
	Point2f lastPos;
};