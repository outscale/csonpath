#if !defined(CSONPATH_JSON) || !defined(CSONPATH_NULL) || !defined(CSONPATH_GET) || \
    !defined(CSONPATH_IS_STR) ||					\
  !defined(CSONPATH_AT) || !defined(CSONPATH_IS_OBJ) || !defined(CSONPATH_IS_ARRAY) || \
  !defined(CSONPATH_CALLBACK) || !defined(CSONPATH_CALLBACK_DATA) || \
  !defined(CSONPATH_EQUAL_STR) || !defined(CSONPATH_CALL_CALLBACK) || \
  !defined(CSONPATH_FOREACH_EXT) || !defined(CSONPATH_APPEND_AT) || \
  !defined(CSONPATH_REMOVE_CHILD) || !defined(CSONPATH_NEED_FOREACH_REDO) || \
  !defined(CSONPATH_ARRAY_APPEND_INCREF) || !defined(CSONPATH_REMOVE)
# error "some defined are missing"
#endif

#ifndef CSONPATH_NO_REGEX
#  if defined CSONPATH_TINY_REGEX
#    include <tiny-regex-c/re.h>
typedef char * csonpath_reg_t;
#  elif defined CSONPATH_PCRE2
#    define PCRE2_CODE_UNIT_WIDTH 8
#    include <pcre2.h>
typedef pcre2_code * csonpath_reg_t;
#  elif defined CSONPATH_PCRE2_POSIX
#    include <pcre2posix.h>
typedef regex_t csonpath_reg_t;
#  else
#    include <regex.h>
typedef regex_t csonpath_reg_t;
#  endif
#endif

#if defined CSONPATH_TINY_REGEX
#  define CSONPATH_HAVE_TINY_REGEX 1
#else
#  define CSONPATH_HAVE_TINY_REGEX 0
#endif


#define CSONPATH_UNUSED __attribute__((unused))
#define MAY_ALIAS __attribute__((__may_alias__))

#ifndef CSONPATH_FOREACH
# define CSONPATH_FOREACH(obj, el, code)	\
  CSONPATH_FOREACH_EXT(obj, el, code, key_idx)
#endif

#ifndef CSONPATH_FIND_ALL_RET
#define CSONPATH_FIND_ALL_RET CSONPATH_JSON
#endif

#ifndef CSONPATH_FIND_ALL_RET_INIT
#define CSONPATH_FIND_ALL_RET_INIT CSONPATH_NEW_ARRAY
#endif

#ifndef CSONPATH_DECREF
#define CSONPATH_DECREF(obj)
#endif

#ifndef CSONPATH_FORMAT_EXCEPTION
#define CSONPATH_FORMAT_EXCEPTION(args...) fprintf(stderr, args)
#endif

#ifndef CSONPATH_EXCEPTION
#define CSONPATH_EXCEPTION(args...) CSONPATH_GETTER_ERR(args)
#endif

enum csonpath_instuction_raw {
	CSONPATH_INST_ROOT,
	CSONPATH_INST_GET_OBJ,
	CSONPATH_INST_GET_ARRAY_SMALL,
	CSONPATH_INST_GET_ARRAY_BIG,
	CSONPATH_INST_FILTER_KEY_SUPERIOR,
	CSONPATH_INST_FILTER_KEY_INFERIOR,
	CSONPATH_INST_FILTER_KEY_EQ,
	CSONPATH_INST_FILTER_KEY_NOT_EQ,
	CSONPATH_INST_FILTER_KEY_REG_EQ,
	CSONPATH_INST_FILTER_OPERAND_STR,
	CSONPATH_INST_FILTER_AND,
	CSONPATH_INST_GET_ALL,
	CSONPATH_INST_FIND_ALL,
	CSONPATH_INST_RANGE,
	CSONPATH_INST_OR,
	CSONPATH_INST_END,
	CSONPATH_INST_BROKEN
};

static int csonpath_instuction_len[] = {
    1, /* 0 CSONPATH_INST_ROOT */
    -1, /* 1 CSONPATH_INST_GET_OBJ */
    2, /* 2 CSONPATH_INST_GET_ARRAY_SMALL */
    5, /* 3 CSONPATH_INST_GET_ARRAY_BIG */
    2, /* 4 CSONPATH_INST_FILTER_KEY_SUPERIOR */
    2, /* 5 CSONPATH_INST_FILTER_KEY_INFERIOR */
    2, /* 6 CSONPATH_INST_FILTER_KEY_EQ */
    2, /* 7 CSONPATH_INST_FILTER_KEY_NOT_EQ */
    2, /* 8 CSONPATH_INST_FILTER_KEY_REG_EQ */
    -1, /* 9 CSONPATH_INST_FILTER_OPERAND_STR */
    1, /* 10 CSONPATH_INST_FILTER_AND */
    1, /* 11 CSONPATH_INST_GET_ALL */
    -1, /* 12 CSONPATH_INST_FIND_ALL */
    1, /* 13 CSONPATH_INST_RANGE */
    1, /* 14 CSONPATH_INST_OR */
    1, /* 15 CSONPATH_INST_END */
    1, /* 16 CSONPATH_INST_BROKEN */
};

/* this should be in an include, but doing so, would break the
 * "no more than 2 file", for single header lib */
CSONPATH_UNUSED static const char *csonpath_instuction_str[] = {
	"ROOT",
	"GET_OBJ",
	"GET_ARRAY_SMALL",
	"GET_ARRAY_BIG",
	"FILTER_KEY_SUPERIOR",
	"FILTER_KEY_INFERIOR",
	"FILTER_KEY_EQ",
	"FILTER_KEY_NOT_EQ",
	"FILTER_KEY_REG_EQ",
	"FILTER_OPERAND_STR",
	"FILTER_AND",
	"GET_ALL",
	"FIND_ALL",
	"RANGE",
	"OR",
	"END",
	"BROKEN"
};

