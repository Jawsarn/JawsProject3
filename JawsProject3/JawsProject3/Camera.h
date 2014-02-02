#define _XM_NO_INTRINSICS_
#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
#include "DDSTextureLoader.h"
#include "Terrain.h"

using namespace DirectX;

#pragma once

class Camera
{
public: Camera(Terrain* t);
	~Camera();

	// Get/Set world camera position.
	XMVECTOR GetPositionXM()const;
	XMFLOAT3 GetPosition()const;
	//void SetPosition(float x, float y, float z);
	//void SetPosition(const XMFLOAT3& v);

	// Get camera basis vectors.
	//XMVECTOR GetRightXM()const;
	//XMFLOAT3 GetRight()const;
	//XMVECTOR GetUpXM()const;
	//XMFLOAT3 GetUp()const;
	//XMVECTOR GetLookXM()const;
	//XMFLOAT3 GetLook()const;

	// Get frustum properties.
	//float GetNearZ()const;
	//float GetFarZ()const;
	//float GetAspect()const;
	//float GetFovY()const;
	float GetFovX()const;

	// Get near and far plane dimensions in view space coordinates.
	float GetNearWindowWidth()const;
	float GetNearWindowHeight()const;
	float GetFarWindowWidth()const;
	float GetFarWindowHeight()const;

	// Set frustum.
	void SetLens(float fovY, float aspect, float zn, float zf);

	// Define camera space via LookAt parameters.
	void LookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR worldUp);
	void LookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up);

	// Get View/Proj matrices.
	XMMATRIX View()const;
	XMMATRIX Proj()const;
	XMMATRIX ViewProj()const;

	// Strafe/Walk the camera a distance d.
	void Strafe(float d);
	void Walk(float d);

	// Rotate the camera.
	void Pitch(float angle);
	void RotateY(float angle);

	// After modifying camera position/orientation, call
	// to rebuild the view matrix once per frame.
	void UpdateViewMatrix();

	XMFLOAT4 planes[6];
	void Camera::ExtractFrustumPlanes( CXMMATRIX M);
	XMFLOAT4 GetPlane(int i);

	private:
	// Camera coordinate system with coordinates relative to world space.
	XMFLOAT3 mPosition; // view space origin
	XMFLOAT3 mRight; // view space x-axis
	XMFLOAT3 mUp; // view space y-axis
	XMFLOAT3 mLook; // view space z-axis

	// Cache frustum properties.
	float mNearZ;
	float mFarZ;
	float mAspect;
	float mFovY;
	float mNearWindowHeight;
	float mFarWindowHeight;

	// Cache View/Proj matrices.
	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;
	Terrain* mTerrain;
};