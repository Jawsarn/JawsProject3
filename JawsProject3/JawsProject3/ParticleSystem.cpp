#include "ParticleSystem.h"
#include <iostream>

ParticleSystem::ParticleSystem()
{
	geometryUpdateShader = nullptr;
	vertexUpdateShader = nullptr;
	geometryDrawShader = nullptr;
	vertexDrawShader = nullptr;
	pixelDrawShader = nullptr;

	vertexUpdateLayout = nullptr;
	vertexDrawLayout = nullptr;
	outputVertexBuffer = nullptr;
	initVertexBuffer = nullptr;
	drawVertexBuffer = nullptr;
	particleBuffer = nullptr;
	fixedBuffer = nullptr;
}


ParticleSystem::~ParticleSystem(void)
{
}

HRESULT ParticleSystem::Initialize( ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
	HRESULT hr = S_OK;
	graphicHelper = GraphicHelper();
	graphicHelper.CreateDepthStencilSatates(device,deviceContext);
	graphicHelper.TurnOnDepth(deviceContext);



	hr = CreateBuffer(device, deviceContext);
	if( FAILED( hr ) )
        return hr;

	hr = CreateRandomTexture1DSRV(device);
	if( FAILED( hr ) )
        return hr;

	hr = CreateVertexInputBuffer(device);
	if( FAILED( hr ) )
        return hr;

	hr = CreateGeometryShaders(deviceContext,device);
	if( FAILED( hr ) )
        return hr;

	hr = CompileVertexShaders( device, deviceContext);
	if( FAILED( hr ) )
        return hr;

	hr = CompilePixelShaders(device);
	if( FAILED( hr ) )
        return hr;

	hr = CreateTextureSamplerState(device);
	if( FAILED( hr ) )
        return hr;

	Reset();

	return hr;
}
HRESULT ParticleSystem::CreateBuffer(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
	HRESULT hr = S_OK;
	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(CParticleBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
	hr = device->CreateBuffer( &bd, nullptr, &particleBuffer );
    if( FAILED( hr ) )
        return hr;


	bd.ByteWidth = sizeof(CBFixed);

	hr = device->CreateBuffer(&bd, nullptr, &fixedBuffer);
	if( FAILED( hr ) )
        return hr;

	CBFixed cbf;
	cbf.gAccelW = XMFLOAT3(0.0f, -9.8f, 0.0f);
	cbf.padding2 = 0;

	deviceContext->UpdateSubresource( fixedBuffer, 0, nullptr, &cbf, 0, 0);
}

HRESULT ParticleSystem::CreateTextureSamplerState(ID3D11Device* device)
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
	hr = device->CreateSamplerState( &sampDesc, &textureSamplerState);

	return hr;
}

HRESULT ParticleSystem::CreateRandomTexture1DSRV(ID3D11Device* device)
{
	HRESULT hr = S_OK;

	//
	// Create the random data.
	XMFLOAT4 randVal[1024];

	for(int i = 0; i < 1024; ++i)
	{
		randVal[i].x = MathHelper::RandF(-1.0f, 1.0f);
		randVal[i].y = MathHelper::RandF(-1.0f, 1.0f);
		randVal[i].z = MathHelper::RandF(-1.0f, 1.0f);
		randVal[i].w = MathHelper::RandF(-1.0f, 1.0f);
	}

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = randVal;
	initData.SysMemPitch = 1024*sizeof(XMFLOAT4);
	initData.SysMemSlicePitch = 0;
	
	// Create the texture.
	D3D11_TEXTURE1D_DESC texDesc;
	texDesc.Width = 1024;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;
	texDesc.ArraySize = 1;


	ID3D11Texture1D* randomTex = 0;
	hr = device->CreateTexture1D(&texDesc, &initData, &randomTex);
	if( FAILED(hr) )
		return hr;
	
	// Create the resource view.
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
	viewDesc.Texture1D.MipLevels = texDesc.MipLevels;
	viewDesc.Texture1D.MostDetailedMip = 0;

	hr = device->CreateShaderResourceView(randomTex, &viewDesc, &randomTexSRV);
	if( FAILED(hr) )
		return hr;

	return hr;
}

HRESULT ParticleSystem::CreateVertexInputBuffer(ID3D11Device* device)
{
	HRESULT hr = S_OK;

	GraphicHelper::Particle input[] =
	{
		{XMFLOAT3(700,0,0),XMFLOAT3(0,0,0),XMFLOAT2(10,10),0,0},
		{XMFLOAT3(700,0,0),XMFLOAT3(0,0,0),XMFLOAT2(10,10),0,0},
		{XMFLOAT3(-700,0,0),XMFLOAT3(0,0,0),XMFLOAT2(10,10),0,0},
		{XMFLOAT3(0,0,700),XMFLOAT3(0,0,0),XMFLOAT2(10,10),0,0},
		{XMFLOAT3(0,0,-700),XMFLOAT3(0,0,0),XMFLOAT2(10,10),0,0},
		{XMFLOAT3(700,0,700),XMFLOAT3(0,0,0),XMFLOAT2(10,10),0,0},
		{XMFLOAT3(700,0,-700),XMFLOAT3(0,0,0),XMFLOAT2(10,10),0,0},
		{XMFLOAT3(-700,0,700),XMFLOAT3(0,0,0),XMFLOAT2(10,10),0,0},
		{XMFLOAT3(-700,0,-700),XMFLOAT3(0,0,0),XMFLOAT2(10,10),0,0},
	};

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = sizeof(GraphicHelper::Particle) * 9;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vinitData;
	ZeroMemory(&vinitData,sizeof(vinitData));
	vinitData.pSysMem = &input[0];
	hr = (device->CreateBuffer(&vbd, &vinitData, &initVertexBuffer));

	return hr;
}

