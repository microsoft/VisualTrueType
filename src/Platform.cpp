// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#define _CRT_SECURE_NO_WARNINGS

#include "pch.h"

#include <time.h>
#include <sys/timeb.h> // _timeb, _ftime
#include <cstdlib> // mbstowcs
#include <cstring> // strlen

#define TIME_FIX 2082844800ll // pc time 1970 - mac time 1904

long long DateTime(void) {
	struct timeb  tstruct;

	//	_tzset();
	ftime(&tstruct);

	//	it seems that we don't have to worry about the time zone, _ftime does that for us	
	return (long long)tstruct.time + TIME_FIX /* 60*(int)(tstruct.timezone) */;
} // DateTime

void DateTimeStrg(wchar_t strg[]) {
	time_t dateTime;

	time(&dateTime);
#ifndef _WIN32
       char *cstring = ctime(&dateTime);
       mbstowcs(strg, cstring, strlen(cstring));
#else
	STRCPYW(strg, _wctime(&dateTime));
#endif
	strg[STRLENW(strg) - 1] = L'\0'; // get rid of \n...
} // DateTimeStrg