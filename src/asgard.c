#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAXTOKSZ 128

static char tok[MAXTOKSZ]; /* current token */
static int tokpos;         /* offset inside the current token */
static int nextc;          /* next char to be pushed into token */
static int line = 1;
static char context[MAXTOKSZ] = "GLOBAL";

#define SYMBOLS 128

struct symbol {
	char type;
	int addr;
	int size;
	char name[MAXTOKSZ];
	char context[MAXTOKSZ];
};

struct symbol *sym;
static int curr_symbols = SYMBOLS;
static int sympos = 0;
int stack_pos = 0;

/* print error and die */
static void error(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "line %d -> ", line);
	vfprintf(stderr, fmt, args);
	va_end(args);
	exit(1);
}

/*
 * LEXER
 */

char fetch()
{
	char c;

	c = getchar();
	if (c == '\n') line++;
	return c;
}

/* read next char */
void readchr()
{
	if (tokpos == MAXTOKSZ - 1) {
		tok[tokpos] = '\0';
		error("error: token too long: %s\n", tok);
	}
	tok[tokpos++] = nextc;
	nextc = fetch();
}

/* read single token */
void readtok()
{
	for (;;) {
		/* skip spaces */
		while (isspace(nextc)) {
			nextc = fetch();
		}
		/* try to read a literal token */
		tokpos = 0;
		while (isalnum(nextc) || nextc == '_' || nextc == '.') {
			readchr();
		}
		/* if it's not a literal token */
		if (tokpos == 0) {
			while (nextc == '<' || nextc == '=' || nextc == '>'
					|| nextc == '!' || nextc == '&' || nextc == '|' || nextc == '^') {
				readchr();
			}
		}
		/* if it's not special chars that looks like an operator */
		if (tokpos == 0) {
			/* try strings and chars inside quotes */
			if (nextc == '\'' || nextc == '"') {
				char c = nextc;
				readchr();
				while (nextc != c) {
					readchr();
				}
				readchr();
			} else if (nextc == '/') { /* skip comments */
				readchr();
				if (nextc == '*') {
					nextc = fetch();
					while (nextc != '/') {
						while (nextc != '*') {
							nextc = fetch();
						}
						nextc = fetch();
					}
					nextc = fetch();
					continue;
				}
			} else if (nextc != EOF) {
				/* otherwise it looks like a single-char symbol, like '+', '-' etc */
				readchr();
			}
		}
		break;
	}
	tok[tokpos] = '\0';
}

/* check if the current token machtes the string */
int peek(char *s)
{
	return (strcmp(tok, s) == 0);
}

/* read the next token if the current token matches the string */
int accept(char *s)
{
	if (peek(s)) {
		readtok();
		return 1;
	}
	return 0;
}

/* throw fatal error if the current token doesn't match the string */
void expect(char *s)
{
	if (accept(s) == 0) {
		error("error: expected '%s', but found: %s\n", s, tok);
	}
}

/* read a number (integer (base 10 or 16) or real)
 *
 * reals are encoded as 16.16 signed fixed point values.
 */
int readnum()
{
	int n, neg = 0;

	if (tok[0] == '-') {
		neg = 1;
		tok[0] = ' ';
	}

	if (tok[0] == '0' && tok[1] == 'x' && tok[2] != '\0') {
		n = strtol(tok, NULL, 16);
	} else {
		char *chk, *chk2;
		chk2 = strstr(tok, ".");
		double tmp = strtod( tok, &chk );
		if (chk2 && (isspace(*chk ) || *chk == 0)) {
			n = ((int)tmp) << 16;
			tmp -= (double)((int)tmp);
			tmp *= 65536;
			n |= (int)(tmp + 0.5);
		} else {
			n = strtol(tok, NULL, 10);
		}
	}
	if (neg)
		return -n;
	else
		return n;
}


/*
 * BACKEND
 */
#define CODESZ 4096
#define DATASZ 4096

static int curr_codebuffsz = CODESZ;
static int curr_databuffsz = DATASZ;
static char *code;
static char *data;
static int codepos = 0, datapos = 0;

