#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "schedule.h"
#include "optimizer.h"

void lower_bound_1(schedule *);
void lower_bound_2(schedule *);
void lower_bound_3(schedule *);

size_t get_schedule_size()
{
	//Get size of schedule without Fixed_Schedule
	int ss = offsetof(schedule, Fixed_Schedule);
	ss += jobs_count;

	//Get number of aligned boundaries in total size
	int st = ss / __alignof__(schedule);

	//Add one padding chunk for alignment of remaining bytes if needed
	st += ((ss % __alignof__(schedule)) > 0);

	//Get total bytes
	st *= __alignof__(schedule);

	return st;
}

void set_default(schedule * pi)
{	
	//Set all defaults (including fixed schedule + its overflow)
	pi->C1 = 0;
	pi->C2 = 0;
	pi->ECj = 0;
	pi->Lower_Bound = 0;
	pi->Level = 0;
	memset(pi->Fixed_Schedule,-1,jobs_count);
}

void copy_schedule(const schedule * src, schedule * dest)
{	
	dest->C1 = src->C1;
	dest->C2 = src->C2;
	dest->ECj = src->ECj;
	dest->Lower_Bound = src->Lower_Bound;
	dest->Level = src->Level; 
	memcpy(dest->Fixed_Schedule,src->Fixed_Schedule, jobs_count); 
}

bool job_unset(const schedule * pi, u_char job_idx)
{
	return pi->Fixed_Schedule[job_idx] == (u_char)-1;
}

void mark_job_set(schedule * pi, u_char job_idx)
{
		pi->Fixed_Schedule[job_idx] = pi->Level++;
}

bool break_occurred(u_int C1)
{
	return breakdown.Start < C1;
}

void calc_job_A(u_int * C1_pi, u_int A_time)
{
	/* Get C1(Pi+A)*/

	//Check whether breakdown already occurred
	bool brk = break_occurred(*C1_pi);

	//Add next job operation to A
	*C1_pi += A_time;

	//no breakdown before + breakdown after -> Breakdown occurred during this job
	if(!brk && break_occurred(*C1_pi))
		*C1_pi += breakdown.Length;
}

void calc_job_B(u_int * C2_pi, u_int C1_pi, u_int B_time)
{
	/* Get C2(pi+b)*/
	if(*C2_pi < C1_pi) 
		*C2_pi = C1_pi;

	*C2_pi += B_time;
}

void calc_times(u_char job_idx, u_int * C1_pi, u_int * C2_pi)
{
	/* Return 
		C1_pi = C1(pi+A1)  
		C2_pi = Max(C1(pi+Ai),C2(pi)) + Bi
	*/

	job J = jobs_collection[job_idx];
	calc_job_A(C1_pi, J.TA);
	calc_job_B(C2_pi, *C1_pi, J.TB);
}

void transpose(const schedule * pi, u_char * dest)
{
	/*	
		Convert from array[job index] = position in schedule
		to array[position in schedule] = job index 
	*/

	memset(dest,-1,jobs_count);
	
	int i; for(i = 0; i < jobs_count; i++)
	{
		if(pi->Fixed_Schedule[i] != (u_char)-1)
			dest[pi->Fixed_Schedule[i]]= i;
	}
}

void print(const schedule * pi)
{
	u_char * ordered_jobs = malloc(jobs_count);
	transpose(pi,ordered_jobs);

	u_int i; 
	for(i=0; i< jobs_count ;i++)
		if(ordered_jobs[i] != (u_char) -1)
		{
			if(i>0) printf(" ");
			printf("%d",(int)ordered_jobs[i]);
		}
	free(ordered_jobs);
}

op_sum calc_lower_bound(schedule * pi)
{
	u_int UB = upper_bound->ECj;

	//Check if inherited lower bound
	//is already less than UB
	if(pi->Lower_Bound < UB) 
	{
		//Calculate lower bounds, stop if able to cut
		lower_bound_1(pi);
		if(pi->Lower_Bound < UB)
		{
			lower_bound_2(pi);

			#ifndef NO_LB3
			if(pi->Lower_Bound < UB)
			lower_bound_3(pi);
			#endif
		}
	}
	return pi->Lower_Bound;
}

void lower_bound_1(schedule * pi)
{
	u_int C1_pi = pi->C1;
	u_int ECj = pi->ECj;

	u_int j, J;
	for(j=0; j < jobs_count; j++)
	{
		//Get j'th job sorted by machine A 
		J = sorted_jobs[SPT1][j]; 

		//Add job B w/o regard for 
		//conflicts on machine B
		if(job_unset(pi,J))
		{
			job job = jobs_collection[J];
			calc_job_A(&C1_pi, job.TA);
			ECj += (C1_pi + job.TB);
		}
	}

	if(pi->Lower_Bound < ECj)
		pi->Lower_Bound = ECj;
}

