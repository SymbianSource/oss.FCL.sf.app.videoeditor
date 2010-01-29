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
  l3sfb.cpp - Scalefactor band (sfb) implementations for layer III.

  Author(s): Juha Ojanpera
  Copyright (c) 1999-2004 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

/*-- Project Headers. --*/
#include "mp3tables.h"
#include "mpaud.h"
#include "mpheader.h"

/**************************************************************************
  External Objects Needed
  *************************************************************************/

/**************************************************************************
  Internal Objects
  *************************************************************************/

static const int16 *III_sfbOffsetLong(TMPEG_Header *header);

static const int16 *III_sfbOffsetShort(TMPEG_Header *header);

static const int16 *III_sfbWidthTblShort(int16 *sfb_offset, int16 *sfb_width);

/**************************************************************************
  Title        : III_SfbDataInit

  Purpose      : Initializes the sfb parameters.

  Usage        : III_SfbDataInit(sfbData, header)

  Input        : sfbData - sfb parameters
                 header  - mp3 header parameters

  Author(s)    : Juha Ojanpera
  *************************************************************************/

void
III_SfbDataInit(CIII_SfbData *sfbData, TMPEG_Header *header)
{
  COPY_MEMORY(sfbData->sfbOffsetLong, III_sfbOffsetLong(header),
              (MAX_LONG_SFB_BANDS + 1) * sizeof(int16));

  sfbData->sfbLong = sfbData->sfbOffsetLong;

  COPY_MEMORY(sfbData->sfbOffsetShort, III_sfbOffsetShort(header),
              (MAX_SHORT_SFB_BANDS + 1) * sizeof(int16));

  sfbData->sfbShort = sfbData->sfbOffsetShort;

  COPY_MEMORY(sfbData->sfbWidthShort, 
              III_sfbWidthTblShort(sfbData->sfbShort, sfbData->sfbWidthShort),
              (MAX_SHORT_SFB_BANDS + 1) * sizeof(int16));

  sfbData->sfbWidth = sfbData->sfbWidthShort;

  sfbData->bandLimit = MAX_MONO_SAMPLES;
}

/**************************************************************************
  Title        : III_BandLimit

  Purpose      : Saves the bandlimit value for Huffman decoding.

  Usage        : III_BandLimit(sfbData, decim_factor)

  Input        : sfbData  - sfb parameters
                 binLimit - number of spectral bins to be decoded from each channel

  Author(s)    : Juha Ojanpera
  *************************************************************************/

void
III_BandLimit(CIII_SfbData *sfbData, uint16 binLimit)
{
  sfbData->bandLimit = binLimit;
}


/**************************************************************************
  Title        : III_sfbOffsetLong

  Purpose      : Obtains the scalefactor table for long blocks.

  Usage        : y = III_sfbOffsetLong(header)

  Input        : header - mp3 header parameters

  Output       : y - scalefactor boundaries for long blocks

  Author(s)    : Juha Ojanpera
  *************************************************************************/

const int16 *
III_sfbOffsetLong(TMPEG_Header *header)
{
  int16 fidx, mp25idx = (int16) (6 * mp25version(header));
  
  fidx = (int16) (mp25idx + 3 * version(header) + sfreq(header));

  return (&sfBandIndex[fidx].l[0]);
}


/**************************************************************************
  Title        : III_sfbOffsetShort

  Purpose      : Obtains the scalefactor table for short blocks.

  Usage        : y = III_sfbOffsetShort(header)

  Input        : header - mp3 header parameters

  Output       : y - scalefactor boundaries for short blocks

  Author(s)    : Juha Ojanpera
  *************************************************************************/

const int16 *
III_sfbOffsetShort(TMPEG_Header *header)
{
  int16 mp25idx = (int16) (6 * mp25version(header));

  return (&sfBandIndex[mp25idx + 3 * version(header) + sfreq(header)].s[0]);
}


/**************************************************************************
  Title        : III_sfbWidthTblShort

  Purpose      : Obtains the scalefactor width table for short blocks.

  Usage        : y = III_sfbWidthTblShort(sfb_offset, sfb_width)

  Input        : sfb_offset - offset table of short blocks
                 sfb_width  - address of buffer to receive the sfb widths

  Output       : y - sfb width table for short blocks

  Author(s)    : Juha Ojanpera
  *************************************************************************/

const int16 *
III_sfbWidthTblShort(int16 *sfb_offset, int16 *sfb_width)
{
  for(int16 i = 0; i < MAX_SHORT_SFB_BANDS; i++)
    sfb_width[i] = (int16) (sfb_offset[i + 1] - sfb_offset[i]);

  return (sfb_width);
}