enum {
	CSONPATH_NONE,
	CSONPATH_INTEGER,
	CSONPATH_STR
};

struct csonpath {
    char *compile_error;
    int return_empty_array;
#if !defined(CSONPATH_NO_REGEX) && !defined(CSONPATH_TINY_REGEX)
    int regex_cnt;
    csonpath_reg_t *regexs;
#ifdef CSONPATH_PCRE2
    pcre2_match_data **match_datas;
#endif
#endif
    char data[];
};

struct csonpath_child_info {
	int type;
	MAY_ALIAS union {
		int idx;
		const char *key;
	};
};

#define CSONPATH_ERROR_MAX_SIZE 1024
#define CSONPATH_TMP_BUF_SIZE 256

/* I'm assuming error message won't be longer than 125 */
#define CSONPATH_COMPILE_ERR(tmp, idx, args...) do {			\
	int ltmp  = strlen(tmp), lidx, oidx = idx;			\
	if (cjp->compile_error)						\
	    free(cjp->compile_error);					\
	cjp->compile_error = malloc(ltmp * 2 + CSONPATH_ERROR_MAX_SIZE); \
	lidx = snprintf(cjp->compile_error, CSONPATH_ERROR_MAX_SIZE, "colum %d:\n", oidx); \
	strcpy(cjp->compile_error + lidx, tmp);				\
	ltmp += lidx;							\
	cjp->compile_error[ltmp++] = '\n';				\
	for (; oidx; --oidx) {cjp->compile_error[ltmp++] = ' ';} \
	snprintf(cjp->compile_error + ltmp, CSONPATH_ERROR_MAX_SIZE - lidx, "\\-- "args);	\
	goto error;							\
    } while (0)

#define CSONPATH_REQUIRE_ERR(c, on) do {		\
	CSONPATH_COMPILE_ERR(tmp, on - orig,		\
			     "'%c' require", c);	\
	goto error;					\
    } while (0)

#define CSONPATH_SKIP_2(c, check, on) do {			\
	if (check) {						\
	    CSONPATH_REQUIRE_ERR(c, on);			\
	}							\
	++on;							\
    } while (0)

#define CSONPATH_SKIP(c, on)			\
    CSONPATH_SKIP_2(c, *on != c, on)


static inline struct csonpath_child_info *csonpath_child_info_set(struct csonpath_child_info *child_info,
								  CSONPATH_JSON j, const intptr_t key)
{
    if (CSONPATH_IS_OBJ(j)) {
	*child_info = (struct csonpath_child_info){.key=(const char *)key, .type=CSONPATH_STR};
    } else {
	*child_info = (struct csonpath_child_info){.idx=key, .type=CSONPATH_INTEGER};
    }
    return child_info;
}

static inline void csonpath_destroy(struct csonpath *cjp)
{
    if (!cjp)
	return;
    free(cjp->compile_error);
#if !defined CSONPATH_NO_REGEX && !defined(CSONPATH_TINY_REGEX)
    if (cjp->regex_cnt) {
	for (int i = 0; i < cjp->regex_cnt; ++i) {
#  ifdef CSONPATH_PCRE2
	    pcre2_code_free(cjp->regexs[i]);
	    pcre2_match_data_free(cjp->match_datas[i]);
#  else
	    regfree(&cjp->regexs[i]);
#  endif
	}

#  ifdef CSONPATH_PCRE2
	free(cjp->match_datas);
#  endif
	free(cjp->regexs);
    }
#endif
    free(cjp);
}

static _Bool csonpath_is_one_char_instruction(int c)
{
    return c == '|' || c == '$';
}

#define CSONPATH_BYTE_PER_INST 2

static int csonpath_compile(struct csonpath *cjp, const char path[static 1]);

static struct csonpath *csonpath_new_(const char path[static 1], int need_destroy) {
    /*
     * max inst is use so we know, we will never overflow.
     */
    int max_inst = strlen(path) / 2 + 1;
    struct csonpath *ret = malloc(sizeof *ret + CSONPATH_BYTE_PER_INST * max_inst + strlen(path) + 1);
    memset(ret, 0, sizeof *ret);
    if (!ret) {
	return NULL;
    }

    if (csonpath_compile(ret, path) < 0) {
	if (!need_destroy)
	    return ret;
	fprintf(stderr, "compile error: %s\n", ret->compile_error);
	csonpath_destroy(ret);
	return NULL;
    }
    return ret;
}

static inline struct csonpath *csonpath_new_no_destroy(const char path[static 1]) {
    return csonpath_new_(path, 0);
}

static inline struct csonpath *csonpath_new(const char path[static 1]) {
    return csonpath_new_(path, 1);
}

static inline struct csonpath *csonpath_set_path(struct csonpath cjp[static 1],
						 const char path[static 1])
{
    csonpath_destroy(cjp);
    struct csonpath *r = csonpath_new(path);
    return r;
}

static inline void csonpath_push_char(struct csonpath cjp[static 1], int inst, int *inst_idx)
{
    cjp->data[(*inst_idx)++] = inst;
}

static const char *csonpath_walker_next_inst(const char *walker)
{
    int l = csonpath_instuction_len[(int)*walker];
    /* skipp 2, 1 for the instruction, and another for \0 */
    if (l < 0)
	return &walker[strlen(walker + 1) + 2];
    return &walker[l];
}

static void csonpath_print_instruction(const struct csonpath cjp[const static 1])
{
    const char *walker = cjp->data, *origin = cjp->data;
    if (*walker == CSONPATH_INST_BROKEN) {
	printf("0: BROKEN\n");
	return;
    }
    for (;*walker != CSONPATH_INST_END; walker = csonpath_walker_next_inst(walker)) {
	printf("%d: %s\n", (int)(intptr_t)(walker - origin), csonpath_instuction_str[(int)*walker]);
    }
    printf("%d: END\n", (int)(intptr_t)(walker - origin));
}

