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


/**************************************************************************
  mpheader.h - MPEG-1 and MPEG-2 bit field (header) parsing functions.

  Author(s): Juha Ojanpera
  Copyright (c) 1999-2004 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

#ifndef MPHEADER_H_
#define MPHEADER_H_

/*-- Project Headers. --*/
#include "mpaud.h"
#include "mp3tables.h"
#include "defines.h"

/* Checks whether window switching is used in layer III. */
inline BOOL win_switch(TGranule_Info *gr_info)
{ return ((BOOL)(gr_info->flags & WINDOW_SWITCHING_FLAG)); }

/* Checks whether short blocks are used in layer III. */
inline BOOL short_block(TGranule_Info *gr_info)
{ return ((gr_info->flags & 3) == 2); }

/* Checks whether mixed blocks are present in layer III. */
inline BOOL mixed_block(TGranule_Info *gr_info)
{ return ((BOOL)(gr_info->flags & MIXED_BLOCK_FLAG)); }

/* Checks whether 'scalefac_scale' bit is set in layer III. */
inline BOOL scalefac_scale(TGranule_Info *gr_info)
{ return ((BOOL)((gr_info->flags & SCALEFAC_SCALE) ? 1 : 0)); }

/* Returns the status of 'pre_flag' bit of layer III. */
inline BOOL pre_flag(TGranule_Info *gr_info)
{ return ((BOOL)((gr_info->flags & PRE_FLAG) ? 1 : 0)); }

/*-- Common header parsing functions. --*/

/* MPEG-1 or MPEG-2 */
inline int16 version(TMPEG_Header *mpheader)
{return ((BOOL)((mpheader->header >> 19) & 1));}

/* Checks whether the current stream is of type MPEG-2.5. */
inline BOOL mp25version(TMPEG_Header *mpheader)
{ return ((BOOL)((mpheader->header & 0x100000) ? FALSE : TRUE)); }

/* Layer (1, 2 or 3) */
inline int16 layer_number(TMPEG_Header *mpheader)
{ return (int16)((4 - (mpheader->header >> 17) & 3)); }

/* Checks if error protection is used in the bitstream. */
inline BOOL error_protection(TMPEG_Header *mpheader)
{ return (!((mpheader->header >> 16) & 1)); }

/* Sampling frequency index */
inline int16 sfreq(TMPEG_Header *mpheader)
{ return (int16)((mpheader->header >> 10) & 3);}

/* Checks whether padding bit is set. */
inline BOOL padding(TMPEG_Header *mpheader)
{ return (BOOL)((mpheader->header >> 9) & 1); }
  
/* Checks if private bit is set. */
inline BOOL private_bit(TMPEG_Header *mpheader)
{ return (BOOL)((mpheader->header >> 8) & 1); }
  
/* Mono, stereo, joint or dual */
inline int16 mode(TMPEG_Header *mpheader)
{ return (int16)((mpheader->header >> 6) & 3); }

/* Value of mode extension field */
inline int16 mode_extension(TMPEG_Header *mpheader)
{ return (int16)((mpheader->header >> 4) & 3); }

/* Checks whether copyright bit is set. */
inline BOOL copyright(TMPEG_Header *mpheader)
{ return (BOOL)((mpheader->header >> 3) & 1); }
  
/* Checks whether original bit. */
inline BOOL original(TMPEG_Header *mpheader)
{ return (BOOL)((mpheader->header >> 2) & 1); }

/* Value of bitrate field */
inline int16 bit_rate_idx(TMPEG_Header *mpheader) 
{ return ((int16)((mpheader->header >> 12) & 0xF)); }

/* Bitrate */
inline int16 bit_rate(TMPEG_Header *mpheader)
{ return (bitrate[version(mpheader)][(int16)((mpheader->header >> 12) & 0xF)]); }

/* Number of channels; 1 for mono, 2 for stereo */
inline int16 channels(TMPEG_Header *mpheader)
{ return ((int16)((mode(mpheader) == MPG_MD_MONO) ? (int16)1 : (int16)2));}

inline const int16 *GetBitRateTable(TMPEG_Header *mpheader)
{ return (&bitrate[version(mpheader)][0]); }

/* Sampling frequency */
inline int32 frequency(TMPEG_Header *mpheader)
{ return (s_freq[(mpheader->header & 0x100000) ? version(mpheader):2][sfreq(mpheader)]); }

/* Frame length in milliseconds */
inline int32 GetFrameTime(TMPEG_Header *mpheader)
{ return (framelength[sfreq(mpheader) + (mp25version(mpheader)) * 3]); }

#endif /* MPHEADER_H_ */
