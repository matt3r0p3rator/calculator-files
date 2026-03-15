import copy
from gauss_elimination_numpy import solve_gauss_jordan_steps, verify_solution

# ==================================================
# DISPLAY HELPERS
# ==================================================

def print_matrix(matrix, var_labels):
    """Pretty print the augmented matrix with variable labels."""
    n_cols = len(matrix[0])
    n_vars = n_cols - 1

    # Header
    header = "  ".join(f"{v:>8}" for v in var_labels[:n_vars]) + "  " + f"{'|':>4}" + f"{'const':>8}"
    print(f"    {header}")
    print(f"    {'-' * len(header)}")

    for row in matrix:
        left  = "  ".join(f"{val:8.4f}" for val in row[:-1])
        right = f"{row[-1]:8.4f}"
        print(f"  [ {left}   |  {right} ]")
    print()


def print_step_header(step_num, description):
    print(f"     Step {step_num}: {description}")


def print_operation(op):
    print(f"     >  {op}")


# ==================================================
# INPUT HELPERS
# ==================================================

def get_matrix_size():
    """Ask the user for the size of the augmented matrix."""
    print("\n" + "=" * 39)
    print("| GAUSS-JORDAN ELIMINATION SOLVER (NUMPY) |")
    print("=" * 39)
    print("\n  Enter the number of equations and variables (e.g. 2 2 for 2 equations with 2 unknowns).")
    while True:
        try:
            raw = input("\n  Enter dimensions (rows cols): ").strip().split()
            if len(raw) != 2:
                print("  [!] Please enter exactly two numbers (e.g. '2 3'). Try again.")
                continue
            n_rows, n_cols = int(raw[0]), int(raw[1])
            if n_rows < 1 or n_cols < 2:
                print("  [!] Rows must be >= 1 and columns must be >= 2 (including constants). Try again.")
                continue
            return n_rows, n_cols
        except ValueError:
            print("  [!] Invalid input. Please enter integers only (e.g. '3 4').")


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


# ==================================================
# CORE ALGORITHM WRAPPER
# ==================================================

def gauss_jordan(matrix, var_labels):
    """
    Perform Gauss-Jordan elimination on an augmented matrix.
    Uses the underlying math library to process and yield steps,
    allowing the original interactive CLI experience while being modular.
    """
    step_num  = 1
    solution = None

    for step in solve_gauss_jordan_steps(matrix):
        if step['type'] == 'start':
            print("\n" + "=" * 54)
            print("   STARTING AUGMENTED MATRIX")
            print("=" * 54)
            print_matrix(step['matrix'], var_labels)

        elif step['type'] == 'col_start':
            print("=" * 54)
            print(f"     WORKING ON COLUMN {step['col'] + 1}  (variable: {var_labels[step['col']]})")
            print("=" * 54 + "\n")

        elif step['type'] == 'swap':
            print_step_header(
                step_num,
                f"Pivot at [{step['row1']+1},{step['row1']+1}] = 0 -> search for a non-zero row to swap"
            )
            step_num += 1
            print_operation(step['msg'])
            print_matrix(step['matrix'], var_labels)

        elif step['type'] == 'scale':
            print_step_header(
                step_num,
                f"Scale R{step['row']+1} so that pivot [{step['row']+1},{step['row']+1}] becomes 1"
            )
            step_num += 1
            print_operation(step['msg'])
            print_matrix(step['matrix'], var_labels)

        elif step['type'] == 'eliminate':
            direction = "below" if step['row'] > step['pivot_row'] else "above"
            print_step_header(
                step_num,
                f"Eliminate entry [{step['row']+1},{step['pivot_row']+1}] = {step['factor']:.4f} ({direction} pivot)"
            )
            step_num += 1
            print_operation(step['msg'])
            print_matrix(step['matrix'], var_labels)

        elif step['type'] == 'error':
            if 'matrix' in step and step['matrix'] is not None:
                pass # Depending on if we want to print matrix on error
            print(f"     {step['msg']}\n")

        elif step['type'] == 'done':
            print("=" * 54)
            print("REDUCED ROW ECHELON FORM (RREF) REACHED")
            print("=" * 54)
            print_matrix(step['matrix'], var_labels)

        elif step['type'] == 'solution':
            solution = step['data']
            if solution is not None:
                print("SOLUTION:")
                for i, val in enumerate(solution):
                    print(f"     {var_labels[i]} = {val:.6f}")
                print()

    return solution


# ==================================================
# VERIFICATION WRAPPER
# ==================================================

def verify_solution_cli(original_matrix, solution, var_labels):
    """Plug the solution back into the original equations and check using the library."""
    print("=" * 54)
    print("      VERIFICATION - Plugging solution back in")
    print("=" * 54)
    
    all_correct, results = verify_solution(original_matrix, solution)
    n_vars = len(solution)

    for res in results:
        i = res['equation_idx']
        status = "Good" if res['ok'] else "Bad"
        row = res['row']
        
        terms = " + ".join(
            f"({row[j]:.4f} * {var_labels[j]}={solution[j]:.4f})"
            for j in range(n_vars)
        )
        print(f"  {status}  Eq{i+1}:  {terms}  =  {res['lhs']:.6f}  (expected {res['rhs']:.4f})")

    print()
    if all_correct:
        print("All equations satisfied. Solution is correct!\n")
    else:
        print("One or more equations failed. Check your input.\n")


# ==================================================
# MAIN
# ==================================================

def main():
    # 1. Get matrix dimensions
    n_rows, n_cols = get_matrix_size()
    n_vars         = n_cols - 1
    var_labels     = ['x', 'y', 'z'][:n_vars]

    # 2. Get matrix input from user
    matrix = get_matrix_input(n_rows, n_cols)

    # 3. Run Gauss-Jordan and show all steps using wrapper
    original = copy.deepcopy(matrix)
    solution = gauss_jordan(matrix, var_labels)

    # 4. Verify if we got a solution
    if solution:
        verify_solution_cli(original, solution, var_labels)

    input("  Press Enter to exit...")


if __name__ == "__main__":
    main()
