import csonpath


def test_simple_source():
    jp = csonpath.CsonPath("$[*].a")
    d = [{"a":0}, {"b":40},{"c": [1,2,3]},{"a":0},{"b":5}]
    
    all=jp.find_all(d)
    assert(len(all) == 2)

def test_multiple_sources():
    jp = csonpath.CsonPath("$[*].a + $[*].c")
    d = [{"a":0}, {"b":40},{"c": [1,2,3]},{"a":0},{"b":5}]
    
    all=jp.find_all(d)
    assert(len(all) == 3)

