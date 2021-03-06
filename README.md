# Webcam Greenscreen Computer Vision Project
## C++ OpenCV project on Linux

## Table of contents
* [Introduction](#introduction)
* [Technologies](#technologies)
* [Setup](#setup)


## <a name="introduction"/> Introduction
This project is a training project to practice working with image processing, statistics, video, and image masks. The webcam used for testing is very cheap and noisy so the program tries to statistically recover data over noise. 

![](556fls.gif)


## <a name="technologies"/> Technologies
Project is created with:
* C++ 17
* OpenCV 4.5.2-pre
* Cmake 3.16.3

## <a name="setup"/> Setup
To make project, download and make OpenCV then run the following code. 

```
$ ./make_run.sh
```

To run enter the following command. Add the directory of the image you would like as an argument. 

```
$ mkdir build/
$ cd build/
$ ./showimg dir_to_image/
```
