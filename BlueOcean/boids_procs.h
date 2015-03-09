#define NOMINMAX

#include "boids.h"

#include <cmath>

#include <algorithm>

using namespace std;

inline int tmin(int a, int b)
{
	return a < b ? a : b;
}

FishSchool* FishSchool::s_school = 0;

static vec3 debug_position;
static vec3 debug_orientation;

extern vec3 BoidSchoolCurve(vec3 school_position, vec3 school_orientation, float t, float scatter);

extern double g_simulation_time;
extern double g_dt;
extern vec3 g_player_position;
extern GLuint g_butterfly_vbo;

void DrawVBO(GLuint vbo, mat4 transform, GLsizei verts, GLsizei stride, int position_offset, int texcoord_offset, int normal_offset, int tangent_offset, int bitangent_offset);

FishSchool::FishSchool()
{
	next_full_think = 0;
}

bool fish_heap_compare(const FishSchool::fish_heap& l, const FishSchool::fish_heap& r)
{
	return l.distancesqr > r.distancesqr;
}

void FishSchool::AddCellToHeap(fish_heap nearest_fish_heap[FISH_HEAP_SIZE], int* heap_size_p, FishSchool::fish_t fish_index, FishSchool::cell_t cell_index)
{
	if (!CellValid(cell_index))
		return;

	BoidCell* cell = &fish_cells[cell_index];
	int num_fish = (int)cell->num_fish;
	Boid* this_fish = &fish[fish_index];
	vec3 fish_position = this_fish->position;

	int heap_size = *heap_size_p;
	for (int k = 0; k < num_fish; k++)
	{
		fish_t other = cell->fish_inside[k];

		if (other == fish_index)
			continue;

		nearest_fish_heap[heap_size].index = other;
		nearest_fish_heap[heap_size].distancesqr = lengthsqr(fish[other].position - fish_position);
		heap_size++;
		push_heap(&nearest_fish_heap[0], &nearest_fish_heap[heap_size], fish_heap_compare);
	}
	*heap_size_p = heap_size;
}

float separation_min_distance = 0.4f;
float separation_max_distance = 0.8f;
float avoid_strength = 2.5f;
float collision_avoidance_factor = 0.5f;
float velocity_match_factor = 0.4f;
float position_match_factor = 0.3f;
float fish_initial_velocity = 1;
float max_velocity = 6;
float max_neighborhood = 20;
float fish_drag = 0.75f;
float fish_brake = 0.25f;

float player_escape_weight = .4f;
float player_hate_distance = 4;
float player_okay_distance = 7;

#define FULL_THINK_DT (1.0f/24)

void FishSchool::Simulate()
{
	while (g_simulation_time > next_full_think)
		FullThink();

	MoveFish();
}

