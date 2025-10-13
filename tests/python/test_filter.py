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

    ref_before = sys.getrefcount(value)
    ret = cp.update_or_create(dict, value)
    ref_after = sys.getrefcount(value)

    assert ret == 1
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
