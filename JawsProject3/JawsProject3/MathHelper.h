#include <windows.h>
#include <d3d11_1.h>
#include <DirectXMath.h>

using namespace DirectX;

#pragma once

class MathHelper
{
public:
	MathHelper(void);
	~MathHelper(void);
	static float RandF();
	static float RandF(float a, float b);

	XMVECTOR RandUnitVec3();
};

