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

#define TYPE_NUM_SIZE	4
#define SW_MULDIV

#define GEN_ADD		"addu	$v0, $v0, $v1\n"
#define GEN_ADDSZ	strlen(GEN_ADD)

#define GEN_SUB		"subu	$v0, $v1, $v0\n"
#define GEN_SUBSZ	strlen(GEN_SUB)

#define GEN_MUL		"add	$sp, $sp, -4			# MUL\nsw	$v1, 0($sp)\nadd	$sp, $sp, -4\nsw	$v0, 0($sp)\nla	$v0, mulsi3\n"
#define GEN_MULSZ	strlen(GEN_MUL)

#define GEN_DIV		"add	$sp, $sp, -4			# DIV\nsw	$v1, 0($sp)\nadd	$sp, $sp, -4\nsw	$v0, 0($sp)\nla	$v0, divsi3\n"
#define GEN_DIVSZ	strlen(GEN_DIV)

#define GEN_REM		"add	$sp, $sp, -4			# REM\nsw	$v1, 0($sp)\nadd	$sp, $sp, -4\nsw	$v0, 0($sp)\nla	$v0, modsi3\n"
#define GEN_REMSZ	strlen(GEN_REM)

#define GEN_SHL		"sllv	$v0, $v1, $v0\n"
#define GEN_SHLSZ	strlen(GEN_SHL)

#define GEN_SHRA	"srav	$v0, $v1, $v0\n"
#define GEN_SHRASZ	strlen(GEN_SHRA)

#define GEN_SHR		"srlv	$v0, $v1, $v0\n"
#define GEN_SHRSZ	strlen(GEN_SHR)

#define GEN_LESS	"slt	$v0, $v1, $v0\naddiu	$v0, $v0, -1\nnor	$v0, $v0, $zero\n"
#define GEN_LESSSZ	strlen(GEN_LESS)
#define GEN_LEQ		"slt	$v0, $v0, $v1\nori	$at, $zero, 1\nsubu	$v0, $at, $v0\naddiu	$v0, $v0, -1\nnor	$v0, $v0, $zero\n"
#define GEN_LEQSZ	strlen(GEN_LEQ)
#define GEN_GREATER		"slt	$v0, $v0, $v1\naddiu	$v0, $v0, -1\nnor	$v0, $v0, $zero\n"
#define GEN_GREATERSZ	strlen(GEN_GREATER)
#define GEN_GEQ		"slt	$v0, $v1, $v0\nori	$at, $zero, 1\nsubu	$v0, $at, $v0\naddiu	$v0, $v0, -1\nnor	$v0, $v0, $zero\n"
#define GEN_GEQSZ	strlen(GEN_GEQ)

#define GEN_EQ		"subu	$v0, $v1, $v0\nori	$at, $zero, 1\nsltu	$v0, $v0, $at\naddiu	$v0, $v0, -1\nnor	$v0, $v0, $zero\n"
#define GEN_EQSZ	strlen(GEN_EQ)
#define GEN_NEQ		"subu	$v0, $v1, $v0\nsltu	$v0, $zero, $v0\naddiu	$v0, $v0, -1\nnor	$v0, $v0, $zero\n"
#define GEN_NEQSZ	strlen(GEN_NEQ)

#define GEN_AND		"and	$v0, $v0, $v1\n"
#define GEN_ANDSZ	strlen(GEN_AND)
#define GEN_XOR		"xor	$v0, $v0, $v1\n"
#define GEN_XORSZ	strlen(GEN_XOR)
#define GEN_OR		"or	$v0, $v0, $v1\n"
#define GEN_ORSZ	strlen(GEN_OR)

#define GEN_ASSIGN	"sw	$v0, 0($v1)\n"
#define GEN_ASSIGNSZ	strlen(GEN_ASSIGN)
#define GEN_ASSIGN8	"sb	$v0, 0($v1)\n"
#define GEN_ASSIGN8SZ	strlen(GEN_ASSIGN8)

#define GEN_JMP		"j             \n"
#define GEN_JMPSZ	strlen(GEN_JMP)

#define GEN_JZ		"beq	$v0, $zero,             \n"
#define GEN_JZSZ	strlen(GEN_JZ)

void gen_start();
void gen_finish();
void gen_const(int n);
void gen_clear();
void gen_complement();
void gen_push();
void gen_poptop();
void gen_pop(int n);
void gen_stack_addr(int addr);
void gen_unref(int type);
void gen_call();
void gen_ret();
void gen_sym(struct symbol *sym);
void gen_loop_start();
void gen_sym_addr(struct symbol *sym);
void gen_fixaddr(void);
void gen_patch(char *op, int value);
void gen_array(int ts, int size);
void gen_array_str(struct symbol *s, char *array, int size, char global, char empty);
void gen_array_int0(struct symbol *s, char global);
void gen_array_int1(struct symbol *s, char global);
void gen_array_int2(char empty);
void gen_array_int3(char global);
