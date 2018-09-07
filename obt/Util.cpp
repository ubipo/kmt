// Util.cpp - Utils for obt
#include "Util.h"

// std
#include <chrono>
using namespace std;

using Time = chrono::steady_clock;
using ms = chrono::milliseconds;

unsigned long toMs(chrono::duration<float> d) {
	return (unsigned long)std::chrono::duration_cast<ms>(d).count();
}