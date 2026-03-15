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

def main():
    print("\n=== Cramer's Rule Solver ===")
    print("This tool solves systems of linear equations using Cramer's rule.")
    
    while True:
        try:
            n_vars = int(input("\nEnter the number of variables (1-3): ").strip())
            if n_vars < 1 or n_vars > 3:
                print("Please enter a number between 1 and 3.")
                continue
            break
        except ValueError:
            print("Invalid input. Please enter an integer between 1 and 3.")

    n_rows = n_vars
    n_cols = n_vars + 1

    matrix = get_matrix_input(n_rows, n_cols)

    try:
        solutions = solve_discriminant(matrix)
        print("\nSolutions:")
        for i, sol in enumerate(solutions):
            print(f"  Variable {i + 1}: {sol:.4f}")
    except ValueError as e:
        print(f"\n[!] Error: {e}")