static void emit(void *buf, size_t len)
{
	if ((codepos + len) >= curr_codebuffsz) {
		curr_codebuffsz *= 2;
		fprintf(stderr, "code buffer: %d bytes\n", curr_codebuffsz);
		code = realloc(code, curr_codebuffsz);
		if (!code)
			error("error: out of memory while rallocating the code buffer.\n");
	}
	memcpy(code + codepos, buf, len);
	codepos += len;
}

static void emitd(void *buf, size_t len)
{
	if ((datapos + len) >= curr_databuffsz) {
		curr_databuffsz *= 2;
		fprintf(stderr, "data buffer: %d bytes\n", curr_databuffsz);
		data = realloc(data, curr_databuffsz);
		if (!data)
			error("error: out of memory while rallocating the data buffer.\n");
	}
	memcpy(data + datapos, buf, len);
	datapos += len;
}

#define TYPE_NUM		0
#define TYPE_CHARVAR		1
#define TYPE_INTVAR		2
#define TYPE_STRUCT		3
#define TYPE_FUNC		10
#define TYPE_GCHARVAR		11
#define TYPE_GINTVAR		12

#ifndef CODEGEN
#error "A code generator (backend) must be specified"
#endif

#include CODEGEN

/*
 * SYMBOLS
 */
static struct symbol *sym_find(char *s)
{
	int i;
	struct symbol *sb = NULL;
	for (i = 0; i < sympos; i++) {
		if ((strcmp(sym[i].name, s) == 0) && ((strcmp(sym[i].context, context) == 0) || (strcmp(sym[i].context, "GLOBAL") == 0))) {
			sb = &sym[i];
		}
	}

	return sb;
}

static struct symbol *sym_declare(char *name, char type, int addr)
{
	if (sym_find(name))
		error("error: redeclaration of symbol %s on context %s\n", name, context);
	strncpy(sym[sympos].name, name, MAXTOKSZ);
	strncpy(sym[sympos].context, context, MAXTOKSZ);
	sym[sympos].addr = addr;
	sym[sympos].size = 0;
	sym[sympos].type = type;
	sympos++;
	if (sympos >= curr_symbols) {
		curr_symbols *= 2;
		fprintf(stderr, "symbol buffer: %d symbols\n", curr_symbols);
		sym = (struct symbol *)realloc(sym, curr_symbols * sizeof(struct symbol));
		if (!sym)
			error("error: out of memory while rallocating the symbol buffer.\n");
	}
#if TYPE_NUM_SIZE == 2
	fprintf(stderr, "%s [%s, offset %04x]\n", context, name, (addr*2) & 0xffff);
#else
	fprintf(stderr, "%s [%s, offset %08x]\n", context, name, addr*4);
#endif
	return &sym[sympos-1];
}

/*
 * PARSER
 */

static int oper = 0;
static int expr();

/* read type name: int, char are supported */
static int typename()
{
	if (peek("func")) {
		readtok();
		return TYPE_FUNC;
	}
	if (peek("int")) {		/* word */
		readtok();
		return TYPE_INTVAR;
	}
	if (peek("char")) {		/* byte */
		readtok();
		return TYPE_CHARVAR;
	}
	if (peek("struct")) {
		readtok();
		return TYPE_STRUCT;
	}

	return 0;
}

