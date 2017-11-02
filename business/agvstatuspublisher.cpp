#include "agvstatuspublisher.h"
#include <algorithm>
#include <sstream>
#include "util/global.h"

AgvStatusPublisher::AgvStatusPublisher(QObject *parent) : QThread(parent),isQuit(false)
{

}

AgvStatusPublisher::~AgvStatusPublisher()
{
    isQuit = true;
}

void AgvStatusPublisher::addSubscribe(int subscribe)
{
    mutex.lock();
    subscribers.push_back(subscribe);
    mutex.unlock();
}

void AgvStatusPublisher::removeSubscribe(int subscribe)
{
    mutex.lock();
    std::list<int>::iterator itr;
    if((itr = std::find(subscribers.begin(),subscribers.end(),subscribe))!=subscribers.end())
        subscribers.erase(itr);
    mutex.unlock();
}

void AgvStatusPublisher::run()
{
    while(!isQuit)
    {
        if(subscribers.size()==0||g_m_agvs.size()==0){
            QyhSleep(400);
            continue;
        }

        //组装订阅信息
        std::map<std::string,std::string> responseDatas;
        std::vector<std::map<std::string,std::string> > responseDatalists;

        responseDatas.insert(std::make_pair(std::string("type"),std::string("agv")));
        responseDatas.insert(std::make_pair(std::string("todo"),std::string("periodica")));


        for(QMap<int,Agv *>::iterator itr = g_m_agvs.begin();itr!=g_m_agvs.end();++itr)
        {
            std::map<std::string,std::string> mm;

            std::stringstream ss_speed;
            std::string str_speed;
            ss_speed<<itr.value()->speed();
            ss_speed>>str_speed;
            mm.insert(std::make_pair(std::string("speed"),str_speed));

            std::stringstream ss_turnSpeed;
            std::string str_turnSpeed;
            ss_turnSpeed<<itr.value()->turnSpeed();
            ss_turnSpeed>>str_turnSpeed;
            mm.insert(std::make_pair(std::string("turnSpeed"),str_turnSpeed));

            std::stringstream ss_cpu;
            std::string str_cpu;
            ss_cpu<<itr.value()->cpu();
            ss_cpu>>str_cpu;
            mm.insert(std::make_pair(std::string("cpu"),str_cpu));

            std::stringstream ss_status;
            std::string str_status;
            ss_status<<itr.value()->status();
            ss_status>>str_status;
            mm.insert(std::make_pair(std::string("status"),str_status));

            std::stringstream ss_leftMotorStatus;
            std::string str_leftMotorStatus;
            ss_leftMotorStatus<<itr.value()->leftMotorStatus();
            ss_leftMotorStatus>>str_leftMotorStatus;
            mm.insert(std::make_pair(std::string("leftMotorStatus"),str_leftMotorStatus));

            std::stringstream ss_rightMotorStatus;
            std::string str_rightMotorStatus;
            ss_rightMotorStatus<<itr.value()->rightMotorStatus();
            ss_rightMotorStatus>>str_rightMotorStatus;
            mm.insert(std::make_pair(std::string("rightMotorStatus"),str_rightMotorStatus));

            std::stringstream ss_systemVoltage;
            std::string str_systemVoltage;
            ss_systemVoltage<<itr.value()->systemVoltage();
            ss_systemVoltage>>str_systemVoltage;
            mm.insert(std::make_pair(std::string("systemVoltage"),str_systemVoltage));

            std::stringstream ss_systemCurrent;
            std::string str_systemCurrent;
            ss_systemCurrent<<itr.value()->systemCurrent();
            ss_systemCurrent>>str_systemCurrent;
            mm.insert(std::make_pair(std::string("systemCurrent"),str_systemCurrent));

            std::stringstream ss_positionMagneticStripe;
            std::string str_positionMagneticStripe;
            ss_positionMagneticStripe<<itr.value()->positionMagneticStripe();
            ss_positionMagneticStripe>>str_positionMagneticStripe;
            mm.insert(std::make_pair(std::string("positionMagneticStripe"),str_positionMagneticStripe));

            std::stringstream ss_frontObstruct;
            std::string str_frontObstruct;
            ss_frontObstruct<<itr.value()->frontObstruct();
            ss_frontObstruct>>str_frontObstruct;
            mm.insert(std::make_pair(std::string("frontObstruct"),str_frontObstruct));

            std::stringstream ss_backObstruct;
            std::string str_backObstruct;
            ss_backObstruct<<itr.value()->backObstruct();
            ss_backObstruct>>str_backObstruct;
            mm.insert(std::make_pair(std::string("backObstruct"),str_backObstruct));

            std::stringstream ss_currentOrder;
            std::string str_currentOrder;
            ss_currentOrder<<itr.value()->currentOrder();
            ss_currentOrder>>str_currentOrder;
            mm.insert(std::make_pair(std::string("currentOrder"),str_currentOrder));

            std::stringstream ss_currentQueueNumber;
            std::string str_currentQueueNumber;
            ss_currentQueueNumber<<itr.value()->currentQueueNumber();
            ss_currentQueueNumber>>str_currentQueueNumber;
            mm.insert(std::make_pair(std::string("currentQueueNumber"),str_currentQueueNumber));

            std::stringstream ss_ip;
            std::string str_ip;
            ss_ip<<itr.value()->ip().toStdString();
            ss_ip>>str_ip;
            mm.insert(std::make_pair(std::string("ip"),str_ip));

            std::stringstream ss_mileage;
            std::string str_mileage;
            ss_mileage<<itr.value()->mileage();
            ss_mileage>>str_mileage;
            mm.insert(std::make_pair(std::string("mileage"),str_mileage));

            std::stringstream ss_rad;
            std::string str_rad;
            ss_rad<<itr.value()->rad();
            ss_rad>>str_rad;
            mm.insert(std::make_pair(std::string("rad"),str_rad));

            std::stringstream ss_currentRfid;
            std::string str_currentRfid;
            ss_currentRfid<<itr.value()->currentRfid();
            ss_currentRfid>>str_currentRfid;
            mm.insert(std::make_pair(std::string("currentRfid"),str_currentRfid));


            responseDatalists.push_back(mm);
        }
        std::string xml = getResponseXml(responseDatas,responseDatalists);

        //发送订阅信息
        mutex.lock();
        for(std::list<int>::iterator itr = subscribers.begin();itr!=subscribers.end();++itr)
        {
            g_netWork->sendToOne(*itr,xml.c_str(),xml.length());
        }
        mutex.unlock();

        QyhSleep(100);
    }
}
