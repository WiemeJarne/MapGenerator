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
#include "ChunkComponent.h"

void load()
{
	//create a scene
	auto scene = dae::SceneManager::GetInstance().CreateScene("scene", true);

	//register the sdl soundSystem
	//dae::ServiceLocator::registerSoundSystem<dae::SDLSoundSystem>();

	auto go{ std::make_shared<dae::GameObject>(scene) };
	go->AddComponent(std::make_unique<GenerateWorldPerlinNoiseComponent>(go.get(), 1024, scene));
	scene->Add(go);
}

int main(int, char* []) {
	
	dae::Engine engine("../Data/");
	engine.Run(load);
	return 0;
}