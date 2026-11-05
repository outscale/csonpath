"""Refcount checks for the per-exec ``extra_objs`` argument.

No exec method may leak a reference to the list or its items: the
engine makes a private copy (``PySequence_List``) that is released
after execution.
"""

import sys

import pytest

import csonpath

pytestmark = pytest.mark.skipif(
    not hasattr(sys, "getrefcount"),
    reason="sys.getrefcount is not available on this interpreter",
)


def test_exec_explicit_kwarg_does_not_leak_list_or_items():
    item = {"a": 42}
    lst = [0, item]
    before_lst = sys.getrefcount(lst)
    before_item = sys.getrefcount(item)

    data = {"nodes": [{"a": 42}]}
    cp = csonpath.CsonPath("$2.a")

    assert cp.find_first(data, extra_objs=lst) == 42
    assert list(cp.find_all(data, extra_objs=lst)) == [42]

    def cb(parent, idx, cur, udata):
        return 0

    assert cp.callback(data, cb, None, extra_objs=lst) >= 1

    # update_or_create and remove mutate the extra-root item ($2.a),
    # so they must run last.
    assert cp.update_or_create(data, 43, extra_objs=lst) >= 1
    assert cp.remove(data, extra_objs=lst) >= 0

    assert sys.getrefcount(lst) == before_lst
    assert sys.getrefcount(item) == before_item


def test_exec_tuple_kwarg_does_not_leak():
    item = {"a": 42}
    tpl = (0, item)
    before_item = sys.getrefcount(item)

    data = {"nodes": [{"a": 42}]}
    assert csonpath.CsonPath("$2.a").find_first(data, extra_objs=tpl) == 42

    assert sys.getrefcount(item) == before_item
