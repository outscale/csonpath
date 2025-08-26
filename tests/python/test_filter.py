import pytest
import csonpath

def test_filter():
    dict = {"ar": [{"0": "a 0", "1": "a 1"}, {"ah": "a 0", "bh": "a 10"}]}
    cp = csonpath.CsonPath("$.ar[?0=\"a 0\"]")

    ret = cp.find_first(dict)

    assert ret == {"0": "a 0", "1": "a 1"}, "fail result look like: {}".format(ret)

    ret = cp.find_all(dict)

    assert ret == [{"0": "a 0", "1": "a 1"}], "fail result look like: {}".format(ret)

    ret = cp.update_or_create(dict, ["wololo"])

    assert ret == 1
    assert dict == {"ar": [["wololo"], {"ah": "a 0", "bh": "a 10"}]}, "fail result look like: {}".format(dict["ar"][0])

    cp.set_path("$.ar[?ah=\"a 0\"]")
    ret = cp.remove(dict)
    assert ret == 1
    assert dict == {"ar": [["wololo"]]}, "fail result look like: {}".format(dict["ar"][0])

    dict = {"ha": [ {"h": "Leodagan"}, {"h": "George"} ]}
    cp = csonpath.CsonPath("$.ha[?h != \"Leodagan\"].h")
    ret = cp.find_all(dict)
    assert(ret)
