/*
Viking16-32 target backend

this is the compiler backend (just a code generator).
no optimization is performed, and the stack is used very often.

used registers:
r0	- temporary
r1	- primary register (low)
r2	- primary register (high)
r3	- secondary register (low)
r4	- secondary register (high)
r5	- 0x8000 (initialized to speed up signed comparisons)
r6	- return address
r7	- stack pointer
*/

#define TYPE_NUM_SIZE 4
#define SW_MULDIV

#define GEN_ADD		"\tadd	r1,r1,r3\n\tadc	r2,r2,r4\n"
#define GEN_ADDSZ	strlen(GEN_ADD)

#define GEN_SUB		"\tsub	r1,r3,r1\n\tsbc	r2,r4,r2\n"
#define GEN_SUBSZ	strlen(GEN_SUB)

#define GEN_MUL		"\tsub	sp,2\n\tstw	r4,sp\n\tsub	sp,2\n\tstw	r3,sp\n\tsub	sp,2\n\tstw	r2,sp\n\tsub	sp,2\n\tstw	r1,sp\n\tldi	r1,mulsi3\n"
#define GEN_MULSZ	strlen(GEN_MUL)

#define GEN_DIV		"\tsub	sp,2\n\tstw	r4,sp\n\tsub	sp,2\n\tstw	r3,sp\n\tsub	sp,2\n\tstw	r2,sp\n\tsub	sp,2\n\tstw	r1,sp\n\tldi	r1,divsi3\n"
#define GEN_DIVSZ	strlen(GEN_DIV)

#define GEN_REM		"\tsub	sp,2\n\tstw	r4,sp\n\tsub	sp,2\n\tstw	r3,sp\n\tsub	sp,2\n\tstw	r2,sp\n\tsub	sp,2\n\tstw	r1,sp\n\tldi	r1,modsi3\n"
#define GEN_REMSZ	strlen(GEN_REM)

#define GEN_SHL		"\tbez	r1,8\n\tlsl	r3,r3\n\trol	r4,r4\n\tsub	r1,1\n\tbnz	r1,-8\n\tand	r1,r3,r3\n\tand	r2,r4,r4\n"
#define GEN_SHLSZ	strlen(GEN_SHL)

#define GEN_SHRA	"\tbez	r1,8\n\tasr	r4,r4\n\tror	r3,r3\n\tsub	r1,1\n\tbnz	r1,-8\n\tand	r1,r3,r3\n\tand	r2,r4,r4\n"
#define GEN_SHRASZ	strlen(GEN_SHRA)

#define GEN_SHR		"\tbez	r1,8\n\tlsr	r4,r4\n\tror	r3,r3\n\tsub	r1,1\n\tbnz	r1,-8\n\tand	r1,r3,r3\n\tand r2,r4,r4\n"
#define GEN_SHRSZ	strlen(GEN_SHR)

//#define GEN_LESS	"\tslt	r1,r3,r1\n"
#define GEN_LESS	"\tldr	r0,0x80\n\tldc	r0,0x00\n\txor	r2,r2,r0\n\txor	r4,r4,r0\n\txor	r0,r0,r0\n\tsub	r1,r3,r1\n\tsbc	r2,r4,r2\n\tadc	r1,r0,r0\n\txor	r2,r2,r2\n"
#define GEN_LESSSZ	strlen(GEN_LESS)
//#define GEN_LEQ		"\tslt	r1,r1,r2\n\txor	r1,1\n"
#define GEN_LEQ		"\tldr	r0,0x80\n\tldc	r0,0x00\n\txor	r2,r2,r0\n\txor	r4,r4,r0\n\txor	r0,r0,r0\n\tsub	r1,r1,r3\n\tsbc	r2,r2,r4\n\tadc	r1,r0,r0\n\txor	r2,r2,r2\n\txor	r1,1\n"
#define GEN_LEQSZ	strlen(GEN_LEQ)
//#define GEN_GREATER	"\tslt	r1,r1,r3\n"
#define GEN_GREATER	"\tldr	r0,0x80\n\tldc	r0,0x00\n\txor	r2,r2,r0\n\txor	r4,r4,r0\n\txor	r0,r0,r0\n\tsub	r1,r1,r3\n\tsbc	r2,r2,r4\n\tadc	r1,r0,r0\n\txor	r2,r2,r2\n"
#define GEN_GREATERSZ	strlen(GEN_GREATER)
//#define GEN_GEQ		"\tslt	r1,r2,r1\n\txor	r1,1\n"
#define GEN_GEQ		"\tldr	r0,0x80\n\tldc	r0,0x00\n\txor	r2,r2,r0\n\txor	r4,r4,r0\n\txor	r0,r0,r0\n\tsub	r1,r3,r1\n\tsbc	r2,r4,r2\n\tadc	r1,r0,r0\n\txor	r2,r2,r2\n\txor	r1,1\n"
#define GEN_GEQSZ	strlen(GEN_GEQ)

//#define GEN_EQ		"\tsub	r1,r3,r1\n\tsltu	r1,1\n"
#define GEN_EQ		"\tsub	r1,r3,r1\n\tsbc	r2,r4,r2\n\tor	r1,r1,r2\n\txor	r2,r2,r2\n\tsltu	r1,1\n"
#define GEN_EQSZ	strlen(GEN_EQ)
//#define GEN_NEQ		"\tsub	r1,r3,r1\n\txor	at,at,at\n\tsltu	r1,at,r1\n"
#define GEN_NEQ		"\tsub	r1,r3,r1\n\tsbc	r2,r4,r2\n\tor	r1,r1,r2\n\txor	r2,r2,r2\n\tsltu	r1,1\n\txor	r1,1\n"
#define GEN_NEQSZ	strlen(GEN_NEQ)

#define GEN_AND		"\tand	r1,r1,r3\n\tand	r2,r2,r4\n"
#define GEN_ANDSZ	strlen(GEN_AND)
#define GEN_XOR		"\txor	r1,r1,r3\n\txor	r2,r2,r4\n"
#define GEN_XORSZ	strlen(GEN_XOR)
#define GEN_OR		"\tor 	r1,r1,r3\n\tor	r2,r2,r4\n"
#define GEN_ORSZ	strlen(GEN_OR)

#define GEN_ASSIGN	"\tstw	r1,r3\n\tadd	r3,2\n\tstw	r2,r3\n"
#define GEN_ASSIGNSZ	strlen(GEN_ASSIGN)
#define GEN_ASSIGN8	"\tstb	r1,r3\n"
#define GEN_ASSIGN8SZ	strlen(GEN_ASSIGN8)

#define GEN_JMP		"\tldi	at,            \n\tbnz	r7,at\n"
#define GEN_JMPSZ	strlen(GEN_JMP)

#define GEN_JZ		"\tldi	at,            \n\tbez	r1,at\n"
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
