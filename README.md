# kmt - Kinect Mouse Tracker

Tool to track a mouse using Microsoft kinect

depth or color streams and opencv image processing.

## Requirements
- OpenCV 343
- Kinect SDK v2.0_1409

## Building
Only the debug profile is set up correctly.
To not use opencv debug, edit the release profile.

## Things to note
- Includes are still hardlinked, change to env variable or manually change when upgrading opencv or kinect SDK
- Tests - No, Memory leaks and bugs - Yes