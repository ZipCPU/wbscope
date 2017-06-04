////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	wbscopc_tb.v
//
// Project:	WBScope, a wishbone hosted scope
//
// Purpose:	This file is a test bench wrapper around the compressed
//		wishbone scope, designed to create a "signal" which can then
//	be scoped and proven.  Unlike the case of the normal wishbone scope,
//	this scope needs a test signal that has lots of idle time surrounded
//	my sudden changes.  We'll handle our sudden changes via a counter.
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
module	wbscope_tb(i_clk,
		// i_rst is required by our test infrastructure, yet unused here
		i_rst,
		// The test data.  o_data is internally generated here from
		// o_counter, i_trigger is given externally
		i_trigger, o_data, o_counter,
		// Wishbone bus interaction
		i_wb_cyc, i_wb_stb, i_wb_we, i_wb_addr, i_wb_data,
		//	wishbone bus outputs
		o_wb_ack, o_wb_stall, o_wb_data,
		// And our output interrupt
		o_interrupt);
	input			i_clk, i_rst, i_trigger;
	output	wire	[30:0]	o_data;
	output	wire	[29:0]	o_counter;
	//
	input			i_wb_cyc, i_wb_stb, i_wb_we;
	input			i_wb_addr;
	input		[31:0]	i_wb_data;
	//
	output	wire		o_wb_ack;
	output	wire	[31:0]	o_wb_data;
	output	wire		o_wb_stall;
	//
	output	o_interrupt;

	reg	[29:0]	counter;
	initial	counter = 0;
	always @(posedge i_clk)
		counter <= counter + 1'b1;
	always @(posedge i_clk)
		if (counter[11:8] == 4'h0)
			o_data <= { i_trigger, counter };
		else if ((counter[10])&&(counter[1]))
			o_data <= { i_trigger, counter };
		else
			o_data <= { i_trigger, counter[29:12], 12'h0 };

	wire	wb_stall_ignored;

	wbscopc	#(.LGMEM(5'd14), .BUSW(32), .SYNCHRONOUS(1), .MAX_STEP(768),
			.DEFAULT_HOLDOFF(36))
		scope(i_clk, 1'b1, i_trigger, o_data,
			i_clk, i_wb_cyc, i_wb_stb, i_wb_we,
					i_wb_addr, i_wb_data,
				o_wb_ack, wb_stall_ignored, o_wb_data,
			o_interrupt);

	assign	o_wb_stall = 1'b0;

endmodule
