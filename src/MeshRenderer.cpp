#include "MeshRenderer.h"
#include <d3dcompiler.h>
#include "D3DHelper.h"

using namespace DirectX;

MeshRenderer::MeshRenderer(ID3D11Device* device, const std::shared_ptr<PassConstants>& constants) :
    Renderer(device), m_Cb0(device), m_Constants(constants) {}

void MeshRenderer::Initialize(ID3D11DeviceContext* context)
{
    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    ThrowIfFailed(D3DReadFileToBlob(L"./shader/MeshVS.cso", &blob));

    ThrowIfFailed(
        m_Device->CreateVertexShader(
            blob->GetBufferPointer(),
            blob->GetBufferSize(),
            nullptr,
            &m_Vs)
        );

    ThrowIfFailed(
        m_Device->CreateInputLayout(
            Vertex::InputElements,
            Vertex::InputElementCount,
            blob->GetBufferPointer(),
            blob->GetBufferSize(),
            &m_InputLayout)
        );

    blob->Release();
    ThrowIfFailed(D3DReadFileToBlob(L"./shader/MeshPS.cso", &blob));
    ThrowIfFailed(
        m_Device->CreatePixelShader(
            blob->GetBufferPointer(),
            blob->GetBufferSize(),
            nullptr,
            &m_Ps)
        );
}

void MeshRenderer::Render(ID3D11DeviceContext* context)
{
    if (!m_Loaded) return;

    UpdateBuffer(context);


    constexpr UINT stride = sizeof(Vertex);
    constexpr UINT offset = 0;
    context->IASetInputLayout(m_InputLayout.Get());
    const auto vb = m_VertexBuffer.Get();
    context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    context->IASetIndexBuffer(m_IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    const auto cb0 = m_Cb0.GetBuffer();
    context->VSSetConstantBuffers(0, 1, &cb0);
    context->VSSetShader(m_Vs.Get(), nullptr, 0);
    context->RSSetState(s_CommonStates->Wireframe());
    context->PSSetShader(m_Ps.Get(), nullptr, 0);
    context->OMSetBlendState(s_CommonStates->Opaque(), nullptr, 0xffffffff);
    context->OMSetDepthStencilState(s_CommonStates->DepthDefault(), 0);
    context->DrawIndexed(m_IndexCount, 0, 0);
}

void MeshRenderer::UpdateBuffer(ID3D11DeviceContext* context)
{
    m_Cb0.SetData(context, *m_Constants);
}

void MeshRenderer::SetVerticesAndIndices(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
    m_VertexCount = static_cast<uint32_t>(vertices.size());
    ThrowIfFailed(DirectX::CreateStaticBuffer<Vertex>(
        m_Device,
        vertices.data(),
        vertices.size(),
        D3D11_BIND_VERTEX_BUFFER,
        m_VertexBuffer.ReleaseAndGetAddressOf()));

    m_IndexCount = static_cast<uint32_t>(indices.size());
    ThrowIfFailed(DirectX::CreateStaticBuffer<uint32_t>(
        m_Device,
        indices.data(),
        indices.size(),
        D3D11_BIND_INDEX_BUFFER,
        m_IndexBuffer.ReleaseAndGetAddressOf()));

    m_Loaded = true;
}
