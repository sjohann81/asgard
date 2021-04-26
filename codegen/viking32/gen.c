/*
Viking32 target backend

this is the compiler backend (just a code generator).
no optimization is performed, and the stack is used very often.

used registers:
r0	- temporary
r1	- primary register
r2	- secondary register
r6	- return address
r7	- stack pointer
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
	printf(".code\n");
	emits("\tldi	at,main\n");
	emits("\tldi	lr,___retmain\n");
	emits("\tbnz	r7,at\n");
	emits("___retmain\n");
	emits("\thcf\n");
	emits("putchar\n");
	emits("\tsub	sp,4\n");
	emits("\tstw	r1,sp\n");
	emits("\tsub	sp,4\n");
	emits("\tstw	r2,sp\n");
	emits("\tand	r2,sp,sp\n");
	emits("\tadd	r2,12\n");
	emits("\tldw	r1,r2\n");
	emits("\tstw	r1,0xf0000000\n");
	emits("\tldw	r2,sp\n");
	emits("\tadd	sp,4\n");
	emits("\tldw	r1,sp\n");
	emits("\tadd	sp,4\n");
	emits("\tbnz	r7,lr\n");
	emits("getchar\n");
	emits("\tldw	r1,0xf0000008\n");
	emits("\tbnz	r7,lr\n");
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
	if (n >= -128 && n <= 127) {
			emitf("\tldr	r1,%d\n", n & 0xff);
	} else {
		if (n >= -32768 && n <= 32767) {
			emitf("\tldr	r1,%d\n", (n >> 8) & 0xff);
			emitf("\tldc	r1,%d\n", n & 0xff);
		} else {
			if (n >= -8388608 && n <= 8388607) {
				emitf("\tldr	r1,%d\n", (n >> 16) & 0xff);
				emitf("\tldc	r1,%d\n", (n >> 8) & 0xff);
				emitf("\tldc	r1,%d\n", n & 0xff);
			} else {
				emitf("\tldr	r1,%d\n", (n >> 24) & 0xff);
				emitf("\tldc	r1,%d\n", (n >> 16) & 0xff);
				emitf("\tldc	r1,%d\n", (n >> 8) & 0xff);
				emitf("\tldc	r1,%d\n", n & 0xff);
			}
		}
	}
}

void gen_clear()
{
	emits("\txor	r1,r1,r1\n");
}

void gen_complement()
{
	emits("\txor	r1,-1\n");
}

void gen_push()
{
	emits("\tsub	sp,4\n\tstw	r1,sp\n");
	stack_pos = stack_pos + 1;
}


void gen_poptop()
{
	emits("\tldw	r2,sp\n\tadd	sp,4\n");
	stack_pos = stack_pos - 1;
}

void gen_pop(int n)
{
	if (n > 0) {
		if ((n * TYPE_NUM_SIZE < -128) || (n * TYPE_NUM_SIZE > 127)) {
			gen_const(n * TYPE_NUM_SIZE);
			emits("\tadd	sp,sp,r1\n");
		} else {
			emitf("\tadd	sp,%d\n", (n * TYPE_NUM_SIZE));
		}
		stack_pos = stack_pos - n;
	}
}

void gen_stack_addr(int addr)
{
	gen_const(addr * TYPE_NUM_SIZE);
	emits("\tadd	r1,sp,r1\n");
}

void gen_unref(int type)
{
	if (type == TYPE_CHARVAR || type == TYPE_GCHARVAR) {
		emits("\tldb	r1,r1\n");
	}else{
		emits("\tldw	r1,r1\n");
	}
}

/* Call function by address stored in primary register */
void gen_call()
{
	static int32_t i = 0;
	char retbuf[50];

	sprintf(retbuf, "___ret%04x\n", i++);
	emits("\tsub	sp,4\n");
	emits("\tstw	lr,sp\n");
	emitf("\tldi	lr,%s", retbuf);
	emits("\tbnz	r7,r1\n");
	emits(retbuf);
	emits("\tldw	lr,sp\n");
	stack_pos = stack_pos + 1;
}

/* return from function */
void gen_ret()
{
	emits("\tbnz	r7,lr\n");
}

void gen_sym(struct symbol *sym)
{
	if (sym->type == 'F') {
		emits(sym->name);
		emits("\n");
	}
}

void gen_loop_start()
{
	emitf("___%08x\n", codepos);
}

void gen_sym_addr(struct symbol *sym) {
	emitf("\tldi	r1,%s\n", sym->name);
}

void gen_fixaddr(void)
{
	emits("\tlsl	r1,r1\n\tlsl	r1,r1\n");
}

/* patch jump address */
void gen_patch(char *op, int value)
{
	char s[32];
	sprintf(s, "___%08x", value);
	if (value >= codepos) {
		emits(s);
		emits("\t\n");
	}
	memcpy(op-strlen(s)-13, s, strlen(s));
}

void gen_array(int ts, int size)
{
	int p = 0;

	while (((size * ts + p) % TYPE_NUM_SIZE) != 0) p++;
	if ((((size * ts) + p) < -128) || (((size * ts) + p) > 127)) {
		gen_const((size * ts) + p);
		emits("\tsub	sp,sp,r1\n");
	} else {
		emitf("\tsub	sp,%d\n", (size * ts) + p);
	}
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
			emitfd("%s	___%s\n", s->name, s->name);
			emitfd("___%s	\"", s->name);
			if (empty) {
				for (i = 0; i < size; i++)
					emitsd(" ");
			} else {
				for (i = 0; i < size; i++)
					emitfd("%c", array[i]);
			}
			emitsd("\"\n");
		} else {
			emitfd("%s	\" \"\n", s->name);
		}
	} else {
		emitfd("___s%d	\"", array_index);
		emitsd(array);
		emitsd("\"\n");
		emitf("\tldi	r1,___s%d\n", array_index);
		array_index++;
	}
}

static int array_index_n = 0;

void gen_array_int0(struct symbol *s, char global)
{
	if (global) {
		emitfd("%s	", s->name);
	} else {
		emitfd("___n%d	", array_index_n);
	}
}

void gen_array_int1(struct symbol *s, char global)
{
	if (global) {
		emitfd("%s	___%s\n", s->name, s->name);
		emitfd("___%s	", s->name);
	} else {
		emitfd("___n%d	", array_index_n);
	}
}

void gen_array_int2(char empty)
{
	int val;

	if (empty) {
		emitsd("0 ");
	} else {
		if isalpha(tok[0]) {
			emitfd("%s ", tok);
		} else {
			val = (int)strtol(tok, NULL, 0);
			emitfd("%d ", val);
		}
	}
}

void gen_array_int3(char global)
{
	emitsd("\n");
	if (!global) {
		emitf("\tldi	r1,___n%d\n", array_index_n);
		array_index_n++;
	}
}
