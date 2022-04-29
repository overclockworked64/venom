import textwrap
import subprocess
import pytest

def source(a: int, b: int, op: str) -> str:
    return textwrap.dedent(
        """\
        print {} {} {};""".format(a, op, b)
    )

@pytest.mark.parametrize(
    "a, op, b, expected",
    [
        [2,  "+", 2,  "4.00"],
        [3,  "+", 10, "13.00"],
        [10, "+", 3,  "13.00"],
        [10, "+", 10, "20.00"],
        [2,  "+", -2, "0.00"],
        [-2, "+", 2,  "0.00"],
        [-2, "+", -2, "-4.00"],
        [2,  "-", 2,  "0.00"],
        [-2, "-", 2,  "-4.00"],
        [2,  "-", -2,  "4.00"],
        [-2, "-", -2,  "0.00"],
        [3,  "-", 10, "-7.00"],
        [10, "-", 3,  "7.00"],
        [10, "-", 10, "0.00"],
        [2,  "*", 2,  "4.00"],
        [3,  "*", 10, "30.00"],
        [10, "*", 3,  "30.00"],
        [4,  "/", 2,  "2.00"],
        [2,  "/", 4,  "0.50"],
        [10, "/", 2,  "5.00"],
        [2,  "/", 10, "0.20"],
    ]
)
def test_declarations(a, op, b, expected):
    output = subprocess.check_output(["./a.out"], input=source(a, b, op).encode('utf-8'))
    assert "dbg print :: {}\n".format(expected).encode('utf-8') in output