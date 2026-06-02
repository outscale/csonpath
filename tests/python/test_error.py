import unittest
import csonpath

# o = csonpath.CsonPath(".c")


def broken_function():
    raise Exception("This is broken")


class CsonPathUpdateOrCreateError(unittest.TestCase):

    # --- root ---
    def test_uoc_root_scalar(self):
        o = csonpath.CsonPath("$")
        for inp in (42, "hello", 1):
            with self.subTest(inp=inp):
                with self.assertRaises(ValueError) as cm:
                    o.update_or_create(inp, "x")
                self.assertIn("can't update root ($)", str(cm.exception))

    def test_update_self(self):
        o = csonpath.CsonPath("$")
        d = {"c": "not-useful"}
        e = "hej hej"

        with self.assertRaises(ValueError) as cm:
            o.update_or_create(d, e)
        self.assertIn("can't update root ($)", str(cm.exception))

    # --- parents scalaires intermédiaires ---
    def test_uoc_scalar_then_dict_access(self):
        o = csonpath.CsonPath("$.a.b")
        with self.assertRaises(ValueError) as cm:
            o.update_or_create({"a": 1}, "x")
        self.assertIn("Dict expected", str(cm.exception))

    def test_uoc_scalar_then_array_access(self):
        o = csonpath.CsonPath("$.a[0]")
        with self.assertRaises(ValueError) as cm:
            o.update_or_create({"a": 1}, "x")
        self.assertIn("List expected", str(cm.exception))

    def test_uoc_string_then_dict_access(self):
        o = csonpath.CsonPath("$.a.b")
        with self.assertRaises(ValueError) as cm:
            o.update_or_create({"a": "x"}, "x")
        self.assertIn("Dict expected", str(cm.exception))

    def test_uoc_string_then_array_access(self):
        o = csonpath.CsonPath("$.a[0]")
        with self.assertRaises(ValueError) as cm:
            o.update_or_create({"a": "x"}, "x")
        self.assertIn("List expected", str(cm.exception))

    def test_uoc_nested_scalar(self):
        o = csonpath.CsonPath("$.a.b.c")
        with self.assertRaises(ValueError) as cm:
            o.update_or_create({"a": {"b": 1}}, "x")
        self.assertIn("Dict expected", str(cm.exception))

    def test_uoc_list_parent_for_obj(self):
        o = csonpath.CsonPath("$.a.b")
        with self.assertRaises(ValueError) as cm:
            o.update_or_create({"a": []}, "x")
        self.assertIn("Dict expected", str(cm.exception))

    # --- subpath ---
    def test_uoc_subpath_returns_array(self):
        o = csonpath.CsonPath("$.a[$.c]")
        with self.assertRaises(ValueError) as cm:
            o.update_or_create({"a": [1], "c": [1]}, "x")
        self.assertEqual(
            "GET_SUBPATH return need to be either number or str",
            str(cm.exception),
        )

    def test_uoc_subpath_returns_dict(self):
        o = csonpath.CsonPath("$.a[$.c]")
        with self.assertRaises(ValueError) as cm:
            o.update_or_create({"a": [0,1], "c": {"x": 1}}, "x")
        self.assertEqual(
            "GET_SUBPATH return need to be either number or str",
            str(cm.exception),
        )


class CsonPathError(unittest.TestCase):
    def test_error(self):
        with self.assertRaises(ValueError):
            o = csonpath.CsonPath("$.[]")

    def test_wrong_path(self):
        path = "$.a.b.c"
        o = csonpath.CsonPath(path)

        d = {"a": [{}]}
        assert o.find_all(d) == None
        with self.assertRaises(ValueError):
            o.update_or_create(d, [])


if __name__ == "__main__":
    unittest.main()
