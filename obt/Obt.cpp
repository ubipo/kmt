// Obt.cpp

// KinectWrapper.cpp
#include "Obt.h"

// Internal
#include "ObtExceptions.h"
#include "KinectWrapper.h"
#include "KinectWrapperExceptions.h"
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
using matSource = Mat(Obt::*)();
using matOutput = void(Obt::*)(Mat(Obt::*)());

template<class Interface> inline void SafeRelease(Interface *& pInterfaceToRelease) {
	if (pInterfaceToRelease != NULL) {
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}

Obt::Obt() {
}

processingOutput Obt::process(Mat frame, int blurSize, int thresholdValue) {
	Mat temp = Mat();
	cv::subtract(frame, bg, temp);

	Mat smoothed = Mat();
	blur(temp, frame, Size(blurSize, blurSize));

	Mat thresholded = Mat();
	threshold(frame, temp, thresholdValue, 256, 0);

	frame = temp;

	processingOutput output;
	output.frame = frame;
	output.x = 0;
	output.y = 0;

	return output;
}

void Obt::setBg(Mat _bg) {
	bg = _bg;
}

tByte byteClamp(int a) {
	return a < 0 ? 0 : a > 255 ? 255 : a;
}

Mat Obt::colorFrameBufToGrayscaleMat(tByte* buf) {
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

Mat Obt::depthBufToGrayscaleMat(tWord* buf) {
	short rangeMin = 600;
	short rangeDelta = 255;

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

//void Obt::stream() {
//	if (!kinect.initKinect())
//		throw new NoKinectException();
//
//	chrono::time_point<Time> tStart = Time::now();
//	while (true) {
//		if (waitKey(1) >= 0) {
//			break;
//		}
//
//		chrono::time_point<Time> tFrameStart = Time::now();
//
//		unsigned char* rgbFrameBuf;
//		bool updated = kinect.updateMultiFrame(frameUpdateTimeout);
//		//if (!updated) {
//		//	cout << "Skipping frame" << endl;
//		//	continue;
//		//}
//
//		rgbFrameBuf = kinect.getColorFrameBuf();
//
//		Mat gs = colorFrameBufToGrayscaleMat(rgbFrameBuf);
//
//		Rect frame(Point(400, 240), Point(1710, 850));
//		Mat cropped = gs(frame);
//		imshow("Test Stream", cropped);
//
//		chrono::time_point<Time> tFrameEnd = Time::now();
//
//		cout << toMs(tFrameEnd - tFrameStart) << "    " << '\r' << flush;
//	}
//}


Mat Obt::getDepthMat() {
	bool updated = kinect.updateMultiFrame(frameUpdateTimeout);
	if (!updated) {
		cout << "Skipping frame" << endl;
		exit(1);
	}

	tWord* depthFrameBuf = kinect.getDepthFrameBuf();

	Mat depthMat = depthBufToGrayscaleMat(depthFrameBuf);

	Rect frame(Point(35, 100), Point(485, 310)); // 512 * 424
	Mat cropped = depthMat(frame);

	return cropped;
}

Mat Obt::getColorMat() {
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

//void Obt::depth() {
//	if (!kinect.initKinect()) {
//		cout << "Default kinect was either not found or doesn't return depth stream" << endl;
//		return;
//	}
//
//	this->streamMat(&Obt::getDepthMat);
//}

//void Obt::background() {
//	if (!kinect.initKinect()) {
//		cout << "Default kinect was either not found or doesn't return depth stream" << endl;
//		return;
//	}
//
//	kinect.updateMultiFrame(frameUpdateTimeout);
//	Mat gs = colorFrameBufToGrayscaleMat(kinect.getColorFrameBuf());
//	Rect frame(Point(400, 240), Point(1710, 850));
//	Mat cropped = gs(frame);
//	imwrite("./bg.bmp", cropped);
//}

void Obt::subtract() {
	if (!kinect.initKinect()) {
		cout << "Default kinect was either not found or doesn't return depth stream" << endl;
		return;
	}

	Mat bg = imread("./bg.bmp", IMREAD_GRAYSCALE);

	chrono::time_point<Time> tStart = Time::now();
	while (true) {
		if (waitKey(1) >= 0) {
			break;
		}

		chrono::time_point<Time> tFrameStart = Time::now();

		kinect.updateMultiFrame(frameUpdateTimeout);

		tByte* rgbFrameBuf;
		kinect.updateMultiFrame(frameUpdateTimeout);
		try {
			rgbFrameBuf = kinect.getColorFrameBuf();
		}
		catch (runtime_error) {
			continue;
		}

		Mat gs = colorFrameBufToGrayscaleMat(rgbFrameBuf);

		Rect frame(Point(400, 240), Point(1710, 850));
		Mat cropped = gs(frame);

		Mat subtracted = Mat();
		cv::subtract(bg, cropped, subtracted);

		Mat smoothed = Mat();
		blur(subtracted, smoothed, Size(7, 7));

		Mat thresholded = Mat();
		threshold(smoothed, thresholded, 30, 256, 0);

		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;

		findContours(thresholded, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

		vector<vector<Point> > contours_poly(contours.size());
		vector<Rect> boundRect(contours.size());
		vector<Point2f>center(contours.size());
		vector<float>radius(contours.size());

		for (int i = 0; i < contours.size(); i++)
		{
			approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
			boundRect[i] = boundingRect(Mat(contours_poly[i]));
			minEnclosingCircle((Mat)contours_poly[i], center[i], radius[i]);
		}

		Mat colored = Mat();
		cvtColor(thresholded, colored, cv::COLOR_GRAY2BGR);

		circle(colored, center[0], 50, 0xFF, 2);
		imshow("Preview Stream", colored);

		chrono::time_point<Time> tFrameEnd = Time::now();

		cout << toMs(tFrameEnd - tFrameStart) << "    " << '\r' << flush;
	}
}
