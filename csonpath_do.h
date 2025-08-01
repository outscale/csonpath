#ifndef CSONPATH_DO_GET_NOTFOUND
#define CSONPATH_DO_GET_NOTFOUND(this_idx)				\
	do {								\
		walker += cjp->inst_lst[idx].next;			\
		while (cjp->inst_lst[++idx].inst != CSONPATH_INST_END) { \
			if (cjp->inst_lst[idx].inst == CSONPATH_INST_OR) { \
				walker += cjp->inst_lst[idx].next;	\
				goto next_inst;				\
			}						\
			walker += cjp->inst_lst[idx].next;		\
		}							\
		return CSONPATH_NONE_FOUND_RET;				\
	} while (0)
#endif

#ifndef CSONPATH_PRE_GET
#define CSONPATH_PRE_GET(this_idx)
#endif

#ifndef CSONPATH_DO_DECLARATION
#define CSONPATH_DO_DECLARATION
#endif

#ifndef CSONPATH_DO_POST_FIND_OBJ
#define CSONPATH_DO_POST_FIND_OBJ
#endif

#ifndef CSONPATH_DO_POST_FIND_ARRAY
#define CSONPATH_DO_POST_FIND_ARRAY
#endif

#ifndef CSONPATH_DO_EXTRA_DECLATION
#define CSONPATH_DO_EXTRA_DECLATION
#endif

#ifndef CSONPATH_DO_FIND_ALL_CLEAUP
#define CSONPATH_DO_FIND_ALL_CLEAUP
#endif

#ifndef CSONPATH_DO_EXTRA_ARGS
#define CSONPATH_DO_EXTRA_ARGS
#endif

#ifndef CSONPATH_DO_EXTRA_ARGS_NEESTED
# ifdef CSONPATH_DO_EXTRA_ARGS_IN
# define CSONPATH_DO_EXTRA_ARGS_NEESTED CSONPATH_DO_EXTRA_ARGS_IN
# else
# define CSONPATH_DO_EXTRA_ARGS_NEESTED
# endif
#endif

#ifndef CSONPATH_DO_EXTRA_ARGS_IN
#define CSONPATH_DO_EXTRA_ARGS_IN
#endif

#ifndef CSONPATH_DO_POST_OPERATION
#define CSONPATH_DO_POST_OPERATION
#endif

#ifndef CSONPATH_DO_PRE_OPERATION
#define CSONPATH_DO_PRE_OPERATION
#endif

#ifndef CSONPATH_DO_FIND_ALL_PRE_LOOP
#define CSONPATH_DO_FIND_ALL_PRE_LOOP
#endif

#ifndef CSONPATH_DO_FOREACH_PRE_SET
#define CSONPATH_DO_FOREACH_PRE_SET
#endif

#ifndef CSONPATH_DO_GET_ALL_OUT
#define CSONPATH_DO_GET_ALL_OUT CSONPATH_DO_FIND_ALL_OUT
#endif

#ifndef CAT
# define CATCAT(a, b, c) a ## b ## c
# define CAT(a, b) a ## b
#endif

#define csonpath_do_internal__(name) CATCAT(csonpath_, name, _internal)
#define csonpath_do_internal csonpath_do_internal__(CSONPATH_DO_FUNC_NAME)

static CSONPATH_DO_RET_TYPE csonpath_do_internal(struct csonpath cjp[static 1],
						 CSONPATH_JSON origin,
						 CSONPATH_JSON value,
						 CSONPATH_JSON ctx,
						 int idx,
						 char *walker CSONPATH_DO_EXTRA_DECLATION);

#define csonpath_do_dotdot__(name) CATCAT(csonpath_, name, _dotdot)
#define csonpath_do_dotdot csonpath_do_dotdot__(CSONPATH_DO_FUNC_NAME)


static CSONPATH_DO_RET_TYPE csonpath_do_dotdot(struct csonpath cjp[static 1],
					       CSONPATH_JSON origin,
					       CSONPATH_JSON tmp,
					       CSONPATH_JSON ctx,
					       int idx,
					       char *walker CSONPATH_DO_EXTRA_DECLATION)
{
  	CSONPATH_JSON el;
	CSONPATH_DO_DECLARATION;
	const int is_obj = CSONPATH_IS_OBJ(tmp);
	CSONPATH_DO_RET_TYPE tret = CSONPATH_NONE_FOUND_RET;

	CSONPATH_DO_FIND_ALL_PRE_LOOP;
	CSONPATH_FOREACH(tmp, el, {
	    CSONPATH_DO_FOREACH_PRE_SET;
	    if (is_obj && !strcmp((char *)key_idx, walker)) {
	      tret = csonpath_do_internal(cjp, origin, el, tmp, idx,
					  walker + cjp->inst_lst[idx].next
					  CSONPATH_DO_EXTRA_ARGS_NEESTED);
	      CSONPATH_DO_FIND_ALL;
	    }
	    if (CSONPATH_IS_OBJ(el) || CSONPATH_IS_ARRAY(el)) {
	      tret = csonpath_do_dotdot(cjp, origin, el, tmp, idx, walker
					CSONPATH_DO_EXTRA_ARGS_NEESTED);
	      CSONPATH_DO_FIND_ALL;
	    }
	  });
	CSONPATH_DO_FIND_ALL_OUT;
	return tret;
}

