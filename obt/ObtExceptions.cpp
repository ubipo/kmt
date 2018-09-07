// ObtExceptions.cpp
#include "ObtExceptions.h"

NoKinectException::NoKinectException() : exception("No kinect initialised") {};
NoKinectException::NoKinectException(const char* msg) : exception(msg) {};
const char* NoKinectException::what() const { return "NoKinectException"; };
