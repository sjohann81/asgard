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

/* some helper macros to emit text */
#define emits(s) emit(s, strlen(s))
#define emitf(fmt, ...) \
	do { \
		char buf[128]; \
		snprintf(buf, sizeof(buf)-1, fmt, __VA_ARGS__); \
		emits(buf); \
	} while (0)
#define emitsd(s) emitd(s, strlen(s))
#define emitfd(fmt, ...) \
	do { \
		char buf[128]; \
		snprintf(buf, sizeof(buf)-1, fmt, __VA_ARGS__); \
		emitsd(buf); \
	} while (0)

#define TYPE_NUM_SIZE 4

#define GEN_ADD   "addu	$v0, $v0, $v1			# ADD\n"
#define GEN_ADDSZ strlen(GEN_ADD)

#define GEN_SUB   "subu	$v0, $v1, $v0			# SUB\n"
#define GEN_SUBSZ strlen(GEN_SUB)

#define GEN_MUL   "mul	$v0, $v1, $v0			# MUL\n"
#define GEN_MULSZ strlen(GEN_MUL)

#define GEN_DIV   "div	$v0, $v1, $v0			# DIV\n"
#define GEN_DIVSZ strlen(GEN_DIV)

#define GEN_REM   "rem	$v0, $v1, $v0			# REM\n"
#define GEN_REMSZ strlen(GEN_REM)

#define GEN_SHL   "sllv	$v0, $v1, $v0			# SHL\n"
#define GEN_SHLSZ strlen(GEN_SHL)

#define GEN_SHRA  "srav	$v0, $v1, $v0			# SHRA\n"
#define GEN_SHRASZ strlen(GEN_SHRA)

#define GEN_SHR   "srlv	$v0, $v1, $v0			# SHR\n"
#define GEN_SHRSZ strlen(GEN_SHR)

#define GEN_LESS  "slt	$v0, $v1, $v0			# LESS\naddiu	$v0, $v0, -1\nnor	$v0, $v0, $zero\n"
#define GEN_LESSSZ strlen(GEN_LESS)
#define GEN_LEQ  "slt	$v0, $v0, $v1			# LEQ\nori	$at, $zero, 1\nsubu	$v0, $at, $v0\naddiu	$v0, $v0, -1\nnor	$v0, $v0, $zero\n"
#define GEN_LEQSZ strlen(GEN_LEQ)
#define GEN_GREATER  "slt	$v0, $v0, $v1		# GREATER\naddiu	$v0, $v0, -1\nnor	$v0, $v0, $zero\n"
#define GEN_GREATERSZ strlen(GEN_GREATER)
#define GEN_GEQ  "slt	$v0, $v1, $v0			# GEQ\nori	$at, $zero, 1\nsubu	$v0, $at, $v0\naddiu	$v0, $v0, -1\nnor	$v0, $v0, $zero\n"
#define GEN_GEQSZ strlen(GEN_GEQ)

#define GEN_EQ "subu	$v0, $v1, $v0			# EQ\nori	$at, $zero, 1\nsltu	$v0, $v0, $at\naddiu	$v0, $v0, -1\nnor	$v0, $v0, $zero\n"
#define GEN_EQSZ strlen(GEN_EQ)
#define GEN_NEQ  "subu	$v0, $v1, $v0			# NEQ\nsltu	$v0, $zero, $v0\naddiu	$v0, $v0, -1\nnor	$v0, $v0, $zero\n"
#define GEN_NEQSZ strlen(GEN_NEQ)

#define GEN_AND  "and	$v0, $v0, $v1			# AND\n"
#define GEN_ANDSZ strlen(GEN_AND)
#define GEN_XOR  "xor	$v0, $v0, $v1			# XOR\n"
#define GEN_XORSZ strlen(GEN_XOR)
#define GEN_OR "or	$v0, $v0, $v1			# OR\n"
#define GEN_ORSZ strlen(GEN_OR)

#define GEN_ASSIGN "sw	$v0, 0($v1)			# ASSIGN\n"
#define GEN_ASSIGNSZ strlen(GEN_ASSIGN)
#define GEN_ASSIGN8 "sb	$v0, 0($v1)			# ASSSIGN8\n"
#define GEN_ASSIGN8SZ strlen(GEN_ASSIGN8)

#define GEN_JMP "j              			# JMP\n"
#define GEN_JMPSZ strlen(GEN_JMP)

#define GEN_JZ "beq	$v0, $zero,         		# JZ\n"
#define GEN_JZSZ strlen(GEN_JZ)

static void gen_start() {
	emits(".text\n");
	emits("lui	$sp, 0x7fff\n");
	emits("ori	$sp, $sp, 0xeffc\n");
	emits("jal	main\n");
	emits("addiu	$v0, $zero, 10\n");
	emits("syscall\n");

	emits("putchar:\n");
	emits("\t\t\taddiu	$sp, $sp, -8\n");
	emits("\t\t\tsw	$a0, 0($sp)\n");
	emits("\t\t\tsw	$v0, 4($sp)\n");
	emits("\t\t\tlw	$a0, 12($sp)\n");
	emits("\t\t\tli	$v0, 11\n");
	emits("\t\t\tsyscall\n");
	emits("\t\t\tlw	$a0, 0($sp)\n");
	emits("\t\t\tlw	$v0, 4($sp)\n");
	emits("\t\t\taddiu	$sp, $sp, 8\n");
	emits("\t\t\tjr	$ra\n");	
}

