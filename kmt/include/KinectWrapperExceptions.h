// KinectWrapperExceptions.h
#pragma once

// std
#include <exception>
using namespace std;

class NoDefaultKinectException : public exception {
public:
	NoDefaultKinectException();
	NoDefaultKinectException(const char* msg);
	const char * what() const;
};

class NoReaderException : public exception {
public:
	NoReaderException();
	NoReaderException(const char* msg);
	const char * what() const;
};

class NoFrameException : public exception {
public:
	NoFrameException();
	NoFrameException(const char* msg);
	const char * what() const;
};