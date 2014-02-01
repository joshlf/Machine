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

typedef struct {
    state state;

    // m->registers
    mword reg[16];

    // Program counter
    mword ctr;

    // Memory
    mword *memory;
    mword memory_size;

    // Protected mode
    bool protected;
    mword lreg[16];
    mword callback;
    mword fault;
    mword lctr;
    mword vlow, vhigh;
    mword timer;

} machine;

void loadMachine(machine *m, unsigned char *bin, mword len);
void runner(machine *m);
void cleanup(machine *m);
void fault(machine *m, mword fcode);

// Used to extract bit fields
typedef union {
    mword word;
    
    // Bit fields for normal instruction word
    struct {
        unsigned int c:4;
        unsigned int b:4;
        unsigned int a:4;
        unsigned int junk:14;
        unsigned int op:6;
    } fields;
    
    // Bit fields for load value instruction word
    struct {
        unsigned int val:22;
        unsigned int a:4;
        unsigned int op:6;
    } loadValueFields;
} instruction;

// Type of functions which handle instructions
typedef state(cmd)(machine *m, instruction instr);

cmd runCmd;

// The name "div" conflicts with a stdlib function
// so the naming convention must be broken with "divide" and "sdivide"

// User mode
cmd move, eq, gt, sgt, lt, slt, cjmp, load, store, add, sadd, sub, ssub, mult, smult, divide, sdivide, and, or, xor, not, lshift, rshift, lval;

// Newly protected mode
cmd hlt, out, in;

// Protected mode
cmd umode, lload, lstore, scall, fmove, pclload, svmlow, svmhi, tload, tstore, trg;

// Enumeration of instruction op codes
enum {
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
    SUB,    // Subtract
    MULT,   // Multiply
    SMULT,  // Signed Multiply
    DIVIDE, // Divide
    SDIV,   // Signed Divide
    AND,    // Bitwise And
    OR,     // Bitwise Or
    XOR,    // Bitwise Exclusive Or
    NOT,    // Bitwise Complement
    LSHIFT, // Bitshift left
    RSHIFT, // Bitshift right
    HLT,    // Halt
    OUT,    // Output
    IN,     // Input
    LVAL,   // Load Value

    // Protected instructions
    UMODE,  // Enter user mode
    LLOAD,  // Lookaside load
    LSTORE, // Lookaside store
    SCALL,  // Set callback
    FMOVE,  // Fault move
    PCLLOAD,// PC Lookaside load
    SVMLOW, // Set virtual memory low
    SVMHI,  // Set virtual memory high
    TLOAD,  // PC Timer load
    TSTORE, // PC Timer store
    TRG     // Trigger
};

// Fault codes
enum {
    INSTR_FAULT,    // Protected instruction
    TRG_FAULT,      // Trigger
    TIME_FAULT,     // Decremented program counter time too far
    VM_FAULT,       // Accessed memory outside of set virtual memory
    VM_EXEC_FAULT,  // Executed an instruction outside of set virtual memory
    WORD_FAULT,     // Invalid instruction word
    DIV_ZERO_FAULT  // Divided by zero
};

state runMachine(unsigned char *bin, uint32_t len) {
    machine m;
    loadMachine(&m, bin, len);
    if (m.state != RUN) {
        cleanup(&m);
        return m.state;
    }
    runner(&m);
    cleanup(&m);
    return m.state;
}

void loadMachine(machine *m, unsigned char *bin, mword len) {
    
    m->memory_size = bin[0];
    m->memory_size <<= 8;
    m->memory_size |= bin[1];
    m->memory_size <<= 8;
    m->memory_size |= bin[2];
    m->memory_size <<= 8;
    m->memory_size |= bin[3];
    
    if ((m->memory_size * 4) < (len - 4)) {
        m->state = FAIL;
        return;
    }
    
    // Use calloc so memory is zero'd
    m->memory = (mword*)calloc(m->memory_size, sizeof(*(m->memory)));
    
    if (m->memory == NULL) {
        m->state = MEM;
        return;
    }
    
    unsigned char *b = bin + 4;
    mword newWord;
    for (int i = 0; b < bin + len; i++) {
        newWord = *b;
        b++;
        newWord <<= 8;
        newWord |= *b;
        b++;
        newWord <<= 8;
        newWord |= *b;
        b++;
        newWord <<= 8;
        newWord |= *b;
        b++;
        m->memory[i] = newWord;
    }
    
    // Zero out registers
    memset(m->reg, 0, sizeof(*(m->reg)) * 16);
    m->ctr = 0;
    m->state = RUN;
}

