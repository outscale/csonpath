#ifndef CSONPATH_DO_GET_NOTFOUND
#define CSONPATH_DO_GET_NOTFOUND(this_idx)				\
    do {								\
	walker += 1;							\
	while (*walker != CSONPATH_INST_END) {				\
	    if (*walker == CSONPATH_INST_OR) {				\
		goto next_inst;						\
	    }								\
	    walker = csonpath_walker_next_inst(walker);			\
	}								\
	return CSONPATH_NONE_FOUND_RET;					\
    } while (0)
#endif

#ifndef CSONPATH_PRE_GET
#define CSONPATH_PRE_GET(this_idx)
#endif

#ifndef CSONPATH_PRE_GET_OBJ
#define CSONPATH_PRE_GET_OBJ(x) CSONPATH_PRE_GET(x)
#endif

#ifndef CSONPATH_PRE_GET_ROOT
#define CSONPATH_PRE_GET_ROOT
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

#ifndef CSONPATH_DO_EXTRA_ARGS_FIND_ALL
#define CSONPATH_DO_EXTRA_ARGS_FIND_ALL CSONPATH_DO_EXTRA_ARGS_NEESTED
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

#ifndef CSONPATH_DO_FILTER_OUT
#define CSONPATH_DO_FILTER_OUT CSONPATH_DO_FIND_ALL_OUT
#endif

#ifndef CSONPATH_DO_FILTER_PRE_LOOP
#define CSONPATH_DO_FILTER_PRE_LOOP
#endif

#ifndef CSONPATH_DO_FILTER_LOOP_PRE_SET
#define CSONPATH_DO_FILTER_LOOP_PRE_SET
#endif

#ifndef CSONPATH_DO_RANGE
#define CSONPATH_DO_RANGE CSONPATH_DO_FIND_ALL
#endif

#ifndef CSONPATH_DO_RANGE
#define CSONPATH_DO_RANGE CSONPATH_DO_FIND_ALL
#endif

#ifndef CSONPATH_DO_RANGE_PRE_LOOP
#define CSONPATH_DO_RANGE_PRE_LOOP CSONPATH_DO_FIND_ALL_PRE_LOOP
#endif

#ifndef CAT
# define CATCAT(a, b, c) a ## b ## c
# define CAT(a, b) a ## b
#endif

#define csonpath_do_internal__(name) CATCAT(csonpath_, name, _internal)
#define csonpath_do_internal csonpath_do_internal__(CSONPATH_DO_FUNC_NAME)

static CSONPATH_DO_RET_TYPE csonpath_do_internal(const struct csonpath cjp[const static 1],
						 CSONPATH_JSON origin,
						 CSONPATH_JSON value,
						 CSONPATH_JSON ctx,
						 const char *walker CSONPATH_DO_EXTRA_DECLATION);


#define csonpath_do_dotdot__(name) CATCAT(csonpath_, name, _dotdot)
#define csonpath_do_dotdot csonpath_do_dotdot__(CSONPATH_DO_FUNC_NAME)


static CSONPATH_DO_RET_TYPE csonpath_do_dotdot(const struct csonpath cjp[const static 1],
					       CSONPATH_JSON origin,
					       CSONPATH_JSON tmp,
					       CSONPATH_JSON ctx,
					       const char *walker CSONPATH_DO_EXTRA_DECLATION)
{
    CSONPATH_JSON el;
    CSONPATH_DO_DECLARATION;
    const int is_obj = CSONPATH_IS_OBJ(tmp);
    intptr_t key_idx;
    CSONPATH_DO_RET_TYPE tret = CSONPATH_NONE_FOUND_RET;
    const char *next_inst = NULL;

    CSONPATH_DO_FIND_ALL_PRE_LOOP;
    if (is_obj) {
	const char *key;
	CSONPATH_FOREACH_OBJ(tmp, el, key) {
	    key_idx = (intptr_t)key;
	    (void)key_idx;
	    CSONPATH_DO_FOREACH_PRE_SET;
	    if (!strcmp(key, walker + 1)) {
		if (!next_inst)
		    next_inst = csonpath_walker_next_inst(walker);
		tret = csonpath_do_internal(cjp, origin, el, tmp,
					    next_inst
					    CSONPATH_DO_EXTRA_ARGS_NEESTED);
		CSONPATH_DO_FIND_ALL;
	    }
	    if (CSONPATH_IS_OBJ(el) || CSONPATH_IS_ARRAY(el)) {
		tret = csonpath_do_dotdot(cjp, origin, el, tmp, walker
					  CSONPATH_DO_EXTRA_ARGS_NEESTED);
		CSONPATH_DO_FIND_ALL;
	    }
	}
    } else {
	CSONPATH_FOREACH_ARRAY(tmp, el, key_idx) {
	    CSONPATH_DO_FOREACH_PRE_SET;
	    if (CSONPATH_IS_OBJ(el) || CSONPATH_IS_ARRAY(el)) {
		tret = csonpath_do_dotdot(cjp, origin, el, tmp, walker
					  CSONPATH_DO_EXTRA_ARGS_NEESTED);
		CSONPATH_DO_FIND_ALL;
	    }
	}
    }
    CSONPATH_DO_FIND_ALL_OUT;
    return tret;
}

