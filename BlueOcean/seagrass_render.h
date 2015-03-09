
void RenderSeagrass()
{
	int program = 1;

	SetupProgram(program);

	set_color(program, vec3(36, 50, 31) / 255, 2.0f*vec3(183, 255, 158) / 255, vec3(0.3f, 0.3f, 0.3f), 50);

	glUniform1f(g_programs[program].m_uniform_flex_length, 0.85f);

	glBindBuffer(GL_ARRAY_BUFFER, g_blade_vbo);

	glEnableVertexAttribArray(g_programs[program].m_attrib_position);
	glVertexAttribPointer(g_programs[program].m_attrib_position, 3, GL_FLOAT, false, asset_blade_vert_size_bytes, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(g_programs[program].m_attrib_normal);
	glVertexAttribPointer(g_programs[program].m_attrib_normal, 3, GL_FLOAT, false, asset_blade_vert_size_bytes, BUFFER_OFFSET(asset_blade_normal_offset_bytes));

	glEnableVertexAttribArray(g_programs[program].m_attrib_flex);
	glVertexAttribPointer(g_programs[program].m_attrib_flex, 1, GL_FLOAT, false, asset_blade_vert_size_bytes, BUFFER_OFFSET(asset_blade_flex_offset_bytes));

	int size = seagrass_num_patches;

	for (int k = 0; k < size; k++)
	{
		int max = seagrass_patch_indexes[k].start + seagrass_patch_indexes[k].count;
		for (int n = seagrass_patch_indexes[k].start; n < max; n++)
		{
			glUniformMatrix4fv(g_programs[program].m_uniform_model, 1, GL_FALSE, seagrass_patches[n]);
			glDrawArrays(GL_TRIANGLES, 0, asset_blade_num_verts);
		}
	}

	glDisableVertexAttribArray(g_programs[program].m_attrib_flex);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

