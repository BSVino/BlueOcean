#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <math.h>
#include <vector>

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

#include "../BlueOcean/maths.h"

#include "../BlueOcean/heightmap.h"
#include "../BlueOcean/heightmap_procs.h"

using namespace std;

int main()
{
	srand(0);

	SimplexNoise n1(0);
	SimplexNoise n2(1);

	FILE* fp = fopen("../../my code/seagrass.h", "w");
	fprintf(fp, "#pragma once\n\n");
	fprintf(fp, "extern Angel::mat4 seagrass_patches[]; \n");
	fprintf(fp, "extern Angel::mat4 seagrass_patches[];\n");
	fprintf(fp, "struct seagrass_patch { int start; int count; };\n");
	fprintf(fp, "extern struct seagrass_patch seagrass_patch_indexes[];\n");
	fprintf(fp, "extern int seagrass_num_patches;\n\n");
	fclose(fp);

	fp = fopen("../../my code/seagrass.cpp", "w");

#define PATCH_DENSITY 10
	float grass[PATCH_DENSITY][PATCH_DENSITY];

	int patch_dimension_x = 60; // How many patches are we making along each dimension?
	int patch_dimension_y = 25;

	fprintf(fp, "#define _CRT_SECURE_NO_DEPRECATE\n");
	fprintf(fp, "#ifndef EMSCRIPTEN\n");
	fprintf(fp, "#define GLEW_STATIC\n");
	fprintf(fp, "#include \"..\\GL\\glew.h\"\n");
	fprintf(fp, "#endif\n");
	fprintf(fp, "#include <string>\n");
	fprintf(fp, "#include <iostream>\n");
	fprintf(fp, "#include \"../CS174a template/mat.h\"\n");

	fprintf(fp, "using namespace Angel;\n");

	fprintf(fp, "#include \"seagrass.h\"\n");
	fprintf(fp, "mat4 seagrass_patches[] = {\n");

	vector<int> patch_indexes;
	int count = 0;

	vec3 center(-5, 0, 0);

	for (int j = 0; j < patch_dimension_y; j++)
	{
		for (int i = 0; i < patch_dimension_x; i++)
		{
			for (size_t y = 0; y < PATCH_DENSITY; y++)
			{
				for (size_t x = 0; x < PATCH_DENSITY; x++)
				{
					grass[x][y] = n1.Noise(i + x*0.02f, j + y*0.02f) * 5;
					grass[x][y] += n2.Noise(i + x*0.04f, j + y*0.04f) * 2;
				}
			}

			vec3 patchTR;
			patchTR.x = RemapValClamped((float)i, 0, (float)patch_dimension_x - 1, -(float)patch_dimension_x / 2, (float)patch_dimension_x / 2 - 1);
			patchTR.y = RemapValClamped((float)j, 0, (float)patch_dimension_y - 1, -(float)patch_dimension_y / 2, (float)patch_dimension_y / 2 - 1);
			patchTR = patchTR + center;

			bool grass_exists = false;
			int first = count;
			int total = 0;

			fprintf(fp, "\t//Patch %d %d\n", i, j);
			for (size_t y = 0; y < PATCH_DENSITY; y++)
			{
				for (size_t x = 0; x < PATCH_DENSITY; x++)
				{
					float probability = RemapValClamped(grass[x][y], 0.5f, 1.5f, 0, RAND_MAX);
					if (rand() < probability)
					{
						vec3 position;
						position.x = RemapValClamped((float)x, 0, PATCH_DENSITY - 1, 0, 1 - (1 / (float)PATCH_DENSITY));
						position.y = RemapValClamped((float)y, 0, PATCH_DENSITY - 1, 0, 1 - (1 / (float)PATCH_DENSITY));

						position.x += (float)rand() / RAND_MAX;
						position.y += (float)rand() / RAND_MAX;

						position = position + patchTR;

						position.z = GetSeaBedHeight(position.x, position.y);
						fprintf(fp, "\tmat4(1, 0, 0, %ff, 0, 1, 0, %ff, 0, 0, 1, %ff, 0, 0, 0, 1),\n", position.x, position.y, position.z);

						grass_exists = true;
						count++;
						total++;
					}
				}
			}

			if (grass_exists)
			{
				patch_indexes.push_back(first);
				patch_indexes.push_back(total);
			}
		}
	}
	fprintf(fp, "};\n\n");

	fprintf(fp, "seagrass_patch seagrass_patch_indexes[] = {\n\t");
	for (size_t k = 0; k < patch_indexes.size(); k += 2)
		fprintf(fp, "{%d, %d}, %s", patch_indexes[k], patch_indexes[k+1], ((k+2)%20==0)?"\n\t":"");
	fprintf(fp, "\n};\n\n");
	fprintf(fp, "int seagrass_num_patches = %d;\n\n", patch_indexes.size()/2);

	fclose(fp);
}

