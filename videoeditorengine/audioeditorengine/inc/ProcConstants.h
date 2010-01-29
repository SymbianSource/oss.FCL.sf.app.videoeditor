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



#ifndef __PROCCONSTANTS_H__
#define __PROCCONSTANTS_H__

#include <e32std.h>

const TInt KAmrBitRates[] = 
    {
    4750,
    5150,
    5900,
    6700,
    7400,
    7950,
    10200,
    12200
    };

// bit locations for fixed codebook gains

// subframe 1, bitrate 12.2 kBit/s
const TUint8 KAmrGains122_1[] = {59+8,63+8,67+8,92+8,104+8};
// subframe 2, bitrate 12.2 kBit/s
const TUint8 KAmrGains122_2[] = {60+8,64+8,68+8,93+8,105+8};
// subframe 3, bitrate 12.2 kBit/s
const TUint8 KAmrGains122_3[] = {61+8,65+8,69+8,94+8,106+8};
// subframe 4, bitrate 12.2 kBit/s
const TUint8 KAmrGains122_4[] = {62+8,66+8,70+8,95+8,107+8};

// subframe 1, bitrate 7.95 kBit/s
const TUint8 KAmrGains795_1[] = {23+8,27+8,31+8,59+8,83+8};
// subframe 2, bitrate 7.95 kBit/s
const TUint8 KAmrGains795_2[] = {24+8,28+8,32+8,60+8,84+8};
// subframe 3, bitrate 7.95 kBit/s
const TUint8 KAmrGains795_3[] = {25+8,29+8,33+8,61+8,85+8};
// subframe 4, bitrate 7.95 kBit/s
const TUint8 KAmrGains795_4[] = {26+8,30+8,34+8,62+8,86+8};


// subframe 1, bitrate 10.2 kBit/s
const TUint8 KAmrGains102_1[] = {33+8,53+8,83+8,34+8,35+8,82+8,84+8};
// subframe 2, bitrate 10.2 kBit/s
const TUint8 KAmrGains102_2[] = {36+8,54+8,86+8,37+8,38+8,85+8,87+8};
// subframe 3, bitrate 10.2 kBit/s
const TUint8 KAmrGains102_3[] = {39+8,55+8,89+8,40+8,41+8,88+8,90+8};
// subframe 4, bitrate 10.2 kBit/s
const TUint8 KAmrGains102_4[] = {42+8,56+8,92+8,43+8,44+8,91+8,93+8};


// subframe 1, bitrate 7.4 kBit/s
const TUint8 KAmrGains740_1[] = {27+8,31+8,78+8,35+8,39+8,57+8,68+8};
// subframe 2, bitrate 7.4 kBit/s
const TUint8 KAmrGains740_2[] = {28+8,32+8,79+8,36+8,40+8,58+8,69+8};
// subframe 3, bitrate 7.4 kBit/s
const TUint8 KAmrGains740_3[] = {29+8,33+8,80+8,37+8,41+8,59+8,70+8};
// subframe 4, bitrate 7.4 kBit/s
const TUint8 KAmrGains740_4[] = {30+8,34+8,81+8,38+8,42+8,60+8,71+8};


// subframe 1, bitrate 6.7 kBit/s
const TUint8 KAmrGains670_1[] = {35+8,85+8,66+8,41+8,45+8,55+8,74+8};
// subframe 2, bitrate 6.7 kBit/s
const TUint8 KAmrGains670_2[] = {36+8,84+8,67+8,42+8,46+8,56+8,75+8};
// subframe 3, bitrate 6.7 kBit/s
const TUint8 KAmrGains670_3[] = {37+8,83+8,68+8,43+8,47+8,57+8,76+8};
// subframe 4, bitrate 6.7 kBit/s
const TUint8 KAmrGains670_4[] = {38+8,82+8,69+8,44+8,48+8,58+8,77+8};


