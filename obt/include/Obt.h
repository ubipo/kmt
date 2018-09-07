
#pragma once

// Internal
#include "KinectWrapper.h"

// OpenCV
#include <opencv2/opencv.hpp>
using namespace cv;

class KinectBAK
{
public:
	static const int        cDepthWidth = 512;
	static const int        cDepthHeight = 424;
	KinectBAK();
	~KinectBAK();
	bool					initKinect();
	bool					updateFrame(unsigned long timeout);
	Mat						getColorMat();
	Mat						getRgbMat(unsigned char * buf);
	unsigned char*			getRgbBuf();
	void					toRGBX(const UINT16* pBuffer, int nWidth, int nHeight, USHORT nMinDepth, USHORT nMaxDepth);

private:

	IKinectSensor*           pKinect = nullptr;
	IMultiSourceFrameReader* pReader = nullptr;
	IMultiSourceFrame*		 pFrame = nullptr;
	unsigned char*			 rgbBuf;
};

class Obt {
public:
	Obt();
	void streamBAK();
	void streamMat(Mat(Obt::* getMat)());
	void stream();
	Mat getDepthMat();
	Mat getColorMat();
	void depth();
	void background();
	void subtract();

private:
	int frameUpdateTimeout = 5000;
	KinectWrapper kinect;
	KinectBAK kinectBAK;
	Mat colorBufToGrayscaleMat(unsigned char* buf);
	Mat depthBufToGrayscaleMat(unsigned short* buf);
};