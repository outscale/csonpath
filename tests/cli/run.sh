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

echo "== type selector string =="
test "$(echo '{"a":"hi","b":1,"c":null}' | "$CLI" -a -o json '$.*@string()')" = '["hi"]'

echo "== type selector integer =="
test "$(echo '{"a":"hi","b":1,"c":null}' | "$CLI" -a -o json '$.*@integer()')" = '[1]'

echo "== type selector null =="
test "$(echo '{"a":"hi","b":1,"c":null}' | "$CLI" -a -o json '$.*@null()')" = '[null]'

echo "== type selector bracket key =="
test "$(echo '{"@odata.id":"foo"}' | "$CLI" -o json '$["@odata.id"]@string()')" = '"foo"'

echo "== type selector no match =="
if echo '{"a":1}' | "$CLI" -o json '$.a@string()'; then
    echo "expected failure"
    exit 1
fi

echo "== type selector recursive descent key =="
test "$(echo '{"a":{"truc":"hi"},"b":{"truc":1}}' | "$CLI" -a -o json '$..truc@string()')" = '["hi"]'

echo "== object wildcard find all =="
test "$(echo '{"a":1,"b":2}' | "$CLI" -a -o json '$.*')" = '[1,2]'

echo "== object wildcard remove =="
test "$(echo '{"a":1,"b":2}' | "$CLI" -d -o json '$.*')" = '{}'

echo "== recursive descent with filter =="
test "$(echo '{"items":[{"x":1},{"x":3}],"other":{"items":[{"x":5}]}}' | "$CLI" -a -o json '$..items[?x>1]')" = '[{"x":3},{"x":5}]'

echo "== keys callback on wildcard =="
test "$(echo '{"a":{"x":1,"y":2},"b":{"z":3}}' | "$CLI" -K -o json '$.*')" = '["x","y","z"]'

echo "== keys callback on filter =="
test "$(echo '[{"name":"a","tags":[1]},{"name":"b","tags":[2]}]' | "$CLI" -K -o json '$[?name=="a"].tags')" = '[0]'

echo "== invalid regex compile error =="
if echo '{"a":1}' | "$CLI" '$[?x=~"[bad"]'; then
    echo "expected failure"
    exit 1
fi

echo "== regex filter on mixed operands =="
test "$(echo '{"items":[{"id":1},{"id":"abc"}]}' | "$CLI" -a -o json '$.items[?id=~"abc"]')" = '[{"id":"abc"}]'

echo "== open-ended slice find all =="
test "$(echo '{"a":[10,20,30,40]}' | "$CLI" -a -o json '$.a[1:]')" = '[20,30,40]'

echo "== open-ended slice remove =="
test "$(echo '{"a":[10,20,30,40]}' | "$CLI" -d -o json '$.a[1:]')" = '{"a":[10,null,null,null]}'

echo "== union with wildcard =="
test "$(echo '{"a":1,"b":2}' | "$CLI" -a -o json '$[*,"a"]')" = '[1,2,1]'

echo "== numeric subpath in find all =="
test "$(echo '{"idx":1,"data":[10,20,30]}' | "$CLI" -a -o json '$.data[$.idx]')" = '[20]'

echo "== or-root remove =="
test "$(echo '{"b":1}' | "$CLI" -d -o json '$.missing|$.b')" = '{}'

echo "== filter null remove =="
test "$(echo '{"items":[{"a":"x"},{"a":null},{}]}' | "$CLI" -d -o json '$.items[?a==null]')" = '{"items":[{"a":"x"},null,null]}'

echo "== recursive descent update on object =="
test "$(echo '{"a":{"b":1},"c":{"b":2}}' | "$CLI" -o json --set 9 '$..b')" = '{"a":{"b":9},"c":{"b":9}}'

echo "== filter subpath equality string =="
test "$(echo '{"ref":"c","items":[{"name":"a"},{"name":"c"},{"name":"d"}]}' | "$CLI" -a -o json '$.items[?@.name == $.ref].name')" = '["c"]'

echo "== filter subpath inequality string =="
test "$(echo '{"ref":"c","items":[{"name":"a"},{"name":"c"},{"name":"d"}]}' | "$CLI" -a -o json '$.items[?@.name != $.ref].name')" = '["a","d"]'

echo "== filter subpath equality number =="
test "$(echo '{"ref":2,"items":[{"n":1},{"n":2},{"n":3}]}' | "$CLI" -a -o json '$.items[?@.n == $.ref].n')" = '[2]'

