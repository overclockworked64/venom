import subprocess
import itertools
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
        source = f"print {a} {op} {b};"
        expected = "%.2f" % eval(f"{a} {op} {b}")
        process = subprocess.run([
            "valgrind",
            "--leak-check=full",
            "--show-leak-kinds=all",
            "./a.out"],
            capture_output=True,
            input=source.encode('utf-8')
        )
        assert f"dbg print :: {expected}\n".encode('utf-8') in process.stdout
        assert process.returncode == 0



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
    for op, op2 in itertools.permutations({'+', '-', '*', '/'}, 2):
        source = f"print ({a} {op} {b}) {op2} {c};"
        expected = "%.2f" % eval(f"({a} {op} {b}) {op2} {c}")
        process = subprocess.run([
            "valgrind",
            "--leak-check=full",
            "--show-leak-kinds=all",
            "./a.out"],
            capture_output=True,
            input=source.encode('utf-8')
        )
        assert f"dbg print :: {expected}\n".encode('utf-8') in process.stdout
        assert process.returncode == 0



def test_calculator_grouping_nested():
    source = "print (((6 + (4 * 2)) - 4) / 2);"
    process = subprocess.run([
        "valgrind",
        "--leak-check=full",
        "--show-leak-kinds=all",
        "./a.out"],
        capture_output=True,
        input=source.encode('utf-8')
    )
    assert "dbg print :: 5.00\n".encode('utf-8') in process.stdout
    assert process.returncode == 0

