#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "csonpath_json-c.h"

/* A regex index is stored as a single byte inside the instruction stream and
 * read back as a signed int at runtime (csonpath_make_match). More than 127
 * regexes would wrap 0x80..0xFF into a negative index (OOB read before the
 * regexs[] table) and >= 256 would overflow the 255-slot allocation (heap
 * OOB write + invalid free in csonpath_destroy). Compilation must therefore
 * reject anything above 127 regexes cleanly. */
#define MAX_REGEX 127

int main(void)
{
	char path[8192];
	struct csonpath *cjp;

	/* MAX_REGEX + 1 -> must be a clean compile error (NULL), not heap
	 * corruption / a negative index. */
	strcpy(path, "$");
	for (int i = 0; i < MAX_REGEX + 1; ++i)
		strcat(path, "[?h=~\"x\"]");

	cjp = csonpath_new(path);
	assert(cjp == NULL);

	/* Exactly MAX_REGEX must compile and be destroyed safely. */
	strcpy(path, "$");
	for (int i = 0; i < MAX_REGEX; ++i)
		strcat(path, "[?h=~\"x\"]");

	cjp = csonpath_new(path);
	assert(cjp != NULL);
	csonpath_destroy(cjp);

	printf("OK: %d regexes rejected cleanly, %d accepted\n",
	       MAX_REGEX + 1, MAX_REGEX);
	return 0;
}