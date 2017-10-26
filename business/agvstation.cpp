#include "agvstation.h"

AgvStation::AgvStation(QObject *parent) : QObject(parent),
    m_x(0),
    m_y(0),
    m_type(0),
    m_name(""),
    m_lineAmount(0),
    m_id(0),
    m_rfid(0),
    m_occuAgv(0)
{

}
