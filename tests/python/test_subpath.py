import pytest
import csonpath
import sys

def test_update_or_create_subpath_index_empty_array():
    """Subpath index on empty array should create the element, not overwrite the index key."""
    d = {"metrics": [], "_idx": 0}
    p = csonpath.CsonPath("$.metrics[$._idx].name")
    p.update_or_create(d, "hello")
    assert d == {"metrics": [{"name": "hello"}], "_idx": 0}


def test_update_or_create_subpath_index_nonempty_array():
    """Subpath index on non-empty array works correctly."""
    d = {"metrics": [{}], "_idx": 0}
    p = csonpath.CsonPath("$.metrics[$._idx].name")
    p.update_or_create(d, "hello")
    assert d == {"metrics": [{"name": "hello"}], "_idx": 0}

def test_update_or_create_callback_subpath_index_empty_array():
    """update_or_create_callback with subpath index on empty array should create parents."""
    d = {"metrics": [], "_idx": 0}
    p = csonpath.CsonPath("$.metrics[$._idx].name")

    def cb(parent, idx, cur, ud):
        parent[idx] = ud
    p.update_or_create_callback(d, cb, "hello")
    assert d == {"metrics": [{"name": "hello"}], "_idx": 0}


def test_extra_objs_find_first():
    """$N can switch to an extra root passed from Python."""
    obj = {"a": {"a": "oh"}}
    extra = {"b": "a"}
    p = csonpath.CsonPath("$.a[$1.b]")
    assert p.find_first(obj, extra_objs=[extra]) == "oh"


def test_extra_objs_find_all():
    """$N works with find_all too."""
    obj = {"a": {"a": "oh"}}
    extra = {"b": "a"}
    p = csonpath.CsonPath("$.a[$1.b]")
    assert p.find_all(obj, extra_objs=[extra]) == ["oh"]


def test_extra_objs_update_or_create_root():
    """$1 alone as update_or_create path returns 0 and does not crash."""
    main = {"a": 1}
    extra = {"b": 2}
    p = csonpath.CsonPath("$1")
    ret = p.update_or_create(main, "new_value", extra_objs=[extra])
    assert ret == 0
    assert main == {"a": 1}
    assert extra == {"b": 2}


def test_extra_objs_update_or_create_extra_path():
    """$1.foo mutates the first extra root, not the main root."""
    main = {}
    extra = {}
    p = csonpath.CsonPath("$1.foo")
    p.update_or_create(main, "bar", extra_objs=[extra])
    assert main == {}
    assert extra == {"foo": "bar"}


def test_extra_objs_missing_raises():
    """$N without extra_objs must raise instead of hanging."""
    p = csonpath.CsonPath("$1.foo")
    with pytest.raises(ValueError, match="out of bounds"):
        p.find_first({"foo": 1})


def test_extra_objs_zero_is_main_root():
    """$0 is an alias for the main root."""
    obj = {"foo": 1}
    extra = {"foo": 2}
    p = csonpath.CsonPath("$0.foo")
    assert p.find_first(obj, extra_objs=[extra]) == 1


def test_subpath_string_key_not_found():
    """GET_SUBPATH returning a missing string key should return None."""
    p = csonpath.CsonPath("$.a[$.b]")
    d = {"a": {"x": 1}, "b": "z"}
    assert p.find_first(d) is None


def test_subpath_index_found_in_array():
    """GET_SUBPATH returning a valid array index hits POST_FIND_ARRAY."""
    p = csonpath.CsonPath("$.a[$.b]")
    d = {"a": [10, 20], "b": 1}
    assert p.find_all(d) == [20]


def test_recursive_descent_array():
    """$..a recurses through arrays of objects."""
    p = csonpath.CsonPath("$..a")
    d = {"x": [{"a": 1}, {"a": 2}]}
    assert p.find_all(d) == [1, 2]
