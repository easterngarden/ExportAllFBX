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

#include "FbxParser.h"
#include <stdio.h>
#include <filesystem>

namespace fs = std::filesystem;

static int WriteMaterials(std::map<std::string, Material*> &materials, const char * filename)
{
	std::string fileName = std::string(filename);
	fileName += ".mtl";

	if (materials.size() > 0)
	{
		FILE *fp;
		fopen_s(&fp, fileName.c_str(), "w");
		if (fp == NULL)return -1;

		fprintf(fp, "#\n# Wavefront material file\n# Created with Dolphin FBX \n#\n\n");

		int current = 0;

		std::map<std::string, Material*>::iterator iter2;
		for (iter2 = materials.begin(); iter2 != materials.end(); ++iter2)
		{
			Material* pMaterial = iter2->second;
			fprintf(fp, "newmtl %s\n", pMaterial->materialName.c_str());
			fprintf(fp, "Ka %f %f %f\n", pMaterial->Ka[0], pMaterial->Ka[1], pMaterial->Ka[2]);
			fprintf(fp, "Kd %f %f %f\n", pMaterial->Kd[0], pMaterial->Kd[1], pMaterial->Kd[2]);
			fprintf(fp, "Ks %f %f %f\n", pMaterial->Ks[0], pMaterial->Ks[1], pMaterial->Ks[2]);
			fprintf(fp, "Tr %f\n", pMaterial->Tr);
			fprintf(fp, "Ns %f\n", pMaterial->Ns);

			fprintf(fp, "\n");

		}

		fclose(fp);
	}
	return 0;
}


/////////////////////////////////////////////////////////////////////////////////
//
MeshNode::MeshNode(MeshNode* parent, std::string name)
	:_parent(parent), _name(name)
{
	_transform.setIdentity();
}

MeshNode::~MeshNode()
{
	std::vector<MeshNode*>::iterator iter;
	for (iter = _children.begin(); iter != _children.end(); ++iter)
	{
		MeshNode* pNode = *iter;
		delete pNode;
	}
	_children.clear();
}

void MeshNode::setTransform(FbxAMatrix& fbxmatrix)
{
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			_transform(i, j) = fbxmatrix.Get(i, j);
}

/////////////////////////////////////////////////////////////////////////////////
//
FbxParser::FbxParser()
	:_pFbxManager(NULL), _pFbxScene(NULL)
{
}

FbxParser::~FbxParser()
{
	if (_pFbxManager) 
		_pFbxManager->Destroy();

	std::vector<PolyMesh*>::iterator iter;
	for (iter = Meshes.begin(); iter != Meshes.end(); ++iter)
	{
		PolyMesh* pMesh = *iter;
		delete pMesh;
	}
	Meshes.clear();

	std::vector<TriMesh*>::iterator iter1;
	for (iter1 = TriMeshes.begin(); iter1 != TriMeshes.end(); ++iter1)
	{
		TriMesh* pTriMesh = *iter1;
		delete pTriMesh;
	}
	TriMeshes.clear();

	std::map<std::string, Material*>::iterator iter2;
	for (iter2 = Materials.begin(); iter2 != Materials.end(); ++iter2)
	{
		Material* pMaterial = iter2->second;
		delete pMaterial;
	}
	Materials.clear();

	std::vector<MeshNode*>::iterator iter3;
	for (iter3 = Nodes.begin(); iter3 != Nodes.end(); ++iter3)
	{
		MeshNode* pNode = *iter3;
		delete pNode;
	}
	Nodes.clear();
}

void FbxParser::Initialize()
{
	_pFbxManager = FbxManager::Create();
	if (!_pFbxManager)
	{
		FBXSDK_printf("Error: Unable to create FBX Manager!\n");
		throw std::runtime_error("Error: Unable to create FBX Manager.");
	}
	else FBXSDK_printf("Autodesk FBX SDK version %s\n", _pFbxManager->GetVersion());

	//Create an IOSettings object. This object holds all import/export settings.
	FbxIOSettings* ios = FbxIOSettings::Create(_pFbxManager, IOSROOT);
	_pFbxManager->SetIOSettings(ios);

	//Load plugins from the executable directory (optional)
	FbxString lPath = FbxGetApplicationDirectory();
	_pFbxManager->LoadPluginsDirectory(lPath.Buffer());

	//Create an FBX scene. This object holds most objects imported/exported from/to files.
	_pFbxScene = FbxScene::Create(_pFbxManager, "FBX Scene");
	if (!_pFbxScene)
	{
		FBXSDK_printf("Error: Unable to create FBX scene!\n");
		throw std::runtime_error("Error: Unable to create FBX scene.");
	}
}

