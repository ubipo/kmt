// KinectWrapperExceptions.cpp
#include "KinectWrapperExceptions.h"

NoReaderException::NoReaderException() : exception("No reader initialised") {};
NoReaderException::NoReaderException(const char* msg) : exception(msg) {};
const char* NoReaderException::what() const { return "NoReaderException"; };

NoFrameException::NoFrameException() : exception("No frame yet captured") {};
NoFrameException::NoFrameException(const char* msg) : exception(msg) {};
const char * NoFrameException::what() const { return "NoFrameException"; };