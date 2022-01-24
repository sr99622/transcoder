#pragma once

#include "yolo_v2_class.hpp"

namespace av
{

template <typename T>
class Box
{
public:
	Box(T x=0, T y=0, T w=0, T h=0, int obj_id = -1, float prob = 0) :
		m_x(x), m_y(y), m_w(w), m_h(h), m_obj_id(obj_id), m_prob(prob) { };

	Box(bbox_t b) :
		m_x(b.x), m_y(b.y), m_w(b.w), m_h(b.h), m_obj_id(b.obj_id), m_prob(b.prob) { };

	void setByXY(T x1, T y1, T x2, T y2, int obj_id = -1, float prob = 0)
	{
		m_x = x1;
		m_y = y1;
		m_w = x2 - x1;
		m_h = y2 - y1;
		m_obj_id = obj_id;
		m_prob = prob;
	}

	T x() { return m_x; }
	T y() { return m_y; }
	T w() { return m_w; }
	T h() { return m_h; }
	T x1() { return m_x; }
	T y1() { return m_y; }
	T x2() { return m_x + m_w; }
	T y2() { return m_y + m_h; }
	float prob() { return m_prob; }
	int obj_id() { return m_obj_id; }

	void setX(T x) { m_x = x; }
	void setY(T y) { m_y = y; }
	void setW(T w) { m_w = w; }
	void setH(T h) { m_h = h; }
	void setX1(T x1) { m_x = x1; }
	void setY1(T y1) { m_y = y1; }
	void setX2(T x2) { m_w = x2 - m_x; }
	void setY2(T y2) { m_h = y2 - m_y; }
	void setProb(float p) { m_prob = p; }
	void setObjID(int id) { m_obj_id = id; }

	bbox_t to_bbox()
	{
		bbox_t b;
		b.x = x();
		b.y = y();
		b.w = w();
		b.h = h();
		return b;
	}

private:
	T m_x, m_y, m_w, m_h;
	float m_prob;
	int m_obj_id;
};

}
