#
# Copyright (c) 2010 Ixonos Plc.
# All rights reserved.
# This component and the accompanying materials are made available
# under the terms of the "Eclipse Public License v1.0"
# which accompanies this distribution, and is available
# at the URL "http://www.eclipse.org/legal/epl-v10.html".
#
# Initial Contributors:
# Nokia Corporation - Initial contribution
#
# Contributors:
# Ixonos Plc
#
# Description:
#

ifeq (WINSCW,$(findstring WINSCW, $(PLATFORM)))
ZDIR=$(EPOCROOT)epoc32\release\$(PLATFORM)\$(CFG)\Z
else
ZDIR=$(EPOCROOT)epoc32\data\Z
endif

TARGETDIR=$(ZDIR)\resource\apps
HEADERDIR=$(EPOCROOT)epoc32\include
ICONTARGETFILENAME=$(TARGETDIR)\ManualVideoEditor.mif
HEADERFILENAME=$(HEADERDIR)\ManualVideoEditor.mbg

do_nothing:
	@rem do_nothing

MAKMAKE : do_nothing

BLD : do_nothing

CLEAN : do_nothing
 
LIB : do_nothing

CLEANLIB : do_nothing

RESOURCE :
	mifconv $(ICONTARGETFILENAME) /h$(HEADERFILENAME) \
	qgn_graf_ve_symbol_audio.svg \
	qgn_prop_ve_file_video.svg \
	qgn_prop_ve_file_audio.svg \
	qgn_prop_ve_slow.svg \
	qgn_prop_ve_bw.svg \
	qgn_prop_ve_colour.svg \
	qgn_prop_ve_muted.svg \
	qgn_graf_ve_symbol_cut_audio.svg \
	qgn_prop_ve_rec.svg \
	qgn_graf_ve_trans_diptoblack01.svg \
	qgn_graf_ve_trans_diptoblack02.svg \
	qgn_graf_ve_trans_diptoblack03.svg \
	qgn_graf_ve_trans_diptoblack04.svg \
	qgn_graf_ve_trans_diptoblack05.svg \
	qgn_graf_ve_trans_diptoblack06.svg \
	qgn_graf_ve_trans_diptoblack07.svg \
	qgn_graf_ve_trans_diptoblack08.svg \
	qgn_graf_ve_trans_diptoblack09.svg \
	qgn_graf_ve_trans_diptoblack10.svg \
	qgn_graf_ve_trans_diptoblack11.svg \
	qgn_graf_ve_trans_diptoblack12.svg \
	qgn_graf_ve_trans_diptoblack13.svg \
	qgn_graf_ve_trans_diptowhite01.svg \
	qgn_graf_ve_trans_diptowhite02.svg \
	qgn_graf_ve_trans_diptowhite03.svg \
	qgn_graf_ve_trans_diptowhite04.svg \
	qgn_graf_ve_trans_diptowhite05.svg \
	qgn_graf_ve_trans_diptowhite06.svg \
	qgn_graf_ve_trans_diptowhite07.svg \
	qgn_graf_ve_trans_diptowhite08.svg \
	qgn_graf_ve_trans_diptowhite09.svg \
	qgn_graf_ve_trans_diptowhite10.svg \
	qgn_graf_ve_trans_diptowhite11.svg \
	qgn_graf_ve_trans_diptowhite12.svg \
	qgn_graf_ve_trans_diptowhite13.svg \
	qgn_graf_ve_trans_crossfade01.svg \
	qgn_graf_ve_trans_crossfade02.svg \
	qgn_graf_ve_trans_crossfade03.svg \
	qgn_graf_ve_trans_crossfade04.svg \
	qgn_graf_ve_trans_crossfade05.svg \
	qgn_graf_ve_trans_crossfade06.svg \
	qgn_graf_ve_trans_crossfade07.svg \
	qgn_graf_ve_trans_crossfade08.svg \
	qgn_graf_ve_trans_fadetoblack1.svg \
	qgn_graf_ve_trans_fadetoblack2.svg \
	qgn_graf_ve_trans_fadetoblack3.svg \
	qgn_graf_ve_trans_fadetoblack4.svg \
	qgn_graf_ve_trans_fadetoblack5.svg \
	qgn_graf_ve_trans_fadetoblack6.svg \
	qgn_graf_ve_trans_fadetoblack7.svg \
	qgn_graf_ve_trans_fadetoblack8.svg \
	qgn_graf_ve_trans_fadetowhite1.svg \
	qgn_graf_ve_trans_fadetowhite2.svg \
	qgn_graf_ve_trans_fadetowhite3.svg \
	qgn_graf_ve_trans_fadetowhite4.svg \
	qgn_graf_ve_trans_fadetowhite5.svg \
	qgn_graf_ve_trans_fadetowhite6.svg \
	qgn_graf_ve_trans_fadetowhite7.svg \
	qgn_graf_ve_trans_fadetowhite8.svg \
	qgn_graf_ve_trans_fadefromblack1.svg \
	qgn_graf_ve_trans_fadefromblack2.svg \
	qgn_graf_ve_trans_fadefromblack3.svg \
	qgn_graf_ve_trans_fadefromblack4.svg \
	qgn_graf_ve_trans_fadefromblack5.svg \
	qgn_graf_ve_trans_fadefromblack6.svg \
	qgn_graf_ve_trans_fadefromblack7.svg \
	qgn_graf_ve_trans_fadefromblack8.svg \
	qgn_graf_ve_trans_fadefromwhite1.svg \
	qgn_graf_ve_trans_fadefromwhite2.svg \
	qgn_graf_ve_trans_fadefromwhite3.svg \
	qgn_graf_ve_trans_fadefromwhite4.svg \
	qgn_graf_ve_trans_fadefromwhite5.svg \
	qgn_graf_ve_trans_fadefromwhite6.svg \
	qgn_graf_ve_trans_fadefromwhite7.svg \
	qgn_graf_ve_trans_fadefromwhite8.svg \
	qgn_graf_ve_trans_wipeleft1.svg \
	qgn_graf_ve_trans_wipeleft2.svg \
	qgn_graf_ve_trans_wipeleft3.svg \
	qgn_graf_ve_trans_wipeleft4.svg \
	qgn_graf_ve_trans_wipeleft5.svg \
	qgn_graf_ve_trans_wipeleft6.svg \
	qgn_graf_ve_trans_wipeleft7.svg \
	qgn_graf_ve_trans_wiperight1.svg \
	qgn_graf_ve_trans_wiperight2.svg \
	qgn_graf_ve_trans_wiperight3.svg \
	qgn_graf_ve_trans_wiperight4.svg \
	qgn_graf_ve_trans_wiperight5.svg \
	qgn_graf_ve_trans_wiperight6.svg \
	qgn_graf_ve_trans_wiperight7.svg \
	qgn_graf_ve_trans_wipebototop1.svg \
	qgn_graf_ve_trans_wipebototop2.svg \
	qgn_graf_ve_trans_wipebototop3.svg \
	qgn_graf_ve_trans_wipebototop4.svg \
	qgn_graf_ve_trans_wipebototop5.svg \
	qgn_graf_ve_trans_wipebototop6.svg \
	qgn_graf_ve_trans_wipebototop7.svg \
	qgn_graf_ve_trans_wipebototop8.svg \
	qgn_graf_ve_trans_wipebototop9.svg \
	qgn_graf_ve_trans_wipetoptobo1.svg \
	qgn_graf_ve_trans_wipetoptobo2.svg \
	qgn_graf_ve_trans_wipetoptobo3.svg \
	qgn_graf_ve_trans_wipetoptobo4.svg \
	qgn_graf_ve_trans_wipetoptobo5.svg \
	qgn_graf_ve_trans_wipetoptobo6.svg \
	qgn_graf_ve_trans_wipetoptobo7.svg \
	qgn_graf_ve_trans_wipetoptobo8.svg \
	qgn_graf_ve_trans_wipetoptobo9.svg

FREEZE  : do_nothing

SAVESPACE : do_nothing

RELEASABLES :
	@echo $(HEADERFILENAME)&& \
	@echo $(ICONTARGETFILENAME)

FINAL : do_nothing


