################################################################################
##
## Filename: 	Makefile
##
## Project:	WBScope, a wishbone hosted scope
##
## Purpose:	This is the master Makefile for the project.  It coordinates
##		the build of a Verilator test, "proving" that this core works
##	(to the extent that any simulated test "proves" anything).  This
##	make file depends upon the proper setup of Verilator.
##
## Creator:	Dan Gisselquist, Ph.D.
##		Gisselquist Technology, LLC
##
################################################################################
##
## Copyright (C) 2015-2020, Gisselquist Technology, LLC
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
all: rtl bench test
SUBMAKE := $(MAKE) --no-print-directory -C

.PHONY: doc
doc:
	$(SUBMAKE) doc

.PHONY: rtl
rtl:
	$(SUBMAKE) rtl

.PHONY: bench
bench: rtl
	$(SUBMAKE) bench/rtl
	$(SUBMAKE) bench/cpp

.PHONY: test
test: bench
	$(SUBMAKE) bench/cpp test

.PHONY: clean
clean:
	$(SUBMAKE) rtl        clean
	$(SUBMAKE) bench/rtl  clean
	$(SUBMAKE) bench/cpp  clean
	$(SUBMAKE) doc        clean


