#ifndef CAT
# define CATCAT(a, b, c) a ## b ## c
# define CAT(a, b) a ## b
#endif

#define csonpath_do_internal__(name) CATCAT(csonpath_, name, _internal)
#define csonpath_do_internal csonpath_do_internal__(CSONPATH_DO_FUNC_NAME)

static CSONPATH_DO_RET_TYPE csonpath_do_internal(struct csonpath *cjp,
						 CSONPATH_JSON value,
						 CSONPATH_JSON ctx,
						 int idx,
						 char *walker)
{
  CSONPATH_JSON tmp = value;
  CSONPATH_DO_DECLARATION;

  (void)ctx; /* maybe unused */

  while (cjp->inst_lst[idx].inst != CSONPATH_INST_END) {
    switch (cjp->inst_lst[idx].inst) {
    case CSONPATH_INST_ROOT:
        walker += cjp->inst_lst[0].next;
	break;
    case CSONPATH_INST_GET_ALL:
      {
	CSONPATH_JSON el;

	CSONPATH_FOREACH(tmp, el, {
	    CSONPATH_DO_RET_TYPE tret =
	      csonpath_do_internal(cjp, el, tmp, idx + 1,
				     walker + cjp->inst_lst[idx].next);

	    CSONPATH_DO_FIND_ALL;
	  })

	  CSONPATH_DO_FIND_ALL_OUT;
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
  CSONPATH_DO_RETURN;
}

#define csonpath_do__(name) CAT(csonpath_, name)
#define csonpath_do_ csonpath_do__(CSONPATH_DO_FUNC_NAME)

static CSONPATH_DO_RET_TYPE csonpath_do_(struct csonpath *cjp, CSONPATH_JSON value)
{
  char *walker = cjp->path;
  CSONPATH_JSON ctx = CSONPATH_NULL;

  (void)ctx;
  csonpath_compile(cjp);
  if (cjp->inst_lst[0].inst == CSONPATH_INST_BROKEN) {
    CSONPATH_GETTER_ERR("fail to compile\n");
    return CSONPATH_NONE_FOUND_RET;
  }

  return csonpath_do_internal(cjp, value, CSONPATH_NULL, 0, walker);
}

#undef CSONPATH_DO_FUNC_NAME
#undef CSONPATH_DO_RET_TYPE
#undef CSONPATH_DO_RETURN
#undef CSONPATH_DO_DECLARATION
#undef CSONPATH_DO_FIND_ALL
#undef CSONPATH_DO_FIND_ALL_OUT