static inline _Bool csonpath_is_dot_operand(int c)
{
    return isalnum(c) || c == '_' || c == '-';
}


static void csonpath_fill_walker_with_int(struct csonpath cjp[static 1], int *inst_idx,
					 int num)
{
    if (num < 100) {
	csonpath_push_char(cjp, CSONPATH_INST_GET_ARRAY_SMALL, inst_idx);
	cjp->data[(*inst_idx)++] = num;
    } else {
	csonpath_push_char(cjp, CSONPATH_INST_GET_ARRAY_BIG, inst_idx);
	union {
	    int n;
	    char c[4];
	} u = {.n=num};
	cjp->data[(*inst_idx)++] = u.c[0];
	cjp->data[(*inst_idx)++] = u.c[1];
	cjp->data[(*inst_idx)++] = u.c[2];
	cjp->data[(*inst_idx)++] = u.c[3];
    }
}

static int csonpath_int_from_walker(int operand_instruction, const char **walker)
{
    int ret = -1;
    switch(operand_instruction) {
    case CSONPATH_INST_GET_ARRAY_SMALL:
	ret = (*walker)[0];
	(*walker) += 1;
	break;
    case CSONPATH_INST_GET_ARRAY_BIG:
    {
	union {int n; char c[4];} to_num =
	    { .c= { (*walker)[0], (*walker)[1], (*walker)[2], (*walker)[3] } };
	ret = to_num.n;
	*walker = &(*walker)[4];
    }
    default:
	break;
    }
    return ret;
}

