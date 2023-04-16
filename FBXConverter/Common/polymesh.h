/*
This file is part of ``FBXConverter'', a library for Autodesk FBX.
Copyright (C) 2023 Bill He <github.com/easterngarden>
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

//polymesh.h

#include <memory>
#include <vector>
#include <map>
#include <utility>
#include <Eigen/Dense>

using namespace Eigen;

class PolyMesh
{
public:
	std::string name;
	unsigned int nVertices;
	uint32_t nFaces;
	std::unique_ptr<uint32_t[]> FaceIndices;
	std::unique_ptr<uint32_t[]> VertsIndices;
	std::unique_ptr<Vector3d[]> Verts;		//vertex positions
	std::unique_ptr<Vector3d[]> Normals;	//normals
	std::unique_ptr<Vector2d[]> UVs;		//texture coordinates
	std::unique_ptr<uint32_t[]> UVIndices;
};

class TriMesh 
{
public:
    // Build a triangle mesh from a face index array and a vertex index array
    TriMesh( const PolyMesh* pMesh )
        :numTris(0), numVert(0), numUV(0)
    {
		name = pMesh->name;
		const uint32_t nfaces = pMesh->nFaces;
		const std::unique_ptr<uint32_t[]> &faceIndices = pMesh->FaceIndices;
		const std::unique_ptr<uint32_t[]> &vertsIndex = pMesh->VertsIndices;
		const std::unique_ptr<uint32_t[]> &uvIndices = pMesh->UVIndices;
		const std::unique_ptr<Vector3d[]> &verts = pMesh->Verts;
		const std::unique_ptr<Vector3d[]> &normals = pMesh->Normals;
		const std::unique_ptr<Vector2d[]> &vt = pMesh->UVs;

        uint32_t k = 0, maxVertIndex = 0, maxUVIndex=0;
        // find out how many triangles we need to create for this mesh
        for (uint32_t i = 0; i < nfaces; ++i) {
			numTris += (faceIndices[i] - 2);
			for (uint32_t j = 0; j < faceIndices[i]; ++j)
			{
                if (vertsIndex[k + j] > maxVertIndex)
                    maxVertIndex = vertsIndex[k + j];
				if (uvIndices[k + j] > maxUVIndex)
					maxUVIndex = uvIndices[k + j];
			}
            k += faceIndices[i];
        }
        maxVertIndex += 1;
		maxUVIndex += 1;
        
        // allocate memory to store the position of the mesh vertices
        P = std::unique_ptr<Vector3d []>(new Vector3d[maxVertIndex]);
        for (uint32_t i = 0; i < maxVertIndex; ++i) {
            P[i] = verts[i];
        }
		numVert = maxVertIndex;
		numUV = maxUVIndex;

        uint32_t l = 0;
        // allocate memory to store triangle indices
        triIndex = std::unique_ptr<uint32_t []>(new uint32_t [numTris * 3]);
		UVIndices = std::unique_ptr<uint32_t[]>(new uint32_t[numTris * 3]);
		N = std::unique_ptr<Vector3d []>(new Vector3d[numTris * 3]);
		T = std::unique_ptr<Vector2d []>(new Vector2d[numTris * 3]);
		PN = std::unique_ptr<Vector3d[]>(new Vector3d[maxVertIndex]);
		UV = std::unique_ptr<Vector2d[]>(new Vector2d[maxUVIndex]);
		for (uint32_t i = 0, k = 0; i < nfaces; ++i) { // for each polygon
            for (uint32_t j = 0; j < faceIndices[i] - 2; ++j) { // for each triangle in the polygon
                triIndex[l] = vertsIndex[k];
                triIndex[l + 1] = vertsIndex[k + j + 1];
                triIndex[l + 2] = vertsIndex[k + j + 2];
				UVIndices[l] = uvIndices[k];
				UVIndices[l + 1] = uvIndices[k + j + 1];
				UVIndices[l + 2] = uvIndices[k + j + 2];
				N[l] = normals[k];
                N[l + 1] = normals[k + j + 1];
                N[l + 2] = normals[k + j + 2];
				T[l] = vt[k];
				T[l + 1] = vt[k + j + 1];
				T[l + 2] = vt[k + j + 2];
				PN[vertsIndex[k]] = normals[k];
				PN[vertsIndex[k + j + 1]] = normals[k + j + 1];
				PN[vertsIndex[k + j + 2]] = normals[k + j + 2];
				UV[uvIndices[k]] = vt[k];
				UV[uvIndices[k + j + 1]] = vt[k + j + 1];
				UV[uvIndices[k + j + 2]] = vt[k + j + 2];
				l += 3;
            }                                                                                                                                                                                                                                
            k += faceIndices[i];
        }
    }

	//member variables
	std::string name;
	std::string matname;
	uint32_t numVert;							// number of vertices
	uint32_t numTris;							// number of triangles
	uint32_t numUV;								// number of UVs
	std::unique_ptr<Vector3d []> P;             // triangles vertex position
    std::unique_ptr<uint32_t []> triIndex;		// vertex index array
    std::unique_ptr<Vector3d []> N;             // triangles vertex normals
	std::unique_ptr<Vector2d []> T;				// triangles texture coordinates
	std::unique_ptr<uint32_t[]> UVIndices;		// triangles texture index
	std::unique_ptr<Vector3d[]> PN;             // vertex normals
	std::unique_ptr<Vector2d[]> UV;             // UV coordinates
};