void cleanup(machine *m) {
    if (m->memory != NULL)
        free(m->memory);
}

void runner(machine *m) {
    while (1) {
        if (m->protected) {
            if (m->ctr >= m->memory_size) {
                m->state = FAIL;
                return;
            }
        } else {
            if (m->ctr < m->vlow || m->ctr > m->vhigh) {
                fault(m, VM_EXEC_FAULT);
                continue;
            }
        }

        // Grab the instruction word before
        // we increment the counter.
        instruction instr;
        instr.word = m->memory[m->ctr];
        
        // Increment counter before running instruction
        // in case the instruction is a load program
        m->ctr++;
        if (!m->protected) {
            // Decrement and then check because
            // fault increments.
            m->timer--;
            if (m->timer == MAX_MWORD) {
                fault(m, TIME_FAULT);
                continue;
            }
        }
        m->state = runCmd(m, instr);
        if (m->state != RUN)
            return;
    }
    
    // Should return from inside while loop
    m->state = INTERN;
}

void fault(machine *m, mword fcode) {
    // Since the runner decrements timer every time,
    // but timer is logically not decremented in the
    // event of a fault, correct that.
    m->timer++;

    memcpy(m->lreg, m->reg, sizeof(*(m->reg)) * 16);
    // TODO: Set fault register

    // Since the runner preemptively increments
    // the program counter, make sure to correct
    // for that.
    m->lctr = m->ctr - 1;
    m->fault = fcode;
    m->protected = true;
    m->ctr = m->callback;
}

state runCmd(machine *m, instruction instr) {
    switch (instr.fields.op) {
        case MOVE:
            return move(m, instr);
        case EQ:
            return eq(m, instr);
        case GT:
            return gt(m, instr);
        case SGT:
            return sgt(m, instr);
        case LT:
            return lt(m, instr);
        case SLT:
            return slt(m, instr);
        case CJMP:
            return cjmp(m, instr);
        case LOAD:
            return load(m, instr);
        case STORE:
            return store(m, instr);
        case ADD:
            return add(m, instr);
        case SUB:
            return sub(m, instr);
        case MULT:
            return mult(m, instr);
        case SMULT:
            return smult(m, instr);
        case DIVIDE:
            return divide(m, instr);
        case SDIV:
            return sdivide(m, instr);
        case AND:
            return and(m, instr);
        case OR:
            return or(m, instr);
        case XOR:
            return xor(m, instr);
        case NOT:
            return not(m, instr);
        case HLT:
            return hlt(m, instr);
        case OUT:
            return out(m, instr);
        case IN:
            return in(m, instr);
        case LVAL:
            return lval(m, instr);
        case UMODE:
            return umode(m, instr);
        case LLOAD:
            return lload(m, instr);
        case LSTORE:
            return lstore(m, instr);
        case SCALL:
            return scall(m, instr);
        case FMOVE:
            return fmove(m, instr);
        case PCLLOAD:
            return pclload(m, instr);
        case SVMLOW:
            return svmlow(m, instr);
        case SVMHI:
            return svmhi(m, instr);
        case TLOAD:
            return tload(m, instr);
        case TSTORE:
            return tstore(m, instr);
        case TRG:
            return trg(m, instr);
    }
    if (m->protected)
        return FAIL;
    fault(m, WORD_FAULT);
    return RUN;
}

// Move
state move(machine *m, instruction instr) {
    m->reg[instr.fields.a] = m->reg[instr.fields.b];
    return RUN;
}

// Equals
state eq(machine *m, instruction instr) {
    m->reg[instr.fields.a] = m->reg[instr.fields.b] == m->reg[instr.fields.c];
    return RUN;
}

// Greater than
state gt(machine *m, instruction instr) {
    m->reg[instr.fields.a] = m->reg[instr.fields.b] > m->reg[instr.fields.c];
    return RUN;
}

// Signed greater than
state sgt(machine *m, instruction instr) {
    signConverter b, c;
    b.unsign = m->reg[instr.fields.b];
    c.unsign = m->reg[instr.fields.c];
    
    m->reg[instr.fields.a] = b.sign > c.sign;
    return RUN;
}

