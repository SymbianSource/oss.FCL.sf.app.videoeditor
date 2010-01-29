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



#ifndef __AEDPROCTIME_H__
#define __AEDPROCTIME_H__


// complexity constants; to be moved to some header
const TReal KAEDMutingComplFactor = 0.02;
const TReal KAEDAMRDecComplFactor = 0.1;
const TReal KAEDAACDecComplFactor = 0.1;
const TReal KAEDAACStereoDecAddComplFactor = 0.1;
const TReal KAEDAMRWBDecComplFactor = 0.1;
const TReal KAEDMP3DecComplFactor = 0.2;
const TReal KAEDWavDecComplFactor = 0.02;
const TReal KAEDAMREncComplFactor = 0.5;
const TReal KAEDAACEncComplFactor = 0.3;
const TReal KAEDAACStereoEncAddComplFactor = 0.2;
const TReal KAEDBitstreamProcComplFactor = 0.05;
const TReal KAEDPassThroughComplFactor = 0.02;

#endif // __AEDPROCTIME_H__

