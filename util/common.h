#ifndef COMMON_H
#define COMMON_H

void TimeSleep(int sleepMS);

int GetTimeTick();

int GetTimeSecond();

unsigned char checkSum(unsigned char *data, int len);

#endif // COMMON_H
