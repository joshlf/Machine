// Copyright 2013 The Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include "machine.h"

// Exit codes
#define NORMAL   0
#define USAGE    1
#define FILEIO   2
#define FAILURE  3
#define MEMORY   4
#define INTERNAL 5

int main (int argc, const char * argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <binary>\n", argv[0]);
        return USAGE;
    }
    
    FILE *f = fopen(argv[1], "rb");
    
    if (f == NULL) {
        fprintf(stderr, "Could not open file: %s\n", argv[1]);
        return FILEIO;
    }
    
    // Get length of file
    fseek(f, 0, SEEK_END);
    long int len = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    // Allocate buffer for file
    unsigned char *bin = (unsigned char*)malloc(len * sizeof(*bin));
    
    // Read file into buffer
    fread(bin, 1, len, f);
    fclose(f);
    
    state st = runMachine(bin, (uint32_t)len);
    
    free(bin);
    
    switch (st) {
        case HALT:
            #ifdef DEBUG
                fprintf(stderr, "\n---\nProgram halted normally.\n");
            #endif
            return NORMAL;
        case FAIL:
            #ifdef DEBUG
                fprintf(stderr, "\n---\nProgram entered a failure mode and has been halted.\n");
            #endif
            return FAILURE;
        case MEM:
            #ifdef DEBUG
                fprintf(stderr, "\n---\nProgram exceeded memory usage.\n");
            #endif
            return MEMORY;
        case INTERN:
            #ifdef DEBUG
                fprintf(stderr, "\n---\nInternal error encountered.\n");
            #endif
            return INTERNAL;
        
        // Prevent compiler warning
        case RUN:
            break;
    }
    #ifdef DEBUG
        fprintf(stderr, "\n---\nInternal error encountered: impossible exit code returned: %d\n", st);
    #endif
    
    return INTERNAL;
}
