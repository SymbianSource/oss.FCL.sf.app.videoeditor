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


#ifndef DECPARAM_H_
#define DECPARAM_H_

/*-- Project Headers --*/
#include "defines.h"

/*
   Purpose:     The # of filter branches of the polyphase filterbank
                to be discarded. By default 2 lowest and highest branches
        will be discarded.
   Explanation: - */
#define WINDOW_PRUNING_START_IDX 2

/*
   Purpose:     Parameters that control the decoding complexity.
   Explanation: - */
typedef struct Out_ComplexityStr
{
  BOOL no_antialias;    /* If TRUE, no alias-reduction is performed. */
  int16 subband_pairs;  /* # of subband pairs for alias-reduction.   */
  int16 imdct_subbands; /* # of IMDCT subbands.                      */

} Out_Complexity;

/*
   Purpose:     Parameters that control the output stream.
   Explanation: - */
typedef struct Out_ParamStr
{
  int32 sampling_frequency; /* Output sampling frequency.           */
  int16 num_out_channels;   /* # of output channels.                */
  int16 decim_factor;       /* Decimation factor.                   */
  int16 num_samples;        /* # of output samples per subband.     */
  int16 num_out_samples;    /* Total # of output samples per frame. */

  /*
   * Specifies how many window coefficients are discarded from the synthesis
   * window. In units of 'SBLIMIT' samples. The start index can vary
   * between 0 and 'NUM_SUBWIN', but if you don't want to reduce the overall
   * audio quality, the default value 2 (or 0) should be used.
   */
  int16 window_pruning_idx;
  int16 num_subwindows;
  int16 window_offset;

} Out_Param;

/*
   Purpose:     Mapping of above structures into one common structure.
   Explanation: - */
typedef struct Out_InfoStr
{
  Out_Param *out_param;
  Out_Complexity *out_complex;

} Out_Info;

#endif /* DECPARAM_H_ */