void lower_bound_2(schedule * pi)
{
	u_int smallest_A = 0;

	//Get smallest unscheduled SPT1 job
	u_int j, J; for(j = 0; j < jobs_count; j++)
	{
		J = sorted_jobs[SPT1][j];
		if(job_unset(pi, J))
			smallest_A = jobs_collection[J].TA;

		if(smallest_A) break;
	}

	u_int C1_pi = pi->C1;
	u_int C2_pi = pi->C2;
	u_int ECj = pi->ECj;

	//Find next B time when A above is scheduled
	calc_job_A(&C1_pi, smallest_A);
	calc_job_B(&C2_pi, C1_pi, 0);

	//Disregard Machine A, schedule Machine B
	for(j=0; j < jobs_count; j++)
	{
		//Get j'th job sorted by Machine B 
		J = sorted_jobs[SPT2][j]; 

		//Add job B w/o regard for 
		//conflicts on machine B
		if(job_unset(pi,J))
		{	
			C2_pi += jobs_collection[J].TB;
			ECj+= C2_pi;
		}
	}

	if(pi->Lower_Bound < ECj)
		pi->Lower_Bound = ECj;
}

void lower_bound_3(schedule * pi)
{
	u_int ECj = 0, C1_pi = 0, C2_pi = 0;

	//True if breakdown already occurred, 
	//If breakdown hasn't occurred,
	//It will be scheduled in the proceeding loop
	bool brk = break_occurred(pi->C1);

	u_int j;
	for(j = 0; j < jobs_count; j++)
	{
		//Go through jobs in SPT order
		u_char J = sorted_jobs[SPT][j];

		if(job_unset(pi, J))
		{
			//place breakdown duration on machine A in SPT order, 
			//with breakdown first
			if(brk && jobs_collection[J].TTot > breakdown.Length)
			{
				brk = 0;	//Set brk to false so it is not set twice
				C1_pi += breakdown.Length;
				ECj += C1_pi;
			}

			//Calculate C1 without accounting for breakdown 
			C1_pi+= jobs_collection[J].TA;
			//Calculate C2 and increment ECj
			calc_job_B(&C2_pi, C1_pi, jobs_collection[J].TB);
			ECj += C2_pi;
		}
	}

	ECj = pi->ECj + (ECj / 2) + (pi->C1 * (jobs_count - pi->Level));

	if(break_occurred(pi->C1)) 
		ECj -= breakdown.Length;

	if(pi->Lower_Bound < ECj)
		pi->Lower_Bound = ECj;
}

u_int upper_bound_SPT(schedule * pi, int spt_rule)
{
	u_int ECj = pi->ECj;
	u_int C1 = pi->C1;
	u_int C2 = pi->C2;

	u_char j, J; for(j = 0; j < jobs_count; j++)
	{
		J = sorted_jobs[spt_rule][j];
		if(job_unset(pi, J))
		{	
			calc_times(J, &C1, &C2);
			ECj += C2;
		}
	}

	return ECj;
}

void calc_upper_bound(schedule * pi, op_sum * best_ECj, int * best_spt)
{
	*best_ECj = -1;
	//Calculate each upper bound
	int rule; 
	for(rule=SPT1; rule<SPT; rule++)
	{
		u_int UBn = upper_bound_SPT(pi,rule);

		if(UBn < *best_ECj)
		{
			*best_ECj = UBn;
			*best_spt = rule;
		}
	}
}

void update(schedule * pi, int ECj, int best_spt)
{
	//Update upper bound
	copy_schedule(pi, upper_bound);

	u_int j; for(j = 0; j < jobs_count; j++)
	{
		u_int J = sorted_jobs[best_spt][j];
		if(job_unset(pi,J))
		{
			mark_job_set(upper_bound,J);
			calc_times(J, &upper_bound->C1, &upper_bound->C2);
		}
	}

	upper_bound->Level = pi->Level;
	upper_bound->ECj = ECj;
}

void schedule_XML(schedule * pi, int UB)
{
	printf("\t\t<pi UB=\"%d\" LB=\"%d\" >",UB, pi->Lower_Bound);
	print(pi);
	printf("</pi>\n");

}

void update_XML(int best_SPT)
{
	printf("\t\t\t\t<update UB=\"%d\" SPT=\"%d\" >",upper_bound->ECj, best_SPT);
	print(upper_bound);
	printf("</update>\n");
}