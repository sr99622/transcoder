#define NO_IMPORT_ARRAY
#include "PyModelSSD.h"

PyModelSSD::PyModelSSD
(
	const char* model_dir, av::Queue<av::Frame>* frame_q_out, int threshold
) :
	frame_out_q(frame_out_q)
{
	try {
		if (!Py_IsInitialized()) throw av::Exception("Python not initialized");

		CPyObject pName = PyUnicode_FromString("model");        if (!pName) throw av::Exception("pName");
		CPyObject pModule = PyImport_Import(pName);             if (!pModule) throw av::Exception("pModule");
		CPyObject pDict = PyModule_GetDict(pModule);            if (!pDict) throw av::Exception("pDict");
		CPyObject pItem = PyDict_GetItemString(pDict, "Model"); if (!pItem) throw av::Exception("pItem");
		pClass = PyObject_CallObject(pItem, NULL);              if (!pClass) throw av::Exception("pClass");

		if (! PyObject_CallMethod(pClass, "set_threshold", "(i)", threshold))    throw av::Exception("pCallHandle2");
		if (! PyObject_CallMethod(pClass, "initialize_model", "(s)", model_dir)) throw av::Exception("pCallHandle1");
	}
	catch (const av::Exception& e) {
		ex.msg(e.what(), MsgPriority::CRITICAL, "PyModelSSD constructor exception: ");
	}
}

void PyModelSSD::setThreshold(int threshold)
{
	CPyObject pCallHandle2 = PyObject_CallMethod(pClass, "set_threshold", "(i)", threshold);
}

CPyObject PyModelSSD::getImage(const cv::Mat& image)
{
	CPyObject result;
	try {
		if (!PyArray_API) throw av::Exception("numpy not initialized");

		npy_intp dimensions[3] = { image.rows, image.cols, image.channels() };
		CPyObject pData = PyArray_SimpleNewFromData(3, dimensions, NPY_UINT8, image.data); if (!pData) throw av::Exception("pData");
		result = Py_BuildValue("(O)", pData);
	}
	catch (const av::Exception& e) {
		ex.msg(e.what(), MsgPriority::CRITICAL, "PyModelSSD::getImage ");
	}
	return result;
}

void PyModelSSD::detect(const cv::Mat& image, std::vector<av::Box<int>>* boxes)
{
	try {
		PyObject* pValue = PyObject_CallObject(pClass, getImage(image));
		if (pValue) {
			npy_intp* dims = PyArray_DIMS((PyArrayObject*)pValue);
			int height = dims[0];
			int width = dims[1];

			int top = 0, left = 0, bottom = 0, right = 0;
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					void* ptr = PyArray_GETPTR2(pValue, y, x);
					CPyObject pElement = PyArray_GETITEM(pValue, ptr);
					double element = PyFloat_AsDouble(pElement);

					switch (x) {
					case 0:
						top = element;
						break;
					case 1:
						left = element;
						break;
					case 2:
						bottom = element;
						break;
					case 3:
						right = element;
						break;
					}
				}
				av::Box<int> b;
				b.setByXY(left, top, right, bottom);
				boxes->push_back(b);
			}
		}
		else {
			throw av::Exception("pValue");
		}
	}
	catch (const av::Exception& e) {
		ex.msg(e.what(), MsgPriority::CRITICAL, "PyModelSSD::detect ");
	}
}
