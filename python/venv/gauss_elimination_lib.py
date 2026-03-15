import copy

def solve_gauss_jordan(matrix):
    """
    Pure function to solve an augmented matrix using Gauss-Jordan elimination.
    Returns the solution as a list of floats, or None if no unique solution exists.
    """
    generator = solve_gauss_jordan_steps(matrix)
    result = None
    for step in generator:
        if step['type'] == 'solution':
            result = step['data']
    return result

def solve_gauss_jordan_steps(matrix):
    """
    A generator that yields the steps of Gauss-Jordan elimination.
    Useful for building UIs, calculators, or step-by-step CLIs.
    Yields dictionaries with 'type', 'msg', 'matrix', and other step-specific data.
    """
    mat = copy.deepcopy(matrix)
    n_rows = len(mat)
    n_cols = len(mat[0])

    yield {'type': 'start', 'msg': 'Starting augmented matrix', 'matrix': copy.deepcopy(mat)}

    for col in range(n_rows):
        yield {'type': 'col_start', 'col': col, 'msg': f"Working on column {col + 1}", 'matrix': copy.deepcopy(mat)}

        # Swap
        if mat[col][col] == 0:
            swapped = False
            for swap_row in range(col + 1, n_rows):
                if mat[swap_row][col] != 0:
                    mat[col], mat[swap_row] = mat[swap_row], mat[col]
                    yield {
                        'type': 'swap', 
                        'row1': col, 
                        'row2': swap_row, 
                        'msg': f"R{col+1} <-> R{swap_row+1}", 
                        'matrix': copy.deepcopy(mat)
                    }
                    swapped = True
                    break
            if not swapped:
                yield {'type': 'error', 'msg': 'No valid pivot found. The system may have no unique solution.', 'matrix': copy.deepcopy(mat)}
                yield {'type': 'solution', 'data': None}
                return

        # Scale
        pivot_val = mat[col][col]
        if pivot_val != 1.0:
            mat[col] = [x / pivot_val for x in mat[col]]
            yield {
                'type': 'scale', 
                'row': col, 
                'factor': pivot_val, 
                'msg': f"R{col+1}  ->  (1 / {pivot_val:.4f}) * R{col+1}", 
                'matrix': copy.deepcopy(mat)
            }

        # Eliminate
        for row in range(n_rows):
            if row == col:
                continue
            factor = mat[row][col]
            if factor != 0:
                mat[row] = [mat[row][i] - factor * mat[col][i] for i in range(n_cols)]
                sign = "-" if factor >= 0 else "+"
                yield {
                    'type': 'eliminate', 
                    'row': row, 
                    'pivot_row': col, 
                    'factor': factor, 
                    'msg': f"R{row+1}  ->  R{row+1}  {sign}  ({abs(factor):.4f} * R{col+1})", 
                    'matrix': copy.deepcopy(mat)
                }

    # Check solution
    solution = []
    for row in range(n_rows):
        left_all_zero = all(abs(mat[row][i]) < 1e-9 for i in range(n_rows))
        if left_all_zero:
            if abs(mat[row][-1]) > 1e-9:
                yield {'type': 'error', 'msg': 'Inconsistent system - No solution exists.', 'matrix': copy.deepcopy(mat)}
                yield {'type': 'solution', 'data': None}
                return
            else:
                yield {'type': 'error', 'msg': 'Infinite solutions exist (dependent system).', 'matrix': copy.deepcopy(mat)}
                yield {'type': 'solution', 'data': None}
                return
        solution.append(mat[row][-1])

    yield {'type': 'done', 'msg': 'Reduced Row Echelon Form (RREF) reached', 'matrix': copy.deepcopy(mat)}
    yield {'type': 'solution', 'data': solution}

def verify_solution(original_matrix, solution):
    """
    Verifies if a given solution satisfies the original augmented matrix.
    Returns (all_correct, results_list)
    """
    n_vars = len(solution)
    all_correct = True
    results = []

    for i, row in enumerate(original_matrix):
        lhs = sum(row[j] * solution[j] for j in range(n_vars))
        rhs = row[-1]
        ok = abs(lhs - rhs) < 1e-6
        if not ok:
            all_correct = False
        results.append({
            'equation_idx': i,
            'lhs': lhs,
            'rhs': rhs,
            'ok': ok,
            'row': row
        })
        
    return all_correct, results
