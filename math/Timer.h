#pragma once

#ifdef _WIN32
#include <windows.h>

class Timer
{
public:
	Timer();

	//in seconds
	float GetElapsedTime();
	void Reset();

private:
	LONGLONG cur_time;

	DWORD time_count;
	LONGLONG perf_cnt;
	bool perf_flag;
	LONGLONG last_time;
	float time_scale;

	bool QPC;
};

inline void Timer::Reset()
{
	last_time = cur_time;
}


#else
//*****************************unix stuff****************************
#include <sys/time.h>

class Timer
{
public:
	Timer();

	void Reset();
	float GetElapsedTime();

private:
	timeval cur_time;

};

inline void Timer::Reset()
{
	gettimeofday(&cur_time, 0);
}



#endif //unix

// Courtesy Alan Gasperini, my roommate

#include "timer.h"

#ifdef _WIN32
#pragma comment(lib, "winmm.lib")
Timer::Timer()
{
	last_time=0;
	if(QueryPerformanceFrequency((LARGE_INTEGER*) &perf_cnt))
	{
		perf_flag=true;
		time_count=DWORD(perf_cnt); //perf_cnt counts per second
		QueryPerformanceCounter((LARGE_INTEGER*) &last_time);
		time_scale=1.0f/perf_cnt;
		QPC=true;
	}
	else
	{
		perf_flag=false;
		last_time=timeGetTime();
		time_scale=0.001f;
		time_count=33;
	}
}

float Timer::GetElapsedTime()
{
	if(perf_flag)
		QueryPerformanceCounter((LARGE_INTEGER*) &cur_time);
	else
		cur_time=timeGetTime();
				
	float time_elapsed=(cur_time-last_time)*time_scale;
	//last_time=cur_time;
	return time_elapsed;
}

#else
//***********************************unix specific*********************************
Timer::Timer()
{
	gettimeofday(&cur_time, 0);
}

float Timer::GetElapsedTime()
{
	float dif;
	timeval newtime;
	gettimeofday(&newtime, 0);
	dif=(newtime.tv_sec-cur_time.tv_sec);
	dif+=(newtime.tv_usec-cur_time.tv_usec)/1000000.0;
	return dif;
}

#endif // unix
