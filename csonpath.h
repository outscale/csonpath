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
#include <regex.h>
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
	CSONPATH_INST_FILTER_OPERAND_BYTE, /* store the same way it's in ARRAY_SMALL  */
	CSONPATH_INST_FILTER_OPERAND_INT, /* store the same way it's in ARRAY_BIG  */
	CSONPATH_INST_FILTER_AND,
	CSONPATH_INST_GET_ALL,
	CSONPATH_INST_FIND_ALL,
	CSONPATH_INST_RANGE,
	CSONPATH_INST_OR,
	CSONPATH_INST_END,
	CSONPATH_INST_BROKEN
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
	"FILTER_OPERAND_BYTE",
	"FILTER_OPERAND_INT",
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

struct csonpath_instruction {
    unsigned char inst;
    union {
	unsigned char filter_next;
	unsigned char regex_idx;
    };
    short int next;
};

struct csonpath {
    char *compile_error;
    char *path;
    struct csonpath_instruction *inst_lst;
    int compiled;
#ifndef CSONPATH_NO_REGEX
    int regex_cnt;
    regex_t *regexs;
#endif
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

static inline void csonpath_destroy(struct csonpath cjp[static 1])
{
	free(cjp->path);
	free(cjp->inst_lst);
	free(cjp->compile_error);
	if (cjp->regex_cnt) {
	    for (int i = 0; i < cjp->regex_cnt; ++i) {
		regfree(&cjp->regexs[i]);
	    }
	    free(cjp->regexs);
	}
	*cjp = (struct csonpath){};
}

static _Bool csonpath_is_one_char_instruction(int c)
{
    return c == '|' || c == '$';
}

static inline int csonpath_init(struct csonpath cjp[static 1],
				const char path[static 1]) {
    /*
     * We overalloc, but this enable us to avoid checking size,
     * as the only instruction that require 1 character is $, and $$ is not allow
     * we can safely assume that we can't have more instructions than path len / 2 + 2
     * we add plus 2, because "$", is a valid path, and contain 2 instruction: ROOT and END
     */
    int max_inst = strlen(path) / 2 + 1;
    for (const char *cpy = path; *cpy; ++cpy) {
	if (csonpath_is_one_char_instruction(*cpy))
	    ++max_inst;
    }
    *cjp = (struct csonpath) {.path=strdup(path),
	.inst_lst = malloc(sizeof(*cjp->inst_lst) * max_inst)};
    if (!cjp->path || !cjp->inst_lst) {
	return -ENOMEM;
    }
    return 0;
}

static inline int csonpath_set_path(struct csonpath cjp[static 1],
				    const char path[static 1])
{
    csonpath_destroy(cjp);
    return csonpath_init(cjp, path);
}

static inline void csonpath_push_inst(struct csonpath cjp[static 1], int inst, int *inst_idx)
{
    cjp->inst_lst[*inst_idx] = (struct csonpath_instruction){.inst=inst};
    *inst_idx += 1;
}

static inline void csonpath_push_inst_cpy(struct csonpath cjp[static 1],
					  const struct csonpath_instruction inst,
					  int *inst_idx)
{
    cjp->inst_lst[*inst_idx] = inst;
    *inst_idx += 1;
}

static void csonpath_print_instruction(struct csonpath cjp[static 1])
{
    int idx = 0;
    for (;cjp->inst_lst[idx].inst != CSONPATH_INST_END; ++idx) {
	printf("%d: %s\n", idx, csonpath_instuction_str[(int)cjp->inst_lst[idx].inst]);
    }
    printf("%d: END\n", idx);
}

static inline _Bool csonpath_is_dot_operand(int c)
{
    return isalnum(c) || c == '_' || c == '-';
}


static int csonpath_fill_walker_with_int(char *walker, int num, int first_ret)
{
    if (num < 100) {
	 *walker = num;
	 return first_ret;
    }
    union {
	int n;
	char c[4];
    } u = {.n=num};
    walker[0] = u.c[0];
    walker[1] = u.c[1];
    walker[2] = u.c[2];
    walker[3] = u.c[3];
    return first_ret + 1;
}

static int csonpath_int_from_walker(int operand_instruction, char *walker)
{
    switch(operand_instruction) {
    case CSONPATH_INST_FILTER_OPERAND_BYTE:
    case CSONPATH_INST_GET_ARRAY_SMALL:
	return *walker;
    case CSONPATH_INST_GET_ARRAY_BIG:
    case CSONPATH_INST_FILTER_OPERAND_INT:
    {
	union {int n; char c[4];} to_num =
	    { .c= { walker[0], walker[1], walker[2], walker[3] } };
	return to_num.n;
    }
    default:
	break;
    }
    return -1;
}

static int csonpath_compile(struct csonpath cjp[static 1])
{
	char *walker = cjp->path;
	char *orig = walker;
	char *next;
	char to_check;
	int inst_idx = 0;
	char *tmp; /* tmp is only here for debug */
	int inst;

	if (cjp->compiled)
		return 0;

	tmp = strdup(cjp->path);
  root_again:
	CSONPATH_SKIP('$', walker);
	csonpath_push_inst(cjp, CSONPATH_INST_ROOT, &inst_idx);
	cjp->inst_lst[inst_idx - 1].next = 1;
	to_check = *walker;

  again:
	switch (to_check) {
	case '[':
	{
	    int end;

	    inst = 0;
	  do_array:
	    cjp->inst_lst[inst_idx - 1].next += 1;
	    ++walker;
	    if (*walker == '*') {
		if (inst == CSONPATH_INST_FIND_ALL) {
		    CSONPATH_COMPILE_ERR(tmp, walker - orig, "'%c' is invalide here\n",
					 *walker);
			goto error;
		}
		csonpath_push_inst(cjp, CSONPATH_INST_GET_ALL, &inst_idx);
		if (walker[1] != ']') {
		    CSONPATH_COMPILE_ERR(tmp, walker - orig, "%s", "unclose bracket\n");
		}
		cjp->inst_lst[inst_idx - 1].next = 2;
		walker += 2;
		to_check = *walker;
		goto again;
	    } else if (*walker == '?') {
		int have_blank;
		int have_parentesis = 0;
		int nb_getter_inst = 0;
		struct csonpath_instruction filter_getter[CSONPATH_TMP_BUF_SIZE];
		struct csonpath_instruction *last_inst;
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
		cjp->inst_lst[inst_idx - 1].next += 1;
		++walker;

		/* skipp blank */
		for (; isblank(*walker); ++walker)
		    cjp->inst_lst[inst_idx - 1].next += 1;

		last_inst = &cjp->inst_lst[inst_idx - 1];
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
		    cjp->inst_lst[inst_idx - 1].next += 3;
		    for (; isblank(*walker); ++walker)
			cjp->inst_lst[inst_idx - 1].next += 1;
		} else if (*walker == '[') {
		    ++walker;
		    last_inst->next++;
		    getter_end = *walker;
		}
	      filter_again:
		if (getter_end) {
		    if (getter_end != '\'' && getter_end != '"') {
			CSONPATH_COMPILE_ERR(tmp, walker - orig,
					     "string require here, got '%c'", getter_end);
			goto error;
		    }
		    last_inst->next += 1;
		    ++walker;
		    for (next = walker; *next != getter_end; ++next);
		} else {
		    for (next = walker; csonpath_is_dot_operand(*next); ++next);
		}
		if (!*next) {
		    CSONPATH_COMPILE_ERR(tmp, next - orig,
					 "filter miss condition");
		    goto error;
		}
		to_check = *next;
		*next = 0;
		if (getter_end) {
		    CSONPATH_SKIP_2(getter_end, to_check != getter_end, next);
		    CSONPATH_SKIP(']', next);
		    to_check = *next;
		    getter_end = 0;
		}
		filter_getter[nb_getter_inst++] = (struct csonpath_instruction){.inst=inst,
		    .next=next - walker};
		last_inst = &filter_getter[nb_getter_inst - 1];
		if (to_check == '.') {
		    filter_getter[nb_getter_inst - 1].next += 1;
		    walker = next + 1;
		    goto filter_again;
		} else if (to_check == '[') {
		    filter_getter[nb_getter_inst - 1].next += 1;
		    walker = next + 1;
		    getter_end = *walker;
		    goto filter_again;
		}


		if (have_parentesis && to_check == ')') {
		    filter_getter[nb_getter_inst-1].next += 1;
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
		    csonpath_push_inst(cjp, CSONPATH_INST_FILTER_KEY_EQ, &inst_idx);
		    if (next[0] == '=')
			++next;
		    else if (next[0] == '~') {
			cjp->inst_lst[inst_idx - 1].inst = CSONPATH_INST_FILTER_KEY_REG_EQ;
			regex_idx = cjp->regex_cnt++;
			++next;
		    }
		} else if (to_check == '!' && next[0] == '=') {
		    csonpath_push_inst(cjp, CSONPATH_INST_FILTER_KEY_NOT_EQ, &inst_idx);
		    ++next;
		} else if (to_check == '>') {
		    csonpath_push_inst(cjp, CSONPATH_INST_FILTER_KEY_SUPERIOR, &inst_idx);
		} else if (to_check == '<') {
		    csonpath_push_inst(cjp, CSONPATH_INST_FILTER_KEY_INFERIOR, &inst_idx);
		} else {
		    CSONPATH_COMPILE_ERR(tmp, next - orig,
					 "'%c': unsuported operation", to_check);
		    goto error;
		}
		operand_instruction = cjp->inst_lst[inst_idx - 1].inst;
		for (;isblank(*next); ++next);
		cjp->inst_lst[inst_idx - 1].next = 0;
		cjp->inst_lst[inst_idx - 1].filter_next = inst_idx + nb_getter_inst;
		for (int i = 0; i < nb_getter_inst; ++i) {
		    csonpath_push_inst_cpy(cjp, filter_getter[i], &inst_idx);
		}
		cjp->inst_lst[inst_idx - 1].next += next - walker;
		walker = next;
		if (*walker == '"' || *walker == '\'' || *walker == '/') {
		    if (operand_instruction == CSONPATH_INST_FILTER_KEY_SUPERIOR ||
			operand_instruction == CSONPATH_INST_FILTER_KEY_INFERIOR) {
			CSONPATH_COMPILE_ERR(tmp, walker - orig, "string unsuported here");
			goto error;
		    }

		    char end = *walker;
		    ++walker;
		    cjp->inst_lst[inst_idx - 1].next++;
		    csonpath_push_inst(cjp, CSONPATH_INST_FILTER_OPERAND_STR, &inst_idx);
		    for (next = walker; *next && *next != end; ++next);
		    if (!*next) {
			CSONPATH_COMPILE_ERR(tmp, walker - orig,
					     "broken filter");
			goto error;
		    }
		    *next = 0;
#ifndef CSONPATH_NO_REGEX
		    if (regex_idx >= 0) {
			if (!regex_idx)
			    cjp->regexs = malloc(sizeof *cjp->regexs * 255);
			int e = regcomp(&cjp->regexs[regex_idx], walker, 0);
			cjp->inst_lst[inst_idx - 1].regex_idx = regex_idx;
			if (e) {
			    CSONPATH_COMPILE_ERR(tmp, next - orig, "regex has error\n");
			    goto error;
			}
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
		    csonpath_push_inst(cjp, csonpath_fill_walker_with_int(
					   walker, n, CSONPATH_INST_FILTER_OPERAND_BYTE), &inst_idx);
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
		    cjp->inst_lst[inst_idx - 1].next = next - walker + 1;
		    walker = next + 1;
		    to_check = *walker;
		    csonpath_push_inst(cjp, CSONPATH_INST_FILTER_AND, &inst_idx);
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
		cjp->inst_lst[inst_idx - 1].next = next - walker + 1;
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
		    csonpath_push_inst(cjp, CSONPATH_INST_RANGE, &inst_idx);
		    num = atoi(walker);
		    csonpath_push_inst(
			cjp, csonpath_fill_walker_with_int(
			    walker, num, CSONPATH_INST_GET_ARRAY_SMALL),
			&inst_idx);
		    cjp->inst_lst[inst_idx - 1].next = next - walker + 1;
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
		*next = 0;
		num = atoi(walker);
		csonpath_push_inst(cjp, csonpath_fill_walker_with_int(
				       walker, num, CSONPATH_INST_GET_ARRAY_SMALL),
				   &inst_idx);
		cjp->inst_lst[inst_idx - 1].next = next - walker + 1;
		walker = next + 1;
		to_check = *walker;
		goto again;
	    } else {
		end = *walker;
		cjp->inst_lst[inst_idx - 1].next += 1;
		if (inst != CSONPATH_INST_FIND_ALL)
		    inst = CSONPATH_INST_GET_OBJ;

		++walker;
		next = walker;
		while (*next++ != end) {
		    /* \" should be ignored */
		    while (*next == '\\')
			++next;
		}
		--next;
		*next = 0;
		++next;
		if (*next != ']')
		    CSONPATH_COMPILE_ERR(tmp, walker - orig,
					 "']' require instead of '%c'\n", *next);

		csonpath_push_inst(cjp, inst, &inst_idx);
		cjp->inst_lst[inst_idx - 1].next = next - walker + 1;

		walker = next + 1;
		to_check = *walker;
		goto again;
	    }
	}
	case '.':
	{
	    inst = CSONPATH_INST_GET_OBJ;

	    cjp->inst_lst[inst_idx - 1].next += 1;
	    ++walker;
	    if (*walker == '.') {
		inst = CSONPATH_INST_FIND_ALL;
		cjp->inst_lst[inst_idx - 1].next += 1;
		++walker;
	    } else if (*walker == '*') {
		inst = CSONPATH_INST_GET_ALL;
		++walker;
		if (*walker != '.' && *walker != '[' && *walker != '\0') {
		    CSONPATH_COMPILE_ERR(tmp, walker - orig, "unsuported characters '%c' after '*'", *walker);
		    goto error;
		}
		csonpath_push_inst(cjp, inst, &inst_idx);
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
	    *next = 0;

	    csonpath_push_inst(cjp, inst, &inst_idx);
	    cjp->inst_lst[inst_idx - 1].next = next - walker;
	    walker = next;
	    goto again;
	}
	case '|':
	    /* cjp->inst_lst[inst_idx - 1].next += 1; */
	    csonpath_push_inst(cjp, CSONPATH_INST_OR, &inst_idx);
	    ++walker;
	    cjp->inst_lst[inst_idx - 1].next += 1;
	    goto root_again;
	}
	if (isblank(*walker)) {
	    cjp->inst_lst[inst_idx - 1].next += 1;
	    ++walker;
	    goto again;
	}
	else if (*walker == 0) {
	    csonpath_push_inst(cjp, CSONPATH_INST_END, &inst_idx);
	    cjp->compiled = 1;
	    free(tmp);
	    return 0;
	} else {
	    CSONPATH_COMPILE_ERR(tmp, walker - orig, "unexpected char '%c'", to_check);
	}
  error:
	cjp->inst_lst[0] = (struct csonpath_instruction){.inst=CSONPATH_INST_BROKEN};
	free(tmp);
	return -1;
}

static _Bool csonpath_do_match(int operand_instruction, CSONPATH_JSON el2, char *owalker)
{
    switch (operand_instruction) {
    case CSONPATH_INST_FILTER_OPERAND_STR:
	return CSONPATH_EQUAL_STR(el2, owalker);
    case CSONPATH_INST_FILTER_OPERAND_BYTE:
	return CSONPATH_EQUAL_NUM(el2, *owalker);
    case CSONPATH_INST_FILTER_OPERAND_INT:
    {
	union {int n; char c[4];} to_num =
	    { .c= { owalker[0], owalker[1], owalker[2], owalker[3] } };

	return CSONPATH_EQUAL_NUM(el2, to_num.n);
    }
    }
    return 0;
}

static CSONPATH_JSON cosnpath_crawl_filter_el(struct csonpath cjp[static 1],
					      int *idx, char **owalker,
					      CSONPATH_JSON el2,
					      int filter_next)
{
    
    for (; *idx < filter_next; ++(*idx)) {
	switch (cjp->inst_lst[*idx].inst) {
	case CSONPATH_INST_GET_OBJ:
	    el2 = CSONPATH_GET(el2, *owalker);
	    break;
	default:
	    CSONPATH_FORMAT_EXCEPTION("unsuported %s inst",
				      csonpath_instuction_str[cjp->inst_lst[*idx].inst]);
	    return NULL;
	}
	(*owalker) += cjp->inst_lst[*idx].next;
    }
    return el2;
}


static _Bool csonpath_make_match(struct csonpath cjp[static 1],
				 struct csonpath_instruction inst[static 1],
				 CSONPATH_JSON el2, char *owalker, int operation)
{
    int operand_instruction = inst->inst;
    
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
#ifdef CSONPATH_NO_REGEX
	CSONPATH_GETTER_ERR("regex deactivate\n");
	return  CSONPATH_NONE_FOUND_RET;
#else
	if (CSONPATH_IS_STR(el2)) {
	    int regex_idx = inst->regex_idx;
	    regex_t *compiled = &cjp->regexs[regex_idx];
	    int match_len = regexec(compiled, CSONPATH_GET_STR(el2),
				    0, NULL, 0);
	    match = match_len == 0;
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
	walker += cjp->inst_lst[idx].next;				\
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

#define CSONPATH_DO_FIND_ALL return tret

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
    if (!nb_res) {				\
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

#define CSONPATH_DO_POST_FIND_ARRAY		\
	child_info.type = CSONPATH_INTEGER;	\
	child_info.idx = this_idx;

#define CSONPATH_DO_POST_FIND_OBJ		\
	child_info.type = CSONPATH_STR;		\
	child_info.key = walker;

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
    int to_check = cjp->inst_lst[idx + 1].inst;				\
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
	int check_at = idx + 1;						\
	int to_check;							\
	do {								\
	    to_check = cjp->inst_lst[check_at].inst;			\
	    ++check_at;							\
	} while (to_check == CSONPATH_INST_GET_ALL || to_check == CSONPATH_INST_FIND_ALL); \
	if (to_check == CSONPATH_INST_END || to_check == CSONPATH_INST_OR) { \
	    CSONPATH_APPEND_AT(ctx, this_idx, to_update);		\
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
  int to_check = cjp->inst_lst[idx + 1].inst;				\
  if (to_check == CSONPATH_INST_END || to_check == CSONPATH_INST_OR)  { \
    CSONPATH_GETTER_ERR("can't upate root ($)\n");			\
    return CSONPATH_NONE_FOUND_RET;					\
  }

#define CSONPATH_PRE_GET(this_idx)					\
  int to_check = cjp->inst_lst[idx].inst;				\
  csonpath_child_info_set(child_info, ctx, (intptr_t)this_idx)


#define CSONPATH_DO_GET_NOTFOUND(this_idx)		\
    CSONPATH_DO_GET_NOTFOUND_UPDATER(this_idx)


#include "csonpath_do.h"
