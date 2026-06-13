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
