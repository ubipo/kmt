// ObtExceptions.h

// std
#include <exception>
using namespace std;


class NoKinectException : public exception {
public:
	NoKinectException();
	NoKinectException(const char* msg);
	const char * what() const;
};