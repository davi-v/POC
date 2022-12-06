#include "Pch.hpp"
#include "CGALStuff.hpp"

bool FaceInfo2::in_domain()
{
	return nesting_level % 2 == 1;
}