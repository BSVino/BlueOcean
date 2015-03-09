attribute vec3 vPosition;
attribute vec3 vNormal;

varying vec3 global_position;
varying vec3 fragment_normal;
varying float fragment_debug;

#ifdef WITH_FLEX
attribute float vFlex;
varying float fragment_flex;
uniform float flex_length;
#endif

uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;
uniform float time;
uniform vec3 camera_position;

#ifdef WITH_FLEX
vec2 flexphi(vec2 position, float t, float depth)
{
	vec2 to_blue = g_blue_position.xy - position;
	float to_blue_length = length(to_blue);
	vec2 to_blue_normalized = to_blue/to_blue_length;

	return 4.5*wave_vectors[0]*sin(dot(wave_vectors[0], position) - wave_frequencies[0]*time) +
		0.01*wave_vectors[1]*sin(dot(wave_vectors[1], position) - wave_frequencies[1]*time) +
		0.01*wave_vectors[2]*sin(dot(wave_vectors[2], position) - wave_frequencies[2]*time) +
		0.01*wave_vectors[3]*sin(dot(wave_vectors[3], position) - wave_frequencies[3]*time) +
		BlueBurst(t) * BlueBurst(t) * BlueBurst(t) * to_blue_normalized * clamp(1.0/(0.5*to_blue_length), 0.0, 1.0) * 1.5;
}

vec3 flex(float h, vec3 position, vec3 global_position, float t, float depth)
{
	vec2 phi = flexphi(global_position.xy, t, depth) - TAU * 0.5;
	float invcos = h * 0.5*(1.0 - cos(phi.x));

	vec3 cardioid = vec3(
		0.5 * sin(phi.x) * h * 0.5*(1.0 - cos(phi.x)),
		0.5 * sin(phi.y) * h * 0.5*(1.0 - cos(phi.y)),
		-cos(phi.x) * h * 0.5*(1.0 - cos(phi.x)) * flex_length
		);
	return mix(vec3(0.0, 0.0, position.z), cardioid, h);
}

float flexangle(float h, vec2 position, float t, float depth)
{
	vec2 phi = flexphi(position, t, depth);
	return h * 2.0 * phi;
}
#endif

void main()
{
	vec3 local_position = vPosition;

#ifdef NO_FIXUP
	vec3 local_normal = vNormal;
#else
	// I think my exporter is wrong.
	mat3 normal_fixup=mat3(1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, -1.0, 0.0);
	vec3 local_normal = normal_fixup * vNormal;
#endif

	vec3 global_origin = (Model * vec4(0.0, 0.0, 0.0, 1.0)).xyz; // Origin of the local in global space

	float surface_height = 10;
	float depth = (surface_height - global_origin.z)/11.0;

#ifdef WITH_FLEX
	vec3 from_camera = global_origin - camera_position;
	from_camera.z = 0;
	vec3 from_camera_flat = normalize(from_camera);
	vec3 up = vec3(0.0, 0.0, 1.0);
	mat3 rotate_to_camera = mat3(from_camera_flat, -cross(from_camera_flat, up), up);
	local_position = rotate_to_camera * local_position;
	local_normal = rotate_to_camera * local_normal;

	fragment_flex = vFlex;

	vec3 flex_result = flex(fragment_flex, local_position, global_origin, time, depth);
	local_position.x += flex_result.x;
	local_position.y += flex_result.y;
	local_position.z = flex_result.z;

	float flex_angle = flexangle(fragment_flex, global_origin.xy, time, depth) * 2.0;
	float cos_angle = cos(flex_angle);
	float sin_angle = sin(flex_angle);
	mat3 normal_flex = mat3(cos_angle, 0.0, sin_angle, 0.0, 1.0, 0.0, -sin_angle, 0.0, cos_angle);
	local_normal = normal_flex * local_normal;
#endif

	fragment_normal = (Model * vec4(local_normal, 0.0)).xyz;
	global_position = (Model * vec4(local_position, 1.0)).xyz;
	gl_Position = Projection * View * vec4(global_position, 1.0);
}

