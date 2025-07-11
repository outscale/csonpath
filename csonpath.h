enum csonpath_instuction_raw {
  CSONPATH_INST_ROOT,
  CSONPATH_INST_GET_OBJ,
  CSONPATH_INST_GET_ARRAY_SMALL,
  CSONPATH_INST_GET_ARRAY_BIG,
  CSONPATH_INST_GET_ALL,
  CSONPATH_INST_END,
  CSONPATH_INST_BROKEN
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
  union {
    int idx;
    const char *key;
  };
};

#define CSONPATH_CLASSIC_ERR(args...) do {	\
    fprintf(stderr, args);			\
    goto error;						\
} while (0)

static inline void csonpath_destroy(struct csonpath *cjp)
{
  free(cjp->path);
  free(cjp->inst_lst);
  cjp->path = NULL;
}

static inline int csonpath_init(struct csonpath *cjp, const char *path) {
  cjp->path = strdup(path);
  cjp->inst_size = CSONPATH_INST_MIN_ALLOC;
  cjp->compiled = 0;
  cjp->inst_lst = malloc(CSONPATH_INST_MIN_ALLOC);
  cjp->inst_idx = 0;
  return 0;
}

static inline void csonpath_set_path(struct csonpath *cjp, const char *path)
{
  free(cjp->path);
  csonpath_init(cjp, path);
}

int csonpath_update_or_create(struct csonpath *cjp, CSONPATH_JSON value)
{
  printf("csonpath_update_or_create not implemented\n");
  return -1;
}

void csonpath_push_inst(struct csonpath *cjp, int inst)
{
  if (cjp->inst_idx + 1 > cjp->inst_size) {
    cjp->inst_size = cjp->inst_size << 1;
    cjp->inst_lst = realloc(cjp->inst_lst, cjp->inst_size);
  }
  cjp->inst_lst[cjp->inst_idx] = (struct csonpath_instruction){inst};
  ++cjp->inst_idx;
}

int csonpath_compile(struct csonpath *cjp)
{
  char *walker = cjp->path;
  char *next;
  char to_check;

  if (cjp->compiled)
    return 0;
  if (*walker != '$') {
    CSONPATH_CLASSIC_ERR("'$' needed");
  }
  ++walker;
  csonpath_push_inst(cjp, CSONPATH_INST_ROOT);
  cjp->inst_lst[0].next = 1;
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
	cjp->inst_lst[cjp->inst_idx - 1].next = next - walker;
	walker = next;
	to_check = *walker;
	goto again;
      }

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
  case '.':
    {
      cjp->inst_lst[cjp->inst_idx - 1].next += 1;
      ++walker;
      for (next = walker; *next && *next != '[' && *next != '.'; ++next);
      to_check = *next;
      *next = 0;

      csonpath_push_inst(cjp, CSONPATH_INST_GET_OBJ);
      cjp->inst_lst[cjp->inst_idx - 1].next = next - walker;
      walker = next;
      goto again;
    }
  }
  if (*walker == 0) {
    csonpath_push_inst(cjp, CSONPATH_INST_END);
    cjp->compiled = 1;
    return 0;
  }
 error:
  cjp->inst_lst[0] = (struct csonpath_instruction){CSONPATH_INST_BROKEN};
  return -1;
}

#define CSONPATH_NONE_FOUND_RET CSONPATH_NULL

#define CSONPATH_GETTER_ERR(args...) do {		\
    fprintf(stderr, args);				\
    return CSONPATH_NULL;				\
} while (0)

/* Find First */

#define CSONPATH_DO_RET_TYPE CSONPATH_JSON
#define CSONPATH_DO_FUNC_NAME find_first
#define CSONPATH_DO_RETURN return tmp

#define CSONPATH_DO_FIND_ALL return tret

#include "csonpath_do.h"

/* Find All */

#define CSONPATH_DO_DECLARATION				\
  CSONPATH_JSON good_ret = CSONPATH_NEW_ARRAY();	\
  int nb_res = 0;

#define CSONPATH_DO_FUNC_NAME find_all
#define CSONPATH_DO_RET_TYPE CSONPATH_JSON
#define CSONPATH_DO_RETURN ({CSONPATH_ARRAY_APPEND_INCREF(good_ret, tmp); return good_ret;})

#define CSONPATH_DO_FIND_ALL				\
  CSONPATH_JSON hel;					\
							\
  CSONPATH_FOREACH(tret, hel, {				\
      CSONPATH_ARRAY_APPEND_INCREF(good_ret, hel);	\
      ++nb_res;						\
    });							\
  CSONPATH_REMOVE(tret);

#define CSONPATH_DO_FIND_ALL_OUT		\
  if (!nb_res)					\
    CSONPATH_NONE_FOUND_RET;			\
  return good_ret;

#include "csonpath_do.h"

/* Delete */

#undef CSONPATH_NONE_FOUND_RET
#undef CSONPATH_GETTER_ERR

#define CSONPATH_NONE_FOUND_RET 0

#define CSONPATH_GETTER_ERR(args...) do {			\
    fprintf(stderr, args);					\
    return -1;							\
} while (0)

#define CSONPATH_DO_ON_FOUND

#define CSONPATH_DO_RET_TYPE int
#define CSONPATH_DO_FUNC_NAME remove
#define CSONPATH_DO_RETURN						\
  ({if (ctx == in_ctx && need_reloop &&					\
	CSONPATH_NEED_FOREACH_REDO(ctx))				\
      *need_reloop = 1;							\
    CSONPATH_REMOVE_CHILD(ctx, child_info); return 1;})

#define CSONPATH_DO_POST_FIND_ARRAY		\
  child_info.type = CSONPATH_INTEGER;		\
  child_info.idx = idx;

#define CSONPATH_DO_POST_FIND_OBJ		\
  child_info.type = CSONPATH_STR;		\
  child_info.key = walker;

#define CSONPATH_DO_DECLARATION  int nb_res = 0; \
  CSONPATH_JSON in_ctx = ctx;

#define CSONPATH_DO_FIND_ALL_OUT return nb_res;

#define CSONPATH_DO_FIND_ALL ({					\
      if (need_reloop_in){ printf("again\n"); goto again; };	\
      if (tret < 0) return -1;					\
      nb_res += tret;						\
    })

#define CSONPATH_DO_EXTRA_DECLATION , struct csonpath_child_info child_info, int *need_reloop

#define CSONPATH_DO_EXTRA_ARGS_IN , (struct csonpath_child_info) {.type = CSONPATH_NONE}, NULL

#define CSONPATH_DO_FIND_ALL_PRE_LOOP		\
  again:


#define CSONPATH_DO_FOREACH_PRE_SET			\
  int need_reloop_in = 0;				\
  if (CSONPATH_IS_OBJ(tmp)) {				\
    child_info.key = (void *)(intptr_t)key_idx;		\
    child_info.type = CSONPATH_STR;			\
  } else {						\
    child_info.idx = (intptr_t)key_idx;			\
    child_info.type = CSONPATH_INTEGER;			\
  }

#define CSONPATH_DO_EXTRA_ARGS_NEESTED , child_info, &need_reloop_in

#include "csonpath_do.h"
