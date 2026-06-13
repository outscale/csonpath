#if !defined(CSONPATH_JSON) || !defined(CSONPATH_NULL) || !defined(CSONPATH_GET) || \
    !defined(CSONPATH_IS_STR) ||					\
  !defined(CSONPATH_AT) || !defined(CSONPATH_IS_OBJ) || !defined(CSONPATH_IS_ARRAY) || \
  !defined(CSONPATH_CALLBACK) || !defined(CSONPATH_CALLBACK_DATA) || \
  !defined(CSONPATH_EQUAL_STR) || !defined(CSONPATH_CALL_CALLBACK) || \
  !defined(CSONPATH_FOREACH_EXT) || !defined(CSONPATH_APPEND_AT) || \
  !defined(CSONPATH_REMOVE_CHILD) || !defined(CSONPATH_NEED_FOREACH_REDO) || \
  !defined(CSONPATH_ARRAY_APPEND_INCREF) || !defined(CSONPATH_REMOVE)
# error "some definitions are missing"
#endif

#ifndef CSONPATH_NO_REGEX
#  if defined CSONPATH_TINY_REGEX
#    include <tiny-regex-c/re.h>
typedef re_t csonpath_reg_t;
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

enum {
    CSONPATH_AUTO_ROOT = 1, /* enable jq-like path, like .root.hello, so we can skipp '$' */
    CSONPATH_NO_DETROY = (1 << 1)
};

enum csonpath_instuction_raw {
	CSONPATH_INST_ROOT,
	CSONPATH_INST_GET_OBJ,
	CSONPATH_INST_GET_SUBPATH,
	CSONPATH_INST_GET_ARRAY_SMALL,
	CSONPATH_INST_GET_ARRAY_BIG,
	CSONPATH_INST_FILTER_KEY_EXIST,
	CSONPATH_INST_FILTER_KEY_TRUE,
	CSONPATH_INST_FILTER_KEY_FALSE,
	CSONPATH_INST_FILTER_KEY_SUPERIOR_EQ,
	CSONPATH_INST_FILTER_KEY_INFERIOR_EQ,
	CSONPATH_INST_FILTER_KEY_SUPERIOR,
	CSONPATH_INST_FILTER_KEY_INFERIOR,
	CSONPATH_INST_FILTER_KEY_EQ,
	CSONPATH_INST_FILTER_KEY_NOT_EQ,
	CSONPATH_INST_FILTER_KEY_REG_EQ,
	CSONPATH_INST_FILTER_AND,
	CSONPATH_INST_GET_ALL,
	CSONPATH_INST_GET_UNION,
	CSONPATH_INST_UNION_JMP,
	CSONPATH_INST_UNION_END,
	CSONPATH_INST_FIND_ALL,
	CSONPATH_INST_RANGE,
	CSONPATH_INST_OR,
	CSONPATH_INST_END,
	CSONPATH_INST_BROKEN
};

static int csonpath_instuction_len[] = {
    1, /* 0 CSONPATH_INST_ROOT */
    -1, /* 1 CSONPATH_INST_GET_OBJ */
    1, /* 2 CSONPATH_INST_GET_SUBPATH */
    2, /* 3 CSONPATH_INST_GET_ARRAY_SMALL */
    5, /* 4 CSONPATH_INST_GET_ARRAY_BIG */
    1, /* 4.5 CSONPATH_INST_FILTER_KEY_EXIST */
    1, /* 4.6 CSONPATH_INST_FILTER_KEY_TRUE */
    1, /* 4.7 CSONPATH_INST_FILTER_KEY_FALSE */
    2, /* 5 CSONPATH_INST_FILTER_KEY_SUPERIOR_EQ */
    2, /* 6 CSONPATH_INST_FILTER_KEY_INFERIOR_EQ */
    2, /* 7 CSONPATH_INST_FILTER_KEY_SUPERIOR */
    2, /* 8 CSONPATH_INST_FILTER_KEY_INFERIOR */
    2, /* 9 CSONPATH_INST_FILTER_KEY_EQ */
    2, /* A CSONPATH_INST_FILTER_KEY_NOT_EQ */
    2, /* B CSONPATH_INST_FILTER_KEY_REG_EQ */
    1, /* D CSONPATH_INST_FILTER_AND */
    1, /* E CSONPATH_INST_GET_ALL */
    1, /* E.5 CSONPATH_INST_GET_UNION */
    1, /* E.5000 CSONPATH_INST_UNION_JMP */
    1, /* E.6 CSONPATH_INST_UNION_END */
    -1, /* F CSONPATH_INST_FIND_ALL */
    1, /* 10 CSONPATH_INST_RANGE */
    1, /* 11 CSONPATH_INST_OR */
    1, /* 12 CSONPATH_INST_END */
    1, /* 13 CSONPATH_INST_BROKEN */
};

/* this should be in an include, but doing so, would break the
 * "no more than 2 file", for single header lib */
CSONPATH_UNUSED static const char *csonpath_instuction_str[] = {
	"ROOT",
	"GET_OBJ",
	"GET_SUBPATH",
	"GET_ARRAY_SMALL",
	"GET_ARRAY_BIG",
	"FILTER_KEY_EXIST",
	"FILTER_KEY_TRUE",
	"FILTER_KEY_FALSE",
	"FILTER_KEY_SUPERIOR_EQ",
	"FILTER_KEY_INFERIOR_EQ",
	"FILTER_KEY_SUPERIOR",
	"FILTER_KEY_INFERIOR",
	"FILTER_KEY_EQ",
	"FILTER_KEY_NOT_EQ",
	"FILTER_KEY_REG_EQ",
	"FILTER_AND",
	"GET_ALL",
	"UNION",
	"UNION_JMP",
	"UNION_END",
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
#if !defined(CSONPATH_NO_REGEX)
    int regex_cnt;
    csonpath_reg_t *regexs;
#ifdef CSONPATH_PCRE2
    pcre2_match_data **match_datas;
#endif
#endif
    char data[];
};

