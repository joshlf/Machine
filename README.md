<!--
Copyright 2013 The Authors. All rights reserved.
Use of this source code is governed by a BSD-style
license that can be found in the LICENSE file.
-->

Machine
=======

An emulator for computer with a simple architecture and instruction set.

##Overview
Machine emulates a computer with a simple memory architecture and small instruction set. It is intended to be used in an educational or research setting, allowing binaries to be run universally and without concern for physical architecture. It is written primarily for my own use in testing new compiler or assembler features, but I would be thrilled if others got use out of it as well. If you have suggested changes or bugs, please let me know!

There are two overall pieces to Machine. The first is a fully-functioning architecture and instruction set that programs wishing to run directly in Machine should use. The second is a set of architecture and instruction set extensions that add support for a protected mode/user mode distinction, which allows kernels to be written for Machine. These extensions are fully backwards-compatible with the normal subset, and any programs written without knowledge of these extensions will run without issue. Note that there's only one version of Machine - the version that includes the extensions - but programs may treat Machine as though it logically only implemented the normal subset.

##Architecture
Machine is a 32-bit architecture. It comprises three simple components - memory, registers, and a program counter.

###Registers
There are 16 registers, each holding a single 32-bit word. The nth register is denoted r[n].

###Memory
Machine has a 32-bit memory address space. It is word-addressed, with each address referencing a single 32-bit word. The nth word in memory is denoted m[n]. The number of words actually available to be written to or read from is determined by the program which is run.

###Program Counter
Machine keeps a counter which holds the address of the word in memory which is to be executed next. The counter is incremented before each execution.

##Binaries
Binaries are files containing the words of a program, in big-endian byte order. The format for binaries is quite simple. The first word specifies how many words will be available in memory. Its minimum value is one less than the size of the binary.

To begin, memory of the proper size is allocated and initialized to zero. All of the registers are also intialized to zero. Then, the words of the binary are loaded into memory starting with word 1 (this excludes the first word of the binary, the one which encodes memory length). Word 1 of the binary is loaded into word 0 of memory, and so on. The program counter is then set to point at word 0 of memory.

Execution then begins. At each execution except for the first one, the counter is incremented before the command is executed (this allows for jumps to work properly, otherwise jumping to word 10 would end up executing word 11, for example). The word at the address given by the counter is then executed as a machine instruction.

##Instructions
In general, the format for instruction words is as follows:
The most significant six bits encode the operator. There are 25 operators.

Most instructions require up to three registers for execution (for example, r[A] := r[B] + r[C]). The least significant four bits encode C, the next four bits encode B, and the last four bits encode A:

<pre>
                    A       C
                    vvvv    vvvv
VUTSRQPONMLKJIHGFEDCBA9876543210
^^^^^^                  ^^^^
opcode                  B
</pre>

So, for example, if the opcode corresponded to the addition instruction, and A = 0, B = 1, C = 2, this would result in r[0] := r[1] + r[2].

There are two versions of the multiplication, division, greater than, less than, and comparison operators - unsigned and signed. The unsigned versions perform the operations assuming that the values in all registers are 320bit unsigned integers. The signed versions perform the operations assuming that the values in all registers are 32-bit two's complement signed integers. There is no signed add or signed subtract instruction because unsigned addition and unsigned subtraction also work on signed values.

###Instruction Semantics

