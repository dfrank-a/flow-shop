#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include "metaheuristics.h"
#include "optimizer.h"

void init_sch(u_char * pi);
void shuffle(u_char * pi);
void neighbor(u_char * pi, int indx);
u_int cost(u_char * pi);

void montecarlo()
{	
	srand(time(NULL));

	u_int best = -1;
	job_index * randsch = malloc(jobs_count);
	job_index * bestsch = malloc(jobs_count);
	
	init_sch(randsch);

	int trials = SA_limit;
	while(trials > 0)
	{
		shuffle(randsch);
		u_int ECj = cost(randsch);		

		//Update for better upper bound
		if(ECj < best)
		{
			best = ECj;
			memcpy(bestsch, randsch, jobs_count);
		}

		trials--;
	}

	if(best < upper_bound->ECj)
	{
		set_default(upper_bound);
		int i;
		for(i=0; i < jobs_count; i++)
			mark_job_set(upper_bound,bestsch[i]);

		upper_bound->ECj = best;
	}

	free(randsch);
	free(bestsch);
}

float init_temperature();
float temperature(float T0, int n){return T0*log(2)/log(n+2);}
int accept_prob(u_int pi_cost, u_int pi_cost_prime, float T)
{	
	int delta_e = pi_cost_prime - pi_cost;
	return RAND_MAX * ((delta_e <= 0) ? 1 : exp(-delta_e/T));
}

void simulated_annealing(int inittemp)
{
	srand(time(NULL));

	u_char * pi = malloc(jobs_count);
	u_char * pi_min = malloc(jobs_count);

	u_int fails = 0;
	u_int cur_cost = -1, min_cost = -1; 
	int n = 0;

	float T0 = inittemp;

	init_sch(pi);
	shuffle(pi);

	min_cost = cur_cost = cost(pi);
	
	while(fails < SA_limit)
	{
		float T = temperature(T0, n++);

		int i = rand() % (jobs_count - 1);
		neighbor(pi, i); //Get random neighbor

		u_int new_cost = cost(pi);

		if(new_cost < min_cost) 
		{
			fails = 0;
			min_cost=new_cost;
			pi_min = memcpy(pi_min,pi,jobs_count);
		}
		else fails++;

		if(rand() < accept_prob(cur_cost,new_cost,T))
			cur_cost = new_cost; 	//Commit new state
		else
			neighbor(pi,i);			//Revert state
	}

	if(min_cost < upper_bound->ECj)
	{
		set_default(upper_bound);
		upper_bound->ECj = min_cost;

		int i;
		for(i = 0; i < jobs_count; i++)
		{
			mark_job_set(upper_bound, pi_min[i]);
			calc_times(pi_min[i], &upper_bound->C1, &upper_bound->C2);
		}
	}

	free(pi);
	free(pi_min);
}



void neighbor(u_char * pi, int i)
{
	u_char p = pi[i];
	pi[i] = pi[i+1];
	pi[i+1] = p;
}


void init_sch(u_char * pi)
{
	int i;
	for(i = 0; i < jobs_count; i++)
		pi[i] = i;
}

void shuffle(u_char * pi)
{
	//Create random shuffle
	int i;
	for(i = jobs_count - 1; i >= 0; i--)
	{
		int j = rand() %  (i + 1);
		job_index k = pi[j];
		pi[j] = pi[i];
		pi[i] = k;
	}
}

u_int cost(u_char * pi)
{
	int i;
	op_time C1=0, C2=0;
	op_sum ECj=0;


	for(i = 0; i < jobs_count; i++)
	{
		calc_times(pi[i], &C1, &C2);
		ECj+=C2;
	}

	return ECj;
}

float init_temperature()
{
	u_char * pi = malloc(jobs_count);
	init_sch(pi);

	int r = 0;
	int s = 2 * (jobs_count) * (jobs_count - 1);
	float delta_avg = 0;

	while(r < s)
	{
		shuffle(pi);

		int pi_cost = cost(pi);
		neighbor(pi,rand() % (jobs_count - 1));
		int pi_cost_p = cost(pi);


		int delta_pi = pi_cost_p - pi_cost;
		if(delta_pi >= 0) 
		{
			delta_avg += delta_pi;
			r++;
		}

	}

	free(pi);
	delta_avg /= s; 

	//Return a temperature such that the average worse difference
	//has an acceptance probability of 0.9
	return -delta_avg / log(0.9);

}

u_int init_UB()
{
	u_int best = -1, i;
	for(i = 0; i < 3; i++)
	{
		int ub = cost(sorted_jobs[i]);
		if(ub < best)
			best = ub;
	}

	return best;
}