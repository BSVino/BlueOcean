#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <math.h>

#include "../BlueOcean/ocean.h"

struct vec3
{
	float x, y, z;

	vec3(){}
	vec3(float X, float Y, float Z)
	{
		x = X; y = Y; z = Z;
	}

	vec3 operator * (float s) const
	{
		return vec3(s*x, s*y, s*z);
	}
	vec3 operator + (vec3 v) const
	{
		return vec3(x + v.x, y + v.y, z + v.z);
	}
	vec3 operator - (vec3 v) const
	{
		return vec3(x - v.x, y - v.y, z - v.z);
	}
};

inline
vec3 cross(const vec3& a, const vec3& b)
{
	return vec3(a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x);
}

inline
float dot(const vec3& u, const vec3& v) {
	return u.x*v.x + u.y*v.y + u.z*v.z;
}

vec3* GetVert(int i)
{
	return (vec3*)&asset_ocean[i*asset_ocean_vert_size_bytes / sizeof(float)];
}

void GetTri(int i, vec3* v1, vec3* v2, vec3* v3)
{
	*v1 = *GetVert(i * 3);
	*v2 = *GetVert(i * 3 + 1);
	*v3 = *GetVert(i * 3 + 2);
}

inline bool RayIntersectsTriangle(const vec3& x0, const vec3& x, const vec3& v0, const vec3& v1, const vec3& v2, vec3* hit = NULL)
{
	vec3 u = v1 - v0;
	vec3 v = v2 - v0;
	vec3 n = cross(u, v);

	vec3 w0 = x0 - v0;

	float a = -dot(n, w0);
	float b = dot(n, x);

	float ep = 1e-4f;

	if (fabs(b) < ep)
	{
		if (a == 0)			// Ray is parallel
			return false;	// Ray is inside plane
		else
			return false;	// Ray is somewhere else
	}

	float r = a / b;
	if (r < 0)
		return false;		// Ray goes away from the triangle

	vec3 p = x0 + x*r;
	if (hit)
		*hit = p;

	float uu = dot(u, u);
	float uv = dot(u, v);
	float vv = dot(v, v);
	vec3 w = p - v0;
	float wu = dot(w, u);
	float wv = dot(w, v);

	float D = uv * uv - uu * vv;

	float s, t;

	s = (uv * wv - vv * wu) / D;
	if (s < 0 || s > 1)		// Intersection point is outside the triangle
		return false;

	t = (uv * wu - uu * wv) / D;
	if (t < 0 || (s + t) > 1)	// Intersection point is outside the triangle
		return false;

	return true;
}

int main()
{
	int total_tris = asset_ocean_num_verts / 3;

	vec3 down = vec3(0, 0, -1);
	vec3 v1, v2, v3, intersection;

	int heights_min = -50;
	int heights_max = 50;

	FILE* fp = fopen("../my code/heightmap.h", "w");
	fprintf(fp, "#pragma once\n#define HEIGHTS_MIN (%d)\n#define HEIGHTS_MAX (%d)\nfloat heights[] = {", heights_min, heights_max);

	for (int y = heights_min; y <= heights_max; y++)
	{
		fprintf(fp, "\n\t");
		for (int x = heights_min; x <= heights_max; x++)
		{
			bool hit = false;

			for (int k = 0; k < total_tris; k++)
			{
				GetTri(k, &v1, &v2, &v3);
				if (RayIntersectsTriangle(vec3((float)x, (float)y, 100.0f), down, v1, v2, v3, &intersection))
				{
					fprintf(fp, "%ff, ", intersection.z);
					hit = true;
					break;
				}
			}

			if (!hit)
			{
				__debugbreak();
				fprintf(fp, "[---], ");
			}
		}
	}

	fprintf(fp, "\n};\n");

	fclose(fp);

	return 0;
}

