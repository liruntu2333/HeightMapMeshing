#pragma once

#include <directxtk/SimpleMath.h>
#include <d3d11.h>
#include "imgui.h"

class Camera
{
public:
	[[nodiscard]] DirectX::SimpleMath::Matrix GetViewProjectionMatrix() const;
	void SetViewPort(ID3D11DeviceContext* context) const;
	void Update(const ImGuiIO& io);

private:
	DirectX::SimpleMath::Vector3 m_Position { 0.0f, 50.0f, -50.0f };
	DirectX::SimpleMath::Vector3 m_Rotation { 0.0f, DirectX::XM_PIDIV2 + 0.1f, 0.0f }; // row pitch yaw
	DirectX::SimpleMath::Vector3 m_Forward;
	DirectX::SimpleMath::Vector3 m_Right;
	D3D11_VIEWPORT m_Viewport {};
	float m_Fov = DirectX::XM_PIDIV4;
	float m_AspectRatio = 4.0f / 3.0f;
	float m_NearPlane = 1.0f;
	float m_FarPlane = 10000.0f;
};
