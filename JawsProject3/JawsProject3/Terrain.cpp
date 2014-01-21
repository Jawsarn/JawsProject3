#include "Terrain.h"




Terrain::Terrain()
{
	g_pQuadPatchVertexBuffer = nullptr;
	g_pQuadPatchIndexBuffer = nullptr;
	g_pQuadSamplerState = nullptr;
	graphicHelper = GraphicHelper();
	g_pVertexShader = nullptr;
	g_pPixelShader = nullptr;
	g_pGeometryShader = nullptr;
	g_pHullShader = nullptr;
	g_pDomainShader = nullptr;
	g_pVertexLayout = nullptr;
	
}

Terrain::~Terrain()
{
	
}

HRESULT Terrain::Initialize(ID3D11Device * device, ID3D11DeviceContext* deviceContext)
{
	HRESULT hr = S_OK;
	
	InitializeInfo();
	

	//not sure where to put
	mNumPatchVerticesRows = ((mInfo.HeightmapHeight - 1) / CellsPerPatch + 1);  //not sure why it wanted mInfo.HeightmapHeight -1 ...
	mNumPatchVerticesCols = ((mInfo.HeightmapWidth - 1) / CellsPerPatch + 1);

	
	mNumPatchVertices = mNumPatchVerticesRows*mNumPatchVerticesCols;
	mNumPatchQuadFaces = (mNumPatchVerticesRows-1)*(mNumPatchVerticesCols-1);


	LoadHeightmap();
	Smooth();
	CalculateNormalMap();
	CalcAllPatchBoundsY();

	LoadBlendMap();
	LoadTextures(device);

	hr = BuildBlendMapShaderResourceView(mBlendMap,device);
	if( FAILED( hr ) )
        return hr;

	hr = BuildHeightmapShaderResourceView(device);
	if( FAILED( hr ) )
        return hr;

	hr = BuildNormalmapShaderResourceView(device);
	if( FAILED( hr ) )
        return hr;

	hr = BuildQuadPatchVertexBuffer(device, deviceContext);
	if( FAILED( hr ) )
        return hr;
	hr = BuildQuadPatchIndexBuffer(device, deviceContext);
	if( FAILED( hr ) )
        return hr;

	hr = CreateHeightmapSamplerState(device);
	if( FAILED( hr ) )
        return hr;

	hr = CreateTextureSamplerState(device);
	if( FAILED( hr ) )
        return hr;
	
	hr = InitializeCBuffers(device,deviceContext);
	if( FAILED( hr ) )
        return hr;

	//compile vertex shader
	hr = CompileVertexShaders(device,deviceContext);
	if( FAILED( hr ) )
        return hr;
	
	//compile pixel shaders
	hr = CompilePixelShaders(device);
	if( FAILED( hr ) )
        return hr;

	//compile geometry shaders
	//hr = CompileGeometryShaders(device);
	//if( FAILED (hr) )
		//return hr;

	//compile hull shaders
	hr = CompileHullShaders(device);
	if( FAILED (hr) )
		return hr;

	//compile domain shaders
	hr = CompileDomainShaders(device);
	if( FAILED (hr) )
		return hr;

	return hr;
}

HRESULT Terrain::InitializeCBuffers(ID3D11Device * device,ID3D11DeviceContext* deviceContext)
{
	HRESULT hr = S_OK;

	g_World = XMMatrixIdentity();

	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(CBOnButtonPress);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;

	hr = device->CreateBuffer( &bd, nullptr, &g_CBOnButtonPress );
    if( FAILED( hr ) )
        return hr;
	
	CBOnButtonPress cbBP;
	cbBP.gMaxDist = 900;
	cbBP.gMinDist = 400;
	cbBP.gMaxTess = 6;
	cbBP.gMinTess = 0;
	cbBP.gTexScale = XMFLOAT2(80.0f, 80.0f); //change when addex texture
	cbBP.paddin1 = 0.0f;
	cbBP.padding2 = 0.0f;
	cbBP.World = XMMatrixTranspose(g_World);
	deviceContext->UpdateSubresource( g_CBOnButtonPress, 0, nullptr, &cbBP, 0, 0 );

	
}

