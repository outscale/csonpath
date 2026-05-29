import pytest
import csonpath

def test_simple_array():
    o = csonpath.CsonPath("$[0]")
    assert o.find_all(["one element"]) == ["one element"]
    o = csonpath.CsonPath("$[1]")
    assert o.find_all(["one element"]) == None
