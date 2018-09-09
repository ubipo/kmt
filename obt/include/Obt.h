
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
	processingOutput process(Mat(Obt::* matSource)());
	void stream(Mat(Obt::* matSource)());
	void saveBackground(Mat(Obt::* matSource)());
	void output(matSource source, matOutput* out);
	Mat getDepthMat();
	Mat getColorMat();
	void subtract();

private:
	int frameUpdateTimeout = 5000;
	KinectWrapper kinect;
	Mat colorFrameBufToGrayscaleMat(tByte* buf);
	Mat depthBufToGrayscaleMat(tWord* buf);
};