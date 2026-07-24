#ifndef CSONPATH_MY_FUZZING_H_
#define CSONPATH_MY_FUZZING_H_

#if !defined(CSONPATH_JSON)
# error "include a csonpath backend before csonpath_my_fuzzing.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef CSONPATH_CNT_OF
#define CSONPATH_CNT_OF(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#define CSONPATH_FUZZER_MAX_ACTION 3

enum csonpath_fuzzer_action {
	CSONPATH_FUZZER_MODIFY_STR,
	CSONPATH_FUZZER_MODIFY_CNT_TYPE,
	CSONPATH_FUZZER_MODIFY_STR_TYPE,
	CSONPATH_FUZZER_NO_ACTION_LEFT
};

struct csonpath_fuzz_parent_frame {
	CSONPATH_JSON container;
	int idx;
	const char *inst;
};

struct csonpath_fuzzer {
	struct csonpath **paths;
	int path_count;
	unsigned int seed;
	unsigned int rng;

	int cur_action;
	const char *cur_walker;
	int cur_path_idx;

	CSONPATH_JSON seen;  /* array of already-mutated targets */

	CSONPATH_JSON snapshot;
	CSONPATH_JSON parent;
	CSONPATH_JSON obj;
	struct csonpath_child_info saved_child_info;

	struct csonpath_fuzz_parent_frame parent_stack[1024];
	int parent_stack_len;
	int dry_run;
};

static int csonpath_fuzz_parent_push(struct csonpath_fuzzer *f,
				     CSONPATH_JSON container,
				     const char *inst)
{
	if (f->parent_stack_len >= 1024)
		return -1;
	CSONPATH_INREF(container);
	f->parent_stack[f->parent_stack_len++] =
		(struct csonpath_fuzz_parent_frame){
			.container = container,
			.idx = -1,
			.inst = inst,
		};
	return 0;
}

static void csonpath_fuzz_parent_pop(struct csonpath_fuzzer *f)
{
	if (f->parent_stack_len > 0) {
		--f->parent_stack_len;
		CSONPATH_REMOVE(f->parent_stack[f->parent_stack_len].container);
	}
}

static const char *csonpath_fuzz_strings[] = {
	"", "a", "fuzz", "42", "true", "false", "null", " ", "\n", "x"
};

static const long csonpath_fuzz_ints[] = {
	0, 1, -1, 42, 2147483647L, -2147483648L
};

static unsigned int csonpath_fuzz_rand(struct csonpath_fuzzer *f)
{
	f->rng = f->rng * 1103515245U + 12345U;
	return f->rng;
}

static CSONPATH_JSON csonpath_fuzz_array_to_obj(CSONPATH_JSON arr)
{
	CSONPATH_JSON ret = CSONPATH_NEW_OBJECT();
	if (ret == CSONPATH_NULL)
		return CSONPATH_NULL;
	CSONPATH_JSON el;
	CSONPATH_FOREACH_ARRAY(arr, el, idx) {
		char key[32];
		snprintf(key, sizeof(key), "%lld", (long long)idx);
		CSONPATH_APPEND_AT(ret, key, el, 1);
	}
	return ret;
}

static CSONPATH_JSON csonpath_fuzz_obj_to_array(CSONPATH_JSON obj)
{
	CSONPATH_JSON ret = CSONPATH_NEW_ARRAY();
	if (ret == CSONPATH_NULL)
		return CSONPATH_NULL;
	CSONPATH_JSON el;
	const char *key;
	CSONPATH_FOREACH_OBJ(obj, el, key) {
		(void)key;
		CSONPATH_ARRAY_APPEND(ret, el);
	}
	return ret;
}

static struct csonpath_fuzzer *csonpath_fuzzer_new(CSONPATH_JSON path_list,
					   unsigned int seed,
					   CSONPATH_JSON obj)
{
	if (!CSONPATH_IS_ARRAY(path_list))
		return NULL;

	size_t n = CSONPATH_ARRAY_LENGTH(path_list);

	if (!n)
		return NULL;
	struct csonpath_fuzzer *f = malloc(sizeof *f);
	if (!f)
		return NULL;
	*f = (struct csonpath_fuzzer){
		.seed=seed,
		.rng=seed,
		.cur_action=-1,
		.cur_path_idx=0,
		.snapshot=CSONPATH_NULL,
		.parent=obj,
		.obj=obj,
		.seen=CSONPATH_NEW_ARRAY(),
		.saved_child_info={.type=-1},
		.parent_stack_len=0,
		.dry_run=0,
	};

	if (f->seen == CSONPATH_NULL)
		goto err_f;

	/*
	 * Own a reference to the user-supplied root object so the fuzzer
	 * survives the caller's frame. parent/obj are just cursors into this
	 * owned graph and must not be released separately.
	 */
	CSONPATH_INREF(obj);

	f->paths = malloc(sizeof *f->paths * n);
	if (!f->paths)
		goto err_obj;

	for (size_t i = 0; i < n; ++i) {
		CSONPATH_JSON p = CSONPATH_AT(path_list, i);
		if (!CSONPATH_IS_STR(p))
			continue;
		const char *s = CSONPATH_GET_STR(p);
		f->paths[f->path_count] = csonpath_new(s);
		if (f->paths[f->path_count])
			++f->path_count;
	}

	if (!f->path_count)
		goto err_paths;

	f->cur_walker = csonpath_walker_next_inst(f->paths[0]->data);
	return f;

 err_paths:
	free(f->paths);
 err_obj:
	CSONPATH_REMOVE(f->obj);
 err_f:
	CSONPATH_REMOVE(f->seen);
	free(f);
	return NULL;
}

