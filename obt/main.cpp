
// Std
#include <iostream>
#include <string>
#include <map>
#include <exception>
#include <chrono>
#include <bitset>
#include <csignal>
#include <vector>
using namespace std;

// Internal
#include "KinectWrapper.h"
#include "Obt.h"

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

//#define width 512
//#define height 424

using byte = unsigned char;

void signalHandler(int signum);
void obt(char outputType, bool colorMode, bool process);

int main(int argc, char** argv) {
	signal(SIGINT, signalHandler);

	cxxopts::Options argParser("obt", "Open Behavioural Tracker");

	argParser.add_options()
		("h,help", "Print this info")
		("c,color", "Color mode, use kinect color camera instead of depth")
		("r,raw", "Raw mode, don't process or output data")
		("o,output", "Output mode(s): (S)tream (and\\or) (V)ideo", cxxopts::value<string>())
		("f,filename", "Data file's name or path", cxxopts::value<string>()->default_value("data.csv"));

	string helpStr = argParser.help({ "", "Group" });

	// Args to parse
	bool colorMode, rawMode, streamOutput, videoOutput;
	const char* dataFile;

	try {
		cxxopts::ParseResult args = argParser.parse(argc, argv);

		// help
		if (args.count("help")) {
			cout << helpStr << endl;
			exit(0);
		}

		// color mode
		colorMode = bool(args.count("color"));

		// raw mode
		rawMode = bool(args.count("raw"));

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
		dataFile = args["filename"].as<string>().c_str();

		//cout << colorMode << " " << rawMode << " " << streamOutput << " " << videoOutput << " " << dataFile << endl;
		return 0;

	} catch (exception& err) {
		cerr << "Exception parsing arguments: " << endl;
		cerr << err.what() << endl << endl;
		cerr << helpStr << endl;
		return 1;
	} catch (...) {
		cerr << "Unknown error parsing arguments occured" << endl;
		return 1;
	}

	obt(colorMode, rawMode, streamOutput, videoOutput, dataFile);

	return EXIT_SUCCESS;
}

void obt(bool colorMode, bool rawMode, bool	streamOutput, bool videoOutput, const char* dataFile) {
	// Init obt
	Obt obt = Obt();

	// Get mat source pointer
	Mat (Obt::*source)();
	if (colorMode) {
		source = &Obt::getColorMat;
	} else {
		source = &Obt::getDepthMat;
	}

	// Get processed mat pointer
	Mat(Obt::*processed)();
	//if (process) {
	//	processed = &Obt::process;
	//} else {
	//	processed = source;
	//}

	processed = source;

	switch (outputType) {
	case 's':
		obt.stream(processed);
		break;
	case 'b':
		obt.saveBackground(processed);
		break;
	default:
		cout << "wat" << endl;
		break;
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