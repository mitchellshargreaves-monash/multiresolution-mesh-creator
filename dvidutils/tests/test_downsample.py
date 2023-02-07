import sys
from itertools import product
import pytest
import numpy as np
from dvidutils import downsample_labels

import faulthandler
faulthandler.enable()


def test_downsample_labels_2D():
    a = [[0,0, 1,1, 2,2, 3,3],
         [0,0, 1,0, 2,0, 3,0],

         [0,0, 0,0, 2,2, 3,3],
         [0,0, 8,9, 8,9, 8,9]]
    #           ^
    #           |
    #           `-- In the suppress_zero case, we choose the LOWER
    #               of the two tied values (in this case, 8).

    a = np.array(a)
    a_copy = a.copy()
    d = downsample_labels(a, 2, suppress_zero=False)
    assert (a == a_copy).all(), "input was overwritten!"

    assert (d == [[0, 1, 2, 3],
                  [0, 0, 2, 3]]).all()
    
    d = downsample_labels(a, 2, suppress_zero=True)
    assert (a == a_copy).all(), "input was overwritten!"
    
    assert (d == [[0, 1, 2, 3],
                  [0, 8, 2, 3]]).all()


def test_downsample_labels_3D():
    a = [[[0,0, 1,1],
          [2,2, 3,3]],
         
         [[0,0, 1,0],
          [2,0, 3,3]]]

    a = np.array(a)
    a_copy = a.copy()
    
    d = downsample_labels(a, 2, suppress_zero=False)
    assert (a == a_copy).all(), "input was overwritten!"
    assert (d == [[[0, 3]]]).all()
    
    d = downsample_labels(a, 2, suppress_zero=True)
    assert (a == a_copy).all(), "input was overwritten!"
    assert (d == [[[2, 3]]]).all()


def test_downsample_not_contiguous():
    # Hopefully if there are memory issues with non-contiguous input,
    # The Xcode AddressSanitizer will catch them when this runs.
    a = np.random.randint(10, size=(100,100,100), dtype=np.uint64)
    d = downsample_labels(a[40:60, 40:60, 40:60], 2)
    assert d.shape == (10,10,10)


def test_input_doesnt_change():
    a = np.zeros((64,64,64), dtype=np.uint32)
    d = downsample_labels(a, 2, suppress_zero=False)
    assert a.shape == (64,64,64), "Shape of a changed!"


def test_all_zeros():
    a = np.zeros((100,100,100), dtype=np.uint64)
    d = downsample_labels(a, 2, suppress_zero=True)
    assert (d == 0).all()
    assert d.shape == (50,50,50)


def test_downsample_with_ties():
    """
    In the event of a tie between two voxels,
    we choose the lesser of the two values
    (or lesser of four values).
    """
    a = [[1,1, 3,0, 2,3],
         [2,2, 3,0, 1,0]]
    
    d = downsample_labels(a, 2)
    assert (d == [[1, 0, 0]]).all()

    d = downsample_labels(a, 2, suppress_zero=True)
    assert (d == [[1, 3, 1]]).all()

def test_downsample_with_ties_3d():
    """
    In the event of a tie between two voxels,
    we choose the lesser of the two values
    (or lesser of four values).
    """
    a = [[[1,1, 3,0,  2,3, 4,3],
          [2,2, 3,0,  1,0, 3,4]],

         [[2,2, 3,0,  1,0, 3,4],
          [2,2, 3,0,  1,0, 3,4]]]
    
    d = downsample_labels(a, 2)
    assert (d == [[[2, 0, 0, 3]]]).all()

    d = downsample_labels(a, 2, suppress_zero=True)
    assert (d == [[[2, 3, 1, 3]]]).all()


def test_zero_size_array():
    a = np.zeros((20,0), np.uint64)
    with pytest.raises(RuntimeError):
        downsample_labels(a, 2)

    a = np.zeros((20,0,10), np.uint64)
    with pytest.raises(RuntimeError):
        downsample_labels(a, 2)


if __name__ == "__main__":
    pytest.main()
