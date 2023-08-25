#include "GenerateWorldPerlinNoiseComponent.h"
#include "GameObject.h"
#include <iostream>
#include <SDL.h>
#include "SDL_image.h"
#include "Renderer.h"
#include "TextureComponent.h"
#include "RenderComponent.h"

GenerateWorldPerlinNoiseComponent::GenerateWorldPerlinNoiseComponent(dae::GameObject* pOwner, int worldXSize, int worldYSize, dae::Scene* )
	: Component(pOwner)
{
	//create the perlin noise generator
	m_Perlin2D = std::make_unique<PerlinNoise2D>();

	//create the texture
	auto pColorTexture{ SDL_CreateTexture(dae::Renderer::GetInstance().GetSDLRenderer(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, worldXSize, worldYSize) };

	//save the biggest side
	int biggestSide{ worldYSize };

	if (worldXSize > worldYSize)
		biggestSide = worldXSize;

	//set the texture's color data
	SDL_PixelFormat* pixelFormat = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
	uint32_t* pPixels = new uint32_t[biggestSide * biggestSide]{};
	
	for (int rowNr{}; rowNr < biggestSide; ++rowNr)
	{
		for (int colNr{}; colNr < biggestSide; ++colNr)
		{
			auto color{ CalculateCellColor(rowNr, colNr) };
			pPixels[rowNr * biggestSide + colNr] = SDL_MapRGBA(pixelFormat, color.r, color.g, color.b, color.a);
		}
	}
	
	SDL_FreeFormat(pixelFormat);

	SDL_UpdateTexture(pColorTexture, nullptr, pPixels, biggestSide * sizeof(uint32_t));

	//create a texture component
	auto textureComp{ std::make_unique<dae::TextureComponent>(pOwner, pColorTexture) };

	//add a renderComponent to the owner of this one
	pOwner->AddComponent(std::make_unique<dae::RenderComponent>(pOwner, std::move(textureComp)));

	//initialize sdl image
	if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != IMG_INIT_PNG)
		std::cout << "SDL_image initialization failed: " << IMG_GetError() << '\n';

	//safe the texture
	auto pSurface{ SDL_CreateRGBSurfaceWithFormatFrom(pPixels, worldXSize, worldYSize, 32, worldXSize * sizeof(uint32_t), SDL_PIXELFORMAT_RGBA8888) };
	if (pSurface)
	{
		if (IMG_SaveJPG(pSurface, "test2.jpg", 75) == 0)
			std::cout << "world saved!\n";
		else
			std::cout << IMG_GetError() << '\n';
		SDL_FreeSurface(pSurface);
	}
}

SDL_Color GenerateWorldPerlinNoiseComponent::CalculateCellColor(int x, int y)
{
	constexpr float devider{ 128 };

	double noise{ (m_Perlin2D->GetNoise(x / devider, y / devider, 4, 0.55f) + 1) / 2.f };
	//return SDL_Color{ static_cast<Uint8>(noise * 255), static_cast<Uint8>(noise * 255), static_cast<Uint8>(noise * 255) };
	if (noise < 0.475f)
		return SDL_Color(0, 255u, 0);
	else
		return SDL_Color(0, 0, 255u);
}