static void gen_finish() {
	printf("%s\n", code);
	printf(".data\n");
	printf("%s\n", data);
}

/* put constant to primary register */
static void gen_const(int n) {
	emitf("li	$v0, %d\n", n);
}

static void gen_clear(){
	emits("xor	$v0, $v0, $v0			# CLEAR\n");
}

static void gen_complement(){
	emits("nor	$v0, $v0, $zero			# NOT\n");
}

static void gen_push() {
	emits("addiu	$sp, $sp, -4			# PUSH\nsw	$v0, 0($sp)\n");
	stack_pos = stack_pos + 1;
}


static void gen_poptop() {
	emits("lw	$v1, 0($sp)			# POP TOP\naddiu	$sp, $sp, 4\n");
	stack_pos = stack_pos - 1;
}

static void gen_pop(int n) {
	if (n > 0) {
		emitf("addiu	$sp, $sp, %d			# POP\n", n * TYPE_NUM_SIZE);
		stack_pos = stack_pos - n;
	}
}

static void gen_stack_addr(int addr) {
	emitf("addiu	$v0, $sp, %d			# STACK_ADDR\n", addr * TYPE_NUM_SIZE);
}

static void gen_unref(int type) {
	if (type == TYPE_CHARVAR || type == TYPE_GCHARVAR) {
		emits("lb	$v0, 0($v0)			# UNREF\n");
	}else{
		emits("lw	$v0, 0($v0)			# UNREF\n");
	} 
}

/* Call function by address stored in primary register */
static void gen_call() {
	emits("addiu	$sp, $sp, -4			# CALL\nsw	$ra, 0($sp)\njalr	$v0\n");
	emits("lw	$ra, 0($sp)			# RESTORE\n");
	stack_pos = stack_pos + 1;
}

/* return from function */
static void gen_ret() {
	emits("jr	$ra				# RETURN\n");
}

static void gen_sym(struct symbol *sym) {
	if (sym->type == 'F') {
		emits("\t");
		emits(sym->name);
		emits(":\n");
	}
}

static void gen_loop_start() {
	emitf("___%08x:\n", codepos);
}

static void gen_sym_addr(struct symbol *sym) {
	emitf("la	$v0, %s			# SYM_ADDR\n", sym->name);
}

static void gen_fixaddr(void){
	emits("sll	$v0, $v0, 2\n");
}

/* patch jump address */
static void gen_patch(char *op, int value) {
	char s[32];
	sprintf(s, "___%08x", value);
	if (value >= codepos) {
		emits(s);
		emits(":\n");
	}
	memcpy(op-strlen(s)-1, s, strlen(s));
}

static void gen_array(int ts, int size) {
	int p = 0;
	
	while(((size * ts + p) % TYPE_NUM_SIZE) != 0) p++;
	emitf("addiu	$sp, $sp, -%d\n", (size * ts) + p);
	stack_pos = stack_pos + ((size * ts) + p) / TYPE_NUM_SIZE;
	gen_stack_addr(0);
}

/* filled array or global array auxiliary functions */
static int array_index = 0;

static void gen_array_str(struct symbol *s, char *array, int size, char global, char empty) {
	int i;
	
	if (global){
		if (size > 1){
			emitfd("%s:	.word ___%s\n", s->name, s->name);
			emitfd("___%s:	.byte ", s->name);
			if (empty){
				for (i = 0; i < size; i++)
					emitsd("0 ");
			}else{
				for (i = 0; i < size; i++)
					emitfd("%d ", array[i]);
			}
			emitsd("\"\n");
		}else{
			emitfd("%s:	.word 0", s->name);
		}
		emitsd("\n");
	}else{
		emitfd("___s%d:	.asciiz \"", array_index);
		emitsd(array);
		emitsd("\"\n");
		emitf("la	$v0, ___s%d			# ARRAY of char\n", array_index);
		array_index++;
	}
}

static int array_index_n = 0;

static void gen_array_int0(struct symbol *s, char global) {
	if (global){
		emitfd("%s:	.word ", s->name);
	}else{
		emitfd("___n%d:	.word ", array_index_n);
	}
}

static void gen_array_int1(struct symbol *s, char global) {
	if (global){
		emitfd("%s:	.word ___%s\n", s->name, s->name);
		emitfd("___%s:	.word ", s->name);
	}else{
		emitfd("___n%d:	.word ", array_index_n);
	}
}

static void gen_array_int2(char empty) {
	if (empty){
		emitsd("0 ");
	}else{
		emitfd("%s ", tok);
	}
}

static void gen_array_int3(char global) {
	emitsd("\n");
	if (!global){
		emitf("la	$v0, ___n%d			# ARRAY of int\n", array_index_n);
		array_index_n++;
	}
}