void Terrain::InitializeInfo()
{
	//the dimension of a heightmap should be 2^x +1
	mInfo.HeightmapFileName = L"World.raw";
	//mInfo.CellSpacing = 0.2;
	mInfo.CellSpacing = 1; //world
	//smalwhite
	//mInfo.HeightmapHeight = 65;
	//mInfo.HeightmapWidth = 65;
	//for flat
	/*mInfo.HeightmapHeight = 257;
	mInfo.HeightmapWidth = 129;*/
	//for europe
	mInfo.HeightmapHeight = 1025;
	mInfo.HeightmapWidth = 2049;
	//mInfo.HeightmapWidth = 1025;
	//mInfo.HeightScale = 5;  //world
	mInfo.HeightScale = 20;  //something else
	mInfo.BlenMapFileName = L"BlendMap.raw";
	mInfo.LayerMapFilename1 = L"tex1.dds";
	mInfo.LayerMapFilename2 = L"tex2.dds";
	mInfo.LayerMapFilename3 = L"tex3.dds";
	mInfo.LayerMapFilename4 = L"tex4.dds";
}

HRESULT Terrain::LoadTextures(ID3D11Device * device)
{
	HRESULT hr = S_OK;

	hr = CreateDDSTextureFromFile(device, mInfo.LayerMapFilename1.c_str(), nullptr, &mTexture1SRV);
	if( FAILED( hr ) )
        return hr;
	
	hr = CreateDDSTextureFromFile(device, mInfo.LayerMapFilename2.c_str(), nullptr, &mTexture2SRV);
	if( FAILED( hr ) )
        return hr;

	hr = CreateDDSTextureFromFile(device, mInfo.LayerMapFilename3.c_str(), nullptr, &mTexture3SRV);
	if( FAILED( hr ) )
        return hr;

	hr = CreateDDSTextureFromFile(device, mInfo.LayerMapFilename4.c_str(), nullptr, &mTexture4SRV);
	if( FAILED( hr ) )
        return hr;

	return hr;
}


void Terrain::LoadBlendMap()
{
	UINT fullMapSize = mInfo.HeightmapHeight*mInfo.HeightmapWidth;

	std::vector<unsigned char> in(fullMapSize*4);

	//open file
	std::ifstream inFile;
	inFile.open(mInfo.BlenMapFileName.c_str(),std::ios_base::binary);
	
	//load map
	if(inFile)
	{
		inFile.read((char*)&in[0],(std::streamsize)in.size());
		inFile.close();
	}

	mBlendMap.resize(fullMapSize, XMFLOAT4(0,0,0,0));

	int a = 0;
	for (UINT i = 0; i < fullMapSize; i++)
	{
		mBlendMap[i].x = (in[a]/255.0f);
		mBlendMap[i].y = (in[a + 1]/255.0f);	
		mBlendMap[i].z = (in[a + 2]/255.0f);
		mBlendMap[i].w = (in[a + 3]/255.0f);
		a += 4;
	}
}


HRESULT Terrain::BuildBlendMapShaderResourceView(std::vector<XMFLOAT4>& inblendMap, ID3D11Device * device)
{
	HRESULT hr = S_OK;

	// Create shader resource view
    D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory( &texDesc, sizeof(texDesc) );
	texDesc.Width = mInfo.HeightmapWidth;
	texDesc.Height = mInfo.HeightmapHeight;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;
	

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &inblendMap[0];
	data.SysMemPitch = mInfo.HeightmapWidth*sizeof(XMFLOAT4);
	data.SysMemSlicePitch = 0;

	ID3D11Texture2D* texBlenMap = 0;
	
	hr = device->CreateTexture2D( &texDesc, &data, &texBlenMap );
	if( FAILED( hr ) )
        return hr;

	// 
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory( &srvDesc, sizeof(srvDesc) );
	srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;

	hr = device->CreateShaderResourceView( texBlenMap, &srvDesc, &mBlendMapSRV); 


	return hr;


}