// subframe 1, bitrate 5.9 kBit/s
const TUint8 KAmrGains590_1[] = {76+8,55+8,51+8,47+8,37+8,29+8};
// subframe 2, bitrate 5.9 kBit/s
const TUint8 KAmrGains590_2[] = {77+8,56+8,52+8,48+8,38+8,30+8};
// subframe 3, bitrate 5.9 kBit/s
const TUint8 KAmrGains590_3[] = {78+8,57+8,53+8,49+8,39+8,31+8};
// subframe 4, bitrate 5.9 kBit/s
const TUint8 KAmrGains590_4[] = {79+8,58+8,54+8,50+8,40+8,32+8};



// subframe 1, bitrate 5.15 kBit/s
const TUint8 KAmrGains515_1[] = {55+8,45+8,36+8,26+8,25+8,24+8};
// subframe 2, bitrate 5.15 kBit/s
const TUint8 KAmrGains515_2[] = {56+8,46+8,37+8,29+8,28+8,27+8};
// subframe 3, bitrate 5.15 kBit/s
const TUint8 KAmrGains515_3[] = {57+8,47+8,38+8,32+8,31+8,30+8};
// subframe 4, bitrate 5.15 kBit/s
const TUint8 KAmrGains515_4[] = {58+8,48+8,39+8,35+8,34+8,33+8};


//subframes 1 & s, bitrate 4.75 kBit/s
const TUint8 KAmrGains475_1_2[] = {49+8,48+8,47+8,46+8,31+8,30+8,29+8,28+8};
const TUint8 KAmrGains475_3_4[] = {43+8,42+8,41+8,40+8,35+8,34+8,33+8,32+8};


// gp:s

// subframe 1, bitrate 12.2 kBit/s
const TUint8 KAmrGPGains122_1[] = {47+8, 51+8,55+8,88+8};
// subframe 2, bitrate 12.2 kBit/s
const TUint8 KAmrGPGains122_2[] = {48+8, 52+8,56+8,89+8};
// subframe 3, bitrate 12.2 kBit/s
const TUint8 KAmrGPGains122_3[] = {49+8, 53+8,57+8,90+8};
// subframe 4, bitrate 12.2 kBit/s
const TUint8 KAmrGPGains122_4[] = {50+8, 54+8,58+8,91+8};


// subframe 1, bitrate 7.95 kBit/s
const TUint8 KAmrGPGains795_1[] = {35+8, 39+8, 79+8, 87+8};
// subframe 2, bitrate 7.95 kBit/s
const TUint8 KAmrGPGains795_2[] = {36+8, 40+8, 80+8, 88+8};
// subframe 3, bitrate 7.95 kBit/s
const TUint8 KAmrGPGains795_3[] = {37+8, 41+8, 81+8, 89+8};
// subframe 4, bitrate 7.95 kBit/s
const TUint8 KAmrGPGains795_4[] = {38+8, 42+8, 82+8, 90+8};

// Gain table 12.2 kBit/s and 7.95 kBit/s, scalar quantized
const TInt KAmrGainTable122[] = 
	{
	159,
	206,
	268,
	349,
	419,
	482,
	554,
	637,
	733,
	842,
	969,
	1114,
	1281,
	1473,
	1694,
	1948,
	2241,
	2577,
	2963,
	3408,
	3919,
	4507,
	5183,
	5960,
	6855,
	7883,
	9065,
	10425,
	12510,
	16263,
	21142,
	27485
	};

#define NB_QUA_PITCH 16

const TInt KAmrGPTable[NB_QUA_PITCH] =
{
    0, 3277, 6556, 8192, 9830, 11469, 12288, 13107,
    13926, 14746, 15565, 16384, 17203, 18022, 18842, 19661
};


const TInt KAmrLargestGain122 = 27485;

// Gain table, 10.2, 6.70 and 7.40 kBit/s vector quantized

// index = 0...127
// KAmrGainTable[2*index] = g_pitch
// KAmrGainTable[2*index+1] = g_fac (fixed codebook gain)

