#pragma once

class SkyRenderer : public BaseComponent
{
public:
	SkyRenderer(const std::wstring& assetFile);
	~SkyRenderer() override;

	SkyRenderer(const SkyRenderer& other) = delete;
	SkyRenderer(SkyRenderer&& other) noexcept = delete;
	SkyRenderer& operator=(const SkyRenderer& other) = delete;
	SkyRenderer& operator=(SkyRenderer&& other) noexcept = delete;

	void SetMaterial(BaseMaterial* pMaterial, UINT8 submeshId = 0);
	void SetMaterial(UINT materialId, UINT8 submeshId = 0);

protected:
	friend class GameScene;
	friend class DeferredRenderer;
	void Initialize(const SceneContext& sceneContext) override;
	void Update(const SceneContext&) override;
	void Draw(const SceneContext& sceneContext) override;
	void PostDraw(const SceneContext&) override {}

	void RenderSky(const SceneContext& sceneContext);

private:

	std::wstring m_AssetFile{};
	MeshFilter* m_pMeshFilter{};

	std::vector<BaseMaterial*> m_Materials{};
	BaseMaterial* m_pDefaultMaterial{};

	bool m_MaterialChanged{};
};