void FishSchool::FullThink()
{
	float dt = FULL_THINK_DT;
	next_full_think += dt;

	fish_heap nearest_fish_heap[FISH_HEAP_SIZE];
	int heap_size = 0;

	vec3 cell_size = GetCellSize();

	float fish_separation_min_distance = separation_min_distance;
	float fish_separation_max_distance = separation_max_distance;
	float fish_max_neighborhood_sqr = max_neighborhood*max_neighborhood;
	float fish_avoid_strength = avoid_strength;
	float fish_collision_avoidance_factor = collision_avoidance_factor;
	float fish_velocity_match_factor = velocity_match_factor;
	float fish_position_match_factor = position_match_factor;
	float all_factors_weight = 1 / (fish_collision_avoidance_factor+fish_velocity_match_factor+fish_position_match_factor);

	float fish_max_velocity = max_velocity;
	float fish_max_velocity_sqr = fish_max_velocity*fish_max_velocity;
	float fish_cruise_velocity = fish_initial_velocity;
	float fish_cruise_velocity_sqr = fish_cruise_velocity*fish_cruise_velocity;
	float drag_amount = pow(fish_drag, dt);
	float brake_amount = pow(fish_brake, dt);

	float fish_player_hate_distance = player_hate_distance;
	float fish_player_hate_distance_sqr = fish_player_hate_distance*fish_player_hate_distance;

	float fish_player_okay_distance = player_okay_distance;
	float fish_player_okay_distance_sqr = fish_player_okay_distance*fish_player_okay_distance;

	float fish_player_escape_weight = player_escape_weight;

	vec3 player_position = g_player_position;

	size_t fish_size = fish.size();

	for (size_t i = 0; i < fish_size; i++)
	{
		Boid* this_fish = &fish[i];

		cell_t old_cell_index = this_fish->cell;

		vec3 this_fish_position = this_fish->position;
		vec3 cell_offset;

		cell_t cell_index = GetCell(this_fish->position, &cell_offset);

		if (old_cell_index != cell_index)
		{
			if (CellValid(old_cell_index))
			{
				BoidCell* old_cell = &fish_cells[old_cell_index];
				if (old_cell->num_fish == 1)
					old_cell->num_fish = 0;
				else if (old_cell->num_fish)
				{
					for (int k = 0; k < FISH_PER_CELL; k++)
					{
						if (old_cell->fish_inside[k] == i)
						{
							std::swap(old_cell->fish_inside[k], old_cell->fish_inside[old_cell->num_fish - 1]);
							old_cell->num_fish--;
							break;
						}
					}

					// If we didn't find ourselves then the cell was full when we were added, no problem.
				}
			}

			if (CellValid(cell_index))
			{
				BoidCell* new_cell = &fish_cells[cell_index];
				if (new_cell->num_fish < FISH_PER_CELL - 1)
					new_cell->fish_inside[new_cell->num_fish++] = i;

				this_fish->cell = cell_index;
			}
		}

		if (!PositionInCellGrid(this_fish_position))
			continue;

		heap_size = 0;

		// Build a heap of the closest few fish.
		int cell_x, cell_y, cell_z;
		GetCell(cell_index, &cell_x, &cell_y, &cell_z);

		// If we're in the lower half of the cell, look downwards. If we're in the upper half, look upwards.
		int next_cell_x = (int)(round(cell_offset.x / cell_size.x) * 2 - 1);
		int next_cell_y = (int)(round(cell_offset.y / cell_size.y) * 2 - 1);
		int next_cell_z = (int)(round(cell_offset.z / cell_size.z) * 2 - 1);

		vec3 avoid_collision(0, 0, 0);
		vec3 other_fish_velocities(0, 0, 0);
		vec3 other_fish_positions(0, 0, 0);
		float total_velocities_weight = 0;
		float total_positions_weight = 0;

		cell_t search_cells[11];
		search_cells[0] = cell_index;
		search_cells[1] = GetCell(cell_x + next_cell_x, cell_y, cell_z);
		search_cells[2] = GetCell(cell_x, cell_y + next_cell_y, cell_z);
		search_cells[3] = GetCell(cell_x, cell_y, cell_z + next_cell_z);
		search_cells[4] = GetCell(cell_x + next_cell_x, cell_y + next_cell_y, cell_z);
		search_cells[5] = GetCell(cell_x, cell_y + next_cell_y, cell_z + next_cell_z);
		search_cells[6] = GetCell(cell_x + next_cell_x, cell_y, cell_z + next_cell_z);
		search_cells[7] = GetCell(cell_x + next_cell_x, cell_y + next_cell_x, cell_z + next_cell_z);
		search_cells[8] = GetCell(cell_x - next_cell_x, cell_y, cell_z);
		search_cells[9] = GetCell(cell_x, cell_y - next_cell_y, cell_z);
		search_cells[10] = GetCell(cell_x, cell_y, cell_z - next_cell_z);

		// We don't need the 100 strictly nearest other boids for this test,
		// so just search around for 100 nearish fish.
		int max_fish = 100;
		for (int k = 0; k < 11; k++)
		{
			if (search_cells[k] > fish_cells.size())
				continue;

			BoidCell* cell = &fish_cells[search_cells[k]];
			unsigned char num_fish = cell->num_fish;
			for (int j = 0; j < num_fish; j++)
			{
				fish_t other_index = cell->fish_inside[j];

				if (other_index == i)
					continue;

				Boid* other = &fish[other_index];

				other_fish_positions += other->position;
				total_positions_weight += 1;
			}

			if (num_fish > max_fish)
				continue;
		}

		AddCellToHeap(nearest_fish_heap, &heap_size, i, cell_index);

		AddCellToHeap(nearest_fish_heap, &heap_size, i, GetCell(cell_x + next_cell_x, cell_y, cell_z));
		AddCellToHeap(nearest_fish_heap, &heap_size, i, GetCell(cell_x, cell_y + next_cell_y, cell_z));
		AddCellToHeap(nearest_fish_heap, &heap_size, i, GetCell(cell_x, cell_y, cell_z + next_cell_z));

		vec3 this_fish_velocity = this_fish->velocity;

		int loop_size = tmin(5, heap_size);
		for (int k = 0; k < loop_size; k++)
		{
			if (nearest_fish_heap[heap_size - 1].distancesqr > fish_max_neighborhood_sqr)
				break;

			pop_heap(&nearest_fish_heap[0], &nearest_fish_heap[heap_size], fish_heap_compare);
			heap_size -= 1;

			fish_t other_index = nearest_fish_heap[heap_size].index;
			Boid* other = &fish[other_index];
			vec3 other_position = other->position;

			vec3 vector_to_other = other_position - this_fish_position;
			float distance_to_other = length(vector_to_other);

			// Avoid him if he's too close.
			float avoid_ramp = RemapValClamped(distance_to_other, fish_separation_min_distance, fish_separation_max_distance, fish_avoid_strength, 0.0f);
			if (distance_to_other > 0)
				avoid_collision -= vector_to_other / distance_to_other * avoid_ramp;

			// Remember his velocity to match it later
			float distance_ramp = exp(-distance_to_other);
			distance_ramp *= dot(other->velocity, this_fish_velocity) * 0.5f + 0.5f; // Ignore fish going completely the other way
			other_fish_velocities += other->velocity * distance_ramp;
			total_velocities_weight += distance_ramp;
		}

		if (total_velocities_weight)
			other_fish_velocities /= total_velocities_weight;

		vec3 other_positions_velocity(0, 0, 0);
		if (total_positions_weight)
		{
			other_fish_positions /= total_positions_weight;
			other_positions_velocity = normalize(other_fish_positions - this_fish_position);
		}

		vec3 flocking_velocity = avoid_collision * fish_collision_avoidance_factor +
			other_fish_velocities * fish_velocity_match_factor +
			other_positions_velocity * fish_position_match_factor;
		flocking_velocity *= all_factors_weight;

		this_fish_velocity += flocking_velocity * dt;

		// We hate the player and we will run away from the player fastly.
		vec3 player_to_fish = this_fish_position - player_position;
		float player_to_fish_length_sqr = lengthsqr(player_to_fish);
		if (player_to_fish_length_sqr < fish_player_hate_distance_sqr)
		{
			vec3 escape_vector = normalize(player_to_fish);

			if (lengthsqr(other_fish_positions))
				escape_vector = LerpValue(escape_vector, normalize(other_positions_velocity), fish_player_escape_weight);

			this_fish_velocity += normalize(escape_vector) * fish_max_velocity * dt;
		}

		// Danger has passed, we can slow down and rejoin the flock
		if (player_to_fish_length_sqr > fish_player_okay_distance_sqr)
			this_fish_velocity *= brake_amount;

		this_fish->velocity = this_fish_velocity;
	}
}

