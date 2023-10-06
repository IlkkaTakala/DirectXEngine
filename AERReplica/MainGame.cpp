#include "stdafx.h"
#include "MainGame.h"

#include <imgui.h>

#include "Scenes/MainMenu.h"

//Game is preparing
void MainGame::OnGamePreparing(GameContext& gameContext)
{
	//Here you can change some game settings before engine initialize
	//gameContext.windowWidth=... (default is 1280)
	//gameContext.windowHeight=... (default is 720)

	gameContext.windowWidth = 1920;
	gameContext.windowHeight = 1080;
	gameContext.fullscreen = true;

	gameContext.windowTitle = L"GP2 - Exam Project (2023) | (2DAE10) Takala Ilkka";
}

void MainGame::Initialize()
{
	ImGui::GetIO().IniFilename = NULL;

	ImGuiStyle* style = &ImGui::GetStyle();
	style->Colors[ImGuiCol_Button] = ImVec4(0.5f, 0.5f, 0.5f, 0.9f);

	auto menu = ImGui::GetIO().Fonts->AddFontFromFileTTF("Resources/SpriteFonts/goudosb.ttf", 36.0f);
	auto title = ImGui::GetIO().Fonts->AddFontFromFileTTF("Resources/SpriteFonts/goudos.ttf", 100.0f);
	AddFont("menu", menu);
	AddFont("title", title);

	hCursor = LoadCursorFromFileW(L"Resources/Textures/Cursor.cur");
	SetCursor(hCursor);

	InputManager::MakeInputMapping("Default",
		{
			{"Jump", {
				{
					Key(' ', DeviceType::Keyboard, InputMappingType::Button, TriggerType::Pressed),
					Key(XINPUT_GAMEPAD_A, DeviceType::Controller, InputMappingType::Button, TriggerType::Pressed),
				}, false
				}
			},
			{"Sprint", {
				{
					Key(VK_SHIFT, DeviceType::Keyboard, InputMappingType::Button, TriggerType::Down),
					Key(XINPUT_GAMEPAD_B, DeviceType::Controller, InputMappingType::Button, TriggerType::Down),
				}, false
				}
			},
			{"Pause", {
				{
					Key(VK_TAB, DeviceType::Keyboard, InputMappingType::Button, TriggerType::Released),
					Key(XINPUT_GAMEPAD_BACK, DeviceType::Controller, InputMappingType::Button, TriggerType::Released),
				}, false
				}
			},
			{"Move",{
				{
					Key('W', DeviceType::Keyboard, InputMappingType::DigitalToY, TriggerType::Down),		// WASD movement, 
					Key('A', DeviceType::Keyboard, InputMappingType::DigitalToNegX, TriggerType::Down),
					Key('S', DeviceType::Keyboard, InputMappingType::DigitalToNegY, TriggerType::Down),
					Key('D', DeviceType::Keyboard, InputMappingType::DigitalToX, TriggerType::Down),
					Key(0, DeviceType::Controller, InputMappingType::Axis2D),	// Movement using stick too
				},
				true }
			},
			{"Look",{
				{
					Key(0, DeviceType::Mouse, InputMappingType::Axis2D),		// WASD movement, 
				},
				false }
			},
			{"CLook",{
				{
					Key(1, DeviceType::Controller, InputMappingType::Axis2D),	// Movement using stick too
				},
				true }
			},
		});
	InputManager::MakeInputMapping("End",
		{
			{"Mainmenu", {
				{
					Key('\t', DeviceType::Keyboard, InputMappingType::Button, TriggerType::Released),
					Key(XINPUT_GAMEPAD_A, DeviceType::Controller, InputMappingType::Button, TriggerType::Released),
				}, false
				}
			},
		});
	InputManager::MakeInputMapping("Controls",
		{
			{"Hide", {
				{
					Key('\t', DeviceType::Keyboard, InputMappingType::Button, TriggerType::Released),
					Key(' ', DeviceType::Keyboard, InputMappingType::Button, TriggerType::Released),
					Key(XINPUT_GAMEPAD_A, DeviceType::Controller, InputMappingType::Button, TriggerType::Released),
				}, false
				}
			},
		});


	SceneManager::Get()->AddGameScene(new MainMenu());
}

LRESULT MainGame::WindowProcedureHook(HWND /*hWnd*/, UINT message, WPARAM /*wParam*/, LPARAM lParam)
{
	switch (message) {
	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT)
		{
			if (hCursor) {
				SetCursor(hCursor);
				return 0;
			}
		}
		break;
	}
//	
//	case WM_KEYUP:
//	{
//		if ((lParam & 0x80000000) != 0x80000000)
//			return -1;
//
//		////[F1] Toggle Scene Info Overlay
//		//if (wParam == VK_F1)
//		//{
//		//	const auto pScene = SceneManager::Get()->GetActiveScene();
//		//	pScene->GetSceneSettings().Toggle_ShowInfoOverlay();
//		//}
//
//		////[F2] Toggle Debug Renderer (Global)
//		//if (wParam == VK_F2)
//		//{
//		//	DebugRenderer::ToggleDebugRenderer();
//		//	return 0;
//
//		//}
//
//		////[F3] Previous Scene
//		//if (wParam == VK_F3)
//		//{
//		//	SceneManager::Get()->PreviousScene();
//		//	return 0;
//
//		//}
//
//		////[F4] Next Scene
//		//if (wParam == VK_F4)
//		//{
//		//	SceneManager::Get()->NextScene();
//		//	return 0;
//		//}
//
//		////[F5] If PhysX Framestepping is enables > Next Frame	
//		//if (wParam == VK_F6)
//		//{
//		//	const auto pScene = SceneManager::Get()->GetActiveScene();
//		//	pScene->GetPhysxProxy()->NextPhysXFrame();
//		//}
//	} break;
//	}

	return -1;
}
