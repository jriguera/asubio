

/*
** adpcm.h - include file for adpcm coder.
**
** Version 1.0, 7-Jul-92.
*/

#ifndef _ADPCM_H_
#define _ADPCM_H_


struct adpcm_state
{
    short valprev;	/* Previous output value */
    char  index;       	/* Index into stepsize table */
};

typedef struct adpcm_state Adpcm_State;




void adpcm_coder (short [], char [], int, struct adpcm_state *);
void adpcm_decoder (char [], short [], int, struct adpcm_state *);


#endif


