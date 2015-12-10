# OpenNI 2 Recorder

Records video and depth frame from a 3D sensing camera (like Microsoft Kinect or Asus Xtion Pro).

Saves current image via key `p`.

## Installation

Colored Sphere Tracking depends on:
- [OpenNI 2](http://structure.io/openni)
- [OpenCV](http://opencv.org) (with OpenNI 2 support, this needs to be enabled in build step)

Colored Sphere Tracking uses [CMake](http://www.cmake.org) for building, therefore run:

1. `mkdir build && cd build`
- `cmake ..`
- `make`

Homebrew: `cmake -DOPENNI2_LIBRARY=/usr/local/lib/ni2 -DOPENNI2_INCLUDE_DIR=/usr/local/include/ni2 ..`

## Contribution

OpenNI 2 Recorder follows the [ROS C++ Coding Style](http://wiki.ros.org/CppStyleGuide).


## License

OpenNI 2 Recorder is released with a BSD license. For full terms and conditions, see the [LICENSE](https://github.com/gaug-cns/openni2-recorder/blob/master/LICENSE) file.


## Authors

See [AUTHORS.md](https://github.com/gaug-cns/openni2-recorder/blob/master/AUTHORS.md) for a full list of contributors.