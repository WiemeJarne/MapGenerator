// Game.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#if _DEBUG
#if __has_include(<vld.h>)
#include <vld.h>
#endif
#endif

#include "GameObject.h"
#include "Engine.h"
#include "SceneManager.h"
#include "SoundServiceLocator.h"
#include "SDLSoundSystem.h"
#include "GenerateWorldPerlinNoiseComponent.h"
#include "Camera.h"
#include "MoveCameraCommand.h"
#include "glm/vec2.hpp"

void load()
{
	//create a scene
	auto scene = dae::SceneManager::GetInstance().CreateScene("scene", true);

	//register the sdl soundSystem
	//dae::ServiceLocator::registerSoundSystem<dae::SDLSoundSystem>();

	auto go{ std::make_shared<dae::GameObject>(scene) };
	go->AddComponent(std::make_unique<GenerateWorldPerlinNoiseComponent>(go.get(), 1280, 640, scene));
	scene->Add(go);
	
	scene->Add(std::make_unique<dae::Camera>(1.f));

	scene->AddKeyboardCommand(std::make_unique<dae::MoveCameraCommand>(glm::vec2{ 1.f, 0.f }, 10.f), dae::KeyState::pressed, dae::InputManager::KeyboardKey::A);
	scene->AddKeyboardCommand(std::make_unique<dae::MoveCameraCommand>(glm::vec2{ -1.f, 0.f }, 10.f), dae::KeyState::pressed, dae::InputManager::KeyboardKey::D);
	scene->AddKeyboardCommand(std::make_unique<dae::MoveCameraCommand>(glm::vec2{ 0.f, 1.f }, 10.f), dae::KeyState::pressed, dae::InputManager::KeyboardKey::W);
	scene->AddKeyboardCommand(std::make_unique<dae::MoveCameraCommand>(glm::vec2{ 0.f, -1.f }, 10.f), dae::KeyState::pressed, dae::InputManager::KeyboardKey::S);
}

int main(int, char* []) {
	
	dae::Engine engine("../Data/");
	engine.Run(load);
	return 0;
}