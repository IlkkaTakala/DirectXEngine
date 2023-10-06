#pragma once
class WaterMaterial : public Material<WaterMaterial>
{
public:
	WaterMaterial();
	~WaterMaterial() override = default;

	WaterMaterial(const WaterMaterial& other) = delete;
	WaterMaterial(WaterMaterial&& other) noexcept = delete;
	WaterMaterial& operator=(const WaterMaterial& other) = delete;
	WaterMaterial& operator=(WaterMaterial&& other) noexcept = delete;

protected:
	void InitializeEffectVariables() override;
	void OnUpdateModelVariables(const SceneContext& /*sceneContext*/, const ModelComponent* /*pModel*/) const;
};
class RippleMaterial : public Material<RippleMaterial>
{
public:
	RippleMaterial();
	~RippleMaterial() override = default;

	RippleMaterial(const RippleMaterial& other) = delete;
	RippleMaterial(RippleMaterial&& other) noexcept = delete;
	RippleMaterial& operator=(const RippleMaterial& other) = delete;
	RippleMaterial& operator=(RippleMaterial&& other) noexcept = delete;

protected:
	void InitializeEffectVariables() override;
	void OnUpdateModelVariables(const SceneContext& /*sceneContext*/, const ModelComponent* /*pModel*/) const;
};


