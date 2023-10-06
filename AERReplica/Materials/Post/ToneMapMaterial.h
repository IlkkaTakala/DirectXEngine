#pragma once
//Resharper Disable All

class ToneMapMaterial : public PostProcessingMaterial
{
public:
	ToneMapMaterial();
	~ToneMapMaterial() override = default;
	ToneMapMaterial(const ToneMapMaterial& other) = delete;
	ToneMapMaterial(ToneMapMaterial&& other) noexcept = delete;
	ToneMapMaterial& operator=(const ToneMapMaterial& other) = delete;
	ToneMapMaterial& operator=(ToneMapMaterial&& other) noexcept = delete;

protected:
	void Initialize(const GameContext& /*gameContext*/) override {}

};