void Terrain::LoadHeightmap(void)
{
	UINT fullMapSize = mInfo.HeightmapHeight*mInfo.HeightmapWidth;

	std::vector<unsigned char> in(fullMapSize);

	//open file
	std::ifstream inFile;
	inFile.open(mInfo.HeightmapFileName.c_str(),std::ios_base::binary);
	
	//load map
	if(inFile)
	{
		inFile.read((char*)&in[0],(std::streamsize)in.size());
		inFile.close();
	}

	//resize my heightmap
	mHeightmap.resize(fullMapSize,0);

	for (UINT i = 0; i < fullMapSize; i++)
	{
		mHeightmap[i] = (in[i] /255.0f)*mInfo.HeightScale;	// value from 0 - 1 * heightscale
	}

	int a = 5;
}

void Terrain::Smooth(void)
{
	std::vector<float> dest(mHeightmap.size());
	for (UINT i = 0; i < mInfo.HeightmapHeight; i++)
	{
		for (UINT j = 0; j < mInfo.HeightmapWidth; j++)
		{
			dest[i*mInfo.HeightmapWidth + j] = Average(i,j);
		}
	}
	mHeightmap = dest;
}

void Terrain::CalculateNormalMap(void)
{
	std::vector<XMFLOAT3> normMap(mHeightmap.size());

	//calculate each quad
	for (UINT z = 0; z < mInfo.HeightmapHeight; z++)
	{
		for (UINT x = 0; x < mInfo.HeightmapWidth; x++)
		{
			XMFLOAT3 n1 = CalculateNormal(x,z,	x + 1,z,	x,z + 1); 
			XMFLOAT3 n2 = CalculateNormal(x,z,	x,z - 1,	x + 1,z);
			XMFLOAT3 n3 = CalculateNormal(x,z,	x - 1,z,	x,z - 1);
			XMFLOAT3 n4 = CalculateNormal(x,z,	x,z + 1,	x - 1,z);

			XMFLOAT3 norm = XMFLOAT3((n1.x + n2.x + n3.x + n4.x),(n1.y + n2.y + n3.y + n4.y),(n1.z + n2.z + n3.z + n4.z));
			
			XMVECTOR n = XMLoadFloat3(&norm);
			n = XMVector3Normalize(n);

			XMStoreFloat3(&norm, n);

			normMap[z*mInfo.HeightmapWidth + x] = norm;
		}
	}

	mNormalmap.resize(mHeightmap.size());
	mNormalmap = normMap;

	
}

XMFLOAT3 Terrain::CalculateNormal(float x, float z, float kx , float kz, float mx, float mz)
{
	float halfWidth = 0.5f*GetWidth();
	float halfDepth = 0.5f*GetDepth();

	//calculate the range of each patch
	float patchWidth = GetWidth() / ((mNumPatchVerticesCols-1)*CellsPerPatch);
	float patchDepth = GetDepth() / ((mNumPatchVerticesRows-1)*CellsPerPatch);
	//-halfWidth + j*patchWidth;
	//float z = halfDepth - i*patchDepth;
	if(InBounds(kz,kx)&&InBounds(mz,mx))
	{
		//cell space need to count in somewhere? can't use HeightmapWidth and Height I think
		XMFLOAT3 v1 = XMFLOAT3((-halfWidth + kx*patchWidth) - (-halfWidth + x*patchWidth), mHeightmap[kz*mInfo.HeightmapWidth + kx] - mHeightmap[z*mInfo.HeightmapWidth + x],(halfDepth - kz*patchDepth) - (halfDepth - x*patchDepth));
		XMVECTOR tang1 = XMLoadFloat3(&v1);

		XMFLOAT3 v2 = XMFLOAT3((-halfWidth + mx*patchWidth) - (-halfWidth + x*patchWidth), mHeightmap[mz*mInfo.HeightmapWidth + mx] - mHeightmap[z*mInfo.HeightmapWidth + x],(halfDepth - mz*patchDepth) - (halfDepth - x*patchDepth));
		XMVECTOR tang2 = XMLoadFloat3(&v2);

		XMVECTOR Norm = XMVector3Cross(tang1,tang2);
		XMFLOAT3 n = XMFLOAT3();

		XMStoreFloat3(&n,Norm);

		return n;
	}
	else
	{
		return XMFLOAT3(0,1,0);
	}
}

