import copy
from discriminant_numpy import solve_discriminant, solve_cramer

def get_matrix_input(n_rows, n_cols):
    """
    Prompt the user to enter the augmented matrix row by row.
    n_cols includes the constants column.
    """
    n_vars = n_cols - 1
    labels = ['x', 'y', 'z'][:n_vars]

    print(f"\n  Enter each row as {n_cols} space-separated numbers.")
    print(f"  Format per row:  [{'  '.join(labels)}  constant]")
    print(f"  Example row:     1 2 5   means  1x + 2y = 5\n")

    matrix = []
    for i in range(n_rows):
        while True:
            try:
                raw = input(f"  Row {i + 1}: ").strip().split()
                if len(raw) != n_cols:
                    print(f"  [!] Expected {n_cols} values. Got {len(raw)}. Try again.")
                    continue
                row = [float(v) for v in raw]
                matrix.append(row)
                break
            except ValueError:
                print("  [!] Invalid input. Use numbers only (e.g. 1 -2 3.5 7).")
    return matrix