#pragma once
#include "BaseComponent.h"

class MeshFilter;
class ModelAnimator;

class ModelComponent : public BaseComponent
{
public:
	ModelComponent(const std::wstring&  assetFile, bool castShadows = true);
	~ModelComponent() override;

	ModelComponent(const ModelComponent& other) = delete;
	ModelComponent(ModelComponent&& other) noexcept = delete;
	ModelComponent& operator=(const ModelComponent& other) = delete;
	ModelComponent& operator=(ModelComponent&& other) noexcept = delete;

	void SetMaterial(BaseMaterial* pMaterial, UINT8 submeshId = 0);
	void SetMaterial(UINT materialId, UINT8 submeshId = 0);
	BaseMaterial* GetMaterial(UINT8 submeshId = 0) const { return m_Materials[submeshId]; }

	ModelAnimator* GetAnimator() const { return m_pAnimator; }
	bool HasAnimator() const { return m_pAnimator != nullptr; }

	UINT AddBlendShape(const std::wstring& asset);
	ID3D11ShaderResourceView* GetShapes() const { return Shapes; }

protected:
	void Initialize(const SceneContext& sceneContext) override;
	void Update(const SceneContext&) override;
	void Draw(const SceneContext& sceneContext) override;
	void PostDraw(const SceneContext& sceneContext) override;

	void ShadowMapDraw(const SceneContext& sceneContext) override; //update_W9

private:

	std::wstring m_AssetFile{};
	MeshFilter* m_pMeshFilter{};
	ID3D11ShaderResourceView* Shapes{};
	ID3D11Buffer* ShapeBuffer{};

	std::vector<BaseMaterial*> m_Materials{};
	BaseMaterial* m_pDefaultMaterial{};

	bool m_MaterialChanged{};

	ModelAnimator* m_pAnimator{};

	//W9
	bool m_CastShadows{ true };
};
