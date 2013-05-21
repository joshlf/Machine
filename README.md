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
Binaries are files containing the words of a program, in big-endian byte order. The format for binaries is quite simple. The first word specifies how many words will be available in memory. Its minimum value is one less than the size of the binary.

To begin, memory of the proper size is allocated and initialized to zero. All of the registers are also intialized to zero. Then, the words of the binary are loaded into memory starting with word 1 (this excludes the first word of the binary, the one which encodes memory length). Word 1 of the binary is loaded into word 0 of memory, and so on. The program counter is then set to point at word 0 of memory.

Execution then begins. At each execution except for the first one, the counter is incremented before the command is executed (this allows for jumps to work properly, otherwise jumping to word 10 would end up executing word 11, for example). The word at the address given by the counter is then executed as a machine instruction.

##Instructions
In general, the format for instruction words is as follows:
The most significant five bits encode the operator. There are 25 operators.

Most instructions require up to three registers for execution (for example, r[A] := r[B] + r[C]). The least significant four bits encode C, the next four bits encode B, and the last four bits encode A:

<pre>
                    A       C
                    vvvv    vvvv
VUTSRQPONMLKJIHGFEDCBA9876543210
^^^^^                   ^^^^
opcode                  B
</pre>

So, for example, if the opcode corresponded to the addition instruction, and A = 0, B = 1, C = 2, this would result in r[0] := r[1] + r[2].

There are two versions of each of the arithmetic operators (addition, subtraction, multiplication, division), and of the greater than and less than comparison operators - unsigned and signed. The unsigned versions perform the operations assuming that the values in all registers are 32-bit unsigned integers. The signed versions perform the operations assuming that the values in all registers are 32-bit two's complement signed integers.

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
		<td>10</td><td>Add (signed)</td><td>r[A] := r[B] + r[C]</td>
	</tr>
	<tr>
		<td>11</td><td>Subtract</td><td>r[A] := r[B] - r[C]</td>
	</tr>
	<tr>
		<td>12</td><td>Subtract (signed)</td><td>r[A] := r[B] - r[C]</td>
	</tr>
	<tr>
		<td>13</td><td>Multiply</td><td>r[A] := r[B] * r[C]</td>
	</tr>
	<tr>
		<td>14</td><td>Multiply (signed)</td><td>r[A] := r[B] * r[C]</td>
	</tr>
	<tr>
		<td>15</td><td>Divide</td><td>r[A] := r[B] / r[C]</td>
	</tr>
	<tr>
		<td>16</td><td>Divide (signed)</td><td>r[A] := r[B] / r[C] (rounds towards 0)</td>
	</tr>
	<tr>
		<td>17</td><td>Bitwise And</td><td>r[A] := r[B] & r[C]</td>
	</tr>
	<tr>
		<td>18</td><td>Bitwise Or</td><td>r[A] := r[B] | r[C]</td>
	</tr>
	<tr>
		<td>19</td><td>Bitwise Exclusive Or</td><td>r[A] := r[B] ^ r[C]</td>
	</tr>
	<tr>
		<td>20</td><td>Bitwise Complement</td><td>r[A] := ~r[B]</td>
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
The load value instruction loads a literal value which is encoded in the instruction word itself. The layout of a load value word differs from a normal instruction word since it must encode a literal value as well as the register addresses. When a load value instruction is executed, the value is loaded into r[A]. The opcode is coded in the most significant five bits. The register address, A, is coded in the next four bits. The value itself is coded in the remaining 23 bits.

<pre>
        A
     vvvv
VUTSRQPONMLKJIHGFEDCBA9876543210
^^^^^    ^^^^^^^^^^^^^^^^^^^^^^^
opcode   value
</pre>

##Failure States
During normal operation, the machine moves from one state to the next with the execution of each instruction (a machine state in this case refers to the state of all of the registers and all of the words in memory). However, certain operations will result in the machine existing in an invalid state, or Failure State. If the machine moves into a failure state, execution will be halted. The following will result in failure states:
* If the program counter refers to a word which is not part of allocated memory, the machine will fail.
* If the program counter refers to a word which does not code for a valid instruction, the machine will fail.
* If an instruction divides by zero, the machine will fail.
* If an instructions loads from or stores to a word which is not part of allocated memory, the machine will fail.
* If an instruction outputs a value not in the range [0,255], the machine will fail.
* If there are more instructions in a binary (exluding the first word) than there are words allocated in memory, the machine will fail.