static int csonpath_compile(struct csonpath *cjp, const char path[static 1])
{
	const char *walker = path;
	const char *orig = walker;
	const char *next;
	char to_check;
	int inst_idx = 0;
	char *tmp; /* tmp is only here for debug */
	int inst;

	tmp = strdup(path);
  root_again:
	CSONPATH_SKIP('$', walker);
	csonpath_push_char(cjp, CSONPATH_INST_ROOT, &inst_idx);
	to_check = *walker;

  again:
	switch (to_check) {
	case '[':
	{
	    int end;

	    inst = 0;
	  do_array:
	    ++walker;
	    if (*walker == '*') {
		if (inst == CSONPATH_INST_FIND_ALL) {
		    CSONPATH_COMPILE_ERR(tmp, walker - orig, "'%c' is invalide here\n",
					 *walker);
			goto error;
		}
		csonpath_push_char(cjp, CSONPATH_INST_GET_ALL, &inst_idx);
		if (walker[1] != ']') {
		    CSONPATH_COMPILE_ERR(tmp, walker - orig, "%s", "unclose bracket\n");
		}
		walker += 2;
		to_check = *walker;
		goto again;
	    } else if (*walker == '?') {
		int have_blank;
		int have_parentesis = 0;
		int nb_getter_inst = 0;
		char filter_getter[CSONPATH_TMP_BUF_SIZE];
		int regex_idx = -1;
		int getter_end = 0;
		int operand_instruction;

		if (inst == CSONPATH_INST_FIND_ALL) {
		    CSONPATH_COMPILE_ERR(tmp, walker - orig, "'%c' is invalide here\n",
					 *walker);
		    goto error;
		}
	      filter_again_root:
		inst = CSONPATH_INST_GET_OBJ;
		filter_getter[nb_getter_inst++] = CSONPATH_INST_GET_OBJ;
		++walker;

		/* skipp blank */
		for (; isblank(*walker); ++walker);

		if (*walker == '(') {
		    ++have_parentesis;
		    ++walker;
		    CSONPATH_SKIP('@', walker);
		    if (*walker == '[') {
			++walker;
			getter_end = *walker;
		    } else {
			CSONPATH_SKIP('.', walker);
		    }
		    for (; isblank(*walker); ++walker);
		} else if (*walker == '[') {
		    ++walker;
		    getter_end = *walker;
		}
	      filter_again:
		if (getter_end) {
		    if (getter_end != '\'' && getter_end != '"') {
			CSONPATH_COMPILE_ERR(tmp, walker - orig,
					     "string require here, got '%c'", getter_end);
			goto error;
		    }
		    ++walker;
		    for (next = walker; *next != getter_end; ++next)
			filter_getter[nb_getter_inst++] = *next;
		    filter_getter[nb_getter_inst++] = 0;
		} else {
		    for (next = walker; csonpath_is_dot_operand(*next); ++next)
			filter_getter[nb_getter_inst++] = *next;
		    filter_getter[nb_getter_inst++] = 0;
		}
		if (!*next) {
		    CSONPATH_COMPILE_ERR(tmp, next - orig,
					 "filter miss condition");
		    goto error;
		}
		to_check = *next;
		if (getter_end) {
		    CSONPATH_SKIP_2(getter_end, to_check != getter_end, next);
		    CSONPATH_SKIP(']', next);
		    to_check = *next;
		    getter_end = 0;
		}
		if (to_check == '.') {
		    walker = next + 1;
		    filter_getter[nb_getter_inst++] = CSONPATH_INST_GET_OBJ;
		    goto filter_again;
		} else if (to_check == '[') {
		    walker = next + 1;
		    filter_getter[nb_getter_inst++] = CSONPATH_INST_GET_OBJ;
		    getter_end = *walker;
		    goto filter_again;
		}


		if (have_parentesis && to_check == ')') {
		    ++next;
		    to_check = *next;
		    have_parentesis--;
		}

		walker = next;
		have_blank = isblank(to_check);

		for (++next; isblank(*next); ++next) {
		    have_blank = 1;
		}

		if (*next && have_blank &&
		    to_check != '=' && to_check != '!') {
		    to_check = *next;
		    ++next;
		}

		if (!*next) {
		    CSONPATH_COMPILE_ERR(tmp, next - orig,
					 "filter miss condition");

		    goto error;
		}
		/* = and == are the same here */
		if (to_check == '=') {
		    if (next[0] == '=') {
			csonpath_push_char(cjp, CSONPATH_INST_FILTER_KEY_EQ, &inst_idx);
			++next;
		    }
#ifndef CSONPATH_NO_REGEX
		    else if (next[0] == '~') {
			csonpath_push_char(cjp, CSONPATH_INST_FILTER_KEY_REG_EQ,  &inst_idx);
#ifndef CSONPATH_TINY_REGEX
			regex_idx = cjp->regex_cnt++;
#endif
			++next;
		    }
#endif
		    else { /* a = b, so same as == */
			csonpath_push_char(cjp, CSONPATH_INST_FILTER_KEY_EQ, &inst_idx);
		    }
		} else if (to_check == '!' && next[0] == '=') {
		    csonpath_push_char(cjp, CSONPATH_INST_FILTER_KEY_NOT_EQ, &inst_idx);
		    ++next;
		} else if (to_check == '>') {
		    csonpath_push_char(cjp, CSONPATH_INST_FILTER_KEY_SUPERIOR, &inst_idx);
		} else if (to_check == '<') {
		    csonpath_push_char(cjp, CSONPATH_INST_FILTER_KEY_INFERIOR, &inst_idx);
		} else {
		    CSONPATH_COMPILE_ERR(tmp, next - orig,
					 "'%c': unsuported operation", to_check);
		    goto error;
		}
		operand_instruction = cjp->data[inst_idx - 1];
		for (;isblank(*next); ++next);
		char *filter_end = &cjp->data[inst_idx++];
		for (int i = 0; i < nb_getter_inst; ++i) {
		    csonpath_push_char(cjp, filter_getter[i], &inst_idx);
		}
		*filter_end = (nb_getter_inst - 1);
		walker = next;
		if (*walker == '"' || *walker == '\'' || *walker == '/') {
		    if (operand_instruction == CSONPATH_INST_FILTER_KEY_SUPERIOR ||
			operand_instruction == CSONPATH_INST_FILTER_KEY_INFERIOR) {
			CSONPATH_COMPILE_ERR(tmp, walker - orig, "string unsuported here");
			goto error;
		    }

		    char end = *walker;
		    ++walker;
		    if (regex_idx < 0 || CSONPATH_HAVE_TINY_REGEX) {
			csonpath_push_char(cjp, CSONPATH_INST_FILTER_OPERAND_STR, &inst_idx);
			for (next = walker; *next && *next != end; ++next)
			    cjp->data[inst_idx++] = *next;
			cjp->data[inst_idx++] = 0;
			if (!*next) {
			    CSONPATH_COMPILE_ERR(tmp, walker - orig,
						 "broken filter");
			    goto error;
			}
		    }
#if !defined(CSONPATH_NO_REGEX) && !defined(CSONPATH_TINY_REGEX)
		    else {
			csonpath_push_char(cjp, CSONPATH_INST_GET_ARRAY_SMALL, &inst_idx);
			csonpath_push_char(cjp, regex_idx, &inst_idx);
			if (!regex_idx) {
			    cjp->regexs = malloc(sizeof *cjp->regexs * 255);
#  ifdef CSONPATH_PCRE2
			    cjp->match_datas = malloc(sizeof *cjp->match_datas * 255);
#  endif
			}
			for (next = walker; *next && *next != end; ++next);
			char *reg_tmp = malloc(next - walker + 1);
			char *crawler = reg_tmp;
			for (next = walker; *next && *next != end; ++next, ++crawler)
			    *crawler = *next;
			*crawler = 0;
#  if defined CSONPATH_PCRE2
			int errorcode;
			PCRE2_SIZE erroroffset;

			cjp->regexs[regex_idx] = pcre2_compile((unsigned char *)reg_tmp,
							       PCRE2_ZERO_TERMINATED, 0,
							       &errorcode, &erroroffset, NULL);
			if (!cjp->regexs[regex_idx]) {
			    PCRE2_UCHAR buffer[256];
			    pcre2_get_error_message(errorcode, buffer, sizeof(buffer));
			    CSONPATH_COMPILE_ERR(tmp, next - orig, "regex has error: \"%s\"\n", buffer);
			    goto error;
			}
			cjp->match_datas[regex_idx] = pcre2_match_data_create_from_pattern(cjp->regexs[regex_idx],
											   NULL);
			pcre2_jit_compile(cjp->regexs[regex_idx], PCRE2_JIT_COMPLETE);
#  else
			int e = regcomp(&cjp->regexs[regex_idx], reg_tmp, 0);
			if (e) {
			    CSONPATH_COMPILE_ERR(tmp, next - orig, "regex has error\n");
			    goto error;
			}
#  endif
			free(reg_tmp);
		    }
#endif
		    ++next;
		    to_check = *next;
		} else {
		    int n;

		    if (operand_instruction == CSONPATH_INST_FILTER_KEY_REG_EQ) {
			CSONPATH_COMPILE_ERR(tmp, walker - orig, "number unsuported for regex");
			goto error;
		    }

		    for (next = walker; isdigit(*next); ++next);
		    if (next == walker) {
			CSONPATH_COMPILE_ERR(tmp, walker - orig,
					     "'%c': broken filter with number",
					     *walker);
			goto error;
		    } else if (!*next) {
			CSONPATH_COMPILE_ERR(tmp, walker - orig,
					     "unclose filter");
			goto error;
		    }

		    n = atoi(walker);
		    to_check = *next;
		    csonpath_fill_walker_with_int(cjp, &inst_idx, n);
		}

		/* skip space */
		if (isblank(to_check)) {
		    for (next++; isblank(*next); ++next);
		    to_check = *next;
		}

		if (to_check == '&') {
		    if (next[1] == '&') {
			++next;
		    }
		    walker = next + 1;
		    to_check = *walker;
		    csonpath_push_char(cjp, CSONPATH_INST_FILTER_AND, &inst_idx);
		    nb_getter_inst = 0;
		    goto filter_again_root;
		}

		while (have_parentesis) {
		    CSONPATH_SKIP(')', next);
		    to_check = *next;
		    --have_parentesis;
		}

		if (to_check != ']') {
		    CSONPATH_REQUIRE_ERR(']', next);
		}
		walker = next + 1;
		to_check = *walker;
		goto again;
		/* Filter out */
	    } else if (*walker != '"' && *walker != '\'') {
		int num;

		if (inst == CSONPATH_INST_FIND_ALL) {
		    CSONPATH_COMPILE_ERR(tmp, walker - orig, ".. require string\n");
		    goto error;
		}

		next = walker;
		do {
		  if (*next == ':') {
		    csonpath_push_char(cjp, CSONPATH_INST_RANGE, &inst_idx);
		    num = atoi(walker);
		    csonpath_fill_walker_with_int(cjp, &inst_idx, num);
		    walker = next + 1;
		    ++next;
		    continue;
		  }
		  if (*next < '0' || *next > '9') {
		    CSONPATH_COMPILE_ERR(tmp, walker - orig,
					 "unexpected '%c', sting, filter or number require\n", *next);
		  }
		  next++;
		} while (*next && *next != ']');
		if (!*next) {
		    CSONPATH_COMPILE_ERR(tmp, walker - orig,
					 "%s", "unclose bracket\n");
		}
		num = atoi(walker);
		csonpath_fill_walker_with_int(cjp, &inst_idx, num);
		walker = next + 1;
		to_check = *walker;
		goto again;
	    } else {
		end = *walker;
		if (inst != CSONPATH_INST_FIND_ALL)
		    inst = CSONPATH_INST_GET_OBJ;

		++walker;
		next = walker;
		csonpath_push_char(cjp, inst, &inst_idx);
		while (*next != end) {
		    /* \" should be ignored */
		    csonpath_push_char(cjp, *next, &inst_idx);
		    ++next;
		    while (*next == '\\')
			++next;
		}
		++next; // skipp end
		csonpath_push_char(cjp, 0, &inst_idx);
		if (*next != ']')
		    CSONPATH_COMPILE_ERR(tmp, walker - orig,
					 "']' require instead of '%c'\n", *next);


		walker = next + 1;
		to_check = *walker;
		goto again;
	    }
	}
	case '.':
	{
	    inst = CSONPATH_INST_GET_OBJ;

	    ++walker;
	    if (*walker == '.') {
		inst = CSONPATH_INST_FIND_ALL;
		++walker;
	    } else if (*walker == '*') {
		inst = CSONPATH_INST_GET_ALL;
		++walker;
		if (*walker != '.' && *walker != '[' && *walker != '\0') {
		    CSONPATH_COMPILE_ERR(tmp, walker - orig, "unsuported characters '%c' after '*'", *walker);
		    goto error;
		}
		csonpath_push_char(cjp, inst, &inst_idx);
		to_check = *walker;
		goto again;
	    }
	    for (next = walker; csonpath_is_dot_operand(*next); ++next);
	    if (next == walker) {
		if (*next == '[')
		    goto do_array;
		CSONPATH_COMPILE_ERR(tmp, walker - orig, "empty getter");
	    }
	    to_check = *next;

	    csonpath_push_char(cjp, inst, &inst_idx);
	    for (; walker != next; ++walker) {
		cjp->data[inst_idx++] = *walker;
	    }
	    cjp->data[inst_idx++] = 0;
	    goto again;
	}
	case '|':
	    csonpath_push_char(cjp, CSONPATH_INST_OR, &inst_idx);
	    ++walker;
	    goto root_again;
	}
	if (isblank(*walker)) {
	    ++walker;
	    goto again;
	}
	else if (*walker == 0) {
	    csonpath_push_char(cjp, CSONPATH_INST_END, &inst_idx);
	    free(cjp->compile_error);
	    cjp->compile_error = NULL;
	    free(tmp);
	    return 0;
	} else {
	    CSONPATH_COMPILE_ERR(tmp, walker - orig, "unexpected char '%c'", to_check);
	}
  error:
	cjp->data[0] = CSONPATH_INST_BROKEN;
	free(tmp);
	return -1;
}

