import subprocess
import pytest
import textwrap

from tests.util import VALGRIND_CMD
from tests.util import TWO_OPERANDS_GROUP


@pytest.mark.parametrize(
    "x, y",
    TWO_OPERANDS_GROUP,
)
def test_block(x, y):
    source = textwrap.dedent(
        f"""{{ let z = {x} + {y}; print z; }}"""
    )
    
    process = subprocess.run(
        VALGRIND_CMD,
        capture_output=True,
        input=source.encode('utf-8')
    )

    assert f"dbg print :: {x + y:.2f}\n".encode('utf-8') in process.stdout
    assert process.returncode == 0
