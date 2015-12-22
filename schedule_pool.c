
#include <stddef.h>
#include "schedule_pool.h"

void * stack_start;
void * stack_top;
void * stack_end;

bool init_pool()
{
	//For DFS, stack needs space for N * (N+1) / 2 elements
	size_t ssize = ((jobs_count) * (jobs_count + 1) / 2) * schedule_size;
	stack_start = malloc(ssize);

	//Place default in first position
	set_default(stack_start);
	stack_top = stack_start + schedule_size;

	stack_end = stack_start + ssize;
	return stack_start != NULL;
}

void free_pool()
{
	free(stack_start);
}

bool add_sch(schedule * s, job_index j)
{
	if(stack_top < stack_end)
	{
		copy_schedule(s,stack_top);
		schedule * top = stack_top;
		mark_job_set(top, j);
		calc_times(j, &top->C1, &top->C2);
		top->ECj += top->C2;

		stack_top += schedule_size;
		return 1;
	}
	else
		return 0;
}

bool get_nxt(schedule * pi)
{
	if(stack_top <= stack_start)
		return 0;
	
	schedule * top = (stack_top -= schedule_size);
	copy_schedule(top, pi);
	return 1;
	
}
