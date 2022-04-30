import textwrap
import subprocess
import pytest

@pytest.mark.parametrize(
    "value, expected",
    [
        [1, "1.00"],
        [-1, "-1.00"],
        [23, "23.00"],
        [-23, "-23.00"],
        [3.14, "3.14"],
        [-3.14, "-3.14"],
        [0, "0.00"],
        [100, "100.00"],
        [-100, "-100.00"],
    ]
)
def test_declarations(value, expected):
    source = textwrap.dedent(
        f"""\
        let x = {value};
        print x;"""
    )
    output = subprocess.check_output(["./a.out"], input=source.encode('utf-8'))
    assert "dbg print :: {}\n".format(expected).encode('utf-8') in output


@pytest.mark.parametrize(
    "a, op, b, expected",
    [   # 1-digit operands ops
        [2,  "+", 2,  "4.00"],
        [2,  "-", 2,  "0.00"],
        [2,  "*", 2,  "4.00"],
        [4,  "/", 2,  "2.00"],
        [2,  "/", 4,  "0.50"],
        # 2-digit operands ops
        [3,  "+", 10, "13.00"],
        [10, "+", 3,  "13.00"],
        [10, "+", 10, "20.00"],
        [3,  "-", 10, "-7.00"],
        [10, "-", 3,  "7.00"],
        [10, "-", 10, "0.00"],
        [3,  "*", 10, "30.00"],
        [10, "*", 3,  "30.00"],
        [10, "/", 2,  "5.00"],
        [2,  "/", 10, "0.20"],
        # negative operands ops
        [2,  "+", -2, "0.00"],
        [-2, "+", 2,  "0.00"],
        [-2, "+", -2, "-4.00"],
        [-2, "-", 2,  "-4.00"],
        [2,  "-", -2,  "4.00"],
        [-2, "-", -2,  "0.00"],
    ]
)
def test_declarations_with_expressions(a, op, b, expected):
    source = textwrap.dedent(
        f"""\
        let x = {a} {op} {b};
        print x;"""
    )
    output = subprocess.check_output(["./a.out"], input=source.encode('utf-8'))
    assert "dbg print :: {}\n".format(expected).encode('utf-8') in output