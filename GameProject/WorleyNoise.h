#pragma once
#include <vector>
#include "glm/vec2.hpp"

class WorleyNoise final
{
public:
	//featurePointRatio example: ratio = 0.001 this means 1 feature point per 1000 pixels
	WorleyNoise(unsigned int seed, int xSize, int ySize, double featurePointRatio = 0.001);
	float GetNoise(int x, int y);

private:
	int m_XSize{};
	int m_YSize{};
	int m_BiggestSide{};
	std::vector<glm::ivec2> m_FeaturePoints{};

	float DeterministicFunction(int input);
	float MapToRange(float input, float minInput, float maxInput, float minOutput, float maxOutput);
};