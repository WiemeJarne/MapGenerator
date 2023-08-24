#include "GenerateWorldPerlinNoiseComponent.h"
#include "GameObject.h"
#include "ChunkComponent.h"
#include <iostream>

GenerateWorldPerlinNoiseComponent::GenerateWorldPerlinNoiseComponent(dae::GameObject* pOwner, int worldSize, dae::Scene* pScene)
	: Component(pOwner)
	, m_WorldSize{ worldSize }
{
	if (m_WorldSize < 16)
	{
		m_WorldSize = 16;
	}

	//create the perlin noise generator
	m_Perlin2D = std::make_unique<PerlinNoise2D>();

	//calculate the amount of chunks
	const int amountOfChunks{ m_WorldSize / 16 };

	//create the gameObjects with a chunkComponent and add it as a child to the owner of this component
	for (int rowNr{}; rowNr < amountOfChunks; ++rowNr)
	{
		for (int colNr{}; colNr < amountOfChunks; ++colNr)
		{
			auto go{ std::make_shared<dae::GameObject>(pScene) };
			go->AddComponent( std::make_unique<ChunkComponent>(go.get(), rowNr * 16, colNr * 16, this));
			go->SetLocalPosition(static_cast<float>(rowNr) * 16, static_cast<float>(colNr) * 16);
			pScene->Add(go);
			go->SetParent(pOwner, false);
		}
	}
}

SDL_Color GenerateWorldPerlinNoiseComponent::CalculateCellColor(int x, int y)
{
	constexpr float devider{ 64 };

	double noise{ (m_Perlin2D->GetNoise(x / devider, y / devider, 4, 0.55f) + 1) / 2.f };
	
	if (noise < 0.45f)
		return SDL_Color(0, 255u, 0);
	else
		return SDL_Color(0, 0, 255u);
}