HRESULT ParticleSystem::CreateGeometryShaders(ID3D11DeviceContext* g_pImmidiateContext, ID3D11Device* device)
{
	HRESULT hr = S_OK;

	// Compile the geometry shader
	ID3DBlob* pPSBlob = nullptr;

	hr = graphicHelper.CompileShaderFromFile( L"ParticleShaders.fx", "GSDraw", "gs_5_0", &pPSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( nullptr,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

	// Create the geometry shader
	hr = device->CreateGeometryShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &geometryDrawShader );
	pPSBlob->Release();
    if( FAILED( hr ) )
        return hr;


	//Create the update Geometry shader for output
	hr = graphicHelper.CompileShaderFromFile( L"ParticleShaders.fx", "GSUpdate", "gs_5_0", &pPSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( nullptr,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }



	D3D11_SO_DECLARATION_ENTRY pDecl[] =
	{
		// semantic name, semantic index, start component, component count, output slot
		{ 0 ,"POSITION", 0, 0, 3, 0 },   // output all components of position
		{ 0 ,"VELOCITY", 0, 0, 3, 0 },     // output the first 3 of the normal
		{ 0 ,"SIZE",	 0, 0, 2, 0 },
		{ 0 ,"AGE",		 0, 0, 1, 0 },
		{ 0 ,"TYPE",	 0, 0, 1, 0 },// output the first 2 texture coordinates
	};

	UINT stride = 10 * sizeof(float);
	UINT hmm  = sizeof(pDecl);
	UINT elms = sizeof(pDecl) / sizeof(D3D11_SO_DECLARATION_ENTRY);

	hr = device->CreateGeometryShaderWithStreamOutput( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), pDecl, 
		ARRAYSIZE(pDecl), NULL, 0, D3D11_SO_NO_RASTERIZED_STREAM, NULL, &geometryUpdateShader );

	pPSBlob->Release();
    if( FAILED( hr ) )
        return hr;

	

	//g_pImmidiateContext->OMSetDepthStencilState(,0);

	//http://msdn.microsoft.com/en-us/library/windows/desktop/bb205122(v=vs.85).aspx#Create_a_Geometry_Shader_Object_with_Stream_Output

	return hr;
}

HRESULT ParticleSystem::CompileVertexShaders(ID3D11Device* device,ID3D11DeviceContext* deviceContext)
{
	HRESULT hr = S_OK;
	
	//CREATE SHADER 1
	ID3DBlob* pVSBlob = nullptr;
    hr = graphicHelper.CompileShaderFromFile( L"ParticleShaders.fx", "VSUpdate", "vs_5_0", &pVSBlob );
	
    if( FAILED( hr ) )
    {
        MessageBox( nullptr,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

	// Create the vertex shader
	hr = device->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &vertexUpdateShader );
	if( FAILED( hr ) )
	{	
		pVSBlob->Release();
        return hr;
	}



		// Define the input layout IMPORTANT SHIET THAT THIS IS THE RIGHT COMPARED TO VS_INPUT atm not fixed
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "VELOCITY", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "AGE", 0, DXGI_FORMAT_R32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TYPE", 0, DXGI_FORMAT_R32_UINT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		
	};
	UINT numElements = ARRAYSIZE( layout );

     // Create the input layout
	hr = device->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
                                          pVSBlob->GetBufferSize(), &vertexUpdateLayout );

	pVSBlob->Release();
	if( FAILED( hr ) )
        return hr;

	//CREATE SHADER 2
	hr = graphicHelper.CompileShaderFromFile( L"ParticleShaders.fx", "VSDraw", "vs_5_0", &pVSBlob );
	
    if( FAILED( hr ) )
    {
        MessageBox( nullptr,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

	// Create the vertex shader
	hr = device->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &vertexDrawShader );
	if( FAILED( hr ) )
	{	
		pVSBlob->Release();
        return hr;
	}
	////DONE----------


	hr = device->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
                                          pVSBlob->GetBufferSize(), &vertexDrawLayout );


    // Set the input layout
    deviceContext->IASetInputLayout( vertexUpdateLayout );


	
	UINT MAX_VERTICES =  10000000;
	D3D11_BUFFER_DESC bdsc;
	bdsc.ByteWidth = sizeof(GraphicHelper::Particle)*MAX_VERTICES;
	bdsc.Usage = D3D11_USAGE_DEFAULT;
	bdsc.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;
	bdsc.CPUAccessFlags = 0;
	bdsc.MiscFlags = 0;
	bdsc.StructureByteStride = 0;

	device->CreateBuffer( &bdsc, NULL, &outputVertexBuffer );
	device->CreateBuffer( &bdsc, NULL, &drawVertexBuffer );

	//set buffer to stage the offset for where to write data, 1 for number of buffers
	UINT offset = 0;
	deviceContext->SOSetTargets( 1, &outputVertexBuffer, &offset );

	return hr;
}

