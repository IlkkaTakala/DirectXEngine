#include "stdafx.h"
#include "MainMenu.h"
#include <imgui.h>

#include "Materials/BasicMaterial.h"
#include "Materials/BasicMaterial_Deferred.h"
#include "Materials/BasicMaterial_Deferred_Skinned.h"
#include "Materials/CloudMaterial.h"
#include "Materials/WaterMaterial.h"
#include "Materials/SkyMaterial.h"
#include "Materials/Post/PostFog.h"
#include "Materials/Post/PostBloom.h"
#include "Materials/Post/ToneMapMaterial.h"

#include "Prefabs/Character.h"

#include "AERGameScene.h"

MainMenu::~MainMenu()
{
	Music->release();
}

void MainMenu::Initialize()
{
	//Settings
	//********
	m_SceneContext.useDeferredRendering = true;

	m_SceneContext.settings.drawGrid = false;
	m_SceneContext.settings.drawPhysXDebug = false;
	m_SceneContext.settings.enableOnGUI = false;
	m_SceneContext.settings.showInfoOverlay = false;

	m_pFont = ContentManager::Load<SpriteFont>(L"SpriteFonts/Goudy_Bold.fnt");

	m_pIslands = AddChild(new GameObject);
	LoadBackground();

	auto Fmod = SoundManager::Get()->GetSystem();

	FMOD_RESULT res = Fmod->createStream("Resources/Sounds/main.wav", FMOD_DEFAULT | FMOD_LOOP_NORMAL, nullptr, &Music);
	SoundManager::Get()->ErrorCheck(res);
	FMOD::Channel* c;
	Fmod->playSound(Music, nullptr, false, &c);
	c->setVolume(0.8f);

	Camera = AddChild(new FixedCamera);
	SetActiveCamera(Camera->GetComponent<CameraComponent>());

	//Directional
	auto& dirLight = m_SceneContext.pLights->GetDirectionalLight();
	dirLight.isEnabled = true;
	dirLight.direction = { -0.577f, -0.577f, 0.577f , 1.0f };
	
	Sky = MaterialManager::Get()->CreateMaterial<SkyMaterial>();
	const auto skyObject = Camera->AddChild(new GameObject);
	const auto sky = skyObject->AddComponent(new SkyRenderer(L"Meshes/SkySphere.ovm"));
	sky->SetMaterial(Sky);
	m_SceneContext.pSky = sky;

	auto logo = new GameObject();
	logo->AddComponent(new SpriteComponent(L"Textures/Logo.png", { 0.5f,0.f }, { 1.f,1.f,1.f,.9f }));
	AddChild(logo);
	logo->GetTransform()->Translate(m_SceneContext.windowWidth / 2.f, 10, .0f);

	auto fog = MaterialManager::Get()->CreateMaterial<PostFog>();
	AddPostProcessingEffect(fog);

	auto tone = MaterialManager::Get()->CreateMaterial<ToneMapMaterial>();
	AddPostProcessingEffect(tone);

	logoTex = ContentManager::Load<TextureData>(L"Textures/DAE_logo.png");
}

void MainMenu::Update()
{
	auto loc = XMVector3Transform(XMVECTOR{ 183, 25, 36 } * 1.5f, 
		XMMatrixRotationAxis({ 0,1,0 }, m_SceneContext.pGameTime->GetTotal() * 0.01f));
	Camera->GetTransform()->Translate(loc);
	auto rotator = XMQuaternionRotationMatrix(XMMatrixLookToLH(loc, XMVector3Normalize(loc), { 0,1,0 }));
	Camera->GetTransform()->Rotate(rotator);

	for (auto& m : TimedMats) {
		m->SetVariable_Scalar(L"gTime", m_SceneContext.pGameTime->GetTotal());
	}
}

void MainMenu::OnSceneActivated()
{
	InputManager::CursorVisible(true);
}

void MainMenu::LoadBackground()
{
	//Mesh 
	const auto meshPath = L"Meshes/Land.ovm";
	const auto leavesPath = L"Meshes/Leaves.ovm";
	const auto cloudPath = L"Meshes/Clouds.ovm";
	const auto pModel = new ModelComponent(meshPath);
	const auto pModelClouds = new ModelComponent(cloudPath);
	const auto pModelLeaves = new ModelComponent(leavesPath);
	const auto pModelWater = new ModelComponent(L"Meshes/Water.ovm");
	const auto pModelAltars = new ModelComponent(L"Meshes/Altars.ovm");

	//BASIC-EFFECT_DEFERRED
	//*********************
	const auto pMaterial = MaterialManager::Get()->CreateMaterial<BasicMaterial_Deferred>();
	const auto pMaterialAltar = MaterialManager::Get()->CreateMaterial<BasicMaterial_Deferred>();
	const auto pMaterialCloud = MaterialManager::Get()->CreateMaterial<CloudMaterial>();
	const auto pMaterialWater = MaterialManager::Get()->CreateMaterial<WaterMaterial>();
	pModel->SetMaterial(pMaterial);
	pModelLeaves->SetMaterial(pMaterial);
	pModelClouds->SetMaterial(pMaterialCloud);
	pModelWater->SetMaterial(pMaterialWater);
	pModelAltars->SetMaterial(pMaterialAltar);
	pMaterial->UseTransparency(false);

	TimedMats.push_back(pMaterialWater);
	//Diffuse
	std::wstring texPath{};
	pMaterial->SetDiffuseMap(L"Textures/Islands/world.png");
	pMaterialAltar->SetDiffuseMap(L"Textures/Islands/world02.png");

	//Append to Root Object
	m_pIslands->AddComponent(pModel);
	m_pIslands->AddComponent(pModelLeaves);
	m_pIslands->AddComponent(pModelClouds);
	m_pIslands->AddComponent(pModelAltars);
	m_pIslands->AddComponent(pModelWater);
	pModelClouds->SetEnableShadows(false);
	pModelWater->SetEnableShadows(false);
}

void MainMenu::OnGUI()
{
	static ImGuiWindowFlags flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;
	ImGui::SetNextWindowPos({ m_SceneContext.windowWidth * 0.5f, m_SceneContext.windowHeight * 0.5f }, ImGuiCond_Always, { 0.5f, 0.f });
	ImGui::Begin("Menu", nullptr, flags);
	ImGui::PushFont(GetFont("menu"));
	if (ImGui::Button("Start", { 250, 65 })) {
		SceneManager::Get()->AddGameScene(new AERGameScene);
		SceneManager::Get()->SetActiveGameScene(L"AERGameScene");
	}
	ImGui::Dummy({0.f, 20.f});
	if (ImGui::Button("Exit", { 250, 65 })) {
		PostQuitMessage(0);
	}
	ImGui::PopFont();
	ImGui::End();

	static ImGuiWindowFlags flags2 = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoNavInputs;
	ImGui::SetNextWindowPos({ m_SceneContext.windowWidth * 0.5f, m_SceneContext.windowHeight }, ImGuiCond_Always, { 0.5f, 1.f });
	ImGui::SetNextWindowSize({ m_SceneContext.windowWidth, 60.f });
	ImGui::Begin("banner", nullptr, flags2);
	ImGui::PushFont(GetFont("menu"));

	ImGui::Text("Graphics Programming 2 / 2023 / Ilkka Takala - 2DAE10");
	ImGui::PopFont();
	ImGui::SameLine();
	ImGui::SetCursorPosX(m_SceneContext.windowWidth - 160.f);
	ImGui::Image(logoTex->GetShaderResourceView(), {150.f, 50.f});

	ImGui::End();
}