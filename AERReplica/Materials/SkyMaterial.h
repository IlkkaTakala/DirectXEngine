#pragma once
class SkyMaterial : public Material<SkyMaterial>
{
public:
	SkyMaterial();
	~SkyMaterial() override = default;

	SkyMaterial(const SkyMaterial& other) = delete;
	SkyMaterial(SkyMaterial&& other) noexcept = delete;
	SkyMaterial& operator=(const SkyMaterial& other) = delete;
	SkyMaterial& operator=(SkyMaterial&& other) noexcept = delete;

protected:
	void InitializeEffectVariables() override;
	void OnUpdateModelVariables(const SceneContext& /*sceneContext*/, const ModelComponent* /*pModel*/) const;
};

