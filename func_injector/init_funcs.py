# init_funcs.py
# Package this as a TI-Nspire Python document using TI-Nspire Student Software
# (the same way you package bundled_app.py).
# Requires OS 5.x+ with Python support.
#
# To add/remove functions, edit the `defines` list below.

from ti_system import eval_expr

defines = [
    # Math
    "Define disc(a,b,c)=b^2-4*a*c",
    "Define quad1(a,b,c)=(-b+sqrt(b^2-4*a*c))/(2*a)",
    "Define quad2(a,b,c)=(-b-sqrt(b^2-4*a*c))/(2*a)",
    "Define dist2d(x1,y1,x2,y2)=sqrt((x2-x1)^2+(y2-y1)^2)",
    "Define slope(x1,y1,x2,y2)=(y2-y1)/(x2-x1)",
    # Physics
    "Define ke(m,v)=0.5*m*v^2",
    "Define pe(m,g,h)=m*g*h",
    "Define fnet(m,a)=m*a",
]

ok = 0
for d in defines:
    try:
        eval_expr(d)
        ok += 1
    except Exception:
        pass

print(str(ok) + "/" + str(len(defines)) + " functions defined.")
