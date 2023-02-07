[![Travis CI Status](https://travis-ci.org/stuarteberg/dvidutils.svg?branch=master)](https://travis-ci.org/stuarteberg/dvidutils)

dvidutils
=========

A collection of utility functions for dealing with dvid data


Installation
------------

    conda install -c flyem-forge dvidutils

Usage
-----

```python
In [1]: import numpy as np

In [2]: from dvidutils import LabelMapper, downsample_labels

In [3]: labels = np.random.randint(0, 3, (9,9), dtype=np.uint64)

In [4]: labels
Out[4]:
array([[2, 0, 0, 0, 1, 0, 1, 0, 1],
       [1, 1, 1, 2, 1, 0, 2, 2, 2],
       [1, 2, 2, 1, 1, 2, 1, 1, 2],
       [0, 2, 1, 0, 1, 0, 2, 0, 2],
       [2, 2, 1, 0, 0, 0, 1, 2, 1],
       [0, 1, 0, 1, 2, 2, 2, 2, 0],
       [0, 0, 2, 2, 1, 2, 1, 0, 2],
       [2, 0, 2, 1, 2, 2, 2, 2, 1],
       [1, 1, 1, 1, 2, 0, 2, 0, 1]], dtype=uint64)

In [5]: mapping = {1:10, 2:20}

In [6]: mapping_keys = np.array( list(mapping.keys()), np.uint64 )

In [7]: mapping_values = np.array( list(mapping.values()), np.uint64 )

In [8]: mapper = LabelMapper(mapping_keys, mapping_values)

In [9]: remapped = mapper.apply( labels, allow_unmapped=True )

In [10]: remapped
Out[10]:
array([[20,  0,  0,  0, 10,  0, 10,  0, 10],
       [10, 10, 10, 20, 10,  0, 20, 20, 20],
       [10, 20, 20, 10, 10, 20, 10, 10, 20],
       [ 0, 20, 10,  0, 10,  0, 20,  0, 20],
       [20, 20, 10,  0,  0,  0, 10, 20, 10],
       [ 0, 10,  0, 10, 20, 20, 20, 20,  0],
       [ 0,  0, 20, 20, 10, 20, 10,  0, 20],
       [20,  0, 20, 10, 20, 20, 20, 20, 10],
       [10, 10, 10, 10, 20,  0, 20,  0, 10]], dtype=uint64)

In [11]: downsample_labels(remapped, factor=3, suppress_zero=True)
Out[11]:
array([[10, 10, 10],
       [10, 10, 20],
       [10, 20, 20]], dtype=uint64)
```

Developer Instructions
----------------------


### Linux (or Mac)

Here's how to make an ordinary Makefile-based build:


    conda install -c conda-forge python=3.6 cmake xtensor-python

    mkdir build
    cd build

    # Makefiles
    cmake .. \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_CXX_FLAGS_DEBUG="-g -O0 -DXTENSOR_ENABLE_ASSERT=ON" \
        -DCMAKE_PREFIX_PATH="${CONDA_PREFIX}" \
    ##

    make
    make install

**Note:** Requires gcc >=4.9 or a recent version of clang. On CentOS, the easiesst way to get gcc-4.9 is with these commands:

    # Install devtoolset-3
    yum install -y centos-release-scl yum-utils devtoolset-3-binutils devtoolset-3-gcc devtoolset-3-gcc-c++
    
    # Activate it (preferably in your .bashrc)
    source /opt/rh/devtoolset-3/enable

### Xcode

On Mac, your best option for C++14 development is to use Xcode.
(On Linux, your best option is to switch to Mac.)

To use Xcode and its debugger, add `-G Xcode` to the cmake command:

    cmake .. \
        -G Xcode \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_CXX_FLAGS_DEBUG="-g -O0 -DXTENSOR_ENABLE_ASSERT=ON" \
        -DCMAKE_PREFIX_PATH="${CONDA_PREFIX}" \

    open dvidutils.xcodeproj

Xcode is finicky about which executables it likes.  Install this special build of Python:
```
conda install -c conda-forge python.app
```

Within Xcode, Opt+click the 'Run' button to edit your executable settings:

- Under "Info":
  - Select `${CONDA_PREFIX}/python.app` as the Executable (NOT `${CONDA_PREFIX}/bin/python.app`).
- Under "Arguments":
  - Add an Environment Variable for `PYTHONPATH`: `/path/to/my-dvidutils-repo/build-for-xcode/Debug`
  - Add an item to "Arguments Passed on Launch": `-m pytest --color=no /path/to/my-dvidutils-repo/tests`
