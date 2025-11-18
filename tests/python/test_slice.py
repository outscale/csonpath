import pytest
import csonpath


def test_slice():
    o = csonpath.CsonPath("$.[0:2]")
    d = [0,10,21,300]
    assert o.find_all(d) == [0,10]

    o = csonpath.CsonPath("$.[2:3]")
    assert o.find_all(d) == [21]

    o = csonpath.CsonPath("$.[0:]")
    assert o.find_all(d) == [0,10,21,300]