static int prim_expr()
{
	int i, j, n, type = TYPE_NUM;

	if (isdigit(tok[0])) {
		int n;
		n = readnum();
		gen_const(n);
	} else if (isalpha(tok[0])) {
		struct symbol *s = sym_find(tok);
		if (s == NULL) {
//			error("error: undeclared symbol: %s\n", tok);
			struct symbol s2;

			s = &s2;
			strncpy(s2.name, tok, MAXTOKSZ);
			gen_sym_addr(s);
			type = TYPE_GINTVAR;
		} else {
			if (s->type == 'L' || s->type == 'l') {
				gen_stack_addr(stack_pos - s->addr - 1);
			} else {
				gen_sym_addr(s);
			}
			if (s->type == 'L')
				type = TYPE_INTVAR;
			if (s->type == 'G')
				type = TYPE_GINTVAR;
			if (s->type == 'l')
				type = TYPE_CHARVAR;
			if (s->type == 'g')
				type = TYPE_GCHARVAR;
		}
	} else if (accept("(")) {
		type = expr();
		type = TYPE_NUM;
	} else if (tok[0] == '"') {
		i = 0; j = 1;
		while (tok[j] != '"') {
			if (tok[j] == '\\' && tok[j+1] == 'x') {
				char s[3] = {tok[j+2], tok[j+3], 0};
				uint8_t n = strtol(s, NULL, 16);
				tok[i++] = n;
				j += 4;
			} else {
				tok[i++] = tok[j++];
			}
		}
		tok[i++] = 0;
		gen_array_str(NULL, tok, i, 0, 0);
		type = TYPE_NUM;
	} else if (tok[0] == '\'' && tok[2] == '\'') {
		int n;
		n = tok[1];
		gen_const(n);
		type = TYPE_NUM;
	} else if (tok[0] == '{') {
		gen_array_int1(NULL, 0);
		while (tok[0] != '}') {
			readtok();
			if (tok[0] == '-') {
				j = 0;
				readtok();
				while (tok[j++] != '\0');
				tok[j+1] = '\0';
				while (j > 0) {
					tok[j] = tok[j-1];
					j--;
				}
				tok[0] = '-';
				n = readnum();
				sprintf(tok, "%d", n);
				gen_array_int2(0);
			} else {
				struct symbol *s = sym_find(tok);
				if (s == NULL) {
					n = readnum();
					sprintf(tok, "%d", n);
				}
				gen_array_int2(0);
			}
			accept(","); readtok();
		}
		gen_array_int3(0);
		type = TYPE_NUM;
	} else {
		error("error: unexpected primary expression: %s\n", tok);
	}
	readtok();

	return type;
}

static int binary(int type, int (*f)(), char *buf, size_t len, int fixref, int array)
{
	if (type != TYPE_NUM) {
		gen_unref(type);
	}
	
	gen_push();
	type = f();
	if (type != TYPE_NUM && !array) {
		gen_unref(type);
	}
	if (fixref) {
		gen_fixaddr();
	}
	gen_poptop();
	emit(buf, len);

	return TYPE_NUM;
}

static int postfix_expr()
{
	int type = prim_expr();

	if ((type == TYPE_INTVAR || type == TYPE_GINTVAR || type == TYPE_NUM) && accept("[")) { /* TYPE_NUM -> implements multiple indirection */
		binary(type, expr, GEN_ADD, GEN_ADDSZ, 1, 1);
		expect("]");
		type = TYPE_INTVAR;
	} else if ((type == TYPE_CHARVAR || type == TYPE_GCHARVAR) && accept("[")) {
		if (type == TYPE_CHARVAR) type = TYPE_INTVAR;
		else type = TYPE_GINTVAR;
		binary(type, expr, GEN_ADD, GEN_ADDSZ, 0, 1);
		expect("]");
		type = TYPE_CHARVAR;
		oper = 2;
	} else if (accept("(")) {
		int prev_stack_pos = stack_pos;
		gen_push(); /* store function address */
		int call_addr = stack_pos - 1;
		oper = 1;
		gen_const(0);	/* push a NULL to the parameters list */
		gen_push();
		if (accept(")") == 0) {
			do {
				type = expr();
				oper = 1;
				gen_push();
			} while (accept(","));
			expect(")");
		}
		oper = 0;
		type = TYPE_NUM;
		gen_stack_addr(stack_pos - call_addr - 1);
		gen_unref(TYPE_INTVAR);
		gen_call();
		/* remove function address and args */
		gen_pop(stack_pos - prev_stack_pos);
		stack_pos = prev_stack_pos;
	}
	if (type == TYPE_GINTVAR) type = TYPE_INTVAR;
	if (type == TYPE_GCHARVAR) type = TYPE_CHARVAR;
	if (type ==  TYPE_CHARVAR && oper != 2)		// fix is performed: this is a pointer to char or a char variable (not an array member): convert to int
		type = TYPE_INTVAR;
// FIXME: implement structure and union access member / through pointer (. and ->)
// FIXME: implement postfix inc/dec operators (++ and --)

	return type;
}

