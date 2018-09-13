// KinectWrapper.h - Wrapper for the Microsoft Kinect SDK
#pragma once

// Kinect SDK
#include <Kinect.h>

// Internal
#include "KinectWrapperExceptions.h"

// Typedef
using tByte = unsigned char; // Random prefix t to avoid conflict
using tWord = unsigned short;

class KinectWrapper {
public:
	static const int        cDepthWidth = 512;
	static const int        cDepthHeight = 424;
	static const int		frameUpdateTimeout = 5000;

	KinectWrapper();
	~KinectWrapper();

	bool					initKinect();
	bool					updateMultiFrame(unsigned long timeout);
	tByte*					getColorFrameBuf();
	tWord*			getDepthFrameBuf();

private:

	IKinectSensor*           pKinect;
	IMultiSourceFrameReader* pReader;
	IMultiSourceFrame*		 pFrame;
	tByte*					 colorBuf;
	tWord*					 depthBuf;
};
