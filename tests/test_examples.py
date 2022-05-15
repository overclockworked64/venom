import subprocess
from pathlib import Path


EXAMPLES_PATH = Path('examples')

def test_examples():
    for file in EXAMPLES_PATH.iterdir():
        process = subprocess.run(
            [
                "valgrind",
                "--leak-check=full",
                "--show-leak-kinds=all",
                "--error-exitcode=1",
                "./a.out",
                file,
            ],
            capture_output=True,
        )
        
        expected = 1

        assert f"dbg print :: {expected:.2f}\n".encode('utf-8') in process.stdout
        assert process.returncode == 0