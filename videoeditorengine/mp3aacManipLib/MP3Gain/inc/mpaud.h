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
  mpaud.h - Interface for the mp3 decoder core.

  Author(s): Juha Ojanpera
  Copyright (c) 1999-2004 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

#ifndef MP_AUD_H_
#define MP_AUD_H_

/**************************************************************************
  External Objects Needed
  *************************************************************************/

/*-- Project Headers --*/
#include "nok_bits.h"
#include "defines.h"
#include "auddef.h"
#include "mpif.h"
#include "Mp3API.h"

/**************************************************************************
  External Objects Provided
  *************************************************************************/

/**************************************************************************
                Common structure definitions for all layers
  *************************************************************************/

/*
   Purpose:     Parent Structure for mp1, mp2, and mp3 frames.
   Explanation: - */
class CMPAudDec;
class TMpTransportHandle;

int16
L3BitReservoir(CMPAudDec *mp);

uint8
GetLayer3Version(TMpTransportHandle *tHandle);

#endif /*-- MP_AUD_H_ --*/