float Terrain::Average(float i, float j)
{
	float avg = 0.0f;
	float num = 0.0f;

	for (int m = i-1; m <= i+1; m++)
	{
		for (int n = j-1; n < j+1; n++)
		{
			if (InBounds(m, n))
			{
				avg += mHeightmap[m*mInfo.HeightmapWidth + n];
				num += 1.0f;
			}
		}
	}

	return avg/num;
}

bool Terrain::InBounds(int z, int x)
{
	return (z>=0&&z<(int)mInfo.HeightmapHeight&&
		x>=0&&x<(int)mInfo.HeightmapWidth);
}

HRESULT Terrain::BuildHeightmapShaderResourceView(ID3D11Device * device)
{
	HRESULT hr = S_OK;

	// Create shader resource view
    D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory( &texDesc, sizeof(texDesc) );
	texDesc.Width = mInfo.HeightmapWidth;
	texDesc.Height = mInfo.HeightmapHeight;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R32_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;
	


	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &mHeightmap[0];
	data.SysMemPitch = mInfo.HeightmapWidth*sizeof(float);
	data.SysMemSlicePitch = 0;

	ID3D11Texture2D* hmapTex = 0;
	
	hr = device->CreateTexture2D( &texDesc, &data, &hmapTex );
	if( FAILED( hr ) )
        return hr;

	// 
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory( &srvDesc, sizeof(srvDesc) );
	srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;

	hr = device->CreateShaderResourceView( hmapTex, &srvDesc, &mHeightMapSRV );
	
	return hr;
}

HRESULT Terrain::BuildNormalmapShaderResourceView(ID3D11Device * device)
{
	HRESULT hr = S_OK;

	// Create shader resource view
    D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory( &texDesc, sizeof(texDesc) );
	texDesc.Width = mInfo.HeightmapWidth;
	texDesc.Height = mInfo.HeightmapHeight;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;
	
	std::vector<XMFLOAT4> nmap(mNormalmap.size());
	//half the heightmap size.. XNA lib used in book, don't know what to use
	//std::transform(mHeightmap.begin(),mHeightmap.end(),hmap.begin(),NULL);

	for (int i = 0; i < mNormalmap.size(); i++)
	{
		nmap[i] = XMFLOAT4(mNormalmap[i].x,mNormalmap[i].y,mNormalmap[i].z, 0);
	}
	
	
	//nmap = mNormalmap;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &nmap[0];
	data.SysMemPitch = mInfo.HeightmapWidth*sizeof(XMFLOAT4);
	data.SysMemSlicePitch = 0;

	ID3D11Texture2D* nmapTex = 0;
	
	hr = device->CreateTexture2D( &texDesc, &data, &nmapTex );
	if( FAILED( hr ) )
        return hr;

	// 
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory( &srvDesc, sizeof(srvDesc) );
	srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;

	hr = device->CreateShaderResourceView( nmapTex, &srvDesc, &mNormalMapSRV );

	return hr;
}


float Terrain::GetWidth(void)const
{
	return (mInfo.HeightmapWidth - 1)*mInfo.CellSpacing; // last pixel is end node, so doesn't have a range to fill
}

float Terrain::GetDepth(void)const
{
	return (mInfo.HeightmapWidth - 1)*mInfo.CellSpacing;
}

