#pragma once
#include <vector>

//source: https://adrianb.io/2014/08/09/perlinnoise.html

class PerlinNoise2D final
{
public:
	//the bigger the permutation table:
	//1. the algorithm uses a wider range of gradient vectors which could result in the generation of noise patterns with high frequencies, which could create more intricate and detailed noise
	//2. because of the more unique gradient vectors this could lead to noise generation with less repetition
	PerlinNoise2D(unsigned int seed = 1, int permutationTableSize = 256);

	double GetNoise(double x, double y) const;
	double GetNoise(double x, double y, int amountOfOctaves, double persistence);

private:
	std::vector<int> m_PermutationTable{}; 
	int m_PermutationTableSize{};

	double Fade(double value) const;
	//x and y are values between [0-255]
	int GetHashValue(int x, int y) const;
	int Increment(int value, int maxValue) const;
	double Gradient(int hash, double x, double y) const;

	static int temp[256];
};