static CSONPATH_DO_RET_TYPE csonpath_do_internal(const struct csonpath cjp[const static 1],
						 CSONPATH_JSON origin,
						 CSONPATH_JSON value,
						 CSONPATH_JSON ctx,
						 const char *walker CSONPATH_DO_EXTRA_DECLATION)
{
    CSONPATH_JSON tmp = value;
    CSONPATH_DO_DECLARATION;

    (void)ctx; /* maybe unused */

    while (*walker != CSONPATH_INST_END &&
	   *walker != CSONPATH_INST_OR) {
	switch (*walker) {
	case CSONPATH_INST_ROOT:
	{
	    CSONPATH_PRE_GET_ROOT
	    value = origin;
	    tmp = value;
	    ctx = CSONPATH_NULL;
	}
	break;
	case CSONPATH_INST_FILTER_KEY_EQ:
	case CSONPATH_INST_FILTER_KEY_REG_EQ:
	case CSONPATH_INST_FILTER_KEY_NOT_EQ:
	case CSONPATH_INST_FILTER_KEY_SUPERIOR:
	case CSONPATH_INST_FILTER_KEY_INFERIOR:
	{
	    CSONPATH_JSON el;
	    int operation_in = *walker, operation = *walker;
	    const char *owalker;
	    int filter_next_in = *(walker+1), filter_next = *(walker+1);
	    const char *next = walker + 2;
	    int foreach_idx;

	    (void) foreach_idx;
	    CSONPATH_DO_FILTER_PRE_LOOP;
	    CSONPATH_FOREACH_ARRAY(tmp, el, foreach_idx)
	    {
		intptr_t key_idx = foreach_idx;
		(void)key_idx;
		owalker = next;
	      match_and_or_or:
		if (CSONPATH_IS_OBJ(el)) {
		    CSONPATH_JSON el2 = el;

		    CSONPATH_DO_FILTER_LOOP_PRE_SET;
		    el2 = cosnpath_crawl_filter_el(cjp, &owalker, el2, filter_next);

		    if (csonpath_make_match(cjp, el2, &owalker, operation)) {
			if (*owalker == CSONPATH_INST_FILTER_AND) {
			    ++owalker; /* skip next */
			    operation = *owalker;
			    ++owalker;
			    filter_next = *owalker;
			    ++owalker;
			    goto match_and_or_or;
			}
			CSONPATH_DO_RET_TYPE tret =
			    csonpath_do_internal(cjp, origin, el, tmp, owalker
						 CSONPATH_DO_EXTRA_ARGS_NEESTED);
			CSONPATH_DO_FILTER_FIND;
		    } else {
			operation = operation_in;
			filter_next = filter_next_in;
		    }
		}
	    }
	    CSONPATH_DO_FILTER_OUT;
	    break;
	}
	case CSONPATH_INST_FIND_ALL:
	{
	    if (CSONPATH_IS_OBJ(tmp) || CSONPATH_IS_ARRAY(tmp)) {
		CSONPATH_DO_RET_TYPE tret =
		    csonpath_do_dotdot(cjp, origin, tmp, ctx,
				       walker CSONPATH_DO_EXTRA_ARGS_FIND_ALL);
		CSONPATH_DO_FIND_ALL_CLEAUP;
		return tret;
	    } else {
		return CSONPATH_NONE_FOUND_RET;
	    }
	}
	case CSONPATH_INST_RANGE:
	{
	    if (!CSONPATH_IS_ARRAY(tmp)) {
		return CSONPATH_NONE_FOUND_RET;
	    }

	    CSONPATH_JSON el;
	    intptr_t key_idx;
	    int getter_instruction;

	    (void)key_idx;
	    ++walker;
	    getter_instruction = walker[0];
	    ++walker;
	    unsigned int beg = csonpath_int_from_walker(getter_instruction, &walker);
	    getter_instruction = walker[0];
	    ++walker;
	    unsigned int end = csonpath_int_from_walker(getter_instruction, &walker);

	    if (!end)
		end = -1;

	    CSONPATH_DO_RANGE_PRE_LOOP;
	    CSONPATH_FOREACH_ARRAY(tmp, el, key_idx) {
		if (key_idx < beg) {
		    key_idx = beg - 1;
		    continue;
		}
		if (key_idx >= end)
		    break;
		CSONPATH_DO_FOREACH_PRE_SET
			CSONPATH_DO_RET_TYPE tret =
			csonpath_do_internal(cjp, origin, el, tmp, walker
					     CSONPATH_DO_EXTRA_ARGS_NEESTED);

		CSONPATH_DO_RANGE;
	    }

	    CSONPATH_DO_GET_ALL_OUT;
	    break;
	}
	case CSONPATH_INST_GET_ALL:
	{
	    CSONPATH_JSON el;

	    CSONPATH_DO_FIND_ALL_PRE_LOOP;
	    CSONPATH_FOREACH(tmp, el, {
		    CSONPATH_DO_FOREACH_PRE_SET
			CSONPATH_DO_RET_TYPE tret =
			csonpath_do_internal(cjp, origin, el, tmp,
					     walker + 1
					     CSONPATH_DO_EXTRA_ARGS_NEESTED);

		    CSONPATH_DO_FIND_ALL;
		});

	    CSONPATH_DO_GET_ALL_OUT;
	    break;
	}
	case CSONPATH_INST_GET_OBJ:
	{
	    CSONPATH_UNUSED const char *this_idx = &walker[1];

	    ctx = tmp;
	    CSONPATH_PRE_GET_OBJ(this_idx);
	    ++walker;
	    tmp = CSONPATH_GET(tmp, walker);
	    walker += strlen(walker);
	    if (tmp == CSONPATH_NULL) {
		CSONPATH_DO_GET_NOTFOUND(this_idx);
	    }
	    CSONPATH_DO_POST_FIND_OBJ;
	    break;
	}
	case CSONPATH_INST_GET_ARRAY_SMALL:
	{
	    int this_idx;

	    ctx = tmp;
	    this_idx =  (int)walker[1];
	    CSONPATH_PRE_GET(this_idx);
	    tmp = CSONPATH_AT(tmp, this_idx);
	    if (tmp == CSONPATH_NULL) {
		CSONPATH_DO_GET_NOTFOUND(this_idx);
	    }
	    walker += 1;
	    CSONPATH_DO_POST_FIND_ARRAY;
	    break;
	}
	case CSONPATH_INST_GET_ARRAY_BIG:
	    ctx = tmp;
	    {
		CSONPATH_UNUSED int this_idx;
		union {int n; char c[4];} to_num =
		    { .c= { walker[1], walker[2], walker[3], walker[4] } };
		this_idx = to_num.n;
		CSONPATH_PRE_GET(this_idx);
		tmp = CSONPATH_AT(tmp, to_num.n);
		if (tmp == CSONPATH_NULL) {
		    CSONPATH_DO_GET_NOTFOUND(this_idx);
		}
		walker += 5;
		CSONPATH_DO_POST_FIND_ARRAY;
		break;
	    }
	default:
	    CSONPATH_GETTER_ERR("unimplemented %d (%s) at idx %d\n", *walker,
				*walker <= CSONPATH_INST_BROKEN ?
				csonpath_instuction_str[(int)*walker] : "(unknow)",
				(int)(intptr_t)(walker - cjp->data));
	}
      next_inst:
	++walker;
    }
    CSONPATH_DO_RETURN;
}

