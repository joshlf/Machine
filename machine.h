// Copyright 2013 The Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef MACHINE_INC
#define MACHINE_INC

#include <stdint.h>

typedef enum {
    RUN,        // State of a running machine
    HALT,       // State of a properly halted machine
    FAIL,       // State of a machine in a failure mode
    MEM,        // State of a machine which has exceeded the emulator's memory limits
    INTERN      // State of a machine which has encoundered an
                //   internal error or conflict in its emulation
                //   (ie, a problem with this library)
} state;

// Returns the state of the machine after execution has halted
// It is a bug for runMachine to return RUN, as runMachine should
// never return while the program is still running.
state runMachine(unsigned char *bin, uint32_t len);

#endif