static _Bool csonpath_do_match(int operand_instruction, CSONPATH_JSON el2, const char **owalker)
{
    switch (operand_instruction) {
    case CSONPATH_INST_FILTER_OPERAND_STR:
    {
	_Bool ret = CSONPATH_EQUAL_STR(el2, *owalker);
	*owalker += strlen(*owalker) + 1;
	return ret;
    }
    case CSONPATH_INST_GET_ARRAY_SMALL:
    {
	_Bool ret = CSONPATH_EQUAL_NUM(el2, **owalker);
	*owalker += 1;
	return ret;
    }
    case CSONPATH_INST_GET_ARRAY_BIG:
    {
	union {int n; char c[4];} to_num =
	    { .c= { (*owalker)[0], (*owalker)[1], (*owalker)[2], (*owalker)[3] } };

	*owalker += 4;
	return CSONPATH_EQUAL_NUM(el2, to_num.n);
    }
    }
    return 0;
}

static CSONPATH_JSON cosnpath_crawl_filter_el(const struct csonpath cjp[const static 1],
					      const char **owalker,
					      CSONPATH_JSON el2,
					      int filter_next)
{
    const char *end = *owalker + filter_next;
    for (; *owalker <= end; ) {
	switch (**owalker) {
	case CSONPATH_INST_GET_OBJ:
	    ++*owalker;
	    el2 = CSONPATH_GET(el2, *owalker);
	    if (!el2)
		return NULL;
	    break;
	default:
	    CSONPATH_FORMAT_EXCEPTION("unsuported %s inst",
				      csonpath_instuction_str[(int)**owalker]);
	    return NULL;
	}
	(*owalker) += (strlen(*owalker) + 1);
    }
    return el2;
}


