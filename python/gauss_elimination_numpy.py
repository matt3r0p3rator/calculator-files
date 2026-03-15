import numpy as np

def solve_gauss_jordan(matrix):
    generator = solve_gauss_jordan_steps(matrix)
    result = None
    for step in generator:
        if step['type'] == 'solution':
            result = step['data']
    return result

def solve_gauss_jordan_steps(matrix):
    # Convert input list to a float64 numpy array
    mat = np.array(matrix, dtype=np.float64)
    n_rows, n_cols = mat.shape

    yield {'type': 'start', 'msg': 'Starting augmented matrix', 'matrix': mat.copy().tolist()}

    for col in range(n_rows):
        yield {'type': 'col_start', 'col': col, 'msg': f"Working on column {col + 1}", 'matrix': mat.copy().tolist()}

        # 1. Swap
        if mat[col, col] == 0:
            # Find the first non-zero element in the column below the pivot
            non_zero_indices = np.where(mat[col+1:, col] != 0)[0]
            if len(non_zero_indices) == 0:
                yield {'type': 'error', 'msg': 'No valid pivot found. The system may have no unique solution.', 'matrix': mat.copy().tolist()}
                yield {'type': 'solution', 'data': None}
                return
            
            swap_row = col + 1 + non_zero_indices[0]
            # NumPy array swapping syntax is incredibly clean:
            mat[[col, swap_row]] = mat[[swap_row, col]]
            
            yield {
                'type': 'swap', 
                'row1': col, 
                'row2': swap_row, 
                'msg': f"R{col+1} <-> R{swap_row+1}", 
                'matrix': mat.copy().tolist()
            }

        # 2. Scale
        pivot_val = mat[col, col]
        if pivot_val != 1.0:
            # NumPy scales the entire row at once (Vectorization)
            mat[col] = mat[col] / pivot_val
            yield {
                'type': 'scale', 
                'row': col, 
                'factor': float(pivot_val), 
                'msg': f"R{col+1}  ->  (1 / {pivot_val:.4f}) * R{col+1}", 
                'matrix': mat.copy().tolist()
            }

        # 3. Eliminate
        for row in range(n_rows):
            if row == col:
                continue
            factor = mat[row, col]
            if factor != 0:
                # NumPy subtracts scaled arrays element-wise instantly
                mat[row] = mat[row] - factor * mat[col]
                sign = "-" if factor >= 0 else "+"
                yield {
                    'type': 'eliminate', 
                    'row': row, 
                    'pivot_row': col, 
                    'factor': float(factor), 
                    'msg': f"R{row+1}  ->  R{row+1}  {sign}  ({abs(factor):.4f} * R{col+1})", 
                    'matrix': mat.copy().tolist()
                }

    # Check solution
    for row in range(n_rows):
        # np.allclose handles the floating point tolerance automatically!
        left_all_zero = np.allclose(mat[row, :-1], 0, atol=1e-9)
        if left_all_zero:
            if not np.isclose(mat[row, -1], 0, atol=1e-9):
                yield {'type': 'error', 'msg': 'Inconsistent system - No solution exists.', 'matrix': mat.copy().tolist()}
                yield {'type': 'solution', 'data': None}
                return
            else:
                yield {'type': 'error', 'msg': 'Infinite solutions exist (dependent system).', 'matrix': mat.copy().tolist()}
                yield {'type': 'solution', 'data': None}
                return

    yield {'type': 'done', 'msg': 'Reduced Row Echelon Form (RREF) reached', 'matrix': mat.copy().tolist()}
    yield {'type': 'solution', 'data': mat[:, -1].tolist()}

def verify_solution(original_matrix, solution):
    mat = np.array(original_matrix, dtype=np.float64)
    sol = np.array(solution, dtype=np.float64)
    
    # Extract coefficients and constants
    A = mat[:, :-1]
    b = mat[:, -1]
    
    # Compute the dot product essentially instantly
    lhs = np.dot(A, sol)
    
    # Vectorized comparison checking if LHS roughly equals RHS
    ok_array = np.isclose(lhs, b, atol=1e-6)
    all_correct = bool(np.all(ok_array))
    
    results = []
    for i in range(len(b)):
        results.append({
            'equation_idx': i,
            'lhs': float(lhs[i]),
            'rhs': float(b[i]),
            'ok': bool(ok_array[i]),
            'row': original_matrix[i]
        })
        
    return all_correct, results
