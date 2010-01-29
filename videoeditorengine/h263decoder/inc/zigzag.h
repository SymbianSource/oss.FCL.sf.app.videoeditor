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
* Definition of the zigzag tables.
*
*/


#ifndef _ZIGZAG_H_
#define _ZIGZAG_H_

/*
 * Structs and typedefs
 */

/* Normal zigzag */
static const int zigzag[64] = {
  0, 1, 5, 6,14,15,27,28,
  2, 4, 7,13,16,26,29,42,
  3, 8,12,17,25,30,41,43,
  9,11,18,24,31,40,44,53,
  10,19,23,32,39,45,52,54,
  20,22,33,38,46,51,55,60,
  21,34,37,47,50,56,59,61,
  35,36,48,49,57,58,62,63
};

/* Horizontal zigzag */
static const int zigzag_h[64] = {
     0, 1, 2, 3,10,11,12,13,
     4, 5, 8, 9,17,16,15,14,
     6, 7,19,18,26,27,28,29,
    20,21,24,25,30,31,32,33,
    22,23,34,35,42,43,44,45,
    36,37,40,41,46,47,48,49,
    38,39,50,51,56,57,58,59,
    52,53,54,55,60,61,62,63
};

/* Vertical zigzag */
static const int zigzag_v[64] = {
     0, 4, 6,20,22,36,38,52,
     1, 5, 7,21,23,37,39,53,
     2, 8,19,24,34,40,50,54,
     3, 9,18,25,35,41,51,55,
    10,17,26,30,42,46,56,60,
    11,16,27,31,43,47,57,61,
    12,15,28,32,44,48,58,62,
    13,14,29,33,45,49,59,63
};

/* Inverse normal zigzag */
static const int zigzag_i[64] =
{
     0, 1, 8,16, 9, 2, 3,10,
    17,24,32,25,18,11, 4, 5,
    12,19,26,33,40,48,41,34,
    27,20,13, 6, 7,14,21,28,
    35,42,49,56,57,50,43,36,
    29,22,15,23,30,37,44,51,
    58,59,52,45,38,31,39,46,
    53,60,61,54,47,55,62,63
};

#endif /* ifndef _ZIGZAG_H_ */
// End of File