const TInt KAmrGainTable102[] =
    {
//g_pit,    g_fac 
    577,      662,        
    806,     1836,           
   3109,     1052,   
   4181,     1387,           
   2373,     1425,           
   3248,     1985,           
   1827,     2320,           
    941,     3314,           
   2351,     2977,           
   3616,     2420,           
   3451,     3096,           
   2955,     4301,           
   1848,     4500,           
   3884,     5416,           
   1187,     7210,           
   3083,     9000,           
   7384,      883,           
   5962,     1506,           
   5155,     2134,           
   7944,     2009,           
   6507,     2250,           
   7670,     2752,           
   5952,     3016,           
   4898,     3764,           
   6989,     3588,           
   8174,     3978,           
   6064,     4404,           
   7709,     5087,           
   5523,     6021,           
   7769,     7126,           
   6060,     7938,           
   5594,    11487,           
  10581,     1356,           
   9049,     1597,           
   9794,     2035,           
   8946,     2415,            
  10296,     2584,         
   9407,     2734,            
   8700,     3218,            
   9757,     3395,            
  10177,     3892,             
   9170,     4528,            
  10152,     5004,            
   9114,     5735,            
  10500,     6266,           
  10110,     7631,            
   8844,     8727,            
   8956,    12496,          
  12924,      976,          
  11435,     1755,           
  12138,     2328,            
  11388,     2368,            
  10700,     3064,            
  12332,     2861,            
  11722,     3327,            
  11270,     3700,            
  10861,     4413,           
  12082,     4533,             
  11283,     5205,            
  11960,     6305,            
  11167,     7534,             
  12128,     8329,            
  10969,    10777,            
  10300,    17376,            
  13899,     1681,           
  12580,     2045,          
  13265,     2439,           
  14033,     2989,            
  13452,     3098,           
  12396,     3658,           
  13510,     3780,            
  12880,     4272,             
  13533,     4861,           
  12667,     5457,             
  13854,     6106,             
  13031,     6483,           
  13557,     7721,             
  12957,     9311,           
  13714,    11551,            
  12591,    15206,           
  15113,     1540,           
  15072,     2333,            
  14527,     2511,           
  14692,     3199,            
  15382,     3560,           
  14133,     3960,             
  15102,     4236,             
  14332,     4824,            
  14846,     5451,           
  15306,     6083,            
  14329,     6888,           
  15060,     7689,             
  14406,     9426,           
  15387,     9741,           
  14824,    14271,           
  13600,    24939,          
  16396,     1969,           
  16817,     2832,           
  15713,     2843,            
  16104,     3336,            
  16384,     3963,            
  16940,     4579,             
  15711,     4599,            
  16222,     5448,             
  16832,     6382,            
  15745,     7141,            
  16326,     7469,           
  16611,     8624,          
  17028,    10418,           
  15905,    11817,            
  16878,    14690,            
  16515,    20870,            
  18142,     2083,            
  19401,     3178,           
  17508,     3426,            
  20054,     4027,            
  18069,     4249,             
  18952,     5066,             
  17711,     5402,             
  19835,     6192,             
  17950,     7014,            
  21318,     7877,             
  17910,     9289,           
  19144,     9290,           
  20517,    11381,           
  18075,    14485,            
  19999,    17882,            
  18842,    32764
  };

const TInt KAmrLargestGain102 = 32764;


// Gain table, 5.9 and 5.15 kBit/s vector quantized

// index = 0...127
// KAmrGainTable[2*index] = g_pitch
// KAmrGainTable[2*index+1] = g_fac 
// g_pitch        (Q14),
// g_fac          (Q12), 

