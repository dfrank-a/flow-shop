#ifndef __SCHEDULE_H__
#define __SCHEDULE_H__

#include <stddef.h>
#include "job.h"
typedef unsigned int op_sum;

typedef struct schedule
{	
		op_time	C1;				/* Time last fixed job part A completed */
		op_time	C2;				/* Time last fixed job part B completed */
		op_sum	ECj;			/* Sum of fixed job completion times 	*/
		op_sum 	Lower_Bound;	/* Value calculated by lower bound 		*/
		u_char	Level;			/* Number of jobs fixed (tree level) 	*/	
		
		/* FIXED SCHEDULE LIST
			Index =  Job index, 
			Value =  Position (-1 = unscheduled)	

			Ex: (5 jobs, 1,4,2 are scheduled)
				Value: -1  0  2  -1  1
				Index:  0  1  2   3  4
		*/
		u_char  Fixed_Schedule[0];	
} schedule;

/*
	Schedules are to be placed in memory such that enough space is left 
	between them to accommodate the size of the Fixed_Schedule array
		get_schedule_size finds the appropriate size, additionalluy accounting 
		for any padding necessary between instances of schedule

	For job j: Fixed_Schedule[j] = -1 if unscheduled
	if the job is scheduled, its position is the value of Fixed_Schedule[j]
	(0 represents the first scheduled job)	
		No extra space needed for boolean array to check if scheduled, 
		lookups in constant time

	The transpose function produces an array where i = position, 
		a[i] = scheduled job index (or -1 if not scheduled) in linear time
*/

size_t schedule_size;

void set_default(schedule*);							//Set all values to 0, Fixed_schedule array all -1
void copy_schedule(const schedule* s, schedule* d); 	//make copy of s to d
void transpose(const schedule*, job_index[]);			//Get array of jobs in schedule order
void print(const schedule*);							//Print fixed schedule 

void mark_job_set(schedule*, job_index);				//Mark job index as set in pi, increment Level
void calc_job_A(op_time* C1, op_time A);				//Add A_time to C1_pi, accounting for breakdown
void calc_job_B(op_time* C2, op_time C1, op_time B);	//Calculate start time of next Job B and add B time
void calc_times(job_index, op_time* C1, op_time* C2);	//Calculate C1 and C2 after scheduling job j

bool job_unset(const schedule * pi, job_index);			//TRUE if job_idx is not set in pi

bool break_occurred(op_time C1);						//TRUE if breakdown occurred before C1_pi
op_sum calc_lower_bound(schedule*);						//Calculate all Lower Bounds
op_sum upper_bound_SPT(schedule*, int spt);				//Return upper bound based on SPT rule
void calc_upper_bound(schedule * pi, op_sum * ECj, int * best_spt);
void update(schedule * pi, int ECj, int best_spt);

void schedule_XML(schedule *, int UB);
void update_XML(int best_SPT);

#endif