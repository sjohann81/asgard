#include <asgard.h>
#include <gen.h>

/*
 * PARSER
 */

static int oper = 0;

/* read type name: int, char are supported */
int typename()
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

int prim_expr()
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

int binary(int type, int (*f)(), char *buf, size_t len, int fixref, int array)
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

int postfix_expr()
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
//		gen_const(0);	/* push a NULL to the parameters list */
//		gen_push();
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

int prefix_expr()
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

int mul_expr()
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

int add_expr()
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

int shift_expr()
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

int rel_expr()
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

int eq_expr()
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

int bitwise_expr()
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

int logical_expr()
{
// FIXME: check implementation of logical and and or operators (&& and ||)
	return bitwise_expr();
}

int ternary_expr()
{
// FIXME: implement ternary operator (? :)
	return logical_expr();
}

int expr()
{
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

void statement(int blabel, int clabel)
{
	int t, s, i; // a = 0;

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
				expect(";");
				
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
					//a = 1;
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
//			if (!a) {		// don't push array address to the stack
				gen_push();
				var->addr = stack_pos-1;
//			}
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

void compile()
{
	int i, j, n, type, s;
	char tok2[MAXTOKSZ], tok3[MAXTOKSZ];

	while (tok[0] != 0) { /* until EOF */
		if ((type = typename()) == 0) {
			error("error: type name expected\n");
		}
		struct symbol *var;

		do {
			s = 1;
			var = sym_declare(tok, 'U', 0);
			strcpy(tok2, tok);
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
				if (type == TYPE_STRUCT) {
					int entries = 0;

					expect("{");
					while ((type = typename())) {
						entries++;
						strcpy(tok3, tok2);
						strcat(tok3, ".");
						strcat(tok3, tok);
						if (type == TYPE_FUNC)
							error("error: functions are not allowed in this context\n");

						s = 1;
						var = sym_declare(tok3, 'U', 0);
						readtok();
						if (accept("[")) {
							if (!accept("]")) {
								s = atoi(tok);
								readtok();
								expect("]");
							}
						}
						expect(";");
						if (type == TYPE_INTVAR) {
							gen_array_int0(var, 1);
							for (i = 0; i < s; i++)
								gen_array_int2(1);
							gen_array_int3(1);
							var->addr = datapos;
							var->type = 'G';
							var->size = s;
						} else {
							gen_array_int0(var, 1);
							for (i = 0; i < (s / TYPE_NUM_SIZE) + 1; i++)
								gen_array_int2(1);
							gen_array_int3(1);
							var->addr = datapos;
							var->type = 'g';
							var->size = s;
						}
					}
					expect("}");
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
