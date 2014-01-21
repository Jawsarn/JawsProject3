#include "MathHelper.h"


MathHelper::MathHelper(void)
{
}


MathHelper::~MathHelper(void)
{
}

// Returns random float in [0, 1).
float MathHelper::RandF()
{
	return (float)(rand()) / (float)RAND_MAX;
}
// Returns random float in [a, b).
float MathHelper::RandF(float a, float b)
{
	return a + RandF()*(b-a);
}
XMVECTOR MathHelper::RandUnitVec3()
{
	XMVECTOR One = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	//XMVECTOR Zero = XMVectorZero();

	// Keep trying until we get a point on/in the hemisphere.
	while(true)
	{
		// Generate random point in the cube [-1,1]^3.
		XMVECTOR v = XMVectorSet(
		MathHelper::RandF(-1.0f, 1.0f),
		MathHelper::RandF(-1.0f, 1.0f),
		MathHelper::RandF(-1.0f, 1.0f), 0.0f);
		// Ignore points outside the unit sphere in order to
		// get an even distribution over the unit sphere. Otherwise
		// points will clump more on the sphere near the corners
		// of the cube.
		if(XMVector3Greater(XMVector3LengthSq(v), One))
			continue;

		return XMVector3Normalize(v);
	}
}