#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>


void TimeSleep(int sleepMS);

int GetTimeTick();

int GetTimeSecond();

unsigned char checkSum(unsigned char *data, int len);

int getInt32FromByte(char *data);

uint16_t getInt16FromByte(char *data);

uint8_t getInt8FromByte(char *data);
#endif // COMMON_H
