#pragma once
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>
#include <numeric>

class perlin
{
public:
	perlin();
	perlin(unsigned int seed);
	~perlin();

	
	// The permutation vector
	std::vector<int> p;
	// Get a noise value, for 2D images z can have any value
	double noise(double x, double y, double z);


private:

	double fade(double t);
	double lerp(double t, double a, double b);
	double grad(int hash, double x, double y, double z);
				 

};

