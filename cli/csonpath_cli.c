/* csonpath CLI in C.
 *
 * Build:
 *     make csonpath
 *
 * Usage:
 *     csonpath [options] PATH [VALUE]
 *
 * Requires: json-c, csonpath_json-c.h, csonpath.h, csonpath_do.h
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>

#include "json.h"
#include "csonpath_json-c.h"

#define VERSION "0.17.0"

#define FLAG_JQ_LIKE     (1U << 0)
#define FLAG_RAW         (1U << 1)
#define FLAG_STRICT      (1U << 2)
#define FLAG_EMPTY_ARRAY (1U << 3)
#define FLAG_QUIET       (1U << 4)
#define FLAG_KEYS        (1U << 5)

#define HAS_FLAG(opts, f) (((opts)->flags & (f)) != 0)

#define AUTOFREE_JSON       __attribute__((cleanup(autofree_json)))
#define AUTOFREE_CSONPATH   __attribute__((cleanup(autofree_csonpath)))

static inline void autofree_json(struct json_object **p)
{
    json_object_put(*p);
}

static inline void autofree_csonpath(struct csonpath **p)
{
    csonpath_destroy(*p);
}

/* exit codes:
 *   0            success
 *   1            no match
 *   EINVAL       invalid input (usage, JSON, path, value)
 *   ENOMEM       out of memory
 *   errno        system errors (I/O)
 */

enum output_format {
    OUT_JSON,
    OUT_PRETTY,
    OUT_RAW,
    OUT_LINES,
};

struct opts {
    struct json_object *root;
    const char *out_file;   /* NULL = stdout, else write to this file */
    const char *path;
    const char *value;
    unsigned flags;
    enum output_format output;
    int (*action)(const struct opts *, struct csonpath *);
};

static void __attribute__((noreturn))
usage(const char *prog, int ok)
{
    FILE *out = ok ? stdout : stderr;
    fprintf(out,
        "usage: %s [options] PATH [VALUE]\n"
        "\n"
        "JSONPath swiss-army knife powered by csonpath.\n"
        "\n"
        "options:\n"
        "  -a, --all             Return all matches instead of the first one.\n"
        "  -d, --delete          Remove matches and print the modified JSON.\n"
        "      --set VALUE       Set PATH to VALUE (parsed as JSON).\n"
        "  -r, --raw             Treat VALUE as a raw string.\n"
        "  -i, --in-place        Edit FILE in place (requires --file).\n"
        "      --strict          Exit with an error if --delete removes nothing.\n"
        "  -f, --file FILE       Read JSON from FILE instead of stdin.\n"
        "  -s, --string JSON     Read JSON from a string.\n"
        "  -j, --jq-like         Allow jq-style paths without a leading '$'.\n"
        "  -p, --pretty          Pretty-print JSON output.\n"
        "  -o, --output FORMAT   Output format: pretty, json, raw, lines (default: pretty).\n"
        "  -e, --empty-array     Return [] instead of nothing when -a matches nothing.\n"
        "  -q, --quiet           Suppress normal output; only exit status matters.\n"
        "  -K, --keys            Print object keys (or array indices) of matches as a JSON array.\n"
        "                        Use -o raw to print one key per line.\n"
        "      --version         Show version and exit.\n"
        "  -h, --help            Show this help message and exit.\n",
        prog);
    exit(ok ? 0 : EINVAL);
}

#define EXIT_IF(cnd, code, args...)				\
    if (cnd) {						\
	fprintf(stderr, "csonpath: " args);			\
	exit(code);						\
    }

#define NO_MATCH 1

static struct json_object *
read_input(const char *file, const char *string)
{
    struct json_object *obj;

    if (string) {
        obj = json_tokener_parse(string);
    } else if (!file || strcmp(file, "-") == 0) {
        obj = json_object_from_fd(STDIN_FILENO);
    } else {
        obj = json_object_from_file(file);
    }

    EXIT_IF(!obj && errno, errno, "%s\n", strerror(errno));
    EXIT_IF(!obj, EINVAL, "invalid JSON\n");
    return obj;
}

static int
output_flags(enum output_format fmt)
{
    switch (fmt) {
    case OUT_PRETTY:
        return JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_SPACED;
    case OUT_JSON:
    case OUT_LINES:
    default:
        return JSON_C_TO_STRING_PLAIN;
    }
}

static void
print_json_raw(struct json_object *obj)
{
    switch (json_object_get_type(obj)) {
    case json_type_string:
        printf("%s", json_object_get_string(obj));
        break;
    case json_type_int:
        printf("%d", json_object_get_int(obj));
        break;
    case json_type_boolean:
        printf("%s", json_object_get_boolean(obj) ? "true" : "false");
        break;
    case json_type_double:
        printf("%f", json_object_get_double(obj));
        break;
    case json_type_null:
        printf("null");
        break;
    default:
        printf("%s", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PLAIN));
        break;
    }
}