<table>
	<tr>
		<td><b>Opcode</b></td><td><b>Name</b></td><td><b>Description</b></td>
	</tr>
	<tr>
		<td>0</td><td>Move</td><td>r[A] := r[B]</td>
  	</tr>
	<tr>
		<td>1</td><td>Equality</td><td>If r[B] == r[C], r[A] := 1, otherwise r[A] := 0</td>
	</tr>
	<tr>
		<td>2</td><td>Greater Than</td><td>If r[B] &gt; r[C], r[A] := 1, otherwise r[A] := 0</td>
	</tr>
	<tr>
		<td>3</td><td>Greater Than (signed)</td><td>If r[B] &gt; r[C], r[A] := 1, otherwise r[A] := 0</td>
	</tr>
	<tr>
		<td>4</td><td>Less Than</td><td>If r[B] &lt; r[C], r[A] := 1, otherwise r[A] := 0</td>
	</tr>
	<tr>
		<td>5</td><td>Less Than (signed)</td><td>If r[B] &lt; r[C], r[A] := 1, otherwise r[A] := 0</td>
	</tr>
	<tr>
		<td>6</td><td>Conditional Jump</td><td>If r[A] != 0, set the program counter to r[B]</td>
	</tr>
	<tr>
		<td>7</td><td>Load</td><td>r[A] := m[r[B]]</td>
	</tr>
	<tr>
		<td>8</td><td>Store</td><td>m[r[A]] := r[B]</td>
	</tr>
	<tr>
		<td>9</td><td>Add</td><td>r[A] := r[B] + r[C]</td>
	</tr>
	<tr>
		<td>10</td><td>Subtract</td><td>r[A] := r[B] - r[C]</td>
	</tr>
	<tr>
		<td>11</td><td>Multiply</td><td>r[A] := r[B] * r[C]</td>
	</tr>
	<tr>
		<td>12</td><td>Multiply (signed)</td><td>r[A] := r[B] * r[C]</td>
	</tr>
	<tr>
		<td>13</td><td>Divide</td><td>r[A] := r[B] / r[C]</td>
	</tr>
	<tr>
		<td>14</td><td>Divide (signed)</td><td>r[A] := r[B] / r[C] (rounds towards 0)</td>
	</tr>
	<tr>
		<td>15</td><td>Bitwise And</td><td>r[A] := r[B] & r[C]</td>
	</tr>
	<tr>
		<td>16</td><td>Bitwise Or</td><td>r[A] := r[B] | r[C]</td>
	</tr>
	<tr>
		<td>17</td><td>Bitwise Exclusive Or</td><td>r[A] := r[B] ^ r[C]</td>
	</tr>
	<tr>
		<td>18</td><td>Bitwise Complement</td><td>r[A] := ~r[B]</td>
	</tr>
	<tr>
		<td>19</td><td>Left Shift</td><td>r[A] := r[B] &lt;&lt; r[C]</td>
	</tr>
	<tr>
		<td>20</td><td>Right Shift</td><td>r[A] := r[B] &gt;&gt; r[C]</td>
	</tr>
	<tr>
		<td>21</td><td>Halt</td><td>Halt the machine</td>
	</tr>
	<tr>
		<td>22</td><td>Output</td><td>The value in r[A] is displayed on the I/O device (as ASCII). Only values in [0,255] allowed.</td>
	</tr>
	<tr>
		<td>23</td><td>Input</td><td>Machine waits for input on the I/O device. Input is stored in r[A], which will be a value in [0,255]. If EOF was signaled, r[A] will be all 1's.</td>
	</tr>
	<tr>
		<td>24</td><td>Load Value</td><td>The value specified will be loaded into r[A] (see Load Value semantics below).</td>
	</tr>
</table>

###Load Value
The *Load Value* instruction loads a literal value which is encoded in the instruction word itself. The layout of a load value word differs from a normal instruction word since it must encode a literal value as well as the register addresses. When a *Load Value* instruction is executed, the value is loaded into r[A]. The opcode is coded in the most significant six bits. The register address, A, is coded in the next four bits. The value itself is coded in the remaining 22 bits.

<pre>
         A
      vvvv
VUTSRQPONMLKJIHGFEDCBA9876543210
^^^^^^    ^^^^^^^^^^^^^^^^^^^^^^
opcode    value
</pre>

##Failure States
During normal operation, the machine moves from one state to the next with the execution of each instruction (a machine state in this case refers to the state of all of the registers and all of the words in memory). However, certain operations will result in the machine existing in an invalid state, or Failure State. If the machine moves into a failure state, execution will be halted. The following will result in failure states:
* If the program counter refers to a word which is not part of allocated memory, the machine will fail.
* If the program counter refers to a word which does not code for a valid instruction, the machine will fail.
* If an instruction divides by zero, the machine will fail.
* If an instructions loads from or stores to a word which is not part of allocated memory, the machine will fail.
* If an instruction outputs a value not in the range [0,255], the machine will fail.
* If there are more instructions in a binary (exluding the first word) than there are words allocated in memory, the machine will fail.


Protected Mode Extensions
=========================

In addition to the features detailed above, referred to from here on out as the *normal subset*, there is a set of extensions known as the *protected mode extensions* which make it possible for Machine to support a fully-functioning kernel with the ability to guarantee memory safety, process termination, and multithreading.

##Architecture
The protected mode extensions provide modes, lookaside registers, a callback register, a fault register, a program counter lookaside register, two virtual memory registers, and a program counter timer.

###Modes
Machine can be in two modes - *protected mode* and *user mode*. At the beginning of execution, Machine is in protected mode.

###Lookaside Registers
There are 16 extra registers called *lookaside registers*, denoted r'. When any illegal operation (defined below) is performed while Machine is in user mode, the contents of the registers are copied into the lookaside registers (that is, for x in [0,15], r'[x] := r[x]).

###Fault Register
There is a single register called the *fault register*, denoted f. The contents of f are denoted f[]. When any illegal operation is performed in user mode, the *fault code* of the operation (defined below) is paced into f. The order of this relative to the setting of the lookaside registers is undefined.

