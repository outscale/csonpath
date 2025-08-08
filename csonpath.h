#define CSONPATH_UNUSED __attribute__((unused))
#define MAY_ALIAS __attribute__((__may_alias__))

#define CSONPATH_FOREACH(obj, el, code)			\
	CSONPATH_FOREACH_EXT(obj, el, code, key_idx)


enum csonpath_instuction_raw {
	CSONPATH_INST_ROOT,
	CSONPATH_INST_GET_OBJ,
	CSONPATH_INST_GET_ARRAY_SMALL,
	CSONPATH_INST_GET_ARRAY_BIG,
	CSONPATH_INST_FILTER_KEY_EQ,
	CSONPATH_INST_FILTER_OPERAND_STR,
	CSONPATH_INST_FILTER_OPERAND_BYTE, /* store the same way it's in ARRAY_SMALL  */
	CSONPATH_INST_FILTER_OPERAND_INT, /* store the same way it's in ARRAY_BIG  */
	CSONPATH_INST_GET_ALL,
	CSONPATH_INST_FIND_ALL,
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
	"FILTER_KEY_EQ",
	"FILTER_OPERAND_STR",
	"FILTER_OPERAND_BYTE",
	"FILTER_OPERAND_INT",
	"GET_ALL",
	"FIND_ALL",
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
	char inst;
	char unused;
	short int next;
};

#define CSONPATH_INST_MIN_ALLOC (1 << 7)

struct csonpath {
	char *path;
	struct csonpath_instruction *inst_lst;
	int compiled;
	int inst_idx;
	int inst_size;
};

struct csonpath_child_info {
	int type;
	MAY_ALIAS union {
		int idx;
		const char *key;
	};
};

#define CSONPATH_CLASSIC_ERR(args...) do {	\
		fprintf(stderr, args);		\
		goto error;			\
	} while (0)

static inline struct csonpath_child_info *csonpath_child_info_set(struct csonpath_child_info *child_info,
								  const CSONPATH_JSON j, const intptr_t key)
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
	cjp->path = NULL;
}

static inline int csonpath_init(struct csonpath cjp[static 1],
				const char path[static 1]) {
	*cjp = (struct csonpath) {.path=strdup(path),
		.inst_size = CSONPATH_INST_MIN_ALLOC,
		.inst_lst = malloc(sizeof(*cjp->inst_lst) * CSONPATH_INST_MIN_ALLOC)};
	if (!cjp->path || !cjp->inst_lst) {
		return -ENOMEM;
	}
	return 0;
}

static inline int csonpath_set_path(struct csonpath cjp[static 1],
				    const char path[static 1])
{
	free(cjp->path);
	free(cjp->inst_lst);
	return csonpath_init(cjp, path);
}

void csonpath_push_inst(struct csonpath cjp[static 1], int inst)
{
	if (cjp->inst_idx + 1 > cjp->inst_size) {
		cjp->inst_size = cjp->inst_size << 1;
		cjp->inst_lst = realloc(cjp->inst_lst, cjp->inst_size);
		assert(cjp->inst_lst);
	}
	cjp->inst_lst[cjp->inst_idx] = (struct csonpath_instruction){.inst=inst};
	++cjp->inst_idx;
}

