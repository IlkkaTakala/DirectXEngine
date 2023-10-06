#pragma once
class Character;
class MainMenu : public GameScene
{
public:
	MainMenu() :GameScene(L"MainMenu") {}
	~MainMenu() override;
	MainMenu(const MainMenu& other) = delete;
	MainMenu(MainMenu&& other) noexcept = delete;
	MainMenu& operator=(const MainMenu& other) = delete;
	MainMenu& operator=(MainMenu&& other) noexcept = delete;

protected:
	void Initialize() override;
	void OnGUI() override;
	void Update() override;
	void OnSceneActivated() override;

	void LoadBackground();

	BaseMaterial* Sky{};
	SpriteFont* m_pFont{};
	FMOD::Sound* Music;
	FixedCamera* Camera;
	std::vector<BaseMaterial*> TimedMats;
	TextureData* logoTex;

	GameObject* m_pButtons[2]{};
	GameObject* m_pIslands{};
};

