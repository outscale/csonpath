import pytest
import csonpath

def split_line(parent, idx, cur, data):
    parent[idx] = cur.splitlines()

def test_callback():
        o = csonpath.CsonPath("$.c")
        d = {"c": "new\nline"}

        o.callback(d, split_line)
        assert type(d["c"]) == list
        assert type(d["c"][0] == "new")
        assert type(d["c"][1] == "line")

def set_data(parent, idx, cur, data):
    parent[idx] = data

def test_callback_data():
        o = csonpath.CsonPath("$.c")
        d = {"c": "new\nline"}

        o.callback(d, set_data, 123)
        assert d["c"] == 123
