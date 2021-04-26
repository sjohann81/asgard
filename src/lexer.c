#include <asgard.h>

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
