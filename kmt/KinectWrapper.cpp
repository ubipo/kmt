// KinectWrapper.cpp
#pragma once
#include "KinectWrapper.h"

// Internal
#include "KinectWrapperExceptions.h"
#include "Util.h"

// std
#include <exception>
#include <iostream>
#include <string>
#include <chrono>
#include <bitset>
using namespace std;

// OpenCV
#include <opencv2/opencv.hpp>
using namespace cv;

// Kinect SDK
#include <Kinect.h>


using Time = chrono::steady_clock;
using ms = chrono::milliseconds;

KinectWrapper::KinectWrapper() {
	pKinect = NULL;
	pReader = NULL;

	if (!initKinect())
		throw NoDefaultKinectException();
}

KinectWrapper::~KinectWrapper() {
	//if (pRGBXBuff) {
	//	delete[] pRGBXBuff;
	//	pRGBXBuff = NULL;
	//}

	//SafeRelease(pDepthFrameReader);

	if (pKinect) {
		pKinect->Close();
	}
	SafeReleaseInterface(pKinect);
}

/**
 * Find and init the default kinect sensor.
 *
 * returns: true iff a default kinect was found and it returns a depth stream
 * throws: runtime_error
 */
bool KinectWrapper::initKinect() {
	cout << "Looking for default kinect..." << endl;

	HRESULT res = GetDefaultKinectSensor(&pKinect);
	if (FAILED(res))
		throw new runtime_error("Error searching for active kinects: " + to_string(res));

	if (!pKinect)
		return false; // No kinect found

	BOOLEAN available;
	res = pKinect->get_IsAvailable(&available);
	if (FAILED(res))
		throw new runtime_error("Error getting kinect availability: " + to_string(res));
	if (available != 0)
		return false;

	res = pKinect->Open();
	if (FAILED(res))
		throw new runtime_error("Error opening kinect: " + to_string(res));

	res = pKinect->OpenMultiSourceFrameReader(FrameSourceTypes::FrameSourceTypes_Color | FrameSourceTypes::FrameSourceTypes_Depth, &pReader);
	if (FAILED(res))
		throw new runtime_error("Error opening kinect frame reader: " + to_string(res));

	if (!updateMultiFrame(5000))
		return false;

	cout << "Kinect found!" << endl;

	return true; // Kinect found and init complete
}

bool KinectWrapper::updateMultiFrame(unsigned long timeout) {
	if (pReader == nullptr)
		throw new NoReaderException();

	chrono::time_point<Time> tStart = Time::now();
	while (toMs(Time::now() - tStart) < timeout) {
		HRESULT res = pReader->AcquireLatestFrame(&pFrame);
		if (res == 0) { // Frame
			return true;
		}
		else if (res == -2147483638) { // No frame?
			pFrame = nullptr;
			continue;
		}
		else { // Error
			throw new runtime_error("Error updating frame: " + to_string(res));
		}
	}

	return false; // Timeout elapased
}

unsigned char* KinectWrapper::getColorFrameBuf() {
	// Check if frame was captured
	if (pFrame == nullptr) {
		updateMultiFrame(frameUpdateTimeout);
	}

	unsigned int size = 1920 * 1080 * 2; // w * h * channels

	// Check if frame buffer is allocated
	if (colorBuf == nullptr) {
		colorBuf = new unsigned char[size];
	}

	// Acquire color frame
	IColorFrame* colorframe;
	IColorFrameReference* frameref = nullptr;
	HRESULT res = pFrame->get_ColorFrameReference(&frameref);
	if (FAILED(res)) throw runtime_error("");
	res = frameref->AcquireFrame(&colorframe);
	if (FAILED(res)) {
		pFrame = nullptr;
		throw NoFrameException();
	}

	// Get raw buffer pointer
	unsigned int rawSize;
	unsigned char* raw;
	colorframe->AccessRawUnderlyingBuffer(&rawSize, &raw); // YUV color space

	// Check raw buffer size
	if (rawSize != size) throw runtime_error("");

	// Copy
	memcpy(colorBuf, raw, size);

	// Release
	SafeReleaseInterface(frameref);
	SafeReleaseInterface(colorframe);
	SafeReleaseInterface(pFrame);

	return colorBuf;
}

unsigned short* KinectWrapper::getDepthFrameBuf() {
	// Check if frame was captured
	if (pFrame == nullptr) {
		updateMultiFrame(frameUpdateTimeout);
	}

	unsigned int size = 512 * 424; // w * h

	// Check if frame buffer is allocated
	if (depthBuf == nullptr) {
		depthBuf = new unsigned short[size];
	}

	IDepthFrame* depthframe;
	IDepthFrameReference* frameref = nullptr;
	HRESULT res = pFrame->get_DepthFrameReference(&frameref);
	if (FAILED(res)) throw runtime_error("");
	res = frameref->AcquireFrame(&depthframe);
	if (FAILED(res)) throw NoFrameException("");

	unsigned int rawSize;
	unsigned short* raw;
	depthframe->AccessUnderlyingBuffer(&rawSize, &raw);

	// Check raw buffer size
	if (rawSize != size) throw runtime_error("");

	// Copy
	memcpy(depthBuf, raw, size * sizeof(unsigned short));

	// Release
	SafeReleaseInterface(frameref);
	SafeReleaseInterface(depthframe);
	SafeReleaseInterface(pFrame);

	return depthBuf;
}
