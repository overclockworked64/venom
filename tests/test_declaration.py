import textwrap
import subprocess
import pytest


@pytest.mark.parametrize(
    "value",
    [1, -1, 23, -23, 3.14, -3.14, 0, 100, -100],
)
def test_declarations(value):
    sources = [
        textwrap.dedent(
            f"""\
            let x = {value};
            print x;"""
        ),
        f"let x = {value}; print x;"
    ]
    for source in sources:
        expected = '%.2f' % value
        process = subprocess.run([
            "valgrind",
            "--leak-check=full",
            "--show-leak-kinds=all",
            "./a.out"],
            capture_output=True,
            input=source.encode('utf-8')
        )
        assert "dbg print :: {}\n".format(expected).encode('utf-8') in process.stdout
        assert process.returncode == 0


@pytest.mark.parametrize(
    "value",
    [1, -1, 23, -23, 3.14, -3.14, 0, 100, -100],
)
def test_printing_declared_variable(value):
    source = textwrap.dedent(
        f"""\
        let x = {value};
        print x + 1;"""
    )
    expected = '%.2f' % (value + 1)
    process = subprocess.run([
        "valgrind",
        "--leak-check=full",
        "--show-leak-kinds=all",
        "./a.out"],
        capture_output=True,
        input=source.encode('utf-8')
    )
    assert "dbg print :: {}\n".format(expected).encode('utf-8') in process.stdout
    assert process.returncode == 0


@pytest.mark.parametrize(
    "x, y",
    [
        [1, -1],
        [23, -23],
        [3.14, -3.14],
        [100, -100],
    ]
)
def test_printing_declared_variables(x, y):
   for op in {'+', '-', '*', '/'}:
        source = textwrap.dedent(
            f"""\
            let x = {x};
            let y = {y};
            print x {op} y + 2;"""
        )
        expected = "%.2f" % eval(f"{x} {op} {y} + 2")
        process = subprocess.run([
            "valgrind",
            "--leak-check=full",
            "--show-leak-kinds=all",
            "./a.out"],
            capture_output=True,
            input=source.encode('utf-8')
        )
        assert "dbg print :: {}\n".format(expected).encode('utf-8') in process.stdout
        assert process.returncode == 0



@pytest.mark.parametrize(
    "a, b",
    [   # 1-digit operands ops
        [2, 2],
        [4, 2],
        [2, 4],
        # 2-digit operands ops
        [3, 10],
        [10, 3],
        [10, 10],
        # negative operands ops
        [2, -2],
        [-2, 2],
        [-2, -2],
    ]
)
def test_declarations_with_expressions(a, b):
    for op in {'+', '-', '*', '/'}:
        source = textwrap.dedent(
            f"""\
            let x = {a} {op} {b};
            print x;"""
        )
        expected = "%.2f" % eval(f"{a} {op} {b}")
        process = subprocess.run([
            "valgrind",
            "--leak-check=full",
            "--show-leak-kinds=all",
            "./a.out"],
            capture_output=True,
            input=source.encode('utf-8')
        )
        assert "dbg print :: {}\n".format(expected).encode('utf-8') in process.stdout
        assert process.returncode == 0



@pytest.mark.parametrize(
    "a, b",
    [   # 1-digit operands ops
        [2, 2],
        [4, 2],
        [2, 4],
        # 2-digit operands ops
        [3, 10],
        [10, 3],
        [10, 10],
        # negative operands ops
        [2, -2],
        [-2, 2],
        [-2, -2],
    ]
)
def test_reuse_declaration(a, b):
    for op in {'+', '-', '*', '/'}:
        source = textwrap.dedent(
            f"""\
            let x = {a} {op} {b};
            print x;
            let x = {a} {op} {b};
            print x;"""
        )
        expected = "%.2f" % eval(f"{a} {op} {b}")
        process = subprocess.run([
            "valgrind",
            "--leak-check=full",
            "--show-leak-kinds=all",
            "./a.out"],
            capture_output=True,
            input=source.encode('utf-8')
        )
        assert "dbg print :: {}\n".format(expected).encode('utf-8') in process.stdout
        assert process.returncode == 0

