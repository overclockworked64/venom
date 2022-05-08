VALGRIND_CMD = [
    "valgrind",
    "--leak-check=full",
    "--show-leak-kinds=all",
    "--error-exitcode=1",
    "./a.out"
]

SINGLE_OPERAND_GROUP = [1, 3, 23, -23, 3.14, -3.14, 0, 100, -100, 5]

TWO_OPERANDS_GROUP = [
    # 1-digit operands ops
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

THREE_OPERANDS_GROUP = [
    [2, 3, 4],
    [-1, 2, 3],
    [4, -5, 6],
    [7, 8, -9],
    [10, 8, 9],
    [-10, 8, 9],
    [10, -8, 9],
    [10, 8, -9],
]
