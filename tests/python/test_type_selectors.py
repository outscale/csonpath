import pytest
import csonpath


def test_type_selector_string():
    d = {"a": "x", "b": 1, "c": None}
    cp = csonpath.CsonPath('$.*@string()')
    assert cp.find_all(d) == ["x"]
    assert cp.find_first(d) == "x"


def test_type_selector_integer():
    d = {"a": "x", "b": 1, "c": None}
    cp = csonpath.CsonPath('$.*@integer()')
    assert cp.find_all(d) == [1]


def test_type_selector_null():
    d = {"a": "x", "b": 1, "c": None}
    cp = csonpath.CsonPath('$.*@null()')
    assert cp.find_all(d) == [None]


def test_type_selector_no_match():
    d = {"a": 1}
    cp = csonpath.CsonPath('$.a@string()')
    assert cp.find_first(d) is None
    assert cp.find_all(d) is None


def test_type_selector_on_array():
    d = ["a", 1, None]
    cp = csonpath.CsonPath('$[*]@string()')
    assert cp.find_all(d) == ["a"]


def test_recursive_descent_wildcard():
    d = {"a": {"b": 1}, "c": 2}
    cp = csonpath.CsonPath('$..*')
    assert cp.find_all(d) == [{"b": 1}, 1, 2]


def test_recursive_descent_wildcard_array():
    d = [1, 2]
    cp = csonpath.CsonPath('$..*')
    assert cp.find_all(d) == [1, 2]


def test_recursive_descent_wildcard_with_type_selector():
    d = {"a": {"b": "hi"}}
    cp = csonpath.CsonPath('$..*@string()')
    assert cp.find_all(d) == ["hi"]


def test_invalid_type_selector():
    with pytest.raises(ValueError):
        csonpath.CsonPath('$.a@foo()')