HRESULT ParticleSystem::CompilePixelShaders(ID3D11Device* device)
{
	HRESULT hr = S_OK;

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
    hr = graphicHelper.CompileShaderFromFile( L"ParticleShaders.fx", "PSDraw", "ps_5_0", &pPSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( nullptr,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

	// Create the pixel shader
	hr = device->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &pixelDrawShader );
	pPSBlob->Release();
    if( FAILED( hr ) )
        return hr;

	return hr;
}

void ParticleSystem::Reset()
{
	mFirstRun = true;
}

void ParticleSystem::Draw(ID3D11DeviceContext* deviceContext, ID3D11Device* device, float dt, float gt)
{
	
	//make fix pointers 554 in book
	CParticleBuffer cpb;
	cpb.gEmitPosW = XMFLOAT3(0,100,0);
	cpb.gGameTime = gt;
	cpb.gEmitDirW = XMFLOAT3(0,0,0);
	cpb.gTimeStep = dt;
	
	deviceContext->UpdateSubresource( particleBuffer, 0, nullptr, &cpb, 0, 0 );

	deviceContext->VSSetShader( vertexUpdateShader, nullptr, 0 );
	deviceContext->VSSetConstantBuffers( 2, 1, &particleBuffer );

	deviceContext->GSSetShader( geometryUpdateShader, nullptr, 0);
	deviceContext->GSSetConstantBuffers( 2, 1, &particleBuffer );
	deviceContext->GSSetShaderResources( 0, 1, &randomTexSRV);
	deviceContext->GSSetSamplers( 0, 1, &textureSamplerState);

	deviceContext->HSSetShader( nullptr, nullptr, 0);

	deviceContext->DSSetShader(nullptr,nullptr,0);

	deviceContext->PSSetShader( nullptr, nullptr, 0);

	graphicHelper.TurnOffDepth(deviceContext);

	deviceContext->IASetInputLayout( vertexUpdateLayout );
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	UINT stride = sizeof(GraphicHelper::Particle);
	UINT offset = 0;

	if(mFirstRun)
	{
		deviceContext->IASetVertexBuffers(0,1,&initVertexBuffer,&stride,&offset);
	}
	else
	{
		deviceContext->IASetVertexBuffers(0,1,&drawVertexBuffer,&stride,&offset);
	}

	//draw using stream out

	deviceContext->SOSetTargets(1, &outputVertexBuffer, &offset);
	

 	if(mFirstRun)
	{
		deviceContext->Draw(9,0);
		mFirstRun = false;
	}
	else
	{
		deviceContext->DrawAuto();
	}

	//done streaming out
	ID3D11Buffer* bufferArray[1] = {0};
	deviceContext->SOSetTargets(1,bufferArray,&offset);

	std::swap( drawVertexBuffer,outputVertexBuffer);

	//test
	deviceContext->IASetInputLayout( vertexDrawLayout );
	deviceContext->IASetVertexBuffers(0,1,&drawVertexBuffer, &stride, &offset);

	//Draw the updated particles
	graphicHelper.TurnOnDepth(deviceContext);
	deviceContext->PSSetShader( pixelDrawShader, nullptr, 0);
	deviceContext->VSSetShader( vertexDrawShader, nullptr, 0 );
	deviceContext->VSSetConstantBuffers( 3, 1, &fixedBuffer );
	deviceContext->GSSetShader( geometryDrawShader, nullptr, 0);
	

	deviceContext->GSSetConstantBuffers( 3, 1, &fixedBuffer );


	deviceContext->DrawAuto();
}

void ParticleSystem::Cleanup(ID3D11Device* device)
{
	if( geometryUpdateShader ) geometryUpdateShader->Release();
	if( vertexUpdateShader ) vertexUpdateShader->Release();
	if( geometryDrawShader )geometryDrawShader->Release();
	if( vertexDrawShader )vertexDrawShader->Release();
	if( pixelDrawShader )pixelDrawShader->Release();

	if( vertexUpdateLayout )vertexUpdateLayout->Release();
	if( vertexDrawLayout )vertexDrawLayout->Release();

	if( outputVertexBuffer )outputVertexBuffer->Release();
	if( initVertexBuffer )initVertexBuffer->Release();
	if( drawVertexBuffer )drawVertexBuffer->Release();

	if( particleBuffer )particleBuffer->Release();
	if( fixedBuffer )fixedBuffer->Release();

	if( randomTexSRV )randomTexSRV->Release();
	if( textureSamplerState )textureSamplerState->Release();
	graphicHelper.Cleanup(device);
}