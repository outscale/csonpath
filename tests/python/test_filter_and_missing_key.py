import pytest
import csonpath


def test_filter_and_missing_key_crash():
    """
    Segfault regression for compound filter (&) on arrays where some
    elements miss the key used in the second condition.

    Before the fix the internal `filter_next` offset was not reset
    between array elements, so the bytecode walker overran into the
    FILTER_OPERAND_STR instruction and dereferenced a NULL el2 inside
    PyUnicode_Check.
    """
    data = {
        "hardware": [
            {"type": "fb", "status": "healthy"},
            {"type": "fb"},  # missing 'status'
        ]
    }
    cp = csonpath.CsonPath('$.hardware[?type = "fb" & status = "healthy"]')
    result = []
    def cb(parent, idx, cur, d):
        d.append(cur)

    cp.callback(data, cb, result)

    assert len(result) == 1
    assert result[0] == {"type": "fb", "status": "healthy"}
