// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#define _CRT_SECURE_NO_WARNINGS

#include "pch.h"

#include <time.h>
#include <sys/timeb.h> // _timeb, _ftime

#define TIME_FIX 2082844800i64 // pc time 1970 - mac time 1904

long long DateTime(void) {
	struct _timeb  tstruct;

	//	_tzset();
	_ftime(&tstruct);

	//	it seems that we don't have to worry about the time zone, _ftime does that for us	
	return (long long)tstruct.time + TIME_FIX /* 60*(long)(tstruct.timezone) */;
} // DateTime

void DateTimeStrg(wchar_t strg[]) {
	time_t dateTime;

	time(&dateTime);
	STRCPYW(strg, _wctime(&dateTime));
	strg[STRLENW(strg) - 1] = L'\0'; // get rid of \n...
} // DateTimeStrg