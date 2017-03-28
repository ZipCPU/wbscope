////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	wbscope_tb.v
//
// Project:	WBScope, a wishbone hosted scope
//
// Purpose:	
//
// Creator:	Dan Gisselquist, Ph.D.
//		Gisselquist Technology, LLC
//
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2015-2017, Gisselquist Technology, LLC
//
// This program is free software (firmware): you can redistribute it and/or
// modify it under the terms of  the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or (at
// your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTIBILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program.  (It's in the $(ROOT)/doc directory.  Run make with no
// target there if the PDF file isn't present.)  If not, see
// <http://www.gnu.org/licenses/> for a copy.
//
// License:	GPL, v3, as defined and found on www.gnu.org,
//		http://www.gnu.org/licenses/gpl.html
//
//
////////////////////////////////////////////////////////////////////////////////
//
//
module	wbscope_tb(i_clk, i_rst, i_trigger, o_data,
	i_wb_cyc, i_wb_stb, i_wb_we, i_wb_addr, i_wb_data,
	o_wb_ack, o_wb_data, o_interrupt);
	input			i_clk, i_rst, i_trigger;
	output	wire	[31:0]	o_data;
	//
	input			i_wb_cyc, i_wb_stb, i_wb_we;
	input			i_wb_addr;
	input		[31:0]	i_wb_data;
	//
	output	wire		o_wb_ack;
	output	wire	[31:0]	o_wb_data;
	//
	output	o_interrupt;

	reg	[30:0]	counter;
	initial	counter = 0;
	always @(posedge i_clk)
		counter <= counter + 1'b1;

	assign	o_data = { i_trigger, counter };

	wire	wb_stall_ignored;

	wbscope	#(5'd6, 32, 1)
		scope(i_clk, 1'b1, i_trigger, o_data,
			i_clk, i_wb_cyc, i_wb_stb, i_wb_we,
					i_wb_addr, i_wb_data,
				o_wb_ack, wb_stall_ignored, o_wb_data,
			o_interrupt);

endmodule
