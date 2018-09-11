// Util.h - Utils for kmt
#pragma once

// std
#include <iostream>
#include <chrono>
#include <string>
using namespace std;

using Time = chrono::steady_clock;
using ms = chrono::milliseconds;

unsigned int toMs(chrono::duration<float> d);

struct VerboseLog {
	bool enabled = false;
	void operator()(string msg);
};

template<class Interface> inline void SafeReleaseInterface(Interface *& pInterfaceToRelease) {
	if (pInterfaceToRelease != NULL) {
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
};