const TInt KAmrGainTable590[] =
    {
//g_pit,    g_fac  
  10813,    28753,            
  20480,     2785,          
  18841,     6594,           
   6225,     7413,           
  17203,    10444,           
  21626,     1269,           
  21135,     4423,           
  11304,     1556,           
  19005,    12820,            
  17367,     2498,           
  17858,     4833,            
   9994,     2498,           
  17530,     7864,            
  14254,     1884,          
  15892,     3153,            
   6717,     1802,           
  18186,    20193,            
  18022,     3031,            
  16711,     5857,             
   8847,     4014,            
  15892,     8970,            
  18022,     1392,           
  16711,     4096,              
   8192,      655,           
  15237,    13926,            
  14254,     3112,            
  14090,     4669,            
   5406,     2703,            
  13434,     6553,             
  12451,      901,           
  12451,     2662,           
   3768,      655,           
  14745,    23511,            
  19169,     2457,            
  20152,     5079,            
   6881,     4096,               
  20480,     8560,           
  19660,      737,           
  19005,     4259,             
   7864,     2088,            
  11468,    12288,           
  15892,     1474,           
  15728,     4628,            
   9175,     1433,           
  16056,     7004,           
  14827,      737,          
  15073,     2252,           
   5079,     1228,           
  13271,    17326,           
  16547,     2334,            
  15073,     5816,             
   3932,     3686,           
  14254,     8601,           
  16875,      778,           
  15073,     3809,           
   6062,      614,           
   9338,     9256,            
  13271,     1761,           
  13271,     3522,           
   2457,     1966,           
  11468,     5529,            
  10485,      737,           
  11632,     3194,           
   1474,      778           
    };

const TInt KAmrLargestGain590 = 23511;


// index = 0...127
// KAmrGainTable[4*index] = g_pitch(even frame)
// KAmrGainTable[4*index+1] = g_fac(even frame)
// KAmrGainTable[4*index+2] = g_pitch(odd frame)
// KAmrGainTable[4*index+3] = g_fac(odd frame)