bool FbxParser::LoadScene(const char* pFilename)
{
	if (!_pFbxManager)
	{
		Initialize();
	}
	if (!_pFbxManager || !_pFbxScene)
	{
		FBXSDK_printf("Error: Unable to create FBX scene!\n");
		return false;
	}

	int lFileMajor, lFileMinor, lFileRevision;
	int lSDKMajor, lSDKMinor, lSDKRevision;
	int lAnimStackCount;
	bool lStatus;
	char lPassword[1024];

	// Get the file version number generate by the FBX SDK.
	FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

	// Create an importer.
	FbxImporter* lImporter = FbxImporter::Create(_pFbxManager, "");

	// Initialize the importer by providing a filename.
	const bool lImportStatus = lImporter->Initialize(pFilename, -1, _pFbxManager->GetIOSettings());
	lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

	if (!lImportStatus)
	{
		FbxString error = lImporter->GetStatus().GetErrorString();
		FBXSDK_printf("Call to FbxImporter::Initialize() failed.\n");
		FBXSDK_printf("Error returned: %s\n\n", error.Buffer());

		if (lImporter->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
		{
			FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);
			FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);
		}

		return false;
	}

	FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);

	FbxIOSettings* ios = _pFbxManager->GetIOSettings();
	if (lImporter->IsFBX())
	{
		FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);
		if (ios)
		{
			ios->SetBoolProp(IMP_FBX_MATERIAL, true);
			ios->SetBoolProp(IMP_FBX_TEXTURE, true);
			ios->SetBoolProp(IMP_FBX_LINK, true);
			ios->SetBoolProp(IMP_FBX_SHAPE, true);
			ios->SetBoolProp(IMP_FBX_GOBO, true);
			ios->SetBoolProp(IMP_FBX_ANIMATION, true);
			ios->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
		}
	}

	// Import the scene.
	lStatus = lImporter->Import(_pFbxScene);
	if (lStatus == false && lImporter->GetStatus() == FbxStatus::ePasswordError)
	{
		FBXSDK_printf("Please enter password: ");

		lPassword[0] = '\0';

		FBXSDK_CRT_SECURE_NO_WARNING_BEGIN
			scanf("%s", lPassword);
		FBXSDK_CRT_SECURE_NO_WARNING_END

		FbxString lString(lPassword);

		if (ios)
		{
			ios->SetStringProp(IMP_FBX_PASSWORD, lString);
			ios->SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);
		}

		lStatus = lImporter->Import(_pFbxScene);

		if (lStatus == false && lImporter->GetStatus() == FbxStatus::ePasswordError)
		{
			FBXSDK_printf("\nPassword is wrong, import aborted.\n");
		}
	}

	if (!lStatus || (lImporter->GetStatus() != FbxStatus::eSuccess))
	{
		FBXSDK_printf("********************************************************************************\n");
		if (lStatus)
		{
			FBXSDK_printf("WARNING:\n");
			FBXSDK_printf("   The importer was able to read the file but with errors.\n");
			FBXSDK_printf("   Loaded scene may be incomplete.\n\n");
		}
		else
		{
			FBXSDK_printf("Importer failed to load the file!\n\n");
		}

		if (lImporter->GetStatus() != FbxStatus::eSuccess)
			FBXSDK_printf("   Last error message: %s\n", lImporter->GetStatus().GetErrorString());

		FbxArray<FbxString*> history;
		lImporter->GetStatus().GetErrorStringHistory(history);
		if (history.GetCount() > 1)
		{
			FBXSDK_printf("   Error history stack:\n");
			for (int i = 0; i < history.GetCount(); i++)
			{
				FBXSDK_printf("      %s\n", history[i]->Buffer());
			}
		}
		FbxArrayDelete<FbxString*>(history);
		FBXSDK_printf("********************************************************************************\n");
	}

	// Destroy the importer.
	lImporter->Destroy();

	return lStatus;

}

