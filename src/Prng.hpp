#ifndef PRNG_H
#define PRNG_H

#include <cstring>



class Prng
{
	const char* seed;
	long hash;
	
	
	/** based on java implementation of hashCode
	 * s[0]*31^(n-1) + s[1]*31^(n-2) + ... + s[n-1]
	 * source: http://docs.oracle.com/javase/6/docs/api/java/lang/String.html#hashCode()
	 */
	long _hash(const char* str)
	{
		long h = 0;
		for (size_t i = 0; i < strlen(str); ++i)
			h = h * 31 + str[i];
		return h;
	}
	
public:
	Prng(const char* s) : seed(s), hash(_hash(seed))
	{}
	
	
	/** based on use of linear congruential generator (lcg) in borland implementation of random()
	 * source: http://www.cigital.com/papers/download/developer_gambling.php
	 *
	 * borland lcg parameters m=2^32, a=134775813, c=1
	 * source: ttp://en.wikipedia.org/wiki/Linear_congruential_generator
	 */
	double random()
	{
		hash = hash * 134775813 + 1 & 0xffffffff;
#ifdef WIN32
		return 0.5 + (double) hash / 0x100000000;
#else
		return (double) hash / 0x100000000;
#endif
	}
};


#endif
