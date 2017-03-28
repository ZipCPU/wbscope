################################################################################
##
## Filename: 	
##
## Project:	WBScope, a wishbone hosted scope
##
## Purpose:	
##
## Creator:	Dan Gisselquist, Ph.D.
##		Gisselquist Technology, LLC
##
################################################################################
##
## Copyright (C) 2015-2017, Gisselquist Technology, LLC
##
## This program is free software (firmware): you can redistribute it and/or
## modify it under the terms of  the GNU General Public License as published
## by the Free Software Foundation, either version 3 of the License, or (at
## your option) any later version.
##
## This program is distributed in the hope that it will be useful, but WITHOUT
## ANY WARRANTY; without even the implied warranty of MERCHANTIBILITY or
## FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
## for more details.
##
## You should have received a copy of the GNU General Public License along
## with this program.  (It's in the $(ROOT)/doc directory.  Run make with no
## target there if the PDF file isn't present.)  If not, see
## <http://www.gnu.org/licenses/> for a copy.
##
## License:	GPL, v3, as defined and found on www.gnu.org,
##		http://www.gnu.org/licenses/gpl.html
##
##
################################################################################
##
##
all: wbscope_tb
CXX  := g++
RTLD := ../rtl
ROBJD:= $(RTLD)/obj_dir
VROOT:= /usr/share/verilator
VINCS:= -I$(VROOT)
INCS := -I$(VROOT)/include -I$(ROBJD)
VSRCS:= $(VROOT)/include/verilated.cpp $(VROOT)/include/verilated_vcd_c.cpp
TBOBJ:= $(ROBJD)/Vwbscope_tb__ALL.a

wbscope_tb:	wbscope_tb.cpp $(ROBJD)/Vwbscope_tb__ALL.a $(ROBJD)/Vwbscope_tb.h
	$(CXX) $(INCS) wbscope_tb.cpp $(VSRCS) $(TBOBJ) -o $@
