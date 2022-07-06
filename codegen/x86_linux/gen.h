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

/* some helper macros to emit text */
/*
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
*/
#define TYPE_NUM_SIZE	4

#define GEN_ADD		"add    %ebx, %eax\n"
#define GEN_ADDSZ	strlen(GEN_ADD)

#define GEN_SUB		"sub    %ebx, %eax\nneg    %eax\n"
#define GEN_SUBSZ	strlen(GEN_SUB)

#define GEN_MUL		"imul    %ebx, %eax\n"
#define GEN_MULSZ	strlen(GEN_MUL)

#define GEN_DIV		"xchg   %eax, %ebx\ncdq\nidiv   %ebx\n"
#define GEN_DIVSZ	strlen(GEN_DIV)

#define GEN_REM		"xchg   %eax, %ebx\ncdq\nidiv   %ebx\nmov    %edx, %eax\n"
#define GEN_REMSZ	strlen(GEN_REM)

#define GEN_SHL		"mov    %al, %cl\nshl    %cl, %ebx\nmov    %ebx, %eax\n"
#define GEN_SHLSZ	strlen(GEN_SHL)

#define GEN_SHRA	"mov    %al, %cl\nsar    %cl, %ebx\nmov    %ebx, %eax\n"
#define GEN_SHRASZ	strlen(GEN_SHRA)

#define GEN_SHR		"mov    %al, %cl\nshr    %cl, %ebx\nmov    %ebx, %eax\n"
#define GEN_SHRSZ	strlen(GEN_SHR)

#define GEN_LESS	"cmp    %eax, %ebx\nsetl   %al\nmovzx  %al, %eax\n"
#define GEN_LESSSZ	strlen(GEN_LESS)
#define GEN_LEQ		"cmp    %eax, %ebx\nsetle  %al\nmovzx  %al, %eax\n"
#define GEN_LEQSZ	strlen(GEN_LEQ)
#define GEN_GREATER	"cmp    %eax, %ebx\nsetg   %al\nmovzx  %al, %eax\n"
#define GEN_GREATERSZ	strlen(GEN_GREATER)
#define GEN_GEQ		"cmp    %eax, %ebx\nsetge  %al\nmovzx  %al, %eax\n"
#define GEN_GEQSZ	strlen(GEN_GEQ)

#define GEN_EQ		"cmp    %eax, %ebx\nsete   %al\nmovzx  %al, %eax\n"
#define GEN_EQSZ	strlen(GEN_EQ)
#define GEN_NEQ		"cmp    %eax, %ebx\nsetne  %al\nmovzx  %al, %eax\n"
#define GEN_NEQSZ	strlen(GEN_NEQ)

#define GEN_AND		"and    %ebx, %eax\n"
#define GEN_ANDSZ	strlen(GEN_AND)
#define GEN_XOR		"xor    %ebx, %eax\n"
#define GEN_XORSZ	strlen(GEN_XOR)
#define GEN_OR		"or     %ebx, %eax\n"
#define GEN_ORSZ	strlen(GEN_OR)

#define GEN_ASSIGN	"movl   %eax, (%ebx)\n"
#define GEN_ASSIGNSZ	strlen(GEN_ASSIGN)
#define GEN_ASSIGN8	"movb   %al, (%ebx)\n"
#define GEN_ASSIGN8SZ	strlen(GEN_ASSIGN8)

#define GEN_JMP		"jmp               \n"
#define GEN_JMPSZ	strlen(GEN_JMP)

#define GEN_JZ		"cmp    $0, %eax\nje                \n"
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
