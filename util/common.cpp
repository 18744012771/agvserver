#include "common.h"

#ifdef WIN32
#include <windows.h>
#endif

void TimeSleep(int sleepMS)
{
#ifdef WIN32
    Sleep(sleepMS);
#else
    usleep(sleepMS * 1000);
#endif
}


int GetTimeTick()
{
    int tickNow = 0;

#ifdef WIN32
    tickNow = GetTickCount();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    tickNow = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#endif
    return tickNow;
}


int GetTimeSecond()
{
    int tickNow = 0;

#ifdef WIN32
    tickNow = GetTimeTick() / 1000;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    tickNow = ts.tv_sec;
#endif
    return tickNow;
}

unsigned char checkSum(unsigned char *data,int len)
{
    int sum = 0;
    for(int i=0;i<len;++i){
        //int v = data[i] &0xFF;
        sum += data[i] & 0xFF;
        sum &= 0xFF;
    }
    return sum & 0xFF;
}
