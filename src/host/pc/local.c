# include "dgd.h"

/*
 * NAME:	P->getevent()
 * DESCRIPTION:	(don't) get the next event
 */
void P_getevent(void)
{
}

/*
 * NAME:	P->srandom()
 * DESCRIPTION:	set random seed
 */
void P_srandom(long seed)
{
    srand((unsigned int) seed);
}

/*
 * NAME:	P->random()
 * DESCRIPTION:	get random number
 */
long P_random(void)
{
    return (long) rand();
}
