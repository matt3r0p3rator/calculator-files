import py_compile
try:
    py_compile.compile('bundled_app.py')
    print("Python 3 syntax is perfectly valid!")
except Exception as e:
    print(f"Error: {e}")
