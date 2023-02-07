from functools import update_wrapper, partial
from . import _dvidutils as _dvidutils

# pybind11 functions aren't pickleable, due to the following error:
#     TypeError: can't pickle PyCapsule objects
#
# So wrap them in something that is pickleable.
# The downside is that it adds an extra attribute
# lookup to every single function invocation.
#
def dvidutils_func(name, *args, **kwargs):
    # Look up the function by name and call it.
    return getattr(_dvidutils, name)(*args, **kwargs)

for name in dir(_dvidutils):
    if name.startswith('_'):
        continue

    o = getattr(_dvidutils, name)
    if isinstance(o, type):
        globals()[name] = o
    elif callable(o):
        f = partial(dvidutils_func, name)
        update_wrapper(f, o)
        del f.__wrapped__
        globals()[name] = f
    else:
        globals()[name] = o
