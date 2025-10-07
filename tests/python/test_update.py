import pytest
import csonpath


def test_update():
    o = csonpath.CsonPath("$.a.b")
    d = {"c": "not-useful"}
    e = "hej hej"

    o.update_or_create(d, e)
    assert d["a"]["b"] == "hej hej"


def test_update_at_out_of_bound():
    o = csonpath.CsonPath("$.a[3]")
    d = {}
    o.update_or_create(d, "hej hej")
    assert d["a"][3] == "hej hej"

def test_update_root():
    o = csonpath.CsonPath("$")
    v = {"1": 1, "2" : "2", "3": 3}
    d = []
    o.update_or_create(d, [1,2,3,4])
    assert d == [1,2,3,4]

    d = {}
    o.update_or_create(d, v)
    assert d == {"1": 1, "2" : "2", "3": 3}


def test_update_create():
    jp = csonpath.CsonPath("$[*].a")
    d = [{"a": 0}, {"a": 40}, {"a": [1, 2, 3]}, {"a": 0}, {"a": 5}]

    e = [{}, {}, {}, {}, {}]

    all = jp.find_all(d)
    assert len(all) == 5
    d = [all, 0]

    def sync_cp(parent, idx, cur, data):
        parent[idx] = data[0][data[1]]
        data[1] += 1

    jp.update_or_create_callback(e, sync_cp, d)
    assert len(e) == 5
    assert e[0]["a"] == 0
    assert e[1]["a"] == 40
    assert e[2]["a"] == [1, 2, 3]
    assert e[3]["a"] == 0
    assert e[4]["a"] == 5


def test_a_b_c():
    path = "$.a.b.c"
    o = csonpath.CsonPath(path)

    d = {}
    o.update_or_create(d, [])
    assert d["a"]["b"]["c"] == []
