/*
Intel x86 linux target backend

this is the compiler backend (just a code generator).
no optimization is performed, and the stack is used very often.

used registers:

%eax (primary register)
%ebx (secondary register)
%ecx (just cl, used for shift counts)
%esp (stack pointer)

http://www.cs.virginia.edu/~evans/cs216/guides/x86.html
http://www.agner.org/optimize/
http://www.wilfred.me.uk/blog/2014/08/27/baby-steps-to-a-c-compiler/
https://github.com/Wilfred/babyc
http://tldp.org/HOWTO/html_single/Assembly-HOWTO/
*/

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <asgard.h>
#include <gen.h>

void gen_start()
{
	emits(".text\n");

	emits(".global _start\n");
	emits("	_start:\n");
	emits("call   main\n");
	emits("mov    $0,%ebx\n");		// first argument: exit code
	emits("mov    $1,%eax\n");		// system call number (sys_exit)
	emits("int    $0x80\n");		// call kernel

	emits("putchar:\n");

	emits("push   %eax\n");
	emits("push   %ebx\n");
	emits("push   %ecx\n");
	emits("push   %edx\n");
	emits("add    $20, %esp\n");
	emits("mov    $4, %eax\n");
	emits("mov    $1, %ebx\n");
	emits("mov    %esp, %ecx\n");
	emits("mov    $1, %edx\n");
	emits("int    $0x80\n");
	emits("sub    $20, %esp\n");
	emits("pop    %edx\n");
	emits("pop    %ecx\n");
	emits("pop    %ebx\n");
	emits("pop    %eax\n");
	emits("ret\n");

	emits("getchar:\n");
	emits("push   %ebx\n");
	emits("push   %ecx\n");
	emits("push   %edx\n");
	emits("add    $20, %esp\n");
	emits("mov    $3, %eax\n");
	emits("mov    $1, %ebx\n");
	emits("mov    %esp, %ecx\n");
	emits("mov    $1, %edx\n");
	emits("int    $0x80\n");
	emits("mov    0(%ecx),%eax\n");
	emits("and    $255,%eax\n");
	emits("sub    $20, %esp\n");
	emits("pop    %edx\n");
	emits("pop    %ecx\n");
	emits("pop    %ebx\n");
	emits("ret\n");
}

void gen_finish()
{
	printf("%s\n", code);
	printf(".data\n");
	printf("%s\n", data);
}

/* put constant to primary register */
void gen_const(int n)
{
	emitf("mov    $0x%x, %%eax\n", n);
}

void gen_clear()
{
	emits("xor    %eax, %eax\n");
}

void gen_complement()
{
	emits("not    %eax\n");
}

void gen_push()
{
	emits("push   %eax\n");
	stack_pos = stack_pos + 1;
}

void gen_poptop()
{
	emits("pop    %ebx\n");
	stack_pos = stack_pos - 1;
}

void gen_pop(int n)
{
	if (n > 0) {
		emitf("add    $0x%04x, %%esp\n", n * TYPE_NUM_SIZE);
		stack_pos = stack_pos - n;
	}
}

void gen_stack_addr(int addr)
{
	emitf("lea    %d(%%esp), %%eax\n", addr * TYPE_NUM_SIZE);
}

void gen_unref(int type)
{
	if (type == TYPE_CHARVAR || type == TYPE_GCHARVAR) {
		emits("movsbl (%eax), %eax\n");
	}else{
		emits("mov    (%eax), %eax\n");
	}
}

/* Call function by address stored in primary register */
void gen_call()
{
	emits("call   *%eax\n");
}

/* return from function */
void gen_ret()
{
	emits("ret\n");
}

void gen_sym(struct symbol *sym)
{
	if (sym->type == 'F') {
		emits("\t");
		emits(sym->name);
		emits(":\n");
	}
}

void gen_loop_start()
{
	emitf("___%08x:\n", codepos);
}

void gen_sym_addr(struct symbol *sym)
{
	emitf("mov    $%s, %%eax\n", sym->name);
}

void gen_fixaddr(void)
{
	emits("shl    $2, %eax\n");
}

/* patch jump address */
void gen_patch(char *op, int value)
{
	char s[32];
	sprintf(s, "___%08x", value);
	if (value >= codepos) {
		emits(s);
		emits(":\n");
	}
	memcpy(op-strlen(s)-1, s, strlen(s));
}

void gen_array(int ts, int size)
{
	int p = 0;

	while(((size * ts + p) % TYPE_NUM_SIZE) != 0) p++;
	emitf("sub    $0x%04x, %%esp\n", (size * ts) + p);
	stack_pos = stack_pos + ((size * ts) + p) / TYPE_NUM_SIZE;
	gen_stack_addr(0);
}

/* filled array or global array auxiliary functions */
static int array_index = 0;

void gen_array_str(struct symbol *s, char *array, int size, char global, char empty)
{
	int i;

	if (global) {
		if (size > 1) {
			emitfd("%s:	.long ___%s\n", s->name, s->name);
			if (empty) {
				emitfd("___%s:", s->name);
				emitfd("   .space %d", size);
			} else {
				emitfd("___%s:	.ascii \"", s->name);
				for (i = 0; i < size; i++)
					emitfd("%c", array[i]);
				emitsd("\\0\"\n");
			}
		} else {
			emitfd("%s:	.long 0", s->name);
		}
		emitsd("\n");
	} else {
		emitfd("___s%d:	.ascii \"", array_index);
		emitsd(array);
		emitsd("\\0\"\n");
		emitf("mov    $___s%d, %%eax\n", array_index);
		array_index++;
	}
}


static int array_index_n = 0;

void gen_array_int0(struct symbol *s, char global)
{
	if (global) {
		emitfd("%s:	.long ", s->name);
	} else {
		emitfd("___n%d:	.long ", array_index_n);
	}
}

void gen_array_int1(struct symbol *s, char global)
{
	if (global) {
		emitfd("%s:	.long ___%s\n", s->name, s->name);
		emitfd("___%s:	.long ", s->name);
	} else {
		emitfd("___n%d:	.long ", array_index_n);
	}
}


static int not_first = 0;

void gen_array_int2(char empty)
{
	if (not_first) emitsd(",");
	if (empty) {
		emitsd("0");
	} else {
		emitfd("%s", tok);
	}
	not_first = 1;
}

void gen_array_int3(char global)
{
	not_first = 0;
	emitsd("\n");
	if (!global) {
		emitf("mov    $___n%d, %%eax\n", array_index_n);
		array_index_n++;
	}
}
