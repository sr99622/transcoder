#pragma once

#define PY_ARRAY_UNIQUE_SYMBOL AVFACTORY_ARRAY_API

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <Python.h>
#include "pyhelper.h"
#include <opencv2/opencv.hpp>
#include <numpy/arrayobject.h>
#include "Box.h"
#include "Queue.h"
#include "Frame.h"
#include "Exception.h"

namespace av
{

class DeepSort
{
public:
	DeepSort() {}
	DeepSort(const char* model_dir, const char* python_dir, av::Queue<av::Frame>* frame_out_q);
	CPyObject getImage(const cv::Mat& image);
	CPyObject getDetections(const std::vector<av::Box<float>>& boxes, float* buffer);
	void track(const cv::Mat& image, const std::vector<av::Box<float>>& dets);

	CPyInstance pyInstance;
	CPyObject pClass;

	av::Queue<av::Frame>* frame_out_q = nullptr;
	av::ExceptionHandler ex;
};

}
