import textwrap
import subprocess
import pytest

@pytest.mark.parametrize(
    "value",
    [1, -1, 23, -23, 3.14, -3.14, 0, 100, -100],
)
def test_declarations(value):
    source = textwrap.dedent(
        f"""\
        let x = {value};
        print x;"""
    )
    expected = '%.2f' % value
    output = subprocess.check_output(["./a.out"], input=source.encode('utf-8'))
    assert "dbg print :: {}\n".format(expected).encode('utf-8') in output


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
def test_declarations_with_expressions(a, b):
    for op in {'+', '-', '*', '/'}:
        source = textwrap.dedent(
            f"""\
            let x = {a} {op} {b};
            print x;"""
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