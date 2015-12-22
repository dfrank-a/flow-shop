#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "optimizer.h"

int main(int argc, char ** argv)
{
	/* Initialize optimizer with options from arg list */
	if(init_opt(argc,argv))
	{
		optimize();

		/* End time measurement */
		free_opt();
	}
	return 0;
}


