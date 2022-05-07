import subprocess
import pytest
import textwrap

@pytest.mark.parametrize(
    "x",
    [1, 3, 23, -23, 3.14, -3.14, 0, 100, -100, 5],
)
def test_assignment(x):
    for i in range(10):
        source = textwrap.dedent(
            f"""\
            let x = {'-' * i + str(x)};
            print x;
            """
        )
        
        process = subprocess.run([
            "valgrind",
            "--leak-check=full",
            "--show-leak-kinds=all",
            "--error-exitcode=1",
            "./a.out"],
            capture_output=True,
            input=source.encode('utf-8')
        )
        assert (f"dbg print :: " + 
            f"{'-' if (x == 0 and i % 2 != 0) else ''}" + 
            f"{eval('-' * i + str(x)):.2f}\n").encode('utf-8') in process.stdout
        assert process.returncode == 0