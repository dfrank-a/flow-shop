#ifndef __FS_JOB_H__
#define __FS_JOB_H__

typedef unsigned char bool;

typedef unsigned char u_char;
typedef u_char job_index;

typedef unsigned int u_int;
typedef u_int op_time;

typedef struct job
{
	op_time TA; 
	op_time TB; 
	op_time TTot;
} job;

typedef struct interrupt
{
	op_time Start;
	op_time Length; 
	op_time End;

} interrupt;

interrupt breakdown;

u_char jobs_count;
job * jobs_collection;

enum {SPT1, SPT2, SPT, JOHN};	//For dereference of sorted_jobs' first index	
u_char * sorted_jobs[4];		//jobs sorted by SPT1, SPT2, SPT, Johnson's rule

bool init_sorts();
void free_sorts();
void jobs_XML();

#endif