/*
* Copyright (c) 2010 Ixonos Plc.
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - Initial contribution
*
* Contributors:
* Ixonos Plc
*
* Description:  
* Internal header for the video demultiplexer module.
*
*/



#ifndef _VDXINT_H_
#define _VDXINT_H_

/*
 * Defines
 */

#ifndef vdxAssert
   #define vdxAssert(exp) assert(exp);
#endif


/*
 * Structs and typedefs
 */

/* type for VLC (variable length code) lookup tables */
typedef struct {
   int val; /* value for code, for example an index of the corresponding table
               in the H.263 recommendation */
   u_char len; /* actual length of code in bits */
} vdxVLCTable_t;

typedef struct{
    int16 code;
    int16 length;
}tVLCTable;


/*
 * Functions defined in viddemux.c (and used from viddemux_mpeg.c)
 */

/* Macroblock Layer */

int vdxGetCBPY(bibBuffer_t *inBuffer, int *index, 
   int *bitErrorIndication);

int vdxGetMCBPCInter(
   bibBuffer_t *inBuffer, 
   int fPLUSPTYPE,
   int fFourMVsPossible,
   int fFirstMBOfPicture,
   int *index, 
   int *bitErrorIndication);

int vdxGetMCBPCIntra(bibBuffer_t *inBuffer, int *index, 
   int *bitErrorIndication);

int vdxGetMVD(bibBuffer_t *inBuffer, int *mvdx10, 
   int *bitErrorIndication);

int vdxUpdateQuant(
   bibBuffer_t *inBuffer, 
   int fMQ,
   int quant,
   int *newQuant,
   int *bitErrorIndication);


/*
 * Functions defined in viddemux_mpeg.c (and used from viddemux.c)
 */

   int vdxGetScaledMVD(bibBuffer_t *inBuffer, int f_code, int *mvd10,
      int *bitErrorIndication);

#endif
// End of File
