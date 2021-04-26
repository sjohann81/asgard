#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAXTOKSZ 128

extern char tok[MAXTOKSZ]; /* current token */
extern int tokpos;         /* offset inside the current token */
extern int nextc;          /* next char to be pushed into token */
extern int line;
extern char context[MAXTOKSZ];

#define SYMBOLS 128

struct symbol {
	char type;
	int addr;
	int size;
	char name[MAXTOKSZ];
	char context[MAXTOKSZ];
};

extern struct symbol *sym;
extern int curr_symbols;
extern int sympos;
extern int stack_pos;

#define CODESZ 4096
#define DATASZ 4096

extern int curr_codebuffsz;
extern int curr_databuffsz;
extern char *code, *data;
extern int codepos, datapos;

#define TYPE_NUM		0
#define TYPE_CHARVAR		1
#define TYPE_INTVAR		2
#define TYPE_STRUCT		3
#define TYPE_FUNC		10
#define TYPE_GCHARVAR		11
#define TYPE_GINTVAR		12

void error(const char *fmt, ...);

char fetch();
void readchr();
void readtok();
int peek(char *s);
int accept(char *s);
void expect(char *s);
int readnum();

struct symbol *sym_find(char *s);
struct symbol *sym_declare(char *name, char type, int addr);

int typename();
int prim_expr();
int binary(int type, int (*f)(), char *buf, size_t len, int fixref, int array);
int postfix_expr();
int prefix_expr();
int mul_expr();
int add_expr();
int shift_expr();
int rel_expr();
int eq_expr();
int bitwise_expr();
int logical_expr();
int ternary_expr();
int expr();
void statement(int blabel, int clabel);
void compile();

void emit(void *buf, size_t len);
void emitd(void *buf, size_t len);
