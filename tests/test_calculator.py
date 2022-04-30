import textwrap
import subprocess
import pytest

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
def test_calculator(a, op, b, expected):
    source = textwrap.dedent(
        f"""\
        print {a} {op} {b};"""
    )
    output = subprocess.check_output(["./a.out"], input=source.encode('utf-8'))
    assert "dbg print :: {}\n".format(expected).encode('utf-8') in output


@pytest.mark.parametrize(
    "a, op, b, op2, c, expected",
    [
        [2, "*", 3, "*", 4, "24.00"],
        [2, "+", 3, "*", 4, "20.00"],
        [2, "-", 3, "*", 4, "-4.00"],
        [6, "/", 3, "+", 4, "6.00"],
        [6, "/", 3, "/", 2, "1.00"],
        [6, "-", 3, "/", 2, "1.50"],
    ]
)
def test_calculator_grouping(a, op, b, op2, c, expected):
    source = textwrap.dedent(
        f"""\
        print ({a} {op} {b}) {op2} {c};"""
    )
    output = subprocess.check_output(["./a.out"], input=source.encode('utf-8'))
    assert "dbg print :: {}\n".format(expected).encode('utf-8') in output

def test_calculator_grouping_nested():
    source = "print (((6 + (4 * 2)) - 4) / 2);"
    output = subprocess.check_output(["./a.out"], input=source.encode('utf-8'))
    assert "dbg print :: 5.00\n".encode('utf-8') in output