// Less than
state lt(machine *m, instruction instr) {
    m->reg[instr.fields.a] = m->reg[instr.fields.b] < m->reg[instr.fields.c];
    return RUN;
}

// Signed less than
state slt(machine *m, instruction instr) {
    signConverter b, c;
    b.unsign = m->reg[instr.fields.b];
    c.unsign = m->reg[instr.fields.c];
    
    m->reg[instr.fields.a] = b.sign < c.sign;
    return RUN;
}

// Conditional jump
state cjmp(machine *m, instruction instr) {
    if (m->reg[instr.fields.a]) {
        m->ctr = m->reg[instr.fields.b];
    }
    return RUN;
}

// Load
state load(machine *m, instruction instr) {
    if (m->protected) {
        if (m->reg[instr.fields.b] >= m->memory_size)
            return FAIL;
    } else {
        if (m->reg[instr.fields.b] < m->vlow || m->reg[instr.fields.b] > m->vhigh) {
            fault(m, VM_FAULT);
            return RUN;
        }
    }

    m->reg[instr.fields.a] = m->memory[m->reg[instr.fields.b]];
    return RUN;
}

// Store
state store(machine *m, instruction instr) {
    if (m->protected) {
        if (m->reg[instr.fields.b] >= m->memory_size)
            return FAIL;
    } else {
        if (m->reg[instr.fields.b] < m->vlow || m->reg[instr.fields.b] > m->vhigh) {
            fault(m, VM_FAULT);
            return RUN;
        }
    }

    m->memory[m->reg[instr.fields.a]] = m->reg[instr.fields.b];
    return RUN;
}

// Add
state add(machine *m, instruction instr) {
    m->reg[instr.fields.a] = m->reg[instr.fields.b] + m->reg[instr.fields.c];
    return RUN;
}

// Subtract
state sub(machine *m, instruction instr) {
    m->reg[instr.fields.a] = m->reg[instr.fields.b] - m->reg[instr.fields.c];
    return RUN;
}

// Multiply
state mult(machine *m, instruction instr) {
    m->reg[instr.fields.a] = m->reg[instr.fields.b] * m->reg[instr.fields.c];
    return RUN;
}

// Signed multiply
state smult(machine *m, instruction instr) {
    signConverter a, b, c;
    a.unsign = m->reg[instr.fields.a];
    b.unsign = m->reg[instr.fields.b];
    c.unsign = m->reg[instr.fields.c];
    
    a.sign = b.sign * c.sign;
    
    m->reg[instr.fields.a] = a.unsign;
    m->reg[instr.fields.b] = b.unsign;
    m->reg[instr.fields.c] = c.unsign;
    return RUN;
}

// Divide
state divide(machine *m, instruction instr) {
    if (m->protected) {
        if (m->reg[instr.fields.c] == 0)
            return FAIL;;
    } else {
        if (m->reg[instr.fields.c] == 0) {
            fault(m, DIV_ZERO_FAULT);
            return RUN;
        }
    }

    m->reg[instr.fields.a] = m->reg[instr.fields.b] / m->reg[instr.fields.c];
    return RUN;
}

// Signed divide
state sdivide(machine *m, instruction instr) {
    if (m->protected) {
        if (m->reg[instr.fields.c] == 0)
            return FAIL;;
    } else {
        if (m->reg[instr.fields.c] == 0) {
            fault(m, DIV_ZERO_FAULT);
            return RUN;
        }
    }
    
    signConverter a, b, c;
    a.unsign = m->reg[instr.fields.a];
    b.unsign = m->reg[instr.fields.b];
    c.unsign = m->reg[instr.fields.c];
    
    a.sign = b.sign / c.sign;
    
    m->reg[instr.fields.a] = a.unsign;
    m->reg[instr.fields.b] = b.unsign;
    m->reg[instr.fields.c] = c.unsign;
    return RUN;
}

// And
state and(machine *m, instruction instr) {
    m->reg[instr.fields.a] = m->reg[instr.fields.b] & m->reg[instr.fields.c];
    return RUN;
}

// Or
state or(machine *m, instruction instr) {
    m->reg[instr.fields.a] = m->reg[instr.fields.b] | m->reg[instr.fields.c];
    return RUN;
}