static _Bool csonpath_make_match(const struct csonpath cjp[const static 1],
				 CSONPATH_JSON el2, const char **owalker, int operation)
{
    int operand_instruction = **owalker;
    ++*owalker;

    if (el2 == CSONPATH_NULL)
	return 0;
    _Bool match = 0;
    switch (operation) {
    case CSONPATH_INST_FILTER_KEY_NOT_EQ:
	match = !csonpath_do_match(operand_instruction, el2, owalker);
	break;
    case CSONPATH_INST_FILTER_KEY_EQ:
	match = csonpath_do_match(operand_instruction, el2, owalker);
	break;
    case CSONPATH_INST_FILTER_KEY_SUPERIOR:
	if (!CSONPATH_IS_NUM(el2))
	    break;
	match = csonpath_int_from_walker(operand_instruction, owalker) <
	    CSONPATH_GET_NUM(el2);
	break;
    case CSONPATH_INST_FILTER_KEY_INFERIOR:
	if (!CSONPATH_IS_NUM(el2))
	    break;
	match = csonpath_int_from_walker(operand_instruction, owalker) >
	    CSONPATH_GET_NUM(el2);
	break;
    case CSONPATH_INST_FILTER_KEY_REG_EQ:
#if defined CSONPATH_NO_REGEX
	fprintf(stderr, "regex deactivate");
	return 0;
#else
	if (CSONPATH_IS_STR(el2)) {
#if defined CSONPATH_TINY_REGEX
	    const char *regex_idx = *owalker;
	    *owalker += strlen(regex_idx) + 1;
	    int match_len = 0;
	    const char *str = CSONPATH_GET_STR(el2);
	    re_match(regex_idx, str, &match_len);
	    match = !!match_len;
#else
	    int regex_idx = **owalker;
	    ++*owalker;
# if defined CSONPATH_PCRE2
	    const unsigned char *str = (const unsigned char *)CSONPATH_GET_STR(el2);
	    int ret = pcre2_match(cjp->regexs[regex_idx], str, PCRE2_ZERO_TERMINATED, 0,
				  0, cjp->match_datas[regex_idx], NULL);

	    if (ret >= 0) {
		// match found
		return 1;
	    } else if (ret == PCRE2_ERROR_NOMATCH) {
		// no match
		return 0;
	    } else {
		fprintf(stderr, "pcre2 match error: %d\n", ret);
		return 0;
	    }
# else
	    regex_t *compiled = &cjp->regexs[regex_idx];
	    int match_len = regexec(compiled, CSONPATH_GET_STR(el2),
				    0, NULL, 0);
	    match = match_len == 0;
# endif
#endif
	}
	break;
#endif
    }
    return match;
}

static _Bool csonpath_is_endish_inst(int instruction)
{
    return instruction == CSONPATH_INST_END || instruction == CSONPATH_INST_OR;
}

/* helper use multiple times */
#define CSONPATH_DO_GET_NOTFOUND_UPDATER(this_idx)			\
    do {								\
	int append_ret = 0;						\
	if (to_check == CSONPATH_INST_GET_OBJ) {			\
	    tmp = CSONPATH_NEW_OBJECT();				\
	    append_ret = CSONPATH_APPEND_AT(ctx, this_idx, tmp);	\
	    CSONPATH_DECREF(tmp);					\
	} else {							\
	    tmp = CSONPATH_NEW_ARRAY();					\
	    append_ret = CSONPATH_APPEND_AT(ctx, this_idx, tmp);	\
	    CSONPATH_DECREF(tmp);					\
	}								\
	if (append_ret < 0) return append_ret;				\
	goto next_inst;							\
    } while (0)


#define CSONPATH_GOTO_ON_RELOOP(where)			\
    nb_res += tret; if (need_reloop_in) goto where;

#define CSONPATH_PREPARE_RELOOP(label)		\
    int need_reloop_in;				\
label:						\
need_reloop_in = 0;


#define CSONPATH_NONE_FOUND_RET CSONPATH_NULL

#define CSONPATH_GETTER_ERR(args...) do {	\
		fprintf(stderr, args);		\
		return CSONPATH_NULL;		\
	} while (0)

/* Find First */

#define CSONPATH_DO_RET_TYPE CSONPATH_JSON
#define CSONPATH_DO_FUNC_NAME find_first
#define CSONPATH_DO_RETURN return tmp

#define CSONPATH_DO_FIND_ALL if (tret) return tret

#define CSONPATH_DO_FILTER_FIND return tret

#define CSONPATH_DO_FIND_ALL_OUT return CSONPATH_NULL

#include "csonpath_do.h"

/* Find All */

#define CSONPATH_DO_PRE_OPERATION		\
  CSONPATH_FIND_ALL_RET ret_ar = CSONPATH_FIND_ALL_RET_INIT();

#define CSONPATH_DO_POST_OPERATION		\
  if (ret == CSONPATH_NULL) CSONPATH_REMOVE(ret_ar)

#define CSONPATH_DO_DECLARATION			\
	int nb_res = 0;

#define CSONPATH_DO_FUNC_NAME find_all
#define CSONPATH_DO_RET_TYPE CSONPATH_FIND_ALL_RET
#define CSONPATH_DO_RETURN ({CSONPATH_ARRAY_APPEND_INCREF(ret_ar, tmp); return ret_ar;})

#define CSONPATH_DO_FIND_ALL						\
    if (tret) ++nb_res;							\

#define CSONPATH_DO_FILTER_FIND CSONPATH_DO_FIND_ALL