static int prefix_expr()
{
	int type;

// FIXME: implement logical/bitwise not operators (! and ~)
	if (peek("~")) {
		accept("~");
		expr();		// shouldn't be postfix_expr()? check this.
		gen_complement();
		type = TYPE_NUM;
// FIXME: implement prefix inc/dec operators (++ and --)
// FIXME: implement indirection / address of operators (* and &)
	} else if (peek("&")) {
		accept("&");
		prim_expr();
		type = TYPE_NUM;
	} else if (peek("*")) {
		accept("*");
		type = prim_expr();
		gen_unref(TYPE_NUM);
		gen_unref(type);
		type = TYPE_NUM;
// FIXME: implement sizeof operator
//	} else if (peek("$")) {
		
// FIXME: implement typecast
	} else if (peek("-")) {
		gen_clear();
		type = TYPE_NUM;
	} else {
		type = postfix_expr();
	}

	return type;
}

static int mul_expr()
{
	int type;

	type = prefix_expr();

	while (peek("*") || peek("/") || peek("%")) {
		if (accept("*")) {
			type = binary(type, prefix_expr, GEN_MUL, GEN_MULSZ, 0, 0);
		} else if (accept("/")) {
			type = binary(type, prefix_expr, GEN_DIV, GEN_DIVSZ, 0, 0);
		} else if (accept("%")) {
			type = binary(type, prefix_expr, GEN_REM, GEN_REMSZ, 0, 0);
		}
#ifdef SW_MULDIV
		gen_call();
		stack_pos = stack_pos + 2;
		gen_pop(3);
#endif
	}

	return type;
}

static int add_expr()
{
	int type;

	type = mul_expr();

	while (peek("+") || peek("-")) {
		if (accept("+")) {
			type = binary(type, mul_expr, GEN_ADD, GEN_ADDSZ, 0, 0);
		} else if (accept("-")) {
			type = binary(type, mul_expr, GEN_SUB, GEN_SUBSZ, 0, 0);
		}
	}

	return type;
}

static int shift_expr()
{
	int type;

	type = add_expr();

	while (peek("<<") || peek(">>") || peek(">>>")) {
		if (accept("<<")) {
			type = binary(type, add_expr, GEN_SHL, GEN_SHLSZ, 0, 0);
		} else if (accept(">>>")) {
			type = binary(type, add_expr, GEN_SHR, GEN_SHRSZ, 0, 0);
		} else if (accept(">>")) {
			type = binary(type, add_expr, GEN_SHRA, GEN_SHRASZ, 0, 0);
		}
	}

	return type;
}

static int rel_expr()
{
	int type;

	type = shift_expr();

	while (peek("<") || peek("<=") || peek(">") || peek(">=")) {
		if (accept("<=")) {
			type = binary(type, shift_expr, GEN_LEQ, GEN_LEQSZ, 0, 0);
		} else if (accept("<")) {
			type = binary(type, shift_expr, GEN_LESS, GEN_LESSSZ, 0, 0);
		} else if (accept(">=")) {
			type = binary(type, shift_expr, GEN_GEQ, GEN_GEQSZ, 0, 0);
		} else if (accept(">")) {
			type = binary(type, shift_expr, GEN_GREATER, GEN_GREATERSZ, 0, 0);
		}
	}

	return type;
}

static int eq_expr()
{
	int type;

	type = rel_expr();

	while (peek("==") || peek("!=")) {
		if (accept("==")) {
			type = binary(type, rel_expr, GEN_EQ, GEN_EQSZ, 0, 0);
		} else if (accept("!=")) {
			type = binary(type, rel_expr, GEN_NEQ, GEN_NEQSZ, 0, 0);
		}
	}

	return type;
}

