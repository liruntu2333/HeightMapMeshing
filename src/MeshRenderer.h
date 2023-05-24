#pragma once

#include <directxtk/BufferHelpers.h>
#include <directxtk/VertexTypes.h>

#include "Renderer.h"

using Vertex = DirectX::VertexPosition;

class MeshRenderer : public Renderer
{
public:
	MeshRenderer(ID3D11Device* device, const std::shared_ptr<PassConstants>& constants);
	~MeshRenderer() override = default;

	MeshRenderer(const MeshRenderer&) = delete;
	MeshRenderer& operator=(const MeshRenderer&) = delete;
	MeshRenderer(MeshRenderer&&) = delete;
	MeshRenderer& operator=(MeshRenderer&&) = delete;

	void Initialize(ID3D11DeviceContext* context) override;
	void Render(ID3D11DeviceContext* context) override;

	void SetVerticesAndIndices(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

protected:

	void UpdateBuffer(ID3D11DeviceContext* context) override;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_Vs = nullptr;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_Ps = nullptr;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_InputLayout = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_VertexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_IndexBuffer = nullptr;
	DirectX::ConstantBuffer<PassConstants> m_Cb0 {};

	std::shared_ptr<PassConstants> m_Constants = nullptr;
	uint32_t m_VertexCount = 0;
	uint32_t m_IndexCount = 0;
	bool m_Loaded = false;
};