int csonpath_compile(struct csonpath cjp[static 1])
{
	char *walker = cjp->path;
	char *next;
	char to_check;

	if (cjp->compiled)
		return 0;
root_again:
	if (*walker != '$') {
		CSONPATH_CLASSIC_ERR("'$' needed");
	}
	++walker;
	csonpath_push_inst(cjp, CSONPATH_INST_ROOT);
	cjp->inst_lst[cjp->inst_idx - 1].next = 1;
	to_check = *walker;

again:
	switch (to_check) {
	case '[':
	{
		int end;

		cjp->inst_lst[cjp->inst_idx - 1].next += 1;
		++walker;
		if (*walker == '*') {
			csonpath_push_inst(cjp, CSONPATH_INST_GET_ALL);
			if (walker[1] != ']') {
				CSONPATH_CLASSIC_ERR("unclose bracket\n");
			}
			cjp->inst_lst[cjp->inst_idx - 1].next = 2;
			walker += 2;
			to_check = *walker;
			goto again;
		} else if (*walker == '?') {
			cjp->inst_lst[cjp->inst_idx - 1].next += 1;
			++walker;
			for (next = walker; isalnum(*next); ++next);
			if (!*next) {
				goto error;
			}
			to_check = *next;
			*next = 0;
			for (++next; isblank(*next); ++next);
			if (!*next)
				goto error;
			/* = and == are the same here */
			if (to_check == '=') {
				csonpath_push_inst(cjp, CSONPATH_INST_FILTER_KEY_EQ);
				if (next[0] == '=')
					++next;
			} else {
				goto error;
			}
			cjp->inst_lst[cjp->inst_idx - 1].next = next - walker;
			walker = next;
			if (*walker == '"' || *walker == '\'') {
				char end = *walker;
				++walker;
				cjp->inst_lst[cjp->inst_idx - 1].next++;
				csonpath_push_inst(cjp, CSONPATH_INST_FILTER_OPERAND_STR);
				for (next = walker; *next && *next != end; ++next);
				if (!*next)
					goto error;
				*next = 0;
				++next;
				to_check = *next;
			} else {
				int n;

				for (next = walker; isdigit(*next); ++next);
				if (next == walker)
					goto error;
				n = atoi(walker);
				if (n < 100) {
					*walker = n;
					csonpath_push_inst(cjp, CSONPATH_INST_FILTER_OPERAND_BYTE);
				} else {
					MAY_ALIAS int *walk_int = (void *)walker;
					*walk_int = n;
					csonpath_push_inst(cjp, CSONPATH_INST_FILTER_OPERAND_INT);
				}
				if (!*next) {
					goto error;
				}
				to_check = *next;
				*next = 0;
			}
			if (isblank(to_check)) {
				for (next++; isblank(*next); ++next);
				to_check = *next;
			}
			if (to_check != ']')
				goto error;
			/* skipp space */
			cjp->inst_lst[cjp->inst_idx - 1].next = next - walker + 1;
			walker = next + 1;
			to_check = *walker;
			goto again;
		} else if (*walker != '"' && *walker != '\'') {
			int num;

			next = walker;
			do {
				if (*next < '0' || *next > '9') {
					CSONPATH_CLASSIC_ERR("unexpected '%c', sting or number require\n", *next);
				}
				next++;
			} while (*next && *next != ']');
			if (!*next) {
				CSONPATH_CLASSIC_ERR("unclose bracket\n");
			}
			*next = 0;
			num = atoi(walker);
			if (num < 100) {
				csonpath_push_inst(cjp, CSONPATH_INST_GET_ARRAY_SMALL);
				*walker = num;
			} else {
				union {
					int n;
					char c[4];
				} u = {.n=num};
				walker[0] = u.c[0];
				walker[1] = u.c[1];
				walker[2] = u.c[2];
				walker[3] = u.c[3];
				csonpath_push_inst(cjp, CSONPATH_INST_GET_ARRAY_BIG);
			}
			cjp->inst_lst[cjp->inst_idx - 1].next = next - walker + 1;
			walker = next + 1;
			to_check = *walker;
			goto again;
		} else {
			end = *walker;
			cjp->inst_lst[cjp->inst_idx - 1].next += 1;

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
				CSONPATH_CLASSIC_ERR("']' require instead of '%c'\n", *next);

			csonpath_push_inst(cjp, CSONPATH_INST_GET_OBJ);
			cjp->inst_lst[cjp->inst_idx - 1].next = next - walker + 1;

			walker = next + 1;
			to_check = *walker;
			goto again;
		}
	}
	case '.':
	{
		int inst = CSONPATH_INST_GET_OBJ;

		cjp->inst_lst[cjp->inst_idx - 1].next += 1;
		++walker;
		if (*walker == '.') {
			inst = CSONPATH_INST_FIND_ALL;
			cjp->inst_lst[cjp->inst_idx - 1].next += 1;
			++walker;
		}
		for (next = walker; *next && *next != '[' && *next != '.' && *next != '|';
		     ++next);
		to_check = *next;
		*next = 0;

		csonpath_push_inst(cjp, inst);
		cjp->inst_lst[cjp->inst_idx - 1].next = next - walker;
		walker = next;
		goto again;
	}
	case '|':
		/* cjp->inst_lst[cjp->inst_idx - 1].next += 1; */
		csonpath_push_inst(cjp, CSONPATH_INST_OR);
		++walker;
		cjp->inst_lst[cjp->inst_idx - 1].next += 1;
		goto root_again;
	}
	if (*walker == 0) {
		csonpath_push_inst(cjp, CSONPATH_INST_END);
		cjp->compiled = 1;
		return 0;
	}
error:
	cjp->inst_lst[0] = (struct csonpath_instruction){.inst=CSONPATH_INST_BROKEN};
	return -1;
}

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

#define CSONPATH_DO_DECLARATION				\
	CSONPATH_JSON good_ret = CSONPATH_NEW_ARRAY();	\
	int nb_res = 0;

#define CSONPATH_DO_FUNC_NAME find_all
#define CSONPATH_DO_RET_TYPE CSONPATH_JSON
#define CSONPATH_DO_RETURN ({CSONPATH_ARRAY_APPEND_INCREF(good_ret, tmp); return good_ret;})

#define CSONPATH_DO_FIND_ALL						\
	CSONPATH_JSON hel;						\
									\
	CSONPATH_FOREACH(tret, hel, {					\
			CSONPATH_ARRAY_APPEND_INCREF(good_ret, hel);	\
			++nb_res;					\
		});							\
	CSONPATH_REMOVE(tret);

#define CSONPATH_DO_FILTER_FIND CSONPATH_DO_FIND_ALL

