#include "stdafx.h"
#include "ModelComponent.h"

ModelComponent::ModelComponent(const std::wstring& assetFile, bool castShadows):
	m_AssetFile(assetFile),
	m_CastShadows(castShadows)
{
}

ModelComponent::~ModelComponent()
{
	SafeDelete(m_pAnimator);
	SafeRelease(ShapeBuffer);
	SafeRelease(Shapes);

	m_pDefaultMaterial = nullptr;
	m_Materials.clear();
}

UINT ModelComponent::AddBlendShape(const std::wstring& asset)
{
	auto device = GetScene()->GetSceneContext().d3dContext.pDevice;

	auto mesh = ContentManager::Load<MeshFilter>(asset);
	auto m = mesh->GetMeshes()[0];

	D3D11_BUFFER_DESC bufferDescMesh = { 
		m.vertexCount * sizeof(XMFLOAT3),
		D3D11_USAGE_IMMUTABLE,
		D3D11_BIND_SHADER_RESOURCE,
		0,0};
	D3D11_SUBRESOURCE_DATA data; 
	data.SysMemPitch = 0; 
	data.SysMemSlicePitch = 0; 
	data.pSysMem = m.positions.data(); 
	HANDLE_ERROR(device->CreateBuffer(&bufferDescMesh, &data, &ShapeBuffer));


	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc; 
	ZeroMemory(&SRVDesc, sizeof(SRVDesc)); 
	SRVDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT; 
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER; 
	SRVDesc.Buffer.ElementOffset = 0; 
	SRVDesc.Buffer.ElementWidth = m.vertexCount * (sizeof(XMFLOAT3) / (3 * sizeof(float))); 
	HANDLE_ERROR(device->CreateShaderResourceView(ShapeBuffer, &SRVDesc, &Shapes));
	return 0;
}

void ModelComponent::Initialize(const SceneContext& sceneContext)
{
	m_pMeshFilter = ContentManager::Load<MeshFilter>(m_AssetFile);
	m_pMeshFilter->BuildIndexBuffer(sceneContext);

	//Resize Materials Array (if needed)
	if(m_Materials.size() < m_pMeshFilter->GetMeshCount())
	{
		m_Materials.resize(m_pMeshFilter->GetMeshCount(), nullptr);
	}

	if (m_pMeshFilter->m_HasAnimations)
		m_pAnimator = new ModelAnimator(m_pMeshFilter);

	if (m_MaterialChanged)
	{
		for(auto& subMesh: m_pMeshFilter->GetMeshes())
		{
			const auto pMaterial = m_Materials[subMesh.id]!=nullptr?m_Materials[subMesh.id]:m_pDefaultMaterial;
			m_pMeshFilter->BuildVertexBuffer(sceneContext, pMaterial, subMesh.id);
		}
		
		m_MaterialChanged = false;
	}

	if(m_CastShadows) //Only if we cast a shadow of course..
	{
		ShadowMapRenderer::Get()->UpdateMeshFilter(sceneContext, m_pMeshFilter);

		m_enableShadowMapDraw = true;

	}
}

void ModelComponent::Update(const SceneContext& sceneContext)
{
	if (!m_IsActive) return;
	if (m_pAnimator)
		m_pAnimator->Update(sceneContext);
}

void ModelComponent::Draw(const SceneContext& sceneContext)
{
	if (!m_IsActive) return;
	if (!m_pDefaultMaterial)
	{
		Logger::LogWarning(L"ModelComponent::Draw() > No Default Material Set!");
		return;
	}

	//Update Materials
	BaseMaterial* pCurrMaterial = nullptr;
	for (const auto& subMesh : m_pMeshFilter->GetMeshes())
	{
		//Gather Material
		pCurrMaterial = m_Materials[subMesh.id] != nullptr ? m_Materials[subMesh.id] : m_pDefaultMaterial;
		pCurrMaterial->UpdateEffectVariables(sceneContext, this);

		const auto pDeviceContext = sceneContext.d3dContext.pDeviceContext;

		//Set Inputlayout
		pDeviceContext->IASetInputLayout(pCurrMaterial->GetTechniqueContext().pInputLayout);

		//Set Vertex Buffer
		const UINT offset = 0;
		const auto& vertexBufferData = m_pMeshFilter->GetVertexBufferData(sceneContext, pCurrMaterial, subMesh.id);
		pDeviceContext->IASetVertexBuffers(0, 1, &vertexBufferData.pVertexBuffer, &vertexBufferData.VertexStride,
			&offset);

		//Set Index Buffer
		pDeviceContext->IASetIndexBuffer(subMesh.buffers.pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		//Set Primitive Topology
		pDeviceContext->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//DRAW
		auto tech = pCurrMaterial->GetTechniqueContext().pTechnique;
		D3DX11_TECHNIQUE_DESC techDesc{};

		tech->GetDesc(&techDesc);
		for (UINT p = 0; p < techDesc.Passes; ++p)
		{
			tech->GetPassByIndex(p)->Apply(0, pDeviceContext);
			pDeviceContext->DrawIndexed(subMesh.indexCount, 0, 0);
		}
	}
}

void ModelComponent::PostDraw(const SceneContext& /*sceneContext*/)
{
}

void ModelComponent::ShadowMapDraw(const SceneContext& sceneContext)
{
	if (!m_IsActive) return;
	if (!m_CastShadows)return;

	ShadowMapRenderer::Get()->DrawMesh(sceneContext, m_pMeshFilter, GetTransform()->GetWorld(), (m_pAnimator == nullptr ? std::vector<XMFLOAT4X4>{} : m_pAnimator->GetBoneTransforms()));
	//1. Call ShadowMapRenderer::DrawMesh with the required function arguments BUT boneTransforms are only required for skinned meshes of course..
}

void ModelComponent::SetMaterial(BaseMaterial* pMaterial, UINT8 submeshId)
{
	//Resize Materials Array (if needed)
	if(m_Materials.size() <= submeshId)
	{
		m_Materials.resize(submeshId + 1, nullptr);
	}

	if (pMaterial == nullptr)
	{
		m_Materials[submeshId] = nullptr;
		return;
	}

	if (!pMaterial->HasValidMaterialId())
	{
		Logger::LogWarning(L"BaseMaterial does not have a valid BaseMaterial Id. Make sure to add the material to the material manager first.");
		return;
	}

	if(m_pDefaultMaterial == nullptr)
	{
		m_pDefaultMaterial = pMaterial;
	}

	m_Materials[submeshId] = pMaterial;
	m_MaterialChanged = true;

	if (m_IsInitialized && GetScene())
	{
		ASSERT_IF(m_pMeshFilter->GetMeshCount() <= submeshId, L"Invalid SubMeshID({}) for current MeshFilter({} submeshes)", submeshId, m_pMeshFilter->GetMeshCount())
		m_pMeshFilter->BuildVertexBuffer(GetScene()->GetSceneContext(), pMaterial, submeshId);
		m_MaterialChanged = false;
	}
}

void ModelComponent::SetMaterial(UINT materialId, UINT8 submeshId)
{
	const auto pMaterial = MaterialManager::Get()->GetMaterial(materialId);
	SetMaterial(pMaterial, submeshId);
}
