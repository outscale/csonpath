import pytest
import csonpath

def split_line(parent, idx, cur, udata):
    parent[idx] = cur.splitlines()

def test_update():
        o = csonpath.CsonPath("$.c")
        d = {"c": "new\nline"}

        o.callback(d, split_line)
        assert type(d["c"]) == list
        assert type(d["c"][0] == "new")
        assert type(d["c"][1] == "line")
