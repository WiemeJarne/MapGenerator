#pragma once
#include <Component.h>
#include "SDL.h"
#include "PerlinNoise2D.h"
#include <memory>
#include "Scene.h"
#include <vector>
#include "TextureComponent.h"

class GenerateWorldPerlinNoiseComponent final : public dae::Component
{
public:
	//the worldSize should be a power of 2 and bigger than or equal to 16x16
	GenerateWorldPerlinNoiseComponent(dae::GameObject* pOwner, int worldXSize, int worldYSize);

	virtual void Update() override;
	virtual void Render() const override {}
	virtual void RenderImGui() override;

	SDL_Color CalculateCellColor(int x, int y);
	void SafeWorldAsJPG();

private:
	int m_PreviousWorldXSize{};
	int m_WorldXSize{};
	int m_PreviousWorldYSize{};
	int m_WorldYSize{};
	int m_PreviousPerlinNoiseSeed{};
	int m_PerlinNoiseSeed{};
	std::vector<uint32_t> m_Pixels{};
	std::unique_ptr<PerlinNoise2D> m_Perlin2D{};
	dae::TextureComponent* m_pTextureComponent{};
	bool m_ShouldRegenerate{};
	bool m_ShouldSaveTextureAsJPG{};
	std::wstring m_JPGSaveFolderPath{};

	void Regenerate();
};