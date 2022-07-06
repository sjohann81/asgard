/*
MARS target backend

this is the compiler backend (just a code generator).
no optimization is performed, and the stack is used very often.

used registers:
$zero	- constant zero
$at	- assembly temporary
$v0	- primary register
$v1	- secondary register
$sp	- stack pointer
$ra	- return address
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
	emits("lui	$sp, 0x7fff\n");
	emits("ori	$sp, $sp, 0xeffc\n");
	emits("jal	main\n");
	emits("addiu	$v0, $zero, 10\n");
	emits("syscall\n");

	emits("putchar:\n");
	emits("addiu	$sp, $sp, -8\n");
	emits("sw	$a0, 0($sp)\n");
	emits("sw	$v0, 4($sp)\n");
	emits("lw	$a0, 12($sp)\n");
	emits("li	$v0, 11\n");
	emits("syscall\n");
	emits("lw	$a0, 0($sp)\n");
	emits("lw	$v0, 4($sp)\n");
	emits("addiu	$sp, $sp, 8\n");
	emits("jr	$ra\n");

	emits("getchar:\n");
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
	emitf("li	$v0, %d\n", n);
}

void gen_clear()
{
	emits("xor	$v0, $v0, $v0			# CLEAR\n");
}

void gen_complement()
{
	emits("nor	$v0, $v0, $zero			# NOT\n");
}

void gen_push()
{
	emits("addiu	$sp, $sp, -4			# PUSH\nsw	$v0, 0($sp)\n");
	stack_pos = stack_pos + 1;
}


void gen_poptop()
{
	emits("lw	$v1, 0($sp)			# POP TOP\naddiu	$sp, $sp, 4\n");
	stack_pos = stack_pos - 1;
}

void gen_pop(int n)
{
	if (n > 0) {
		emitf("addiu	$sp, $sp, %d			# POP\n", n * TYPE_NUM_SIZE);
		stack_pos = stack_pos - n;
	}
}

void gen_stack_addr(int addr)
{
	emitf("addiu	$v0, $sp, %d			# STACK_ADDR\n", addr * TYPE_NUM_SIZE);
}

void gen_unref(int type)
{
	if (type == TYPE_CHARVAR || type == TYPE_GCHARVAR) {
		emits("lb	$v0, 0($v0)			# UNREF\n");
	} else {
		emits("lw	$v0, 0($v0)			# UNREF\n");
	} 
}

/* Call function by address stored in primary register */
void gen_call()
{
	emits("addiu	$sp, $sp, -4			# CALL\nsw	$ra, 0($sp)\njalr	$v0\n");
	emits("lw	$ra, 0($sp)			# RESTORE\n");
	stack_pos = stack_pos + 1;
}

/* return from function */
void gen_ret()
{
	emits("jr	$ra				# RETURN\n");
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
	emitf("la	$v0, %s			# SYM_ADDR\n", sym->name);
}

void gen_fixaddr(void)
{
	emits("sll	$v0, $v0, 2\n");
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
	emitf("addiu	$sp, $sp, -%d		# GEN_ARRAY\n", (size * ts) + p);
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
			emitfd("%s:	.word ___%s\n", s->name, s->name);
			emitfd("___%s:	.byte ", s->name);
			if (empty) {
				for (i = 0; i < size; i++)
					emitsd("0 ");
			} else {
				for (i = 0; i < size; i++)
					emitfd("%d ", array[i]);
			}
			emitsd("\n");
		} else {
			emitfd("%s:	.word 0", s->name);
		}
		emitsd("\n");
	} else {
		emitfd("___s%d:	.asciiz \"", array_index);
		emitsd(array);
		emitsd("\"\n");
		emitf("la	$v0, ___s%d			# ARRAY of char\n", array_index);
		array_index++;
	}
}

static int array_index_n = 0;

void gen_array_int0(struct symbol *s, char global)
{
	if (global) {
		emitfd("%s:	.word ", s->name);
	}else{
		emitfd("___n%d:	.word ", array_index_n);
	}
}

void gen_array_int1(struct symbol *s, char global)
{
	if (global) {
		emitfd("%s:	.word ___%s\n", s->name, s->name);
		emitfd("___%s:	.word ", s->name);
	}else{
		emitfd("___n%d:	.word ", array_index_n);
	}
}

void gen_array_int2(char empty)
{
	if (empty) {
		emitsd("0 ");
	} else {
		emitfd("%s ", tok);
	}
}

void gen_array_int3(char global)
{
	emitsd("\n");
	if (!global) {
		emitf("la	$v0, ___n%d			# ARRAY of int\n", array_index_n);
		array_index_n++;
	}
}
