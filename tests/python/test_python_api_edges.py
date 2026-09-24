import pytest
import csonpath


def test_extra_objs_must_be_list_or_tuple():
    cp = csonpath.CsonPath('$.a')
    with pytest.raises(TypeError):
        cp.find_first({'a': 1}, extra_objs='nope')


def test_print_instructions():
    cp = csonpath.CsonPath('$.a.b')
    assert cp.print_instructions() is None


def test_callback():
    d = {'a': 1, 'b': 2}
    seen = []

    def cb(parent, idx, cur, udata):
        seen.append(cur)
        return 0

    cp = csonpath.CsonPath('$.*')
    assert cp.callback(d, cb, None) == 2
    assert sorted(seen) == [1, 2]


def test_callback_bad_args():
    cp = csonpath.CsonPath('$.*')
    with pytest.raises(TypeError):
        cp.callback({'a': 1})


def test_update_or_create_callback():
    d = {'items': [{'v': 1}, {'v': 2}]}

    def cb(parent, idx, cur, udata):
        parent[idx] = {'v': cur['v'] * 10}
        return 0

    cp = csonpath.CsonPath('$.items[*]')
    assert cp.update_or_create_callback(d, cb) == 2
    assert d == {'items': [{'v': 10}, {'v': 20}]}


def test_remove_with_extra_objs_error():
    cp = csonpath.CsonPath('$.a')
    with pytest.raises(TypeError):
        cp.remove({'a': 1}, extra_objs=42)


def test_update_or_create_with_extra_objs_error():
    cp = csonpath.CsonPath('$.a')
    with pytest.raises(TypeError):
        cp.update_or_create({'a': 1}, 2, extra_objs=42)
