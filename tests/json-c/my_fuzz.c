#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "csonpath_json-c.h"
#include "csonpath_my_fuzzing.h"

const char *json_str = "{"
  "\"mydict\": {\"str\": \"hello\", \"num\": 42},"
  "\"items\": [\"a\", \"b\", \"c\"],"
  "\"root_str\": \"world\""
  "}";

int main(void)
{
	struct json_object *jobj = json_tokener_parse(json_str);
	struct json_object *paths = json_object_new_array();
	int changes = 0;
	int cnt_type_seen = 0;
	int no_action_left_seen = 0;

	json_object_array_add(paths, json_object_new_string("$.mydict.str"));
	json_object_array_add(paths, json_object_new_string("$.root_str"));

	struct csonpath_fuzzer *f = csonpath_fuzzer_new(paths, 12345U, jobj);
	assert(f);

	for (int i = 0; i < 60; ++i) {
		enum csonpath_fuzzer_action a = csonpath_fuzz_step(f);
		switch (a) {
		case CSONPATH_FUZZER_MODIFY_STR:
		case CSONPATH_FUZZER_MODIFY_CNT_TYPE:
		case CSONPATH_FUZZER_MODIFY_STR_TYPE:
			++changes;
			if (a == CSONPATH_FUZZER_MODIFY_CNT_TYPE)
				++cnt_type_seen;
			break;
		case CSONPATH_FUZZER_NO_ACTION_LEFT:
			++no_action_left_seen;
			break;
		default:
			break;
		}
	}

	assert(changes > 0);
	assert(cnt_type_seen == 1);
	assert(no_action_left_seen > 0);

	/* root_str must never have been mutated because obj was already seen */
	struct json_object *root_str = json_object_object_get(jobj, "root_str");
	assert(root_str);
	assert(json_object_is_type(root_str, json_type_string));
	assert(!strcmp(json_object_get_string(root_str), "world"));

	csonpath_fuzzer_destroy(f);
	json_object_put(paths);
	json_object_put(jobj);

	printf("OK (changes=%d, cnt=%d, no_action=%d, root_str unchanged)\n",
	       changes, cnt_type_seen, no_action_left_seen);
	return 0;
}
