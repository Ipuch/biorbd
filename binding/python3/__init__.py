import numpy as np
from . import biorbd  # This is created while installing using CMake
from .biorbd import *
from ._version import __version__
from .surface_max_torque_actuator import *
from .rigid_body import *
from .utils import *
from .external_forces import *


if biorbd.currentLinearAlgebraBackend() == 1:
    from casadi import Function, MX, SX, horzcat

    def to_casadi_func(name, func, *all_param, expand=True):
        cx_param = []
        for p in all_param:
            if isinstance(p, (MX, SX)):
                cx_param.append(p)

        if isinstance(func, (MX, SX, Function)):
            func_evaluated = func
        else:
            func_evaluated = func(*all_param)
            if isinstance(func_evaluated, (list, tuple)):
                func_evaluated = horzcat(*[val if isinstance(val, MX) else val.to_mx() for val in func_evaluated])
            elif not isinstance(func_evaluated, MX):
                func_evaluated = func_evaluated.to_mx()
        func = Function(name, cx_param, [func_evaluated])
        return func.expand() if expand else func
