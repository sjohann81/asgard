#include <asgard.h>

/*
 * SYMBOLS
 */
struct symbol *sym_find(char *s)
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

struct symbol *sym_declare(char *name, char type, int addr)
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