void FbxParser::ExtractContent()
{
	if (!_pFbxScene)
	{
		FBXSDK_printf("Error: No FBX scene!\n");
		return;
	}

	int lDepth = 0;
	FbxNode* rootNode = _pFbxScene->GetRootNode();
	MeshNode* pRootMeshNode = new MeshNode(NULL, rootNode->GetName());
	Nodes.push_back(pRootMeshNode);
	ExtractNode(rootNode, lDepth, pRootMeshNode);
}

void FbxParser::ExtractNode(FbxNode* pNode, int lDepth, MeshNode* pMeshNode)
{
	if (!pNode) return;

	std::string sname = pNode->GetName();
	FbxString lString;

	for (unsigned int i = 0; i < lDepth; i++)
	{
		lString += "     ";
	}

	FbxVector4 TVector, RVector, SVector;
	//FBXSDK_printf(lString + "%s Geometric Transformations\n", sname.c_str());
	// Translation
	TVector = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	//FBXSDK_printf(lString + "Translation: %f %f %f\n", TVector[0], TVector[1], TVector[2]);
	// Rotation
	RVector = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
	//FBXSDK_printf(lString + "Rotation:    %f %f %f\n", RVector[0], RVector[1], RVector[2]);
	// Scaling
	SVector = pNode->GetGeometricScaling(FbxNode::eSourcePivot);
	//FBXSDK_printf(lString + "Scaling:     %f %f %f\n", SVector[0], SVector[1], SVector[2]);

	FbxNodeAttribute* pNodeAttribute = pNode->GetNodeAttribute();
	FbxNodeAttribute::EType etype = FbxNodeAttribute::eUnknown;
	if (pNodeAttribute)
	{
		etype = pNodeAttribute->GetAttributeType();
		if (etype == FbxNodeAttribute::eMesh)
		{
			FbxMesh* pFbxMesh = (FbxMesh*)pNodeAttribute;
			assert(pFbxMesh);
			if (pFbxMesh && pFbxMesh->GetPolygonCount() == 0) //heck for group node was assigned as eMesh, identified by no mesh 
				etype = FbxNodeAttribute::eNull;
		}
	}
	if (pNodeAttribute && etype == FbxNodeAttribute::eMesh)
	{
		FbxMesh* pFbxMesh = (FbxMesh*)pNodeAttribute;
		assert(pFbxMesh);
		PolyMesh* pMesh = ExtractMesh(pFbxMesh);
		assert(pMesh);
		Meshes.push_back(pMesh);
		TriMesh* pTriMesh = new TriMesh(pMesh);
		assert(pTriMesh);
		TriMeshes.push_back(pTriMesh);
		FbxMeshMap[pFbxMesh] = pTriMesh;
		if (pMeshNode->_parent)
			pMeshNode->_parent->_TriMeshes.push_back(pTriMesh);
		pMeshNode->setTransform(pNode->EvaluateLocalTransform());
		ExtractMaterial(pFbxMesh);
		ExtractMaterialConnections(pFbxMesh);
	}
	else
	{
		pMeshNode->setTransform(pNode->EvaluateGlobalTransform());
		const unsigned int childCount = pNode->GetChildCount();
		for (unsigned int i = 0; i < childCount; i++)
		{
			FbxNode* pChildNode = pNode->GetChild(i);
			MeshNode* pDolChildNode = new MeshNode(pMeshNode, pChildNode->GetName());
			pMeshNode->_children.push_back(pDolChildNode);
			ExtractNode(pChildNode, lDepth+1, pDolChildNode);
		}
	}
}

