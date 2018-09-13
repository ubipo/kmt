# kmt - Kinect Mouse Tracker

Tool to track a mouse using Microsoft kinect
depth or color streams and opencv image processing.

## Requirements
- OpenCV 343
- Kinect SDK v2.0_1409

## Building
This project was made using visual studio so
there's no cmake list included.
Only the visual studio debug profile is set up correctly.
In order not to use opencv debug, edit the release profile accordingly.

## Things to note
- Includes are still hardlinked, change to env variable or manually change when upgrading opencv or kinect SDK
- Tests - No | Memory leaks and bugs - Yes
