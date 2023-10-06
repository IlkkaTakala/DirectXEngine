#include "stdafx.h"
#include "AERGameScene.h"

#include "Materials/BasicMaterial.h"
#include "Materials/BasicMaterial_Deferred.h"
#include "Materials/BasicMaterial_Deferred_Skinned.h"
#include "Materials/CloudMaterial.h"
#include "Materials/WaterMaterial.h"
#include "Materials/SkyMaterial.h"
#include "Materials/Post/PostFog.h"
#include "Materials/Post/ToneMapMaterial.h"


#include "Prefabs/Animal.h"
#include "Prefabs/Character.h"
#include "Prefabs/Pickable.h"

AERGameScene::~AERGameScene()
{
	windAmbient->release();
	birdAmbient->release();
}

void AERGameScene::RemoveShard(Pickable* p)
{
	std::erase(Shards, p);
}

void AERGameScene::Initialize()
{
	//Settings
	//********
	m_SceneContext.useDeferredRendering = true;

	m_SceneContext.settings.drawGrid = false;
	m_SceneContext.settings.drawPhysXDebug = false;
	m_SceneContext.settings.enableOnGUI = true;
	m_SceneContext.settings.showInfoOverlay = false;

	m_pIslands = AddChild(new GameObject);

	LoadIslandMesh();

	auto Fmod = SoundManager::Get()->GetSystem();

	FMOD_RESULT res = Fmod->createStream("Resources/Sounds/chirps.Wav", FMOD_DEFAULT | FMOD_LOOP_NORMAL, nullptr, &birdAmbient);
	SoundManager::Get()->ErrorCheck(res);
	res = Fmod->createStream("Resources/Sounds/abm_wind.Wav", FMOD_DEFAULT | FMOD_LOOP_NORMAL, nullptr, &windAmbient);
	SoundManager::Get()->ErrorCheck(res);

	Fmod->playSound(birdAmbient, nullptr, true, &birdChannel);
	birdChannel->setVolume(0.5f);
	Fmod->playSound(windAmbient, nullptr, true, &windChannel);
	windChannel->setVolume(0.3f);

	//Directional
	auto& dirLight = m_SceneContext.pLights->GetDirectionalLight();
	dirLight.isEnabled = true;
	//dirLight.direction = { 0, 0, -1, 0.0f };
	dirLight.direction = { -0.577f, -0.577f, 0.577f , 0.0f };

	const auto pDefaultMaterial = PxGetPhysics().createMaterial(0.5f, 0.5f, 0.5f);

	//Character
	CharacterDesc characterDesc{ pDefaultMaterial };
	characterDesc.controller.height = 1.f;
	characterDesc.controller.radius = 0.5f;

	m_pCharacter = AddChild(new Character(characterDesc));

	const auto skyMaterial = MaterialManager::Get()->CreateMaterial<SkyMaterial>();

	const auto skyObject = AddChild(new GameObject);
	const auto sky = skyObject->AddComponent(new SkyRenderer(L"Meshes/SkySphere.ovm"));
	sky->SetMaterial(skyMaterial);
	sky->GetTransform()->Scale(5.f);

	m_SceneContext.pSky = sky;

	auto fog = MaterialManager::Get()->CreateMaterial<PostFog>();
	AddPostProcessingEffect(fog);

	auto tone = MaterialManager::Get()->CreateMaterial<ToneMapMaterial>();
	AddPostProcessingEffect(tone);


	const auto updator = [](ParticleEmitter* system, float delta) {
		ParticleSystemConstruction::Updator u(system);
		u.UpdateLifetime(delta);
		u.UpdateVelocities(delta);
		u.Alpha(delta, CurveData({ {0.f, 0.f}, {0.1f, 1.f}, { 1.f, 0.f } }));
		//u.Color(delta, VectorCurveData({ {0.f, {1.f}} }));
		u.SpriteSize(delta, CurveData({ {0.f, 0.3f}, {1.f, 1.0f}}));
	};

	const auto constructor = [](ParticleEmitter* /*sys*/, Particle& p) {
		p.initialSize = MathHelper::randF(10, 15);
		p.max_lifetime = MathHelper::randF(2, 3);
		p.isActive = false;
		p.lifetime = 0.f;
		ParticleSystemConstruction::Constructor::SphereLocation(p,
			MathHelper::randF(4.f, 10.f)
		);
		XMStoreFloat3(&p.velocity, XMVector3Normalize(XMLoadFloat3(&p.vertexInfo.Position)) * MathHelper::randF(.5f, 1.5f));
		p.alpha = 0.f;
		p.initialColour = { 2.f, 2.f, 2.5f };
	};

	static std::vector<XMFLOAT3> Waterfall {
		{ 55.4f, -31.1f, 138.f},
		{ 49.4f, -31.1f, 134.5f},
		{ 35.0f, -31.1f, 126.6f},
		{ 12.5f, -31.1f, 109.1f},
		{ 19.8f, -31.1f, 115.5f},
		{ -4.5f, -31.1f, 89.6f},
		{-11.1f, -31.1f, 80.8f},
		{-18.2f, -31.1f, 71.8f},
	};

	for (auto& loc : Waterfall) {
		auto pParticle = AddChild(new GameObject)->AddComponent(new ParticleSystemComponent());
		pParticle->AddEmitter({ L"Textures/BaseParticle.png", constructor, updator }, [pParticle] { return ParticleSystemConstruction::MakeSpawnRate(25.f); });
		pParticle->GetTransform()->Translate(loc);
	}

	static std::vector<XMFLOAT3> Llams {
		{ -53.5f, -27.f, -12.f},
		{ -47.4f, -26.6f, -13.1f},
		{ -55.7f, -27.7f, -16.2f},
		{ 22.2f, -28.5f, 27.7f},
		{ 28.8f, -27.7f, 28.2f},
		{ 34.1f, -27.47f, 24.6f },
	};

	for (auto& loc : Llams) {
		auto lam = AddChild(new Animal());
		lam->GetTransform()->Translate(loc);
		lam->GetTransform()->Rotate(0, MathHelper::randF(0.f, 360.f), 0);
	}
}

