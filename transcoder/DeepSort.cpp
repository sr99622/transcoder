#define NO_IMPORT_ARRAY
#include "DeepSort.h"

av::DeepSort::DeepSort(const char* model_dir, const char* python_dir, av::Queue<av::Frame>* frame_out_q
) :
	frame_out_q(frame_out_q)
{
	try {
		if (!Py_IsInitialized()) throw av::Exception("Python not intialized");

		CPyObject sysPath = PySys_GetObject("path");
		CPyObject dirName = PyUnicode_FromString(python_dir);
		PyList_Append(sysPath, dirName);

		const char* py_file = "interface";
		CPyObject pName = PyUnicode_FromString(py_file);           if (!pName)   throw av::Exception("pName");
		CPyObject pModule = PyImport_Import(pName);	               if (!pModule) throw av::Exception("pModule");
		CPyObject pDict = PyModule_GetDict(pModule);               if (!pDict)   throw av::Exception("pDict");
		CPyObject pItem = PyDict_GetItemString(pDict, "DeepSort"); if (!pItem)   throw av::Exception("pItem");
		pClass = PyObject_CallObject(pItem, NULL);                 if (!pClass)  throw av::Exception("pClass");

		if (!PyObject_CallMethod(pClass, "initialize", "(s)", model_dir)) throw av::Exception("initialize");
	}
	catch (const av::Exception& e) {
		ex.msg(e.what(), MsgPriority::CRITICAL, "DeepSort constructor exception: ");
	}
}

CPyObject av::DeepSort::getDetections(const std::vector<av::Box<float>>& boxes, float* buffer)
{
	CPyObject result;
	try {
		if (!PyArray_API) throw av::Exception("numpy not initialized");
		int rows = boxes.size();
		int cols = 5;

		for (int y = 0; y < rows; y++) {
			for (int x = 0; x < cols; x++) {
				int i = y * cols + x;
				buffer[i] = x;
				switch (x) {
				case 0:
					buffer[i] = boxes[y].x();
					break;
				case 1:
					buffer[i] = boxes[y].y();
					break;
				case 2:
					buffer[i] = boxes[y].w();
					break;
				case 3:
					buffer[i] = boxes[y].h();
					break;
				case 4:
					buffer[i] = boxes[y].prob();
					break;
				}
			}
		}

		npy_intp dimensions[2] = { rows, cols };
		CPyObject pData = PyArray_SimpleNewFromData(2, dimensions, NPY_FLOAT32, buffer);
		if (!pData) throw av::Exception("pData");
		result = Py_BuildValue("(O)", pData);
	}
	catch (const av::Exception& e) {
		ex.msg(e.what(), MsgPriority::CRITICAL, "av::DeepSort::getDetections exception: ");
	}
	return result;
}

CPyObject av::DeepSort::getImage(const cv::Mat& image)
{
	CPyObject result;
	try {
		if (!PyArray_API) throw av::Exception("numpy not initialized");

		npy_intp dimensions[3] = { image.rows, image.cols, image.channels() };
		CPyObject pData = PyArray_SimpleNewFromData(3, dimensions, NPY_UINT8, image.data); 
		if (!pData) throw av::Exception("pData");
		result = Py_BuildValue("(O)", pData);
	}
	catch (const av::Exception& e) {
		ex.msg(e.what(), MsgPriority::CRITICAL, "av::DeepSort::getImage exception: ");
	}
	return result;
}

void av::DeepSort::track(const cv::Mat& image, const std::vector<av::Box<float>>& dets)
{
	try {
		if (!pClass) throw av::Exception("pClass NULL");
		CPyObject pImage = getImage(image);
		int rows = dets.size();
		int cols = 5;
		float* buffer = new float[rows * cols];
		CPyObject pDets = getDetections(dets, buffer);
		CPyObject pArg = PyTuple_New(2);
		PyTuple_SetItem(pArg, 0, pImage);
		PyTuple_SetItem(pArg, 1, pDets);
		CPyObject pWrap = PyTuple_New(1);
		PyTuple_SetItem(pWrap, 0, pArg);
		PyObject* pValue = PyObject_CallObject(pClass, pWrap);
		if (pValue) {
			npy_intp* dims = PyArray_DIMS((PyArrayObject*)pValue);
			int height = dims[0];
			int width = dims[1];
			for (int i = 0; i < height; i++) {
				int track_id, x, y, w, h;
				for (int j = 0; j < width; j++) {
					void* ptr = PyArray_GETPTR2(pValue, i, j);
					CPyObject pElement = PyArray_GETITEM(pValue, ptr);
					double element = PyFloat_AsDouble(pElement);

					switch (j) {
					case 0:
						track_id = (int)element;
						break;
					case 1:
						x = (int)element;
						break;
					case 2:
						y = (int)element;
						break;
					case 3:
						w = (int)element;
						break;
					case 4:
						h = (int)element;
						break;
					}


				}
				cv::Point p1(x, y);
				cv::Point p2(x + w, y + h);
				cv::rectangle(image, p1, p2, cv::Scalar(255, 0, 0), 1);
				cv::putText(image, std::to_string(track_id), p1, cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1);
			}
		}
		else {
			throw av::Exception("pValue NULL");
		}
		delete[] buffer;
	}
	catch (const av::Exception& e) {
		ex.msg(e.what(), MsgPriority::CRITICAL, "av::DeepSort::track exception: ");
	}
}

