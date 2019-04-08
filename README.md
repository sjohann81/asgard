# A compiler for the Asgard programming language

---
### Description

This is an experimental programming language and a compiler for this language. The compiler uses a minimal scanner, a simple recursive-descent parser and direct code generation. The compiler backend uses patterns for code generation, making it easily retargetable. The compiler backends are so simple that a retarget to a new architecture can be done in an hour or less.

### Build process

Just type 'make' to build compilers for different targets. Currently, the MARS (MIPS simulator), x86 (Linux) and Viking (16 and 32 bit) targets are supported. 

### Compilation process

Nothing fancy here. The compiler just reads Asgard code from 'stdin' and emits assembly code to 'stdout'. In order to build an example application, application code and the required libraries must be used as inputs for the compiler through stdin. The assembly is generated to stdout, so it should be redirected to a file for further processing (like feeding it to an assembler). For example, in order to compile the 'tests/dijkstra2.asg' application for the Viking16 target, the following command is used:

- $ cat tests/dijkstra2.asg lib/std.asg lib/softmul.asg | ./asgard-viking16 > out.s

You will need the Viking assembler and simulator to run this. Another example, building the 'tests/fixed_point.asg' application for the x86 target:

- $ cat tests/fixed_point.asg lib/std.asg lib/softmul.asg lib/fixed.asg | ./asgard-x86_linux > out.s

The 'makefile' makes it easy to build a Linux ELF for x86 using the GNU tools:

- $ cat tests/dijkstra2.asg lib/std.asg lib/softmul.asg | ./asgard-x86_linux > out.s
- $ make app_x86_linux

To run it, just do:

- $ ./out.elf


