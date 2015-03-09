
float GetSeaBedHeight(float x, float y)
{
	int range = HEIGHTS_MAX - HEIGHTS_MIN;
	float x2 = RemapValClamped(x, HEIGHTS_MIN, HEIGHTS_MAX, 0, (float)range);
	float y2 = RemapValClamped(y, HEIGHTS_MIN, HEIGHTS_MAX, 0, (float)range);

	int x_index = (int)x2;
	int y_index = (int)y2;
	int X_index = x_index+1;
	int Y_index = y_index+1;

	int y_column = y_index * (range+1);
	int Y_column = Y_index * (range+1);

	float height_xy = heights[y_column + x_index];
	float height_xY = heights[Y_column + x_index];
	float height_Xy = heights[y_column + X_index];
	float height_XY = heights[Y_column + X_index];

	float ramp = ((x2 - (float)x_index) / ((float)(X_index - x_index)));
	float height_y = ramp * (height_Xy - height_xy) + height_xy;
	float height_Y = ramp * (height_XY - height_xY) + height_xY;

	return RemapVal(y2, (float)y_index, (float)Y_index, height_y, height_Y);
}

