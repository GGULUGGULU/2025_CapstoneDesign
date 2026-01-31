#include "stdafx.h"
#include "MeshEffect.h"
#include "DDSTextureLoader12.h"

CMeshEffect::CMeshEffect(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
    m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_xmf3Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_xmf3Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);

    XMStoreFloat4x4(&m_xmf4x4World, XMMatrixIdentity());
}

CMeshEffect::~CMeshEffect()
{
    if (m_pVertexBuffer) m_pVertexBuffer->Release();
    if (m_pIndexBuffer) m_pIndexBuffer->Release();

    if (m_pTexture) m_pTexture->Release();
    if (m_pTextureUploadBuffer) m_pTextureUploadBuffer->Release();
    if (m_pSrvHeap) m_pSrvHeap->Release();
}

void CMeshEffect::CreateMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, float fRadius, int nSlices, int nStacks)
{
    std::vector<EFFECT_VERTEX> vVertices;
    std::vector<UINT> vIndices;

    float phiStep = XM_PI / nStacks;
    float thetaStep = 2.0f * XM_PI / nSlices;

    for (int i = 0; i <= nStacks; ++i)
    {
        float phi = i * phiStep;

        for (int j = 0; j <= nSlices; ++j)
        {
            float theta = j * thetaStep;

            EFFECT_VERTEX v;

            v.position.x = fRadius * sinf(phi) * cosf(theta);
            v.position.y = fRadius * cosf(phi);
            v.position.z = fRadius * sinf(phi) * sinf(theta);

            XMVECTOR n = XMLoadFloat3(&v.position);
            XMStoreFloat3(&v.normal, XMVector3Normalize(n));

            v.uv.x = (float)j / nSlices;
            v.uv.y = (float)i / nStacks;

            vVertices.push_back(v);
        }
    }

    for (int i = 0; i < nStacks; ++i)
    {
        for (int j = 0; j < nSlices; ++j)
        {
            vIndices.push_back(i * (nSlices + 1) + j);
            vIndices.push_back((i + 1) * (nSlices + 1) + j);
            vIndices.push_back(i * (nSlices + 1) + (j + 1));

            vIndices.push_back(i * (nSlices + 1) + (j + 1));
            vIndices.push_back((i + 1) * (nSlices + 1) + j);
            vIndices.push_back((i + 1) * (nSlices + 1) + (j + 1));
        }
    }
    m_nIndices = (UINT)vIndices.size();

    UINT nVertexStride = sizeof(EFFECT_VERTEX);
    UINT nVertexBufferBytes = nVertexStride * (UINT)vVertices.size();

    D3D12_HEAP_PROPERTIES heapPropUpload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC resDescVertex = CD3DX12_RESOURCE_DESC::Buffer(nVertexBufferBytes);

    pd3dDevice->CreateCommittedResource(
        &heapPropUpload,
        D3D12_HEAP_FLAG_NONE,
        &resDescVertex,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_pVertexBuffer)
    );

    void* pVertexDataBegin = nullptr;
    CD3DX12_RANGE readRange(0, 0);
    m_pVertexBuffer->Map(0, &readRange, &pVertexDataBegin);
    memcpy(pVertexDataBegin, vVertices.data(), nVertexBufferBytes);
    m_pVertexBuffer->Unmap(0, nullptr);

    m_d3dVertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
    m_d3dVertexBufferView.StrideInBytes = nVertexStride;
    m_d3dVertexBufferView.SizeInBytes = nVertexBufferBytes;

    UINT nIndexBufferBytes = sizeof(UINT) * (UINT)vIndices.size();
    D3D12_RESOURCE_DESC resDescIndex = CD3DX12_RESOURCE_DESC::Buffer(nIndexBufferBytes);

    pd3dDevice->CreateCommittedResource(
        &heapPropUpload,
        D3D12_HEAP_FLAG_NONE,
        &resDescIndex,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_pIndexBuffer)
    );

    void* pIndexDataBegin = nullptr;
    m_pIndexBuffer->Map(0, &readRange, &pIndexDataBegin);
    memcpy(pIndexDataBegin, vIndices.data(), nIndexBufferBytes);
    m_pIndexBuffer->Unmap(0, nullptr);

    m_d3dIndexBufferView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
    m_d3dIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    m_d3dIndexBufferView.SizeInBytes = nIndexBufferBytes;
}

