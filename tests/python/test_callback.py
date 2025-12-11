import pytest
import csonpath
import sys


def split_line(parent, idx, cur, data):
    parent[idx] = cur.splitlines()


def test_callback():
    o = csonpath.CsonPath("$.c")
    d = {"c": "new\nline"}

    o.callback(d, split_line)
    assert type(d["c"]) == list
    assert type(d["c"][0] == "new")
    assert type(d["c"][1] == "line")


VALUE_REF: int = 0


def set_data(parent, idx, cur, data):
    parent[idx] = data


def test_callback_data():
    o = csonpath.CsonPath("$.c")
    d = {"c": "new\nline"}
    value = [123]

    global VALUE_REF
    VALUE_REF = sys.getrefcount(value)
    assert VALUE_REF > 0
    o.callback(d, set_data, value)
    assert VALUE_REF == sys.getrefcount(value) - 1
    assert d["c"] == [123]


def syncronyse_in_out(source_path, json_in, json_out):
    if json_in is json_out:
        return
    jp = csonpath.CsonPath(source_path)
    all = jp.find_all(json_in)
    d = [all, 0]

    def syn_cp(parent, idx, cur, data):
        parent[idx] = data[0][data[1]]
        data[1] += 1

    jp.update_or_create_callback(json_out, syn_cp, d)


def test_callback_more():
    d = {"a": "wololo\nayoyoyo\nw0l0l0"}
    e = {"a": []}

    jp = csonpath.CsonPath("$.a")
    syncronyse_in_out("$.a", d, e)
    jp.callback(e, split_line)
    assert e == {"a": ["wololo", "ayoyoyo", "w0l0l0"]}
