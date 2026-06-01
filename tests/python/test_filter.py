import pytest
import csonpath
import sys


def test_simple_filter():
    dict = {"ar": [{"0": "a 0", "1": "a 1"}, {"ah": "a 0", "bh": "a 10"}]}
    cp = csonpath.CsonPath('$.ar[?0="a 0"]')

    ret = cp.find_first(dict)

    assert ret == {"0": "a 0", "1": "a 1"}, "fail result look like: {}".format(ret)

    ret = cp.find_all(dict)

    assert ret == [{"0": "a 0", "1": "a 1"}], "fail result look like: {}".format(ret)

    value = ["wololo"]

    ref_before = sys.getrefcount(value) if hasattr(sys, 'getrefcount') else None
    ret = cp.update_or_create(dict, value)
    ref_after = sys.getrefcount(value) if hasattr(sys, 'getrefcount') else None

    assert ret == 1
    if ref_before is not None:
        assert ref_before == ref_after - ret
    assert dict == {
        "ar": [["wololo"], {"ah": "a 0", "bh": "a 10"}]
    }, "fail result look like: {}".format(dict["ar"][0])

    cp.set_path('$.ar[?ah="a 0"]')
    ret = cp.remove(dict)
    assert ret == 1
    assert dict == {"ar": [["wololo"]]}, "fail result look like: {}".format(
        dict["ar"][0]
    )

    dict = {"ha": [{"h": "Leodagan"}, {"h": "George"}]}
    cp = csonpath.CsonPath('$.ha[?h != "Leodagan"].h')
    ret = cp.find_all(dict)
    assert ret == ["George"]

    dict = {"ha": [{"h": {"h1": "Leodagan"}}, {"h": "George"}]}
    cp = csonpath.CsonPath('$.ha[?h.h1 = "Leodagan"].h.h1')
    ret = cp.find_all(dict)

    dict = {"ha": [{"h": {"h1": "Leodagan"}}, {"h": "George"}]}
    cp = csonpath.CsonPath('$.ha[?h.h1 =~ "gan"].h.h1')
    ret = cp.find_all(dict)

    assert ret == ["Leodagan"], dict

    dict = {"ha": [{"h": {"h1": "Leodagan"}}, {"h": "George"}]}
    cp = csonpath.CsonPath('$.ha[?h.h1 =~ "Assim"].h.h1')
    ret = cp.find_all(dict)

    assert ret is None

    dict = {"ha": [{"h": {"h1": "Leodagan"}}, {"h": "George"}]}
    cp = csonpath.CsonPath('$.ha[?h.h1 =~ "Assim"].h.h1', return_empty_array=True)
    ret = cp.find_all(dict)

    assert ret == []

    cp = csonpath.CsonPath('$.ha[?h.h1 =~ "Assim"].h.h1', True)
    ret = cp.find_all(dict)

    assert ret == []


def test_multiple_filters():
    # cf. https://github.com/h2non/jsonpath-ng?tab=readme-ov-file#extensions, last example of filter
    dict = {"knights": [{"name": "Leodagan", "laterality": "left",
                         "sub": {"a": 10}}, {"name": "George", "laterality": "left"}]}

    cp = csonpath.CsonPath('$.knights[?laterality = "left" & name = "George"]')
    ret = cp.find_all(dict)
    assert ret == [{'name': 'George', 'laterality': 'left'}]

    cp = csonpath.CsonPath('$.knights[?laterality = "left" & sub.a = 10]')
    ret = cp.find_all(dict)
    assert ret == [{'name': 'Leodagan', 'laterality': 'left', "sub": {"a": 10}}]

    cp = csonpath.CsonPath('$.knights[?laterality = "left" & sub.a = 10].sub')
    ret = cp.find_first(dict)
    assert ret == {"a": 10}

    cp = csonpath.CsonPath('$.knights[?(@.name == "Assim")].laterality')
    ret = cp.find_first(dict)

    assert ret is None

    cp = csonpath.CsonPath('$.knights[?(@.name == "Assim")].laterality', return_empty_array=True)
    ret = cp.find_first(dict)

    assert ret is None


def test_filter_on_none():
    cp = csonpath.CsonPath('$.ar[?(@.a==1)]')
    assert cp.find_first(None) is None
    assert cp.find_all(None) is None


def test_filter_on_scalar():
    cp = csonpath.CsonPath('$.ar[?(@.a==1)]')
    assert cp.find_first(42) is None
    assert cp.find_all(42) is None

