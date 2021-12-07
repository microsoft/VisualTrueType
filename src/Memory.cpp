// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#define _CRT_SECURE_NO_DEPRECATE 

#include <stdio.h> // swprintf
#include <stdlib.h>
#include "Memory.h"

void *NewP(size_t bytes) {	
	void *ptr = malloc(bytes);	
	return ptr;
} // NewP

void DisposeP(void** ptr) {	
	free(*ptr);
	*ptr = NULL;
} // DisposeP