###Program Counter Lookaside Register
There is a single register called the *program counter lookaside register*, denoted pc'. The contents of pc' are denoted pc'[]. When any illegal operation is performed in user mode, the value of the program counter prior to the execution of the illegal instruction is placed into pc'[]. The order of this relative to the setting of the lookaside and fault registers is undefined.

###Program Counter Timer
There is a single register called the *program counter timer*, denoted t. The contents of t are denoted t[]. Prior to the execution of any instruction in user mode, t is decremented (t[] := t[] - 1). If, prior to this, t[] = 0, then instead of t being decremented, it is considered an illegal operation, and a fault is triggered. Additionally, if execution of the instruction causes any other fault, t is not decremented.

###Callback Register
There is a single register called the *callback register*, denoted c. The contents of c are denoted c[]. When any illegal operation is performed in user mode, after the lookaside registers, fault register, and program counter lookaside register have been set, Machine is placed into protected mode, and execution continues from c[].

###Virtual Memory Registers
There is a pair of registers called *virtual memory registers*, denoted v (v[0] and v[1]). When a memory access operation is performed in user mode, the physical address, p, that is accessed is defined in relation to the logical address, l, that is coded by the instruction word, by the relation p = v[0] + l. If p < v[0] or p > v[1], this is an illegal operation and will trigger a fault.

##Instructions
The protected mode extensions include 11 additional instructions known as *protected instructions*. Executing any of these instructions in user mode is an illegal operation, and will trigger a fault.

###Instruction semantics
<table>
	<tr>
		<td><b>Opcode</b></td><td><b>Name</b></td><td><b>Description</b></td>
	</tr>
	<tr>
		<td>25</td><td>Enter User Mode</td><td>Machine is placed into user mode, the program counter is set to r[A], all registers are set such that r[X] := r'[X], and execution is continued</td>
  	</tr>
	<tr>
		<td>26</td><td>Lookaside Load</td><td>r[A] := r'[B]</td>
  	</tr>
	<tr>
		<td>27</td><td>Lookaside Store</td><td>r'[A] := r[B]</td>
  	</tr>
	<tr>
		<td>28</td><td>Set Callback</td><td>c[] := r[A]</td>
  	</tr>
	<tr>
		<td>29</td><td>Fault Load</td><td>r[A] := f[]</td>
  	</tr>
	<tr>
		<td>30</td><td>PC Lookaside Load</td><td>r[A] := pc'[]</td>
  	</tr>
	<tr>
		<td>31</td><td>Set Virtual Memory Low</td><td>v[0] := r[A]</td>
  	</tr>
	<tr>
		<td>32</td><td>Set Virtual Memory High</td><td>v[1] := r[A]</td>
  	</tr>
	<tr>
		<td>33</td><td>PC Timer Load</td><td>r[A] := t[]</td>
  	</tr>
	<tr>
		<td>34</td><td>PC Timer Store</td><td>t[] := r[A]</td>
  	</tr>
	<tr>
		<td>36</td><td>Trigger</td><td>do nothing (still triggers fault in user mode)</td>
  	</tr>
</table>

Additionally, the *Halt* (opcode 21), *Output* (opcode 22), and *Input* (opcode 23) instructions are considered illegal operations if executed in user mode, and will trigger a fault.

##Failure States
The protected mode extensions include two extra failure states.
* If the *Enter User Mode* instruction is executed when v[0] + r[A] does not address a word in the range [v[0], v[1]], Machine will fail.
* If the *Set Virtual Memory Low* or *Set Virtual Memory High* instructions are executed when r[A] does not address a word in allocated memory, Machine will fail.

##Fault Codes
Whenever a fault is triggered, there is an associated *fault code* which is placed into the fault register, f, which describes the fault. This is a list of faults and associated fault codes. Note that every action which would cause Machine to enter a failure state in protected mode triggers a fault if performed in user mode. Thus, it is not possible for code executing in user mode to cause Machine to enter a failure state.

<table>
	<tr>
		<td><b>Fault Code</b></td><td><b>Fault</b></td>
	</tr>
	<tr>
		<td>0</td><td>Executing a protected instruction other than Trigger</td>
	</tr>
	<tr>
		<td>1</td><td>Executing Trigger</td>
	</tr>
	<tr>
		<td>2</td><td>Executing an instruction which would decrement the program counter timer below 0</td>
	</tr>
	<tr>
		<td>3</td><td>Executing an instruction whose address is not in [v[0], v[1]]</td>
	</tr>
	<tr>
		<td>4</td><td>Accessing a memory word whose address is not in [v[0], v[1]]</td>
	</tr>
	<tr>
		<td>5</td><td>Executing an instruction which does not code for a valid operation</td>
	</tr>
	<tr>
		<td>6</td><td>Division by 0</td>
	</tr>
</table>

##Building
To build Machine, simply do:
```shell
make
```