def test_filter_superior_inferior():
    # number superior
    d = {"items": [{"v": 1}, {"v": 5}, {"v": 10}]}
    cp = csonpath.CsonPath('$.items[?v > 2]')
    assert cp.find_all(d) == [{"v": 5}, {"v": 10}]

    # number inferior
    d = {"items": [{"v": 1}, {"v": 5}, {"v": 10}]}
    cp = csonpath.CsonPath('$.items[?v < 6]')
    assert cp.find_all(d) == [{"v": 1}, {"v": 5}]

    # string superior
    d = {"items": [{"v": "a"}, {"v": "c"}, {"v": "z"}]}
    cp = csonpath.CsonPath('$.items[?v > "b"]')
    assert cp.find_all(d) == [{"v": "c"}, {"v": "z"}]

    # string inferior
    d = {"items": [{"v": "a"}, {"v": "c"}, {"v": "z"}]}
    cp = csonpath.CsonPath('$.items[?v < "b"]')
    assert cp.find_all(d) == [{"v": "a"}]

    # mixing string and number should fail (return None)
    d = {"items": [{"v": 5}]}
    cp = csonpath.CsonPath('$.items[?v > "2"]')
    assert cp.find_all(d) is None


def test_filter_superior_inferior_eq():
    # number >=
    d = {"items": [{"v": 1}, {"v": 5}, {"v": 10}]}
    cp = csonpath.CsonPath('$.items[?v >= 5]')
    assert cp.find_all(d) == [{"v": 5}, {"v": 10}]

    # number <=
    d = {"items": [{"v": 1}, {"v": 5}, {"v": 10}]}
    cp = csonpath.CsonPath('$.items[?v <= 5]')
    assert cp.find_all(d) == [{"v": 1}, {"v": 5}]

    # string >=
    d = {"items": [{"v": "a"}, {"v": "c"}, {"v": "z"}]}
    cp = csonpath.CsonPath('$.items[?v >= "c"]')
    assert cp.find_all(d) == [{"v": "c"}, {"v": "z"}]

    # string <=
    d = {"items": [{"v": "a"}, {"v": "c"}, {"v": "z"}]}
    cp = csonpath.CsonPath('$.items[?v <= "c"]')
    assert cp.find_all(d) == [{"v": "a"}, {"v": "c"}]

    # mix str/num with >= and <=
    d = {"items": [{"v": 5}]}
    cp = csonpath.CsonPath('$.items[?v >= "2"]')
    assert cp.find_all(d) is None

    d = {"items": [{"v": 5}]}
    cp = csonpath.CsonPath('$.items[?v <= "2"]')
    assert cp.find_all(d) is None

    # subpath with >= and <= (different code path than inline)
    d = {"items": [{"v": 10}, {"v": 20}], "threshold": 15}
    cp = csonpath.CsonPath('$.items[?v >= $.threshold]')
    assert cp.find_all(d) == [{"v": 20}]

    d = {"items": [{"v": 10}, {"v": 20}], "threshold": 15}
    cp = csonpath.CsonPath('$.items[?v <= $.threshold]')
    assert cp.find_all(d) == [{"v": 10}]


def test_filter_exist():
    # exist simple
    d = {"items": [{"name": "Alice"}, {"age": 30}, {"name": "Bob"}]}
    cp = csonpath.CsonPath('$.items[?(@.name)]')
    assert cp.find_all(d) == [{"name": "Alice"}, {"name": "Bob"}]

    # exist with nested field
    d = {"items": [{"a": {"b": 1}}, {"a": {}}, {"x": 1}]}
    cp = csonpath.CsonPath('$.items[?(@.a.b)]')
    assert cp.find_all(d) == [{"a": {"b": 1}}]

    # exist returning empty
    d = {"items": [{"x": 1}, {"y": 2}]}
    cp = csonpath.CsonPath('$.items[?(@.z)]')
    assert cp.find_all(d) is None

    # exist with find_first
    d = {"items": [{"name": "Alice"}, {"age": 30}]}
    cp = csonpath.CsonPath('$.items[?(@.name)]')
    assert cp.find_first(d) == {"name": "Alice"}


def test_triple_eq():
    d = {"items": [{"v": 1}, {"v": 5}, {"v": 10}]}
    cp = csonpath.CsonPath('$.items[?v === 5]')
    assert cp.find_all(d) == [{"v": 5}]