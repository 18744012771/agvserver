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

int getInt32FromByte(char *data)
{
    int j = data[0] & 0x000000ff;
    j |= ((data[1] << 8) & 0x0000ff00);
    j |= ((data[2] << 16) & 0x00ff0000);
    j |= ((data[3] << 24) & 0xff000000);
    return j;
}

uint16_t getInt16FromByte(char *data)
{
    uint16_t j = data[0] & 0x000000ff;
    j |= ((data[1] << 8) & 0x0000ff00);
    return j;
}

uint8_t getInt8FromByte(char *data)
{
    return (uint8_t)(data[0]);
}