void FishSchool::MoveFish()
{
	float dt = (float)g_dt;

	float fish_max_velocity = max_velocity;
	float fish_max_velocity_sqr = fish_max_velocity*fish_max_velocity;
	float drag_amount = pow(fish_drag, dt);

	size_t fish_size = fish.size();

	for (size_t i = 0; i < fish_size; i++)
	{
		Boid* this_fish = &fish[i];

		this_fish->position += this_fish->velocity * dt;

		vec3 this_fish_position = this_fish->position;

		if (!PositionInCellGrid(this_fish_position))
		{
			// Steer us back to the playing field.
			this_fish->velocity += normalize(this_fish->position-GetCellGridCenter()) * -dt;
			float length_sqr = lengthsqr(this_fish->velocity);
			if (length_sqr > 1)
				this_fish->velocity /= sqrt(length_sqr);
			continue;
		}

		if (this_fish_position.z < GetSeaBedHeight(this_fish_position.x, this_fish_position.y) + 0.8f)
		{
			if (this_fish_position.z < GetSeaBedHeight(this_fish_position.x, this_fish_position.y) + 0.6f)
			{
				// Steer clear of the sea bed.
				this_fish->velocity += vec3(0, 0, 1) * dt;
				float length_sqr = lengthsqr(this_fish->velocity);
				if (length_sqr > 1)
					this_fish->velocity /= sqrt(length_sqr);
				else
				{
					float length_2d = sqrt(this_fish->velocity.x*this_fish->velocity.x + this_fish->velocity.y*this_fish->velocity.y);
					if (length_2d < 0.5f)
					{
						this_fish->velocity.x *= 0.5f / length_2d;
						this_fish->velocity.y *= 0.5f / length_2d;
					}
				}
				continue;
			}
			else
			{
				// Steer level to the sea bed.
				float updot = dot(this_fish->velocity, vec3(0, 0, 1));
				if (updot < 0)
				{
					float velocity = length(this_fish->velocity);
					if (velocity < 0.5f)
						velocity = 0.5f;

					this_fish->velocity -= vec3(0, 0, 1) * updot * dt;
					this_fish->velocity = normalize(this_fish->velocity) * velocity;
				}
				continue;
			}
		}

		vec3 this_fish_velocity = this_fish->velocity;

		if (lengthsqr(this_fish_velocity) > fish_max_velocity_sqr)
			this_fish_velocity = normalize(this_fish_velocity)*fish_max_velocity;

		this_fish_velocity *= drag_amount;

		if (lengthsqr(this_fish_velocity) < 0.5*0.5)
			this_fish_velocity = normalize(this_fish_velocity)*0.5;

		this_fish->velocity = this_fish_velocity;
	}
}

