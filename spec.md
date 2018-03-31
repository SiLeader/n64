N64 CPU Architecture
==========

&copy; 2018 SiLeader.

## Overview
+ 64bit ISA
+ 64bit fixed instructions
+ 64bit registers
+ x86 like instructions
+ 0 register
+ RISC

sizes
+ byte: 8bit
+ word: 16bit
+ dword: 32bit
+ qword: 64bit

## registers
max 128
### General registers
`rs0-31`, `rt0-31`.
`rs*` must be save. You should restore until `ret` instruction. 
`rt*` is temporary registers.
### system registers
`ip` is instruction pointer. point next instruction address.
`flags` is flag register
#### flags
```
63                                                              32
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| | | | | | | | | | | | | | | | | | | | | | | | | | | | | |h|a|e|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
31                                                              0
```
| flags(short name) | full name | meaning |
|:------------:|:------:|:-----|
| e | equal flag | Result of `cmp` instruction. It means operands is same. you can use as error flag. |
| a | above flag | Result of `cmp` instruction. It means left > right. |
| h | halt flag | It means CPU is halted. |

### Detail
| name | role |
|:----:|:-----|
| r0 | 0 register |
| rs0 - rs31 | save |
| rt0 - rt31 | not save |
| ip | instruction pointer |
| flags | flag register |
| sp | stack pointer |
| bp | base pointer |

## instructions
length of the command is fixed at 8 bits
### instruction type
#### TA (Three-address type, 011)
it has one destination and two sources.

```
+--------+---+-------+-------+-------+----------+----------+----------+
|  ins   | t |  dest |  src1 |  src2 | dest-opt | src1-opt | src2-opt |
+--------+---+-------+-------+-------+----------+----------+----------+
```

| name | meangin |
|:----:|:-----|
| ins | instruction |
| t | operand type. 3 bits. 0 is register. 1 is register as pointer. |
| dest | destination register |
| src1 | source register 1(left hand side) |
| src2 | source register 2(right hand side) |
| dest-opt | option of destination operand |
| src1-opt | option of source 1 operand |
| src2-opt | option of source 2 operand |

option treated as index if register as pointer.
it treated as register width. option=1:2 bytes, option=2:2 bytes, option=3:4bytes

#### B (Binomial type, 010)

```
+--------+--+-------+-------+--------------------+--------------------+
|  ins   | t|  op1  |  op2  |     op1-option     |      op2-option    |
+--------+--+-------+-------+--------------------+--------------------+
```

#### U型 (Unary type, 001)

```
+--------+--+-------+---------------------------------------------+
|  ins   | t|  reg  |                register-option              |
+--------+--+-------+---------------------------------------------+

+--------+--+----------------------------------------------------+
|  ins   | t|                       immediate                    |
+--------+--+----------------------------------------------------+
```
t-flag

| value | meaning |
|:-----:|:--------|
| 00 | use register value |
| 01 | use register as pointer |
| 11 | use immediate |

#### RI (Register Immediate type, 100)
Operation between register and immediate value.

```
+--------+-------+-----------------------------------------------+
|  ins   |  reg  |                   immediate                   |
+--------+-------+-----------------------------------------------+
```

#### NO型 (No-operand type, 000)
it has no operand.

```
+--------+------------------------------------------------------+
|  ins   |                                                      |
+--------+------------------------------------------------------+
```

### instructions
#### Arithmetic instruction
| value | instruction | type | behavior |
|:---:|:----:|:---:|:----|
| 011,00000 | add | TA | addition |
| 011,00001 | sub | TA | subtraction |
| 011,00010 | mul | TA | multiplication |
| 011,00011 | div | TA | division |
| 011,00100 | shr | TA | right-shift |
| 011,00101 | shl | TA | left-shift |
| 001,00000 | inc | U | increment |
| 001,00001 | dec | U | decrement |
| 010,00000 | not | B | bit-NOT |
| 011,00110 | and | TA | bit-AND |
| 011,00111 | or | TA | bit-OR |
| 011,01000 | xor | TA | bit-XOR |

#### unconditional jump instructions
| value | instruction | type | behavior |
|:---:|:----:|:---:|:----|
| 001,00010 | call | U | PUSH return address. and jump to specified address |
| 001,00011 | jmp | U | jump to specified address |
| 001,00100 | jr | U | jump to specified relative address |

#### conditional jump instructions
| value | instruction | type | behavior |
|:---:|:----:|:---:|:----|
| 001,00101 | je | U | jump to specified address if flags set e-bit. |
| 001,00110 | jne | U | jump to specified address if flags is not set e-bit. |
| 001,00111 | ja | U | jump to specified address if flags set a-bit. |
| 001,01000 | jae | U | jump to specified address if flags set e-bit or a-bit. |
| 001,01001 | jb | U | jump to specified address if flags is not set e-bit and not set a-bit. |
| 001,01010 | jbe | U | jump to specified address if flags is not set a-bit. |

#### stack operate instructions
| value | instruction | type | behavior |
|:---:|:----:|:---:|:----|
| 001,01011 | push | U | push to stack |
| 001,01100 | pop | U | pop from stack |

#### other instructions
| value | instruction | type | behavior |
|:---:|:----:|:---:|:----|
| 000,00000 | hlt | NO | halt cpu |
| 010,00001 | xchg | B | swap value |
| 000,00001 | ret | NO | return to caller |
| 010,00010 | cmp | B | compare operands |
| 100,00000 | asgn | RI | assign value to register |
| 100,00001 | asgnh | RI | assign value to upper 32 bits of register. |
| 100,00010 | asgnl | RI | assign value to lower 32 bits of register. |

#### pseudo-instructions (expand other instruction)
| instruction | type | expanded | behavior |
|:----:|:---:|:-------------:|:-----|
| mov | B | add dest, src, r0 | copy to dest from src |
| nop | NO | xchg r0, r0 | do nothing |
| raise | NO | cmp r0, r0 | set e-bit of flags |
