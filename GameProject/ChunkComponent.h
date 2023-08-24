#pragma once
#include <Component.h>
#include <vector>
#include "SDL.h"

class GenerateWorldPerlinNoiseComponent;

class ChunkComponent final : public dae::Component
{
public:
	ChunkComponent(dae::GameObject* pOwner, int topLeftX, int topLeftY, GenerateWorldPerlinNoiseComponent* worldGenerateComponent);

	virtual void Update() override {}
	virtual void Render() const override;
	virtual void RenderImGui() override {}

private:
	std::vector<SDL_Color> m_CellColors{};
	GenerateWorldPerlinNoiseComponent* m_WorldGenerateComponent{};
};