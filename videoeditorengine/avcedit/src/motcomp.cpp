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
*
*/


#ifdef CHECK_MV_RANGE
#include <stdio.h>
#endif

#include "globals.h"
#include "motcomp.h"
#include "framebuffer.h"

#if defined(USE_CLIPBUF) && !defined(AVC_MOTION_COMP_ASM)
#include "clipbuf.h"
#endif


#define ONEFOURTH1  20
#define ONEFOURTH2  -5
#define ONEFOURTH3  1


static const int numModeMotVecs[MOT_NUM_MODES+1] = {
  0, 1, 2, 2, 4, 8, 8, 16
};

static const int numModeMotVecs8x8[4] = {
  1, 2, 2, 4
};

/*
static const int blockShapes[][2] = {
  {4, 4},
  {4, 2},
  {2, 4},
  {2, 2}
};

static const int blockShapes8x8[][2] = {
  {2, 2},
  {2, 1},
  {1, 2},
  {1, 1}
};
*/


#ifdef CHECK_MV_RANGE
extern int maxVerticalMvRange;
#endif


/*
 *
 * mcpGetNumMotVecs:
 *
 * Parameters:
 *      mode                  MB partition type (16x16, 8x16, 16x8, 8x8)
 *      subMbTypes            Sub-MB partition types (8x8, 4x8, 8x4, 4x4)
 *
 * Function:
 *      Get the number of motion vectors that need to be decoded for given
 *      MB type and Sub-MB partition types.
 *      
 * Returns:
 *      Number of motion vectors.
 *
 */
int mcpGetNumMotVecs(int interMode, int subMbTypes[4])
{
  int numVecs;

  if (interMode < MOT_8x8)
    numVecs = numModeMotVecs[interMode];
  else {
    numVecs  = numModeMotVecs8x8[subMbTypes[0]];
    numVecs += numModeMotVecs8x8[subMbTypes[1]];
    numVecs += numModeMotVecs8x8[subMbTypes[2]];
    numVecs += numModeMotVecs8x8[subMbTypes[3]];
  }

  return numVecs;
}


