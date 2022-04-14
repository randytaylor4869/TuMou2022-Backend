#pragma once
#define _CRT_SECURE_NO_WARNINGS

struct Coordinate
{
	int x, y, z;
	Coordinate() { x = 0, y = 0, z = 0; }
	Coordinate(int a, int b, int c) { x = a, y = b, z = c; }

	bool operator==(Coordinate t)
	{
		if (this->x == t.x && this->y == t.y && this->z == t.z)
		{
			return true;
		}
		return false;
	}
};