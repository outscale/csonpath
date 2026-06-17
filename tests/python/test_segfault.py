"""
TDD regression tests for known segfaults.

These tests describe the *desired* behaviour.  Right now many of them crash
(SIGSEGV) so they will kill the pytest runner if executed in-process.
Run individual tests in a subprocess while debugging, e.g.:

    python -c "import tests.python.test_segfault as t; t.test_set_path_bad_then_find()"

Once the underlying C bugs are fixed every test below should pass without
any change to this file.
"""

import pytest
import csonpath


# ---------------------------------------------------------------------------
# update_or_create with non-string keys in the replacement dict
# ---------------------------------------------------------------------------

def test_update_or_create_int_key_raises_type_error():
    """Only string keys are valid in a JSON/dict update value."""
    cp = csonpath.CsonPath('$')
    with pytest.raises(TypeError):
        cp.update_or_create({'a': 1}, {1: 'a'})


def test_update_or_create_tuple_key_raises_type_error():
    cp = csonpath.CsonPath('$')
    with pytest.raises(TypeError):
        cp.update_or_create({'a': 1}, {(1, 2): 'a'})


def test_update_or_create_none_key_raises_type_error():
    cp = csonpath.CsonPath('$')
    with pytest.raises(TypeError):
        cp.update_or_create({'a': 1}, {None: 'a'})


# ---------------------------------------------------------------------------
# set_path with an invalid path must raise and leave the object usable
# ---------------------------------------------------------------------------

def test_set_path_bad_then_find_still_works():
    """A failed set_path must not corrupt the compiled path object."""
    cp = csonpath.CsonPath('$.a')
    with pytest.raises(ValueError):
        cp.set_path('bad[')
    # original path must still be functional
    assert cp.find_first({'a': 42}) == 42


def test_multiple_set_path_bad_keeps_original():
    cp = csonpath.CsonPath('$.a')
    for bad in ('bad1[', 'bad2[', 'bad3['):
        with pytest.raises(ValueError):
            cp.set_path(bad)
    assert cp.find_first({'a': 42}) == 42


def test_find_all_after_failed_set_path():
    cp = csonpath.CsonPath('$.a')
    with pytest.raises(ValueError):
        cp.set_path('bad[')
    assert cp.find_all({'a': 1}) == [1]


def test_remove_after_failed_set_path():
    cp = csonpath.CsonPath('$.a')
    with pytest.raises(ValueError):
        cp.set_path('bad[')
    d = {'a': 1}
    cp.remove(d)
    assert 'a' not in d


def test_callback_after_failed_set_path():
    cp = csonpath.CsonPath('$.a')
    with pytest.raises(ValueError):
        cp.set_path('bad[')
    called = []
    cp.callback({'a': 1}, lambda ctx, k, v, ud: called.append((k, v)))
    assert called == [('a', 1)]


def test_update_or_create_after_failed_set_path():
    cp = csonpath.CsonPath('$.a')
    with pytest.raises(ValueError):
        cp.set_path('bad[')
    cp.update_or_create({'a': 1}, 2)
    assert cp.find_first({'a': 2}) == 2


# ---------------------------------------------------------------------------
# print_instructions must be safe even after a failed set_path
# ---------------------------------------------------------------------------

def test_print_instructions_after_failed_set_path():
    cp = csonpath.CsonPath('$.a')
    with pytest.raises(ValueError):
        cp.set_path('bad[')
    # must not segfault
    cp.print_instructions()


def test_print_instructions_twice_after_failed_set_path():
    cp = csonpath.CsonPath('$.a')
    with pytest.raises(ValueError):
        cp.set_path('bad[')
    cp.print_instructions()
    cp.print_instructions()


# ---------------------------------------------------------------------------
# Callbacks must not segfault when the user mutates the container
# ---------------------------------------------------------------------------

def test_callback_clearing_list_during_iteration():
    """Mutating a list while CsonPath iterates over it must raise RuntimeError."""
    l = [1, 2, 3]
    cp = csonpath.CsonPath('$[*]')

    def cb(ctx, idx, val, ud):
        if isinstance(ctx, list):
            ctx.clear()

    with pytest.raises(RuntimeError):
        cp.callback(l, cb)


def test_callback_popping_list_during_iteration():
    l = [1, 2, 3]
    cp = csonpath.CsonPath('$[*]')

    def cb(ctx, idx, val, ud):
        if isinstance(ctx, list) and len(ctx) > 0:
            ctx.pop(0)

    with pytest.raises(RuntimeError):
        cp.callback(l, cb)


# ---------------------------------------------------------------------------
# update_or_create_callback on root must raise, not crash
# ---------------------------------------------------------------------------

def test_update_or_create_callback_on_root():
    """Root ($) cannot be updated via callback; it must raise ValueError."""
    cp = csonpath.CsonPath('$')
    with pytest.raises(ValueError, match="can't update root"):
        cp.update_or_create_callback({'a': 1}, lambda *a: None)


def test_callback_exception_with_multiple_matches():
    """A callback that raises on the first match must stop iteration."""
    cp = csonpath.CsonPath('$.a[*]')
    d = {'a': [1, 2]}

    def cb(ctx, idx, val, ud):
        raise ValueError('boom')

    with pytest.raises(ValueError, match='boom'):
        cp.callback(d, cb)


def test_callback_exception_recursive_descent():
    """Same via recursive descent with multiple matches."""
    cp = csonpath.CsonPath('$..b')
    d = {'x': {'b': 1}, 'y': {'b': 2}}

    def cb(ctx, k, val, ud):
        raise ValueError('boom')

    with pytest.raises(ValueError, match='boom'):
        cp.callback(d, cb)


def test_update_or_create_callback_exception_with_multiple_matches():
    """update_or_create_callback with multiple matches raising."""
    cp = csonpath.CsonPath('$.a[*]')
    d = {'a': [1, 2]}

    def cb(ctx, idx, val, ud):
        raise ValueError('boom')

    with pytest.raises(ValueError, match='boom'):
        cp.update_or_create_callback(d, cb)
