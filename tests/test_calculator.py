import textwrap
import subprocess
import pytest


@pytest.mark.parametrize(
    "a, b",
    [   # 1-digit operands ops
        [2, 2],
        [4, 2],
        [2, 4],
        # 2-digit operands ops
        [3, 10],
        [10, 3],
        [10, 10],
        # negative operands ops
        [2, -2],
        [-2, 2],
        [-2, -2],
        [-2, 2],
    ]
)
def test_calculator(a, b):
    for op in {'+', '-', '*', '/'}:
        source = textwrap.dedent(
            f"""\
            print {a} {op} {b};"""
        )
        match op:
            case '+': 
                expected = "%.2f" % (a + b)
            case '-': 
                expected = "%.2f" % (a - b)
            case '*': 
                expected = "%.2f" % (a * b)
            case '/': 
                expected = "%.2f" % (a / b)
        output = subprocess.check_output(["./a.out"], input=source.encode('utf-8'))
        assert "dbg print :: {}\n".format(expected).encode('utf-8') in output


@pytest.mark.parametrize(
    "a, b, c",
    [
        [2, 3, 4],
        [-1, 2, 3],
        [4, -5, 6],
        [7, 8, -9],
        [10, 8, 9],
        [-10, 8, 9],
        [10, -8, 9],
        [10, 8, -9],
    ]
)
def test_calculator_grouping(a, b, c):
    for op in {'+', '-', '*', '/'}:
        for op2 in {'+', '-', '*', '/'}:
            source = textwrap.dedent(
                f"""\
                print ({a} {op} {b}) {op2} {c};"""
            )

            match op:
                case '+': in_parens = (a + b)
                case '-': in_parens = (a - b)
                case '*': in_parens = (a * b)
                case '/': in_parens = (a / b)

            match op2:
                case '+': expected = "%.2f" % (in_parens + c)
                case '-': expected = "%.2f" % (in_parens - c)
                case '*': expected = "%.2f" % (in_parens * c)
                case '/': expected = "%.2f" % (in_parens / c)
            
            output = subprocess.check_output(["./a.out"], input=source.encode('utf-8'))
            assert "dbg print :: {}\n".format(expected).encode('utf-8') in output

def test_calculator_grouping_nested():
    source = "print (((6 + (4 * 2)) - 4) / 2);"
    output = subprocess.check_output(["./a.out"], input=source.encode('utf-8'))
    assert "dbg print :: 5.00\n".encode('utf-8') in output
