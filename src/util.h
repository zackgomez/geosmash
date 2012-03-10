#pragma once

void *file_contents(const char *filename, GLint *length);

// returns a float in the range [min, max]
float random_float(float min, float max);
