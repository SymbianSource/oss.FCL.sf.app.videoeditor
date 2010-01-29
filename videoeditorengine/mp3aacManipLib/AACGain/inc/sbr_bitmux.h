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


/*
  \file
  \brief SBR bitstream multiplexer interface $Revision: 1.1.1.1 $
*/

/**************************************************************************
  sbr_bitmux.cpp - TBitStream definitions for SBR encoder.
 
  Author(s): Juha Ojanpera
  Copyright (c) 2004 by Nokia Research Center, Multimedia Technologies.
  *************************************************************************/

#ifndef SBR_BITMUX_H_
#define SBR_BITMUX_H_

/*-- Project Headers. --*/
#include "env_extr.h"

int16
SBR_WriteHeaderData(SbrHeaderData *headerData, 
                    TBitStream *bs, 
                    uint8 writeFlag);

int16
SBR_WriteSCE(SbrHeaderData *headerData,
             SbrFrameData *frameData,
             SbrExtensionData *sbrExtData,
             TBitStream *bs,
             uint8 isMono,
             uint8 writeFlag);

int16
SBR_WriteCPE(SbrHeaderData *headerData,
             SbrFrameData *frameDataLeft,
             SbrFrameData *frameDataRight,
             SbrExtensionData *sbrExtData,
             TBitStream *bs,
             uint8 writeFlag);

#endif /*-- SBR_BITMUX_H_ --*/