#define CSONPATH_DO_FIND_ALL_OUT		\
	if (!cjp->return_empty_array && !nb_res) {				\
	return CSONPATH_NONE_FOUND_RET;		\
    }						\
    return ret_ar;

#define CSONPATH_DO_EXTRA_ARGS_IN , ret_ar
#define CSONPATH_DO_EXTRA_DECLATION , CSONPATH_FIND_ALL_RET ret_ar

#include "csonpath_do.h"

/* Delete */

#define CSONPATH_DO_FUNC_NAME remove

#undef CSONPATH_NONE_FOUND_RET
#undef CSONPATH_GETTER_ERR

#define CSONPATH_NONE_FOUND_RET 0

#define CSONPATH_GETTER_ERR(args...) do {	\
		fprintf(stderr, args);		\
		return -1;			\
	} while (0)

#define CSONPATH_DO_ON_FOUND

#define CSONPATH_DO_RET_TYPE int
#define CSONPATH_DO_RETURN						\
	({if (ctx == in_ctx && need_reloop &&				\
	      CSONPATH_NEED_FOREACH_REDO(ctx))				\
			*need_reloop = 1;				\
		CSONPATH_REMOVE_CHILD(ctx, child_info); return 1;})

#define CSONPATH_PRE_GET_OBJ(val)		\
    const char *to_del = val;

#define CSONPATH_DO_POST_FIND_ARRAY		\
	child_info.type = CSONPATH_INTEGER;	\
	child_info.idx = this_idx;

#define CSONPATH_DO_POST_FIND_OBJ		\
	child_info.type = CSONPATH_STR;		\
	child_info.key = to_del;

#define CSONPATH_DO_DECLARATION  int nb_res = 0;	\
	CSONPATH_JSON in_ctx = ctx;			\
	(void)in_ctx;

#define CSONPATH_DO_FIND_ALL_OUT return nb_res;

#define CSONPATH_DO_FILTER_FIND nb_res += tret;

#define CSONPATH_DO_FIND_ALL ({					\
	    if (tret < 0) return -1;				\
	    nb_res += tret;					\
	    if (need_reloop_in){ goto again; };			\
	})

#define CSONPATH_DO_RANGE ({						\
	    if (tret < 0) return -1;					\
	    nb_res += tret;						\
	    --end;							\
	    if (need_reloop_in){ goto range_again; };			\
	})

#define CSONPATH_DO_EXTRA_DECLATION , struct csonpath_child_info child_info, int *need_reloop

#define CSONPATH_DO_EXTRA_ARGS_IN , (struct csonpath_child_info) {.type = CSONPATH_NONE}, NULL

#define CSONPATH_DO_RANGE_PRE_LOOP		\
    int need_reloop_in;				\
range_again:

#define CSONPATH_DO_FILTER_PRE_LOOP		\
	int need_reloop_in;

#define CSONPATH_DO_FIND_ALL_PRE_LOOP		\
	int need_reloop_in;			\
again:

#define CSONPATH_DO_FILTER_LOOP_PRE_SET					\
    csonpath_child_info_set(&child_info, tmp, foreach_idx);

#define CSONPATH_DO_FOREACH_PRE_SET					\
    need_reloop_in = 0;							\
    csonpath_child_info_set(&child_info, tmp, (intptr_t)key_idx);

#define CSONPATH_DO_EXTRA_ARGS_NEESTED , child_info, &need_reloop_in

#define CSONPATH_DO_EXTRA_ARGS_FIND_ALL , child_info, need_reloop


#include "csonpath_do.h"

/* update_or_create */

#define CSONPATH_DO_DECLARATION			\
	int nb_res = 0;

#define CSONPATH_DO_RET_TYPE int
#define CSONPATH_DO_FUNC_NAME update_or_create


/*
 * assuming tmp == value can only be true,
 * while been called from FIND/FILTER or GET
 * otherwise CSONPATH_PRE_GET, is the part doing the buisness
 */
#define CSONPATH_DO_RETURN						\
	if (tmp == value) {						\
		*need_reloop = 1;					\
		if (child_info->type == CSONPATH_INTEGER)		\
			return CSONPATH_APPEND_AT(ctx, child_info->idx, to_update); \
		else							\
			return CSONPATH_APPEND_AT(ctx, child_info->key, to_update); \
		return 1;						\
	}								\
	return 0;


#define CSONPATH_DO_EXTRA_ARGS_FIND_ALL , to_update, NULL, need_reloop
#define CSONPATH_DO_EXTRA_ARGS_NEESTED , to_update,			\
		csonpath_child_info_set(&(struct csonpath_child_info ){}, tmp, (intptr_t)key_idx), &need_reloop_in
#define CSONPATH_DO_EXTRA_ARGS , CSONPATH_JSON to_update
#define CSONPATH_DO_EXTRA_ARGS_IN , to_update, NULL, NULL
#define CSONPATH_DO_EXTRA_DECLATION CSONPATH_DO_EXTRA_ARGS, struct csonpath_child_info *child_info, int *need_reloop
#define CSONPATH_DO_FIND_ALL nb_res += tret;
#define CSONPATH_DO_FILTER_FIND CSONPATH_GOTO_ON_RELOOP(filter_again)

#define CSONPATH_DO_FIND_ALL_PRE_LOOP int need_reloop_in = 0;

#define CSONPATH_DO_FILTER_PRE_LOOP CSONPATH_PREPARE_RELOOP(filter_again)

#define CSONPATH_DO_FIND_ALL_OUT return nb_res;

static int csonpath_sync_root_array(CSONPATH_JSON parent, CSONPATH_JSON to_update)
{
    CSONPATH_JSON child;
    size_t idx;
    (void) idx;

    CSONPATH_ARRAY_CLEAR(parent);
    CSONPATH_FOREACH_ARRAY(to_update, child, idx) {
	CSONPATH_APPEND_AT(parent, idx, child);
    }
    return 1;
}

