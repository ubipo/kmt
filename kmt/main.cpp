// main.cpp - kmt (kinect mouse tracker) CLI

/**
 * kmt - Kinect Mouse Tracker
 *
 * Tool to track a mouse using Microsoft kinect
 * depth or color streams and opencv image processing.
 *
 * Licensed under the MIT License included in the source
 */

// Std
#include <iostream>
#include <string>
#include <map>
#include <exception>
#include <chrono>
#include <bitset>
#include <csignal>
#include <vector>
#include <sys/stat.h>
#include <thread>
using namespace std;

// Internal
#include "KinectWrapper.h"
#include "KinectWrapperExceptions.h"
#include "Kmt.h"
#include "Util.h"

// win
#include <Windows.h>
#include <Ole2.h>

// OpenCV
#include <opencv2/opencv.hpp>
using namespace cv;

// Kinect
#include <Kinect.h>

// Argparse
#include <cxxopts.hpp>

// Typedef
using Time = chrono::steady_clock;
using ms = chrono::milliseconds;
using byte = unsigned char;

void signalHandler(int signum);
void kmt(bool colorMode, bool rawMode, int blurSize, int threshold, float minimumSize, bool triggerMode, bool overwrite, bool streamOutput, bool videoOutput, string dataFileName, string videoFileName, int fps);

// Global verbose logger
VerboseLog verbose;

// Videowriter ptr, to gracefully close on exit
unique_ptr<VideoWriter> pVideo;


int main(int argc, char** argv) {
	signal(SIGINT, signalHandler);

	cxxopts::Options argParser("kmt", "Open Behavioural Tracker");

	argParser.add_options()
		("h,help", "Print this info")
		("v,verbose", "Verbose log")
		("c,color", "Color mode, use kinect color camera instead of depth")
		("r,raw", "Raw mode, don't process or output data")
		("b,blur", "Blur size", cxxopts::value<int>()->default_value("15"))
		("s,threshold", "Threshold value", cxxopts::value<int>()->default_value("30"))
		("m,minimum", "Minimum object size", cxxopts::value<float>()->default_value("6"))
		("t,trigger", "Wait for trigger before starting capture")
		("o,output", "Output mode(s): (S)tream (and\\or) (V)ideo", cxxopts::value<string>())
		("w,overwrite", "Overwrite files on conflict")
		("d,datafile", "Data file's name or path", cxxopts::value<string>()->default_value("data.csv"))
		("i,videofile", "Video file's name or path", cxxopts::value<string>()->default_value("video.avi"))
		("f,fps", "Video framerate (not stabalised, could time shift)", cxxopts::value<int>()->default_value("15"));

	string helpStr = argParser.help({ "", "Group" });

	// Args to parse
	bool colorMode, rawMode, triggerMode, overwrite, streamOutput, videoOutput;
	int blurSize, thresholdValue, fps;
	float minimumSize;
	string dataFileName;
	string videoFileName;

	try {
		cxxopts::ParseResult args = argParser.parse(argc, argv);

		// Extract args
		if (args.count("help")) {
			cout << helpStr << endl;
			return 0;
		}
		verbose.enabled = args.count("verbose"); // Verbose mode
		colorMode = args.count("color"); // Color mode
		rawMode = args.count("raw"); // Raw mode
		blurSize = args["blur"].as<int>(); // Blur size
		thresholdValue = args["threshold"].as<int>(); // Threshold value
		minimumSize = args["minimum"].as<float>(); // Minimum size
		triggerMode = args.count("trigger"); // Trigger mode
		overwrite = args.count("overwrite"); // Overwrite mode

		streamOutput = videoOutput = false; // Output mode(s)
		if (args.count("output")) {
			string outputModesStr = args["output"].as<string>();
			const char* outputModes = outputModesStr.c_str();
			for (byte i = 0; i < outputModesStr.length(); i++) {
				switch (tolower(outputModes[i])) {
					case 's':
						streamOutput = true; break;
					case 'v':
						videoOutput = true; break;
				}
			}
		}
		
		dataFileName = args["datafile"].as<string>(); // Data filename
		videoFileName = args["videofile"].as<string>(); // Video filename
		fps = args["fps"].as<int>(); // Fps

	} catch (exception& err) {
		cerr << "Exception parsing arguments: " << endl;
		cerr << err.what() << endl << endl;
		cerr << helpStr << endl;
		return 1;
	} catch (...) {
		cerr << "Unknown error parsing arguments occured" << endl;
		return 1;
	}

	verbose("Verbose logging enabled");

	// Run kmt, quit on error
	try {
		kmt(colorMode, rawMode, blurSize, thresholdValue, minimumSize, triggerMode, overwrite, streamOutput, videoOutput, dataFileName, videoFileName, fps);
	} catch (exception& err) {
		cerr << "Unrecoverable exception occured:" << endl;
		cerr << err.what() << endl << endl;
		cout << "Exiting..." << endl;
		return 1;
	} catch (...) {
		cerr << "Unrecoverable unknown error occured" << endl;
		cout << "Exiting..." << endl;
		return 1;
	}

	return 0;
}

