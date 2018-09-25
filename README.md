# A compiler for the Asgard programming language

---
### Description

This is an experimental programming language and a compiler for this language. The compiler uses a minimal scanner, a simple recursive-descent parser and direct code generation. The compiler backend uses patterns for code generation, making it easily retargetable.

### Build process

Just type 'make' to build compilers for different targets. Currently, the MARS (MIPS simulator), x86 (Linux) and Viking (16 and 32 bit) targets are supported.

### Compilation process

Nothing fancy here. The compiler just reads Asgard code from 'stdin' and emits assembly code to 'stdout'. In order to build an example application, we must concatenate the application code with the required libraries and put the output in a file for further processing (like feeding it to an assembler). If we want to compile the 'tests/dijkstra2.asg' application for the Viking16 target, the following command could be used:

- $ cat tests/dijkstra2.asg lib/std.asg lib/softmul.asg | ./asgard-viking16 > out.s

You will need the Viking assembler and simulator to run this. Another example, building the 'tests/fixed_point.asg' application for the x86 target:

- $ cat tests/fixed_point.asg lib/std.asg lib/softmul.asg lib/fixed.asg | ./asgard-x86_linux > out.s

The 'makefile' makes it easy to build a Linux ELF for x86 using the GNU tools:

- $ make app_x86_linux

To run it, just do:

- $ ./out.elf


