import sys
from itertools import product
import pytest
import numpy as np
import pandas as pd
from dvidutils import encode_faces_to_drc_bytes, decode_drc_bytes_to_faces

import faulthandler
faulthandler.enable()

def test_decode_invalid():
    bogus_data = b"This ain't a mesh file!"
    try:
        decode_drc_bytes_to_faces(bogus_data)
    except RuntimeError:
        pass
    else:
        raise RuntimeError("Expected an exception, but didn't see one!")


def test_empty():
    vertices = np.zeros((0,3), np.float32)
    normals = np.zeros((0,3), np.float32)
    faces = np.zeros((0,3), np.uint32)
    drc_bytes = encode_faces_to_drc_bytes(vertices, normals, faces)
    
    assert drc_bytes == b'', "Expected an empty buffer for an empty mesh (as a special case)"
    
    rt_vertices, rt_normals, rt_faces = decode_drc_bytes_to_faces(drc_bytes)
    assert rt_vertices.shape == (0,3)
    assert rt_normals.shape == (0,3)
    assert rt_faces.shape == (0,3)

def test_hexagon_empty_normals():
    _hexagon_roundtrip('empty')

def test_hexagon():
    _hexagon_roundtrip('unique')
  
def test_hexagon_nonunique_normals():
    _hexagon_roundtrip('nonunique')

def _hexagon_roundtrip(normals_set):
    # This map is correctly labeled with the vertex indices
    _ = -1
    hexagon = [[[_,_,_,_,_,_,_],
                [_,_,0,_,1,_,_],
                [_,_,_,_,_,_,_],
                [_,2,_,3,_,4,_],
                [_,_,_,_,_,_,_],
                [_,_,5,_,6,_,_],
                [_,_,_,_,_,_,_]]]

    hexagon = 1 + np.array(hexagon)
    vertices = np.transpose(hexagon.nonzero()).astype(np.float32)

    if normals_set == 'empty':
        normals = np.zeros((0,3), np.float32)
    elif normals_set == 'unique':
        # fake 'normals' for this test
        normals = vertices + 10
        normals /= np.linalg.norm(normals, axis=1)[:,None]
    elif normals_set == 'nonunique':
        normals = np.ones_like(vertices)
        normals /= np.linalg.norm(normals, axis=1)[:,None]
    else:
        raise RuntimeError("bad normals_set option")
    
    faces = [[3,1,4],
             [3,4,6],
             [3,6,5],
             [3,5,2],
             [3,2,0],
             [3,0,1]]

    faces = np.asarray(faces, dtype=np.uint32)
    
    # verts/norms: ZYX -> XYZ
    drc_bytes = encode_faces_to_drc_bytes(vertices[:,::-1], normals[:,::-1], faces,
                                          normal_quantization_bits=14) # Must use better than default normals quantization, or comparisons
                                                                       # in this test will fail (rounding to nearest .1 isn't enough).

    rt_vertices, rt_normals, rt_faces = decode_drc_bytes_to_faces(drc_bytes)
    rt_vertices = rt_vertices[:,::-1] # XYZ -> ZYX
    rt_normals = rt_normals[:,::-1] # XYZ -> ZYX

    # draco changes the order of the faces and vertices,
    # so verifying that the mesh hasn't changed is a little tricky.
    _compare(vertices, normals, faces,
             rt_vertices, rt_normals, rt_faces,
             normals_set != 'empty')
    