static int bitwise_expr()
{
	int type;

	type = eq_expr();

	while (peek("|") || peek("&") || peek("^")) {
		if (accept("&")) {
			type = binary(type, eq_expr, GEN_AND, GEN_ANDSZ, 0, 0);
		} else if (accept("^")) {
			type = binary(type, eq_expr, GEN_XOR, GEN_XORSZ, 0, 0);
		} else if (accept("|")) {
			type = binary(type, eq_expr, GEN_OR, GEN_ORSZ, 0, 0);
		}
	}

	return type;
}

static int logical_expr()
{
// FIXME: check implementation of logical and and or operators (&& and ||)
	return bitwise_expr();
}

static int ternary_expr()
{
// FIXME: implement ternary operator (? :)
	return logical_expr();
}

static int expr() {
	int type = ternary_expr();
	if (type != TYPE_NUM) {
// FIXME: implement assignment by sum, difference, product, quotient, remainder, bitwise left
// and right shift, bitwise and, xor and or operators (+=, -=, *=, /=, %=, <<=, >>=, &=, ^=, |=)
		if (accept("=")) {
			gen_push();
			expr();
			if (type == TYPE_INTVAR) {
				gen_poptop();
				emit(GEN_ASSIGN, GEN_ASSIGNSZ);
			} else {
				gen_poptop();
				emit(GEN_ASSIGN8, GEN_ASSIGN8SZ);
			}
			type = TYPE_NUM;
		} else {
			if (oper == 1) {
				gen_unref(TYPE_INTVAR);
			} else {
				gen_unref(type);
			}
		}
	}

	return type;
}

