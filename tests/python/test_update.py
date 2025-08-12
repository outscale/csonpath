import pytest
import csonpath

def test_update():
        o = csonpath.CsonPath("$.a.b")
        d = {"c": "not-useful"}
        e = "hej hej"

        o.update_or_create(d, e)
        assert d["a"]["b"] == "hej hej"
