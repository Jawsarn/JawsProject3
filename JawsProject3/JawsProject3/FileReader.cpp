#include "FileReader.h"


FileReader::FileReader(void)
{
}


FileReader::~FileReader(void)
{
}

HRESULT FileReader::ReadObjFile(std::string filename,		//.obj filename
	ID3D11Buffer* vertBuff,			//mesh vertex buffer
	ID3D11Device* device,
	ID3D11DeviceContext* deviceContext,
	std::vector<std::vector<GraphicHelper::SimpleVertex>> &vertices,
	std::vector<Material> &outMaterials, 
	std::vector<ID3D11ShaderResourceView*> &textureViews
	)
{
	HRESULT hr = S_OK;

	std::vector<Group> groups;
	Group tempGroup;
	std::vector<std::string> accMaterialNames;
	
	std::vector<XMFLOAT3> temp_vertices;
	std::vector<XMFLOAT2> temp_uvs;
	std::vector<XMFLOAT3> temp_normals;

	

	char matFileName[128];
	

	FILE * file;
	fopen_s(&file,filename.c_str(), "r");

	

	if( file == NULL )
	{
		printf("Impossible to open the file !\n");
		return false;
	}

	while(true)
	{
		char lineHeader[128];

		// read the first word of the line
		int res = fscanf_s(file, "%s", lineHeader, _countof(lineHeader));
		if (res == EOF)
		{
			break; // EOF = End Of File. Quit the loop.
		}

		if ( strcmp( lineHeader, "v" ) == 0 )
		{
			XMFLOAT3 vertex;
			fscanf_s(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z );
			temp_vertices.push_back(vertex);
		}
		else if(strcmp( lineHeader, "vt" ) == 0 )
		{
			XMFLOAT2 uv;
			fscanf_s(file, "%f %f\n", &uv.x, &uv.y );
			temp_uvs.push_back(uv);
				// else : parse lineHeader
		}
		else if ( strcmp( lineHeader, "vn" ) == 0 )
		{
			XMFLOAT3 normal;
			fscanf_s(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z );
			temp_normals.push_back(normal);
		}
		else if ( strcmp( lineHeader, "f" ) == 0 )
		{
			//std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf_s(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n",
				&vertexIndex[0], &uvIndex[0], &normalIndex[0],
				&vertexIndex[1], &uvIndex[1], &normalIndex[1],
				&vertexIndex[2], &uvIndex[2], &normalIndex[2] );
			if (matches != 9){
				printf("File can't be read by our simple parser : ( Try exporting with other options\n");
				return false;
			}
			tempGroup.vertexIndices.push_back(vertexIndex[0]);
			tempGroup.vertexIndices.push_back(vertexIndex[1]);
			tempGroup.vertexIndices.push_back(vertexIndex[2]);
			tempGroup.uvIndices.push_back(uvIndex[0]);
			tempGroup.uvIndices.push_back(uvIndex[1]);
			tempGroup.uvIndices.push_back(uvIndex[2]);
			tempGroup.normalIndices.push_back(normalIndex[0]);
			tempGroup.normalIndices.push_back(normalIndex[1]);
			tempGroup.normalIndices.push_back(normalIndex[2]);
		}
		else if( strcmp( lineHeader, "mtllib" ) == 0 )
		{
			int outp = fscanf_s(file, "%s\n", matFileName, _countof(matFileName));
			if(outp != 1)
			{
				return false;
				//I guess? 
			}
		}
		else if( strcmp( lineHeader, "usemtl" ) == 0 )
		{
			char temp[128];
			int outp = fscanf_s(file, "%s\n", temp, _countof(temp));
			if(outp != 1)
			{
				return false;
				//I guess? 
			}
			tempGroup.materialName = temp;
		}
		else if( strcmp( lineHeader, "g" ) == 0 )
		{
			char temp[128];
			int outp = fscanf_s(file, "%s\n", temp, _countof(temp));
			
			if(outp != 1)
			{
				return false;
				//I guess? 
			}
			if(tempGroup.groupName == "") //if first group
			{
				tempGroup.groupName = temp;
			}
			else
			{
				groups.push_back(tempGroup);
				tempGroup.normalIndices.clear();
				tempGroup.uvIndices.clear();
				tempGroup.vertexIndices.clear();
				tempGroup.materialName = "";
				tempGroup.groupName = temp;
			}
		}
		else if( strcmp( lineHeader, "s" ) == 0 )
		{
			//save side (could be averaged)
		}
	}
	groups.push_back(tempGroup);
	
	for (int i = 0; i < groups.size(); i++)
	{
		if(groups[i].vertexIndices.size() > 0)
		{
			std::vector<GraphicHelper::SimpleVertex> tempVerticeList;
			std::vector <XMFLOAT3>  out_vertices;
			std::vector <XMFLOAT2>  out_uvs;
			std::vector <XMFLOAT3>  out_normals;


			for(unsigned int j = 0; j< groups[i].vertexIndices.size(); j++)
			{
				unsigned int vertexIndex = groups[i].vertexIndices[j];
				XMFLOAT3  vertex = temp_vertices[vertexIndex - 1];
				out_vertices.push_back(vertex);
			}
			for(unsigned int j = 0; j<groups[i].normalIndices.size(); j++)
			{
				unsigned int normalIndex = groups[i].normalIndices[j];
				XMFLOAT3  normal = temp_normals[normalIndex - 1];
				out_normals.push_back(normal);
			
			}
			for(unsigned int j = 0; j<groups[i].uvIndices.size(); j++)
			{
				unsigned int uvIndex = groups[i].uvIndices[j];
				XMFLOAT2  uv = temp_uvs[uvIndex - 1];
				out_uvs.push_back(uv);
			}
			for (int j = 0; j < out_vertices.size(); j++)
			{
				GraphicHelper::SimpleVertex temp;
				temp.Pos = out_vertices[j];
				temp.Norm = out_normals[j];
				temp.Tex = out_uvs[j];
				tempVerticeList.push_back(temp);
			}
			accMaterialNames.push_back(groups[i].materialName);
			vertices.push_back(tempVerticeList);
		}
	}


	hr = ReadMaterial(device,matFileName,accMaterialNames,outMaterials,textureViews);
	fclose(file);

	return hr;
}

