#include <asgard.h>

/*
 * BACKEND
 */
int curr_codebuffsz = CODESZ;
int curr_databuffsz = DATASZ;
char *code;
char *data;
int codepos = 0, datapos = 0;

void emit(void *buf, size_t len)
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

void emitd(void *buf, size_t len)
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