void CMeshEffect::CreateTexture(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const wchar_t* pszFileName)
{
    std::unique_ptr<uint8_t[]> ddsData;
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;

    HRESULT hr = DirectX::LoadDDSTextureFromFile(pd3dDevice, pszFileName, &m_pTexture, ddsData, subresources);

    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_pTexture, 0, static_cast<UINT>(subresources.size()));

    auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

    pd3dDevice->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_pTextureUploadBuffer));

    UpdateSubresources(pd3dCommandList, m_pTexture, m_pTextureUploadBuffer, 0, 0, static_cast<UINT>(subresources.size()), subresources.data());

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    pd3dCommandList->ResourceBarrier(1, &barrier);

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 1;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    pd3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_pSrvHeap));

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = m_pTexture->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = -1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    pd3dDevice->CreateShaderResourceView(m_pTexture, &srvDesc, m_pSrvHeap->GetCPUDescriptorHandleForHeapStart());
}


void CMeshEffect::CreateProceduralTexture(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
    const UINT width = 256;
    const UINT height = 256;
    const UINT pixelSize = 4;
    const UINT rowPitch = width * pixelSize;
    const UINT textureSize = rowPitch * height;

    std::vector<UINT8> pixels(textureSize);
    for (UINT y = 0; y < height; ++y)
    {
        for (UINT x = 0; x < width; ++x)
        {
            UINT i = (y * width + x) * pixelSize;

            uint8_t noise = (rand() % 100 > 90) ? 255 : 0; 

            pixels[i + 0] = 200; 
            pixels[i + 1] = 255; 
            pixels[i + 2] = 255; 
            pixels[i + 3] = noise; 
        }
    }

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    auto heapPropDefault = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    pd3dDevice->CreateCommittedResource(
        &heapPropDefault,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_pTexture)
    );

    UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_pTexture, 0, 1);

    auto heapPropUpload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

    pd3dDevice->CreateCommittedResource(
        &heapPropUpload,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_pTextureUploadBuffer)
    );

    D3D12_SUBRESOURCE_DATA subresourceData = {};
    subresourceData.pData = pixels.data();
    subresourceData.RowPitch = rowPitch;
    subresourceData.SlicePitch = textureSize;

    UpdateSubresources(pd3dCommandList, m_pTexture, m_pTextureUploadBuffer, 0, 0, 1, &subresourceData);

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    pd3dCommandList->ResourceBarrier(1, &barrier);

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 1;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    pd3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_pSrvHeap));

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    pd3dDevice->CreateShaderResourceView(m_pTexture, &srvDesc, m_pSrvHeap->GetCPUDescriptorHandleForHeapStart());
}

void CMeshEffect::Update(float fTimeElapsed)
{
    if (!m_bActive) return;

    m_fCurrentTime += fTimeElapsed;

    XMMATRIX mScale = XMMatrixScaling(m_xmf3Scale.x, m_xmf3Scale.y, m_xmf3Scale.z);

    XMMATRIX mRot = XMMatrixRotationRollPitchYaw(
        XMConvertToRadians(m_xmf3Rotation.x),
        XMConvertToRadians(m_xmf3Rotation.y),
        XMConvertToRadians(m_xmf3Rotation.z)
    );


    XMMATRIX mTrans = XMMatrixTranslation(m_xmf3Position.x, m_xmf3Position.y, m_xmf3Position.z);

    XMMATRIX mWorld = mScale * mRot * mTrans;
    XMStoreFloat4x4(&m_xmf4x4World, mWorld);
}

void CMeshEffect::Render(ID3D12GraphicsCommandList* pd3dCommandList)
{
    if (!m_bActive) return;

    CB_EFFECT_DATA cbData;
    XMStoreFloat4x4(&cbData.m_xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4World)));
    cbData.m_fTime = m_fCurrentTime;
    cbData.m_fScrollSpeed = XMFLOAT3(0.0f, 5.0f, 0.0f); 

    pd3dCommandList->SetGraphicsRoot32BitConstants(2, sizeof(CB_EFFECT_DATA) / 4, &cbData, 0);

    if (m_pSrvHeap)
    {
        ID3D12DescriptorHeap* ppHeaps[] = { m_pSrvHeap };
        pd3dCommandList->SetDescriptorHeaps(1, ppHeaps);
        pd3dCommandList->SetGraphicsRootDescriptorTable(0, m_pSrvHeap->GetGPUDescriptorHandleForHeapStart());
    }

    pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pd3dCommandList->IASetVertexBuffers(0, 1, &m_d3dVertexBufferView);
    pd3dCommandList->IASetIndexBuffer(&m_d3dIndexBufferView);

    pd3dCommandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
}