PolyMesh* FbxParser::ExtractMesh(FbxMesh* pMesh)
{
	PolyMesh* polyMesh = new PolyMesh();
	FbxNode* pNode = pMesh->GetNode();
	polyMesh->name = pNode->GetName();

	int i, j, lPolygonCount = pMesh->GetPolygonCount();
	char header[100];

	polyMesh->nFaces = lPolygonCount;
	int controlPointCount = pMesh->GetControlPointsCount();
	polyMesh->nVertices = controlPointCount;
	int UVCount = pMesh->GetElementUVCount();
	int normalCount = pMesh->GetElementNormalCount();

	polyMesh->FaceIndices = std::unique_ptr<uint32_t[]>(new uint32_t[lPolygonCount]);
	polyMesh->Verts = std::unique_ptr<Vector3d[]>(new Vector3d[controlPointCount]);

	int vertsIndexCount = 0;
	for (i = 0; i < lPolygonCount; i++)
	{
		uint32_t lPolygonSize = pMesh->GetPolygonSize(i);
		vertsIndexCount += lPolygonSize;
	}
	polyMesh->VertsIndices = std::unique_ptr<uint32_t[]>(new uint32_t[vertsIndexCount]);
	polyMesh->UVs = std::unique_ptr<Vector2d[]>(new Vector2d[vertsIndexCount]);
	polyMesh->Normals = std::unique_ptr<Vector3d[]>(new Vector3d[vertsIndexCount]);
	polyMesh->UVIndices = std::unique_ptr<uint32_t[]>(new uint32_t[vertsIndexCount]);

	FbxVector4* lControlPoints = pMesh->GetControlPoints();
	for (i = 0; i < polyMesh->nVertices; i++)
	{
		FbxVector4 point = lControlPoints[i];
		polyMesh->Verts[i][0] = point[0];
		polyMesh->Verts[i][1] = point[1];
		polyMesh->Verts[i][2] = point[2];
	}

	int vertexId = 0;
	for (i = 0; i < lPolygonCount; i++)
	{
		int l;
		for (l = 0; l < pMesh->GetElementPolygonGroupCount(); l++)
		{
			FbxGeometryElementPolygonGroup* lePolgrp = pMesh->GetElementPolygonGroup(l);
			switch (lePolgrp->GetMappingMode())
			{
			case FbxGeometryElement::eByPolygon:
				if (lePolgrp->GetReferenceMode() == FbxGeometryElement::eIndex)
				{
					FBXSDK_sprintf(header, 100, "        Assigned to group: ");
					int polyGroupId = lePolgrp->GetIndexArray().GetAt(i);
					break;
				}
			default:
				// any other mapping modes don't make sense
				FBXSDK_printf("        \"unsupported group assignment\"");
				break;
			}
		}

		int lPolygonSize = pMesh->GetPolygonSize(i);
		polyMesh->FaceIndices[i] = lPolygonSize;

		for (j = 0; j < lPolygonSize; j++)
		{
			int lControlPointIndex = pMesh->GetPolygonVertex(i, j);
			if (lControlPointIndex < 0)
			{
				FBXSDK_printf("            Coordinates: Invalid index found!");
				continue;
			}
			else
			{
				polyMesh->VertsIndices[vertexId] = lControlPointIndex;
			}

			for (l = 0; l < pMesh->GetElementUVCount(); ++l)
			{
				FbxGeometryElementUV* leUV = pMesh->GetElementUV(l);
				FBXSDK_sprintf(header, 100, "            Texture UV: ");

				switch (leUV->GetMappingMode())
				{
				default:
					break;
				case FbxGeometryElement::eByControlPoint:
					switch (leUV->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
						polyMesh->UVs[vertexId][0] = leUV->GetDirectArray().GetAt(lControlPointIndex)[0];
						polyMesh->UVs[vertexId][1] = leUV->GetDirectArray().GetAt(lControlPointIndex)[1];
						break;
					case FbxGeometryElement::eIndexToDirect:
					{
						int id = leUV->GetIndexArray().GetAt(lControlPointIndex);
						polyMesh->UVs[vertexId][0] = leUV->GetDirectArray().GetAt(id)[0];
						polyMesh->UVs[vertexId][1] = leUV->GetDirectArray().GetAt(id)[1];
					}
					break;
					default:
						break; // other reference modes not shown here!
					}
					break;

				case FbxGeometryElement::eByPolygonVertex:
				{
					int lTextureUVIndex = pMesh->GetTextureUVIndex(i, j);
					polyMesh->UVIndices[vertexId] = lTextureUVIndex;
					switch (leUV->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
					case FbxGeometryElement::eIndexToDirect:
					{
						polyMesh->UVs[vertexId][0] = leUV->GetDirectArray().GetAt(lTextureUVIndex)[0];
						polyMesh->UVs[vertexId][1] = leUV->GetDirectArray().GetAt(lTextureUVIndex)[1];
					}
					break;
					default:
						break; // other reference modes not shown here!
					}
				}
				break;

				case FbxGeometryElement::eByPolygon: // doesn't make much sense for UVs
				case FbxGeometryElement::eAllSame:   // doesn't make much sense for UVs
				case FbxGeometryElement::eNone:       // doesn't make much sense for UVs
					break;
				}

				break; //heck: one uv per vertex as autodesk fbx converter
			}

			for (l = 0; l < pMesh->GetElementNormalCount(); ++l)
			{
				FbxGeometryElementNormal* leNormal = pMesh->GetElementNormal(l);
				FBXSDK_sprintf(header, 100, "            Normal: ");

				if (leNormal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
				{
					switch (leNormal->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
						polyMesh->Normals[vertexId][0] = leNormal->GetDirectArray().GetAt(vertexId)[0];
						polyMesh->Normals[vertexId][1] = leNormal->GetDirectArray().GetAt(vertexId)[1];
						polyMesh->Normals[vertexId][2] = leNormal->GetDirectArray().GetAt(vertexId)[0];
						break;
					case FbxGeometryElement::eIndexToDirect:
					{
						int id = leNormal->GetIndexArray().GetAt(vertexId);
						polyMesh->Normals[vertexId][0] = leNormal->GetDirectArray().GetAt(id)[0];
						polyMesh->Normals[vertexId][1] = leNormal->GetDirectArray().GetAt(id)[1];
						polyMesh->Normals[vertexId][2] = leNormal->GetDirectArray().GetAt(id)[0];
					}
					break;
					default:
						break; // other reference modes not shown here!
					}
				}
				else if (leNormal->GetMappingMode() == FbxGeometryElement::eByControlPoint)
				{
					switch (leNormal->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
						polyMesh->Normals[vertexId][0] = leNormal->GetDirectArray().GetAt(lControlPointIndex)[0];
						polyMesh->Normals[vertexId][1] = leNormal->GetDirectArray().GetAt(lControlPointIndex)[1];
						polyMesh->Normals[vertexId][2] = leNormal->GetDirectArray().GetAt(lControlPointIndex)[2];
						break;
					case FbxGeometryElement::eIndexToDirect:
					{
						int id = leNormal->GetIndexArray().GetAt(lControlPointIndex);
						polyMesh->Normals[vertexId][0] = leNormal->GetDirectArray().GetAt(id)[0];
						polyMesh->Normals[vertexId][1] = leNormal->GetDirectArray().GetAt(id)[1];
						polyMesh->Normals[vertexId][2] = leNormal->GetDirectArray().GetAt(id)[2];
					}
					break;
					default:
						break; // other reference modes not shown here!
					}
				}
				else {
					assert(false); //not supported
				}

			}
			vertexId++;
		} // for polygonSize
	} // for polygonCount

	return polyMesh;
}

int FbxParser::ExportOBJ(const char* pFilename)
{
	if (TriMeshes.size() == 0)
		return E_NO_MESH;
	
	FILE* fp; 
	fopen_s(&fp, pFilename, "w");
	if (fp == NULL) return E_FAILOPENFILE;

	std::string shortFilename(pFilename);
	int LastSlash = shortFilename.size() - 1;
	while (LastSlash >= 0 && shortFilename[LastSlash] != '/')
		--LastSlash;
	shortFilename = shortFilename.substr(LastSlash + 1);
	int len = shortFilename.length() - 4;
	shortFilename = shortFilename.substr(0, len);

	fprintf(fp, "###################\n#\n# Wavefront OBJ File\n# Created with Dolphin FBX\n#\n###################\n\n");

	//library material
	if (Materials.size() > 0)
		fprintf(fp, "mtllib ./%s.mtl\n\n", shortFilename.c_str());
	
	uint32_t vplus = 1, vtplus = 1;
	for (auto iter = TriMeshes.begin(); iter != TriMeshes.end(); ++iter)
	{
		TriMesh* m = *iter;
		fprintf(fp, "g %s\n", m->name.c_str());
		/*********************************** VERTICES *********************************/
		for (uint32_t i = 0, k = 0; i < m->numVert; ++i)
		{
			fprintf(fp, "v %f %f %f\n", m->P[i][0], m->P[i][1], m->P[i][2]);
		}

		for (uint32_t i = 0, k = 0; i < m->numUV; ++i)
		{
			fprintf(fp, "vt %f %f\n", m->UV[i][0], m->UV[i][1]);
		}

		for (uint32_t i = 0, k = 0; i < m->numVert; ++i)
		{
			fprintf(fp, "vn %f %f %f\n", m->PN[i][0], m->PN[i][1], m->PN[i][2]);
		}

		if (!m->matname.empty())
			fprintf(fp, "usemtl %s\n", m->matname.c_str());
		uint32_t l = 0;
		for (uint32_t i = 0, k = 0; i < m->numTris; ++i)
		{
			fprintf(fp, "f ");
			for (unsigned k = 0; k < 3; ++k)
			{
				int vn = m->triIndex[l + k] + vplus;
				int tn = m->UVIndices[l + k] + vtplus;
				fprintf(fp, "%d/%d/%d", vn, tn, vn);
				fprintf(fp, " ");
			}
			fprintf(fp, "\n");
			l += 3;
		}

		vplus += m->numVert;
		vtplus += m->numUV;
	}
	fclose(fp); 

	std::string mtlfile = shortFilename;
	WriteMaterials(Materials, shortFilename.c_str());

	return E_NOERROR;
}

void FbxParser::ExtractMaterial(FbxMesh* pMesh)
{
	if (!pMesh)
		return;

	int lMaterialCount = 0;
	FbxNode* lNode = pMesh->GetNode();
	assert(lNode);
	lMaterialCount = lNode->GetMaterialCount();

	if (lMaterialCount > 0)
	{
		FbxPropertyT<FbxDouble3> dFbxAmbient, dFbxDiffuse, dFbxSpecular, dFbxEmissive;
		FbxPropertyT<FbxDouble> dFbxTransparency, dFbxShininess, dFbxReflectivity;
		FbxColor theColor;

		for (int lCount = 0; lCount < lMaterialCount; lCount++)
		{
			FbxSurfaceMaterial *lMaterial = lNode->GetMaterial(lCount);
			std::string matName = lMaterial->GetName();
			if (Materials.find(matName) == Materials.end())
			{
				Material* pMaterial = new Material;
				assert(pMaterial);
				pMaterial->index = lCount;
				pMaterial->materialName = lMaterial->GetName();

				if (lMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
				{
					// the Ambient Color
					dFbxAmbient = ((FbxSurfacePhong *)lMaterial)->Ambient;
					theColor.Set(dFbxAmbient.Get()[0], dFbxAmbient.Get()[1], dFbxAmbient.Get()[2]);

					// the Diffuse Color
					dFbxDiffuse = ((FbxSurfacePhong *)lMaterial)->Diffuse;
					theColor.Set(dFbxDiffuse.Get()[0], dFbxDiffuse.Get()[1], dFbxDiffuse.Get()[2]);

					// the Specular Color (unique to Phong materials)
					dFbxSpecular = ((FbxSurfacePhong *)lMaterial)->Specular;
					theColor.Set(dFbxSpecular.Get()[0], dFbxSpecular.Get()[1], dFbxSpecular.Get()[2]);

					// Display the Emissive Color
					dFbxEmissive = ((FbxSurfacePhong *)lMaterial)->Emissive;
					theColor.Set(dFbxEmissive.Get()[0], dFbxEmissive.Get()[1], dFbxEmissive.Get()[2]);

					//Opacity is Transparency factor now
					dFbxTransparency = ((FbxSurfacePhong *)lMaterial)->TransparencyFactor;

					// the Shininess
					dFbxShininess = ((FbxSurfacePhong *)lMaterial)->Shininess;

					// the Reflectivity
					dFbxReflectivity = ((FbxSurfacePhong *)lMaterial)->ReflectionFactor;

					pMaterial->Ks = Vector3d(dFbxSpecular.Get()[0], dFbxSpecular.Get()[1], dFbxSpecular.Get()[2]);
					pMaterial->Ns = dFbxShininess.Get();
				}
				else if (lMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
				{
					// We found a Lambert material. Display its properties.
					// the Ambient Color
					dFbxAmbient = ((FbxSurfaceLambert *)lMaterial)->Ambient;
					theColor.Set(dFbxAmbient.Get()[0], dFbxAmbient.Get()[1], dFbxAmbient.Get()[2]);

					// the Diffuse Color
					dFbxDiffuse = ((FbxSurfaceLambert *)lMaterial)->Diffuse;
					theColor.Set(dFbxDiffuse.Get()[0], dFbxDiffuse.Get()[1], dFbxDiffuse.Get()[2]);

					// the Emissive
					dFbxEmissive = ((FbxSurfaceLambert *)lMaterial)->Emissive;
					theColor.Set(dFbxEmissive.Get()[0], dFbxEmissive.Get()[1], dFbxEmissive.Get()[2]);

					// Display the Opacity
					dFbxTransparency = ((FbxSurfaceLambert *)lMaterial)->TransparencyFactor;
				}
				else
				{
					FBXSDK_printf("Unknown or unsupported type of Material");
					continue;
				}

				pMaterial->Ka = Vector3d(dFbxAmbient.Get()[0], dFbxAmbient.Get()[1], dFbxAmbient.Get()[2]);
				pMaterial->Kd = Vector3d(dFbxDiffuse.Get()[0], dFbxDiffuse.Get()[1], dFbxDiffuse.Get()[2]);
				pMaterial->Tr = 1.0 - dFbxTransparency.Get();
				Materials[matName] = pMaterial;
			}
		}
	}
}

void FbxParser::ExtractMaterialConnections(FbxMesh* pMesh)
{
	int i, l, lPolygonCount = pMesh->GetPolygonCount();

	//check whether the material maps with only one mesh
	bool lIsAllSame = true;
	for (l = 0; l < pMesh->GetElementMaterialCount(); l++)
	{
		FbxGeometryElementMaterial* lMaterialElement = pMesh->GetElementMaterial(l);
		if (lMaterialElement->GetMappingMode() == FbxGeometryElement::eByPolygon)
		{
			lIsAllSame = false;
			break;
		}
	}

	//For eAllSame mapping type, just out the material and texture mapping info once
	if (lIsAllSame)
	{
		for (l = 0; l < pMesh->GetElementMaterialCount(); l++)
		{
			FbxGeometryElementMaterial* lMaterialElement = pMesh->GetElementMaterial(l);
			if (lMaterialElement->GetMappingMode() == FbxGeometryElement::eAllSame)
			{
				FbxSurfaceMaterial* lMaterial = pMesh->GetNode()->GetMaterial(lMaterialElement->GetIndexArray().GetAt(0));
				int lMatId = lMaterialElement->GetIndexArray().GetAt(0);
				if (lMatId >= 0)
				{
					//FBXSDK_printf("        All polygons share the same material in mesh ", l);
					TriMesh* pTriMesh = FbxMeshMap[pMesh];
					pTriMesh->matname = lMaterial->GetName();
				}
			}
		}

		//no material
		if (l == 0)
			FBXSDK_printf("        no material applied");
	}
	//For eByPolygon mapping type, just out the material and texture mapping info once
	else
	{
		//FBXSDK_printf("   not supported material connection.\n");
		TriMesh* pTriMesh = FbxMeshMap[pMesh];
		for (i = 0; i < lPolygonCount; i++)
		{
			for (l = 0; l < pMesh->GetElementMaterialCount(); l++)
			{
				FbxGeometryElementMaterial* lMaterialElement = pMesh->GetElementMaterial(l);
				FbxSurfaceMaterial* lMaterial = NULL;
				int lMatId = -1;
				lMaterial = pMesh->GetNode()->GetMaterial(lMaterialElement->GetIndexArray().GetAt(i));
				lMatId = lMaterialElement->GetIndexArray().GetAt(i);

				if (lMatId >= 0)
				{
					pTriMesh->matname = lMaterial->GetName(); //temperary hack, need map matid <--> material name and tris <--> matid
				}
			}
		}
	}
}