static void statement(int blabel, int clabel)
{
	int t, s, i;

	if (accept("{")) {
		int prev_stack_pos = stack_pos;
		/* statement prolog */
		while (accept("}") == 0) {
			statement(blabel, clabel);
		}
		/* statement epilog */
		gen_pop(stack_pos-prev_stack_pos);
		stack_pos = prev_stack_pos;
	} else if ((t = typename())) {
		do {
			struct symbol *var;

			if (t == TYPE_FUNC)
				error("error: functions are not allowed in this context\n");
			if (t == TYPE_INTVAR || t == TYPE_GINTVAR)
				var = sym_declare(tok, 'L', stack_pos);
			else
				var = sym_declare(tok, 'l', stack_pos);

			if (t == TYPE_STRUCT) {
				struct symbol *var2;
				char tok2[MAXTOKSZ], tok3[MAXTOKSZ];
				int entries = 0;

				strcpy(tok2, tok);
				readtok();
				expect("{");
				while ((t = typename())) {
					entries++;
					strcpy(tok3, tok2);
					strcat(tok3, ".");
					strcat(tok3, tok);
					if (t == TYPE_FUNC)
						error("error: functions are not allowed in this context\n");
					if (t == TYPE_INTVAR || t == TYPE_GINTVAR)
						var2 = sym_declare(tok3, 'L', stack_pos);
					else
						var2 = sym_declare(tok3, 'l', stack_pos);
						
					readtok();
					if (accept("[")) {
						if (!accept("]")) {
							s = atoi(tok);
							if (t == TYPE_INTVAR)
								gen_array(TYPE_NUM_SIZE, s);
							if (t == TYPE_CHARVAR)
								gen_array(1, s);
							var2->size = s;
							readtok();
							expect("]");
						}
					} else {
						gen_push();
					}
					expect(";");
					var2->addr = stack_pos-1;
				}
				expect("}");
				
				for (i = 0; i < entries; i++)
					fprintf(stderr, "\n>>>%s, addr: %08x sz: %d", sym[sympos - 1 - i].name, sym[sympos - 1 - i].addr, sym[sympos - 1 - i].size);

				int addr = sym[sympos - 1].addr;
				int k = 0;
				for (i = 0; i < entries; i++) {
					sym[sympos - entries + i].addr = addr - i - k;
					if (sym[sympos - entries + i].size)
						k += sym[sympos - entries + i].size - 1;
				}

				for (i = 0; i < entries; i++)
					fprintf(stderr, "\n>>>FIX: %s, addr: %08x sz: %d", sym[sympos - 1 - i].name, sym[sympos - 1 - i].addr, sym[sympos - 1 - i].size);

				gen_stack_addr(0);
				gen_push();
				var->addr = stack_pos-1;
				oper = 0;
				return;
			}

			readtok();
			if (accept("[")) {
				if (!accept("]")) {
					s = atoi(tok);
					if (t == TYPE_INTVAR)
						gen_array(TYPE_NUM_SIZE, s);
					if (t == TYPE_CHARVAR)
						gen_array(1, s);
					readtok();
					expect("]");
				}
			} else {
				if (accept("=")) {
					expr();
				} else {
//					gen_clear();
				}
			}
			gen_push();
			var->addr = stack_pos-1;
		} while (accept(","));
		expect(";");
	} else if (accept("if")) {
		expect("(");
		expr();
		emit(GEN_JZ, GEN_JZSZ);
		int p1 = codepos;
		expect(")");
		int prev_stack_pos = stack_pos;
		statement(blabel, clabel);
		emit(GEN_JMP, GEN_JMPSZ);
		int p2 = codepos;
		gen_patch(code + p1, codepos);
		if (accept("else")) {
			stack_pos = prev_stack_pos;
			statement(blabel, clabel);
		}
		stack_pos = prev_stack_pos;
		gen_patch(code + p2, codepos);
	} else if (accept("while")) {
		expect("(");
		int p1 = codepos;
		gen_loop_start();
		expr();
		int p3 = codepos;
		gen_loop_start();
		emit(GEN_JZ, GEN_JZSZ);
		int p2 = codepos;
		expect(")");
		statement(p3, p1);
		emit(GEN_JMP, GEN_JMPSZ);
		gen_patch(code + codepos, p1);
		gen_patch(code + p2, codepos);
	} else if (accept("do")) {
		emit(GEN_JMP, GEN_JMPSZ);
		int p1 = codepos;
		gen_loop_start();
		emit(GEN_JMP, GEN_JMPSZ);
		int p2 = codepos;
		gen_loop_start();
		statement(p1, p2);
		expect("while");
		expect("(");
		expr();
		emit(GEN_JZ, GEN_JZSZ);
		expect(")");
		expect(";");
		int p3 = codepos;
		emit(GEN_JMP, GEN_JMPSZ);
		gen_patch(code + p1, p2);
		gen_patch(code + codepos, p2);
		gen_patch(code + p2, codepos);
		gen_patch(code + p3, codepos);
	} else if (accept("break")) {
		expect(";");
		gen_clear();
		emit(GEN_JMP, GEN_JMPSZ);
		gen_patch(code + codepos, blabel);
	} else if (accept("continue")) {
		expect(";");
		emit(GEN_JMP, GEN_JMPSZ);
		gen_patch(code + codepos, clabel);
	} else if (accept("return")) {
		if (peek(";") == 0) {
			expr();
		}
		expect(";");
		gen_pop(stack_pos); /* remove all locals from stack (except return address) */
		gen_ret();
	} else {
		expr();
		expect(";");
	}
	oper = 0;
}

