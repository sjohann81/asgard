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

#define GEN_ADD   "add    %ebx, %eax\n"
#define GEN_ADDSZ strlen(GEN_ADD)

#define GEN_SUB   "sub    %ebx, %eax\nneg    %eax\n"
#define GEN_SUBSZ strlen(GEN_SUB)

#define GEN_MUL   "imul    %ebx, %eax\n"
#define GEN_MULSZ strlen(GEN_MUL)

#define GEN_DIV   "xchg   %eax, %ebx\ncdq\nidiv   %ebx\n"
#define GEN_DIVSZ strlen(GEN_DIV)

#define GEN_REM   "xchg   %eax, %ebx\ncdq\nidiv   %ebx\nmov    %edx, %eax\n"
#define GEN_REMSZ strlen(GEN_REM)

#define GEN_SHL   "mov    %al, %cl\nshl    %cl, %ebx\nmov    %ebx, %eax\n"
#define GEN_SHLSZ strlen(GEN_SHL)

#define GEN_SHRA  "mov    %al, %cl\nsar    %cl, %ebx\nmov    %ebx, %eax\n"
#define GEN_SHRASZ strlen(GEN_SHRA)

#define GEN_SHR   "mov    %al, %cl\nshr    %cl, %ebx\nmov    %ebx, %eax\n"
#define GEN_SHRSZ strlen(GEN_SHR)

#define GEN_LESS  "cmp    %eax, %ebx\nsetl   %al\nmovzx  %al, %eax\n"
#define GEN_LESSSZ strlen(GEN_LESS)
#define GEN_LEQ  "cmp    %eax, %ebx\nsetle  %al\nmovzx  %al, %eax\n"
#define GEN_LEQSZ strlen(GEN_LEQ)
#define GEN_GREATER  "cmp    %eax, %ebx\nsetg   %al\nmovzx  %al, %eax\n"
#define GEN_GREATERSZ strlen(GEN_GREATER)
#define GEN_GEQ  "cmp    %eax, %ebx\nsetge  %al\nmovzx  %al, %eax\n"
#define GEN_GEQSZ strlen(GEN_GEQ)

#define GEN_EQ "cmp    %eax, %ebx\nsete   %al\nmovzx  %al, %eax\n"
#define GEN_EQSZ strlen(GEN_EQ)
#define GEN_NEQ  "cmp    %eax, %ebx\nsetne  %al\nmovzx  %al, %eax\n"
#define GEN_NEQSZ strlen(GEN_NEQ)

#define GEN_AND  "and    %ebx, %eax\n"
#define GEN_ANDSZ strlen(GEN_AND)
#define GEN_XOR  "xor    %ebx, %eax\n"
#define GEN_XORSZ strlen(GEN_XOR)
#define GEN_OR "or     %ebx, %eax\n"
#define GEN_ORSZ strlen(GEN_OR)

#define GEN_ASSIGN "movl   %eax, (%ebx)\n"
#define GEN_ASSIGNSZ strlen(GEN_ASSIGN)
#define GEN_ASSIGN8 "movb   %al, (%ebx)\n"
#define GEN_ASSIGN8SZ strlen(GEN_ASSIGN8)

#define GEN_JMP "jmp               \n"
#define GEN_JMPSZ strlen(GEN_JMP)

#define GEN_JZ "cmp    $0, %eax\nje                \n"
#define GEN_JZSZ strlen(GEN_JZ)

static void gen_start() {
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

static void gen_finish() {
	printf("%s\n", code);
	printf(".data\n");
	printf("%s\n", data);
}

/* put constant to primary register */
static void gen_const(int n) {
	emitf("mov    $0x%x, %%eax\n", n);
}

static void gen_clear(){
	emits("xor    %eax, %eax\n");
}

static void gen_complement(){
	emits("not    %eax\n");
}

static void gen_push() {
	emits("push   %eax\n");
	stack_pos = stack_pos + 1;
}

static void gen_poptop() {
	emits("pop    %ebx\n");
	stack_pos = stack_pos - 1;
}

static void gen_pop(int n) {
	if (n > 0) {
		emitf("add    $0x%04x, %%esp\n", n * TYPE_NUM_SIZE);
		stack_pos = stack_pos - n;
	}
}

static void gen_stack_addr(int addr) {
	emitf("lea    %d(%%esp), %%eax\n", addr * TYPE_NUM_SIZE);
}

static void gen_unref(int type) {
	if (type == TYPE_CHARVAR || type == TYPE_GCHARVAR) {
		emits("movsbl (%eax), %eax\n");
	}else{
		emits("mov    (%eax), %eax\n");
	}
}

/* Call function by address stored in primary register */
static void gen_call() {
	emits("call   *%eax\n");
}

/* return from function */
static void gen_ret() {
	emits("ret\n");
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
	emitf("mov    $%s, %%eax\n", sym->name);
}

static void gen_fixaddr(void){
	emits("shl    $2, %eax\n");
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
	emitf("sub    $0x%04x, %%esp\n", (size * ts) + p);
	stack_pos = stack_pos + ((size * ts) + p) / TYPE_NUM_SIZE;
	gen_stack_addr(0);
}

/* filled array or global array auxiliary functions */
static int array_index = 0;

static void gen_array_str(struct symbol *s, char *array, int size, char global, char empty) {
	int i;

	if (global){
		if (size > 1){
			emitfd("%s:	.long ___%s\n", s->name, s->name);
			if (empty){
				emitfd("___%s:", s->name);
				emitfd("   .space %d", size);
			}else{
				emitfd("___%s:	.ascii \"", s->name);
				for (i = 0; i < size; i++)
					emitfd("%c", array[i]);
				emitsd("\\0\"\n");
			}
		}else{
			emitfd("%s:	.long 0", s->name);
		}
		emitsd("\n");
	}else{
		emitfd("___s%d:	.ascii \"", array_index);
		emitsd(array);
		emitsd("\\0\"\n");
		emitf("mov    $___s%d, %%eax\n", array_index);
		array_index++;
	}
}

static int array_index_n = 0;

static void gen_array_int0(struct symbol *s, char global) {
	if (global){
		emitfd("%s:	.long ", s->name);
	}else{
		emitfd("___n%d:	.long ", array_index_n);
	}
}

static void gen_array_int1(struct symbol *s, char global) {
	if (global){
		emitfd("%s:	.long ___%s\n", s->name, s->name);
		emitfd("___%s:	.long ", s->name);
	}else{
		emitfd("___n%d:	.long ", array_index_n);
	}
}

static int not_first = 0;

static void gen_array_int2(char empty) {
	if (not_first) emitsd(",");
	if (empty){
		emitsd("0");
	}else{
		emitfd("%s", tok);
	}
	not_first = 1;
}

static void gen_array_int3(char global) {
	not_first = 0;
	emitsd("\n");
	if (!global){
		emitf("mov    $___n%d, %%eax\n", array_index_n);
		array_index_n++;
	}
}