#  if defined CSONPATH_TINY_REGEX
static inline int csonpath_reg_compile(struct csonpath *cjp, int idx, const char *pat, char **errbuff) {
    (void)errbuff;
    cjp->regexs[idx] = re_compile(pat);
    return cjp->regexs[idx] ? 0 : -1;
}
static inline _Bool csonpath_reg_exec(csonpath_reg_t reg, const char *str) {
    int match_len = 0;
    re_matchp(reg, str, &match_len);
    return !!match_len;
}
static inline void csonpath_reg_free(csonpath_reg_t reg) {
	free(reg);
}
#  elif !defined CSONPATH_PCRE2
static inline int csonpath_reg_compile(struct csonpath *cjp, int idx, const char *pat, char **errbuff)
{
    (void)errbuff;
    return regcomp(&cjp->regexs[idx], pat, 0);
}

static inline _Bool csonpath_reg_exec(csonpath_reg_t reg, const char *str)
{
    return regexec(&reg, str, 0, NULL, 0) == 0;
}
static inline void csonpath_reg_free(csonpath_reg_t reg) { regfree(&reg); }
#  else
static inline int csonpath_reg_compile(struct csonpath *cjp, int regex_idx,
				       const char *pat, char **errbuff)
{
  int errorcode;
  PCRE2_SIZE erroroffset;

  cjp->regexs[regex_idx] = pcre2_compile((unsigned char *)pat,
					 PCRE2_ZERO_TERMINATED, 0,
					 &errorcode, &erroroffset, NULL);
  if (!cjp->regexs[regex_idx]) {
    static PCRE2_UCHAR buffer[256];
    pcre2_get_error_message(errorcode, buffer, sizeof(buffer));
    *errbuff = (char *)buffer;
    return -1;
  }
  cjp->match_datas[regex_idx] = pcre2_match_data_create_from_pattern(cjp->regexs[regex_idx],
								     NULL);
  pcre2_jit_compile(cjp->regexs[regex_idx], PCRE2_JIT_COMPLETE);
  return 0;
}
#  endif
#endif

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
	if (!cjp->compile_error)					\
	    goto error;							\
	lidx = snprintf(cjp->compile_error, CSONPATH_ERROR_MAX_SIZE, "column %d:\n", oidx); \
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
#if !defined CSONPATH_NO_REGEX
    if (cjp->regex_cnt) {
	for (int i = 0; i < cjp->regex_cnt; ++i) {
#  ifdef CSONPATH_PCRE2
	    pcre2_code_free(cjp->regexs[i]);
	    pcre2_match_data_free(cjp->match_datas[i]);
#  else
	    csonpath_reg_free(cjp->regexs[i]);
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

#define CSONPATH_BYTE_PER_INST 2

static int csonpath_compile_(struct csonpath *cjp, const char path[static 1], int);

static int csonpath_compile_do(struct csonpath *cjp, const char orig[static 1],
			       const char walker[static 1], char tmp[static 1],
			       int *inst_idx, int flag, int end_path, const char **end_sentinel);


static inline struct csonpath *csonpath_new_ex(const char path[static 1], int flag) {
    /*
     * max inst is use so we know, we will never overflow.
     */
    int max_inst = strlen(path) / 2 + 1;
    int extra = 0;
    for (const char *tmp = path; *tmp; ++tmp)
      if (*tmp == ',')
	++extra;
    struct csonpath *ret = malloc(sizeof *ret + CSONPATH_BYTE_PER_INST * max_inst + strlen(path) + 1 + extra);
    if (!ret) {
	return NULL;
    }
    memset(ret, 0, sizeof *ret);

    if (csonpath_compile_(ret, path, flag) < 0) {
	if ((flag & CSONPATH_NO_DETROY))
	    return ret;
	fprintf(stderr, "compile error: %s\n",
		ret->compile_error ? ret->compile_error : "(unknown error)");
	csonpath_destroy(ret);
	return NULL;
    }
    return ret;
}

static inline struct csonpath *csonpath_new(const char path[static 1]) {
    return csonpath_new_ex(path, 0);
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
    int cnt = 0;
    int cnt_sub = 0;
    for (;cnt_sub > 0 || *walker != CSONPATH_INST_END; walker = csonpath_walker_next_inst(walker)) {
	for (int i = 0; i < cnt_sub; ++i) {
	    printf("  ");
	}
	printf("%d: %s\n", (int)(intptr_t)(walker - origin), csonpath_instuction_str[(int)*walker]);
	if (*walker == CSONPATH_INST_END)
	  --cnt_sub;
	else if (*walker == CSONPATH_INST_GET_SUBPATH)
	  ++cnt_sub;
	if (cnt++ == 1000) {
	    printf("%d: TOO MUCH INSTRUCTION, BAIL OUT\n", (int)(intptr_t)(walker - origin));
	    return;
	}
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
	cjp->data[(*inst_idx)++] = (char)num;
	cjp->data[(*inst_idx)++] = (char)(num >> 8);
	cjp->data[(*inst_idx)++] = (char)(num >> 16);
	cjp->data[(*inst_idx)++] = (char)(num >> 24);
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

static int csonpath_compile_(struct csonpath *cjp, const char path[static 1], int flag)
{
	const char *walker = path;
	const char *orig = walker;
	int inst_idx = 0;
	char *tmp; /* tmp is use only to print error */
	int ret;

	tmp = strdup(path);
	if (!tmp) {
		return -1;
	}
	ret = csonpath_compile_do(cjp, orig, walker, tmp, &inst_idx, flag, '\0', NULL);
	free(tmp);
	return ret;
}

static void push_filter_getter(struct csonpath *cjp, int *inst_idx, int nb_getter_inst,
			       char filter_getter[static nb_getter_inst])
{
  char *filter_end = &cjp->data[(*inst_idx)++];
  for (int i = 0; i < nb_getter_inst; ++i) {
    csonpath_push_char(cjp, filter_getter[i], inst_idx);
  }
  *filter_end = (nb_getter_inst - 1);

}

static int csonpath_compile_do(struct csonpath *cjp, const char orig[static 1],
			       const char walker[static 1], char tmp[static 1],
			       int *inst_idx, int flag, int end_path,
			       const char **end_sentinel)
{
	const char *next;
	char to_check;
	int inst;
	int in_union = 0;

root_again:
	if (!(flag & CSONPATH_AUTO_ROOT)) {
	    CSONPATH_SKIP('$', walker);
	}
	csonpath_push_char(cjp, CSONPATH_INST_ROOT, inst_idx);
	to_check = *walker;

  again:
	if (in_union) {
	    csonpath_push_char(cjp, CSONPATH_INST_UNION_END, inst_idx);
	    in_union = 0;
	}

	switch (to_check) {
	case '[':
	{
	    int end;

	    inst = 0;


	  do_array:
	    if (!in_union) {
		int cnt_brackets = 0;
		for (const char *tmp_walk = walker + 1;
		     *tmp_walk && *tmp_walk != ']' && !cnt_brackets;
		     ++tmp_walk) {
		    if (!cnt_brackets && *tmp_walk == ',') {
			csonpath_push_char(cjp, CSONPATH_INST_GET_UNION, inst_idx);
			in_union = 1;
			break;
		    }
		    if (*tmp_walk == '[')
			++cnt_brackets;
		    if (cnt_brackets && *tmp_walk == ']')
			--cnt_brackets;
		    if (*tmp_walk == '"')
			for (++tmp_walk; *tmp_walk != '"' && *tmp_walk; ++tmp_walk);
		    if (*tmp_walk == '\'')
			for (++tmp_walk; *tmp_walk !=  '\'' && *tmp_walk; ++tmp_walk);
		}
	    }
	    ++walker;
	    for (; isblank(*walker); ++walker);

	    if (*walker == '*') {
		if (inst == CSONPATH_INST_FIND_ALL) {
		    CSONPATH_COMPILE_ERR(tmp, walker - orig, "'%c' is invalide here\n",
					 *walker);
		    goto error;
		}
		csonpath_push_char(cjp, CSONPATH_INST_GET_ALL, inst_idx);
		if (in_union && walker[1] == ',') {
		    csonpath_push_char(cjp, CSONPATH_INST_UNION_JMP, inst_idx);
		    walker += 1;
		    goto do_array;
		}
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
		} else if (*walker == '@') {
		    ++walker;
		    if (*walker == '[') {
			++walker;
			getter_end = *walker;
		    } else {
			CSONPATH_SKIP('.', walker);
		    }
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

		/* = and == are the same here */
		if (to_check == '=') {
		    if (next[0] == '=') {
			++next;
			/* some implementation support === */
			if (next[0] == '=')
			  ++next;
			csonpath_push_char(cjp, CSONPATH_INST_FILTER_KEY_EQ, inst_idx);
		    }
#ifndef CSONPATH_NO_REGEX
		    else if (next[0] == '~') {
			csonpath_push_char(cjp, CSONPATH_INST_FILTER_KEY_REG_EQ,  inst_idx);
			regex_idx = cjp->regex_cnt++;
			++next;
		    }
#endif
		    else { /* a = b, so same as == */
			csonpath_push_char(cjp, CSONPATH_INST_FILTER_KEY_EQ, inst_idx);
		    }
		} else if (to_check == '!' && next[0] == '=') {
		    csonpath_push_char(cjp, CSONPATH_INST_FILTER_KEY_NOT_EQ, inst_idx);
		    ++next;
		} else if (to_check == '>') {
		    if (next[0] == '=') {
			csonpath_push_char(cjp, CSONPATH_INST_FILTER_KEY_SUPERIOR_EQ, inst_idx);
			++next;
		    } else {
		      csonpath_push_char(cjp, CSONPATH_INST_FILTER_KEY_SUPERIOR, inst_idx);
		    }
		} else if (to_check == '<') {
		    if (next[0] == '=') {
			csonpath_push_char(cjp, CSONPATH_INST_FILTER_KEY_INFERIOR_EQ, inst_idx);
			++next;
		    } else {
		      csonpath_push_char(cjp, CSONPATH_INST_FILTER_KEY_INFERIOR, inst_idx);
		    }
		} else if (to_check == ']' || (in_union && to_check == ',')) {
		    int union_jmp = in_union && to_check == ',';
		    if (have_parentesis > 0)
			CSONPATH_COMPILE_ERR(tmp, next - orig, "too many open parentesis");
		    csonpath_push_char(cjp, CSONPATH_INST_FILTER_KEY_EXIST, inst_idx);
		    for (;isblank(*next); ++next);
		    push_filter_getter(cjp, inst_idx, nb_getter_inst, filter_getter);
		    if (union_jmp) {
			walker = next - 1; /* will be re-skipp hopefully :) */
			csonpath_push_char(cjp, CSONPATH_INST_UNION_JMP, inst_idx);
			goto do_array;
		    }
		    walker = next;
		    to_check = *walker;
		    goto again;
		} else {
		    CSONPATH_COMPILE_ERR(tmp, next - orig,
					 "'%c': unsuported operation", to_check);
		    goto error;
		}
		if (!*next) {
		    CSONPATH_COMPILE_ERR(tmp, next - orig,
					 "filter miss operand");

		    goto error;
		}
		operand_instruction = cjp->data[(*inst_idx) - 1];
		for (;isblank(*next); ++next);
		push_filter_getter(cjp, inst_idx, nb_getter_inst, filter_getter);
		walker = next;
		if (*walker == '"' || *walker == '\'' || *walker == '/') {
		    char end = *walker;
		    ++walker;
		    if (regex_idx < 0) {
			csonpath_push_char(cjp, CSONPATH_INST_GET_OBJ, inst_idx);
			for (next = walker; *next && *next != end; ++next)
				csonpath_push_char(cjp, *next, inst_idx);
			csonpath_push_char(cjp, 0, inst_idx);
			if (!*next) {
			    CSONPATH_COMPILE_ERR(tmp, walker - orig,
						 "broken filter");
			    goto error;
			}
		    }
#if !defined(CSONPATH_NO_REGEX)
		    else {
			csonpath_push_char(cjp, CSONPATH_INST_GET_ARRAY_SMALL, inst_idx);
			csonpath_push_char(cjp, regex_idx, inst_idx);
			if (!regex_idx) {
			    cjp->regexs = malloc(sizeof *cjp->regexs * 255);
#  ifdef CSONPATH_PCRE2
			    cjp->match_datas = malloc(sizeof *cjp->match_datas * 255);
#  endif
			}
			for (next = walker; *next && *next != end; ++next);
			char *reg_tmp = malloc(next - walker + 1);
			char *crawler = reg_tmp;
			char *errbuff = NULL;
			for (next = walker; *next && *next != end; ++next, ++crawler)
			    *crawler = *next;
			*crawler = 0;

			int e = csonpath_reg_compile(cjp, regex_idx, reg_tmp, &errbuff);
			if (e) {
			    CSONPATH_COMPILE_ERR(tmp, next - orig, "regex has error: %s\n",
						 errbuff ? errbuff : "(unknow)");
			    goto error;
			}
			free(reg_tmp);
		    }
#endif
		    ++next;
		    to_check = *next;
		} else if (*walker == '$') { /* handle subpath here */
		    const char *end_sentinel;

		    if (operand_instruction == CSONPATH_INST_FILTER_KEY_REG_EQ) {
			CSONPATH_COMPILE_ERR(tmp, walker - orig, "subpath unsuported for regex");
			goto error;
		    }
		    csonpath_push_char(cjp, CSONPATH_INST_GET_SUBPATH, inst_idx);
		    int ret = csonpath_compile_do(cjp, orig, walker, tmp,
						  inst_idx, flag, in_union ? 'u' : ']',
						  &end_sentinel);
		    if (ret < 0)
			return -1;
		    walker = end_sentinel;
		    next = walker;
		    to_check = *next;
		} else if (!strncmp(walker, "null", 4) && !isalnum(walker[4])) {
		    next = walker + 4;
		    to_check = *next;
		    csonpath_push_char(cjp, CSONPATH_INST_FILTER_KEY_EXIST, inst_idx);
		} else if (!strncmp(walker, "true", 4) && !isalnum(walker[4])) {
		    next = walker + 4;
		    to_check = *next;
		    csonpath_push_char(cjp, CSONPATH_INST_FILTER_KEY_TRUE, inst_idx);
		} else if (!strncmp(walker, "false", 5) && !isalnum(walker[5])) {
		    next = walker + 5;
		    to_check = *next;
		    csonpath_push_char(cjp, CSONPATH_INST_FILTER_KEY_FALSE, inst_idx);
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
		    csonpath_fill_walker_with_int(cjp, inst_idx, n);
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
		    csonpath_push_char(cjp, CSONPATH_INST_FILTER_AND, inst_idx);
		    nb_getter_inst = 0;
		    goto filter_again_root;
		}

		while (have_parentesis) {
		    CSONPATH_SKIP(')', next);
		    to_check = *next;
		    --have_parentesis;
		}

		if (in_union && to_check == ',') {
		    walker = next;
		    csonpath_push_char(cjp, CSONPATH_INST_UNION_JMP, inst_idx);
		    goto do_array;
		}
		if (to_check != ']') {
		    CSONPATH_REQUIRE_ERR(']', next);
		}
		walker = next + 1;
		to_check = *walker;
		goto again;
		/* Filter out */
	    } else if (*walker == '$') { /* handle subpath here */
		const char *end_sentinel;
		csonpath_push_char(cjp, CSONPATH_INST_GET_SUBPATH, inst_idx);
		int ret = csonpath_compile_do(cjp, orig, walker, tmp,
					      inst_idx, flag,in_union ? 'u' : ']', &end_sentinel);
		if (ret < 0)
		    return -1;
		walker = end_sentinel;
		if (in_union && *end_sentinel == ',') {
		    csonpath_push_char(cjp, CSONPATH_INST_UNION_JMP, inst_idx);
		    goto do_array;
		}
		++walker;
		to_check = *walker;
		goto again;
	    } else if (*walker != '"' && *walker != '\'') {
		int num;

		if (inst == CSONPATH_INST_FIND_ALL) {
		    CSONPATH_COMPILE_ERR(tmp, walker - orig, ".. require string\n");
		    goto error;
		}

		next = walker;
		do {
		  if (*next == ':') {
		    csonpath_push_char(cjp, CSONPATH_INST_RANGE, inst_idx);
		    num = atoi(walker);
		    csonpath_fill_walker_with_int(cjp, inst_idx, num);
		    walker = next + 1;
		    ++next;
		    continue;
		  }
		  for (; isblank(*next); ++next);
		  if (*next < '0' || *next > '9') {
		    CSONPATH_COMPILE_ERR(tmp, walker - orig,
					 "unexpected '%c', sting, filter or number require\n", *next);
		  }
		  next++;
		  if (in_union && *next == ',')
		      break;
		} while (*next && *next != ']');
		if (!*next) {
		    CSONPATH_COMPILE_ERR(tmp, walker - orig,
					 "%s", "unclose bracket\n");
		}
		num = atoi(walker);
		csonpath_fill_walker_with_int(cjp, inst_idx, num);
		if (in_union && *next == ',') {
		    csonpath_push_char(cjp, CSONPATH_INST_UNION_JMP, inst_idx);
		    walker = next;
		    goto do_array;
		}
		walker = next + 1;
		to_check = *walker;
		goto again;
	    } else {
		end = *walker;
		if (inst != CSONPATH_INST_FIND_ALL)
		    inst = CSONPATH_INST_GET_OBJ;

		++walker;
		next = walker;
		csonpath_push_char(cjp, inst, inst_idx);
		while (*next != end) {
		    /* \" should be ignored */
		    csonpath_push_char(cjp, *next, inst_idx);
		    ++next;
		    while (*next == '\\')
			++next;
		}
		++next; // skipp end
		csonpath_push_char(cjp, 0, inst_idx);
		if (in_union && *next == ',') {
		    walker = next;
		    csonpath_push_char(cjp, CSONPATH_INST_UNION_JMP, inst_idx);
		    goto do_array;
		}
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
		csonpath_push_char(cjp, inst, inst_idx);
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

	    csonpath_push_char(cjp, inst, inst_idx);
	    for (; walker != next; ++walker) {
		    cjp->data[(*inst_idx)++] = *walker;
	    }
	    cjp->data[(*inst_idx)++] = 0;
	    goto again;
	}
	case '|':
	    csonpath_push_char(cjp, CSONPATH_INST_OR, inst_idx);
	    ++walker;
	    goto root_again;
	}
	if (isblank(*walker)) {
	    ++walker;
	    goto again;
	}
	else if ((end_path == 'u' && (*walker == ']' || *walker == ',')) || *walker == end_path) {
	    csonpath_push_char(cjp, CSONPATH_INST_END, inst_idx);
	    free(cjp->compile_error);
	    cjp->compile_error = NULL;
	    if (end_sentinel)
		*end_sentinel = walker;
	    return 0;
	} else {
	    CSONPATH_COMPILE_ERR(tmp, walker - orig, "unexpected char '%c'", to_check);
	}
  error:
	cjp->data[0] = CSONPATH_INST_BROKEN;
	return -1;
}

static int csonpath_compile(struct csonpath *cjp, const char path[static 1])
{
    return csonpath_compile_(cjp, path, 0);
}

static _Bool csonpath_do_match(int operand_instruction, CSONPATH_JSON el2, const char **owalker)
{
    switch (operand_instruction) {
    case CSONPATH_INST_FILTER_KEY_TRUE:
    {
	if (!CSONPATH_IS_BOOL(el2))
	    return 0;
	return CSONPATH_GET_BOOL(el2) == 1;
    }
    case CSONPATH_INST_FILTER_KEY_FALSE:
    {
	if (!CSONPATH_IS_BOOL(el2))
	    return 0;
	return CSONPATH_GET_BOOL(el2) == 0;
    }
    case CSONPATH_INST_FILTER_KEY_EXIST:
    {
	return CSONPATH_IS_NULL(el2);
    }
    case CSONPATH_INST_GET_OBJ:
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
	int num = ((unsigned char)(*owalker)[0]) |
	    ((unsigned char)(*owalker)[1] << 8) |
	    ((unsigned char)(*owalker)[2] << 16) |
	    ((unsigned char)(*owalker)[3] << 24);

	*owalker += 4;
	return CSONPATH_EQUAL_NUM(el2, num);
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
	    if (el2)
		el2 = CSONPATH_GET(el2, *owalker);
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
				 CSONPATH_JSON origin, CSONPATH_JSON el2,
				 const char **owalker, int operation);


static inline _Bool csonpath_is_endish_inst(int instruction)
{
    return instruction == CSONPATH_INST_END || instruction == CSONPATH_INST_OR;
}



static const char *csonpath_skipp_union_jmp(const char *walker)
{
    int union_cnt = 0;
    while (union_cnt || *walker != CSONPATH_INST_UNION_END) {
	walker = csonpath_walker_next_inst(walker);
	if (*walker == CSONPATH_INST_GET_UNION)
	    ++union_cnt;
    }
    return walker;
}

#define CSONPATH_GOTO_ON_RELOOP(where)			\
    nb_res += tret; if (need_reloop_in) goto where;

#define CSONPATH_PREPARE_RELOOP(label)		\
    int need_reloop_in;				\
label:						\
need_reloop_in = 0;


#define CSONPATH_NONE_FOUND_RET CSONPATH_NULL

#define CSONPATH_GETTER_ERR(args...) do {	\
	CSONPATH_FORMAT_EXCEPTION(args);	\
	return CSONPATH_NULL;			\
    } while (0)

/* Find First */

#define CSONPATH_DO_RET_TYPE CSONPATH_JSON
#define CSONPATH_DO_FUNC_NAME find_first
#define CSONPATH_DO_RETURN if (end_sentinel) *end_sentinel = walker; return tmp

#define CSONPATH_DO_FIND_ALL if (tret) {if (end_sentinel) *end_sentinel = walker; return tret;}

#define CSONPATH_DO_FILTER_FIND if (end_sentinel) *end_sentinel = owalker; return tret

#define CSONPATH_DO_FIND_ALL_OUT if (end_sentinel) *end_sentinel = walker;  return CSONPATH_NULL

#define CSONPATH_DO_EXTRA_DECLATION , const char **end_sentinel
#define CSONPATH_DO_EXTRA_ARGS_IN , NULL
#define CSONPATH_DO_EXTRA_ARGS_NEESTED , end_sentinel

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

#define CSONPATH_GETTER_ERR(args...) do {		\
	CSONPATH_FORMAT_EXCEPTION(args);		\
	return -1;					\
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

#define CSONPATH_PRE_GET_ROOT						\
    int to_check = walker[1];						\
    if (csonpath_is_endish_inst(to_check)) {				\
	if (CSONPATH_IS_OBJ(origin) && CSONPATH_IS_OBJ(to_update)) {	\
	    return csonpath_sync_root_obj(origin, to_update);		\
	} else if (CSONPATH_IS_ARRAY(origin) && CSONPATH_IS_ARRAY(to_update)) { \
	    return csonpath_sync_root_array(origin, to_update);		\
	} else {							\
	    CSONPATH_EXCEPTION("can't update root ($)\n");		\
	}								\
    }

#define CSONPATH_DO_DECLARATION			\
	int nb_res = 0;

#define CSONPATH_DO_RET_TYPE int
#define CSONPATH_DO_FUNC_NAME update_or_create

#define CSONPATH_DO_GET_NOTFOUND(osef) goto next_inst; /* useless but make gcc shup up on warning */

/*
 * assuming tmp == value can only be true,
 * while been called from FIND/FILTER or GET
 * otherwise CSONPATH_PRE_GET, is the part doing the buisness
 */
#define CSONPATH_DO_RETURN						\
    if (child_info->type != CSONPATH_NONE) {				\
	if (need_reloop) *need_reloop = 1;				\
	if (child_info->type == CSONPATH_INTEGER)			\
	    return CSONPATH_APPEND_AT(ctx, child_info->idx, to_update); \
	else								\
	    return CSONPATH_APPEND_AT(ctx, child_info->key, to_update); \
	return 1;							\
    }									\
    return 0;


#define CSONPATH_DO_EXTRA_ARGS_FIND_ALL , to_update, child_info, need_reloop
#define CSONPATH_DO_EXTRA_ARGS_NEESTED , to_update,			\
		csonpath_child_info_set(&(struct csonpath_child_info ){}, tmp, (intptr_t)key_idx), &need_reloop_in
#define CSONPATH_DO_EXTRA_ARGS , CSONPATH_JSON to_update
#define CSONPATH_DO_EXTRA_ARGS_IN , to_update, &(struct csonpath_child_info ){}, NULL
#define CSONPATH_DO_EXTRA_DECLATION CSONPATH_DO_EXTRA_ARGS, struct csonpath_child_info *child_info, int *need_reloop
#define CSONPATH_DO_FIND_ALL nb_res += tret;
#define CSONPATH_DO_FILTER_FIND CSONPATH_GOTO_ON_RELOOP(filter_again)

#define CSONPATH_NEW_GUESS_CNT()					\
    ({									\
	const char *tmp_wal = csonpath_walker_next_inst(walker);	\
	int is_array = 1;						\
	for (; *tmp_wal && !csonpath_is_endish_inst(*tmp_wal);		\
	     tmp_wal = csonpath_walker_next_inst(walker)) {		\
	    if (*tmp_wal == CSONPATH_INST_GET_UNION)			\
		tmp_wal = csonpath_skipp_union_jmp(tmp_wal);		\
	    if (*tmp_wal == CSONPATH_INST_GET_OBJ ||			\
		(*tmp_wal >= CSONPATH_INST_FILTER_KEY_EXIST && *tmp_wal <= CSONPATH_INST_FILTER_KEY_REG_EQ)) { \
		is_array = 0;						\
		break;							\
	    } else if (*tmp_wal == CSONPATH_INST_GET_ARRAY_SMALL || *tmp_wal == CSONPATH_INST_GET_ARRAY_BIG) { \
		break;							\
	    }								\
	}								\
	CSONPATH_JSON r;						\
	if (is_array)							\
	    r = CSONPATH_NEW_ARRAY();					\
	else								\
	    r = CSONPATH_NEW_OBJECT();					\
	r;								\
    })

#define CSONPATH_DO_FIND_ALL_PRE_LOOP int need_reloop_in = 0;	\
    CSONPATH_UPDATE_CHECK_EXIST(CSONPATH_NEW_GUESS_CNT);

#define CSONPATH_DO_FILTER_PRE_LOOP			\
    CSONPATH_UPDATE_CHECK_EXIST(CSONPATH_NEW_OBJECT);	\
    CSONPATH_PREPARE_RELOOP(filter_again)

#define CSONPATH_DO_FIND_ALL_OUT return nb_res;

static int csonpath_sync_root_array(CSONPATH_JSON parent, CSONPATH_JSON to_update)
{
    CSONPATH_JSON child;
    size_t idx;

    (void) idx;
    CSONPATH_ARRAY_CLEAR(parent);
    CSONPATH_FOREACH_ARRAY(to_update, child, idx) {
	if (CSONPATH_APPEND_AT(parent, idx, child) < 0)
	    return -1;
    }
    return 1;
}

static int csonpath_sync_root_obj(CSONPATH_JSON parent, CSONPATH_JSON to_update)
{
    CSONPATH_JSON child;
    const char *key;

    (void )key;
    CSONPATH_OBJ_CLEAR(parent);
    CSONPATH_FOREACH_OBJ(to_update, child, key) {
	if (CSONPATH_APPEND_AT(parent, key, child) < 0)
	    return -1;
    }
    return 1;
}


#define CSONPATH_UPDATE_CHECK_EXIST(mk_cnt)				\
    if (tmp == CSONPATH_NULL) {						\
	int append_ret = 0;						\
	tmp = mk_cnt();							\
	if (child_info->type == CSONPATH_INTEGER)			\
	    append_ret = CSONPATH_APPEND_AT(ctx, child_info->idx, tmp);	\
	else if (child_info->type == CSONPATH_STR)			\
	    append_ret = CSONPATH_APPEND_AT(ctx, child_info->key, tmp);	\
	else								\
	    CSONPATH_GETTER_ERR("CSONPATH_NONE is nope\n");		\
	CSONPATH_DECREF(tmp);						\
	if (append_ret < 0) return append_ret;				\
	ctx = tmp;							\
    }

#define CSONPATH_PRE_GET_OBJ(this_idx)					\
    if (tmp != CSONPATH_NULL && !CSONPATH_IS_OBJ(tmp)) {		\
	CSONPATH_GETTER_ERR("Unable to follow path(%s): Dict expected", this_idx); \
    }									\
    CSONPATH_UPDATE_CHECK_EXIST(CSONPATH_NEW_OBJECT);			\
    csonpath_child_info_set(child_info, tmp, (intptr_t)this_idx);


#define CSONPATH_PRE_GET(this_idx)					\
    if (tmp != CSONPATH_NULL && !CSONPATH_IS_ARRAY(tmp)) {		\
	CSONPATH_GETTER_ERR("Unable to follow path(%d): List expected", this_idx); \
    }									\
    CSONPATH_UPDATE_CHECK_EXIST(CSONPATH_NEW_ARRAY);			\
    csonpath_child_info_set(child_info, tmp, this_idx);



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
    if (csonpath_is_endish_inst(to_check))  { \
	CSONPATH_GETTER_ERR("can't update root ($)\n");			\
	return CSONPATH_NONE_FOUND_RET;					\
    }


#define CSONPATH_PRE_GET_OBJ(this_idx)					\
    if (tmp != CSONPATH_NULL && !CSONPATH_IS_OBJ(tmp)) {		\
	CSONPATH_GETTER_ERR("Unable to follow path(%s): Dict expected", this_idx); \
    }									\
    CSONPATH_UPDATE_CHECK_EXIST(CSONPATH_NEW_OBJECT);			\
    csonpath_child_info_set(child_info, tmp, (intptr_t)this_idx);

#define CSONPATH_PRE_GET(this_idx)					\
    if (tmp != CSONPATH_NULL && !CSONPATH_IS_ARRAY(tmp)) {		\
	CSONPATH_GETTER_ERR("Unable to follow path(%d): List expected", this_idx); \
    }									\
    CSONPATH_UPDATE_CHECK_EXIST(CSONPATH_NEW_ARRAY);			\
    csonpath_child_info_set(child_info, tmp, this_idx);

#define CSONPATH_DO_GET_NOTFOUND(osef) goto next_inst;

#include "csonpath_do.h"



static _Bool csonpath_make_match(const struct csonpath cjp[const static 1],
				 CSONPATH_JSON origin, CSONPATH_JSON el2,
				 const char **owalker, int operation)
{
    int operand_instruction = **owalker;

    if (operation == CSONPATH_INST_FILTER_KEY_EXIST) {
	return el2 != CSONPATH_NULL;
    }

    if (operand_instruction == CSONPATH_INST_GET_SUBPATH) {
	const char *end_sentinel;
	*owalker = csonpath_walker_next_inst(*owalker);
	CSONPATH_JSON jret = csonpath_find_first_internal(
	    cjp, origin, origin, CSONPATH_NULL, *owalker, &end_sentinel);
	*owalker = end_sentinel + 1;
	if (!jret)
	    return 0;
	switch (operation) {
	case CSONPATH_INST_FILTER_KEY_NOT_EQ:
	    if (CSONPATH_IS_NUM(el2) && CSONPATH_IS_NUM(jret)) {
		return CSONPATH_GET_NUM(el2) != CSONPATH_GET_NUM(jret);
	    }
	    if (CSONPATH_IS_STR(el2) && CSONPATH_IS_STR(jret)) {
		return (!!strcmp(CSONPATH_GET_STR(el2), CSONPATH_GET_STR(jret)));
	    }
	    return 0;
	case CSONPATH_INST_FILTER_KEY_EQ:
	    if (CSONPATH_IS_NUM(el2) && CSONPATH_IS_NUM(jret)) {
		return CSONPATH_GET_NUM(el2) == CSONPATH_GET_NUM(jret);
	    }
	    if (CSONPATH_IS_STR(el2) && CSONPATH_IS_STR(jret)) {
		return (!strcmp(CSONPATH_GET_STR(el2), CSONPATH_GET_STR(jret)));
	    }
	    return 0;
	case CSONPATH_INST_FILTER_KEY_SUPERIOR_EQ:
	case CSONPATH_INST_FILTER_KEY_SUPERIOR:
	    if (CSONPATH_IS_NUM(el2) && CSONPATH_IS_NUM(jret)) {
		if (operation == CSONPATH_INST_FILTER_KEY_SUPERIOR_EQ) {
		    return CSONPATH_GET_NUM(el2) >= CSONPATH_GET_NUM(jret);
		}
		return CSONPATH_GET_NUM(el2) > CSONPATH_GET_NUM(jret);
	    }
	    if (CSONPATH_IS_STR(el2) && CSONPATH_IS_STR(jret)) {
		if (operation == CSONPATH_INST_FILTER_KEY_SUPERIOR_EQ) {
		    return strcmp(CSONPATH_GET_STR(el2), CSONPATH_GET_STR(jret)) >= 0;
		}
		return strcmp(CSONPATH_GET_STR(el2), CSONPATH_GET_STR(jret)) > 0;
	    }
	    return 0;
	case CSONPATH_INST_FILTER_KEY_INFERIOR:
	case CSONPATH_INST_FILTER_KEY_INFERIOR_EQ:
	    if (CSONPATH_IS_NUM(el2) && CSONPATH_IS_NUM(jret)) {
		if (operation == CSONPATH_INST_FILTER_KEY_INFERIOR_EQ) {
		    return CSONPATH_GET_NUM(el2) <= CSONPATH_GET_NUM(jret);
		}
		return CSONPATH_GET_NUM(el2) < CSONPATH_GET_NUM(jret);
	    }
	    if (CSONPATH_IS_STR(el2) && CSONPATH_IS_STR(jret)) {
		if (operation == CSONPATH_INST_FILTER_KEY_INFERIOR_EQ) {
		    return strcmp(CSONPATH_GET_STR(el2), CSONPATH_GET_STR(jret)) <= 0;
		}
		return strcmp(CSONPATH_GET_STR(el2), CSONPATH_GET_STR(jret)) < 0;
	    }
	    return 0;
	default:
	    return 0;
	}
    }
    ++*owalker;

    _Bool match = 0;
    switch (operation) {
    case CSONPATH_INST_FILTER_KEY_NOT_EQ:
	match = !csonpath_do_match(operand_instruction, el2, owalker);
	break;
    case CSONPATH_INST_FILTER_KEY_EQ:
	match = csonpath_do_match(operand_instruction, el2, owalker);
	break;
    case CSONPATH_INST_FILTER_KEY_SUPERIOR:
    case CSONPATH_INST_FILTER_KEY_SUPERIOR_EQ:
	if (el2 == CSONPATH_NULL)
	    return 0;
	if (operand_instruction == CSONPATH_INST_GET_OBJ) {
	    if (!CSONPATH_IS_STR(el2))
		break;
	    if (operation == CSONPATH_INST_FILTER_KEY_SUPERIOR_EQ)
		match = strcmp(CSONPATH_GET_STR(el2), *owalker) >= 0;
	    else
		match = strcmp(CSONPATH_GET_STR(el2), *owalker) > 0;
	    *owalker += strlen(*owalker) + 1;
	} else {
	    if (!CSONPATH_IS_NUM(el2))
		break;
	    if (operation == CSONPATH_INST_FILTER_KEY_SUPERIOR_EQ)
		match = csonpath_int_from_walker(operand_instruction, owalker) <=
		    CSONPATH_GET_NUM(el2);
	    else
		match = csonpath_int_from_walker(operand_instruction, owalker) <
		    CSONPATH_GET_NUM(el2);
	}
	break;
    case CSONPATH_INST_FILTER_KEY_INFERIOR_EQ:
    case CSONPATH_INST_FILTER_KEY_INFERIOR:
	if (el2 == CSONPATH_NULL)
	    return 0;
	if (operand_instruction == CSONPATH_INST_GET_OBJ) {
	    if (!CSONPATH_IS_STR(el2))
		break;
	    if (operation == CSONPATH_INST_FILTER_KEY_INFERIOR_EQ)
		match = strcmp(CSONPATH_GET_STR(el2), *owalker) <= 0;
	    else
		match = strcmp(CSONPATH_GET_STR(el2), *owalker) < 0;
	    *owalker += strlen(*owalker) + 1;
	} else {
	    if (!CSONPATH_IS_NUM(el2))
		break;
	    if (operation == CSONPATH_INST_FILTER_KEY_INFERIOR_EQ)
		match = csonpath_int_from_walker(operand_instruction, owalker) >=
		    CSONPATH_GET_NUM(el2);
	    else
		match = csonpath_int_from_walker(operand_instruction, owalker) >
		    CSONPATH_GET_NUM(el2);
	}
	break;
    case CSONPATH_INST_FILTER_KEY_REG_EQ:
#if defined CSONPATH_NO_REGEX
	fprintf(stderr, "regex deactivate");
	return 0;
#else
	if (el2 == CSONPATH_NULL)
	    return 0;

	if (CSONPATH_IS_STR(el2)) {
	    int regex_idx = **owalker;
	    ++*owalker;
#	if !defined CSONPATH_PCRE2
	    match = csonpath_reg_exec(cjp->regexs[regex_idx], CSONPATH_GET_STR(el2));
#	else
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
#	endif
	}
	break;
#endif
    }
    return match;
}
