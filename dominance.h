#ifndef __FS_DOMINANCE_H__
#define __FS_DOMINANCE_H__
#include "optimizer.h"

void calc_dominance();
bool spawnable(job_index);

bool init_spawn();
void free_spawn();

#endif