#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include "dominance.h"

u_char * spawn_list;					//for elimination by dominance rules
schedule * pi_john;						//Temporary schedule for work in rule 3
job_index * pi_ord;						//Temporary space for transposed schedule in rules 3,5

void Dominance_1(schedule *);
void Dominance_2(schedule *);
bool Dominance_3(schedule *);
void Dominance_4(schedule *);
bool Dominance_5(schedule *);

bool init_spawn()
{
	bool success = (spawn_list = malloc(jobs_count)) != NULL;
	success = success && (pi_john = malloc(schedule_size)) != NULL;
	success = success && (pi_ord = malloc(jobs_count)) != NULL;

	return (spawn_list = malloc(jobs_count)) != NULL;
}

void free_spawn()
{
	free(spawn_list);
	free(pi_john);
	free(pi_ord);
}

bool spawnable(job_index j)
{
	return spawn_list[j];
}

void calc_dominance(schedule * pi)
{
	/*
	If dominance rule 3 or 5 return true--no spawning occurs
	Dominance 1 2 and 4 eliminate individual branches from list
	*/

	if(Dominance_3(pi) || Dominance_5(pi))
	{
		memset(spawn_list, 0, jobs_count);
	}
	else
	{
		//initialize spawn list with unfixed jobs marked true
		int j; for(j = 0; j < jobs_count; j++)
			spawn_list[j] = job_unset(pi,j);

		Dominance_1(pi);
		Dominance_2(pi);
		Dominance_4(pi);
	}
}

void Dominance_1(schedule * pi)
{
	/*  J1 dominates J2 if A(J1) <= A(J2) && B(J1)=B(J2) && J1 != J2 
			
		Iterates through jobs in SPT2 order: 
		This places jobs satisfying B(J1) == B(J2) adjacent and ensures that 
		A(J1) <= A(J2) when B(J1) == B(J2)
	*/

	u_int i;
	for(i = 0; i < jobs_count; i++)
	{
		//Get SPT2 job at index i
		job_index J1 = sorted_jobs[SPT2][i]; 

		if(job_unset(pi,J1)) //Skip item if it is already scheduled
		{
			//Eliminate all J2 with B(J1) == B(J2) and A(J1) <= A(J2)
			//This loop increments i to skip past all dominated jobs
			while(i < jobs_count - 1)
			{
				job_index J2 = sorted_jobs[SPT2][i + 1];
				if(jobs_collection[J1].TB == jobs_collection[J2].TB)
				{
					//printf("\t<dominance rule=\"1\" kill=\"%d\" />\n",J2);
					spawn_list[J2] = 0; //J2 is dominated, remove it from spawn list
					++i;				//Advance i
				}
				else
					break;
			}
		}
	}
}

void Dominance_2(schedule * pi)
{
	/*
		Implements Dominance Rule 3:
		For all i,j not in pi && i != j
		Ai <= Bi 
		Ai <= Aj && Bi <= Bj 				(job i has smallest A,B of all unscheduled jobs)
		s < C1(pi) || C1(pi + i + j) < s 	(Breakdown does not occur for any 2 unscheduled jobs)
	*/

	const job_index NA = -1;
	if(pi->Level >= jobs_count - 1) return;	//There must be at least 2 jobs left to schedule

	//Find smallest A, largest A in unscheduled jobs
	op_time min_A = -1, min_B = -1, max_A = 0;
	job_index Ji = NA;
	
	job_index x;
	for(x = 0; x < jobs_count; x++)
	if(job_unset(pi,x))
	{	
		job * jobx = jobs_collection + x;

		//Find smallest job A's value
		if(jobx->TA < min_A )
			min_A = jobx->TA;

		//Record smallest job B's value
		if(jobx->TB < min_B)
			min_B = jobx->TB;

		//Record largest job A's value
		if(jobx->TA > max_A)
			max_A = jobx->TA;

		//if jobx is minimum in A and B, set Ji
		if(jobx->TA == min_A && jobx->TB == min_B)
			Ji = x;
		else
			Ji = NA;
	}

	//Check that a Ji such that Ai <= Aj && Bi <= Bj && Ai <= Bi exists
	if(Ji != NA && min_A <= min_B) 				
	{
		//Make sure that breakdown has already happened or will not happen
		//if the smallest A and largest A are scheduled 
		if(break_occurred(pi->C1) || !break_occurred(pi->C1 + min_A + max_A))
		{
			//printf("\t<dominance rule=\"2\" keep=\"%d\" />\n",Ji);
			//All jobs except for Ji are dominated
			for(x = 0; x < jobs_count; x++)
				if(x != Ji)
					spawn_list[x] = 0;	
		}
	}
}

