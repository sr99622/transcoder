#pragma once

#define PY_ARRAY_UNIQUE_SYMBOL PyModelSSD_ARRAY_API

#include <iostream>
#include <Python.h>
#include "pyhelper.h"
#include <opencv2/opencv.hpp>
#include <numpy/arrayobject.h>
#include "Box.h"
#include "Queue.h"
#include "Frame.h"
#include "Exception.h"

class PyModelSSD
{
public:
	PyModelSSD() {}
	PyModelSSD(const char* model_dir, av::Queue<av::Frame>* frame_out_q, int threshold = 20);
	void setThreshold(int threshold);
	void detect(const cv::Mat& image, std::vector<av::Box<int>>* boxes);
	CPyObject getImage(const cv::Mat& image);

	CPyInstance pyInstance;
	CPyObject pClass;

	av::Queue<av::Frame>* frame_out_q = nullptr;
	av::ExceptionHandler ex;
};