void AERGameScene::Update()
{
	XMFLOAT4 pos{};
	pos.x = sin(m_SceneContext.pGameTime->GetTotal()) * 30.0f;
	pos.y = 15.0f;
	pos.z = cos(m_SceneContext.pGameTime->GetTotal()) * 30.0f;
	pos.w = 1.0f;
	auto rotator = XMQuaternionRotationAxis({ .5, 0, -.5 }, 0.4f * m_SceneContext.pGameTime->GetElapsed());
	auto& dir = m_SceneContext.pLights->GetDirectionalLight().direction;
	XMStoreFloat4(&dir, XMVector3Rotate(XMLoadFloat4(&dir), rotator));

	for (auto& m : TimedMats) {
		m->SetVariable_Scalar(L"gTime", m_SceneContext.pGameTime->GetTotal());
	}
}

void AERGameScene::OnGUI()
{
	m_pCharacter->DrawImGui();
}

void AERGameScene::LoadIslandMesh() 
{
	//Mesh 
	const auto meshPath = L"Meshes/Land.ovm";
	const auto leavesPath = L"Meshes/Leaves.ovm";
	const auto cloudPath = L"Meshes/Clouds.ovm";
	const auto pModel = new ModelComponent(meshPath);
	const auto pModelClouds = new ModelComponent(cloudPath);
	const auto pModelLeaves = new ModelComponent(leavesPath);
	const auto pModelFoliage1 = new ModelComponent(L"Meshes/Foliage.ovm");
	const auto pModelFoliage2 = new ModelComponent(L"Meshes/Foliage2.ovm");
	const auto pModelWater = new ModelComponent(L"Meshes/Water.ovm");
	const auto pModelAltars = new ModelComponent(L"Meshes/Altars.ovm");

	//BASIC-EFFECT_DEFERRED
	//*********************
	const auto pMaterial = MaterialManager::Get()->CreateMaterial<BasicMaterial_Deferred>();
	const auto pMaterialFoliage = MaterialManager::Get()->CreateMaterial<BasicMaterial_Deferred>();
	const auto pMaterialAltar = MaterialManager::Get()->CreateMaterial<BasicMaterial_Deferred>();
	const auto pMaterialCloud = MaterialManager::Get()->CreateMaterial<CloudMaterial>();
	const auto pMaterialWater = MaterialManager::Get()->CreateMaterial<WaterMaterial>();
	const auto pMaterialRipple = MaterialManager::Get()->CreateMaterial<RippleMaterial>();
	const auto pMaterialRipple1 = MaterialManager::Get()->CreateMaterial<RippleMaterial>();

	TimedMats.push_back(pMaterialWater);
	TimedMats.push_back(pMaterialRipple);
	TimedMats.push_back(pMaterialRipple1);

	pModel->SetMaterial(pMaterial);
	pModelLeaves->SetMaterial(pMaterial);
	pModelClouds->SetMaterial(pMaterialCloud);
	pModelFoliage1->SetMaterial(pMaterialFoliage);
	pModelFoliage2->SetMaterial(pMaterialFoliage);
	pModelWater->SetMaterial(pMaterialWater);
	pModelWater->SetMaterial(pMaterialRipple, 1);
	pModelWater->SetMaterial(pMaterialRipple1, 2);
	pModelAltars->SetMaterial(pMaterialAltar);
	pMaterial->UseTransparency(false);

	//Diffuse
	std::wstring texPath{};
	pMaterial->SetDiffuseMap(L"Textures/Islands/wol.png");
	pMaterialFoliage->SetDiffuseMap(L"Textures/Islands/foliage.png");
	pMaterialAltar->SetDiffuseMap(L"Textures/Islands/world02.png");
	pMaterialRipple->SetVariable_Texture(L"gRippleTexture", ContentManager::Load<TextureData>(L"Textures/Islands/circularWater.png"));
	pMaterialRipple1->SetVariable_Texture(L"gRippleTexture", ContentManager::Load<TextureData>(L"Textures/Islands/squareWater.png"));

	//Append to Root Object
	m_pIslands->AddComponent(pModel);
	m_pIslands->AddComponent(pModelLeaves);
	m_pIslands->AddComponent(pModelClouds);
	m_pIslands->AddComponent(pModelFoliage1);
	m_pIslands->AddComponent(pModelFoliage2);
	m_pIslands->AddComponent(pModelAltars);
	m_pIslands->AddComponent(pModelWater);
	pModelClouds->SetEnableShadows(false);
	pModelWater->SetEnableShadows(false);

	auto convexMesh = ContentManager::Load<PxTriangleMesh>(L"Meshes/Land.ovpt");
	auto convexMesh2 = ContentManager::Load<PxTriangleMesh>(L"Meshes/Altars.ovpt");
	auto physMat = PxGetPhysics().createMaterial(0.5, 0.5, 1.0);
	auto Rigid = m_pIslands->AddComponent(new RigidBodyComponent(true));
	auto Rigid2 = m_pIslands->AddComponent(new RigidBodyComponent(true));
	Rigid->AddCollider(PxTriangleMeshGeometry{ convexMesh }, *physMat);
	Rigid2->AddCollider(PxTriangleMeshGeometry{ convexMesh2 }, *physMat);
}