// Exclusive or
state xor(machine *m, instruction instr) {
    m->reg[instr.fields.a] = m->reg[instr.fields.b] ^ m->reg[instr.fields.c];
    return RUN;
}

// Not
state not(machine *m, instruction instr) {
    m->reg[instr.fields.a] = ~m->reg[instr.fields.b];
    return RUN;
}

// Left shift
state lshift(machine *m, instruction instr) {
    m->reg[instr.fields.a] = m->reg[instr.fields.b] << m->reg[instr.fields.c];
    return RUN;
}

// Right shift
state rshift(machine *m, instruction instr) {
    m->reg[instr.fields.a] = m->reg[instr.fields.b] >> m->reg[instr.fields.c];
    return RUN;
}

// Halt
state hlt(machine *m, instruction instr) {
    if (!m->protected) {
        fault(m, INSTR_FAULT);
        return RUN;
    }
    (void)instr;
    return HALT;
}

// Output
state out(machine *m, instruction instr) {
    if (!m->protected) {
        fault(m, INSTR_FAULT);
        return RUN;
    }

    if (m->reg[instr.fields.a] > 255)
        return FAIL;
    fprintf(stdout, "%c", m->reg[instr.fields.a]);
    return RUN;
}

// Input
state in(machine *m, instruction instr) {
    if (!m->protected) {
        fault(m, INSTR_FAULT);
        return RUN;
    }

    int c = getc(stdin);
    if (c == EOF)
        m->reg[instr.fields.a] = MAX_MWORD;
    else
        m->reg[instr.fields.a] = c;
    return RUN;
}

// Load value
state lval(machine *m, instruction instr) {
    m->reg[instr.loadValueFields.a] = instr.loadValueFields.val;
    return RUN;
}

// User mode
state umode(machine *m, instruction instr) {
    if (!m->protected) {
        fault(m, INSTR_FAULT);
        return RUN;
    }
    m->ctr = m->reg[instr.fields.a];
    memcpy(m->reg, m->lreg, sizeof(*(m->reg)) * 16);
    return RUN;
}

// Lookaside load
state lload(machine *m, instruction instr) {
    if (!m->protected) {
        fault(m, INSTR_FAULT);
        return RUN;
    }
    m->reg[instr.fields.a] = m->lreg[instr.fields.b];
    return RUN;
}

// Lookaside store
state lstore(machine *m, instruction instr) {
    if (!m->protected) {
        fault(m, INSTR_FAULT);
        return RUN;
    }
    m->lreg[instr.fields.a] = m->reg[instr.fields.b];
    return RUN;
}

// Set callback
state scall(machine *m, instruction instr) {
    if (!m->protected) {
        fault(m, INSTR_FAULT);
        return RUN;
    }
    m->callback = m->reg[instr.fields.a];
    return RUN;
}

// Fault move
state fmove(machine *m, instruction instr) {
    if (!m->protected) {
        fault(m, INSTR_FAULT);
        return RUN;
    }
    m->reg[instr.fields.a] = m->fault;
    return RUN;
}

// Program counter lookaside load
state pclload(machine *m, instruction instr) {
    if (!m->protected) {
        fault(m, INSTR_FAULT);
        return RUN;
    }
    m->reg[instr.fields.a] = m->lctr;
    return RUN;
}

// Store virtual memory low
state svmlow(machine *m, instruction instr) {
    if (!m->protected) {
        fault(m, INSTR_FAULT);
        return RUN;
    }
    m->vlow = m->reg[instr.fields.a];
    return RUN;
}

// Store virtual memory high
state svmhi(machine *m, instruction instr) {
    if (!m->protected) {
        fault(m, INSTR_FAULT);
        return RUN;
    }
    m->vhigh = m->reg[instr.fields.a];
    return RUN;
}

// Program counter timer load
state tload(machine *m, instruction instr) {
    if (!m->protected) {
        fault(m, INSTR_FAULT);
        return RUN;
    }
    m->reg[instr.fields.a] = m->timer;
    return RUN;
}

// Program counter timer store
state tstore(machine *m, instruction instr) {
    if (!m->protected) {
        fault(m, INSTR_FAULT);
        return RUN;
    }
    m->timer = m->reg[instr.fields.a];
    return RUN;
}

// Trigger
state trg(machine *m, instruction instr) {
    if (!m->protected) {
        fault(m, TRG_FAULT);
        return RUN;
    }
    return RUN;
}