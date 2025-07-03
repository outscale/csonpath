enum csonpath_instuction_raw {
  CSONPATH_INST_ROOT,
  CSONPATH_INST_GET_OBJ,
  CSONPATH_INST_GET_ARRAY_SMALL,
  CSONPATH_INST_GET_ARRAY_BIG,
  CSONPATH_INST_GET_ALL,
  CSONPATH_INST_END,
  CSONPATH_INST_BROKEN
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

/*
 * field contain an array of:
 * CSONPATH_JSON value;
 * CSONPATH_JSON context;
 * CSONPATH_JSON (int) index | CSONPATH_JSON (str) key
 */
struct csonpath_ret {
  CSONPATH_JSON fields;
  int number;
};

#define CSONPATH_GETTER_ERR(__VA_ARGS__...) do {	\
    fprintf(stderr, __VA_ARGS__);			\
    return CSONPATH_NONE_FOUND_RET;			\
} while (0)

#define CSONPATH_CLASSIC_ERR(__VA_ARGS__...) do {	\
    fprintf(stderr, __VA_ARGS__);			\
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

#define CSONPATH_NONE_FOUND_RET(last_idx) ({			\
      struct csonpath_ret r;					\
      r.number = 0;						\
      r.fields = CSONPATH_NEW_ARRAY();				\
      CSONPATH_JSON rr = CSONPATH_APPEND_NEW_OBJECT(r.fields);	\
      CSONPATH_APPEND_PUSH_OBJECT(rr, "value", CSONPATH_NULL);	\
      CSONPATH_APPEND_PUSH_OBJECT(rr, "ctx", ctx);		\
      _Generic(last_idx, int: CSONPATH_SET_INT_AT,		\
	       char *: CSONPATH_SET_STR_AT,			\
	       const char *: CSONPATH_SET_STR_AT,		\
	       )(rr, "key", last_idx);				\
      CSONPATH_APPEND_PUSH_OBJECT(rr, "", CSONPATH_NULL);	\
      r;							\
    })

// (struct csonpath_ret){CSONPATH_NULL, ctx, 0};

static void csonpath_destroy_ret(struct csonpath_ret *r)
{
  if (r->number) {
    CSONPATH_REMOVE(r->value);
  }
  r->number = 0;
  r->value = CSONPATH_NULL;
}

static struct csonpath_ret csonpath_find_internal(struct csonpath *cjp,
						  CSONPATH_JSON value,
						  CSONPATH_JSON ctx,
						  int idx,
						  char *walker)
{
  CSONPATH_JSON tmp = value;


  while (cjp->inst_lst[idx].inst != CSONPATH_INST_END) {
    switch (cjp->inst_lst[idx].inst) {
    case CSONPATH_INST_ROOT:
        walker += cjp->inst_lst[0].next;
	break;
    case CSONPATH_INST_GET_ALL:
      {
	int nb_res = 0;
	CSONPATH_JSON good_ret = CSONPATH_NEW_ARRAY();
	CSONPATH_JSON good_ctxs = CSONPATH_NEW_ARRAY();
	CSONPATH_JSON el;

	CSONPATH_FOREACH(tmp, el, {
	    struct csonpath_ret tret =
	      csonpath_find_internal(cjp, el, tmp, idx + 1,
				     walker + cjp->inst_lst[idx].next);

	    if (tret.number == 1) {
	      CSONPATH_ARRAY_APPEND(good_ret, tret.value);
	      CSONPATH_ARRAY_APPEND(good_ctxs, tret.context);
	      ++nb_res;
	    } else if (tret.number > 1) {
	      CSONPATH_JSON hel;

	      CSONPATH_FOREACH(tret.value, hel, {
		  CSONPATH_ARRAY_APPEND(good_ret, hel);
		  ++nb_res;
		});

	      CSONPATH_FOREACH(tret.context, hel, {
		  CSONPATH_ARRAY_APPEND(good_ctxs, hel);
		});

	      CSONPATH_REMOVE(tret.value);
	    }
	  });
	if (!nb_res)
	  CSONPATH_NONE_FOUND_RET;
	return (struct csonpath_ret){good_ret, good_ctxs, nb_res};
	break;
      }
    case CSONPATH_INST_GET_OBJ:
      ctx = tmp;
      tmp = CSONPATH_GET(tmp, walker);
      if (tmp == CSONPATH_NULL)
	return CSONPATH_NONE_FOUND_RET;
      walker += cjp->inst_lst[idx].next;
      break;
    case CSONPATH_INST_GET_ARRAY_SMALL:
      ctx = tmp;
      tmp = CSONPATH_AT(tmp, (int)*walker);
      if (tmp == CSONPATH_NULL)
	return CSONPATH_NONE_FOUND_RET;
      walker += cjp->inst_lst[idx].next;
      break;
    case CSONPATH_INST_GET_ARRAY_BIG:
      ctx = tmp;
      {
	union {int n; char c[4];} to_num =
	  { .c= { walker[0], walker[1], walker[2], walker[3] } }; 
	tmp = CSONPATH_AT(tmp, to_num.n);
	if (tmp == CSONPATH_NULL)
	  return CSONPATH_NONE_FOUND_RET;
	walker += cjp->inst_lst[idx].next;
	break;
      }
    default:
      CSONPATH_GETTER_ERR("unimplemented %d\n", cjp->inst_lst[idx].inst);
    }
    ++idx;
  }
  return (struct csonpath_ret){tmp, ctx, 1};
}

static struct csonpath_ret csonpath_find(struct csonpath *cjp, CSONPATH_JSON value)
{
  char *walker = cjp->path;
  CSONPATH_JSON ctx = CSONPATH_NULL;

  csonpath_compile(cjp);
  if (cjp->inst_lst[0].inst == CSONPATH_INST_BROKEN) {
    CSONPATH_GETTER_ERR("fail to compile\n");
    return CSONPATH_NONE_FOUND_RET;
  }

  return csonpath_find_internal(cjp, value, CSONPATH_NULL, 0, walker);
}
