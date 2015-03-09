#define M_TAU 6.28318530718

inline float RemapVal(float flInput, float flInLo, float flInHi, float flOutLo, float flOutHi)
{
	return (((flInput - flInLo) / (flInHi - flInLo)) * (flOutHi - flOutLo)) + flOutLo;
}

inline float RemapValClamped(float flInput, float flInLo, float flInHi, float flOutLo, float flOutHi)
{
	float flReturn = RemapVal(flInput, flInLo, flInHi, flOutLo, flOutHi);

	if (flOutHi < flOutLo)
		std::swap(flOutHi, flOutLo);

	if (flReturn < flOutLo)
		return flOutLo;

	if (flReturn > flOutHi)
		return flOutHi;

	return flReturn;
}

inline vec3 LerpValue(vec3 from, vec3 to, float flLerp)
{
	return to * flLerp + from * (1 - flLerp);
}

inline float Approach(float flGoal, float flInput, float flAmount)
{
	float flDifference = flGoal - flInput;

	if (flDifference > flAmount)
		return flInput + flAmount;
	else if (flDifference < -flAmount)
		return flInput -= flAmount;
	else
		return flGoal;
}

inline float RandomFloat(float flLow, float flHigh)
{
	return RemapVal((float)rand(), 0, RAND_MAX, flLow, flHigh);
}

vec3 AngleVector(float p, float y)
{
	vec3 result;

	p = (float)(p * (M_TAU / 360));
	y = (float)(y * (M_TAU / 360));

	float sp = sin(p);
	float cp = cos(p);
	float sy = sin(y);
	float cy = cos(y);

	result.x = cp*cy;
	result.y = cp*sy;
	result.z = sp;

	return result;
}

// Simplex noise implementation by Ken Perlin.
class SimplexNoise
{
public:
	SimplexNoise();
	SimplexNoise(size_t seed);

public:
	void Init(size_t seed);

	float Noise(float x, float y);

private:
	inline float Grad(int hash, float x, float y)
	{
		int h = hash & 7;
		float u = h<4 ? x : y;
		float v = h<4 ? y : x;
		return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f*v : 2.0f*v);
	}

	inline float Fade(float t)
	{
		return t*t*t*(t*(t * 6 - 15) + 10);
	}

	inline int Floor(float x)
	{
		if (x >= 0)
			return (int)x;
		else
			return (int)x - 1;
	}

	inline float Lerp(float t, float a, float b)
	{
		return a + t*(b - a);
	}

private:
	unsigned char rand_table[512];
};

inline SimplexNoise::SimplexNoise()
{
}

inline SimplexNoise::SimplexNoise(size_t seed)
{
	Init(seed);
}

inline void SimplexNoise::Init(size_t seed)
{
	srand(seed);
	for (size_t i = 0; i < 256; i++)
		rand_table[i] = (unsigned char)(rand() % 255);
	memcpy(&rand_table[256], &rand_table[0], 256);
}

inline float SimplexNoise::Noise(float x, float y)
{
	int ix0, iy0, ix1, iy1;
	float fx0, fy0, fx1, fy1;
	float s, t, nx0, nx1, n0, n1;

	ix0 = Floor(x);
	iy0 = Floor(y);
	fx0 = x - ix0;
	fy0 = y - iy0;
	fx1 = fx0 - 1.0f;
	fy1 = fy0 - 1.0f;
	ix1 = (ix0 + 1) & 0xff;
	iy1 = (iy0 + 1) & 0xff;
	ix0 = ix0 & 0xff;
	iy0 = iy0 & 0xff;

	t = Fade(fy0);
	s = Fade(fx0);

	nx0 = Grad(rand_table[ix0 + rand_table[iy0]], fx0, fy0);
	nx1 = Grad(rand_table[ix0 + rand_table[iy1]], fx0, fy1);
	n0 = Lerp(t, nx0, nx1);

	nx0 = Grad(rand_table[ix1 + rand_table[iy0]], fx1, fy0);
	nx1 = Grad(rand_table[ix1 + rand_table[iy1]], fx1, fy1);
	n1 = Lerp(t, nx0, nx1);

	return 0.507f * (Lerp(s, n0, n1));
}
