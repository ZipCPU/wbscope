################################################################################
##
## Filename: 	bench/formal/Makefile
## {{{
## Project:	WBScope, a wishbone hosted scope
##
## Purpose:	
##
## Creator:	Dan Gisselquist, Ph.D.
##		Gisselquist Technology, LLC
##
################################################################################
## }}}
## Copyright (C) 2021-2024, Gisselquist Technology, LLC
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
all: axilscope axisrle memscope
RTL := ../../rtl

.PHONY: axilscope
axilscope: axilscope_sync/PASS axilscope_async/PASS
axilscope_sync/PASS: axilscope.sby $(RTL)/axilscope.v faxil_slave.v
	sby -f axilscope.sby sync
axilscope_async/PASS: axilscope.sby $(RTL)/axilscope.v faxil_slave.v
	sby -f axilscope.sby async

.PHONY: axisrle
axisrle: axisrle_prf/PASS axisrle_prf8/PASS axisrle_prf16/PASS axisrle_cvr/PASS
axisrle_prf/PASS:   axisrle.sby $(RTL)/skidbuffer.v $(RTL)/axisrle.v
	sby -f axisrle.sby prf
axisrle_prf8/PASS:  axisrle.sby $(RTL)/skidbuffer.v $(RTL)/axisrle.v
	sby -f axisrle.sby prf8
axisrle_prf16/PASS: axisrle.sby $(RTL)/skidbuffer.v $(RTL)/axisrle.v
	sby -f axisrle.sby prf16
axisrle_cvr/PASS:   axisrle.sby $(RTL)/skidbuffer.v $(RTL)/axisrle.v
	sby -f axisrle.sby cvr

.PHONY: memscope
memscope: memscope_prf/PASS
AXI := faxi_master.v faxi_slave.v faxi_addr.v faxi_valaddr.v faxi_wstrb.v
WB2AXIP := ../../../../wb2axip/gitlab-axi/rtl
memscope_prf/PASS: memscope.sby $(RTL)/memscope.v $(WB2AXIP)/skidbuffer.v $(WB2AXIP)/sfifo.v $(AXI)
	sby -f memscope.sby prf

.PHONY: clean
clean:
	rm -rf axilscope_*/
	rm -rf axisrle_*/
	rm -rf memscope_*/

