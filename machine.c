// Copyright 2013 The Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "machine.h"

// Type of a machine word
typedef uint32_t mword;

// Used for converting between signed
// and unsigned machine words
typedef union {
    mword unsign;
    int32_t sign;
} signConverter;

#define MAX_MWORD 0xFFFFFFFF

state loadMachine(unsigned char *bin, mword len);
state runner();
void  cleanup();

// Used to extract bit fields
typedef union {
    mword word;
    
    // Bit fields for normal instruction word
    struct {
        unsigned int c:4;
        unsigned int b:4;
        unsigned int a:4;
        unsigned int junk:16;
        unsigned int op:4;
    } fields;
    
    // Bit fields for load value instruction word
    struct {
        unsigned int val:24;
        unsigned int a:4;
        unsigned int op:4;
    } loadValueFields;
} instruction;

// Type of functions which handle instructions
typedef state(cmd)(instruction);

cmd runCmd;

// The name "div" conflicts with a stdlib function
// so the naming convention must be broken with "divide" and "sdivide"
cmd move, eq, gt, sgt, lt, slt, cjmp, load, store, add, sadd, sub, ssub, mult, smult, divide, sdivide, and, or, xor, not, hlt, out, in, lval;

// Enumeration of instruction op codes
typedef enum {
    MOVE,   // Move
    EQ,     // Equality
    GT,     // Greater Than
    SGT,    // Signed Greater Than
    LT,     // Less Than
    SLT,    // Signed Less Than
    CJMP,   // Conditional Jump
    LOAD,   // Load
    STORE,  // Store
    ADD,    // Add
    SADD,   // Signed Add
    SUB,    // Subtract
    SSUB,   // Signed Subtract
    MULT,   // Multiply
    SMULT,  // Signed Multiply
    DIVIDE, // Divide
    SDIV,   // Signed Multiply
    AND,    // Bitwise And
    OR,     // Bitwise Or
    XOR,    // Bitwise Exclusive Or
    NOT,    // Bitwise Complement
    HLT,    // Halt
    OUT,    // Output
    IN,     // Input
    LVAL    // Load Value
} INSTR;

// Registers
mword reg[16];

// Program counter
mword ctr = 0;

// Memory
mword *memory = NULL;
mword memory_size = 0;

state runMachine(unsigned char *bin, uint32_t len) {
    state ret = loadMachine(bin, len);
    if (ret != RUN) {
        cleanup();
        return ret;
    }
    ret = runner();
    cleanup();
    return ret;
}

// Return relevant error, or RUN upon success
state loadMachine(unsigned char *bin, mword len) {
    
    memory_size = bin[0];
    memory_size <<= 8;
    memory_size |= bin[1];
    memory_size <<= 8;
    memory_size |= bin[2];
    memory_size <<= 8;
    memory_size |= bin[3];
    
    if ((memory_size * 4) < (len - 1))
        return FAIL;
    
    // Use calloc so memory is zero'd
    memory = (mword*)calloc(memory_size, sizeof(*memory));
    
    if (memory == NULL)
        return MEM;
    
    memcpy(memory, bin + sizeof(*memory), len);
    
    // Zero out registers
    memset(reg, 0, sizeof(*reg) * 16);
    ctr = 0;
    
    return RUN;
}

void cleanup() {
    if (memory != NULL)
        free(memory);
}

state runner() {
    while (1) {
        if (ctr >= memory_size)
            return FAIL;
        
        instruction instr;
        instr.word = memory[ctr];
        
        // Increment counter before running instruction
        // in case the instruction is a load program
        ctr++;
        state st = runCmd(instr);
        if (st != RUN)
            return st;
    }
    
    // Should return from inside while loop
    return INTERN;
}

state runCmd(instruction instr) {
    switch (instr.fields.op) {
        case MOVE:
            return move(instr);
        case EQ:
            return eq(instr);
        case GT:
            return gt(instr);
        case SGT:
            return sgt(instr);
        case LT:
            return lt(instr);
        case SLT:
            return slt(instr);
        case CJMP:
            return cjmp(instr);
        case LOAD:
            return load(instr);
        case STORE:
            return store(instr);
        case ADD:
            return add(instr);
        case SADD:
            return sadd(instr);
        case SUB:
            return sub(instr);
        case SSUB:
            return ssub(instr);
        case MULT:
            return mult(instr);
        case SMULT:
            return smult(instr);
        case DIVIDE:
            return divide(instr);
        case SDIV:
            return sdivide(instr);
        case AND:
            return and(instr);
        case OR:
            return or(instr);
        case XOR:
            return xor(instr);
        case NOT:
            return not(instr);
        case HLT:
            return hlt(instr);
        case OUT:
            return out(instr);
        case IN:
            return in(instr);
        case LVAL:
            return lval(instr);
    }
    return FAIL;
}

state move(instruction instr) {
    reg[instr.fields.a] = reg[instr.fields.b];
    return RUN;
}

