import copy

import csonpath


def test_fuzzer_checkpoints_original():
    obj = {
        "mydict": {"str": "hello", "num": 42},
        "items": ["a", "b", "c"],
        "root_str": "world",
    }
    checkpoints = {
        0: {
            "mydict": {"str": "hello", "num": 42},
            "items": ["a", "b", "c"],
            "root_str": "world",
        },
        1: {
            "mydict": ["hello", 42],
            "items": ["a", "b", "c"],
            "root_str": "world",
        },
        2: {
            "mydict": {"str": "42", "num": 42},
            "items": ["a", "b", "c"],
            "root_str": "world",
        },
        3: {
            "mydict": {"str": 0, "num": 42},
            "items": ["a", "b", "c"],
            "root_str": "world",
        },
        4: {
            "mydict": {"str": "hello", "num": 42},
            "items": {"0": "a", "1": "b", "2": "c"},
            "root_str": "world",
        },
        5: {
            "mydict": {"str": "hello", "num": 42},
            "items": ["null", "b", "c"],
            "root_str": "world",
        },
        6: {
            "mydict": {"str": "hello", "num": 42},
            "items": [None, "b", "c"],
            "root_str": "world",
        },
        7: {
            "mydict": {"str": "hello", "num": 42},
            "items": ["a", "b", "c"],
            "root_str": "",
        },
        8: {
            "mydict": {"str": "hello", "num": 42},
            "items": ["a", "b", "c"],
            "root_str": None,
        },
        9: {
            "mydict": {"str": "hello", "num": 42},
            "items": ["a", "", "c"],
            "root_str": "world",
        },
        10: {
            "mydict": {"str": "hello", "num": 42},
            "items": ["a", True, "c"],
            "root_str": "world",
        },
        11: {
            "mydict": {"str": "hello", "num": 42},
            "items": ["a", "b", "true"],
            "root_str": "world",
        },
        12: {
            "mydict": {"str": "hello", "num": 42},
            "items": ["a", "b", 42],
            "root_str": "world",
        },
    }

    f = csonpath.Fuzzer(
        ["$.mydict.str", "$.mydict", "$.items[0]", "$.root_str", "$.items[*]"],
        seed=12345,
        obj=obj,
    )

    for step in range(13):
        if step in checkpoints:
            assert obj == checkpoints[step], f"step {step} mismatch: {obj} != {checkpoints[step]}"
        f.step()


def test_fuzzer_checkpoints_nested_deep():
    obj = {"a": {"b": {"c": {"d": "deep"}}}}
    checkpoints = {
        0: {"a": {"b": {"c": {"d": "deep"}}}},
        1: {"a": [{"c": {"d": "deep"}}]},
        2: {"a": {"b": [{"d": "deep"}]}},
        3: {"a": {"b": {"c": ["deep"]}}},
        4: {"a": {"b": {"c": {"d": "true"}}}},
        5: {"a": {"b": {"c": {"d": 42}}}},
    }

    f = csonpath.Fuzzer(["$.a.b.c.d"], seed=42, obj=obj)

    for step in range(6):
        if step in checkpoints:
            assert obj == checkpoints[step], f"step {step} mismatch: {obj} != {checkpoints[step]}"
        f.step()


def test_fuzzer_checkpoints_array_wildcard_and_index():
    obj = {"arr": [0, 1, 2, 3, 4, 5]}
    checkpoints = {
        0: {"arr": [0, 1, 2, 3, 4, 5]},
        1: {"arr": {"0": 0, "1": 1, "2": 2, "3": 3, "4": 4, "5": 5}},
        2: {"arr": [0, 1, 2, 3, 4, 5]},
    }

    f = csonpath.Fuzzer(["$.arr[*]", "$.arr[4]"], seed=99, obj=obj)

    for step in range(3):
        if step in checkpoints:
            assert obj == checkpoints[step], f"step {step} mismatch: {obj} != {checkpoints[step]}"
        f.step()


def test_fuzzer_checkpoints_multiple_object_keys():
    obj = {"x": {"a": 1, "b": 2, "c": 3}}
    checkpoints = {
        0: {"x": {"a": 1, "b": 2, "c": 3}},
        1: {"x": [1, 2, 3]},
    }

    f = csonpath.Fuzzer(["$.x.a", "$.x.b", "$.x.c"], seed=7, obj=obj)

    for step in range(3):
        if step in checkpoints:
            assert obj == checkpoints[step], f"step {step} mismatch: {obj} != {checkpoints[step]}"
        f.step()


def test_fuzzer_checkpoints_filter():
    obj = {"users": [{"name": "alice", "age": 30}, {"name": "bob", "age": 25}]}
    checkpoints = {
        0: {"users": [{"name": "alice", "age": 30}, {"name": "bob", "age": 25}]},
        1: {"users": {"0": {"name": "alice", "age": 30}, "1": {"name": "bob", "age": 25}}},
    }

    f = csonpath.Fuzzer(["$.users[?(@.age > 25)].name"], seed=1, obj=obj)

    for step in range(3):
        if step in checkpoints:
            assert obj == checkpoints[step], f"step {step} mismatch: {obj} != {checkpoints[step]}"
        f.step()