static int csonpath_sync_root_obj(CSONPATH_JSON parent, CSONPATH_JSON to_update)
{
    CSONPATH_JSON child;
    const char *key;

    CSONPATH_OBJ_CLEAR(parent);
    CSONPATH_FOREACH_OBJ(to_update, child, key) {
	CSONPATH_APPEND_AT(parent, key, child);
    }
    return 1;
}

#define CSONPATH_PRE_GET_ROOT						\
    int to_check = walker[1];						\
    if (to_check == CSONPATH_INST_END || to_check == CSONPATH_INST_OR) { \
	if (CSONPATH_IS_OBJ(origin) && CSONPATH_IS_OBJ(to_update)) {	\
	    return csonpath_sync_root_obj(origin, to_update);		\
	} else if (CSONPATH_IS_ARRAY(origin) && CSONPATH_IS_ARRAY(to_update)) { \
	    return csonpath_sync_root_array(origin, to_update);		\
	} else {							\
	    CSONPATH_EXCEPTION("can't upate root ($)\n");		\
	}								\
    }

#define CSONPATH_PRE_GET(this_idx)					\
    const char *check_at = walker;					\
    int to_check;							\
    do {								\
	check_at = csonpath_walker_next_inst(check_at);			\
	to_check = *check_at;						\
    } while (to_check == CSONPATH_INST_GET_ALL || to_check == CSONPATH_INST_FIND_ALL); \
    if (to_check == CSONPATH_INST_END || to_check == CSONPATH_INST_OR) { \
	CSONPATH_APPEND_AT(ctx, this_idx, to_update);			\
	return 1;							\
    }


#define CSONPATH_DO_GET_NOTFOUND(this_idx)		\
    CSONPATH_DO_GET_NOTFOUND_UPDATER(this_idx)


#include "csonpath_do.h"

/* callback */

#define CSONPATH_DO_DECLARATION			\
  int nb_res = 0;

#define CSONPATH_DO_RET_TYPE int
#define CSONPATH_DO_FUNC_NAME callback
#define CSONPATH_DO_RETURN  do {					\
    CSONPATH_CALL_CALLBACK(callback, ctx, child_info, tmp, udata); return 1;} \
  while (0)

#define CSONPATH_DO_EXTRA_ARGS_FIND_ALL , callback, udata, child_info
#define CSONPATH_DO_EXTRA_ARGS_NEESTED , callback, udata,		\
    csonpath_child_info_set(child_info, tmp, (intptr_t)key_idx)
#define CSONPATH_DO_EXTRA_ARGS , CSONPATH_CALLBACK callback, CSONPATH_CALLBACK_DATA udata
#define CSONPATH_DO_EXTRA_ARGS_IN , callback, udata, &(struct csonpath_child_info ){}
#define CSONPATH_DO_EXTRA_DECLATION CSONPATH_DO_EXTRA_ARGS, struct csonpath_child_info *child_info

#define CSONPATH_DO_FIND_ALL nb_res += tret;
#define CSONPATH_DO_FILTER_FIND nb_res += tret;

#define CSONPATH_DO_FIND_ALL_OUT return nb_res;

#define CSONPATH_PRE_GET(this_idx)					\
  csonpath_child_info_set(child_info, ctx, (intptr_t)this_idx)



#include "csonpath_do.h"

/* update_or_create_callback */

#define CSONPATH_DO_DECLARATION			\
	int nb_res = 0;

#define CSONPATH_DO_RET_TYPE int
#define CSONPATH_DO_FUNC_NAME update_or_create_callback
#define CSONPATH_DO_RETURN						\
	if (tmp == value) {						\
		*need_reloop = 1;					\
	}								\
	CSONPATH_CALL_CALLBACK(callback, ctx, child_info, tmp, udata);	\
	return 1;

#define CSONPATH_DO_EXTRA_ARGS_FIND_ALL , callback, udata, NULL, need_reloop
#define CSONPATH_DO_EXTRA_ARGS_NEESTED , callback, udata,	\
	csonpath_child_info_set(&(struct csonpath_child_info ){}, tmp, (intptr_t)key_idx), \
	&need_reloop_in
#define CSONPATH_DO_EXTRA_ARGS , CSONPATH_CALLBACK callback, CSONPATH_CALLBACK_DATA udata
#define CSONPATH_DO_EXTRA_ARGS_IN , callback, udata, &(struct csonpath_child_info ){}, NULL
#define CSONPATH_DO_EXTRA_DECLATION CSONPATH_DO_EXTRA_ARGS, struct csonpath_child_info *child_info, int *need_reloop
#define CSONPATH_DO_FIND_ALL nb_res += tret;
#define CSONPATH_DO_FILTER_FIND CSONPATH_GOTO_ON_RELOOP(filter_again)

#define CSONPATH_DO_FIND_ALL_PRE_LOOP int need_reloop_in = 0;

#define CSONPATH_DO_FILTER_PRE_LOOP CSONPATH_PREPARE_RELOOP(filter_again)

#define CSONPATH_DO_FIND_ALL_OUT return nb_res;

#define CSONPATH_PRE_GET_ROOT						\
    int to_check = *walker;						\
    if (to_check == CSONPATH_INST_END || to_check == CSONPATH_INST_OR)  { \
	CSONPATH_GETTER_ERR("can't upate root ($)\n");			\
	return CSONPATH_NONE_FOUND_RET;					\
    }


#define CSONPATH_PRE_GET(this_idx)					\
    int to_check = *walker;						\
    csonpath_child_info_set(child_info, ctx, (intptr_t)this_idx)


#define CSONPATH_DO_GET_NOTFOUND(this_idx)		\
    CSONPATH_DO_GET_NOTFOUND_UPDATER(this_idx)


#include "csonpath_do.h"
