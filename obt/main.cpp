
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
#include "Obt.h"
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
void obt(bool colorMode, bool rawMode, int blurSize, int threshold, bool triggerMode, bool overwrite, bool streamOutput, bool videoOutput, string dataFileName, string videoFileName);

int main(int argc, char** argv) {
	signal(SIGINT, signalHandler);

	cxxopts::Options argParser("obt", "Open Behavioural Tracker");

	argParser.add_options()
		("h,help", "Print this info")
		("c,color", "Color mode, use kinect color camera instead of depth")
		("r,raw", "Raw mode, don't process or output data")
		("b,blur", "Blur size", cxxopts::value<int>()->default_value("20"))
		("s,threshold", "Threshold value", cxxopts::value<int>()->default_value("30"))
		("t,trigger", "Wait for trigger before starting capture")
		("o,output", "Output mode(s): (S)tream (and\\or) (V)ideo", cxxopts::value<string>())
		("w,overwrite", "Overwrite files on conflict")
		("d,datafile", "Data file's name or path", cxxopts::value<string>()->default_value("data.csv"))
		("v,videofile", "Video file's name or path", cxxopts::value<string>()->default_value("video.avi"));

	string helpStr = argParser.help({ "", "Group" });

	// Args to parse
	bool colorMode, rawMode, triggerMode, overwrite, streamOutput, videoOutput;
	int blurSize, thresholdValue;
	string dataFileName;
	string videoFileName;

	try {
		cxxopts::ParseResult args = argParser.parse(argc, argv);

		// help
		if (args.count("help")) {
			cout << helpStr << endl;
			exit(0);
		}

		// color mode
		colorMode = args.count("color");

		// raw mode
		rawMode = args.count("raw");

		// Blur size
		blurSize = args["blur"].as<int>();

		// Threshold value
		thresholdValue = args["threshold"].as<int>();

		// trigger mode
		triggerMode = args.count("trigger");

		// overwrite
		overwrite = args.count("overwrite");

		// output mode(s)
		streamOutput = videoOutput = false;
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

		// data file
		dataFileName = args["datafile"].as<string>();

		// video file
		videoFileName = args["videofile"].as<string>();

		//cout << colorMode << " " << rawMode << " " << streamOutput << " " << videoOutput << " " << dataFile << endl;

	} catch (exception& err) {
		cerr << "Exception parsing arguments: " << endl;
		cerr << err.what() << endl << endl;
		cerr << helpStr << endl;
		return 1;
	} catch (...) {
		cerr << "Unknown error parsing arguments occured" << endl;
		return 1;
	}

	obt(colorMode, rawMode, blurSize, thresholdValue, triggerMode, overwrite, streamOutput, videoOutput, dataFileName, videoFileName);

	return EXIT_SUCCESS;
}

bool fileExists(string file) {
	struct stat buffer;
	return stat(file.c_str(), &buffer) == 0;
}

void obt(bool colorMode, bool rawMode, int blurSize, int thresholdValue, bool triggerMode, bool overwrite, bool streamOutput, bool videoOutput, string dataFileName, string videoFileName) {
	// Init obt
	Obt obt = Obt();

	// Get mat source pointer
	Mat(Obt::*source)();
	if (colorMode)
		source = &Obt::getColorMat;
	else
		source = &Obt::getDepthMat;

	// Inititalise data file
	if (!rawMode) {
		if (!overwrite && fileExists(dataFileName)) {
			cerr << "Data file \"" << dataFileName << "\" already exists, choose another name using the -d option" << endl;
			exit(1);
		}
		ofstream dataOut;
		dataOut.open(dataFileName);
	}

	// Inititalise stream & video
	const char* streamWindowName = "kmt stream";
	VideoWriter videoOut;
	if (streamOutput) {
		namedWindow(streamWindowName, WINDOW_AUTOSIZE);
	}
	//if (videoOutput) {
	//	if (!overwrite && fileExists(videoFileName)) {
	//		cerr << "Video file \"" << videoFileName << "\" already exists, choose another name using the -v option" << endl;
	//		exit(1);
	//	}
	//	int fourcc = VideoWriter::fourcc('D', 'I', 'V', 'X');
	//	videoOut = VideoWriter(videoFileName, fourcc, fpsTarget, Size(640, 480), true);
	//}

	// Set bg file
	if (!rawMode) {
		Mat bg;
		bg = imread("./bg.bmp", IMREAD_GRAYSCALE);
		if (bg.data != NULL) {
			obt.setBg(bg);
		} else {
			cout << "bg.bmp not found, press enter to capture..." << endl;
			cin.get();
			bg = (obt.*source)();
			imwrite("./bg.bmp", bg);
			obt.setBg(bg);
			cout << "bg.bmp saved" << endl;
		}
	}

	// Wait for trigger
	if (triggerMode) {
		cout << "Trigger mode active, press enter to start..." << endl;
		cin.get();
	}

	// Stream
	chrono::time_point<Time> tStart = Time::now();
	while (true) {
		if (waitKey(1) >= 0) {
			break;
		}

		chrono::time_point<Time> tFrameStart = Time::now();

		// Fetch frame
		Mat frame;
		try {
			frame = (obt.*source)();
		} catch (NoFrameException) {
			cout << "Skipping frame..." << endl;
			continue;
		}

		// Process
		if (!rawMode) {
			processingOutput processed = obt.process(frame, blurSize, thresholdValue);
			frame = processed.frame;
		}

		// Output
		if (streamOutput) imshow(streamWindowName, frame);
		//if (videoOutput) videoOut.write(frame);

		chrono::time_point<Time> tFrameEnd = Time::now();
		int frameTime = toMs(tFrameEnd - tFrameStart);
		//if (frameTime < frameTimeTarget) {
		//	int delta = frameTimeTarget - frameTime;
		//}
		int fps = 1000 / frameTime;
		cout << "fps:" << fps << "               " << '\r' << flush;
	}
}

void signalHandler(int signum) {
	exit(signum);
}

/*
Old menu

while (true) {
	cout << "[T]est stream | Save [B]ackground | [P]review >";
	char input = cin.get();
	if (input != '\n')
		cin.ignore(INT_MAX, '\n');

	switch (tolower(input)) {
	case 't':
		cout << "Test stream" << endl;
		stream();
		break;
	case 'b':
		cout << "Background" << endl;
		background();
		break;
	case 'p':
		cout << "Preview" << endl;
		subtract();
		break;
	case 'd':
		cout << "Depth" << endl;
		depth();
		break;
	case '\n':
		break;
	default:
		cout << "Unrecognised input \"" << input << "\"" << endl;
	};

	//kinect.Update();
	//if (waitKey(1) >= 0) {
	//	break;
	//}

	//Kinect kinect;
	//if (!kinect.InitKinect()) {
	//	cout << "Default kinect was either not found or doesn't return depth stream" << endl;
	//}
}
*/