void FishSchool::Render()
{
	int program = 0;

	glUseProgram(g_programs[program].m_program);

	glBindBuffer(GL_ARRAY_BUFFER, g_butterfly_vbo);

	glEnableVertexAttribArray(g_programs[program].m_attrib_position);
	glVertexAttribPointer(g_programs[program].m_attrib_position, 3, GL_FLOAT, false, asset_butterfly_vert_size_bytes, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(g_programs[program].m_attrib_normal);
	glVertexAttribPointer(g_programs[program].m_attrib_normal, 3, GL_FLOAT, false, asset_butterfly_vert_size_bytes, BUFFER_OFFSET(asset_butterfly_normal_offset_bytes));

	set_color(program, vec3(0.1f, 0.1f, 0.1f), 1.5f*vec3(255, 200, 0) / 255, vec3(1.5f, 1.5f, 1.5f), 30);

	mat4 transform;
	vec4 world_up(0, 0, 1, 0);

	for (size_t i = 0; i < fish.size(); i++)
	{
		auto* this_fish = &fish[i];

		vec3 forward = normalize(this_fish->velocity);
		vec3 right = -normalize(cross(forward, world_up));
		vec3 up = cross(forward, right);

		transform = mat4(vec4(forward, 0.0f), vec4(right, 0.0f), vec4(up, 0.0f), this_fish->position);

		glUniformMatrix4fv(g_programs[0].m_uniform_model, 1, GL_FALSE, transform);

		glDrawArrays(GL_TRIANGLES, 0, asset_butterfly_num_verts);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

float procession_frequency = 5;
float procession_length = 8;
float procession_scatter_frequency = 15;

// A nice pretty curvy line to spawn fish along
vec3 BoidSchoolCurve(vec3 school_position, vec3 school_orientation, float t, float scatter)
{
	vec3 right = cross(school_orientation, vec3(0, 0, 1));
	vec3 up = cross(right, school_orientation);

	float t_freq = t * procession_frequency;

	vec3 base_curve = school_position + school_orientation * t * procession_length + right * cos(t_freq) + up * sin(t_freq);

	float t_scatter_freq = t * procession_scatter_frequency;

	vec3 this_fish = right * scatter * RandomFloat(-1, 1) * cos(t_scatter_freq) + up * scatter * RandomFloat(-1, 1) * sin(t_scatter_freq);

	return base_curve + this_fish;
}

int fish_schools = 10;

void FishSchool::MakeAllTheFish()
{
	if (s_school)
		delete s_school;

	FishSchool* school = s_school = new FishSchool();

	school->next_full_think = g_simulation_time;

	int schools = fish_schools;
#ifdef _DEBUG
	int fish_per_school = 70;
#else
	int fish_per_school = 100;
#endif

	school->fish.clear();
	school->fish.reserve(schools*fish_per_school);

	vec3 school_spawn_positions[] = {
		vec3(15, 5, -5),
		vec3(15, -5, -5),
		vec3(5, -5, -5),
		vec3(-0, 5, -5),
		vec3(-5, -5, -3),
		vec3(-10, 5, -5),
		vec3(-15, -5, -7),
		vec3(-20, 5, -10),
		vec3(-25, -5, -10),
		vec3(-30, 0, -10),
	};

	for (int k = 0; k < schools; k++)
	{
		vec3 school_position = school_spawn_positions[k % (sizeof(school_spawn_positions)/sizeof(vec3))];

		vec3 school_direction = normalize(vec3(RandomFloat(-100, 100), RandomFloat(-100, 100), RandomFloat(-20, 20)));
		float school_velocity = fish_initial_velocity;

		for (int i = 0; i < fish_per_school; i++)
		{
			school->fish.push_back(Boid());
			Boid* b = &school->fish.back();
			b->position = BoidSchoolCurve(school_position, school_direction, (float)i / 100, 1);
			vec3 orientation = BoidSchoolCurve(school_position, school_direction, (float)(i + 1) / 100, 0) - BoidSchoolCurve(school_position, school_direction, (float)i / 100, 0);
			b->velocity = normalize(orientation) * school_velocity;
		}
	}

	school->cell_grid_size_x = 70;
	school->cell_grid_size_y = 25;
	school->cell_grid_size_z = 16;
	school->cell_size = school->GetCellSize();
	school->fish_cells.resize(school->cell_grid_size_x*school->cell_grid_size_y*school->cell_grid_size_z);
	school->PutAllBoidsInCells();

	school->next_full_think = g_simulation_time;
}

vec3 FishSchool::GetCellGridCenter()
{
	return vec3(-15, 0, -3);
}

vec3 FishSchool::GetCellGridSize()
{
	return vec3(70, 25, 16);
}

vec3 FishSchool::GetCellSize()
{
	vec3 grid_size = GetCellGridSize();
	return vec3(grid_size.x / cell_grid_size_x, grid_size.y / cell_grid_size_y, grid_size.z / cell_grid_size_z);
}

float frealmod(float v1, float v2)
{
	float r = fmod(v1, v2);
	if (r < 0)
		return v2 + r;
	else
		return r;
}

FishSchool::cell_t FishSchool::GetCell(vec3 position, vec3* celloffset)
{
	vec3 grid_max = GetCellGridCenter() + GetCellGridSize() / 2;
	vec3 grid_min = GetCellGridCenter() - GetCellGridSize() / 2;

	position -= GetCellGridCenter();

	int x = (int)RemapValClamped(position.x, grid_min.x, grid_max.x, 0.0f, (float)cell_grid_size_x);
	int y = (int)RemapValClamped(position.y, grid_min.y, grid_max.y, 0.0f, (float)cell_grid_size_y);
	int z = (int)RemapValClamped(position.z, grid_min.z, grid_max.z, 0.0f, (float)cell_grid_size_z);

	x = tmin(x, cell_grid_size_x - 1);
	y = tmin(y, cell_grid_size_y - 1);
	z = tmin(z, cell_grid_size_z - 1);

	if (celloffset)
	{
		vec3 grid_cell_size = cell_size;
		*celloffset = vec3(frealmod(position.x, grid_cell_size.x), frealmod(position.y, grid_cell_size.y), frealmod(position.z, grid_cell_size.z));
	}

	return GetCell(x, y, z);
}

FishSchool::cell_t FishSchool::GetCell(int cell_x, int cell_y, int cell_z)
{
	// Avoid memory problems
	cell_x %= cell_grid_size_x;
	cell_y %= cell_grid_size_y;
	cell_z %= cell_grid_size_z;

	return cell_z*cell_grid_size_x*cell_grid_size_y + cell_y*cell_grid_size_x + cell_x;
}

void FishSchool::GetCell(cell_t cell_index, int* cell_x, int* cell_y, int* cell_z)
{
	*cell_x = cell_index%cell_grid_size_x;
	*cell_y = (cell_index / cell_grid_size_x) % cell_grid_size_y;
	*cell_z = (cell_index / (cell_grid_size_x*cell_grid_size_y)) % cell_grid_size_z;
}

bool FishSchool::CellValid(cell_t cell_index)
{
	if (cell_index < 0 || cell_index >= (unsigned)cell_grid_size_x*cell_grid_size_y*cell_grid_size_z)
		return false;

	return true;
}

bool FishSchool::PositionInCellGrid(vec3 position)
{
	vec3 grid_hi = GetCellGridCenter() + GetCellGridSize()/2;
	vec3 grid_lo = GetCellGridCenter() - GetCellGridSize()/2;
	if (position.x < grid_lo.x)
		return false;
	if (position.y < grid_lo.y)
		return false;
	if (position.z < grid_lo.z)
		return false;

	if (position.x > grid_hi.x)
		return false;
	if (position.y > grid_hi.y)
		return false;
	if (position.z > grid_hi.z)
		return false;

	return true;
}

void FishSchool::PutAllBoidsInCells()
{
	memset(&fish_cells[0], 0, sizeof(BoidCell)*fish_cells.size());

	size_t num_fish = fish.size();
	for (size_t i = 0; i < num_fish; i++)
	{
		Boid* this_fish = &fish[i];
		cell_t cell_index = GetCell(this_fish->position);
		BoidCell* cell = &fish_cells[cell_index];

		if (cell->num_fish < FISH_PER_CELL - 1)
			cell->fish_inside[cell->num_fish++] = i;
		//else
		//	TMsg("Too many fish!\n");

		this_fish->cell = cell_index;
	}
}
