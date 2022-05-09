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
    for minus_count in range(10):
        source = textwrap.dedent(
            f"""\
            let x = {'-' * minus_count + str(x)};
            print x;
            """
        )
        
        process = subprocess.run(
            VALGRIND_CMD,
            capture_output=True,
            input=source.encode('utf-8')
        )

        assert ("dbg print :: " + 
            # for x == 0 and for every odd number of minuses,
            # the result is going to have a minus in front. 
            f"{'-' if (x == 0 and minus_count % 2 != 0) else ''}" +
            f"{eval('-' * minus_count + str(x)):.2f}\n").encode('utf-8') in process.stdout
        assert process.returncode == 0