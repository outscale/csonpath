import gc
import weakref

import pytest
import sys

if not hasattr(sys, 'getrefcount'):
    pytest.skip("refcount tests not applicable on PyPy", allow_module_level=True)

import csonpath


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
    old_value = ["old-------"]
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


def test_fuzzer_keeps_obj_alive_after_caller_frame_returns():
    """The fuzzer must hold a strong reference to the user-supplied object.

    Before the fix, the fuzzer stored ``obj`` as a borrowed reference. When
    ``obj`` was a local variable and the fuzzer outlived the function frame,
    the object could be garbage-collected, leaving the fuzzer with dangling
    pointers and causing segfaults or use-after-free errors on ``step()``.
    """

    def make_fuzzer():
        obj = {"str": "hello", "items": ["a", "b", "c"]}
        return csonpath.Fuzzer(["$.str", "$.items[*]"], seed=1, obj=obj)

    f = make_fuzzer()
    gc.collect()
    # This used to crash or read freed memory.
    assert f.step() in {
        csonpath.MODIFY_STR,
        csonpath.MODIFY_CNT_TYPE,
        csonpath.MODIFY_STR_TYPE,
    }


def test_fuzzer_releases_obj_on_destruction():
    obj = {"str": "hello"}
    ref_before = sys.getrefcount(obj)
    f = csonpath.Fuzzer(["$.str"], seed=1, obj=obj)
    # The fuzzer now owns one reference.
    assert sys.getrefcount(obj) == ref_before + 1
    del f
    gc.collect()
    # After destruction the reference must be released.
    assert sys.getrefcount(obj) == ref_before