static CSONPATH_DO_RET_TYPE csonpath_do_internal(struct csonpath cjp[static 1],
						 CSONPATH_JSON origin,
						 CSONPATH_JSON value,
						 CSONPATH_JSON ctx,
						 int idx,
						 char *walker CSONPATH_DO_EXTRA_DECLATION)
{
  CSONPATH_JSON tmp = value;
  CSONPATH_DO_DECLARATION;

  (void)ctx; /* maybe unused */

  while (cjp->inst_lst[idx].inst != CSONPATH_INST_END &&
	 cjp->inst_lst[idx].inst != CSONPATH_INST_OR) {
    switch (cjp->inst_lst[idx].inst) {
    case CSONPATH_INST_ROOT:
      value = origin;
      tmp = value;
      ctx = CSONPATH_NULL;
      walker += cjp->inst_lst[idx].next;
      break;
    case CSONPATH_INST_FIND_ALL:
      {
	int need_reloop_in;

	(void)need_reloop_in;
	CSONPATH_DO_RET_TYPE tret =
	  csonpath_do_dotdot(cjp, origin, tmp, ctx, idx + 1,
			     walker
			     CSONPATH_DO_EXTRA_ARGS_NEESTED);
	CSONPATH_DO_FIND_ALL_CLEAUP;
	return tret;
      }
    case CSONPATH_INST_GET_ALL:
      {
	CSONPATH_JSON el;

	CSONPATH_DO_FIND_ALL_PRE_LOOP;
	CSONPATH_FOREACH(tmp, el, {
	    CSONPATH_DO_FOREACH_PRE_SET
	    CSONPATH_DO_RET_TYPE tret =
	      csonpath_do_internal(cjp, origin, el, tmp, idx + 1,
				     walker + cjp->inst_lst[idx].next
				   CSONPATH_DO_EXTRA_ARGS_NEESTED);

	    CSONPATH_DO_FIND_ALL;
	  })

	  CSONPATH_DO_GET_ALL_OUT;
	break;
      }
    case CSONPATH_INST_GET_OBJ:
      ctx = tmp;
      CSONPATH_PRE_GET(walker);
      tmp = CSONPATH_GET(tmp, walker);
      if (tmp == CSONPATH_NULL) {
	CSONPATH_DO_GET_NOTFOUND(walker);
      }
      CSONPATH_DO_POST_FIND_OBJ
      walker += cjp->inst_lst[idx].next;
      break;
    case CSONPATH_INST_GET_ARRAY_SMALL:
      {
	int this_idx;

	ctx = tmp;
	this_idx =  (int)*walker;
	CSONPATH_PRE_GET(this_idx);
	tmp = CSONPATH_AT(tmp, this_idx);
	if (tmp == CSONPATH_NULL) {
	  CSONPATH_DO_GET_NOTFOUND(this_idx);
	}
	walker += cjp->inst_lst[idx].next;
	CSONPATH_DO_POST_FIND_ARRAY
	break;
      }
    case CSONPATH_INST_GET_ARRAY_BIG:
      ctx = tmp;
      {
	CSONPATH_UNUSED int this_idx;
	union {int n; char c[4];} to_num =
	  { .c= { walker[0], walker[1], walker[2], walker[3] } };
	this_idx = to_num.n;
	CSONPATH_PRE_GET(this_idx);
	tmp = CSONPATH_AT(tmp, to_num.n);
	if (tmp == CSONPATH_NULL) {
	  CSONPATH_DO_GET_NOTFOUND(this_idx);
	}
	walker += cjp->inst_lst[idx].next;
	CSONPATH_DO_POST_FIND_ARRAY
	break;
      }
    default:
      CSONPATH_GETTER_ERR("unimplemented %d\n", cjp->inst_lst[idx].inst);
    }
  next_inst:
    ++idx;
  }
  CSONPATH_DO_RETURN;
}

#define csonpath_do__(name) CAT(csonpath_, name)
#define csonpath_do_ csonpath_do__(CSONPATH_DO_FUNC_NAME)

static CSONPATH_DO_RET_TYPE csonpath_do_(struct csonpath cjp[static 1],
					 CSONPATH_JSON value CSONPATH_DO_EXTRA_ARGS)
{
  char *walker = cjp->path;
  CSONPATH_JSON ctx = CSONPATH_NULL;

  (void)ctx;
  csonpath_compile(cjp);
  if (cjp->inst_lst[0].inst == CSONPATH_INST_BROKEN) {
    CSONPATH_GETTER_ERR("fail to compile\n");
    return CSONPATH_NONE_FOUND_RET;
  }

  CSONPATH_DO_PRE_OPERATION;

  CSONPATH_DO_RET_TYPE ret =  csonpath_do_internal(cjp, value, value,
						   CSONPATH_NULL, 0,
						   walker CSONPATH_DO_EXTRA_ARGS_IN);

  CSONPATH_DO_POST_OPERATION;

  return ret;
}

#undef CSONPATH_PRE_GET
#undef CSONPATH_DO_PRE_OPERATION
#undef CSONPATH_DO_POST_OPERATION
#undef CSONPATH_DO_FUNC_NAME
#undef CSONPATH_DO_RET_TYPE
#undef CSONPATH_DO_RETURN
#undef CSONPATH_DO_DECLARATION
#undef CSONPATH_DO_FIND_ALL
#undef CSONPATH_DO_FIND_ALL_OUT
#undef CSONPATH_DO_POST_FIND_ARRAY
#undef CSONPATH_DO_POST_FIND_OBJ
#undef CSONPATH_DO_EXTRA_ARGS_NEESTED
#undef CSONPATH_DO_EXTRA_ARGS_IN
#undef CSONPATH_DO_EXTRA_ARGS
#undef CSONPATH_DO_EXTRA_DECLATION
#undef CSONPATH_DO_FIND_ALL_PRE_LOOP
#undef CSONPATH_DO_FOREACH_PRE_SET
#undef CSONPATH_DO_GET_ALL_OUT
#undef CSONPATH_DO_GET_NOTFOUND
#undef CSONPATH_DO_FIND_ALL_CLEAUP
