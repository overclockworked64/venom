import subprocess
import pytest
import textwrap

from tests.util import VALGRIND_CMD
from tests.util import TWO_OPERANDS_GROUP


@pytest.mark.parametrize(
    "a, b",
    TWO_OPERANDS_GROUP,
)
def test_assignment(a, b):
    source = textwrap.dedent(
        f"""\
        let x = {a};
        print x;
        x = {b};
        print x;"""
    )
    
    process = subprocess.run(
        VALGRIND_CMD,
        capture_output=True,
        input=source.encode('utf-8')
    )
    for value in [a, b]:
        assert f"dbg print :: {value:.2f}\n".encode('utf-8') in process.stdout
        assert process.returncode == 0