static void
print_array(struct json_object *arr, enum output_format fmt)
{
    if (fmt == OUT_LINES) {
        int n = json_object_array_length(arr);
        for (int i = 0; i < n; ++i) {
            struct json_object *el = json_object_array_get_idx(arr, i);
            printf("%s\n", json_object_to_json_string_ext(el, JSON_C_TO_STRING_PLAIN));
        }
    } else if (fmt == OUT_RAW) {
        int n = json_object_array_length(arr);
        for (int i = 0; i < n; ++i) {
            print_json_raw(json_object_array_get_idx(arr, i));
            putchar('\n');
        }
    } else {
        printf("%s\n", json_object_to_json_string_ext(arr, output_flags(fmt)));
    }
}

static int
write_output(const struct opts *opts, const char *text)
{
    FILE *fh = stdout;

    if (opts->out_file) {
	fh = fopen(opts->out_file, "w");
	EXIT_IF(!fh, errno, "%s\n", strerror(errno));
    }
    fprintf(fh, "%s\n", text);
    if (opts->out_file)
	fclose(fh);
    return 0;
}

static inline int
serialize_and_write(const struct opts *opts, struct json_object *obj)
{
    return write_output(opts, json_object_to_json_string_ext(obj, output_flags(opts->output)));
}

static int
cmd_get(const struct opts *opts, struct csonpath *p)
{
    struct json_object *ret = csonpath_find_first(p, opts->root);
    if (!ret)
        return NO_MATCH;
    if (!HAS_FLAG(opts, FLAG_QUIET)) {
        if (opts->output == OUT_RAW) {
            print_json_raw(ret);
            putchar('\n');
        } else {
            printf("%s\n", json_object_to_json_string_ext(ret, output_flags(opts->output)));
        }
    }
    return 0;
}

static int
cmd_find(const struct opts *opts, struct csonpath *p)
{
    struct json_object *ret = csonpath_find_all(p, opts->root);
    if (!ret) {
        if (HAS_FLAG(opts, FLAG_EMPTY_ARRAY)) {
            if (!HAS_FLAG(opts, FLAG_QUIET)) {
                struct json_object *empty = json_object_new_array();
                print_array(empty, opts->output);
                json_object_put(empty);
            }
            return 0;
        }
        return NO_MATCH;
    }
    if (!HAS_FLAG(opts, FLAG_QUIET))
        print_array(ret, opts->output);
    return 0;
}

static int
cmd_rm(const struct opts *opts, struct csonpath *p)
{
    int n = csonpath_remove(p, opts->root);
    if (n == 0 && HAS_FLAG(opts, FLAG_STRICT))
        return NO_MATCH;
    if (!HAS_FLAG(opts, FLAG_QUIET))
        serialize_and_write(opts, opts->root);
    return 0;
}

static struct json_object *
parse_value(const char *text, int raw)
{
    if (raw)
        return json_object_new_string(text);

    struct json_object *obj = json_tokener_parse(text);
    if (!obj && strcmp(text, "null") == 0)
        return json_object_new_null();
    EXIT_IF(!obj, EINVAL, "invalid JSON value\n");
    return obj;
}

static int
cmd_set(const struct opts *opts, struct csonpath *p)
{
    AUTOFREE_JSON struct json_object *val = parse_value(opts->value, HAS_FLAG(opts, FLAG_RAW));
    csonpath_update_or_create(p, opts->root, val);
    if (!HAS_FLAG(opts, FLAG_QUIET))
        serialize_and_write(opts, opts->root);
    return 0;
}

struct keys_cb_data {
    const struct opts *opts;
    struct json_object *keys;
    int count;
};

static void
keys_callback(json_object *parent, struct csonpath_child_info *child_info,
              json_object *current, void *udata)
{
    struct keys_cb_data *d = udata;

    (void)parent;
    (void)child_info;
    if (json_object_is_type(current, json_type_object)) {
        json_object_object_foreach(current, key, val) {
            (void)val;
            json_object_array_add(d->keys, json_object_new_string(key));
            ++d->count;
        }
    } else if (json_object_is_type(current, json_type_array)) {
        int len = json_object_array_length(current);
        for (int i = 0; i < len; ++i) {
            json_object_array_add(d->keys, json_object_new_int(i));
            ++d->count;
        }
    }
}

static int
cmd_keys(const struct opts *opts, struct csonpath *p)
{
    struct keys_cb_data d = { .opts = opts, .keys = json_object_new_array(), .count = 0 };
    int ret = csonpath_callback(p, opts->root, keys_callback, &d);

    if (ret < 0) {
        json_object_put(d.keys);
        return EINVAL;
    }
    if (d.count == 0) {
        json_object_put(d.keys);
        return NO_MATCH;
    }

    if (!HAS_FLAG(opts, FLAG_QUIET))
        print_array(d.keys, opts->output);

    json_object_put(d.keys);
    return 0;
}

