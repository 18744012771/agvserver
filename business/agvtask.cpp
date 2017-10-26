#include "agvtask.h"

AgvTask::AgvTask(QObject *parent) : QObject(parent),
    m_id(0),
    m_aimStation(0),
    m_type(0),
    m_pickupStation(0),
    m_excuteCar(0),
    m_status(0),
    m_waitTypePick(0),
    m_waitTypeAim(0),
    m_waitTimePick(0),
    m_waitTimeAim(0),
    m_goPickGoAim(false)
{

}
