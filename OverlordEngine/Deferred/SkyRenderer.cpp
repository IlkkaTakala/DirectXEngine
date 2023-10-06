#include "stdafx.h"
#include "SkyRenderer.h"

SkyRenderer::SkyRenderer(const std::wstring& assetFile) :
	m_AssetFile(assetFile)
{
}

SkyRenderer::~SkyRenderer()
{
	m_pDefaultMaterial = nullptr;
	m_Materials.clear();
}

void SkyRenderer::SetMaterial(BaseMaterial* pMaterial, UINT8 submeshId)
{
	//Resize Materials Array (if needed)
	if (m_Materials.size() <= submeshId)
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

	if (m_pDefaultMaterial == nullptr)
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

void SkyRenderer::SetMaterial(UINT materialId, UINT8 submeshId)
{
	const auto pMaterial = MaterialManager::Get()->GetMaterial(materialId);
	SetMaterial(pMaterial, submeshId);
}

void SkyRenderer::Initialize(const SceneContext& sceneContext)
{
	m_pMeshFilter = ContentManager::Load<MeshFilter>(m_AssetFile);
	m_pMeshFilter->BuildIndexBuffer(sceneContext);

	//Resize Materials Array (if needed)
	if (m_Materials.size() < m_pMeshFilter->GetMeshCount())
	{
		m_Materials.resize(m_pMeshFilter->GetMeshCount(), nullptr);
	}

	if (m_MaterialChanged)
	{
		for (auto& subMesh : m_pMeshFilter->GetMeshes())
		{
			const auto pMaterial = m_Materials[subMesh.id] != nullptr ? m_Materials[subMesh.id] : m_pDefaultMaterial;
			m_pMeshFilter->BuildVertexBuffer(sceneContext, pMaterial, subMesh.id);
		}

		m_MaterialChanged = false;
	}
}

void SkyRenderer::Update(const SceneContext&)
{
}

void SkyRenderer::Draw(const SceneContext&)
{
	
}

void SkyRenderer::RenderSky(const SceneContext& sceneContext)
{
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
		auto world = XMLoadFloat4x4(&GetTransform()->GetWorld());
		auto view = XMLoadFloat4x4(&sceneContext.pCamera->GetView());
		const auto projection = XMLoadFloat4x4(&sceneContext.pCamera->GetProjection());
		auto wvp = world * view * projection;
		pCurrMaterial->SetVariable_Matrix(L"gWorld", reinterpret_cast<float*>(&world));
		pCurrMaterial->SetVariable_Matrix(L"gWorldViewProj", reinterpret_cast<float*>(&wvp));
		pCurrMaterial->SetVariable_Vector(L"gSunDirection", sceneContext.pLights->GetDirectionalLight().direction);

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