HRESULT Terrain::BuildQuadPatchVertexBuffer(ID3D11Device *device, ID3D11DeviceContext* deviceContext)
{
	HRESULT hr = S_OK;

	std::vector<GraphicHelper::Terrain> patchVertices(mNumPatchVerticesRows*mNumPatchVerticesCols);

	float halfWidth = 0.5f*GetWidth();
	float halfDepth = 0.5f*GetDepth();

	//calculate the range of each patch
	float patchWidth = GetWidth() / (mNumPatchVerticesCols-1);
	float patchDepth = GetDepth() / (mNumPatchVerticesRows-1);

	//for calculating how much of texture each Patch should fill
	float du = 1.0f / (mNumPatchVerticesCols-1); // 1 / patch faces in column
	float dv = 1.0f / (mNumPatchVerticesRows-1); // 1 / patch faces in rows

	for(UINT i = 0; i < mNumPatchVerticesRows; ++i)
	{
		float z = halfDepth - i*patchDepth;

		for(UINT j = 0; j < mNumPatchVerticesCols; ++j)
		{
			float x = -halfWidth + j*patchWidth;

			patchVertices[i*mNumPatchVerticesCols+j].Pos = XMFLOAT3(x, 0.0f, z);
			// Stretch texture over grid.
			patchVertices[i*mNumPatchVerticesCols+j].Tex.x = j*du;
			patchVertices[i*mNumPatchVerticesCols+j].Tex.y = i*dv;
		}
	}

	// Store axis-aligned bounding box y-bounds in upper-left patch corner.
	for(UINT i = 0; i < mNumPatchVerticesRows-1; ++i)
	{
		for(UINT j = 0; j < mNumPatchVerticesCols-1; ++j)
		{
			UINT patchID = i*(mNumPatchVerticesCols-1)+j;
			patchVertices[i*mNumPatchVerticesCols+j].BoundsY = mPatchBoundsY[patchID];
		}
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(GraphicHelper::Terrain) * patchVertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &patchVertices[0];
	hr = (device->CreateBuffer(&vbd, &vinitData, &g_pQuadPatchVertexBuffer));

	UINT stride = sizeof( GraphicHelper::Terrain );
	UINT offset = 0;
	deviceContext->IASetVertexBuffers( 0, 1, &g_pQuadPatchVertexBuffer, &stride, &offset );


	return hr;
}

HRESULT Terrain::BuildQuadPatchIndexBuffer(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
	std::vector<USHORT> indices(mNumPatchQuadFaces*4); //4 indices a quad face

	int k = 0;
	for(UINT i = 0; i < mNumPatchVerticesRows-1; ++i)
	{
		for(UINT j = 0; j < mNumPatchVerticesCols-1; ++j)
		{
			// Top row of 2x2 quad patch
			indices[k] = i*mNumPatchVerticesCols+j;
			indices[k+1] = i*mNumPatchVerticesCols+j+1;
			// Bottom row of 2x2 quad patch
			indices[k+2] = (i+1)*mNumPatchVerticesCols+j;
			indices[k+3] = (i+1)*mNumPatchVerticesCols+j+1;
			k += 4; // next quad
		}
	}

	HRESULT hr = S_OK;

	D3D11_BUFFER_DESC bd;

	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.ByteWidth = sizeof(USHORT) *indices.size();        // 36 vertices needed for 12 triangles in a triangle list
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	bd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = &indices[0];
	hr = device->CreateBuffer( &bd, &InitData, &g_pQuadPatchIndexBuffer );

	deviceContext->IASetIndexBuffer(g_pQuadPatchIndexBuffer,DXGI_FORMAT_R16_UINT,0);

        
	return hr;
}

HRESULT Terrain::CreateHeightmapSamplerState(ID3D11Device* device)
{
	HRESULT hr = S_OK;
	D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory( &sampDesc, sizeof(sampDesc) );
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = device->CreateSamplerState( &sampDesc, &g_pQuadSamplerState);

	return hr;
}


HRESULT Terrain::CreateTextureSamplerState(ID3D11Device* device)
{
	HRESULT hr = S_OK;
	D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory( &sampDesc, sizeof(sampDesc) );
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = device->CreateSamplerState( &sampDesc, &g_pTextureSamplerState);

	return hr;
}



void Terrain::CalcAllPatchBoundsY(void)
{
	mPatchBoundsY.resize(mNumPatchQuadFaces);
	// For each patch
	for(UINT i = 0; i < mNumPatchVerticesRows-1; ++i)  //per quad face in row and col
	{
		for(UINT j = 0; j < mNumPatchVerticesCols-1; ++j)
		{
			CalcPatchBoundsY(i, j);
		}
	}
}

void Terrain::CalcPatchBoundsY(UINT i, UINT j)
{
	// Scan the heightmap values this patch covers and
	// compute the min/max height.
	UINT x0 = j*CellsPerPatch;
	UINT x1 = (j+1)*CellsPerPatch;
	UINT y0 = i*CellsPerPatch;
	UINT y1 = (i+1)*CellsPerPatch;

	float minY = +D3D11_FLOAT32_MAX;
	float maxY = -D3D11_FLOAT32_MAX;
	
	for(UINT y = y0; y <= y1; ++y)
	{
		for(UINT x = x0; x <= x1; ++x)
		{
			UINT k = y*mInfo.HeightmapWidth + x;
			minY = min(minY, mHeightmap[k]);
			maxY = max(maxY, mHeightmap[k]);
			
			
		}
	}

	UINT patchID = i*(mNumPatchVerticesCols-1)+j; //quad face still

	mPatchBoundsY[patchID] = XMFLOAT2(minY, maxY);
}





HRESULT Terrain::CompileVertexShaders(ID3D11Device* device,ID3D11DeviceContext* deviceContext)
{
	HRESULT hr = S_OK;
	
	ID3DBlob* pVSBlob = nullptr;
	hr = graphicHelper.CompileShaderFromFile( L"ShadersLast.fx", "VS", "vs_5_0", &pVSBlob );
	
    if( FAILED( hr ) )
    {
        MessageBox( nullptr,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

	// Create the vertex shader
	hr = device->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader );
	if( FAILED( hr ) )
	{	
		pVSBlob->Release();
        return hr;
	}


		// Define the input layout IMPORTANT SHIET THAT THIS IS THE RIGHT COMPARED TO VS_INPUT atm not fixed
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "WORLDPOS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BOUNDS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		
	};
	UINT numElements = ARRAYSIZE( layout );

     // Create the input layout
	hr = device->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
                                          pVSBlob->GetBufferSize(), &g_pVertexLayout );
	pVSBlob->Release();
	if( FAILED( hr ) )
        return hr;

    // Set the input layout
    deviceContext->IASetInputLayout( g_pVertexLayout );

	return hr;
}

