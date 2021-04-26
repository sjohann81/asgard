#include <asgard.h>
#include <gen.h>

char tok[MAXTOKSZ]; /* current token */
int tokpos;         /* offset inside the current token */
int nextc;          /* next char to be pushed into token */
int line = 1;
char context[MAXTOKSZ] = "GLOBAL";

struct symbol *sym;
int curr_symbols = SYMBOLS;
int sympos = 0;
int stack_pos = 0;

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