HRESULT FileReader::ReadMaterial(ID3D11Device* device,
							  std::string filename, 
							  std::vector<std::string> materialNames, 
							  std::vector<Material> &outMaterials, 
							  std::vector<ID3D11ShaderResourceView*> &textureViews)
{
	HRESULT hr = S_OK;
	FILE * file;
	fopen_s(&file,filename.c_str(), "r");

	if( file == NULL )
	{
		printf("Impossible to open the file !\n");
		return false;
	}
	std::vector<MatGroup> matGroups;

	ID3D11ShaderResourceView* tempResource;
	std::string tempName;
	Material tempMat;
	tempMat.Ambient = XMFLOAT4(0,0,0,0);
	tempMat.Diffuse = XMFLOAT4(0,0,0,0);
	tempMat.Specular = XMFLOAT4(0,0,0,0);
	tempMat.Transmission = XMFLOAT4(0,0,0,0);
	bool firstMat = true;

	while(true)
	{
		char lineHeader[128];

		// read the first word of the line
		int res = fscanf_s(file, "%s", lineHeader, _countof(lineHeader));
		if (res == EOF)
		{
			break; // EOF = End Of File. Quit the loop.
		}
		else if ( strcmp( lineHeader, "newmtl" ) == 0 )
		{
			char tempN[128];
			int outp = fscanf_s(file, "%s\n", tempN, _countof(tempN));
			if(outp != 1)
			{
				return false;
				//I guess? 
			}

			if(!firstMat)
			{
				MatGroup temp;
				temp.name = tempName;
				temp.mMaterial = tempMat;
				temp.tempResource = tempResource;
				matGroups.push_back(temp);

				outMaterials.push_back(tempMat);
				tempMat.Ambient = XMFLOAT4(0,0,0,0);
				tempMat.Diffuse = XMFLOAT4(0,0,0,0);
				tempMat.Specular = XMFLOAT4(0,0,0,0);
				tempMat.Transmission = XMFLOAT4(0,0,0,0);
			}
			else
			{
				firstMat = false;
			}
			tempName = tempN;
			
		}
		else if ( strcmp( lineHeader, "Ks" ) == 0 ) //diffuse color
		{
			XMFLOAT4 specCol;
			fscanf_s(file, "%f %f %f\n", &specCol.x, &specCol.y, &specCol.z );
			specCol.w = 1;
			tempMat.Diffuse = specCol;
		}
		else if ( strcmp( lineHeader, "Kd" ) == 0 ) //diffuse color
		{
			XMFLOAT4 difCol;
			fscanf_s(file, "%f %f %f\n", &difCol.x, &difCol.y, &difCol.z );
			difCol.w = 1;
			tempMat.Diffuse = difCol;
		}
		else if ( strcmp( lineHeader, "Ka" ) == 0 ) //ambient
		{
			XMFLOAT4 ambCol;
			fscanf_s(file, "%f %f %f\n", &ambCol.x, &ambCol.y, &ambCol.z );
			ambCol.w = 1;
			tempMat.Diffuse = ambCol;
		}
		else if ( strcmp( lineHeader, "Tf" ) == 0 ) //transmission filter
		{
			XMFLOAT4 trans;
			fscanf_s(file, "%f %f %f\n", &trans.x, &trans.y, &trans.z );
			trans.w = 1;
			tempMat.Transmission = trans;
		}
		else if ( strcmp( lineHeader, "map_Kd" ) == 0 )
		{
			char temp[128];
			int outp = fscanf_s(file, "%s\n", temp, _countof(temp));
			if(outp != 1)
			{
				return false;
				//I guess? 
			}
			while(outMaterials.size()> textureViews.size())
			{
				textureViews.push_back(nullptr); //dont think this works
			}

			size_t newsize = strlen(temp) + 1;
			wchar_t * texName = new wchar_t[newsize];
			size_t convertedChars = 0;
			mbstowcs_s(&convertedChars, texName, newsize, temp, _TRUNCATE);

			
			
			hr = CreateDDSTextureFromFile(device , texName, nullptr, &tempResource);
			delete(texName);
		}
	}
	MatGroup temp;
	temp.name = tempName;
	temp.mMaterial = tempMat;
	temp.tempResource = tempResource;
	matGroups.push_back(temp);
	
	for (int i = 0; i < materialNames.size(); i++)
	{
		for (int j = 0; j < matGroups.size(); j++)
		{
			if(materialNames[i] == matGroups[i].name)
			{
				outMaterials.push_back(matGroups[j].mMaterial);
				textureViews.push_back(matGroups[j].tempResource);
			}
			else
			{

			}
		}
	}
	
	
	return hr;
}


void FileReader::Cleanup()
{
	if(meshVertBuff) meshVertBuff->Release();

	if(meshIndexBuffer) meshIndexBuffer->Release();
}