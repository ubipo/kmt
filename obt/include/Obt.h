
#pragma once

// Internal
#include "KinectWrapper.h"

// OpenCV
#include <opencv2/opencv.hpp>
using namespace cv;

class Obt {
public:
	Obt();
	Mat process(Mat(Obt::* matSource)());
	Mat getFromSource(Mat(Obt::* matSource)());
	void stream(Mat(Obt::* matSource)());
	void saveBackground(Mat(Obt::* matSource)());
	Mat getDepthMat();
	Mat getColorMat();
	void subtract();

private:
	int frameUpdateTimeout = 5000;
	KinectWrapper kinect;
	Mat colorFrameBufToGrayscaleMat(unsigned char * buf);
	Mat depthBufToGrayscaleMat(unsigned short* buf);
};