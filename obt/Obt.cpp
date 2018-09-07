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

using Time = chrono::steady_clock;
using ms = chrono::milliseconds;

template<class Interface> inline void SafeRelease(Interface *& pInterfaceToRelease) {
	if (pInterfaceToRelease != NULL) {
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}

// ======= KINECT BAK ========
KinectBAK::KinectBAK()
{
	pKinect = NULL;
	pReader = NULL;

	//rgbBuf = new unsigned char[1920 * 1080 * 4];

	//if (!initKinect()) {
	//	cout << "Default kinect was either not found or doesn't return depth stream" << endl;
	//}
}

KinectBAK::~KinectBAK() {
	//if (pRGBXBuff) {
	//	delete[] pRGBXBuff;
	//	pRGBXBuff = NULL;
	//}

	//SafeRelease(pDepthFrameReader);

	if (pKinect) {
		pKinect->Close();
	}
	SafeRelease(pKinect);
}

/**
 * Find and init the default kinect sensor.
 *
 * returns: true iff a default kinect was found and it returns a depth stream
 * throws: runtime_error
 */
bool KinectBAK::initKinect() {
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

	res = pKinect->OpenMultiSourceFrameReader(FrameSourceTypes::FrameSourceTypes_Color, &pReader);
	if (FAILED(res))
		throw new runtime_error("Error opening kinect frame reader: " + to_string(res));

	if (!updateFrame(5000))
		return false;

	return true; // Kinect found and init complete
}

bool KinectBAK::updateFrame(unsigned long timeout) {
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

//throw new runtime_error("Error getting kinect frame: " + to_string(res));

//IFrameDescription* pFrameDescription = NULL;
//int nWidth = 0;
//int nHeight = 0;
//USHORT nDepthMinReliableDistance = 0;
//USHORT nDepthMaxDistance = 0;
//UINT nBufferSize = 0;
//UINT16 *pBuffer = NULL;

//HRESULT hr = pDepthFrame->get_FrameDescription(&pFrameDescription);

//if (SUCCEEDED(hr))
//{
//	hr = pFrameDescription->get_Width(&nWidth);
//}

//if (SUCCEEDED(hr))
//{
//	hr = pFrameDescription->get_Height(&nHeight);
//}

//if (SUCCEEDED(hr))
//{
//	hr = pDepthFrame->get_DepthMinReliableDistance(&nDepthMinReliableDistance);
//}

//if (SUCCEEDED(hr))
//{
//	// In order to see the full range of depth (including the less reliable far field depth)
//	// we are setting nDepthMaxDistance to the extreme potential depth threshold
//	nDepthMaxDistance = USHRT_MAX;

//	// Note:  If you wish to filter by reliable depth distance, uncomment the following line.
//	//// hr = pDepthFrame->get_DepthMaxReliableDistance(&nDepthMaxDistance);
//}

//if (SUCCEEDED(hr))
//{
//	hr = pDepthFrame->AccessUnderlyingBuffer(&nBufferSize, &pBuffer);
//}

//if (SUCCEEDED(hr))
//{
//	toRGBX(pBuffer, nWidth, nHeight, nDepthMinReliableDistance, nDepthMaxDistance);
//}

//SafeRelease(pFrameDescription);

//SafeRelease(pFrame);

//return false;

Mat KinectBAK::getColorMat() {
	unsigned char* buf;

	try {
		buf = getRgbBuf();
	} catch (NoFrameException) {
		throw;
	}

	Mat colorImg = getRgbMat(buf);

	Rect frame(Point(400, 240), Point(1710, 850));
	Mat cropped = colorImg(frame);

	return cropped;
}

Mat KinectBAK::getRgbMat(unsigned char* buf) {
	Mat colorImg = Mat();

	colorImg.create(1080, 1920, CV_8UC1);
	//memcpy(colorImg->ptr<uchar>(0), buf, 1920 * 1080 * 4);

	uchar* matPtr = colorImg.ptr<uchar>(0);
	int i = 0;
	while (i < 1920 * 1080 * 4) {
		*matPtr = (buf[i++] + buf[i++]) / 2;
		matPtr++;
		i++;
		i++;
	}

	return colorImg;
}

unsigned char* KinectBAK::getRgbBuf() {
	if (pFrame == nullptr) {
		cout << "Null" << endl;
		throw new NoFrameException();
	}

	IColorFrame* colorframe;
	IColorFrameReference* frameref = nullptr;
	HRESULT res = pFrame->get_ColorFrameReference(&frameref);
	if (FAILED(res)) throw NoFrameException();
	res = frameref->AcquireFrame(&colorframe);
	if (FAILED(res)) throw NoFrameException();
	if (frameref) frameref->Release();

	colorframe->CopyConvertedFrameDataToArray(1920 * 1080 * 4, rgbBuf, ColorImageFormat_Rgba);

	SafeRelease(colorframe);
	SafeRelease(pFrame);

	return rgbBuf;
}

//unsigned char* KinectWrapper::getRgbFrameBuf() {
//	if (pFrame == nullptr) {
//		cout << "Null" << endl;
//		throw new NoFrameException();
//	}
//
//	IColorFrame* colorframe;
//	IColorFrameReference* frameref = nullptr;
//	HRESULT res = pFrame->get_ColorFrameReference(&frameref);
//	if (FAILED(res)) throw runtime_error("");
//	res = frameref->AcquireFrame(&colorframe);
//	if (FAILED(res)) throw NoFrameException();
//	if (frameref) frameref->Release();
//
//	colorframe->CopyConvertedFrameDataToArray(1920 * 1080 * 4, rgbBuf, ColorImageFormat_Rgba);
//
//	SafeRelease(colorframe);
//	SafeRelease(pFrame);
//
//	return rgbBuf;
//}

void KinectBAK::toRGBX(const UINT16* pBuffer, int nWidth, int nHeight, USHORT nMinDepth, USHORT nMaxDepth) {
	USHORT rangeMin = 600;
	USHORT rangeDelta = 255;

	if (!(pBuffer && (nWidth == cDepthWidth) && (nHeight == cDepthHeight)))
		throw new runtime_error("Invalid kinect data");

	// Allocate grayscale buffer
	Mat grayscale = Mat();
	grayscale.create(nHeight, nWidth, CV_8U);

	// end pixel is start + width*height - 1
	const UINT16* pBufferEnd = pBuffer + (nWidth * nHeight);

	int i;
	uchar* p;
	p = grayscale.ptr<uchar>(0);
	for (i = 0; i < nWidth * nHeight; ++i) {
		UINT16 depth = *pBuffer++;
		depth -= rangeMin;
		uchar intensity = static_cast<uchar>((depth >= 0) && (depth <= rangeDelta) ? depth : 0);
		p[i] = intensity;
	}

	//while (pBuffer < pBufferEnd) {
	//	USHORT depth = *pBuffer;

	//	// To convert to a byte, we're discarding the most-significant
	//	// rather than least-significant bits.
	//	// We're preserving detail, although the intensity will "wrap."
	//	// Values outside the reliable depth range are mapped to 0 (black).

	//	// Note: Using conditionals in this loop could degrade performance.
	//	// Consider using a lookup table instead when writing production code.
	//	depth -= rangeMin;
	//	BYTE intensity = static_cast<BYTE>((depth >= 0) && (depth <= rangeDelta) ? depth : 0);

	//	pRGBX->rgbRed = intensity;
	//	pRGBX->rgbGreen = intensity;
	//	pRGBX->rgbBlue = intensity;

	//	++pRGBX;
	//	++pBuffer;
	//}

	// Draw the data with OpenCV
	//Mat DepthImage(nHeight, nWidth, CV_8UC4, pRGBXBuff);
	//return DepthImage;
	Mat show = grayscale.clone();
	imshow("DepthImage", show);
}
// Kinect BAK ===========





Obt::Obt() {
}

void Obt::streamBAK() {
	chrono::time_point<Time> tStart = Time::now();
	while (true) {
		if (waitKey(1) >= 0) break;

		chrono::time_point<Time> tFrameStart = Time::now();

		kinectBAK.updateFrame(frameUpdateTimeout);
		
		Mat colorImg;
		try {
			colorImg = kinectBAK.getColorMat();
		} catch (NoFrameException) {
			continue;
		}
		
		imshow("DepthImage", colorImg);

		chrono::time_point<Time> tFrameEnd = Time::now();

		cout << toMs(tFrameEnd - tFrameStart) << "    " << '\r' << flush;
	}
}

void Obt::streamMat(Mat(Obt::*matSource)()) {
	chrono::time_point<Time> tStart = Time::now();
	while (true) {
		if (waitKey(1) >= 0) break;

		chrono::time_point<Time> tFrameStart = Time::now();

		kinect.updateMultiFrame(frameUpdateTimeout);
		//if (!updated) {
		//	cout << "Skipping frame" << endl;
		//	continue;
		//}

		// Get frame
		Mat toShow;
		try {
			toShow = (this->*matSource)();
		}
		catch (NoFrameException ex) {
			cout << "Skipping frame" << endl;
			continue;
		}

		imshow("Depth Stream", toShow);

		chrono::time_point<Time> tFrameEnd = Time::now();
		cout << toMs(tFrameEnd - tFrameStart) << "    " << '\r' << flush;
	}
}


Mat Obt::colorBufToGrayscaleMat(unsigned char* buf) {
	Mat mat = *new Mat();
	mat.create(1080, 1920, CV_8UC1);
	uchar* matPtr = mat.ptr<uchar>(0);

	for (int i = 0; i < 1920 * 1080 * 4;) {
		*matPtr = (buf[i++] + buf[i++]) / 2; // average g & b
		matPtr++;
		i++;
		i++;
	}

	return mat;
}

Mat Obt::depthBufToGrayscaleMat(unsigned short* buf) {
	short rangeMin = 600;
	short rangeDelta = 255;

	Mat mat = *new Mat();
	mat.create(424, 512, CV_8U);
	uchar* matPtr = mat.ptr<uchar>(0);

	for (int i = 0; i < 512 * 424; i++) {
		unsigned short depth = *buf++;
		uchar intensity = static_cast<uchar>((depth >= rangeMin) && (depth < rangeMin + rangeDelta) ? depth - rangeMin : 0);
		matPtr[i] = intensity;
	}

	return mat;
}

void Obt::stream() {
	if (!kinect.initKinect())
		throw new NoKinectException();

	chrono::time_point<Time> tStart = Time::now();
	while (true) {
		if (waitKey(1) >= 0) {
			break;
		}

		chrono::time_point<Time> tFrameStart = Time::now();

		unsigned char* rgbFrameBuf;
		bool updated = kinect.updateMultiFrame(frameUpdateTimeout);
		//if (!updated) {
		//	cout << "Skipping frame" << endl;
		//	continue;
		//}

		rgbFrameBuf = kinect.getRgbFrameBuf();

		Mat gs = colorBufToGrayscaleMat(rgbFrameBuf);

		Rect frame(Point(400, 240), Point(1710, 850));
		Mat cropped = gs(frame);
		imshow("Test Stream", cropped);

		chrono::time_point<Time> tFrameEnd = Time::now();

		cout << toMs(tFrameEnd - tFrameStart) << "    " << '\r' << flush;
	}
}


Mat Obt::getDepthMat() {
	bool updated = kinect.updateMultiFrame(frameUpdateTimeout);
	//if (!updated) {
	//	cout << "Skipping frame" << endl;
	//	return false;
	//}

	Mat depthMat = depthBufToGrayscaleMat(kinect.getDepthFrameBuf());

	Rect frame(Point(400, 240), Point(1710, 850));
	Mat cropped = depthMat(frame);

	return cropped;
}

Mat Obt::getColorMat() {
	unsigned char* buf;
	try {
		buf = kinect.getRgbFrameBuf();
	}
	catch (NoFrameException) {
		cout << "no frame" << endl;
		throw;
	}
	Mat colorMat = colorBufToGrayscaleMat(buf);

	Rect frame(Point(400, 240), Point(1710, 850));
	Mat cropped = colorMat(frame);

	return cropped;
}

void Obt::depth() {
	if (!kinect.initKinect()) {
		cout << "Default kinect was either not found or doesn't return depth stream" << endl;
		return;
	}

	this->streamMat(&Obt::getDepthMat);
}

void Obt::background() {
	if (!kinect.initKinect()) {
		cout << "Default kinect was either not found or doesn't return depth stream" << endl;
		return;
	}

	kinect.updateMultiFrame(frameUpdateTimeout);
	Mat gs = colorBufToGrayscaleMat(kinect.getRgbFrameBuf());
	Rect frame(Point(400, 240), Point(1710, 850));
	Mat cropped = gs(frame);
	imwrite("./bg.bmp", cropped);
}

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

		unsigned char* rgbFrameBuf;
		kinect.updateMultiFrame(frameUpdateTimeout);
		try {
			rgbFrameBuf = kinect.getRgbFrameBuf();
		}
		catch (runtime_error) {
			continue;
		}

		Mat gs = colorBufToGrayscaleMat(rgbFrameBuf);

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