static void csonpath_fuzzer_destroy(struct csonpath_fuzzer *f)
{
	if (!f)
		return;
	for (int i = 0; i < f->path_count; ++i) {
		if (f->paths[i])
			csonpath_destroy(f->paths[i]);
	}
	free(f->paths);

	/* parent_stack frames own references to their containers */
	while (f->parent_stack_len > 0)
		csonpath_fuzz_parent_pop(f);

	CSONPATH_REMOVE(f->seen);
	CSONPATH_REMOVE(f->snapshot);
	CSONPATH_REMOVE(f->obj);
	free(f);
}

static CSONPATH_JSON csonpath_fuzz_current(struct csonpath_fuzzer *f)
{
	if (f->saved_child_info.type == CSONPATH_STR)
		return CSONPATH_GET(f->parent, f->saved_child_info.key);
	if (f->saved_child_info.type == CSONPATH_INTEGER)
		return CSONPATH_AT(f->parent, f->saved_child_info.idx);
	return CSONPATH_NULL;
}

static int csonpath_fuzz_is_seen(struct csonpath_fuzzer *f, CSONPATH_JSON target)
{
	int n = (int)CSONPATH_ARRAY_LENGTH(f->seen);
	for (int i = 0; i < n; ++i) {
		if (CSONPATH_AT(f->seen, i) == target)
			return 1;
	}
	return 0;
}

static int csonpath_fuzzer_mark_seen(struct csonpath_fuzzer *f, CSONPATH_JSON target)
{
	if (target == CSONPATH_NULL)
		return 0;
	if (csonpath_fuzz_is_seen(f, target))
		return 0;
	int seen_len = (int)CSONPATH_ARRAY_LENGTH(f->seen);
	CSONPATH_INREF(target);
	CSONPATH_APPEND_AT(f->seen, seen_len, target, 1);
	return 0;
}

/* look at current element, find the next applicable action, apply it and
 * return it. If nothing is applicable, reset cur_action and return
 * CSONPATH_FUZZER_NO_ACTION_LEFT so the caller can crawl to the next target.
 */
static enum csonpath_fuzzer_action csonpath_fuzz_apply_action(struct csonpath_fuzzer *f)
{
	CSONPATH_JSON cur = csonpath_fuzz_current(f);

	while (f->cur_action < CSONPATH_FUZZER_MAX_ACTION - 1) {
		++f->cur_action;
		enum csonpath_fuzzer_action action = f->cur_action;
		CSONPATH_JSON new_val = CSONPATH_NULL;
		unsigned int r;

		switch (action) {
		case CSONPATH_FUZZER_MODIFY_STR:
			if (!CSONPATH_IS_STR(cur))
				continue;
			r = csonpath_fuzz_rand(f);
			new_val = CSONPATH_NEW_STR(
				csonpath_fuzz_strings[r % CSONPATH_CNT_OF(csonpath_fuzz_strings)]
			);
			break;
		case CSONPATH_FUZZER_MODIFY_STR_TYPE:
			if (!CSONPATH_IS_STR(cur))
				continue;
			r = csonpath_fuzz_rand(f);
			{
				int kind = (int)(r % 3U);
				if (kind == 0)
					new_val = CSONPATH_NEW_INT(
						csonpath_fuzz_ints[r % CSONPATH_CNT_OF(csonpath_fuzz_ints)]
					);
				else if (kind == 1)
					new_val = CSONPATH_NEW_BOOL(r & 1U);
				else
					new_val = CSONPATH_NULL;
			}
			break;
		case CSONPATH_FUZZER_MODIFY_CNT_TYPE:
			if (!CSONPATH_IS_ARRAY(cur) && !CSONPATH_IS_OBJ(cur))
				continue;
			(void)csonpath_fuzz_rand(f);
			if (CSONPATH_IS_ARRAY(cur))
				new_val = csonpath_fuzz_array_to_obj(cur);
			else
				new_val = csonpath_fuzz_obj_to_array(cur);
			break;
		default:
			continue;
		}

		if (!f->dry_run) {
			if (!CSONPATH_IS_NULL(cur)) {
				f->snapshot = cur;
				CSONPATH_INREF(f->snapshot);
			}

			if (f->saved_child_info.type == CSONPATH_INTEGER)
				CSONPATH_APPEND_AT(f->parent, f->saved_child_info.idx, new_val, 1);
			else
				CSONPATH_APPEND_AT(f->parent, f->saved_child_info.key, new_val, 1);
		}
		if (new_val != CSONPATH_NULL)
			CSONPATH_REMOVE(new_val);

		return action;
	}