HRESULT Terrain::CompilePixelShaders(ID3D11Device* device)
{
	HRESULT hr = S_OK;

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
    hr = graphicHelper.CompileShaderFromFile( L"ShadersLast.fx", "PS", "ps_5_0", &pPSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( nullptr,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

	// Create the pixel shader
	hr = device->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader );
	pPSBlob->Release();
    if( FAILED( hr ) )
        return hr;

	return hr;
}

HRESULT Terrain::CompileGeometryShaders(ID3D11Device* device)
{
	HRESULT hr = S_OK;

	// Compile the geometry shader
	ID3DBlob* pPSBlob = nullptr;
    hr = graphicHelper.CompileShaderFromFile( L"ShadersLast.fx", "GS", "gs_5_0", &pPSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( nullptr,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

	// Create the geometry shader
	hr = device->CreateGeometryShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pGeometryShader );
	pPSBlob->Release();
    if( FAILED( hr ) )
        return hr;

	return hr;
}

HRESULT Terrain::CompileHullShaders(ID3D11Device* device)
{
	HRESULT hr = S_OK;

	// Compile the hull shader
	ID3DBlob* pPSBlob = nullptr;
    hr = graphicHelper.CompileShaderFromFile( L"ShadersLast.fx", "HS", "hs_5_0", &pPSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( nullptr,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

	// Create the hull shader
	hr = device->CreateHullShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pHullShader );
	pPSBlob->Release();
    if( FAILED( hr ) )
        return hr;

	return hr;
}

HRESULT Terrain::CompileDomainShaders(ID3D11Device* device)
{
	HRESULT hr = S_OK;

	// Compile the domain shader
	ID3DBlob* pPSBlob = nullptr;
    hr = graphicHelper.CompileShaderFromFile( L"ShadersLast.fx", "DS", "ds_5_0", &pPSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( nullptr,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

	// Create the domain shader
	hr = device->CreateDomainShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pDomainShader );
	pPSBlob->Release();
    if( FAILED( hr ) )
        return hr;

	return hr;
}

void Terrain::Draw(ID3D11DeviceContext* deviceContext)
{
	//set input right
	UINT stride = sizeof( GraphicHelper::Terrain );
	UINT offset = 0;
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
	deviceContext->IASetInputLayout( g_pVertexLayout );
	//maybe change depth buffers
	deviceContext->IASetVertexBuffers( 0, 1, &g_pQuadPatchVertexBuffer, &stride, &offset );
	deviceContext->IASetIndexBuffer(g_pQuadPatchIndexBuffer,DXGI_FORMAT_R16_UINT,0);

	//draw
	//VS
	deviceContext->VSSetShader( g_pVertexShader, nullptr, 0 );
	deviceContext->VSSetSamplers(0, 1, &g_pQuadSamplerState);
	deviceContext->VSSetShaderResources( 0, 1, &mHeightMapSRV);
	deviceContext->VSSetConstantBuffers( 2, 1, &g_CBOnButtonPress );

	//HS
	deviceContext->HSSetShader( g_pHullShader, nullptr, 0 );
	deviceContext->HSSetConstantBuffers( 2, 1, &g_CBOnButtonPress );

	//DS
	deviceContext->DSSetShader( g_pDomainShader, nullptr, 0 );
	deviceContext->DSSetSamplers(0, 1, &g_pQuadSamplerState);
	deviceContext->DSSetShaderResources( 0, 1, &mHeightMapSRV);
	deviceContext->DSSetShaderResources( 1, 1, &mNormalMapSRV);
	deviceContext->DSSetConstantBuffers( 2, 1, &g_CBOnButtonPress );

	deviceContext->GSSetShader(nullptr, nullptr, 0 );

	//PS
	deviceContext->PSSetShader( g_pPixelShader, nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, &g_pQuadSamplerState);
	deviceContext->PSSetSamplers(1, 1, &g_pTextureSamplerState);
	deviceContext->PSSetShaderResources( 2, 1, &mBlendMapSRV);
	deviceContext->PSSetShaderResources( 3, 1, &mTexture1SRV);
	deviceContext->PSSetShaderResources( 4, 1, &mTexture2SRV);
	deviceContext->PSSetShaderResources( 5, 1, &mTexture3SRV);
	deviceContext->PSSetShaderResources( 6, 1, &mTexture4SRV);
	deviceContext->PSSetConstantBuffers( 2, 1, &g_CBOnButtonPress );

	deviceContext->DrawIndexed(mNumPatchQuadFaces*4 ,0,0);
}

void Terrain::Cleanup(ID3D11Device* device)
{
	if( g_pQuadPatchVertexBuffer ) g_pQuadPatchVertexBuffer->Release();
	if( g_pQuadPatchIndexBuffer ) g_pQuadPatchIndexBuffer->Release();
	//if( g_pVertexLayout ) g_pVertexLayout->Release();
	if( g_pQuadSamplerState ) g_pQuadSamplerState->Release();
	if ( mHeightMapSRV ) mHeightMapSRV->Release();
	if ( mNormalMapSRV ) mNormalMapSRV->Release();
	if ( mBlendMapSRV ) mBlendMapSRV->Release();
	if( mTexture1SRV ) mTexture1SRV->Release();
	if( mTexture2SRV ) mTexture1SRV->Release();
	if( mTexture3SRV ) mTexture1SRV->Release();
	if( mTexture4SRV ) mTexture1SRV->Release();
    
    if( g_pVertexShader ) g_pVertexShader->Release();
    if( g_pPixelShader ) g_pPixelShader->Release();
	if( g_pGeometryShader ) g_pGeometryShader->Release();
	if( g_pHullShader ) g_pHullShader->Release();
	if( g_pDomainShader ) g_pDomainShader->Release();
}