def test_random_roundtrip():
    np.random.seed(0) # Force deterministic testing.
    
    vertices = np.zeros((10,3), dtype=np.float32)
    vertices[:,0] = np.random.choice(list(range(10)), size=(10,))
    vertices[:,1] = np.random.choice(list(range(10)), size=(10,))
    vertices[:,2] = np.random.choice(list(range(10)), size=(10,))
  
    # These 'normals' are totally fake
    normals = np.zeros((10,3), dtype=np.float32)
    normals[:,0] = np.random.choice(list(range(10)), size=(10,))
    normals[:,1] = np.random.choice(list(range(10)), size=(10,))
    normals[:,2] = np.random.choice(list(range(10)), size=(10,))
    
    # ...but they must still have magnitude == 1.0
    normals /= np.linalg.norm(normals, axis=1)[:,None]
    
    # Choose carefully to ensure no degenerate faces
    faces = np.zeros((100,3), dtype=np.uint32)
    for face in faces:
        face[:] = np.random.choice(list(range(10)), size=(3,), replace=False)
 
    faces = pd.DataFrame(faces)
    faces.drop_duplicates(inplace=True)
    faces = faces.values
 
    #print(f"\nEncoding {len(vertices)} verts, {len(normals)} norms, {len(faces)} faces\n")
 
    # Must use better than default normals quantization, or comparisons
    # in this test will fail (rounding to nearest .1 isn't enough).
    drc_bytes = encode_faces_to_drc_bytes(vertices, normals, faces, normal_quantization_bits=14)
    rt_vertices, rt_normals, rt_faces = decode_drc_bytes_to_faces(drc_bytes)
      
    #print(f"\nGot {len(rt_vertices)} verts, {len(rt_normals)} norms, {len(rt_faces)} faces\n")
         
    _compare(vertices, normals, faces, rt_vertices, rt_normals, rt_faces, True)


def _compare(vertices, normals, faces, rt_vertices, rt_normals, rt_faces, check_normals): 
    # Draco compression involves dropping some bits during quantization
    # For comparisons, we need to round the results.
    vertices = np.round(vertices, 2)
    normals = np.round(normals, 2)
    rt_vertices = np.round(rt_vertices, 2)
    rt_normals = np.round(rt_normals, 2)

    # Vertexes are easy to check -- just sort first.
    sorted_verts = np.asarray((sorted(vertices.tolist())))
    sorted_rt_verts = np.asarray((sorted(rt_vertices.tolist())))

    sorted_norms = np.asarray((sorted(normals.tolist())))
    sorted_rt_norms = np.asarray((sorted(rt_normals.tolist())))
    
#     print('')
#     print(sorted_verts)
#     print('--------------')
#     print(sorted_rt_verts)

#     print('')
#     print(sorted_norms)
#     print('--------------')
#     print(sorted_rt_norms)

    assert set(map(tuple, sorted_verts)) == set(map(tuple, sorted_rt_verts))
    assert set(map(tuple, sorted_norms)) == set(map(tuple, sorted_rt_norms))
    assert (sorted_norms == sorted_rt_norms).all()

#     print('================')    
#     print('--orig faces--')
#     print(faces)
#     print('--rt faces faces--')
#     print(rt_faces)

    # Faces are a little trickier.
    # Generate a triangle (3 vertices) for every face and original face,
    # And make sure the set of triangles matches.    
    def to_triangle_set(v, f):
        triangles = v[f, :]
        assert triangles.shape == (f.shape[0], 3, 3)
        triangle_set = set()
        for triangle in triangles:
            v1, v2, v3 = map(tuple, triangle)
            triangle = sorted([v1,v2,v3])
            triangle_set.add( tuple(triangle) )
        return triangle_set

    orig_vert_triangles = to_triangle_set(vertices, faces)
    rt_vert_triangles = to_triangle_set(rt_vertices, rt_faces)
    
#     print('')
#     print('---- orig triangles ---')
#     print(np.array(sorted(orig_vert_triangles)))
#  
#     print('---- rt triangles ---')
#     print(np.array(sorted(rt_vert_triangles)))
     
    assert orig_vert_triangles == rt_vert_triangles
 
    # Same for normals
    if check_normals:
        orig_norm_triangles = to_triangle_set(normals, faces)
        rt_norm_triangles = to_triangle_set(rt_normals, rt_faces)
        assert orig_norm_triangles == rt_norm_triangles


if __name__ == "__main__":
    pytest.main()
