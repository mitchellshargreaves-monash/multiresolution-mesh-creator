import sys
from itertools import product
import pytest
import numpy as np
from dvidutils import remap_duplicates

import faulthandler
faulthandler.enable()

def test_basic():
    vertices = np.zeros((10, 3), np.float32)
    vertices[:, 2] = np.arange(10, dtype=int) % 3

    # Maps indices of duplicate vertices to each vertex's first index
    duplicate_mapping = remap_duplicates(vertices)    
    expected = np.array((list(zip(np.arange(10), np.arange(10) % 3))))[3:]
    assert (duplicate_mapping == expected).all()
    

if __name__ == "__main__":
    pytest.main()
