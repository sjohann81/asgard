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

static void gen_start() {
	printf(".code\n");
	emits("\tldi	at,main\n");
	emits("\tldi	lr,___retmain\n");
	emits("\tbnz	r7,at\n");
	emits("___retmain\n");
	emits("\thcf\n");
	emits("putchar\n");
	emits("\tand	r2,sp,sp\n");
	emits("\tadd	r2,4\n");
	emits("\tldw	r1,r2\n");
	emits("\tstw	r1,0xf000\n");
	emits("\tbnz	r7,lr\n");
	emits("getchar\n");
	emits("\tldw	r1,0xf004\n\txor	r2,r2,r2\n");
	emits("\tbnz	r7,lr\n");
}

static void gen_finish() {
	printf("%s\n", code);
	printf(".data\n");
	printf("%s\n", data);
}

/* put constant to primary register */
static void gen_const(int n) {
	if (n >= -128 && n <= 127) {
			emitf("\tldr	r1,%d\n", n & 0xff);
			emits("\txor	r2,r2,r2\n");
	} else {
		if (n >= -32768 && n <= 32767) {
			emitf("\tldr	r1,%d\n", (n >> 8) & 0xff);
			emitf("\tldc	r1,%d\n", n & 0xff);
			emits("\txor	r2,r2,r2\n");
		} else {
			if (n >= -8388608 && n <= 8388607) {
				emitf("\tldr	r2,%d\n", (n >> 16) & 0xff);
				emitf("\tldr	r1,%d\n", (n >> 8) & 0xff);
				emitf("\tldc	r1,%d\n", n & 0xff);
			} else {
				emitf("\tldr	r2,%d\n", (n >> 24) & 0xff);
				emitf("\tldc	r2,%d\n", (n >> 16) & 0xff);
				emitf("\tldr	r1,%d\n", (n >> 8) & 0xff);
				emitf("\tldc	r1,%d\n", n & 0xff);
			}
		}
	}
}

static void gen_clear(){
	emits("\txor	r1,r1,r1\n\txor	r2,r2,r2\n");
}

static void gen_complement(){
	emits("\txor	r1,-1\n\txor	r2,-1\n");
}

static void gen_push() {
	emits("\tsub	sp,2\n\tstw	r2,sp\n\tsub	sp,2\n\tstw	r1,sp\n");
	stack_pos = stack_pos + 1;
}


static void gen_poptop() {
	emits("\tldw	r3,sp\n\tadd	sp,2\n\tldw	r4,sp\n\tadd	sp,2\n");
	stack_pos = stack_pos - 1;
}

static void gen_pop(int n) {
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

static void gen_stack_addr(int addr) {
	gen_const(addr * TYPE_NUM_SIZE);
	emits("\tadd	r1,sp,r1\n\txor	r2,r2,r2\n");
}

static void gen_unref(int type) {
	if (type == TYPE_CHARVAR || type == TYPE_GCHARVAR) {
		emits("\tldb	r1,r1\n\txor	r2,r2,r2\n");
	}else{
		emits("\tand	r0,r1,r1\n\tldw	r1,r0\n\tadd	r0,2\n\tldw	r2,r0\n");
	}
}

/* Call function by address stored in primary register */
static void gen_call() {
	static int32_t i = 0;
	char retbuf[50];

	sprintf(retbuf, "___ret%04x\n", i++);
	emits("\tsub	sp,2\n");
	emits("\txor	r0,r0,r0\n");
	emits("\tstw	r0,sp\n");
	emits("\tsub	sp,2\n");
	emits("\tstw	lr,sp\n");
	emitf("\tldi	lr,%s", retbuf);
	emits("\tbnz	r7,r1\n");
	emits(retbuf);
	emits("\tldw	lr,sp\n");
	stack_pos = stack_pos + 1;
}

/* return from function */
static void gen_ret() {
	emits("\tbnz	r7,lr\n");
}

static void gen_sym(struct symbol *sym) {
	if (sym->type == 'F') {
		emits(sym->name);
		emits("\n");
	}
}

static void gen_loop_start() {
	emitf("___%08x\n", codepos);
}

static void gen_sym_addr(struct symbol *sym) {
	emitf("\tldi	r1,%s\n", sym->name);
	emits("\txor	r2,r2,r2\n");
}

static void gen_fixaddr(void){
//	emits("\tlsl	r1,r1\n\tlsl	r1,r1\n");
	emits("\tlsl	r1,r1\n");
}

/* patch jump address */
static void gen_patch(char *op, int value) {
	char s[32];
	sprintf(s, "___%08x", value);
	if (value >= codepos) {
		emits(s);
		emits("\t\n");
	}
	memcpy(op-strlen(s)-13, s, strlen(s));
}

static void gen_array(int ts, int size) {
	int p = 0;

	while(((size * ts + p) % TYPE_NUM_SIZE) != 0) p++;
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

static void gen_array_str(struct symbol *s, char *array, int size, char global, char empty) {
	int i;

	if (global){
		if (size > 1){
			emitfd("%s	___%s\n", s->name, s->name);
			emitfd("___%s	\"", s->name);
			if (empty){
				for (i = 0; i < size; i++)
					emitsd(" ");
			}else{
				for (i = 0; i < size; i++)
					emitfd("%c", array[i]);
			}
			emitsd("\"\n");
		}else{
			emitfd("%s	\" \"\n", s->name);
		}
	}else{
		emitfd("___s%d	\"", array_index);
		emitsd(array);
		emitsd("\"\n");
		emitf("\tldi	r1,___s%d\n", array_index);
		emits("\txor	r2,r2,r2\n");
		array_index++;
	}
}

static int array_index_n = 0;

static void gen_array_int0(struct symbol *s, char global) {
	if (global){
		emitfd("%s	", s->name);
	}else{
		emitfd("___n%d	", array_index_n);
	}
}

static void gen_array_int1(struct symbol *s, char global) {
	if (global){
		emitfd("%s	___%s\n", s->name, s->name);
		emitfd("___%s	", s->name);
	}else{
		emitfd("___n%d	", array_index_n);
	}
}

static void gen_array_int2(char empty) {
	int val;

	if (empty){
		emitsd("0 ");
	}else{
		if isalpha(tok[0]) {
			emitfd("%s ", tok);
		} else {
			val = (int)strtol(tok, NULL, 0);
			emitfd("%d %d ", ((val >> 16) & 0xffff), (val & 0xffff));
		}
	}
}

static void gen_array_int3(char global) {
	emitsd("\n");
	if (!global){
		emitf("\tldi	r1,___n%d\n", array_index_n);
		emits("\txor	r2,r2,r2\n");
		array_index_n++;
	}
}