#define csonpath_do__(name) CAT(csonpath_, name)
#define csonpath_do_ csonpath_do__(CSONPATH_DO_FUNC_NAME)

static CSONPATH_DO_RET_TYPE csonpath_do_(struct csonpath cjp[static 1],
					 CSONPATH_JSON value CSONPATH_DO_EXTRA_ARGS)
{
    char *walker = cjp->data;
    CSONPATH_JSON ctx = CSONPATH_NULL;

    (void)ctx;
    if (*walker == CSONPATH_INST_BROKEN) {
	CSONPATH_GETTER_ERR("fail to compile: %s\n", cjp->compile_error ?
			    cjp->compile_error : "(unknow error)");
	return CSONPATH_NONE_FOUND_RET;
    }

    CSONPATH_DO_PRE_OPERATION;

    CSONPATH_DO_RET_TYPE ret =  csonpath_do_internal(cjp, value, value,
						     CSONPATH_NULL,
						     walker CSONPATH_DO_EXTRA_ARGS_IN);

    CSONPATH_DO_POST_OPERATION;

	return ret;
}

#ifndef CSONPATH_NO_UNDEF

#undef CSONPATH_PRE_GET_ROOT
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
#undef CSONPATH_DO_EXTRA_ARGS_FIND_ALL
#undef CSONPATH_DO_EXTRA_ARGS
#undef CSONPATH_DO_EXTRA_DECLATION
#undef CSONPATH_DO_FIND_ALL_PRE_LOOP
#undef CSONPATH_DO_FOREACH_PRE_SET
#undef CSONPATH_DO_GET_ALL_OUT
#undef CSONPATH_DO_GET_NOTFOUND
#undef CSONPATH_DO_FIND_ALL_CLEAUP
#undef CSONPATH_DO_FILTER_OUT
#undef CSONPATH_DO_FILTER_PRE_LOOP
#undef CSONPATH_DO_FILTER_FIND
#undef CSONPATH_DO_FILTER_LOOP_PRE_SET
#undef CSONPATH_DO_RANGE
#undef CSONPATH_DO_RANGE_PRE_LOOP
#undef CSONPATH_PRE_GET_OBJ

#endif
