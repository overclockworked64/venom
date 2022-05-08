import subprocess
import pytest
import textwrap

from tests.util import VALGRIND_CMD
from tests.util import SINGLE_OPERAND_GROUP

@pytest.mark.parametrize(
    "x",
    SINGLE_OPERAND_GROUP,
)
def test_assignment(x):
    for i in range(10):
        source = textwrap.dedent(
            f"""\
            let x = {'-' * i + str(x)};
            print x;
            """
        )
        
        process = subprocess.run(VALGRIND_CMD,
            capture_output=True,
            input=source.encode('utf-8')
        )
        assert (f"dbg print :: " + 
            f"{'-' if (x == 0 and i % 2 != 0) else ''}" + 
            f"{eval('-' * i + str(x)):.2f}\n").encode('utf-8') in process.stdout
        assert process.returncode == 0