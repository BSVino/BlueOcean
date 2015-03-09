const float g_burst_time = 31.0;
const float g_grow_time = 37.0;
const vec3 g_blue_position = vec3(15.0, 0, -6);
//const float g_burst_time = 4.0;
//const float g_grow_time = 10.0;
//const vec3 g_blue_position = vec3(-26.0, 2.2, -12);

// A wavelet that specifies a burst of energy before the expansion begins.
float BlueBurst(float t)
{
	float burst_time = t-g_burst_time;
	return -2.4 * burst_time * exp(-burst_time*burst_time);
}

const vec2 wave_vectors[4] = vec2[4](
	vec2(0.955, 0.296)*0.1,
	vec2(0.866, 0.5)*3.0,
	vec2(0.866, -0.5)*3.0,
	vec2(-1.0, 0.0)*3.0
);

const float wave_magnitudes[4] = float[4](
	43.0,
	0.0003,
	0.0004,
	0.0005
);

const float wave_frequencies[4] = float[4](
	0.5,
	4.0,
	5.0,
	3.0
);

// My nifty cycloid function that simulates the surface of the water
vec2 cycloid_v(vec2 position, float time, float depth, vec2 k)
{
	time = -time;
	return (position - depth*(
		wave_vectors[0]*wave_magnitudes[0]*sin(dot(wave_vectors[0], position) + wave_frequencies[0]*time) +
		wave_vectors[1]*wave_magnitudes[1]*sin(dot(wave_vectors[1], position) + wave_frequencies[1]*time) +
		wave_vectors[2]*wave_magnitudes[2]*sin(dot(wave_vectors[2], position) + wave_frequencies[2]*time) +
		wave_vectors[3]*wave_magnitudes[3]*sin(dot(wave_vectors[3], position) + wave_frequencies[3]*time)
		)) - k;
}

// The first derivative for doing newton's method
vec2 cycloid_dv(vec2 position, float time, float depth)
{
	time = -time;
	return vec2(1.0, 1.0) - depth*(
		wave_vectors[0]*wave_vectors[0]*wave_magnitudes[0]*cos(dot(wave_vectors[0], position) + wave_frequencies[0]*time) +
		wave_vectors[1]*wave_vectors[1]*wave_magnitudes[1]*cos(dot(wave_vectors[1], position) + wave_frequencies[1]*time) +
		wave_vectors[2]*wave_vectors[2]*wave_magnitudes[2]*cos(dot(wave_vectors[2], position) + wave_frequencies[2]*time) +
		wave_vectors[3]*wave_vectors[3]*wave_magnitudes[3]*cos(dot(wave_vectors[3], position) + wave_frequencies[3]*time)
		);
}

// The second derivative for the weight function
vec2 cycloid_ddv(vec2 position, float time, float depth)
{
	time = -time;
	return depth*(
		wave_vectors[0]*wave_vectors[0]*wave_vectors[0]*wave_magnitudes[0]*sin(dot(wave_vectors[0], position) + wave_frequencies[0]*time) +
		wave_vectors[1]*wave_vectors[1]*wave_vectors[1]*wave_magnitudes[1]*sin(dot(wave_vectors[1], position) + wave_frequencies[1]*time) +
		wave_vectors[2]*wave_vectors[2]*wave_vectors[2]*wave_magnitudes[2]*sin(dot(wave_vectors[2], position) + wave_frequencies[2]*time) +
		wave_vectors[3]*wave_vectors[3]*wave_vectors[3]*wave_magnitudes[3]*sin(dot(wave_vectors[3], position) + wave_frequencies[3]*time)
		);
}
