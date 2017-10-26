#include "agvline.h"

AgvLine::AgvLine(QObject *parent) : QObject(parent),
    m_startX(0),
    m_startY(0),
    m_endX(0),
    m_endY(0),
    m_radius(0),
    m_clockwise(false),
    m_line(false),
    m_midX(0),
    m_midY(0),
    m_centerX(0),
    m_centerY(0),
    m_angle(0),
    m_id(0),
    m_draw(false),
    m_occuAgv(0),
    m_length(0)
{

}
