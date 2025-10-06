import unittest
import csonpath

# o = csonpath.CsonPath(".c")


def broken_function():
    raise Exception("This is broken")


class CsonPathError(unittest.TestCase):
    def test_error(self):
        with self.assertRaises(ValueError):
            o = csonpath.CsonPath("$.[]")

    def test_update_self(self):
        o = csonpath.CsonPath("$")
        d = {"c": "not-useful"}
        e = "hej hej"

        with self.assertRaises(ValueError):
            o.update_or_create(d, e)

    def test_wrong_path(self):
        path = "$.a.b.c"
        o = csonpath.CsonPath(path)

        d = {"a": [{}]}
        assert o.find_all(d) == None
        with self.assertRaises(ValueError):
            o.update_or_create(d, [])


if __name__ == "__main__":
    unittest.main()
