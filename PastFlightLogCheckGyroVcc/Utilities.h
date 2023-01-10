#pragma once


#include <cmath>
#include <string>


float Mean(float* x, int count);

float CrossCorrelation(float* x, float* y, int count);

std::string MillisecondsToTimeStamp(uint64_t elapsed_time, char separator, bool mills);