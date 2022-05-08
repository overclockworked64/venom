import textwrap
import subprocess
import pytest

from tests.util import VALGRIND_CMD
from tests.util import SINGLE_OPERAND_GROUP
from tests.util import TWO_OPERANDS_GROUP


@pytest.mark.parametrize(
    "value",
    SINGLE_OPERAND_GROUP,
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
        expected = f"{value:.2f}"
        process = subprocess.run(
            VALGRIND_CMD,
            capture_output=True,
            input=source.encode('utf-8')
        )
        assert f"dbg print :: {expected}\n".encode('utf-8') in process.stdout
        assert process.returncode == 0


@pytest.mark.parametrize(
    "value",
    SINGLE_OPERAND_GROUP,
)
def test_printing_declared_variable(value):
    source = textwrap.dedent(
        f"""\
        let x = {value};
        print x + 1;"""
    )
    expected = '%.2f' % (value + 1)
    process = subprocess.run(
        VALGRIND_CMD,
        capture_output=True,
        input=source.encode('utf-8')
    )
    assert f"dbg print :: {expected}\n".encode('utf-8') in process.stdout
    assert process.returncode == 0


@pytest.mark.parametrize(
    "x, y",
    TWO_OPERANDS_GROUP,
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
        process = subprocess.run(
            VALGRIND_CMD,
            capture_output=True,
            input=source.encode('utf-8')
        )
        assert f"dbg print :: {expected}\n".encode('utf-8') in process.stdout
        assert process.returncode == 0



@pytest.mark.parametrize(
    "a, b",
    TWO_OPERANDS_GROUP,
)
def test_declarations_with_expressions(a, b):
    for op in {'+', '-', '*', '/'}:
        source = textwrap.dedent(
            f"""\
            let x = {a} {op} {b};
            print x;"""
        )
        expected = "%.2f" % eval(f"{a} {op} {b}")
        process = subprocess.run(
            VALGRIND_CMD,
            capture_output=True,
            input=source.encode('utf-8')
        )
        assert f"dbg print :: {expected}\n".encode('utf-8') in process.stdout
        assert process.returncode == 0



@pytest.mark.parametrize(
    "a, b",
    TWO_OPERANDS_GROUP,
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
        process = subprocess.run(
            VALGRIND_CMD,
            capture_output=True,
            input=source.encode('utf-8')
        )
        assert f"dbg print :: {expected}\n".encode('utf-8') in process.stdout
        assert process.returncode == 0
