// per-fragment interpolated values from the vertex shader
varying vec3 global_position;
varying vec3 fragment_normal;
varying float fragment_debug;

uniform vec3  AmbientProduct, DiffuseProduct, SpecularProduct;
uniform mat4  Model;
uniform float Shininess;
uniform sampler2D Tex;
uniform int   EnableTex;
uniform vec3 camera_position;
uniform float time;
uniform float alpha;

#ifdef WITH_FLEX
varying float fragment_flex;
#endif

const vec3 g_blue = vec3(0.0/255.0, 60.0/255.0, 255.0/255.0);
const vec3 g_fog = vec3(0.0, 126.0/255.0, 159.0/255.0);
const vec3 g_white = vec3(2.5, 2.5, 2.5);
const float g_blue_initial_radius = 1.5;

float PrePostBurst(float t)
{
	return clamp(t - g_grow_time + 0.5, 0.0, 1.0);
}

float BlueRadius(float t)
{
	return g_blue_initial_radius + mix(sin(t)*0.5, t-g_grow_time, PrePostBurst(t)) + BlueBurst(t);
}

vec3 AddBlueFog(vec3 color, vec3 position, vec3 camera, float t)
{
	vec3 vec = position - camera;

	float vec_length = length(vec);
	vec3 vec_normalized = vec / vec_length;
	float ramp = pow(0.89, vec_length);

	// Check to see if we have any blue in here.
	float blue_radius = BlueRadius(t);
	vec3 to_blue = g_blue_position - camera;
	float to_blue_projection_coefficient = dot(to_blue, vec_normalized);
	vec3 to_blue_projected = to_blue_projection_coefficient * vec_normalized; // to_blue projected onto the camera->position line
	vec3 distance_from_blue = to_blue - to_blue_projected; // The shortest vector from the center of blue to the camera->position line
	float distance_from_blue_length = length(distance_from_blue);
	float proportion_in_blue = distance_from_blue_length / blue_radius;
	float in_blue = sign(1.0 - proportion_in_blue) * 0.5 + 0.5; // 1 if we're in the blue, 0 otherwise
	float in_front = sign(to_blue_projection_coefficient) * 0.5 + 0.5; // 1 if we're in front of the camera, 0 otherwise. (Used to cull results from behind.)

	float amount_of_blue = 2 * blue_radius * sin(acos(proportion_in_blue));

	float to_blue_projected_sqr = dot(to_blue_projected, to_blue_projected);
	vec3 blue_sphere_intersection = to_blue_projected - vec_normalized * amount_of_blue * 0.5;
	float intersected_sphere = sign(dot(vec, vec) - dot(blue_sphere_intersection, blue_sphere_intersection)) * 0.5 + 0.5; // 0 if the end vertex is before the start of the sphere

	float ratio_of_blue = clamp(amount_of_blue/vec_length, 0.0, 1.0);

	return mix(mix(g_fog, g_blue, ratio_of_blue * in_blue * intersected_sphere * in_front), color, ramp);
}

vec2 cycloid_newton(vec2 guess, float time, float depth, vec2 k)
{
	vec2 f = cycloid_v(guess, time, depth, k);
	vec2 df = cycloid_dv(guess, time, depth);
	return guess - vec2(f.x / df.x, f.y / df.y);
}

void cycloid_bisection(inout vec2 left, inout vec2 midpoint, float time, float depth, vec2 k, inout vec2 a, inout vec2 b)
{
	vec2 mid_value;
	float x_sign, x_test, y_sign, y_test;

	mid_value = cycloid_v(midpoint, time, depth, k);

	x_sign = sign(left.x * mid_value.x);
	x_test = (x_sign+1.0)*0.5;
	b.x = mix(midpoint.x, b.x, x_test);
	a.x = mix(a.x, midpoint.x, x_test);

	y_sign = sign(left.y * mid_value.y);
	y_test = (y_sign+1.0)*0.5;
	b.y = mix(midpoint.y, b.y, y_test);
	a.y = mix(a.y, midpoint.y, y_test);

	midpoint = (a+b)*0.5;
}

float caustic_weight(vec2 position, float time, float depth)
{
	vec2 ddv = cycloid_ddv(position, time, depth);
	return (-ddv.x -ddv.y); // If we're concave down then the light rays are more focused, so negative values here are better.
}

