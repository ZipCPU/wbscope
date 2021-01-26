################################################################################
##
## Filename:	bench/cpp/Makefile
## {{{
## Project:	WBScope, a wishbone hosted scope
##
## Purpose:	This file directs the build of a Verilator-based test bench to
##		prove that the wbscope and wbscopc work.  This build must be
##	called after building in bench/rtl, since it depends upon the products
##	of that build.
##
## Targets:
## {{{
##	all:	Builds both wbscope_tb and wbscopc_tb
##
##	clean:	Cleans up all of the build products, together with the .vcd
##		files, so you can start over from scratch.
##
##	wbscope_tb:	A test bench for the basic wishbone scope.
##			Prints success or failure on the last line.
##
##	wbscopc_tb:	A test bench for the compressed wishbone scope.
##			Prints success or failure on the last line.
##
##	test:	Runs both testbenches, printing success if both succeed, or
##		failure if one of the two does not.
## }}}
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
all: wbscope_tb wbscopc_tb
CXX  := g++
RTLD := ../rtl
ROBJD:= $(RTLD)/obj_dir
VERILATOR_ROOT ?= $(shell bash -c 'verilator -V|grep VERILATOR_ROOT| head -1|sed -e " s/^.*=\s*//"')
VROOT:= $(VERILATOR_ROOT)
INCS := -I$(VROOT)/include -I$(ROBJD)
VSRCS:= $(VROOT)/include/verilated.cpp $(VROOT)/include/verilated_vcd_c.cpp
TBOBJ:= $(ROBJD)/Vwbscope_tb__ALL.a
TCOBJ:= $(ROBJD)/Vwbscopc_tb__ALL.a

## WBSCOPE
## {{{
wbscope_tb:	wbscope_tb.cpp $(TBOBJ) $(ROBJD)/Vwbscope_tb.h wb_tb.h testb.h
	$(CXX) $(INCS) wbscope_tb.cpp $(VSRCS) $(TBOBJ) -o $@
## }}}

## WBSCOPC -- The compressed WBSCOPE
## {{{
wbscopc_tb:	wbscopc_tb.cpp $(TCOBJ) $(ROBJD)/Vwbscopc_tb.h wb_tb.h testb.h
	$(CXX) $(INCS) wbscopc_tb.cpp $(VSRCS) $(TCOBJ) -o $@
## }}}

.PHONY: test
## {{{
test:	wbscope_tb wbscopc_tb
	./wbscope_tb
	./wbscopc_tb
## }}}

.PHONY: clean
## {{{
clean:
	rm -f wbscope_tb     wbscopc_tb
	rm -f wbscope_tb.vcd wbscopc_tb.vcd
## }}}
