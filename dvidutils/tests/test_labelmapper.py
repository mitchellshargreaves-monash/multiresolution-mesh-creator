import sys
from itertools import product
import pytest
import numpy as np
from dvidutils import LabelMapper

import faulthandler
faulthandler.enable()

UINT_DTYPES = [np.uint8, np.uint16, np.uint32, np.uint64]
#INT_DTYPES = [np.int8, np.int16, np.int32, np.int64]
 
dtype_pairs = list(zip(UINT_DTYPES,UINT_DTYPES))
dtype_pairs += [(np.uint64, np.uint32)] # Special case: Map uint64 down to uint32
dtype_pairs += [(np.uint32, np.uint64)] # Special case: Map uint32 up to uint64
params = list(product(dtype_pairs, [1,2,3]))
ids = [f'{ndim}d-{dtype_in.__name__}-{dtype_out.__name__}' for ((dtype_in, dtype_out), ndim) in params]

@pytest.fixture(params=params, ids=ids)
def labelmapper_args(request):
    """
    pytest "fixture" to cause the test functions below to be called
    with all desired combinations of ndim and dtype.
    """
    (dtype_in, dtype_out), ndim = request.param
    
    mapping = {k: k+100 for k in range(10)}
    original = np.random.randint(0, 10, (3,)*ndim, dtype=dtype_in)
    expected = (original + 100).astype(dtype_out)

    domain = np.fromiter(mapping.keys(), dtype=original.dtype)
    codomain = np.fromiter(mapping.values(), dtype=expected.dtype)

    yield (original, expected, mapping, domain, codomain)
   
   
def test_LabelMapper(labelmapper_args):
    original, expected, mapping, domain, codomain = labelmapper_args
       
    mapper = LabelMapper(domain, codomain)
    remapped = mapper.apply(original)

    assert remapped.dtype == expected.dtype, f"Wrong dtype: Expected {expected.dtype}, got {remapped.dtype}"
    assert remapped.shape == original.shape, f"Wrong shape: Expected {original.shape}, got {remapped.shape}"
    assert (remapped == expected).all(), f"Mapping was not applied correctly!"


def test_LabelMapper_allow_unmapped(labelmapper_args):
    original, expected, mapping, domain, codomain = labelmapper_args

    original.flat[0] = 127
    expected.flat[0] = 127

    mapper = LabelMapper(domain, codomain)
    remapped = mapper.apply(original, allow_unmapped=True)

    assert remapped.dtype == expected.dtype, f"Wrong dtype: Expected {expected.dtype}, got {remapped.dtype}"
    assert remapped.shape == original.shape, f"Wrong shape: Expected {original.shape}, got {remapped.shape}"
    assert (remapped == expected).all(), f"Mapping was not applied correctly!"

def test_LabelMapper_with_default(labelmapper_args):
    original, expected, mapping, domain, codomain = labelmapper_args

    original.flat[0] = 127 # Not in the mapping
    expected.flat[0] = 115 # Default value (below)

    mapper = LabelMapper(domain, codomain)
    remapped = mapper.apply_with_default(original, 115)

    assert remapped.dtype == expected.dtype, f"Wrong dtype: Expected {expected.dtype}, got {remapped.dtype}"
    assert remapped.shape == original.shape, f"Wrong shape: Expected {original.shape}, got {remapped.shape}"
    assert (remapped == expected).all(), f"Mapping was not applied correctly!"


def test_LabelMapper_inplace(labelmapper_args):
    original, expected, mapping, domain, codomain = labelmapper_args
 
    mapper = LabelMapper(domain, codomain)
    remapped = original.copy()
    result = mapper.apply_inplace(remapped)
    assert result is None, "apply_inplace returns None"

    assert remapped.shape == original.shape, f"Wrong shape: Expected {original.shape}, got {remapped.shape}"
    assert (remapped == expected).all(), f"Mapping was not applied correctly!"

def test_LabelMapper_other_dtypes(labelmapper_args):
    """
    Test what happens when you provide an array that is not
    of the same type as either the domain or codomain. 
    """
    original, expected, mapping, domain, codomain = labelmapper_args
       
    mapper = LabelMapper(domain, codomain)
    
    for dtype in UINT_DTYPES:
        remapped = mapper.apply(original.astype(dtype))
        assert remapped.dtype == expected.dtype, f"Wrong dtype: Expected {expected.dtype}, got {remapped.dtype}"
        assert remapped.shape == original.shape, f"Wrong shape: Expected {original.shape}, got {remapped.shape}"
        assert (remapped == expected).all(), f"Mapping was not applied correctly!"

        # Again, in-place
        remapped = original.astype(dtype).copy('C')
        mapper.apply_inplace(remapped)
        assert (remapped == expected).all(), f"Mapping was not applied correctly!"

def test_larger_dtype():
    """
    No matter what the 'native' dtypes of the LabelMapper are (based on its domain and codomain dtypes),
    it is permitted to call apply() and apply_inplace() with arrays of any (unsigned) type.
    But when the input array's dtype is wider than the 'native' dtype, we need to be
    careful not to truncate any input values when allow_unmapped=True.
    
    This test verifies that we can use a large value (1000) which remains intact
    during processing by a LabelMapper with a small 'native' dtype (np.uint8).
    """
    mapping = {k: k+100 for k in range(10)}

    # Small 'native' dtype (uint8)
    domain = np.fromiter(mapping.keys(), dtype=np.uint8)
    codomain = np.fromiter(mapping.values(), dtype=np.uint16)

    # Test with larger input
    original = np.random.randint(0, 10, (10,10), dtype=np.uint16)
    original[1,2] = 1000
    original[2,2] = 1000
    
    expected = (original + 100).astype(np.uint16)
    expected[1,2] = 1000
    expected[2,2] = 1000

    mapper = LabelMapper(domain, codomain)
    
    # Not in-place
    remapped = mapper.apply(original, allow_unmapped=True)
    assert (remapped == expected).all()
    
    # In-place
    remapped = original.copy()
    mapper.apply_inplace(remapped, allow_unmapped=True)
    assert (remapped == expected).all()

if __name__ == "__main__":
    pytest.main()
