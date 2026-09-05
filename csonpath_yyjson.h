#ifndef CSONPATH_YYJSON_H_
#define CSONPATH_YYJSON_H_

/* Aggregator header: includes both the immutable (const) and mutable
 * yyjson backends with distinct prefixes so they can coexist in the same
 * translation unit. */

#include "yyjson.h"

/* Multi-backend mode: csonpath.h will keep extra_roots backend-agnostic
 * (void *) so both const and mut yyjson backends can share struct csonpath.
 * The immutable backend uses the yyjson_ prefix; the mutable one uses
 * yyjson_mut_ by default. */
#define CSONPATH_USE_PREFIX

#include "csonpath_yyjson_mut.h"
#include "csonpath_yyjson_const.h"

#endif
