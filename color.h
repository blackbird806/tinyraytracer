#pragma once
#include "geometry.h"

using color = vec4f;

namespace Color
{
	const color black = { 0.0f, 0.0f, 0.0f, 1.0f };
	const color none = { 0.0f, 0.0f, 0.0f, 0.0f };
	const color red = { 1.0f, 0, 0, 1.0f };
	const color blue = { 0, 0, 1.0f, 1.0f };
	const color green = { 0, 1.0f, 0, 1.0f };
	const color yellow = { 1.0f, 1.0f, 0, 1.0f };
	const color orange = { 1.0f, 0.0f, 1.0, 1.0f };
	const color white = { 1.0f, 1.0f, 1.0f, 1.0f };
}