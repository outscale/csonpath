import csonpath
import sys


def test_update_or_create_refcount_insert_once():
    value = ["wololo"]
    d = {}
    d_ref_bef = sys.getrefcount(d)
    cp = csonpath.CsonPath("$.a")
    ref_before = sys.getrefcount(value)
    ret = cp.update_or_create(d, value)
    ref_after = sys.getrefcount(value)
    assert ret == 1
    assert d["a"] is value
    assert d_ref_bef == sys.getrefcount(d)
    # Should be exactly +1 for the one insertion
    assert ref_after == ref_before + 1


def test_update_or_create_refcount_insert_twice():
    value = ["wololo"]
    d = {}
    cp = csonpath.CsonPath("$.a")
    ref_before = sys.getrefcount(value)
    ret1 = cp.update_or_create(d, value)
    cp.set_path("$.b")
    ret2 = cp.update_or_create(d, value)
    ref_after = sys.getrefcount(value)
    # Should be exactly +2 for two insertions
    assert d["a"] is value
    assert d["b"] is value
    assert ret1 == 1
    assert ret2 == 1
    assert ref_after == sys.getrefcount(value)  # This is not enough! See below.
    # The right way:
    expected = ret1 + ret2
    assert ref_after == ref_before + expected


def test_update_or_create_refcount_replaces_and_releases_old():
    old_value = "old-------"
    new_value = ["new"]
    d = {"a": old_value}
    cp = csonpath.CsonPath("$.a")

    old_ref_before = sys.getrefcount(old_value)
    new_ref_before = sys.getrefcount(new_value)

    ret = cp.update_or_create(d, new_value)
    old_ref_after = sys.getrefcount(old_value)
    new_ref_after = sys.getrefcount(new_value)

    # Check that the update happened
    assert d["a"] is new_value
    assert ret == 1
    # Old value refcount decreased by one (container dropped it)
    assert old_ref_after == old_ref_before - 1
    # New value refcount increased by one (container now holds it)
    assert new_ref_after == new_ref_before + 1


def test_update_or_create_refcount_shared_subobject():
    sub = {"a": 1}
    value = [sub, sub]
    d = {}
    cp = csonpath.CsonPath("$.x")
    ret = cp.update_or_create(d, value)
    ref_after = sys.getrefcount(sub)
    # Only value holds refs to sub, d["x"] holds one to value
    assert ret == 1
    # If we update another key with sub:
    cp.set_path("$.y")
    ret2 = cp.update_or_create(d, sub)
    ref_after2 = sys.getrefcount(sub)
    # Should be +1 for the direct insertion
    assert ret2 == 1
    assert ref_after2 == ref_after + 1
