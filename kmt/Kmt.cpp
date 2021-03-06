// Kmt.cpp - Logic for Kinect Mouse Tracker

// KinectWrapper.cpp
#include "Kmt.h"

// Internal
#include "KinectWrapper.h"
#include "Util.h"

// std
#include <chrono>
using namespace std;

// OpenCV
#include <opencv2/opencv.hpp>
using namespace cv;

// Typedef
using tByte = unsigned char; // Random prefix t to avoid conflict
using tWord = unsigned short;
using Time = chrono::steady_clock;
using ms = chrono::milliseconds;
using matSource = Mat(Kmt::*)();
using matOutput = void(Kmt::*)(Mat(Kmt::*)());


Kmt::Kmt() {}

void Kmt::setBg(Mat _bg) {
	bg = _bg;
}

/**
 * Blurs (low-pass filters) a frame.
 *
 * args: frame
 *		 blurSize: size of kernel in pixels
 * returns: processed frame
 */
Mat Kmt::blur(Mat frame, int blurSize) {
	Mat temp = Mat();
	cv::blur(frame, temp, Size(blurSize, blurSize));
	return temp;
}

/**
 * Takes absolute diff with background and thresholds frame.
 *
 * pre: setBg() has been called
 * args: frame
 *		 thresholdValue: value for boolean threshold
 * returns: processed frame
 */
Mat Kmt::diffThreshold(Mat frame, int thresholdValue) {
	Mat temp = Mat();
	cv::absdiff(frame, bg, temp);
	threshold(temp, frame, thresholdValue, 256, 0);

	return frame;
}

/**
 * Finds and marks position of the mouse in the given frame
 * if no sufficiently large object is found the last known
 * position is returned.
 *
 * args: frame
 *		 minimumSize: objects under this size (in px) will be ignored
 * returns: findPosOutput
 *			.frame: marked frame
 *			.x:		x-pos
 *			.y:		y-pos
 */
findPosOutput Kmt::findPos(Mat frame, float minimumSize) {
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;

	findContours(frame, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

	vector<Point> contoursPoly;
	Point2f center;
	float radius;

	float largestFoundRadius = 0;
	Point2f posLargest = lastPos; // If no sufficiently large object is found, default to last known pos

	for (int i = 0; i < contours.size(); i++) {
		approxPolyDP(Mat(contours[i]), contoursPoly, 3, true);
		minEnclosingCircle((Mat)contoursPoly, center, radius);

		if (radius >= minimumSize && radius > largestFoundRadius) {
			largestFoundRadius = radius;
			posLargest = center;
			lastPos = posLargest;
		}
	}

	Mat colored = Mat();
	cvtColor(frame, colored, cv::COLOR_GRAY2BGR);

	circle(colored, posLargest, 50, 0x0000FF, 2);

	findPosOutput output;
	output.frame = colored;
	output.x = (tWord)posLargest.x;
	output.y = (tWord)posLargest.y;

	return output;
}

Mat Kmt::getDepthMat() {
	bool updated = kinect.updateMultiFrame(frameUpdateTimeout);
	if (!updated) {
		cout << "Skipping frame" << endl;
		exit(1);
	}

	tWord* depthFrameBuf = kinect.getDepthFrameBuf();

	Mat depthMat = depthBufToGrayscaleMat(depthFrameBuf);

	Rect frame(Point(45, 40), Point(475, 250)); // 512 * 424
	Mat cropped = depthMat(frame);

	return cropped;
}

Mat Kmt::getColorMat() {
	tByte* buf;
	try {
		buf = kinect.getColorFrameBuf();
	}
	catch (NoFrameException) {
		cout << "no frame" << endl;
		throw;
	}
	Mat colorMat = colorFrameBufToGrayscaleMat(buf);

	Rect frame(Point(400, 240), Point(1710, 850));
	Mat cropped = colorMat(frame);

	return cropped;
}

/**
 * Converts the given color frame buffer (of size
 * 1920 * 1080 * 3) to a bgr Mat frame.
 *
 * args: buffer
 * returns: color frame
 */
Mat Kmt::colorFrameBufToGrayscaleMat(tByte* buf) {
	Mat yuv(1080, 1920, CV_8UC2, buf); // YUV color space

	Mat bgr = Mat();
	bgr.create(1080, 1920, CV_8UC3);

	cvtColor(yuv, bgr, CV_YUV2BGR_YUYV);

	Mat channels[3];
	split(bgr, channels);
	channels[1] = Mat::zeros(1080, 1920, CV_8UC1);
	merge(channels, 3, bgr);

	Mat gray = Mat();
	cvtColor(bgr, gray, CV_BGR2GRAY);

	return gray;
}

/**
 * Converts the given depth frame buffer (of size
 * 650 * 135) to a depth Mat frame.
 *
 * args: buffer
 * returns: depth frame
 */
Mat Kmt::depthBufToGrayscaleMat(tWord* buf) {
	short rangeMin = 650;
	short rangeDelta = 135;

	Mat mat = *new Mat();
	mat.create(424, 512, CV_8U);
	tByte* matPtr = mat.ptr<tByte>(0);

	for (int i = 0; i < 512 * 424; i++) {
		tWord depth = *buf++;
		tByte intensity = (depth >= rangeMin) && (depth < rangeMin + rangeDelta) ? depth - rangeMin : 0;
		matPtr[i] = intensity;
	}

	return mat;
}