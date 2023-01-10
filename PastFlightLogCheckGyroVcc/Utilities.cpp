#include "Utilities.h"


float Mean(float* x, int count)
{
	float result = 0.0f;
	if (count == 0)
	{
		return result;
	}
	for (int i = 0; i < count; i++)
	{
		result += x[i];
	}
	result /= count;
	return result;
}


float CrossCorrelation(float* x, float* y, int count)
{
	float result = 0.0f;
	float mx = Mean(x, count);
	float my = Mean(y, count);
	float numerator = 0.0f;
	for (int i = 0; i < count; i++)
	{
		numerator += ((x[i] - mx) * (y[i] - my));
	}
	float denominator_x = 0.0f;
	for (int i = 0; i < count; i++)
	{
		denominator_x += ((x[i] - mx) * (x[i] - mx));
	}
	float denominator_y = 0.0f;
	for (int i = 0; i < count; i++)
	{
		denominator_y += ((y[i] - my) * (y[i] - my));
	}
	float denominator = sqrtf(denominator_x) * sqrtf(denominator_y);

	if (denominator == 0.0f)
	{
		return result;
	}
	result = (numerator / denominator);
	return result;
}


std::string MillisecondsToTimeStamp(uint64_t elapsed_time, char separator, bool mills)
{
	uint64_t hours = (elapsed_time / 1000 / 60 / 60) % 24;
	uint64_t minutes = (elapsed_time / 1000 / 60) % 60;
	uint64_t seconds = (elapsed_time / 1000) % 60;
	uint64_t milliseconds = elapsed_time % 1000;

	std::string milliseconds_filler = (milliseconds < 100) ? (milliseconds < 10 ? "00" : "0") : "";
	std::string seconds_filler = (seconds < 10) ? "0" : "";
	std::string minutes_filler = (minutes < 10) ? "0" : "";
	std::string hours_filler = (hours < 10) ? "0" : "";

	std::string timeStamp =
		hours_filler + std::to_string(hours) + separator +
		minutes_filler + std::to_string(minutes) + separator +
		seconds_filler + std::to_string(seconds);

	if (mills)
	{
		timeStamp += separator;
		timeStamp += milliseconds_filler;
		timeStamp += std::to_string(milliseconds);
	}

	return timeStamp;
}