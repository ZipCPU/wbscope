################################################################################
##
## Filename:	Makefile
## {{{
## Project:	WBScope, a wishbone hosted scope
##
## Purpose:	This makefile builds a verilator simulation of the rtl
##		testbenches necessary to test certain components of both the
##	wishbone scope and its RLE compressed brother using Verilator.
##
##
## Creator:	Dan Gisselquist, Ph.D.
##		Gisselquist Technology, LLC
##
################################################################################
## }}}
## Copyright (C) 2015-2021, Gisselquist Technology, LLC
## {{{
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
## }}}
## License:	GPL, v3, as defined and found on www.gnu.org,
## {{{
##		http://www.gnu.org/licenses/gpl.html
##
################################################################################
##
## }}}
.PHONY: all
all: wbscope_tb wbscopc_tb

RTLD := ../../rtl
VOBJ := obj_dir

# Building the wbscope test bench
## {{{
#
#
$(VOBJ)/Vwbscope_tb.cpp: $(RTLD)/wbscope.v wbscope_tb.v
	verilator -Wall -O3 -trace -cc  -y $(RTLD) wbscope_tb.v
$(VOBJ)/Vwbscope_tb.h: $(VOBJ)/Vwbscope_tb.cpp

$(VOBJ)/Vwbscope_tb__ALL.a: $(VOBJ)/Vwbscope_tb.cpp $(VOBJ)/Vwbscope_tb.h
	make --no-print-directory --directory=$(VOBJ) -f Vwbscope_tb.mk

.PHONY: wbscope_tb
wbscope_tb: $(VOBJ)/Vwbscope_tb__ALL.a
## }}}

# Building the wbscopc test bench, for the compressed wbscope
## {{{
#
#
$(VOBJ)/Vwbscopc_tb.cpp: $(RTLD)/wbscopc.v wbscopc_tb.v
	verilator -Wall -O3 -trace -cc  -y $(RTLD) wbscopc_tb.v
$(VOBJ)/Vwbscopc_tb.h: $(VOBJ)/Vwbscopc_tb.cpp

$(VOBJ)/Vwbscopc_tb__ALL.a: $(VOBJ)/Vwbscopc_tb.cpp $(VOBJ)/Vwbscopc_tb.h
	make --no-print-directory --directory=$(VOBJ) -f Vwbscopc_tb.mk

.PHONY: wbscopc_tb
wbscopc_tb: $(VOBJ)/Vwbscopc_tb__ALL.a
## }}}

# $(VOBJ)/Vaxiscope_tb.cpp: $(RTLD)/axiscope.v axiscope.v
#	verilator -trace -cc  -y $(RTLD) wbscope_tb.v
# $(VOBJ)/Vaxiscope_tb.h: $(VOBJ)/Vwbscope_tb.cpp

.PHONY: clean
clean:
	rm -rf $(VOBJ)