#define CSONPATH_DO_FIND_ALL_CLEAUP CSONPATH_REMOVE(good_ret);

#define CSONPATH_DO_FIND_ALL_OUT		\
	if (!nb_res)				\
		CSONPATH_NONE_FOUND_RET;	\
	return good_ret;

#include "csonpath_do.h"

/* Delete */

#undef CSONPATH_NONE_FOUND_RET
#undef CSONPATH_GETTER_ERR

#define CSONPATH_NONE_FOUND_RET 0

#define CSONPATH_GETTER_ERR(args...) do {	\
		fprintf(stderr, args);		\
		return -1;			\
	} while (0)

#define CSONPATH_DO_ON_FOUND

#define CSONPATH_DO_RET_TYPE int
#define CSONPATH_DO_FUNC_NAME remove
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
			if (need_reloop_in){ goto again; };	\
			if (tret < 0) return -1;		\
			nb_res += tret;				\
		})

#define CSONPATH_DO_EXTRA_DECLATION , struct csonpath_child_info child_info, int *need_reloop

#define CSONPATH_DO_EXTRA_ARGS_IN , (struct csonpath_child_info) {.type = CSONPATH_NONE}, NULL

#define CSONPATH_DO_FILTER_PRE_LOOP		\
	int need_reloop_in;

#define CSONPATH_DO_FIND_ALL_PRE_LOOP		\
	int need_reloop_in;			\
again:

#define CSONPATH_DO_FOREACH_PRE_SET					\
	need_reloop_in = 0;						\
	csonpath_child_info_set(&child_info, tmp, (intptr_t)key_idx);

#define CSONPATH_DO_EXTRA_ARGS_NEESTED , child_info, &need_reloop_in

#define CSONPATH_DO_EXTRA_ARGS_FIND_ALL , child_info, need_reloop


#include "csonpath_do.h"

/* update_or_create */

#define CSONPATH_DO_DECLARATION			\
	int nb_res = 0;

#define CSONPATH_DO_RET_TYPE int
#define CSONPATH_DO_FUNC_NAME update_or_ceate
#define CSONPATH_DO_RETURN						\
	if (tmp == value) {						\
		*need_reloop = 1;					\
		printf("should update %d - %d !!\n", child_info->type == CSONPATH_INTEGER, child_info->idx); \
		if (child_info->type == CSONPATH_INTEGER)		\
			CSONPATH_APPEND_AT(ctx, child_info->idx, to_update); \
		else							\
			CSONPATH_APPEND_AT(ctx, child_info->key, to_update); \
	}								\
	return 0;

#define CSONPATH_DO_EXTRA_ARGS_FIND_ALL , to_update, NULL, need_reloop
#define CSONPATH_DO_EXTRA_ARGS_NEESTED , to_update,			\
		csonpath_child_info_set(&(struct csonpath_child_info ){}, tmp, (intptr_t)key_idx), &need_reloop_in
#define CSONPATH_DO_EXTRA_ARGS , CSONPATH_JSON to_update
#define CSONPATH_DO_EXTRA_ARGS_IN , to_update, NULL, NULL
#define CSONPATH_DO_EXTRA_DECLATION CSONPATH_DO_EXTRA_ARGS, struct csonpath_child_info *child_info, int *need_reloop
#define CSONPATH_DO_FIND_ALL if (need_reloop_in) goto find_again; nb_res += tret;
#define CSONPATH_DO_FILTER_FIND if (need_reloop_in) goto filter_again; nb_res += tret;

#define CSONPATH_DO_FIND_ALL_PRE_LOOP		\
	int need_reloop_in;			\
find_again:					\
need_reloop_in = 0;

#define CSONPATH_DO_FILTER_PRE_LOOP		\
	int need_reloop_in;			\
filter_again:					\
need_reloop_in = 0;

#define CSONPATH_DO_FIND_ALL_OUT return nb_res;


#define CSONPATH_PRE_GET(this_idx)					\
	int check_at = idx + 1;						\
	int to_check;							\
	do {								\
		to_check = cjp->inst_lst[check_at].inst;		\
		++check_at;						\
	} while (to_check == CSONPATH_INST_GET_ALL || to_check == CSONPATH_INST_FIND_ALL); \
	if (to_check == CSONPATH_INST_END || to_check == CSONPATH_INST_OR) { \
		CSONPATH_APPEND_AT(ctx, this_idx, to_update);		\
		return 1;						\
	}


#define CSONPATH_DO_GET_NOTFOUND(this_idx)		\
	if (to_check == CSONPATH_INST_GET_OBJ) {	\
		tmp = CSONPATH_NEW_OBJECT();		\
		CSONPATH_APPEND_AT(ctx, this_idx, tmp);	\
	} else {					\
		tmp = CSONPATH_NEW_ARRAY();		\
		CSONPATH_APPEND_AT(ctx, this_idx, tmp);	\
	}						\
	goto next_inst;



#include "csonpath_do.h"
