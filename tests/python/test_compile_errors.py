import pytest
import csonpath


LONG_KEY = "k" * 300

COMPILE_ERROR_CASES = [
    ("$[a]", "unexpected 'a'"),
    ("$[1a]", "unexpected 'a'"),
    ("$.a[-1]", "unexpected '-'"),
    ("$[']", "unclose union"),
    ("$['a", "unclose union"),
    ("$['a']b", "unexpected char 'b'"),
    ("$['a'b]", "']' require instead of"),
    ("$..[a]", ".. require string"),
    ("$.a.", "empty getter"),
    ("$.a.b$", "unexpected char '$'"),
    ("$.a.*b", "unsuported characters 'b' after '*'"),
    ("$[*a]", "unclose bracket"),
    ("$..[*]", "'*' is invalide here"),
    ("$..[?(@.x)]", "'?' is invalide here"),
    ("$.a[?(@[x])]", "string require here, got 'x'"),
    ("$.a[?(@.x % 1)]", "unsuported operation"),
    ("$.a[?(@.x == )]", "broken filter"),
    ("$.a[?(@.x == \"a\" &&)]", "too many open parentesis"),
    ("$.a[?(@.x == \"a\"]", "')' require"),
    ("$.a[?@.x == a1]", "broken filter with number"),

    ("$.a[?b =~ \"[\"]", "regex has error"),
    ("$.a[?b =~ $.x]", "subpath unsuported for regex"),
    ("$.a[?@.x =~ 0]", "number unsuported for regex"),
    ("$..[0]", ".. require string"),
    ("$257", "extra root index too big"),
    ("$.arr[?['" + LONG_KEY + "']=\"y\"]", "filter key too long"),
]


@pytest.mark.parametrize("path,expected", COMPILE_ERROR_CASES, ids=lambda x: x[0])
def test_compile_error(path, expected):
    with pytest.raises(ValueError) as exc_info:
        csonpath.CsonPath(path)
    assert expected in str(exc_info.value)


def test_wildcard_first_in_union_compiles():
    """$[*,'a'] is valid and exercises the UNION_JMP branch at compile time."""
    p = csonpath.CsonPath("$[*,'a']")
    assert p is not None
    # wildcard returns all values, then 'a' returns its value again.
    result = p.find_all({"a": 1, "b": 2})
    assert len(result) == 3
