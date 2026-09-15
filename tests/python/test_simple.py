import pytest
import csonpath

def test_simple_array():
    o = csonpath.CsonPath("$[0]")
    assert o.find_all(["one element"]) == ["one element"]
    o = csonpath.CsonPath("$[1]")
    assert o.find_all(["one element"]) == None


def test_get_array_big_index():
    """Indices >= 100 use GET_ARRAY_BIG instead of GET_ARRAY_SMALL."""
    p = csonpath.CsonPath("$.a[100]")
    d = {"a": [0] * 100 + ["target"]}
    assert p.find_first(d) == "target"
    assert p.find_all(d) == ["target"]


def test_get_array_big_update_or_create():
    p = csonpath.CsonPath("$.a[100].b")
    d = {"a": [{"b": i} for i in range(101)]}
    p.update_or_create(d, "new")
    assert d["a"][100]["b"] == "new"


def test_get_array_big_remove():
    p = csonpath.CsonPath("$.a[100]")
    d = {"a": [0] * 101}
    assert p.remove(d) == 1


def test_get_array_big_callback():
    p = csonpath.CsonPath("$.a[100]")
    d = {"a": [0] * 100 + ["target"]}
    seen = []
    def cb(parent, idx, cur, ud):
        seen.append(cur)
    p.callback(d, cb)
    assert seen == ["target"]