static void compile()
{
	int i, j, n, type, s;

	while (tok[0] != 0) { /* until EOF */
		if ((type = typename()) == 0) {
			error("error: type name expected\n");
		}
		struct symbol *var;

		do {
			s = 1;
			var = sym_declare(tok, 'U', 0);
			readtok();

			if (accept("[")) {
				if (!accept("]")) {
					s = atoi(tok);
					readtok();
					expect("]");
				}
			}
			if (tok[0] == '(') {
				if (type != TYPE_FUNC)
					error("error: function declarations should not have a explicit return type\n");
				break;
			}
			if (accept("=")) {
				if (tok[0] == '"') {
					i = 0; j = 1;
					while (tok[j] != '"') {
						if (tok[j] == '\\' && tok[j+1] == 'x') {
							char s[3] = {tok[j+2], tok[j+3], 0};
							uint8_t n = strtol(s, NULL, 16);
							tok[i++] = n;
							j += 4;
						} else {
							tok[i++] = tok[j++];
						}
						s++;
					}
					tok[i++] = 0;
					gen_array_str(var, tok, i, 1, 0);
					var->addr = datapos;
					var->type = 'g';
					var->size = s;
					readtok();
				} else {
					if (type == TYPE_CHARVAR || type == TYPE_GCHARVAR)
						error("error: integer type expected\n");

					if (peek("{")) {
						accept("{");
						gen_array_int1(var, 1);
						while (tok[0] != '}') {
							if (tok[0] == '-') {
								j = 0;
								readtok();
								while (tok[j++] != '\0');
								tok[j+1] = '\0';
								while (j > 0) {
									tok[j] = tok[j-1];
									j--;
								}
								tok[0] = '-';
								n = readnum();
								sprintf(tok, "%d", n);
								gen_array_int2(0);
							} else {
								struct symbol *s = sym_find(tok);
								if (s == NULL) {
									n = readnum();
									sprintf(tok, "%d", n);
								}
								gen_array_int2(0);
							}
							readtok();
							accept(",");
							s++;
						}
						accept("}");
					} else {
						gen_array_int0(var, 1);
						if (tok[0] == '-') {
							j = 0;
							readtok();
							while (tok[j++] != '\0');
							tok[j+1] = '\0';
							while (j > 0) {
								tok[j] = tok[j-1];
								j--;
							}
							tok[0] = '-';
							n = readnum();
							sprintf(tok, "%d", n);
							gen_array_int2(0);
						} else {
							struct symbol *s = sym_find(tok);
							if (s == NULL) {
								n = readnum();
								sprintf(tok, "%d", n);
							}
							gen_array_int2(0);
						}
						readtok();
					}
					gen_array_int3(1);
					var->addr = datapos;
					var->type = 'G';
					var->size = s;
				}
			} else {
				if (type == TYPE_INTVAR) {
					if (s > 1) {
						gen_array_int1(var, 1);
						for (i = 0; i < s; i++)
							gen_array_int2(1);
					} else {
						gen_array_int0(var, 1);
						gen_array_int2(1);
					}
					gen_array_int3(1);
					var->addr = datapos;
					var->type = 'G';
					var->size = s;
				} else {
					gen_array_str(var, tok, s, 1, 1);
					var->addr = datapos;
					var->type = 'g';
					var->size = s;
				}
			}
		} while (accept(","));
		if (accept(";")) continue;
		strcpy(context, var->name);
		expect("(");
		int argc = 0;
		for (;;) {
			argc++;
			if ((type = typename()) == 0) {
				break;
			}
			if (type == TYPE_CHARVAR)
				sym_declare(tok, 'l', -argc-1);
			else
				sym_declare(tok, 'L', -argc-1);
			readtok();
			if (peek(")")) {
				break;
			}
			expect(",");
		}
		/* reverse the order of declared function parameters so it matches pushed parameters on calls */
		struct symbol s;
		for (i = 0; i < argc / 2; i++) {
			s = sym[sympos - argc + i];
			sym[sympos - argc + i].addr = sym[sympos - 1 - i].addr;
			sym[sympos - 1 - i].addr = s.addr;
		}
		expect(")");
		if (accept(";") == 0) {
			stack_pos = 0;
			var->addr = codepos;
			var->type = 'F';
			gen_sym(var);
			statement(0, 0); /* function body */
			gen_ret(); /* another ret if user forgets to put 'return' */
			strcpy(context, "GLOBAL");
		}
	}
}

int main(int argc, char *argv[])
{
	/* setup buffers */
	sym = (struct symbol *)malloc(sizeof(struct symbol) * SYMBOLS);
	code = malloc(sizeof(char) * CODESZ);
	data = malloc(sizeof(char) * DATASZ);

	if (!sym || !code || !data)
		error("error: out of memory\n");

	/* prefetch first char and first token */
	nextc = fetch();
	readtok();
	gen_start();
	sym_declare("putchar", 'U', 0);
	sym_declare("getchar", 'U', 0);
	compile();
	gen_finish();

	fprintf(stderr, "\nMemory usage:\ncode: %d bytes, data: %d bytes\nsymbol table: %lu bytes (%d symbols)\n",
		curr_codebuffsz, curr_databuffsz, curr_symbols * sizeof(struct symbol), curr_symbols);

	free(sym);
	free(code);
	free(data);

	return 0;
}