void AERGameScene::PostDraw()
{
	//ShadowMapRenderer::Get()->Debug_DrawDepthSRV({ m_SceneContext.windowWidth - 10.f, 10.f }, { m_ShadowMapScale, m_ShadowMapScale }, { 1.f,0.f });
}

void AERGameScene::PostInitialize()
{
	/*for (int i = 0; i < 4; ++i) {
		m_pEmitter[i]->Warmup(6.f);
	}*/
}

void AERGameScene::OnSceneActivated()
{
	static std::vector<XMFLOAT3> ShardLocs = {
		{-2.4f, -25.5f, 5.f},
		{-66.f, -28.f, 33.f},
		{-94.f, 35.5f, 3.f},
		{18.6f, 34.4f, 170.5f},
		{9.7f, -28.4f, 177.f},
		{185.f, 37.f, 17.f},
		{-157.f, 134.4f, 227.f},
		{100.f, -29.5f, 95.f},
	};
	for (auto& l : ShardLocs) {
		auto pick = AddChild(new Pickable());
		pick->GetTransform()->Translate(l);
		Shards.push_back(pick);
	}
	InputManager::ForceMouseToCenter(true);
	InputManager::CursorVisible(false);

	birdChannel->setPaused(false);
	windChannel->setPaused(false);

	m_pCharacter->ClearScore(int(ShardLocs.size() * 0.8));
	m_pCharacter->GetTransform()->Translate(0.f, -24.f, 0.f);
	m_pCharacter->ShowControls();
}

void AERGameScene::OnSceneDeactivated()
{
	m_pCharacter->ClearScore(1000);
	m_pCharacter->GetTransform()->Translate(0.f, -24.f, 0.f);
	InputManager::ForceMouseToCenter(false);
	InputManager::CursorVisible(true);
	for (auto& s : Shards)
		s->Destroy();
	Shards.clear();

	birdChannel->setPaused(true);
	windChannel->setPaused(true);

	m_pCharacter->Restart();
}