static int
parse_output_format(const char *s)
{
    if (strcmp(s, "json") == 0)
        return OUT_JSON;
    if (strcmp(s, "pretty") == 0)
        return OUT_PRETTY;
    if (strcmp(s, "raw") == 0)
        return OUT_RAW;
    if (strcmp(s, "lines") == 0)
        return OUT_LINES;
    return -1;
}

static int
parse_args(int argc, char **argv, struct opts *opts,
           const char **file, const char **string, int *in_place)
{
    static const struct option longopts[] = {
        {"file", required_argument, 0, 'f'},
        {"string", required_argument, 0, 's'},
        {"jq-like", no_argument, 0, 'j'},
        {"pretty", no_argument, 0, 'p'},
        {"output", required_argument, 0, 'o'},
        {"empty-array", no_argument, 0, 'e'},
        {"quiet", no_argument, 0, 'q'},
        {"keys", no_argument, 0, 'K'},
        {"all", no_argument, 0, 'a'},
        {"delete", no_argument, 0, 'd'},
        {"set", required_argument, 0, 'S'},
        {"raw", no_argument, 0, 'r'},
        {"in-place", no_argument, 0, 'i'},
        {"strict", no_argument, 0, 'T'},
        {"version", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0},
    };

    int c;
    while ((c = getopt_long(argc, argv, "f:s:jpo:eaqKdirvh", longopts, NULL)) != -1) {
        switch (c) {
        case 'f':
            *file = optarg;
            break;
        case 's':
            *string = optarg;
            break;
        case 'j':
            opts->flags |= FLAG_JQ_LIKE;
            break;
        case 'p':
            opts->output = OUT_PRETTY;
            break;
        case 'o':
            {
                int fmt = parse_output_format(optarg);
                if (fmt < 0)
                    return -1;
                opts->output = fmt;
            }
            break;
        case 'e':
            opts->flags |= FLAG_EMPTY_ARRAY;
            break;
        case 'q':
            opts->flags |= FLAG_QUIET;
            break;
        case 'K':
            if (opts->action && opts->action != cmd_keys)
                return -1;
            opts->action = cmd_keys;
            break;
        case 'a':
            if (opts->action && opts->action != cmd_find)
                return -1;
            opts->action = cmd_find;
            break;
        case 'd':
            if (opts->action && opts->action != cmd_rm)
                return -1;
            opts->action = cmd_rm;
            break;
        case 'S':
            if (opts->action && opts->action != cmd_set)
                return -1;
            opts->action = cmd_set;
            opts->value = optarg;
            break;
        case 'r':
            opts->flags |= FLAG_RAW;
            break;
        case 'i':
            *in_place = 1;
            break;
        case 'T':
            opts->flags |= FLAG_STRICT;
            break;
        case 'v':
            printf("csonpath %s\n", VERSION);
            exit(0);
        case 'h':
            usage(argv[0], 1);
        default:
            return -1;
        }
    }

    if (optind >= argc)
        return -1;
    opts->path = argv[optind++];

    if (optind < argc) {
        if (opts->action == cmd_set)
            return -1;
        opts->value = argv[optind++];
        opts->action = cmd_set;
    }

    if (optind < argc)
        return -1;

    if (!opts->action)
        opts->action = cmd_get;

    return 0;
}

static void
validate_args(const struct opts *opts, const char *file, const char *string,
              int in_place)
{
    EXIT_IF(string && file, EINVAL, "--file and --string are mutually exclusive\n");
    EXIT_IF(in_place && (!file || strcmp(file, "-") == 0), EINVAL,
            "--in-place requires --file\n");
    EXIT_IF(HAS_FLAG(opts, FLAG_RAW) && opts->action != cmd_set, EINVAL,
            "--raw requires a value to set\n");
    EXIT_IF(HAS_FLAG(opts, FLAG_STRICT) && opts->action != cmd_rm, EINVAL,
            "--strict requires --delete\n");
}

int
main(int argc, char **argv)
{
    const char *file = NULL;
    const char *string = NULL;
    int in_place = 0;

    struct opts opts = (struct opts){ .output = OUT_PRETTY };
    if (parse_args(argc, argv, &opts, &file, &string, &in_place) < 0)
        usage(argv[0], 0);

    validate_args(&opts, file, string, in_place);

    opts.root = read_input(file, string);
    opts.out_file = in_place ? file : NULL;

    int flags = HAS_FLAG(&opts, FLAG_JQ_LIKE) ? CSONPATH_AUTO_ROOT : 0;
    AUTOFREE_CSONPATH struct csonpath *p = csonpath_new_ex(opts.path, flags);
    EXIT_IF(!p, EINVAL, "invalid path\n");

    int rc = opts.action(&opts, p);
    json_object_put(opts.root);
    return rc;
}