bool Dominance_3(schedule * pi)
{
	//Use blank copy of schedule
	set_default(pi_john);

	//If schedule has experienced breakdown, 
	//Schedule all jobs in same order as pi
	//Until breakdown occurs
	if(break_occurred(pi->C1))
	{
		//Get schedule in ordered form
		transpose(pi, pi_ord);

		int x;
		for(x = 0; x < jobs_count; x++)
		{
			mark_job_set(pi_john, pi_ord[x]);
			calc_times(pi_ord[x], &pi_john->C1, &pi_john->C2);
			pi_john->ECj += pi_john->C2;

			if(break_occurred(pi_john->C1)) break;
		}
	}

	//Add all remaining jobs in johnson's rule order 
	//(Ascending by SPT1 for all Ai <= Bi, Descending by SPT2 for all Ai > Bi)
	int x;
	for(x = 0; x < jobs_count; x++)
	{
		//Get next job ordered by johnson's rule
		job_index J = sorted_jobs[JOHN][x];

		if(job_unset(pi_john,J) && !job_unset(pi,J))
		{
			mark_job_set(pi_john, J);
			calc_times(J, &pi_john->C1, &pi_john->C2);
			pi_john->ECj += pi_john->C2;
		}
	}

	bool dominated = pi_john->ECj < pi->ECj;

	//if(dominated)
		//printf("\t<dominance rule=\"3\" />\n");

	return dominated;
}

void Dominance_4(schedule * pi)
{
	/*
		Implements Dominance rule 4:
		
		pi + j is dominated if 

		C1(pi) > s (breakdown has already occurred)
		Ai <= Aj && Bi >= Bj 

		NOTE: Cases where Ai <= Aj && Bi = Bj are covered more generally by 
		Dominance rule 1, the implementation of this function will only 
		dominate schedules with Bi > Bj to prevent collisions between these rules

	*/

	//Check that breakdown has occurred
	if(break_occurred(pi->C1))
	{
		int x;
		for(x = 0; x < jobs_count; x++)
		{
			//Get index of next unscheduled SPT1 job 
			job_index i = sorted_jobs[SPT1][x];

			if(job_unset(pi, i))
			{
				//Calculate C2(PI+i) (don't change pi's values)
				op_time C1_i = pi->C1;
				op_time C2_i = pi->C2;
				calc_times(i, &C1_i, &C2_i);

				int y; for(y = x + 1; y < jobs_count; y++)
				{
					job_index j = sorted_jobs[SPT1][y];

					if(job_unset(pi, j) && spawn_list[j])  				//skip fixed/dominated jobs 
					if(jobs_collection[i].TB > jobs_collection[j].TB)	//SPT1 -> Ai <= Aj, check Bi > Bj
					{
						//Calculate C2(PI+J2) (don't change pi's values)
						op_time C1_j = pi->C1;
						op_time C2_j = pi->C2;
						calc_times(j, &C1_j, &C2_j);

						//If C2(pi + i) <= C2(pi + j), pi + j is dominated
						if(C2_i <= C2_j) 
						{
							//printf("\t<dominance rule=\"4\" kill=\"%d\" />\n", j);
							spawn_list[j] = 0;
						}

					}
				}
			}
		}
	}
}

bool Dominance_5(schedule * pi)
{
	if(pi->Level < 2) return 0;
	bool dominated;

	//Get schedule in position order
	job_index * pi_swap = pi_ord;
	transpose(pi,pi_swap);

	//Get index of last fixed job in schedule
	job_index last_fixed = pi_swap[pi->Level-1];

	int insert_pos; //Insert last fixed job into position insert_pos;
	for(insert_pos = 0; insert_pos < pi->Level - 1; insert_pos++)
	{
		dominated = 0;

		//Set time variables = 0;
		op_time C1 = 0, C2 = 0;
		op_sum ECj = 0;

		//add jobs up to insertion point
		int x = 0;
		while(x < insert_pos)
		{
			calc_times(pi_swap[x], &C1, &C2);
			ECj += C2;
			++x;
		}
		//Insert last job before insertion point
		calc_times(last_fixed, &C1, &C2);
		ECj += C2;
		//Schedule remaining jobs after insertion point
		while(x < pi->Level - 1)
		{
			calc_times(pi_swap[x], &C1, &C2);
			ECj += C2;
			++x;
		}

		if(ECj < pi->ECj)
		{
			if(break_occurred(C1))
				dominated = (C2 - pi->C2) * (jobs_count - pi->Level) < (pi->ECj - ECj);
			else
				dominated = C2 < pi->C2;
		}

		if(dominated) break;
	}

	//if(dominated) printf("\t<dominance rule=\"5\" />\n");
	return dominated;
}