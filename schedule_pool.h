#ifndef __SCHEDULE_POOL_H__
#define __SCHEDULE_POOL_H__

#include <stdlib.h>
#include <stdio.h>
#include "schedule.h"

bool init_pool();
void free_pool();
size_t get_schedule_size();								

bool add_sch(schedule *,job_index); //Add schedule to stack(DFS)/queue(BFS)
bool get_nxt(schedule*);	    //Get schedule from stack(DFS)/queue(BFS)

#endif
