#include <stdlib.h>
#include <stdio.h>
#include "job.h"

void sort_jobs(u_char * job_is, int leftarg, int rightarg, int comp);
bool init_sorts()
{
	//Allocate all sorts
	u_char j;
	for(j=0; j<4; j++)
		if(!(sorted_jobs[j] = malloc(jobs_count)))
			return 0;

	//Sort jobs in SPT1, SPT2, SPT orders
	for(j = 0; j < 3; j++)
	{
		job_index J;
		for(J = 0; J < jobs_count; J++) sorted_jobs[j][J]=J;
		sort_jobs(sorted_jobs[j],0,jobs_count - 1, j);
	}

	//Populate simpson's rule sorted array
	u_char x = 0, a;
	for(a = 0; a < jobs_count; a++) //All Ai <= Bi ascending SPT1
	{
		job_index i = sorted_jobs[SPT1][a];
		job J = jobs_collection[i];

		if(J.TA <= J.TB)
		{
			sorted_jobs[JOHN][x] = i;
			++x;
		}

	}

	for(a = 1; a <= jobs_count; a++) //All Ai > Bi descending SPT2
	{
		job_index i = sorted_jobs[SPT2][jobs_count - a];
		job J = jobs_collection[i];

		if(J.TA > J.TB)
		{
			sorted_jobs[JOHN][x] = i;
			++x;
		}

	}

	return 1;
}

void free_sorts()
{
	u_char j;
	for(j=0; j<4; j++)
		free(sorted_jobs[j]);
}

bool less(job_index, job*, int);
bool gret(job_index, job*, int);
void sort_jobs(u_char * job_is, int leftarg, int rightarg, int comp)
{
	int left = leftarg, right = rightarg;

	job * mid = &jobs_collection[job_is[(left + right) >> 1]];
	while(left <= right)
	{
		while(less(job_is[left],mid,comp)) 
			left++;
		while(gret(job_is[right],mid,comp)) 
			right--;

		if(left <= right)
		{
			u_char J = job_is[right];
			job_is[right] = job_is[left];
			job_is[left] = J;

			left++;
			right--;
		}
	}

	if(leftarg < right)
		sort_jobs(job_is, leftarg, right, comp);
	if(left < rightarg)
		sort_jobs(job_is, left, rightarg, comp);
}

bool less(job_index j1, job* j2, int comp)
{
	//Tie breaker is next node (modular)
	int tie = (comp + 1) % 3;

	//
	u_int * J1 = (u_int *) &jobs_collection[j1];
	u_int * J2 = (u_int *) j2;
	
	return (J1[comp] < J2[comp] || 
		(J1[comp] == J2[comp] && J1[tie] < J2[tie]));
}

bool gret(job_index j1, job* j2, int comp)
{
	int tie = (comp + 1) % 3;
	u_int * J1 = (u_int *) &jobs_collection[j1];
	u_int * J2 = (u_int *) j2;
	
	return (J1[comp] > J2[comp] || 
		(J1[comp] == J2[comp] && J1[tie] > J2[tie]));
}

void jobs_XML()
{
	int i;
	printf("\t<jobs>\n");
	for(i = 0; i < jobs_count; i++)
	{
		job j = jobs_collection[i];
		printf("\t\t<job id=\"%d\" A=\"%d\" B=\"%d\" T=\"%d\" />\n",i, j.TA, j.TB, j.TTot);
	}
	printf("\t</jobs>\n");

	int r;
	printf("\t<sorts>\n");
	for(r = 0; r < 3; r++)
	{
		printf("\t\t<sort type=\"%d\">\n",r);

		for(i = 0; i < jobs_count; i++)
		{
			int s = sorted_jobs[r][i];
			job j = jobs_collection[s];
			printf("\t\t\t<job id=\"%d\" A=\"%d\" B=\"%d\" T=\"%d\" />\n", s, j.TA, j.TB, j.TTot);
		}
		printf("\t\t</sort>\n");
	}
	printf("\t</sorts>\n");
	fflush(stdout);
}