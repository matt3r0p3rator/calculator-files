# Gauss-Jordan Elimination Library

This library provides a Python implementation of the **Gauss-Jordan Elimination** algorithm to solve systems of linear equations. It is designed to be educational and UI-friendly by yielding each step of the process.

## The Math Behind It: Gauss-Jordan Elimination

A system of linear equations can be represented as an **augmented matrix**. For example, the system:
$$
\begin{align*}
2x + y - z &= 8 \\
-3x - y + 2z &= -11 \\
-2x + y + 2z &= -3
\end{align*}
$$
Is represented as:
$$
\left[\begin{array}{ccc|c}
2 & 1 & -1 & 8 \\
-3 & -1 & 2 & -11 \\
-2 & 1 & 2 & -3
\end{array}\right]
$$

The goal of Gauss-Jordan elimination is to transform this augmented matrix into **Reduced Row Echelon Form (RREF)** using three valid **elementary row operations**:
1. **Swap**: Interchanging two rows.
2. **Scale**: Multiplying a row by a non-zero scalar.
3. **Eliminate**: Adding or subtracting a multiple of one row to another row.

### The Algorithm Steps

1. **Pivot Selection:** For each column $j$ (which corresponds to a variable), we want the diagonal element $a_{j,j}$ (the pivot) to be $1$, and all other elements in that column to be $0$.
2. **Swapping:** If the current pivot $a_{j,j}$ is $0$, we search the rows below it for a non-zero entry in the same column and swap the two rows. If no such row exists, the system does not have a unique solution.
3. **Scaling:** We divide the entire pivot row by the value of the pivot $a_{j,j}$. This makes the new pivot exactly $1$.
4. **Elimination:** For every *other* row $i$ (where $i \neq j$), we eliminate the entry in the pivot's column. We do this by subtracting $(a_{i,j} \times \text{pivot\_row})$ from row $i$. This forces $a_{i,j}$ to become $0$.
5. **Repeat** this process for all columns until the left side of the augmented matrix becomes an Identity Matrix (1s on the diagonal, 0s everywhere else).
6. **Interpret the Result:** 
   - Once in RREF, the rightmost column contains the solution for each variable.
   - If we end up with a row like `[0, 0, 0 | 5]`, it means $0 = 5$, which is an **inconsistent system** (no solution).
   - If we end up with a row like `[0, 0, 0 | 0]`, it means $0 = 0$, indicating an **dependent system** (infinite solutions).

---

## How the Code Works

This project provides two separate implementations: a zero-dependency pure Python version (`gauss_elimination_lib.py`) and a high-performance version utilizing the NumPy library (`gauss_elimination_numpy.py`).

Both implementations share the same core functions and output formats:

### 1. `solve_gauss_jordan_steps(matrix)`
This is the core engine, implemented as a **Python generator**. Instead of just returning the final answer, it `yield`s a dictionary at every step of the mathematical process.
- **`yield {'type': 'swap', ...}`**: Emitted when rows are swapped to avoid division by zero.
- **`yield {'type': 'scale', ...}`**: Emitted when a row is normalized so its pivot becomes 1.
- **`yield {'type': 'eliminate', ...}`**: Emitted when a multiple of the pivot row is subtracted from another row to create zeros in the column.

Because it yields the state iteratively, this function is perfectly suited for building GUIs, web apps, or CLI tools that need to show the user a step-by-step breakdown of how the matrix is solved. The raw matrices are yielded as standard Python lists even in the NumPy variant to maintain output compatibility.

### 2. `solve_gauss_jordan(matrix)`
This is a standard pure function wrapper around the generator. It consumes all the steps silently and only returns the final solution array (or `None` if the system is inconsistent/dependent).

### 3. `verify_solution(original_matrix, solution)`
Due to floating-point arithmetic, matrices can suffer from extremely minor precision issues (e.g., $0.99999999$ instead of $1.0$). 
This function takes the calculated solution and plugs the variables back into the **original** equations. It calculates the Left Hand Side (LHS) and compares it strictly to the Right Hand Side (RHS) using a tiny tolerance margin (`1e-6` or `np.isclose`) to verify if the math perfectly aligns.

### Why two versions?
- **Pure Python**: Good for environments where installing third-party libraries is impossible or not allowed. Great for learning standard Python techniques like list comprehensions and generators.
- **NumPy Version**: The industry standard. Utilizes highly-optimized C code under the hood for "vectorized" operations. It eliminates inner `for` loops, cuts down precision-handling boilerplate, and is vastly faster when computing large matrices.