/**
 * Runs kmt with the given arguments.
 *
 */
void kmt(bool colorMode, bool rawMode, int blurSize, int thresholdValue, float minimumSize, bool triggerMode, bool overwrite, bool streamOutput, bool videoOutput, string dataFileName, string videoFileName, int fps) {	
	// Init kmt
	unique_ptr<Kmt> pKmt;
	try {
		pKmt.reset(new Kmt());
	} catch (NoDefaultKinectException) {
		cerr << "Default kinect was either not found or doesn't return depth stream" << endl;
		exit(1);
	}

	// Get mat source pointer
	Mat(Kmt::*source)();
	if (colorMode)
		source = &Kmt::getColorMat;
	else
		source = &Kmt::getDepthMat;

	// Set bg file
	if (!rawMode) {
		Mat bg;
		bg = imread("./bg.bmp", IMREAD_GRAYSCALE);
		if (bg.data != nullptr) {
			pKmt->setBg(bg);
		}
		else {
			cout << "bg.bmp not found, press enter to capture..." << endl;
			cin.get();
			bg = pKmt->blur((*pKmt.*source)(), blurSize);
			imwrite("./bg.bmp", bg);
			pKmt->setBg(bg);
			cout << "bg.bmp saved" << endl;
		}
	}

	// Inititalise data file
	ofstream dataOut;
	if (!rawMode) {
		if (!overwrite && fileExists(dataFileName)) {
			cerr << "Data file \"" << dataFileName << "\" already exists, choose another name using the -d option" << endl;
			exit(1);
		}
		dataOut.open(dataFileName);
		dataOut << "t (ms),x (px),y (px)\n";
	}

	// Inititalise video
	if (videoOutput) {
		if (!overwrite && fileExists(videoFileName)) {
			cerr << "Video file \"" << videoFileName << "\" already exists, choose another name using the -v option" << endl;
			exit(1);
		}
		// Get test frame
		Mat frame = (*pKmt.*source)();
		pVideo.reset(new VideoWriter(videoFileName, CV_FOURCC('M', 'J', 'P', 'G'), fps, Size(frame.cols, frame.rows)));
	}

	// Initialise stream
	const char* streamWindowName = "kmt stream";
	if (streamOutput) {
		namedWindow(streamWindowName, WINDOW_AUTOSIZE);
	}

	// Wait for trigger
	if (triggerMode) {
		cout << "Trigger mode active, press enter to start..." << endl;
		cin.get();
	}

	// Stream
	verbose("Starting stream...");
	unsigned int t;
	chrono::time_point<Time> tStart, tFrameStart, tFrameCap, tFrameEnd;
	tStart = Time::now();
	if (!rawMode) {
		dataOut << "0,0,0\n";
	}
	while (true) {
		if (waitKey(1) >= 0) {
			break;
		}

		tFrameStart = Time::now();

		// Fetch frame
		Mat frame;
		try {
			frame = (*pKmt.*source)();
		} catch (NoFrameException) {
			cout << "Skipping frame..." << endl;
			continue;
		}

		// Calc time of capture
		tFrameCap = Time::now();
		t = toMs(tFrameCap - tStart);

		// Process
		findPosOutput posOutput;
		if (!rawMode) {
			Mat processed = pKmt->diffThreshold(pKmt->blur(frame, blurSize), thresholdValue);
			posOutput = pKmt->findPos(processed, minimumSize);
			frame = posOutput.frame;
		}

		// Output
		if (streamOutput) imshow(streamWindowName, frame);
		if (videoOutput) pVideo->write(frame);
		if (!rawMode) dataOut << t << "," << posOutput.x << "," << posOutput.y << "\n";

		// Print fps
		tFrameEnd = Time::now();
		unsigned int frameTime = toMs(tFrameEnd - tFrameStart);
		int fps = 1000 / frameTime;
		cout << "fps: " << fps << "               " << '\r' << flush;
	}
}

void signalHandler(int signum) {
	cout << "Exiting..." << endl;

	if (pVideo != nullptr) {
		pVideo.release();
	}

	exit(0);
}
