#include <asgard.h>

/* print error and die */
void error(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "line %d -> ", line);
	vfprintf(stderr, fmt, args);
	va_end(args);
	exit(1);
}