	f->cur_action = -1;
	return CSONPATH_FUZZER_NO_ACTION_LEFT;
}

static enum csonpath_fuzzer_action csonpath_fuzz_step(struct csonpath_fuzzer *f)
{
	if (!f->dry_run && f->snapshot != CSONPATH_NULL) {
		if (f->saved_child_info.type == CSONPATH_INTEGER)
			CSONPATH_APPEND_AT(f->parent, f->saved_child_info.idx,
					   f->snapshot, 1);
		else
			CSONPATH_APPEND_AT(f->parent, f->saved_child_info.key,
					   f->snapshot, 1);
		CSONPATH_REMOVE(f->snapshot);
		f->snapshot = CSONPATH_NULL;
	}

	if (f->cur_path_idx >= f->path_count)
		return CSONPATH_FUZZER_NO_ACTION_LEFT;

do_action: {
	enum csonpath_fuzzer_action a = csonpath_fuzz_apply_action(f);
	if (a == CSONPATH_FUZZER_NO_ACTION_LEFT)
		goto next_inst;
	return a;
}

	/* crawl time */
next_inst:
	;
	const char *walker = f->cur_walker;
	f->cur_walker = csonpath_walker_next_inst(f->cur_walker);

	if (f->saved_child_info.type != -1) {
		CSONPATH_JSON cur = csonpath_fuzz_current(f);
		if (cur == CSONPATH_NULL)
			goto next_path;
		f->parent = cur;
	}

	CSONPATH_JSON new_cur = CSONPATH_NULL;

	switch (*walker) {
	case CSONPATH_INST_GET_OBJ: {
		const char *key = walker + 1;
		f->saved_child_info.type = CSONPATH_STR;
		f->saved_child_info.key = key;
		new_cur = CSONPATH_GET(f->parent, key);
		break;
	}
	case CSONPATH_INST_GET_ARRAY_SMALL:
	case CSONPATH_INST_GET_ARRAY_BIG: {
		const char *tmp = walker + 1;
		int idx = csonpath_int_from_walker(*walker, &tmp);
		if (idx < 0)
			goto next_path;
		f->saved_child_info.type = CSONPATH_INTEGER;
		f->saved_child_info.idx = idx;
		new_cur = CSONPATH_AT(f->parent, idx);
		break;
	}
	case CSONPATH_INST_GET_ALL: {
		struct csonpath_fuzz_parent_frame *frame;

		if (f->parent_stack_len == 0 ||
		    f->parent_stack[f->parent_stack_len - 1].inst != walker) {
			if (csonpath_fuzz_parent_push(f, f->parent, walker) < 0)
				return CSONPATH_FUZZER_NO_ACTION_LEFT;
		}

		frame = &f->parent_stack[f->parent_stack_len - 1];
		++frame->idx;

		{
			int i = 0;
			CSONPATH_JSON el;
			CSONPATH_FOREACH_EXT(frame->container, el, {
				if (i == frame->idx) {
					csonpath_child_info_set(
						&f->saved_child_info,
						frame->container,
						(intptr_t)key_idx);
					new_cur = el;
					break;
				}
				++i;
			}, key_idx);
		}

		if (new_cur == CSONPATH_NULL) {
			csonpath_fuzz_parent_pop(f);
			goto next_path;
		}
		break;
	}
	case CSONPATH_INST_END:
	case CSONPATH_INST_OR:
	case CSONPATH_INST_BROKEN:
	default:
		goto next_path;
	}

	if (new_cur == CSONPATH_NULL)
		goto next_path;
	if (csonpath_fuzz_is_seen(f, new_cur))
		goto next_inst;
	{
		int seen_len = (int)CSONPATH_ARRAY_LENGTH(f->seen);
		CSONPATH_APPEND_AT(f->seen, seen_len, new_cur, 1);
	}
	goto do_action;

next_path:
	if (f->parent_stack_len > 0) {
		struct csonpath_fuzz_parent_frame *frame =
			&f->parent_stack[f->parent_stack_len - 1];
		f->parent = frame->container;
		f->saved_child_info.type = -1;
		f->cur_action = -1;
		f->cur_walker = frame->inst;
		goto next_inst;
	}

	++f->cur_path_idx;
	if (f->cur_path_idx >= f->path_count)
		return CSONPATH_FUZZER_NO_ACTION_LEFT;
	f->parent = f->obj;
	f->saved_child_info.type = -1;
	f->cur_walker = csonpath_walker_next_inst(
		f->paths[f->cur_path_idx]->data);
	goto next_inst;
}

#endif
