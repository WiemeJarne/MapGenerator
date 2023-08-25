#pragma once
#include <Component.h>
#include "SDL.h"
#include "PerlinNoise2D.h"
#include <memory>
#include "Scene.h"

class GenerateWorldPerlinNoiseComponent final : public dae::Component
{
public:
	//the worldSize should be a power of 2 and bigger than or equal to 16x16
	GenerateWorldPerlinNoiseComponent(dae::GameObject* pOwner, int worldXSize, int worldYSize, dae::Scene* pScene);

	virtual void Update() override {}
	virtual void Render() const override {}
	virtual void RenderImGui() override {}

	SDL_Color CalculateCellColor(int x, int y);

private:
	std::unique_ptr<PerlinNoise2D> m_Perlin2D{};
};