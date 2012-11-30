#include <cmath>
#include <algorithm>

#include "Prng.hpp"
#include "PerlinNoise.hpp"

#include <iostream>


const char PerlinNoise::m_grad3[32][3] = {
	// 2*12 edges
	{ 0, 1, 1 },
	{ 0, 1,-1 },
	{ 0,-1, 1 },
	{ 0,-1,-1 },
	{ 1, 0, 1 },
	{ 1, 0,-1 },
	{-1, 0, 1 },
	{-1, 0,-1 },
	{ 1, 1, 0 },
	{ 1,-1, 0 },
	{-1, 1, 0 },
	{-1,-1, 0 },
	
	{ 0, 1, 1 },
	{ 0, 1,-1 },
	{ 0,-1, 1 },
	{ 0,-1,-1 },
	{ 1, 0, 1 },
	{ 1, 0,-1 },
	{-1, 0, 1 },
	{-1, 0,-1 },
	{ 1, 1, 0 },
	{ 1,-1, 0 },
	{-1, 1, 0 },
	{-1,-1, 0 },
	
	// 8 corners
	{ 1, 1, 1 },
	{ 1, 1,-1 },
	{ 1,-1, 1 },
	{-1, 1, 1 },
	{ 1,-1,-1 },
	{-1, 1,-1 },
	{-1,-1, 1 },
	{-1,-1,-1 },
};


PerlinNoise::PerlinNoise(const char * seed) : m_seed(seed)
{
	Prng prng(m_seed);
	
	// build sorted permutation array
	for (short i=0; i<256; ++i)
		m_ps[i] = i;

	// shuffle permutation array, Fisher-Yates style
	// simultaneously put copy of array after the array
	for (int i=255, j, k; i>0; --i)
	{
		j = fastfloor(prng.random() * (double)(i+1));
		k = m_ps[i];
		m_ps[i] = m_ps[i+256] = m_ps[j];
		m_ps[j] = m_ps[j+256] = k;
	}
}


PerlinNoise::~PerlinNoise() {}


double PerlinNoise::noise(double x, double y)
{
	// floor, get integer parts
	int ix = fastfloor(x);
	int iy = fastfloor(y);
	
	// get decimal parts
	x -= ix;
	y -= iy;
	
	// limit values to int range [0, 255]
	ix &= 255;
	iy &= 255;
	
	// get all adjacent gradient vector indexes
	int gi00 = m_ps[ix   + m_ps[iy]]   & 31;
	int gi10 = m_ps[ix+1 + m_ps[iy]]   & 31;
	int gi01 = m_ps[ix   + m_ps[iy+1]] & 31;
	int gi11 = m_ps[ix+1 + m_ps[iy+1]] & 31;
	
	// calculate noise from each adjacent gradient
	double n00 = dot(m_grad3[gi00], x,   y);
	double n10 = dot(m_grad3[gi10], x-1, y);
	double n01 = dot(m_grad3[gi01], x,   y-1);
	double n11 = dot(m_grad3[gi11], x-1, y-1);
	
	// blend decimal parts
	double fx = blend(x);
	double fy = blend(y);
	
	// bilinear interpolate noise from all adjacent gradients
	double nx0 = lerp(n00, n10, fx);
	double nx1 = lerp(n01, n11, fx);
	double nxy = lerp(nx0, nx1, fy);
	
	return nxy;
}


double PerlinNoise::noise(double x, double y, double z)
{
	// floor, get integer parts
	int ix = fastfloor(x);
	int iy = fastfloor(y);
	int iz = fastfloor(z);
	
	// get decimal parts
	x -= ix;
	y -= iy;
	z -= iz;
	
	// limit values to int range [0, 255]
	ix &= 255;
	iy &= 255;
	iz &= 255;
	
	// get all adjacent gradient vector indexes
	int gi000 = m_ps[ix   + m_ps[iy   + m_ps[iz]]]   & 31;
	int gi100 = m_ps[ix+1 + m_ps[iy   + m_ps[iz]]]   & 31;
	int gi010 = m_ps[ix   + m_ps[iy+1 + m_ps[iz]]]   & 31;
	int gi001 = m_ps[ix   + m_ps[iy   + m_ps[iz+1]]] & 31;
	int gi110 = m_ps[ix+1 + m_ps[iy+1 + m_ps[iz]]]   & 31;
	int gi101 = m_ps[ix+1 + m_ps[iy   + m_ps[iz+1]]] & 31;
	int gi011 = m_ps[ix   + m_ps[iy+1 + m_ps[iz+1]]] & 31;
	int gi111 = m_ps[ix+1 + m_ps[iy+1 + m_ps[iz+1]]] & 31;
	
	// calculate noise from each adjacent gradient
	double n000 = dot(m_grad3[gi000], x,   y,   z);
	double n100 = dot(m_grad3[gi100], x-1, y,   z);
	double n010 = dot(m_grad3[gi010], x,   y-1, z);
	double n001 = dot(m_grad3[gi001], x,   y,   z-1);
	double n110 = dot(m_grad3[gi110], x-1, y-1, z);
	double n101 = dot(m_grad3[gi101], x-1, y,   z-1);
	double n011 = dot(m_grad3[gi011], x,   y-1, z-1);
	double n111 = dot(m_grad3[gi111], x-1, y-1, z-1);
	
	// blend decimal parts
	double fx = blend(x);
	double fy = blend(y);
	double fz = blend(z);
	
	// trilinear interpolate noise from all adjacent gradients
	double nx00 = lerp(n000, n100, fx);
	double nx10 = lerp(n010, n110, fx);
	double nx01 = lerp(n001, n101, fx);
	double nx11 = lerp(n011, n111, fx);
	double nxy0 = lerp(nx00, nx10, fy);
	double nxy1 = lerp(nx01, nx11, fy);
	double nxyz = lerp(nxy0, nxy1, fz);
	
	return nxyz;
}
	


inline double PerlinNoise::calculateSignal(double x, double y, double offset)
{
	double signal = noise(x, y); // get first octave
	signal = std::abs(signal); // absolute value creates the ridges
	signal = offset - signal; // invert and translate (note that “offset” should be ~= 1.0)
	signal *= signal; // square the signal, to increase “sharpness” of ridges
	return signal;
}


/* Source: Texturing & Modeling: A Procedural Approach, 3rd ed., p. 504
 *
 * Ridged multifractal terrain model.
 * Some good parameter values to start with:
 *
 * H: 1.0
 * offset: 1.0
 * gain: 2.0
 */
double PerlinNoise::ridgedMultifractalNoise(double x, double y, double H, double lacunarity, double octaves, double offset, double gain)
{
	double result, signal,
		frequency = 1.0,
		weight = 1.0;
	
	// compute spectral weights
	// TODO: exponent_array can/should be cached
	double * exponent_array = new double[(int) octaves+1];
	for (int i=0; i<=octaves; ++i)
	{
		// compute weight for each frequency
		exponent_array[i] = pow( frequency, -H );
		frequency *= lacunarity;
	}
	
	result = calculateSignal(x, y, offset); // assign initial values
	
	for(int i=1; i<octaves; ++i)
	{
		// increase the frequency
		x *= lacunarity;
		y *= lacunarity;
		
		// weight successive contributions by previous signal
		weight = signal * gain;
		weight = std::max(0.0, std::min(1.0, weight));
		signal = calculateSignal(x, y, offset);
		
		// weight the contribution
		signal *= weight;
		result += signal * exponent_array[i];
	}
	
	delete[] exponent_array;

	return result;
}