state eq(instruction instr) {
    reg[instr.fields.a] = reg[instr.fields.b] == reg[instr.fields.c];
    return RUN;
}

state gt(instruction instr) {
    reg[instr.fields.a] = reg[instr.fields.b] > reg[instr.fields.c];
    return RUN;
}

state sgt(instruction instr) {
    signConverter b, c;
    b.unsign = reg[instr.fields.b];
    c.unsign = reg[instr.fields.c];
    
    reg[instr.fields.a] = b.sign > c.sign;
    return RUN;
}

state lt(instruction instr) {
    reg[instr.fields.a] = reg[instr.fields.b] < reg[instr.fields.c];
    return RUN;
}

state slt(instruction instr) {
    signConverter b, c;
    b.unsign = reg[instr.fields.b];
    c.unsign = reg[instr.fields.c];
    
    reg[instr.fields.a] = b.sign < c.sign;
    return RUN;
}

state cjmp(instruction instr) {
    if (reg[instr.fields.a]) {
        ctr = reg[instr.fields.b];
    }
    return RUN;
}

state load(instruction instr) {
    if (reg[instr.fields.b] >= memory_size)
        return FAIL;
    reg[instr.fields.a] = memory[reg[instr.fields.b]];
    return RUN;
}

state store(instruction instr) {
    if (reg[instr.fields.a] >= memory_size)
        return FAIL;
    memory[reg[instr.fields.a]] = reg[instr.fields.b];
    return RUN;
}

state add(instruction instr) {
    reg[instr.fields.a] = reg[instr.fields.b] + reg[instr.fields.c];
    return RUN;
}

state sadd(instruction instr) {
    signConverter a, b, c;
    a.unsign = reg[instr.fields.a];
    b.unsign = reg[instr.fields.b];
    c.unsign = reg[instr.fields.c];
    
    a.sign = b.sign + c.sign;
    
    reg[instr.fields.a] = a.unsign;
    reg[instr.fields.b] = b.unsign;
    reg[instr.fields.c] = c.unsign;
    return RUN;
}

state sub(instruction instr) {
    reg[instr.fields.a] = reg[instr.fields.b] - reg[instr.fields.c];
    return RUN;
}

state ssub(instruction instr) {
    signConverter a, b, c;
    a.unsign = reg[instr.fields.a];
    b.unsign = reg[instr.fields.b];
    c.unsign = reg[instr.fields.c];
    
    a.sign = b.sign + c.sign;
    
    reg[instr.fields.a] = a.unsign;
    reg[instr.fields.b] = b.unsign;
    reg[instr.fields.c] = c.unsign;
    return RUN;
}

state mult(instruction instr) {
    reg[instr.fields.a] = reg[instr.fields.b] * reg[instr.fields.c];
    return RUN;
}

state smult(instruction instr) {
    signConverter a, b, c;
    a.unsign = reg[instr.fields.a];
    b.unsign = reg[instr.fields.b];
    c.unsign = reg[instr.fields.c];
    
    a.sign = b.sign * c.sign;
    
    reg[instr.fields.a] = a.unsign;
    reg[instr.fields.b] = b.unsign;
    reg[instr.fields.c] = c.unsign;
    return RUN;
}

state divide(instruction instr) {
    if (reg[instr.fields.c] == 0)
        return FAIL;
    reg[instr.fields.a] = reg[instr.fields.b] / reg[instr.fields.c];
    return RUN;
}

state sdivide(instruction instr) {
    if (reg[instr.fields.c] == 0)
        return FAIL;
    
    signConverter a, b, c;
    a.unsign = reg[instr.fields.a];
    b.unsign = reg[instr.fields.b];
    c.unsign = reg[instr.fields.c];
    
    a.sign = b.sign / c.sign;
    
    reg[instr.fields.a] = a.unsign;
    reg[instr.fields.b] = b.unsign;
    reg[instr.fields.c] = c.unsign;
    return RUN;
}

state and(instruction instr) {
    reg[instr.fields.a] = reg[instr.fields.b] & reg[instr.fields.c];
    return RUN;
}

state or(instruction instr) {
    reg[instr.fields.a] = reg[instr.fields.b] | reg[instr.fields.c];
    return RUN;
}

state xor(instruction instr) {
    reg[instr.fields.a] = reg[instr.fields.b] ^ reg[instr.fields.c];
    return RUN;
}

state not(instruction instr) {
    reg[instr.fields.a] = ~reg[instr.fields.b];
    return RUN;
}

state hlt(instruction instr) {
    (void)instr;
    return HALT;
}

state out(instruction instr) {
    if (reg[instr.fields.a] > 255)
        return FAIL;
    fprintf(stdout, "%c", reg[instr.fields.a]);
    return RUN;
}

state in(instruction instr) {
    int c = getc(stdin);
    if (c == EOF)
        reg[instr.fields.a] = MAX_MWORD;
    else
        reg[instr.fields.a] = c;
    return RUN;
}

state lval(instruction instr) {
    reg[instr.loadValueFields.a] = instr.loadValueFields.val;
    return RUN;
}