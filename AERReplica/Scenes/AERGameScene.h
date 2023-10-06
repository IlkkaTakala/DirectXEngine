#pragma once
class Character;
class Pickable;
class AERGameScene : public GameScene
{
public:
	AERGameScene() :GameScene(L"AERGameScene") {}
	~AERGameScene() override;
	AERGameScene(const AERGameScene& other) = delete;
	AERGameScene(AERGameScene&& other) noexcept = delete;
	AERGameScene& operator=(const AERGameScene& other) = delete;
	AERGameScene& operator=(AERGameScene&& other) noexcept = delete;

	void RemoveShard(Pickable*);

protected:
	void Initialize() override;
	void OnGUI() override;
	void Update() override;
	void PostDraw() override;
	void PostInitialize() override;
	void OnSceneActivated() override;
	void OnSceneDeactivated() override;
	void LoadIslandMesh();

	GameObject* m_pIslands{};
	ParticleEmitter* m_pEmitter[4]{ nullptr };
	bool m_FlashLightMode{ false };

	float m_ShadowMapScale{0.13f};

	enum InputIds
	{
		CharacterMoveLeft,
		CharacterMoveRight,
		CharacterMoveForward,
		CharacterMoveBackward,
		CharacterJump
	};

	Character* m_pCharacter{};

	FMOD::Sound* birdAmbient;
	FMOD::Sound* windAmbient;
	FMOD::Channel* birdChannel;
	FMOD::Channel* windChannel;

	std::vector<BaseMaterial*> TimedMats;

	std::vector<Pickable*> Shards;
};

