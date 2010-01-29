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
  \brief SBR ROM tables interface $Revision: 1.1.1.1.4.1 $
*/

/**************************************************************************
  sbr_rom.h - SBR ROM tables interface.
 
  Author(s): Juha Ojanpera
  Copyright (c) 2004 by Nokia Research Center, Multimedia Technologies.
  *************************************************************************/

#ifndef SBR_ROM_H_
#define SBR_ROM_H_

/*-- Project Headers. --*/
#include "env_extr.h"

#define INV_INT_TABLE_SIZE    (55)
#define LOG_DUALIS_TABLE_SIZE (65)
#define SBR_BIT_ARRAY_SIZE    (8)

extern const uint32 bitArray[SBR_BIT_ARRAY_SIZE];

extern const uint8 sbr_start_freq_16[16];
extern const uint8 sbr_start_freq_22[16];
extern const uint8 sbr_start_freq_24[16];
extern const uint8 sbr_start_freq_32[16];
extern const uint8 sbr_start_freq_44[16];
extern const uint8 sbr_start_freq_48[16];
extern const uint8 sbr_start_freq_64[16];
extern const uint8 sbr_start_freq_88[16];

extern const FLOAT sbr_invIntTable[INV_INT_TABLE_SIZE];
extern const FLOAT logDualisTable[LOG_DUALIS_TABLE_SIZE];

extern const FRAME_INFO sbr_staticFrameInfo[3];

extern const SbrHeaderData sbr_defaultHeader;

extern const int8 sbr_huffBook_EnvLevel10T[120][2];
extern const int8 sbr_huffBook_EnvLevel10F[120][2];
extern const int8 sbr_huffBook_EnvBalance10T[48][2];
extern const int8 sbr_huffBook_EnvBalance10F[48][2];
extern const int8 sbr_huffBook_EnvLevel11T[62][2];
extern const int8 sbr_huffBook_EnvLevel11F[62][2];
extern const int8 sbr_huffBook_EnvBalance11T[24][2];
extern const int8 sbr_huffBook_EnvBalance11F[24][2];
extern const int8 sbr_huffBook_NoiseLevel11T[62][2];
extern const int8 sbr_huffBook_NoiseBalance11T[24][2];

extern const int32 v_Huff_envelopeLevelC10T[121];
extern const uint8 v_Huff_envelopeLevelL10T[121];
extern const int32 v_Huff_envelopeLevelC10F[121];
extern const uint8 v_Huff_envelopeLevelL10F[121];
extern const int32 bookSbrEnvBalanceC10T[49];
extern const uint8 bookSbrEnvBalanceL10T[49];
extern const int32 bookSbrEnvBalanceC10F[49];
extern const uint8 bookSbrEnvBalanceL10F[49];
extern const int32 v_Huff_envelopeLevelC11T[63];
extern const uint8 v_Huff_envelopeLevelL11T[63];
extern const int32 v_Huff_envelopeLevelC11F[63];
extern const uint8 v_Huff_envelopeLevelL11F[63];
extern const uint16 bookSbrEnvBalanceC11T[25];
extern const uint8 bookSbrEnvBalanceL11T[25];
extern const uint16 bookSbrEnvBalanceC11F[25];
extern const uint8 bookSbrEnvBalanceL11F[25];
extern const uint16 v_Huff_NoiseLevelC11T[63];
extern const uint8 v_Huff_NoiseLevelL11T[63];
extern const uint16 bookSbrNoiseBalanceC11T[25];
extern const uint8 bookSbrNoiseBalanceL11T[25];

#endif /*-- SBR_ROM_H_ --*/
