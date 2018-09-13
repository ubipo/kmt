// KinectWrapper.cpp
#pragma once
#include "KinectWrapper.h"

// Internal
#include "KinectWrapperExceptions.h"
#include "Util.h"

// std
#include <iostream>
#include <chrono>
#include <string>
using namespace std;

using Time = chrono::steady_clock;
using ms = chrono::milliseconds;

unsigned int toMs(chrono::duration<float> d) {
	return (unsigned int)std::chrono::duration_cast<ms>(d).count();
};

void VerboseLog::operator()(string msg) {
	if (enabled) cout << msg << endl;
}

tByte byteClamp(int a) {
	return a < 0 ? 0 : a > 255 ? 255 : a;
}

bool fileExists(string file) {
	struct stat buffer;
	return stat(file.c_str(), &buffer) == 0;
}