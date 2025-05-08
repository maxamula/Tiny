#pragma once
#include <d3d12.h>
#include <DirectXMath.h>

namespace tiny
{
	class Camera
	{
	public:
		Camera();
		Camera(DirectX::XMVECTOR position, DirectX::XMVECTOR target, f32 fov, f32 aspectRatio);
		DirectX::XMMATRIX GetViewMatrix() const;
		DirectX::XMMATRIX GetProjectionMatrix() const;
		D3D12_VIEWPORT GetViewport() const;
		D3D12_RECT GetScissorRect() const;

		const DirectX::XMVECTOR& GetPosition() const { return mPosition; }
		DirectX::XMVECTOR& GetPosition() { return mPosition; }
		const DirectX::XMVECTOR& GetTarget() const { return mTarget; }
		DirectX::XMVECTOR& GetTarget() { return mTarget; }

		void SetFov(f32 fov) { mFov = fov; }
		void SetAspectRatio(f32 aspectRatio) { mAspectRatio = aspectRatio; }
		void SetNearZ(f32 nearZ) { mNearZ = nearZ; }
		void SetFarZ(f32 farZ) { mFarZ = farZ; }
		void SetViewport(const D3D12_VIEWPORT& viewport) { mViewport = viewport; }
		void SetScissorRect(const D3D12_RECT& scissorRect) { mScissorRect = scissorRect; }
	private:
		DirectX::XMVECTOR mPosition;
		DirectX::XMVECTOR mTarget;
		f32 mFov;
		f32 mAspectRatio;
		f32 mNearZ;
		f32 mFarZ;
		D3D12_VIEWPORT mViewport;
		D3D12_RECT mScissorRect;
	};
}