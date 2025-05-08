#include "camera.h"

namespace tiny
{
	Camera::Camera()
		: mPosition(0.0f, 0.0f, -5.0f), mTarget(0.0f, 0.0f, 0.0f), mFov(DirectX::XM_PI / 4), mAspectRatio(16.0f / 9.0f), mNearZ(0.1f), mFarZ(100.0f),
		mViewport({}), mScissorRect({})
	{}

	Camera::Camera(DirectX::XMVECTOR position, DirectX::XMVECTOR target, f32 fov, f32 aspectRatio)
			: mPosition(position), mTarget(target), mFov(fov), mAspectRatio(aspectRatio)
	{
		mViewport = { 0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f };
		mScissorRect = { 0, 0, 1280, 720 };
	}

	DirectX::XMMATRIX Camera::GetViewMatrix() const
	{
		return DirectX::XMMatrixLookAtLH(mPosition, mTarget, DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	}

	DirectX::XMMATRIX Camera::GetProjectionMatrix() const
	{
		return DirectX::XMMatrixPerspectiveFovLH(mFov, mAspectRatio, 0.1f, 100.0f);
	}

	D3D12_VIEWPORT Camera::GetViewport() const
	{
		return mViewport;
	}

	D3D12_RECT Camera::GetScissorRect() const
	{
		return mScissorRect;
	}
}