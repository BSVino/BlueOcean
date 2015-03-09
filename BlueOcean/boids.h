#pragma once

#define _CRT_SECURE_NO_DEPRECATE
#ifndef EMSCRIPTEN
#include <Windows.h>
#define GLEW_STATIC
#include "..\GL\glew.h"
#endif
#define GL_GLEXT_PROTOTYPES
#include "..\GL\freeglut.h"

#include <math.h>
#include <assert.h>
#include <cmath>
#include <vector>

#include "../math/ArcBall.h"
#include "../math/FrameSaver.h"
#include "../math/Timer.h"
#include "../math/Shapes.h"
#include "../math/mat.h"
#include "../math/vec.h"
#include "../math/InitShaders.h"

class FishSchool
{
	typedef unsigned short fish_t;
	typedef unsigned int cell_t;

public:
	FishSchool();

public:
	virtual void Render();

	struct fish_heap
	{
		int   index;
		float distancesqr;
	};

#define FISH_PER_CELL 20
	// One for the current cell and one for each cardinal direction, total = 4
#define FISH_HEAP_SIZE (FISH_PER_CELL*4)

	void Simulate();
	void FullThink();
	void MoveFish();
	void AddCellToHeap(fish_heap nearest_fish_heap[FISH_HEAP_SIZE], int* heap_size_p, FishSchool::fish_t fish_index, FishSchool::cell_t cell_index);

	double next_full_think;

	struct Boid
	{
		vec3       position;
		vec3       velocity;
		cell_t     cell;
	};

	std::vector<Boid> fish;

	static void MakeAllTheFish();
	static FishSchool* s_school;

	struct BoidCell
	{
		fish_t         fish_inside[FISH_PER_CELL];
		unsigned char  num_fish;
		char           buf; // Alignment
	};

	std::vector<BoidCell> fish_cells;
	int cell_grid_size_x; // How many cells in the x dimension
	int cell_grid_size_y; // How many cells in the y dimension
	int cell_grid_size_z; // How many cells in the z dimension
	vec3 cell_size;

	vec3 GetCellGridCenter(); // The center of the entire grid
	vec3 GetCellGridSize();   // The AABB half-size of the entire grid
	vec3 GetCellSize();       // The AABB half-size of a single cell

	cell_t GetCell(vec3 position, vec3* celloffset = NULL);
	cell_t GetCell(int cell_x, int cell_y, int cell_z);
	void GetCell(cell_t cell_index, int* cell_x, int* cell_y, int* cell_z);
	bool CellValid(cell_t cell_index);
	bool PositionInCellGrid(vec3 position);
	void PutAllBoidsInCells();
};

