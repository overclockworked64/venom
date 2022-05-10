import subprocess
import pytest
import textwrap

from tests.util import VALGRIND_CMD
from tests.util import TWO_OPERANDS_GROUP


@pytest.mark.parametrize(
    "x, y",
    TWO_OPERANDS_GROUP,
)
def test_equality(x, y):
    source = textwrap.dedent(
        f"""\
        let x = {x};
        print x == {y};
        """
    )
    
    process = subprocess.run(
        VALGRIND_CMD,
        capture_output=True,
        input=source.encode('utf-8')
    )

    expected = 'true' if x == y else 'false'

    assert f"dbg print :: {expected}\n".encode('utf-8') in process.stdout
    assert process.returncode == 0
