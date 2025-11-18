import pytest
import csonpath


def test_torefacto():
    o = csonpath.CsonPath("$.a")
    # breakpoint()
    o = csonpath.CsonPath("$.a")
    d = {"B": "Bh", "a": "le a"}
    r = o.find_first(d)

    assert r == "le a"

    r = o.find_all(d)

    assert r == ["le a"]

    o = csonpath.CsonPath("$.C")
    r = o.find_first(d)

    assert r is None

    o = csonpath.CsonPath("$[*]")
    r = o.find_all(d)

    assert r == ["Bh", "le a"]

    d = {"B": {"a": "true A"}, "a": "le a", "C": {"B": "not good", "a": "oh"}}
    o = csonpath.CsonPath("$[*].a")
    r = o.find_all(d)
    assert(r != ["oh"])

    o = csonpath.CsonPath("$..a")
    r = o.find_all(d)
    assert r == ["true A", "le a", "oh"]

    o.remove(d)
    assert d == {"B": {}, "C": {"B": "not good"}}

    d["C"]["la"] = "lo"
    o = csonpath.CsonPath("$.C[*]")
    o.remove(d)
    assert d == {"B": {}, "C": {}}

    o = csonpath.CsonPath("$.C")
    o.remove(d)
    assert d == {"B": {}}

    d["ar"] = [1, 2, 3]

    o = csonpath.CsonPath("$.ar[1]")
    o.remove(d)
    assert d == {"B": {}, "ar": [1, 3]}

    o = csonpath.CsonPath("$.ar[*]")
    o.remove(d)
    assert d == {"B": {}, "ar": []}
