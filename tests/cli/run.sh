#!/usr/bin/env bash
set -euo pipefail

CLI="${1:-./csonpath}"
TMP=$(mktemp)
trap 'rm -f "$TMP"' EXIT

echo "== get first match =="
test "$(echo '{"a":"value"}' | "$CLI" -o json '$.a')" = '"value"'

echo "== get missing key =="
if echo '{"a":"value"}' | "$CLI" '$.missing'; then
    echo "expected failure"
    exit 1
fi

echo "== raw output =="
test "$(echo '{"a":"hello","n":42}' | "$CLI" -o raw '$.a')" = 'hello'
test "$(echo '{"a":"hello","n":42}' | "$CLI" -o raw '$.n')" = '42'

echo "== pretty output =="
echo '{"a":{"b":1}}' | "$CLI" -p '$.a' | grep -q '^{'

echo "== find all =="
test "$(echo '{"items":[{"x":1},{"x":2}]}' | "$CLI" -a -o json '$.items[*].x')" = '[1,2]'

echo "== find lines =="
out=$(echo '{"items":[{"x":1},{"x":2}]}' | "$CLI" -a -o lines '$.items[*].x')
test "$out" = $'1\n2'

echo "== find empty =="
if echo '{"items":[]}' | "$CLI" -a '$.items[*].x'; then
    echo "expected failure"
    exit 1
fi

echo "== find empty with -e =="
test "$(echo '{"items":[]}' | "$CLI" -a -e -o json '$.items[*].x')" = '[]'

echo "== set with --set =="
test "$(echo '{"a":1}' | "$CLI" -o json --set '42' '$.b')" = '{"a":1,"b":42}'

echo "== set positional value =="
test "$(echo '{"a":1}' | "$CLI" -o json '$.b' '42')" = '{"a":1,"b":42}'

echo "== set raw =="
test "$(echo '{"a":1}' | "$CLI" -o json -r --set 'hello' '$.b')" = '{"a":1,"b":"hello"}'

echo "== set invalid json without -r fails =="
if echo '{"a":1}' | "$CLI" --set 'hello' '$.b'; then
    echo "expected failure"
    exit 1
fi

echo "== set creates parents =="
test "$(echo '{"a":1}' | "$CLI" -o json --set '[]' '$.x.y.z')" = '{"a":1,"x":{"y":{"z":[]}}}'

echo "== remove =="
test "$(echo '{"a":1,"b":2}' | "$CLI" -o json -d '$.b')" = '{"a":1}'

echo "== remove strict =="
if echo '{"a":1}' | "$CLI" -d --strict '$.missing'; then
    echo "expected failure"
    exit 1
fi

echo "== in-place set =="
echo '{"a":1}' > "$TMP"
"$CLI" -f "$TMP" -i -o json --set '2' '$.b'
test "$(cat "$TMP")" = '{"a":1,"b":2}'

echo "== in-place remove =="
echo '{"a":1,"b":2}' > "$TMP"
"$CLI" -f "$TMP" -i -o json -d '$.b'
test "$(cat "$TMP")" = '{"a":1}'

echo "== jq-like path =="
test "$(echo '{"a":{"b":1}}' | "$CLI" -j -o json '.a.b')" = '1'

if echo '{"a":{"b":1}}' | "$CLI" -j 'a.b'; then
    echo "expected failure"
    exit 1
fi

echo "== invalid json =="
if echo 'not json' | "$CLI" '$.a'; then
    echo "expected failure"
    exit 1
fi

echo "== invalid path =="
if echo '{"a":1}' | "$CLI" '$$$.'; then
    echo "expected failure"
    exit 1
fi

echo "== version =="
"$CLI" --version | grep -q '^csonpath '

echo "== help =="
"$CLI" --help | grep -q 'usage:'

echo "== file input =="
echo '{"a":1}' > "$TMP"
test "$("$CLI" -f "$TMP" -o json '$.a')" = '1'

echo "== string input =="
test "$("$CLI" -s '{"a":1}' -o json '$.a')" = '1'

echo "== file and string mutually exclusive =="
if "$CLI" -f "$TMP" -s '{"a":1}' '$.a'; then
    echo "expected failure"
    exit 1
fi

echo "== raw without value fails =="
if echo '{"a":1}' | "$CLI" -r '$.a'; then
    echo "expected failure"
    exit 1
fi

echo "== strict without delete fails =="
if echo '{"a":1}' | "$CLI" --strict '$.a'; then
    echo "expected failure"
    exit 1
fi

echo "== in-place without file fails =="
if echo '{"a":1}' | "$CLI" -i '$.a'; then
    echo "expected failure"
    exit 1
fi

echo "== in-place with stdin dash fails =="
if echo '{"a":1}' | "$CLI" -f - -i '$.a'; then
    echo "expected failure"
    exit 1
fi

echo "== output json explicit =="
test "$(echo '{"a":1}' | "$CLI" -o json '$.a')" = '1'

echo "== pretty exact =="
test "$(echo '{"a":{"b":1}}' | "$CLI" -p '$.a')" = $'{\n  "b": 1\n}'

echo "== lines on empty with -e =="
test "$(echo '{"items":[]}' | "$CLI" -a -e -o lines '$.items[*].x')" = ''

echo "== delete no match non-strict =="
test "$(echo '{"a":1}' | "$CLI" -o json -d '$.missing')" = '{"a":1}'

echo "== delete strict with match =="
test "$(echo '{"a":1,"b":2}' | "$CLI" -o json -d --strict '$.b')" = '{"a":1}'

echo "== set null =="
test "$(echo '{"a":1}' | "$CLI" -o json --set 'null' '$.b')" = '{"a":1,"b":null}'

echo "== set bool =="
test "$(echo '{"a":1}' | "$CLI" -o json --set 'true' '$.b')" = '{"a":1,"b":true}'

echo "== set array =="
test "$(echo '{"a":1}' | "$CLI" -o json --set '[1,2]' '$.b')" = '{"a":1,"b":[1,2]}'

echo "== set object =="
test "$(echo '{"a":1}' | "$CLI" -o json --set '{"x":1}' '$.b')" = '{"a":1,"b":{"x":1}}'

echo "== set update existing =="
test "$(echo '{"a":1}' | "$CLI" -o json --set '2' '$.a')" = '{"a":2}'

echo "== set raw empty =="
test "$(echo '{"a":1}' | "$CLI" -o json -r --set '' '$.b')" = '{"a":1,"b":""}'

echo "== root path =="
test "$(echo '{"a":1}' | "$CLI" -o json '$')" = '{"a":1}'

echo "== unicode =="
test "$(echo '{"a":"éè"}' | "$CLI" -o json '$.a')" = '"éè"'

echo "== file not found =="
if "$CLI" -f /nonexistent '$.a'; then
    echo "expected failure"
    exit 1
fi

echo "== all CLI tests passed =="
