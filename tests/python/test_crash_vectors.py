import pytest
import csonpath

def test_recursive_descent_non_str_dict_key():
    """
    A dict with a non-string key is invalid JSON but valid Python.
    Recursive descent (`$..a`) iterates dict entries with
    PyUnicode_AsUTF8AndSize; on a non-string key it returns NULL and the
    result is passed to strcmp() -> NULL dereference. It must raise a
    TypeError instead of crashing (only string keys are valid).
    """
    p = csonpath.CsonPath("$..a")
    o = {1: "x", "a": "target"}
    with pytest.raises(TypeError):
        p.find_first(o)
    with pytest.raises(TypeError):
        p.find_all(o)

def test_callback_that_removes_list_element():
    """
    If a callback removes elements from a list during find_all iteration,
    the next iteration might access an out-of-bounds index and crash.
    """
    obj = {"a": [1, 2, 3]}
    call_count = 0

    def cb(ctx, key, val, data):
        nonlocal call_count
        call_count += 1
        # key is the integer index, val is the element
        if isinstance(key, int) and len(ctx) > 0:
            del ctx[key]
        return None

    p = csonpath.CsonPath("$.a[*]")
    try:
        result = p.callback(obj, cb)
    except Exception as e:
        # If it raises safely that's fine, but we must not segfault.
        print(f"Caught expected exception: {e}")
        result = -1
    print(f"calls={call_count}, result={result}, list={obj['a']}")


def test_callback_that_clears_list():
    obj = {"a": [1, 2, 3]}
    call_count = 0

    def cb(ctx, key, val, data):
        nonlocal call_count
        call_count += 1
        ctx.clear()
        return None

    p = csonpath.CsonPath("$.a[*]")
    try:
        result = p.callback(obj, cb)
    except Exception as e:
        print(f"Caught expected exception: {e}")
        result = -1
    print(f"calls={call_count}, result={result}, list={obj['a']}")


def test_find_all_refcount_uaf():
    """
    find_all result list borrows references (no Py_INCREF).
    If the original object becomes unreachable, accessing elements
    of the result list can trigger a use-after-free.
    """
    import gc
    import weakref

    class Obj:
        def __init__(self, x):
            self.x = x

    p = csonpath.CsonPath("$.a[*]")

    # Create nested structure; only hold a reference to the result list
    # and a weak reference to the custom objects.
    inner = {"a": [Obj(i) for i in range(10)]}
    result = p.find_all(inner)
    refs = [weakref.ref(o) for o in result]

    # Delete the only strong reference to the original data
    del inner
    gc.collect()

    alive = sum(1 for w in refs if w() is not None)
    # With proper refcounting all 10 objects should still be alive.
    assert alive == 10, (
        f"BUG: only {alive}/10 objects survived GC — "
        f"find_all result list holds borrowed references"
    )


# ---------------------------------------------------------------------------
# update_or_create with huge array index must not hang/OOM (None-padding DoS)
# ---------------------------------------------------------------------------

def test_update_or_create_huge_array_index_raises():
    """Creating at a humongous index pads with None in a tight loop, eating
    unbounded CPU + memory.  Must raise IndexError immediately."""
    p = csonpath.CsonPath("$.a[100000000000000]")
    with pytest.raises((IndexError, MemoryError)):
        p.update_or_create({"a": [1]}, "x")


def test_update_or_create_array_gap_still_pads():
    """Moderate out-of-bounds gaps must still produce the expected padded
    array — only absurdly large gaps are rejected."""
    p = csonpath.CsonPath("$.a[5]")
    d = {"a": [1, 2, 3]}
    p.update_or_create(d, 42)
    assert d["a"] == [1, 2, 3, None, None, 42]
