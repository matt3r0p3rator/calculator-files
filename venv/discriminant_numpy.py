import numpy as np

def solve_discriminant(matrix):
    """
    Solves a system of linear equations using Cramer's rule (via determinants).
    Expects an augmented matrix where the last column is the constants vector.
    """
    arr = np.array(matrix, dtype=float)
    rows, cols = arr.shape
    
    if cols != rows + 1:
        raise ValueError("Expected an augmented matrix of size N x (N+1).")
        
    A = arr[:, :-1]
    B = arr[:, -1]
    
    det_A = np.linalg.det(A)
    
    if np.isclose(det_A, 0):
        raise ValueError("The system is singular (determinant is zero) and cannot be solved using Cramer's rule.")
        
    solutions = []
    for i in range(rows):
        A_i = A.copy()
        A_i[:, i] = B
        det_Ai = np.linalg.det(A_i)
        solutions.append(det_Ai / det_A)
        
    return np.array(solutions)

def solve_cramer(A, b):
    """
    Solves a system of linear equations using Cramer's rule given coefficient matrix A and constant vector b.
    """
    A = np.array(A, dtype=float)
    b = np.array(b, dtype=float)
    
    det_A = np.linalg.det(A)
    if np.isclose(det_A, 0):
        raise ValueError("The system is singular (determinant is zero).")
        
    n = len(b)
    solutions = []
    for i in range(n):
        A_i = A.copy()
        A_i[:, i] = b
        solutions.append(np.linalg.det(A_i) / det_A)
        
    return np.array(solutions)