const TInt KAmrGainTable475[] = 
    {
//g_pit(0),    g_fac(0),      g_pit(1),    g_fac(1)      
   812,          128,           542,      140,
  2873,         1135,          2266,     3402,
  2067,          563,         12677,      647,
  4132,         1798,          5601,     5285,
  7689,          374,          3735,      441,
 10912,         2638,         11807,     2494,
 20490,          797,          5218,      675,
  6724,         8354,          5282,     1696,
  1488,          428,          5882,      452,
  5332,         4072,          3583,     1268,
  2469,          901,         15894,     1005,
 14982,         3271,         10331,     4858,
  3635,         2021,          2596,      835,
 12360,         4892,         12206,     1704,
 13432,         1604,          9118,     2341,
  3968,         1538,          5479,     9936,
  3795,          417,          1359,      414,
  3640,         1569,          7995,     3541,
 11405,          645,          8552,      635,
  4056,         1377,         16608,     6124,
 11420,          700,          2007,      607,
 12415,         1578,         11119,     4654,
 13680,         1708,         11990,     1229,
  7996,         7297,         13231,     5715,
  2428,         1159,          2073,     1941,
  6218,         6121,          3546,     1804,
  8925,         1802,          8679,     1580,
 13935,         3576,         13313,     6237,
  6142,         1130,          5994,     1734,
 14141,         4662,         11271,     3321,
 12226,         1551,         13931,     3015,
  5081,        10464,          9444,     6706,
  1689,          683,          1436,     1306,
  7212,         3933,          4082,     2713,
  7793,          704,         15070,      802,
  6299,         5212,          4337,     5357,
  6676,          541,          6062,      626,
 13651,         3700,         11498,     2408,
 16156,          716,         12177,      751,
  8065,        11489,          6314,     2256,
  4466,          496,          7293,      523,
 10213,         3833,          8394,     3037,
  8403,          966,         14228,     1880,
  8703,         5409,         16395,     4863,
  7420,         1979,          6089,     1230,
  9371,         4398,         14558,     3363,
 13559,         2873,         13163,     1465,
  5534,         1678,         13138,    14771,
  7338,          600,          1318,      548,
  4252,         3539,         10044,     2364,
 10587,          622,         13088,      669,
 14126,         3526,          5039,     9784,
 15338,          619,          3115,      590,
 16442,         3013,         15542,     4168,
 15537,         1611,         15405,     1228,
 16023,         9299,          7534,     4976,
  1990,         1213,         11447,     1157,
 12512,         5519,          9475,     2644,
  7716,         2034,         13280,     2239,
 16011,         5093,          8066,     6761,
 10083,         1413,          5002,     2347,
 12523,         5975,         15126,     2899,
 18264,         2289,         15827,     2527,
 16265,        10254,         14651,    11319,
  1797,          337,          3115,      397,
  3510,         2928,          4592,     2670,
  7519,          628,         11415,      656,
  5946,         2435,          6544,     7367,
  8238,          829,          4000,      863,
 10032,         2492,         16057,     3551,
 18204,         1054,          6103,     1454,
  5884,         7900,         18752,     3468,
  1864,          544,          9198,      683,
 11623,         4160,          4594,     1644,
  3158,         1157,         15953,     2560,
 12349,         3733,         17420,     5260,
  6106,         2004,          2917,     1742,
 16467,         5257,         16787,     1680,
 17205,         1759,          4773,     3231,
  7386,         6035,         14342,    10012,
  4035,          442,          4194,      458,
  9214,         2242,          7427,     4217,
 12860,          801,         11186,      825,
 12648,         2084,         12956,     6554,
  9505,          996,          6629,      985,
 10537,         2502,         15289,     5006,
 12602,         2055,         15484,     1653,
 16194,         6921,         14231,     5790,
  2626,          828,          5615,     1686,
 13663,         5778,          3668,     1554,
 11313,         2633,          9770,     1459,
 14003,         4733,         15897,     6291,
  6278,         1870,          7910,     2285,
 16978,         4571,         16576,     3849,
 15248,         2311,         16023,     3244,
 14459,        17808,         11847,     2763,
  1981,         1407,          1400,      876,
  4335,         3547,          4391,     4210,
  5405,          680,         17461,      781,
  6501,         5118,          8091,     7677,
  7355,          794,          8333,     1182,
 15041,         3160,         14928,     3039,
 20421,          880,         14545,      852,
 12337,        14708,          6904,     1920,
  4225,          933,          8218,     1087,
 10659,         4084,         10082,     4533,
  2735,          840,         20657,     1081,
 16711,         5966,         15873,     4578,
 10871,         2574,          3773,     1166,
 14519,         4044,         20699,     2627,
 15219,         2734,         15274,     2186,
  6257,         3226,         13125,    19480,
  7196,          930,          2462,     1618,
  4515,         3092,         13852,     4277,
 10460,          833,         17339,      810,
 16891,         2289,         15546,     8217,
 13603,         1684,          3197,     1834,
 15948,         2820,         15812,     5327,
 17006,         2438,         16788,     1326,
 15671,         8156,         11726,     8556,
  3762,         2053,          9563,     1317,
 13561,         6790,         12227,     1936,
  8180,         3550,         13287,     1778,
 16299,         6599,         16291,     7758,
  8521,         2551,          7225,     2645,
 18269,         7489,         16885,     2248,
 17882,         2884,         17265,     3328,
  9417,        20162,         11042,     8320,
  1286,          620,          1431,      583,
  5993,         2289,          3978,     3626,
  5144,          752,         13409,      830,
  5553,         2860,         11764,     5908,
 10737,          560,          5446,      564,
 13321,         3008,         11946,     3683,
 19887,          798,          9825,      728,
 13663,         8748,          7391,     3053,
  2515,          778,          6050,      833,
  6469,         5074,          8305,     2463,
  6141,         1865,         15308,     1262,
 14408,         4547,         13663,     4515,
  3137,         2983,          2479,     1259,
 15088,         4647,         15382,     2607,
 14492,         2392,         12462,     2537,
  7539,         2949,         12909,    12060,
  5468,          684,          3141,      722,
  5081,         1274,         12732,     4200,
 15302,          681,          7819,      592,
  6534,         2021,         16478,     8737,
 13364,          882,          5397,      899,
 14656,         2178,         14741,     4227,
 14270,         1298,         13929,     2029,
 15477,         7482,         15815,     4572,
  2521,         2013,          5062,     1804,
  5159,         6582,          7130,     3597,
 10920,         1611,         11729,     1708,
 16903,         3455,         16268,     6640,
  9306,         1007,          9369,     2106,
 19182,         5037,         12441,     4269,
 15919,         1332,         15357,     3512,
 11898,        14141,         16101,     6854,
  2010,          737,          3779,      861,
 11454,         2880,          3564,     3540,
  9057,         1241,         12391,      896,
  8546,         4629,         11561,     5776,
  8129,          589,          8218,      588,
 18728,         3755,         12973,     3149,
 15729,          758,         16634,      754,
 15222,        11138,         15871,     2208,
  4673,          610,         10218,      678,
 15257,         4146,          5729,     3327,
  8377,         1670,         19862,     2321,
 15450,         5511,         14054,     5481,
  5728,         2888,          7580,     1346,
 14384,         5325,         16236,     3950,
 15118,         3744,         15306,     1435,
 14597,         4070,         12301,    15696,
  7617,         1699,          2170,      884,
  4459,         4567,         18094,     3306,
 12742,          815,         14926,      907,
 15016,         4281,         15518,     8368,
 17994,         1087,          2358,      865,
 16281,         3787,         15679,     4596,
 16356,         1534,         16584,     2210,
 16833,         9697,         15929,     4513,
  3277,         1085,          9643,     2187,
 11973,         6068,          9199,     4462,
  8955,         1629,         10289,     3062,
 16481,         5155,         15466,     7066,
 13678,         2543,          5273,     2277,
 16746,         6213,         16655,     3408,
 20304,         3363,         18688,     1985,
 14172,        12867,         15154,    15703,
  4473,         1020,          1681,      886,
  4311,         4301,          8952,     3657,
  5893,         1147,         11647,     1452,
 15886,         2227,          4582,     6644,
  6929,         1205,          6220,      799,
 12415,         3409,         15968,     3877,
 19859,         2109,          9689,     2141,
 14742,         8830,         14480,     2599,
  1817,         1238,          7771,      813,
 19079,         4410,          5554,     2064,
  3687,         2844,         17435,     2256,
 16697,         4486,         16199,     5388,
  8028,         2763,          3405,     2119,
 17426,         5477,         13698,     2786,
 19879,         2720,          9098,     3880,
 18172,         4833,         17336,    12207,
  5116,          996,          4935,      988,
  9888,         3081,          6014,     5371,
 15881,         1667,          8405,     1183,
 15087,         2366,         19777,     7002,
 11963,         1562,          7279,     1128,
 16859,         1532,         15762,     5381,
 14708,         2065,         20105,     2155,
 17158,         8245,         17911,     6318,
  5467,         1504,          4100,     2574,
 17421,         6810,          5673,     2888,
 16636,         3382,          8975,     1831,
 20159,         4737,         19550,     7294,
  6658,         2781,         11472,     3321,
 19397,         5054,         18878,     4722,
 16439,         2373,         20430,     4386,
 11353,        26526,         11593,     3068,
  2866,         1566,          5108,     1070,
  9614,         4915,          4939,     3536,
  7541,          878,         20717,      851,
  6938,         4395,         16799,     7733,
 10137,         1019,          9845,      964,
 15494,         3955,         15459,     3430,
 18863,          982,         20120,      963,
 16876,        12887,         14334,     4200,
  6599,         1220,          9222,      814,
 16942,         5134,          5661,     4898,
  5488,         1798,         20258,     3962,
 17005,         6178,         17929,     5929,
  9365,         3420,          7474,     1971,
 19537,         5177,         19003,     3006,
 16454,         3788,         16070,     2367,
  8664,         2743,          9445,    26358,
 10856,         1287,          3555,     1009,
  5606,         3622,         19453,     5512,
 12453,          797,         20634,      911,
 15427,         3066,         17037,    10275,
 18883,         2633,          3913,     1268,
 19519,         3371,         18052,     5230,
 19291,         1678,         19508,     3172,
 18072,        10754,         16625,     6845,
  3134,         2298,         10869,     2437,
 15580,         6913,         12597,     3381,
 11116,         3297,         16762,     2424,
 18853,         6715,         17171,     9887,
 12743,         2605,          8937,     3140,
 19033,         7764,         18347,     3880,
 20475,         3682,         19602,     3380,
 13044,        19373,         10526,    23124
};

