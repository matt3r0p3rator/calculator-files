# Deep Dive: Python Syntax for Matrix Math

This document breaks down the specific technical implementations and Pythonic syntax used to perform matrix math in both `gauss_elimination_lib.py` (Pure Python) and `gauss_elimination_numpy.py` (NumPy). 

## 1. Row Swapping
When a pivot is exactly `0`, the algorithm has to swap the current row with another row that has a non-zero value in that column.

**Pure Python: Tuple Unpacking**
```python
mat[col], mat[swap_row] = mat[swap_row], mat[col]
```
In many languages, swapping two variables requires a temporary storage variable. Python allows for **tuple packing and unpacking**. It evaluates the right side into a temporary tuple in memory, and then assigns those values directly into the variables on the left side. It operates in $O(1)$ time by merely swapping pointers.

**NumPy: Array Indexing**
```python
mat[[col, swap_row]] = mat[[swap_row, col]]
```
NumPy allows "fancy indexing" where passing a list of indices returns those mutually exclusive rows, letting you reassign them instantly in a similar one-line swap.

## 2. Scaling a Row
To turn the pivot element into a `1`, the entire pivot row must be divided by the pivot's current value.

**Pure Python: List Comprehensions**
```python
mat[col] = [x / pivot_val for x in mat[col]]
```
This uses a **List Comprehension**, which is a compact and computationally faster way to generate lists in Python.
- `for x in mat[col]` iterations through every element.
- `x / pivot_val` performs the division.
- `[...]` wraps the resulting values into a brand new Python list.

**NumPy: Vectorization**
```python
mat[col] = mat[col] / pivot_val
```
NumPy treats the array row as a single mathematical vector. Division is automatically applied to every element via highly optimized internal C loops, eliminating the need to write an explicit Python loop.

## 3. Row Elimination
The most mathematically complex part of Gauss-Jordan is the elimination step, where a multiple of the pivot row is subtracted from another row to make the target column zero.

**Pure Python: Indexed List Comprehensions**
```python
mat[row] = [mat[row][i] - factor * mat[col][i] for i in range(n_cols)]
```
Because we are dealing with two different rows simultaneously (`mat[row]` and `mat[col]`), we cannot just iterate over the elements directly; we must iterate over their **indices** using `range(n_cols)` to align them.

**NumPy: Direct Vector Subtraction**
```python
mat[row] = mat[row] - factor * mat[col]
```
Again, NumPy removes the complexity. The scalar `factor` multiplies against the entire pivot row `mat[col]`, and the resulting array is subtracted element-wise from `mat[row]` instantly.

## 4. Evaluating Precision & Floating-Point Inaccuracies
When checking if a system is inconsistent or dependent, the algorithm must verify if the entire left side of a row consists of zeroes. Because of floating-point math issues, `1.0 / 3.0 * 3.0` might yield `0.99999999` instead of `1.0`.

**Pure Python: Generators and `all()`**
```python
left_all_zero = all(abs(mat[row][i]) < 1e-9 for i in range(n_rows))
```
- We check using a tolerance `abs(value) < 1e-9` instead of examining absolute zeroes.
- `(condition for item...)` creates a lazy generator.
- `all()` consumes the generator and returns `True` only if every item evaluates to True. It strictly "short-circuits": the moment it finds a single cell larger than `1e-9`, it stops calculating.

**NumPy: `np.allclose()`**
```python
left_all_zero = np.allclose(mat[row, :-1], 0, atol=1e-9)
```
NumPy comes with built-in functions designed for precision bounds. `np.allclose` checks if the entire array matches the target (`0`) within the absolute tolerance (`atol`), and the array slicing `[:-1]` efficiently ignores the constant terms column.

## 5. Computing the Dot Product
In the verification step, we check answers by multiplying the original matrix row variables against the final solution array (the math "dot product").

**Pure Python: The `sum()` Generator**
```python
lhs = sum(row[j] * solution[j] for j in range(n_vars))
```
This combines the built-in `sum()` function with a generator expression to iterate strictly over the variable columns, multiply coefficients by solutions, and add them up to find the Left Hand Side (LHS) equation result.

**NumPy: `np.dot()`**
```python
lhs = np.dot(A, sol)
```
Matrix multiplication is a fundamental construct of NumPy, making it incredibly fast to verify an entire system of equations simultaneously against the solutions without writing any explicit iteration code.
