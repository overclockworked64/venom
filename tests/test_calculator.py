import subprocess
import itertools
import pytest

from tests.util import VALGRIND_CMD
from tests.util import TWO_OPERANDS_GROUP
from tests.util import THREE_OPERANDS_GROUP


@pytest.mark.parametrize(
    "a, b",
    TWO_OPERANDS_GROUP,
)
def test_calculator(a, b):
    for op in {'+', '-', '*', '/'}:
        source = f"print {a} {op} {b};"
        expected = "%.2f" % eval(f"{a} {op} {b}")
        process = subprocess.run(
            VALGRIND_CMD,
            capture_output=True,
            input=source.encode('utf-8')
        )
        assert f"dbg print :: {expected}\n".encode('utf-8') in process.stdout
        assert process.returncode == 0



@pytest.mark.parametrize(
    "a, b, c",
    THREE_OPERANDS_GROUP,
)
def test_calculator_grouping(a, b, c):
    for op, op2 in itertools.permutations({'+', '-', '*', '/'}, 2):
        source = f"print ({a} {op} {b}) {op2} {c};"
        expected = "%.2f" % eval(f"({a} {op} {b}) {op2} {c}")
        process = subprocess.run(VALGRIND_CMD,
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
        "--error-exitcode=1",
        "./a.out"],
        capture_output=True,
        input=source.encode('utf-8')
    )
    assert "dbg print :: 5.00\n".encode('utf-8') in process.stdout
    assert process.returncode == 0