const TInt KAmrLargestGain475 = 26526;


// KAmrGaindB2Gamma[0] is gamma*10000 at -127 dB
// KAmrGaindB2Gamma[1] is gamma*10000 at -126 dB
// KAmrGaindB2Gamma[127] is gamma*10000 at 0 dB
// KAmrGaindB2Gamma[128] is gamma*10000 at 1 dB
// KAmrGaindB2Gamma[255] is gamma*10000 at 128 dB

const TInt KAmrGain_dB2Gamma[] = 

    {
53,
55,
58,
60,
62,
65,
68,
71,
74,
77,
80,
83,
87,
91,
94,
98,
103,
107,
111,
116,
121,
126,
131,
137,
143,
149,
155,
161,
168,
175,
183,
190,
198,
207,
215,
225,
234,
244,
254,
265,
276,
288,
300,
312,
325,
339,
353,
368,
384,
400,
417,
435,
453,
472,
492,
512,
534,
557,
580,
604,
630,
656,
684,
713,
743,
774,
807,
841,
876,
913,
952,
992,
1034,
1077,
1122,
1170,
1219,
1270,
1324,
1380,
1438,
1498,
1562,
1627,
1696,
1767,
1842,
1919,
2000,
2084,
2172,
2264,
2359,
2459,
2562,
2670,
2783,
2900,
3022,
3149,
3282,
3420,
3564,
3714,
3871,
4034,
4204,
4381,
4566,
4758,
4958,
5167,
5385,
5612,
5848,
6095,
6351,
6619,
6898,
7188,
7491,
7807,
8136,
8478,
8836,
9208,
9596,
10000,
10421,
10860,
11318,
11795,
12292,
12809,
13349,
13911,
14497,
15108,
15745,
16408,
17099,
17820,
18570,
19353,
20168,
21017,
21903,
22826,
23787,
24789,
25834,
26922,
28056,
29238,
30470,
31754,
33091,
34485,
35938,
37452,
39030,
40674,
42388,
44173,
46034,
47974,
49995,
52101,
54296,
56583,
58967,
61451,
64040,
66738,
69549,
72479,
75533,
78715,
82031,
85487,
89088,
92841,
96753,
100829,
105076,
109503,
114116,
118924,
123934,
129155,
134596,
140266,
146175,
152334,
158751,
165439,
172409,
179672,
187241,
195129,
203350,
211917,
220844,
230148,
239844,
249948,
260478,
271451,
282887,
294804,
307224,
320167,
333655,
347711,
362360,
377625,
393534,
410113,
427390,
445395,
464159,
483713,
504091,
525327,
547459,
570522,
594557,
619605,
645708,
672910,
701258,
730801,
761589,
793673,
827109,
861954,
898266,
936108,
975545,
1016643,
1059472,
1104106,
1150620,
1199093,
1249609,
1302253,
1357114,
1414287,
1473869,
1535960,
1600667,
1668101,
1738375,
1811609,
1887929,
    };




/*
 * definition of modes for decoder
 */
enum Mode 
    { 
    MR475 = 0,
    MR515,
    MR59,
    MR67,
    MR74,
    MR795,
    MR102,
    MR122,
    MRDTX,
    N_MODES     /* number of (SPC) modes */
    };




#endif