def test_fuzzer_parent_seen_once():
    """A target receives all its applicable actions before the fuzzer moves on."""
    obj = {"str": "hello"}
    actions = []

    f = csonpath.Fuzzer(["$.str"], seed=12345, obj=obj)

    for _ in range(20):
        a = f.step()
        actions.append(a)
        if a == csonpath.NO_ACTION_LEFT:
            break

    assert actions[0] == csonpath.MODIFY_STR
    assert actions[1] == csonpath.MODIFY_STR_TYPE
    assert actions[2] == csonpath.NO_ACTION_LEFT


def test_fuzzer_getall_visits_all_array_children():
    obj = {"items": ["a", "b", "c"]}
    actions = []

    f = csonpath.Fuzzer(["$.items[*]"], seed=12345, obj=obj)

    for _ in range(20):
        a = f.step()
        actions.append(a)
        if a == csonpath.NO_ACTION_LEFT:
            break

    assert actions.count(csonpath.MODIFY_STR) == 3
    assert actions.count(csonpath.MODIFY_STR_TYPE) == 3
    assert actions[-1] == csonpath.NO_ACTION_LEFT


def test_fuzzer_getall_visits_all_object_children():
    obj = {"x": {"a": "1", "b": "2", "c": "3"}}
    actions = []

    f = csonpath.Fuzzer(["$.x.*"], seed=1, obj=obj)

    for _ in range(20):
        a = f.step()
        actions.append(a)
        if a == csonpath.NO_ACTION_LEFT:
            break

    assert actions.count(csonpath.MODIFY_STR) == 3
    assert actions.count(csonpath.MODIFY_STR_TYPE) == 3
    assert actions[-1] == csonpath.NO_ACTION_LEFT


def test_fuzzer_getall_nested():
    obj = {"foo": [{"x": "a"}, {"x": "b"}]}
    actions = []

    f = csonpath.Fuzzer(["$.foo.*.x"], seed=1, obj=obj)

    for _ in range(20):
        a = f.step()
        actions.append(a)
        if a == csonpath.NO_ACTION_LEFT:
            break

    # two objects, each has one string child: 2 × 2 actions + NO_ACTION_LEFT
    assert actions.count(csonpath.MODIFY_STR) == 2
    assert actions.count(csonpath.MODIFY_STR_TYPE) == 2
    assert actions[-1] == csonpath.NO_ACTION_LEFT


def test_fuzzer_dry_run_no_mutation():
    obj = {
        "mydict": {"str": "hello", "num": 42},
        "items": ["a", "b", "c"],
        "root_str": "world",
    }
    original = copy.deepcopy(obj)

    f = csonpath.Fuzzer(
        ["$.mydict.str", "$.mydict", "$.items[0]", "$.root_str", "$.items[*]"],
        seed=12345,
        obj=obj,
        dry_run=True,
    )

    for _ in range(100):
        if f.step() == csonpath.NO_ACTION_LEFT:
            break

    assert obj == original


def test_fuzzer_dry_run_same_action_sequence():
    paths = ["$.mydict.str", "$.mydict", "$.items[0]", "$.root_str", "$.items[*]"]
    seed = 12345
    original = {
        "mydict": {"str": "hello", "num": 42},
        "items": ["a", "b", "c"],
        "root_str": "world",
    }

    obj_normal = copy.deepcopy(original)
    obj_dry = copy.deepcopy(original)

    f_normal = csonpath.Fuzzer(paths, seed=seed, obj=obj_normal)
    f_dry = csonpath.Fuzzer(paths, seed=seed, obj=obj_dry, dry_run=True)

    actions_normal = []
    actions_dry = []
    for _ in range(100):
        a_normal = f_normal.step()
        a_dry = f_dry.step()
        actions_normal.append(a_normal)
        actions_dry.append(a_dry)
        if a_normal == csonpath.NO_ACTION_LEFT or a_dry == csonpath.NO_ACTION_LEFT:
            break

    assert actions_normal == actions_dry
    assert obj_dry == original


def test_fuzzer_dry_run_with_skip_objects():
    obj = {
        "mydict": {"str": "hello", "num": 42},
        "items": ["a", "b", "c"],
    }
    original = copy.deepcopy(obj)

    f = csonpath.Fuzzer(
        ["$.mydict.str", "$.mydict", "$.items[*]"],
        seed=12345,
        obj=obj,
        skip_objects=[obj["mydict"]],
        dry_run=True,
    )

    actions = []
    for _ in range(100):
        a = f.step()
        actions.append(a)
        if a == csonpath.NO_ACTION_LEFT:
            break

    # In dry-run mode the object must remain unchanged regardless of skip_objects.
    assert obj == original
    assert actions[-1] == csonpath.NO_ACTION_LEFT


def test_fuzzer_skip_objects_direct_target():
    """skip_objects prevents a path that ends exactly on the skipped object from mutating it."""
    obj = {"mydict": {"str": "hello"}}
    original = copy.deepcopy(obj)

    f = csonpath.Fuzzer(
        ["$.mydict"],
        seed=12345,
        obj=obj,
        skip_objects=[obj["mydict"]],
    )

    assert f.step() == csonpath.NO_ACTION_LEFT
    assert obj == original
