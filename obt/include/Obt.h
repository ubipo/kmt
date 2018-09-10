
#pragma once

// Internal
#include "KinectWrapper.h"

// OpenCV
#include <opencv2/opencv.hpp>
using namespace cv;

class Obt;

// Typedef
using tByte = unsigned char; // Random prefix t to avoid conflict
using tWord = unsigned short;
using matSource = Mat(Obt::*)();
using matOutput = void(Obt::*)(Mat(Obt::*)());

struct processingOutput {
	Mat frame;
	tWord x;
	tWord y;
};

class Obt {
public:
	Obt();
	processingOutput process(Mat frame, int blurSize, int thresholdValue);
	Mat getDepthMat();
	Mat getColorMat();
	void subtract();
	void setBg(Mat bg);

private:
	int frameUpdateTimeout = 5000;
	KinectWrapper kinect;
	Mat colorFrameBufToGrayscaleMat(tByte* buf);
	Mat depthBufToGrayscaleMat(tWord* buf);
	Mat bg;
};