float caustic_test_for_root(vec2 position, float time, float depth, vec2 offset, vec2 a, vec2 b)
{
	vec2 left = cycloid_v(a, time, depth, offset);

	// A test to make sure I can always find my roots.
	/*vec2 right = cycloid_v(b, time, depth, offset);
	if (left.x * right.x > 0 || left.y * right.y > 0)
		return 0;
	return 1;*/

	vec2 midpoint = (a+b)*0.5;

	cycloid_bisection(left, midpoint, time, depth, offset, a, b);
	cycloid_bisection(left, midpoint, time, depth, offset, a, b);
	cycloid_bisection(left, midpoint, time, depth, offset, a, b);
	cycloid_bisection(left, midpoint, time, depth, offset, a, b);

	vec2 cycloid_position = midpoint;

	cycloid_position = cycloid_newton(cycloid_position, time, depth, offset);
	cycloid_position = cycloid_newton(cycloid_position, time, depth, offset);
	cycloid_position = cycloid_newton(cycloid_position, time, depth, offset);

	//return abs(cycloid_v(cycloid_position, time, depth, offset).x); // An accuracy test, blacker is better
	return caustic_weight(cycloid_position, time, depth);
}

float caustic(float time, float depth, vec2 position)
{
	vec2 cycloid_position = cycloid_v(position, time, depth, vec2(0.0, 0.0));

	vec2 initial_guess = 2*position - cycloid_position; // Back up a bit to try to undo what the cycloid did. This will be the initial guess.

	float bound = 2.5*depth; // A bound for where my root is, determined by testing.

	float r1 = caustic_test_for_root(initial_guess, time, depth, position, initial_guess - bound, initial_guess + bound);

	// Let's not do the other tests, we'll assume for simplicity there's only one root in this range.
	//float r2 = caustic_test_for_root(initial_guess, time, depth, position, initial_guess - depth, initial_guess);
	//float r3 = caustic_test_for_root(initial_guess, time, depth, position, initial_guess, initial_guess + depth);
	//float r4 = caustic_test_for_root(initial_guess, time, depth, position, initial_guess + depth, initial_guess + bound*depth);

	return r1;
}

vec3 AddBlue(vec3 color, vec3 position, float t)
{
	float blue_radius = BlueRadius(t);
	float blue_radius_sqr = blue_radius*blue_radius;

	vec3 distance_to_blue = g_blue_position - position;
	float distance_to_blue_sqr = dot(distance_to_blue, distance_to_blue);
	float distance_to_blue_border = blue_radius_sqr - distance_to_blue_sqr;
	float is_blue = sign(distance_to_blue_border) * 0.5 + 0.5;

	float degree_of_blue = 1.0 - distance_to_blue_border/blue_radius_sqr;
	float amount_of_white = clamp(degree_of_blue - 0.5, 0.0, 1.0) * mix((cos(t) * 0.5 + 0.5), 1.0, PrePostBurst(t));
	degree_of_blue = degree_of_blue * 0.5 + 0.5;
	vec3 final_color = mix(g_blue, color + g_white, amount_of_white);

	return mix(color, final_color, degree_of_blue * is_blue);
}

void main()
{
	float surface_height = 10;
	float depth = (surface_height - global_position.z)/11.0;

	vec3 light_direction = vec3(0, 0, 1);
	vec3 camera_direction = normalize(camera_position - global_position);

	vec3 unit_fragment_normal = normalize(fragment_normal);

	float sun_dot = clamp(dot(unit_fragment_normal, light_direction), 0.0, 1.0);

	vec3 sun_term = vec3(sun_dot, sun_dot, sun_dot);

	// Never show speculars on the other sides of lights.
	float specular_weight = clamp(20.0*dot(light_direction, unit_fragment_normal), 0.0, 1.0);
	vec3 reflection = mix(vec3(0.0,0.0,0.0), (2.0 * sun_dot) * unit_fragment_normal - light_direction, specular_weight);

	float caustic_term = caustic(time, depth, global_position.xy) + 0.08;
	float caustic_sqr = caustic_term*caustic_term; // Increase contrast
	sun_term *= clamp(caustic_sqr*30.0, 0.0, 1.0);

	float caustic_specular = clamp(caustic_term * 8.0 - 0.4, 0.0, 1.0);
	vec3 phong = AmbientProduct + sun_term*DiffuseProduct + clamp(pow(dot(reflection, camera_direction), Shininess) * caustic_specular * SpecularProduct, 0.0, 1.0);

#ifdef WITH_FADE
	// Don't know why.
	vec3 fragcolor = AmbientProduct + sun_term*DiffuseProduct;
	fragcolor *= 0.001;
	fragcolor += AmbientProduct.xyz;
#else
	vec3 blued = AddBlue(phong, global_position, time);
	vec3 fragcolor = AddBlueFog(blued, global_position, camera_position, time);
#endif

//#ifdef WITH_FLEX
//	fragcolor.r = fragment_debug;
//	fragcolor.gb *= 0.01;
//#endif

	gl_FragColor = vec4(fragcolor, 1.0);

#ifdef WITH_FADE
	gl_FragColor.a = alpha;
#endif
}



