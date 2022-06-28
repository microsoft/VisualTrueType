// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#define _CRT_SECURE_NO_WARNINGS

#include "pch.h"

#include <cstdlib> // mbstowcs
#include <cstring> // strlen
#include <chrono>

#define TIME_FIX 2082844800ll // pc time 1970 - mac time 1904

using namespace std::chrono;

long long DateTime(void) {
	auto timepoint = std::chrono::system_clock::now();
	auto since_epoch =  timepoint.time_since_epoch();
	auto secs = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);

	//	it seems that we don't have to worry about the time zone, _ftime does that for us	
	return secs.count() + TIME_FIX /* 60*(long)(tstruct.timezone) */;
} // DateTime

void DateTimeStrg(wchar_t strg[]) {
	time_t dateTime;

	time(&dateTime);
#ifndef _WIN32
       char *cstring = ctime(&dateTime);
       mbstowcs(strg, cstring, strlen(cstring) + 1 /* the final '\0' */);
#else
	STRCPYW(strg, _wctime(&dateTime));
#endif
	strg[STRLENW(strg) - 1] = L'\0'; // get rid of \n...
} // DateTimeStrg
