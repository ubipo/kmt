// KinectWrapper.h - Wrapper for the Microsoft Kinect SDK
#pragma once

// Kinect SDK
#include <Kinect.h>

// Internal
#include "KinectWrapperExceptions.h"

class KinectWrapper {
public:
	static const int        cDepthWidth = 512;
	static const int        cDepthHeight = 424;
	static const int		frameUpdateTimeout = 5000;

	KinectWrapper();
	~KinectWrapper();

	bool					initKinect();
	bool					updateMultiFrame(unsigned long timeout);
	unsigned char*			getColorFrameBuf();
	unsigned short*			getDepthFrameBuf();
	void					toRGBX(const UINT16* pBuffer, int nWidth, int nHeight, USHORT nMinDepth, USHORT nMaxDepth);

private:

	IKinectSensor*           pKinect;
	IMultiSourceFrameReader* pReader;
	IMultiSourceFrame*		 pFrame;
	unsigned char*			 colorBuf;
	unsigned short*			 depthBuf;
};