echo "== filter subpath equality type mismatch =="
if echo '{"ref":"x","items":[{"n":1},{"n":2}]}' | "$CLI" -a -o json '$.items[?@.n == $.ref].n'; then
    echo "expected failure"
    exit 1
fi

echo "== filter subpath equality missing key =="
if echo '{"items":[{"name":"a"},{"name":"c"}]}' | "$CLI" -a -o json '$.items[?@.name == $.missing].name'; then
    echo "expected failure"
    exit 1
fi

echo "== filter subpath ordering string > =="
test "$(echo '{"ref":"b","items":[{"name":"a"},{"name":"b"},{"name":"c"}]}' | "$CLI" -a -o json '$.items[?@.name > $.ref].name')" = '["c"]'

echo "== filter subpath ordering string >= =="
test "$(echo '{"ref":"b","items":[{"name":"a"},{"name":"b"},{"name":"c"}]}' | "$CLI" -a -o json '$.items[?@.name >= $.ref].name')" = '["b","c"]'

echo "== filter subpath ordering string < =="
test "$(echo '{"ref":"d","items":[{"name":"a"},{"name":"c"},{"name":"d"}]}' | "$CLI" -a -o json '$.items[?@.name < $.ref].name')" = '["a","c"]'

echo "== filter subpath ordering string <= =="
test "$(echo '{"ref":"c","items":[{"name":"a"},{"name":"c"},{"name":"d"}]}' | "$CLI" -a -o json '$.items[?@.name <= $.ref].name')" = '["a","c"]'

echo "== filter subpath ordering number > =="
test "$(echo '{"ref":2,"items":[{"n":1},{"n":2},{"n":3}]}' | "$CLI" -a -o json '$.items[?@.n > $.ref].n')" = '[3]'

echo "== filter subpath ordering number <= =="
test "$(echo '{"ref":2,"items":[{"n":1},{"n":2},{"n":3}]}' | "$CLI" -a -o json '$.items[?@.n <= $.ref].n')" = '[1,2]'

echo "== filter union =="
test "$(echo '{"items":[{"a":1,"b":2},{"a":3,"b":2},{"a":1,"b":3}]}' | "$CLI" -a -o json '$.items[?(@.a == 1), ?(@.b == 2)]')" = '[{"a":1,"b":2},{"a":1,"b":3},{"a":1,"b":2},{"a":3,"b":2}]'

echo "== filter whitespace before operator =="
test "$(echo '[{"a":1},{"a":2}]' | "$CLI" -a -o json '$[?(@.a   == 1)]')" = '[{"a":1}]'

echo "== filter big number literal =="
test "$(echo '[{"n":150},{"n":50}]' | "$CLI" -a -o json '$[?@.n == 150].n')" = '[150]'

echo "== regex filter non-string operand =="
test "$(echo '[{"n":1},{"n":"abc"}]' | "$CLI" -a -o json '$[?@.n =~ ".*"]')" = '[{"n":"abc"}]'

echo "== regex filter invalid pattern =="
if echo '[{"n":"a"}]' | "$CLI" -a -o json '$[?@.n =~ "["]'; then
    echo "expected failure"
    exit 1
fi

echo "== regex filter with subpath operand =="
if echo '[{"a":"x","pat":"x"}]' | "$CLI" -a -o json '$[?@.a =~ $.pat]'; then
    echo "expected failure"
    exit 1
fi

echo "== regex filter with numeric operand =="
if echo '[{"a":"x"}]' | "$CLI" -a -o json '$[?@.a =~ 1]'; then
    echo "expected failure"
    exit 1
fi

echo "== filter ordering missing key > =="
if echo '[{"a":1}]' | "$CLI" -a -o json '$[?@.missing > 1]'; then
    echo "expected failure"
    exit 1
fi

echo "== filter ordering missing key < =="
if echo '[{"a":1}]' | "$CLI" -a -o json '$[?@.missing < 1]'; then
    echo "expected failure"
    exit 1
fi

echo "== filter ordering type mismatch =="
if echo '[{"name":"x"}]' | "$CLI" -a -o json '$[?@.name < 1]'; then
    echo "expected failure"
    exit 1
fi

echo "== file not found =="
if "$CLI" -f /nonexistent '$.a'; then
    echo "expected failure"
    exit 1
fi

echo "== all CLI tests passed =="
