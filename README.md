Machine
=======

An emulator for computer with a simple architecture and instruction set

##Overview
Machine emulates a computer with a simple memory architecture and small instruction set. It is intended to be used in an educational or research setting, allowing binaries to be run universally and without concern for physical architecture. It is written primarily for my own use in testing new compiler or assembler features, but I would be thrilled if others got use out of it as well. If you have suggested changes or bugs, please let me know!

##Architecture
Machine is a 32-bit architecture. It comprises three simple components - memory, registers, and a program counter.

###Registers
There are 16 registers, each holding a single 32-bit word. The nth register is denoted r[n].

###Memory
Machine has a 32-bit memory address space. It is word-addressed, with each address referencing a single 32-bit word. The nth word in memory is denoted m[n]. The number of words actually available to be written to or read from is determined by the program which is run.

###Program Counter
Machine keeps a counter which holds the address of the word in memory which is to be executed next. The counter is incremented before each execution.

##Binaries
Binaries are files containing the words of a program, in big-endian byte order. The format for binaries is quite simple. The first word specifies how many words will be available in memory. It must be at least as large as the size of the binary itself.

To begin, memory of the proper size is allocated and initialized to zero. All of the registers are also intialized to zero. Then, the words of the binary are loaded into memory starting with word 0 (this includes the first word of the binary, the one which encodes memory length). The program counter is then set to point at word 1 (the second word).

Execution then begins. At each execution except for the first one, the counter is incremented before the command is executed (this allows for jumps to work properly, otherwise jumping to word 10 would end up executing word 11, for example). The word at the address given by the counter is then executed as a machine instruction.

##Instructions
In general, the format for instruction words is as follows:
The most significant five bits encode the operator. There are 18 operators.

Most instructions require up to three registers for execution (for example, r[A] := r[B] + r[C]). The least significant four bits encode C, the next four bits encode B, and the last four bits encode A:

<pre>
                    A       C
                    vvvv    vvvv
VUTSRQPONMLKJIHGFEDCBA9876543210
^^^^^                   ^^^^
opcode                  B
</pre>

So, for example, if the opcode corresponded to the addition instruction, and A = 0, B = 1, C = 2, this would result in r[0] := r[1] + r[2].

There are two versions of each of the arithmetic operators (addition, subtraction, multiplication, division) - unsigned and signed. The unsigned versions perform the operations assuming that the values in all registers are 32-bit unsigned integers. The signed versions perform the operations assuming that the values in all registers are 32-bit two's complement signed integers.

###Instruction Semantics

<table>
	<tr>
		<td><b>Opcode</b></td><td><b>Name</b></td><td><b>Description</b></td>
	</tr>
	<tr>
		<td>0</td><td>Move</td><td>r[A] := r[B]</td>
  	</tr>
	<tr>
		<td>1</td><td>Compare</td><td>Compare r[A] and r[B] and store the result in r[C] (see description below)</td>
	</tr>
	<tr>
		<td>2</td><td>Conditional Jump</td><td>If bit B of r[A] is 1, set the program counter to r[C] (see description below)</td>
	</tr>
	<tr>
		<td>3</td><td>Load</td><td>r[A] := m[r[B]]</td>
	</tr>
	<tr>
		<td>4</td><td>Store</td><td>m[r[A]] := r[B]</td>
	</tr>
	<tr>
		<td>5</td><td>Add</td><td>r[A] := r[B] + r[C]</td>
	</tr>
	<tr>
		<td>6</td><td>Add (signed)</td><td>r[A] := r[B] + r[C]</td>
	</tr>
	<tr>
		<td>7</td><td>Subtract</td><td>r[A] := r[B] - r[C]</td>
	</tr>
	<tr>
		<td>8</td><td>Subtract (signed)</td><td>r[A] := r[B] - r[C]</td>
	</tr>
	<tr>
		<td>9</td><td>Multiply</td><td>r[A] := r[B] * r[C]</td>
	</tr>
	<tr>
		<td>10</td><td>Multiply (signed)</td><td>r[A] := r[B] * r[C]</td>
	</tr>
	<tr>
		<td>11</td><td>Divide</td><td>r[A] := r[B] / r[C]</td>
	</tr>
	<tr>
		<td>12</td><td>Divide (signed)</td><td>r[A] := r[B] / r[C] (rounds towards 0)</td>
	</tr>
	<tr>
		<td>13</td><td>Bitwise NAND</td><td>r[A] := r[B] NAND r[C]</td>
	</tr>
	<tr>
		<td>14</td><td>Halt</td><td>Halt the machine</td>
	</tr>
	<tr>
		<td>15</td><td>Output</td><td>The value in r[A] is displayed on the I/O device (as ASCII). Only values in [0,255] allowed.</td>
	</tr>
	<tr>
		<td>16</td><td>Input</td><td>Machine waits for input on the I/O device. Input is stored in r[A], which will be a value in [0,255]. If EOF was signaled, r[A] will be all 1's.</td>
	</tr>
	<tr>
		<td>17</td><td>Load Value</td><td>The value specified will be loaded into r[A] (see Load Value semantics below).</td>
	</tr>
</table>

###Special Operations
<b>Load Value</b><br>
The load value instruction loads a literal value which is encoded in the instruction word itself. The layout of a load value word differs from a normal instruction word since it must encode a literal value as well as the register addresses. When a load value instruction is executed, the value is loaded into r[A].

<pre>
        A
     vvvv
VUTSRQPONMLKJIHGFEDCBA9876543210
^^^^^    ^^^^^^^^^^^^^^^^^^^^^^^
opcode   value
</pre>

<b>Compare</b><br>
The compare instruction performs multiple comparisons of r[A] and r[B], storing the results as bits in r[C]. For example, if r[A] == r[B], the least significant bit of r[C] is set to 1. Otherwise, it is set to 0. Some of these comparisons are performed assuming that the values in the registers are unsigned integers, and others are performed assuming that they are two's complement signed integers. The table below outlines all comparisons.

<table>
	<tr>
		<td><b>Bit (offset from least significant)</b></td><td><b>Comparison</b></td>
  	</tr>
	<tr>
		<td>0</td><td>r[A] == r[B]</td>
	</tr>
	<tr>
		<td>1</td><td>r[A] &gt; r[B] (unsigned)</td>
	</tr>
	<tr>
		<td>2</td><td>r[A] &lt; r[B] (unsigned)</td>
	</tr>
	<tr>
		<td>3</td><td>r[A] &gt; r[B] (singed)</td>
	</tr>
	<tr>
		<td>4</td><td>r[A] &lt; r[B] (signed)</td>
	</tr>
</table>

<b>Conditional Jump</b><br>
The conditional jump instruction checks the bth least significant bit (lsb) of r[A]. If it is 1, the program counter is set equal to r[C]. The bth lsb is meant to be one of the bits set by the compare instruction above.

##Failure States
During normal operation, the machine moves from one state to the next with the execution of each instruction (a machine state in this case refers to the state of all of the registers and all of the words in memory). However, certain operations will result in the machine existing in an invalid state, or Failure State. If the machine moves into a failure state, execution will be halted. The following will result in failure states:
* If the program counter refers to a word which is not part of allocated memory, the machine will fail.
* If the program counter refers to a word which does not code for a valid instruction, the machine will fail.
* If an instruction divides by zero, the machine will fail.
* If an instructions loads from or stores to a word which is not part of allocated memory, the machine will fail.
* If an instruction outputs a value not in the range [0,255], the machine will fail.
