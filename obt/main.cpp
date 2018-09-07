

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

void signalHandler(int signum);
void obt(char outputType, bool colorMode, bool process);

int main(int argc, char** argv) {
	signal(SIGINT, signalHandler);

	cxxopts::Options argParser("obt", "Open Behavioural Tracker");

	argParser.add_options()
		("h,help", "Print this message")
		("o,output", "Output type - [S]tream, (F)ile or (B)ackground", cxxopts::value<char>()->default_value("s"))
		("c,color", "Color mode, use kinect color camera instead of depth")
		("p,process", "Process the output");

	// Args to parse
	char outputType;
	bool colorMode;
	bool process;

	try {
		cxxopts::ParseResult args = argParser.parse(argc, argv);

		// help
		if (args.count("help")) {
			cout << argParser.help({ "", "Group" }) << endl;
			exit(0);
		}

		// output type
		outputType = args["output"].as<char>();
		char outputTypes[3] = { 's', 'f', 'b' };
		char* selectedOutputType = find(begin(outputTypes), end(outputTypes), outputType);
		if (selectedOutputType == end(outputTypes)) {
			cout << "Unknown output type '" << outputType << "', selecting 's'" << endl;
			outputType = 's';
		}

		// color mode
		colorMode = bool(args.count("color"));

		// process
		process = bool(args.count("process"));

	} catch (exception& err) {
		cout << "Exception parsing arguments: " << typeid(err).name() << endl;
		cerr << err.what() << endl << endl;
		cout << argParser.help({ "", "Group" }) << endl;
		return 1;
	} catch (...) {
		cout << "Unknown error occured" << endl;
		return 1;
	}

	obt(outputType, colorMode, process);

	return EXIT_SUCCESS;
}

void obt(char outputType, bool colorMode, bool process) {
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
	//	source = &Obt::getColorMat;
	//} else {
	//	processed = &Obt::getDepthMat;
	//}

	processed = source;

	obt.streamMat(processed);
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