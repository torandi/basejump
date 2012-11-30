#ifndef PERLIN_NOISE_H
#define PERLIN_NOISE_H



class PerlinNoise
{
	static const char m_grad3[32][3];// gradients for both 2d and 3d noise
	short m_ps[512];// permutations, random numbers shuffled by seed
	
	const char * m_seed;
	
	static inline int fastfloor(double x) {
		return x>0.0 ? static_cast<int>(x) : static_cast<int>(x-1.0); }
	
	static inline double blend(double t) {
		return t*t*(3-2*t); }
	
	static inline double dot(const char grad[], double x, double y) {
		return grad[0]*x + grad[1]*y; }
	
	static inline double dot(const char grad[], double x, double y, double z) {
		return grad[0]*x + grad[1]*y + grad[2]*z; }
	
	static inline double lerp(double a, double b, double t) {
		return a + t*(b-a); }
	
	inline double calculateSignal(double x, double y, double offset);
	
public:
	PerlinNoise(const char * seed);
	virtual ~PerlinNoise();
	
	double noise(double x);
	double noise(double x, double y);
	double noise(double x, double y, double z);
	double ridgedMultifractalNoise(double x, double y, double H, double lacunarity, double octaves, double offset, double gain);
};


#endif
