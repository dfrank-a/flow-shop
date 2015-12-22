#ifndef __FS_OPTIMIZER_H__
#define __FS_OPTIMIZER_H__

#include "schedule.h"
#include "job.h"

schedule * upper_bound;

bool init_opt(int argc, char ** argv);
void free_opt();
bool optimize();


#endif