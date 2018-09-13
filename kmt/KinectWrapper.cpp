// KinectWrapper.cpp - Wrapper for the Microsoft Kinect SDK
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

// Typedef
using Time = chrono::steady_clock;
using ms = chrono::milliseconds;
using tByte = unsigned char; // Random prefix t to avoid conflict
using tWord = unsigned short;

KinectWrapper::KinectWrapper() {
	pKinect = nullptr;
	pReader = nullptr;

	if (!initKinect())
		throw NoDefaultKinectException();
}

KinectWrapper::~KinectWrapper() {
	SafeReleaseInterface(pReader);

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

/**
 * Waits for and stores a new multi-frame.
 *
 * args: timeout: time in milliseconds to return regardless
 * returns: true iff a frame arrived within the specified timeout
 * throws: NoReaderException iff no reader has been initialised, try calling initKinect()
 * throws: runtime_error
 */
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

/**
 * Extracts a color frame from the latest multi-frame, if no multi-frame is available it calls updateMultiFrame()
 *
 * returns: color frame buffer (byte*, 1920 * 1080 * 2)
 * throws: NoFrameException iff the multi-frame didn't include a color frame
 * throws: runtime_error
 */
tByte* KinectWrapper::getColorFrameBuf() {
	// Check if frame was captured
	if (pFrame == nullptr) {
		updateMultiFrame(frameUpdateTimeout);
	}

	unsigned int size = 1920 * 1080 * 2; // w * h * channels

	// Check if frame buffer is allocated
	if (colorBuf == nullptr) {
		colorBuf = new tByte[size];
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
	tByte* raw;
	colorframe->AccessRawUnderlyingBuffer(&rawSize, &raw); // YUV color space

	// Check raw buffer size
	if (rawSize != size) throw runtime_error("Invalid frame captured by kinect SDK");

	// Copy
	memcpy(colorBuf, raw, size);

	// Release
	SafeReleaseInterface(frameref);
	SafeReleaseInterface(colorframe);
	SafeReleaseInterface(pFrame);

	return colorBuf;
}


/**
 * Extracts a depth frame from the latest multi-frame, if no multi-frame is available it calls updateMultiFrame()
 *
 * returns: depth frame buffer (word*, 512 * 424)
 * throws: NoFrameException iff the multi-frame didn't include a depth frame
 * throws: runtime_error
 */
tWord* KinectWrapper::getDepthFrameBuf() {
	// Check if frame was captured
	if (pFrame == nullptr) {
		updateMultiFrame(frameUpdateTimeout);
	}

	unsigned int size = 512 * 424; // w * h

	// Check if frame buffer is allocated
	if (depthBuf == nullptr) {
		depthBuf = new tWord[size];
	}

	IDepthFrame* depthframe;
	IDepthFrameReference* frameref = nullptr;
	HRESULT res = pFrame->get_DepthFrameReference(&frameref);
	if (FAILED(res)) throw runtime_error("");
	res = frameref->AcquireFrame(&depthframe);
	if (FAILED(res)) {
		pFrame = nullptr;
		throw NoFrameException();
	}

	unsigned int rawSize;
	tWord* raw;
	depthframe->AccessUnderlyingBuffer(&rawSize, &raw);

	// Check raw buffer size
	if (rawSize != size) throw runtime_error("Invalid frame captured by kinect SDK");

	// Copy
	memcpy(depthBuf, raw, size * sizeof(tWord));

	// Release
	SafeReleaseInterface(frameref);
	SafeReleaseInterface(depthframe);
	SafeReleaseInterface(pFrame);

	return depthBuf;
}
