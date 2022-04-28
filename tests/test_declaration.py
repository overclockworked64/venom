import textwrap
import subprocess
import pytest

def source(x: int) -> str:
    return textwrap.dedent(
        """\
        let x = {};
        print x;""".format(x)
    )

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
    output = subprocess.check_output(["./a.out"], input=source(value).encode('utf-8'))
    assert "dbg print :: {}\n".format(expected).encode('utf-8') in output