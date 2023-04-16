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

#pragma once

#include <fbxsdk.h>
#include <memory>
#include <vector>
#include "../Common/polymesh.h"


struct Material
{
	unsigned int index = -1;//index of material
	std::string materialName;

	Vector3d Ka = Vector3d(0.2f, 0.2f, 0.2f); //ambient
	Vector3d Kd = Vector3d(1.0f, 1.0f, 1.0f); //diffuse
	Vector3d Ks = Vector3d(1.0f, 1.0f, 1.0f); //specular

	float d; //alpha
	float Tr = 1.0f; //alpha

	int illum = 2; //specular illumination
	float Ns = 0.f;

	std::string map_Kd; //filename texture
};

class MeshNode
{
public:
	MeshNode(MeshNode* parent, std::string name);
	~MeshNode();

	bool hasMeshNodes() { return _TriMeshes.size() > 0; }
	void setTransform(FbxAMatrix& fbxmatrix);

	std::string _name;
	Eigen::Matrix4d _transform;
	std::vector<MeshNode* > _children;
	std::vector<TriMesh* > _TriMeshes;
	MeshNode* _parent;
};

class FbxParser
{
public:
	FbxParser();
	~FbxParser();
	enum { E_NOERROR, E_NO_MESH, E_FAILOPENFILE, };

	bool LoadScene(const char* pFilename);

	void ExtractContent();

	int ExportOBJ(const char* pFilename);

private:
	void Initialize();
	void ExtractNode(FbxNode* pNode, int lDepth, MeshNode* pMeshNode);
	PolyMesh* ExtractMesh(FbxMesh* lMesh);
	void ExtractMaterial(FbxMesh* lMesh);
	void ExtractMaterialConnections(FbxMesh* lMesh);

	std::vector<PolyMesh* > Meshes;
	std::vector<TriMesh* > TriMeshes;
	std::map<std::string, Material*> Materials;
	std::map<FbxMesh*, TriMesh*> FbxMeshMap;
	std::vector<MeshNode* > Nodes;

	FbxManager* _pFbxManager;
	FbxScene* _pFbxScene;

};

