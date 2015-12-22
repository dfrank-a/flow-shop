#include <stdio.h>
#include <time.h>
#include <signal.h>
#include "optimizer.h"
#include "dominance.h"
#include "schedule_pool.h"
#include "metaheuristics.h"

unsigned long visited, overflows;

u_int seed, job_dist;
float int_pos;
int SA_reps;
int inittemp;
clock_t stt;

bool spawnD(schedule * pi); //spawn with dominance
bool spawnN(schedule * pi); //spawn without dominance

void print_details();
void print_finish();

enum{N=1,SEED,DIST,INTP,SALIM,SAREPS,ITEMP,TLIM};

float timeouthours;
bool timeoutflag;

void settimeoutflag(int arg){ timeoutflag=1; }

bool optimize()
{	
	visited = 0; overflows = 0;

	int rep;
	for(rep = 0; rep < SA_reps; rep++)
		simulated_annealing(inittemp);

	print_details();

	schedule * next = malloc(schedule_size);

	/* SET TIMER HERE */	
	timer_t watchdog;

	timer_create(CLOCK_REALTIME, NULL, &watchdog);
	struct itimerspec timeout, old;

	/* Expire in 24 hours */
	timeout.it_value.tv_sec = timeouthours * 60 * 60;
	timeout.it_value.tv_nsec = 0;
	timeout.it_interval.tv_sec = 0;
	timeout.it_interval.tv_nsec = 0;

	timer_settime(watchdog, 0, &timeout, &old);
	signal(SIGALRM, settimeoutflag);

	timeoutflag = 0;
	while(get_nxt(next) && !timeoutflag)
	{ 
		op_sum next_LB = calc_lower_bound(next);
		op_sum next_UB = -1;
		int best_spt = -1;
		
		if(next_LB < upper_bound->ECj)
		{
			calc_upper_bound(next, &next_UB, &best_spt);

			if(next_UB < upper_bound->ECj)
				update(next, next_UB, best_spt);

			if(next_UB > next_LB) spawnD(next);
		}

		//Count # of nodes visited, track overflows
		if((++visited) == 0) overflows++;
	}

	timer_delete(watchdog);

	free(next);
	print_finish();
	return 1;
}

bool spawnD(schedule * pi)
{
	calc_dominance(pi);

	int j;
	for(j = jobs_count-1; j >= 0; --j)
	{
		if(spawnable(j))
			if(!add_sch(pi,j))
				return 0;		
	}

	return 1;
}

bool spawnN(schedule * pi)
{
	int j;
	for(j = jobs_count-1; j >= 0; --j)
	{
		if(job_unset(pi,j))
			if(!add_sch(pi,j))
				return 0;		
	}

	return 1;
}

bool init_jobs();
bool init_opt(int argc, char ** argv)
{
	stt = clock();
	jobs_count = atoi(argv[N]);
	seed = atoi(argv[SEED]);
	job_dist = atoi(argv[DIST]);
	int_pos = atof(argv[INTP]);

	SA_limit = atoi(argv[SALIM]);
	SA_reps = atoi(argv[SAREPS]);
	inittemp = atoi(argv[ITEMP]);
	timeouthours = atof(argv[TLIM]);

	/* Initialize jobs from argument list */
	bool success = init_jobs(argc,argv);
	if(!success) 
		return 0;

	/* Calculate schedule size */
	if(success) 
		schedule_size = get_schedule_size();	

	/* Sort job arrays, initialize memory pool */
	success = success && init_sorts() && init_pool();

	/* Initialize spawn if using dominance*/
	success = success && init_spawn();

	success = success && (upper_bound = malloc(schedule_size));

	if(success)
	{
		set_default(upper_bound);			 		//Initialize upper_bound sum value = -1
		upper_bound->ECj = -1;				//Set UB Sum to -1 (maximum unsigned int value)
	}

	return success;
}

bool init_jobs()
{	
	if(jobs_count < 2) return 0; 
	if(job_dist < 1) return 0;

	srand(seed);

	//Initialize jobs
	jobs_collection = malloc(sizeof(job) * jobs_count);
	int i, sumA = 0;
	for(i = 0; i < jobs_count; i++)
	{
		int Aj = (rand() % job_dist) + 1;
		int Bj = (rand() % job_dist) + 1;
		sumA += Aj;
		if(Aj == 0 || Bj == 0)
			return 0;

		jobs_collection[i].TA = Aj;
		jobs_collection[i].TB = Bj;
		jobs_collection[i].TTot = Aj + Bj;
	}

	//Initialize breakdown
	breakdown.Start = sumA * int_pos;
	breakdown.Length = sumA / jobs_count;
	breakdown.End = breakdown.Start + breakdown.Length;

	return 1;
}

void free_opt()
{
	free_spawn();
	free_sorts();
	free_pool();
	free(jobs_collection);
	free(upper_bound);
}

void print_details()
{
	/* Print Root Upper Bound */
	float SA_time = 1.0 * (clock() - stt) / CLOCKS_PER_SEC;
	printf("%d %d %0.3f ", init_UB(), upper_bound->ECj, SA_time);
	fflush(stdout);
}

void print_finish()
{	
	float opt_time = 1.0 * (clock() - stt) / CLOCKS_PER_SEC;
	printf("%lu %lu %d %0.3f %d\n", visited, overflows, upper_bound->ECj, opt_time, timeoutflag);
}