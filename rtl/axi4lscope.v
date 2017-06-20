`timescale 1 ns / 1 ps
////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	axi4lscope.v
//
// Project:	WBScope, a wishbone hosted scope
//
// Purpose:	This is a generic/library routine for providing a bus accessed
//	'scope' or (perhaps more appropriately) a bus accessed logic analyzer.
//	The general operation is such that this 'scope' can record and report
//	on any 32 bit value transiting through the FPGA.  Once started and
//	reset, the scope records a copy of the input data every time the clock
//	ticks with the circuit enabled.  That is, it records these values up
//	until the trigger.  Once the trigger goes high, the scope will record
//	for br_holdoff more counts before stopping.  Values may then be read
//	from the buffer, oldest to most recent.  After reading, the scope may
//	then be reset for another run.
//
//	In general, therefore, operation happens in this fashion:
//		1. A reset is issued.
//		2. Recording starts, in a circular buffer, and continues until
//		3. The trigger line is asserted.
//			The scope registers the asserted trigger by setting
//			the 'o_triggered' output flag.
//		4. A counter then ticks until the last value is written
//			The scope registers that it has stopped recording by
//			setting the 'o_stopped' output flag.
//		5. The scope recording is then paused until the next reset.
//		6. While stopped, the CPU can read the data from the scope
//		7. -- oldest to most recent
//		8. -- one value per i_rd&i_data_clk
//		9. Writes to the data register reset the address to the
//			beginning of the buffer
//
//	Although the data width DW is parameterized, it is not very changable,
//	since the width is tied to the width of the data bus, as is the 
//	control word.  Therefore changing the data width would require changing
//	the interface.  It's doable, but it would be a change to the interface.
//
//	The SYNCHRONOUS parameter turns on and off meta-stability
//	synchronization.  Ideally a wishbone scope able to handle one or two
//	clocks would have a changing number of ports as this SYNCHRONOUS
//	parameter changed.  Other than running another script to modify
//	this, I don't know how to do that so ... we'll just leave it running
//	off of two clocks or not.
//
//
//	Internal to this routine, registers and wires are named with one of the
//	following prefixes:
//
//	i_	An input port to the routine
//	o_	An output port of the routine
//	br_	A register, controlled by the bus clock
//	dr_	A register, controlled by the data clock
//	bw_	A wire/net, controlled by the bus clock
//	dw_	A wire/net, controlled by the data clock
//
//	And, of course, since AXI wants to be particular about their port
//	naming conventions, anything beginning with
//
//	S_AXI_
//
//	is a signal associated with this function as an AXI slave.
//	
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
`default_nettype	none
//
module axi4lscope
	#(
		// Users to add parameters here
		parameter [4:0]	LGMEM = 5'd10,
		parameter	BUSW = 32,
		parameter	SYNCHRONOUS=1,
		parameter	HOLDOFFBITS = 20,
		parameter [(HOLDOFFBITS-1):0]	DEFAULT_HOLDOFF
						= ((1<<(LGMEM-1))-4),
		// User parameters ends
		// DO NOT EDIT BELOW THIS LINE ---------------------
		// Do not modify the parameters beyond this line
		// Width of S_AXI data bus
		parameter integer C_S_AXI_DATA_WIDTH	= 32,
		// Width of S_AXI address bus
		parameter integer C_S_AXI_ADDR_WIDTH	= 4
	)
	(
		// Users to add ports here
		input wire	i_data_clk, // The data clock, can be set to ACLK
		input wire	i_ce,	// = '1' when recordable data is present
		input wire	i_trigger,// = '1' when interesting event hapns
		input wire	[31:0]	i_data,
		output	wire	o_interrupt,	// ='1' when scope has stopped
		// User ports ends
		// DO NOT EDIT BELOW THIS LINE ---------------------
		// Do not modify the ports beyond this line
		// Global Clock Signal
		input wire  S_AXI_ACLK,
		// Global Reset Signal. This Signal is Active LOW
		input wire  S_AXI_ARESETN,
		// Write address (issued by master, acceped by Slave)
		input wire [C_S_AXI_ADDR_WIDTH-1 : 0] S_AXI_AWADDR,
		// Write channel Protection type. This signal indicates the
    		// privilege and security level of the transaction, and whether
    		// the transaction is a data access or an instruction access.
		input wire [2 : 0] S_AXI_AWPROT,
		// Write address valid. This signal indicates that the master
    		// signaling valid write address and control information.
		input wire  S_AXI_AWVALID,
		// Write address ready. This signal indicates that the slave
    		// is ready to accept an address and associated control signals.
		output wire  S_AXI_AWREADY,
		// Write data (issued by master, acceped by Slave) 
		input wire [C_S_AXI_DATA_WIDTH-1 : 0] S_AXI_WDATA,
		// Write strobes. This signal indicates which byte lanes hold
    		// valid data. There is one write strobe bit for each eight
    		// bits of the write data bus.    
		input wire [(C_S_AXI_DATA_WIDTH/8)-1 : 0] S_AXI_WSTRB,
		// Write valid. This signal indicates that valid write
    		// data and strobes are available.
		input wire  S_AXI_WVALID,
		// Write ready. This signal indicates that the slave
    		// can accept the write data.
		output wire  S_AXI_WREADY,
		// Write response. This signal indicates the status
    		// of the write transaction.
		output wire [1 : 0] S_AXI_BRESP,
		// Write response valid. This signal indicates that the channel
    		// is signaling a valid write response.
		output wire  S_AXI_BVALID,
		// Response ready. This signal indicates that the master
    		// can accept a write response.
		input wire  S_AXI_BREADY,
		// Read address (issued by master, acceped by Slave)
		input wire [C_S_AXI_ADDR_WIDTH-1 : 0] S_AXI_ARADDR,
		// Protection type. This signal indicates the privilege
    		// and security level of the transaction, and whether the
    		// transaction is a data access or an instruction access.
		input wire [2 : 0] S_AXI_ARPROT,
		// Read address valid. This signal indicates that the channel
    		// is signaling valid read address and control information.
		input wire  S_AXI_ARVALID,
		// Read address ready. This signal indicates that the slave is
    		// ready to accept an address and associated control signals.
		output wire  S_AXI_ARREADY,
		// Read data (issued by slave)
		output wire [C_S_AXI_DATA_WIDTH-1 : 0] S_AXI_RDATA,
		// Read response. This signal indicates the status of the
    		// read transfer.
		output wire [1 : 0] S_AXI_RRESP,
		// Read valid. This signal indicates that the channel is
    		// signaling the required read data.
		output wire  S_AXI_RVALID,
		// Read ready. This signal indicates that the master can
    		// accept the read data and response information.
		input wire  S_AXI_RREADY
		// DO NOT EDIT ABOVE THIS LINE ---------------------
	);

	// AXI4LITE signals
	reg [C_S_AXI_ADDR_WIDTH-1 : 0] 	axi_awaddr;
	reg			 	axi_awready;
	reg 				axi_wready;
	// reg		[1 : 0] 	axi_bresp;
	reg 				axi_bvalid;
	reg [C_S_AXI_ADDR_WIDTH-1 : 0] 	axi_araddr;
	reg 			 	axi_arready;
	// reg		 [1 : 0] 	axi_rresp;
	reg  				axi_rvalid;


	wire	write_stb;

	///////////////////////////////////////////////////
	//
	// Decode and handle the AXI/Bus signaling
	//
	///////////////////////////////////////////////////
	//
	// Sadly, the AXI bus is *way* more complicated to
	// deal with than it needs to be.  Still, we offer
	// the following as a simple means of dealing with
	// it.  The majority of the code in this section 
	// comes directly from a Xilinx/Vivado generated
	// file.
	//
	// Gisselquist Technology, LLC, claims no copyright
	// or ownership of this section of the code.
	//
	wire	i_reset;
	assign	i_reset = !S_AXI_ARESETN;

	always @(posedge S_AXI_ACLK)
		if (i_reset)
			axi_awready <= 1'b0;
		else if ((!axi_awready)&&(S_AXI_AWVALID)&&(S_AXI_WVALID))
			axi_awready <= 1'b1;
		else
			axi_awready <= 1'b0;
	assign	S_AXI_AWREADY = axi_awready;

	always @(posedge S_AXI_ACLK)
		if ((!axi_awready)&&(S_AXI_AWVALID)&&(S_AXI_WVALID))
			axi_awaddr <= S_AXI_AWADDR;

	always @(posedge S_AXI_ACLK)
		if (i_reset)
			axi_wready <= 1'b0;
		else if ((!axi_wready)&&(S_AXI_WVALID)&&(S_AXI_AWVALID))
			axi_wready <= 1'b1;
		else
			axi_wready <= 1'b0;
	assign	S_AXI_WREADY = axi_wready;

	always @(posedge S_AXI_ACLK)
		if (i_reset)
		begin
			axi_bvalid <= 0;
			// axi_bresp <= 2'b00;
		end else if ((~axi_bvalid)&&(write_stb))
		begin
			axi_bvalid <= 1'b1;
			// axi_bresp <= 2'b00; // 'Okay' response
		end else if ((S_AXI_BREADY)&&(axi_bvalid))
			axi_bvalid <= 1'b0;
	assign	S_AXI_BRESP = 2'b00;	// An 'OKAY' response
	assign	S_AXI_BVALID= axi_bvalid;



	always @(posedge S_AXI_ACLK)
		if (i_reset)
		begin
			axi_arready <= 1'b0;
			axi_araddr <= 0;
		end else if ((!axi_arready)&&(S_AXI_ARVALID))
		begin
			axi_arready <= 1'b1;
			axi_araddr <= S_AXI_ARADDR;
		end else
			axi_arready <= 1'b0;
	assign	S_AXI_ARREADY = axi_arready;

	always @(posedge S_AXI_ACLK)
		if (i_reset)
		begin
			axi_rvalid <= 0;
			// axi_rresp  <= 0;
		end else if ((axi_arready)&&(S_AXI_ARVALID)&&(!axi_rvalid))
		begin
			axi_rvalid <= 1'b0;
			// axi_rresp <= 2'b00;
		end else if ((axi_rvalid)&&(S_AXI_RREADY))
			axi_rvalid <= 1'b0;
	assign	S_AXI_RVALID = axi_rvalid;
	assign	S_AXI_RRESP  = 2'b00;




	///////////////////////////////////////////////////
	//
	// Final simplification of the AXI code
	//
	///////////////////////////////////////////////////
	//
	// Now that we've provided all of the bus signaling
	// above, can we make any sense of it?
	//
	// The following wires are here to provide some
	// simplification of the complex bus protocol.  In
	// particular, are we reading or writing during this
	// clock?  The two *should* be mutually exclusive
	// (i.e., you *shouldn't* be able to both read and
	// write on the same clock) ... but Xilinx's default
	// implementation does nothing to ensure that this
	// would be the case.
	//
	// From here on down, Gisselquist Technology, LLC,
	// claims a copyright on the code.
	//
	wire	bus_clock;
	assign	bus_clock = S_AXI_ACLK;

	wire	read_from_data;
	assign	read_from_data = (S_AXI_ARVALID)&&(S_AXI_ARREADY)
					&&(axi_araddr[0]);

	assign	write_stb = ((axi_awready)&&(S_AXI_AWVALID)
				&&(axi_wready)&&(S_AXI_WVALID));
	wire	write_to_control;
	assign	write_to_control = (write_stb)&&(!axi_awaddr[0]);

	reg	read_address;
	always @(posedge bus_clock)
		read_address <= axi_araddr[0];

	wire	[31:0]	i_wb_data;
	assign	i_wb_data = S_AXI_WDATA;


	///////////////////////////////////////////////////
	//
	// The actual SCOPE
	//
	///////////////////////////////////////////////////
	//
	// Now that we've finished reading/writing from the
	// bus, ... or at least acknowledging reads and 
	// writes from and to the bus--even if they haven't
	// happened yet, now we implement our actual scope.
	// This includes implementing the actual reads/writes
	// from/to the bus.
	//
	// From here on down, is the heart of the scope itself.
	//
	reg	[(LGMEM-1):0]	raddr;
	reg	[(BUSW-1):0]	mem[0:((1<<LGMEM)-1)];

	// Our status/config register
	wire		bw_reset_request, bw_manual_trigger,
			bw_disable_trigger, bw_reset_complete;
	reg	[2:0]	br_config;
	reg	[(HOLDOFFBITS-1):0]	br_holdoff;
	initial	br_config = 3'b0;
	initial	br_holdoff = DEFAULT_HOLDOFF;
	always @(posedge bus_clock)
		if (write_to_control)
		begin
			br_config <= { i_wb_data[31],
				i_wb_data[27],
				i_wb_data[26] };
			br_holdoff <= i_wb_data[(HOLDOFFBITS-1):0];
		end else if (bw_reset_complete)
			br_config[2] <= 1'b1;
	assign	bw_reset_request   = (!br_config[2]);
	assign	bw_manual_trigger  = (br_config[1]);
	assign	bw_disable_trigger = (br_config[0]);

	wire	dw_reset, dw_manual_trigger, dw_disable_trigger;
	generate
	if (SYNCHRONOUS > 0)
	begin
		assign	dw_reset = bw_reset_request;
		assign	dw_manual_trigger = bw_manual_trigger;
		assign	dw_disable_trigger = bw_disable_trigger;
		assign	bw_reset_complete = bw_reset_request;
	end else begin
		reg		r_reset_complete;
		(* ASYNC_REG = "TRUE" *) reg	[2:0]	q_iflags;
		reg	[2:0]	r_iflags;

		// Resets are synchronous to the bus clock, not the data clock
		// so do a clock transfer here
		initial	q_iflags = 3'b000;
		initial	r_reset_complete = 1'b0;
		always @(posedge i_data_clk)
		begin
			q_iflags <= { bw_reset_request, bw_manual_trigger, bw_disable_trigger };
			r_iflags <= q_iflags;
			r_reset_complete <= (dw_reset);
		end

		assign	dw_reset = r_iflags[2];
		assign	dw_manual_trigger = r_iflags[1];
		assign	dw_disable_trigger = r_iflags[0];

		(* ASYNC_REG = "TRUE" *) reg	q_reset_complete;
		reg	qq_reset_complete;
		// Pass an acknowledgement back from the data clock to the bus
		// clock that the reset has been accomplished
		initial	q_reset_complete = 1'b0;
		initial	qq_reset_complete = 1'b0;
		always @(posedge bus_clock)
		begin
			q_reset_complete  <= r_reset_complete;
			qq_reset_complete <= q_reset_complete;
		end

		assign bw_reset_complete = qq_reset_complete;
	end endgenerate

	//
	// Set up the trigger
	//
	//
	// Write with the i-clk, or input clock.  All outputs read with the
	// bus clock, or bus_clock  as we've called it here.
	reg	dr_triggered, dr_primed;
	wire	dw_trigger;
	assign	dw_trigger = (dr_primed)&&(
				((i_trigger)&&(!dw_disable_trigger))
				||(dw_manual_trigger));
	initial	dr_triggered = 1'b0;
	always @(posedge i_data_clk)
		if (dw_reset)
			dr_triggered <= 1'b0;
		else if ((i_ce)&&(dw_trigger))
			dr_triggered <= 1'b1;

	//
	// Determine when memory is full and capture is complete
	//
	// Writes take place on the data clock
	// The counter is unsigned
	(* ASYNC_REG="TRUE" *) reg	[(HOLDOFFBITS-1):0]	counter;

	reg		dr_stopped;
	initial	dr_stopped = 1'b0;
	initial	counter = 0;
	always @(posedge i_data_clk)
		if (dw_reset)
			counter <= 0;
		else if ((i_ce)&&(dr_triggered)&&(!dr_stopped))
		begin
			counter <= counter + 1'b1;
		end
	always @(posedge i_data_clk)
		if ((!dr_triggered)||(dw_reset))
			dr_stopped <= 1'b0;
		else if (HOLDOFFBITS > 1) // if (i_ce)
			dr_stopped <= (counter >= br_holdoff);
		else if (HOLDOFFBITS <= 1)
			dr_stopped <= ((i_ce)&&(dw_trigger));

	//
	//	Actually do our writes to memory.  Record, via 'primed' when
	//	the memory is full.
	//
	//	The 'waddr' address that we are using really crosses two clock
	//	domains.  While writing and changing, it's in the data clock
	//	domain.  Once stopped, it becomes part of the bus clock domain.
	//	The clock transfer on the stopped line handles the clock
	//	transfer for these signals.
	//
	reg	[(LGMEM-1):0]	waddr;
	initial	waddr = {(LGMEM){1'b0}};
	initial	dr_primed = 1'b0;
	always @(posedge i_data_clk)
		if (dw_reset) // For simulation purposes, supply a valid value
		begin
			waddr <= 0; // upon reset.
			dr_primed <= 1'b0;
		end else if ((i_ce)&&(!dr_stopped))
		begin
			// mem[waddr] <= i_data;
			waddr <= waddr + {{(LGMEM-1){1'b0}},1'b1};
			if (!dr_primed)
			begin
				//if (br_holdoff[(HOLDOFFBITS-1):LGMEM]==0)
				//	dr_primed <= (waddr >= br_holdoff[(LGMEM-1):0]);
				// else
				
					dr_primed <= (&waddr);
			end
		end

	// Delay the incoming data so that we can get our trigger
	// logic to line up with the data.  The goal is to have a
	// hold off of zero place the trigger in the last memory
	// address.
	localparam	STOPDELAY = 1;
	wire	[(BUSW-1):0]		wr_piped_data;
	generate
	if (STOPDELAY == 0)
		// No delay ... just assign the wires to our input lines
		assign	wr_piped_data = i_data;
	else if (STOPDELAY == 1)
	begin
		//
		// Delay by one means just register this once
		reg	[(BUSW-1):0]	data_pipe;
		always @(posedge i_data_clk)
			if (i_ce)
				data_pipe <= i_data;
		assign	wr_piped_data = data_pipe;
	end else begin
		// Arbitrary delay ... use a longer pipe
		reg	[(STOPDELAY*BUSW-1):0]	data_pipe;

		always @(posedge i_data_clk)
			if (i_ce)
				data_pipe <= { data_pipe[((STOPDELAY-1)*BUSW-1):0], i_data };
		assign	wr_piped_data = { data_pipe[(STOPDELAY*BUSW-1):((STOPDELAY-1)*BUSW)] };
	end endgenerate

	always @(posedge i_data_clk)
		if ((i_ce)&&(!dr_stopped))
			mem[waddr] <= wr_piped_data;

	//
	// Clock transfer of the status signals
	//
	wire	bw_stopped, bw_triggered, bw_primed;
	generate
	if (SYNCHRONOUS > 0)
	begin
		assign	bw_stopped   = dr_stopped;
		assign	bw_triggered = dr_triggered;
		assign	bw_primed    = dr_primed;
	end else begin
		// These aren't a problem, since none of these are strobe
		// signals.  They goes from low to high, and then stays high
		// for many clocks.  Swapping is thus easy--two flip flops to
		// protect against meta-stability and we're done.
		//
		(* ASYNC_REG = "TRUE" *) reg	[2:0]	q_oflags;
		reg	[2:0]	r_oflags;
		initial	q_oflags = 3'h0;
		initial	r_oflags = 3'h0;
		always @(posedge bus_clock)
			if (bw_reset_request)
			begin
				q_oflags <= 3'h0;
				r_oflags <= 3'h0;
			end else begin
				q_oflags <= { dr_stopped, dr_triggered, dr_primed };
				r_oflags <= q_oflags;
			end

		assign	bw_stopped   = r_oflags[2];
		assign	bw_triggered = r_oflags[1];
		assign	bw_primed    = r_oflags[0];
	end endgenerate

	// Reads use the bus clock
	always @(posedge bus_clock)
	begin
		if ((bw_reset_request)||(write_to_control))
			raddr <= 0;
		else if ((read_from_data)&&(bw_stopped))
			raddr <= raddr + 1'b1; // Data read, when stopped
	end

	reg	[(LGMEM-1):0]	this_addr;
	always @(posedge bus_clock)
		if (read_from_data)
			this_addr <= raddr + waddr + 1'b1;
		else
			this_addr <= raddr + waddr;

	reg	[31:0]	nxt_mem;
	always @(posedge bus_clock)
		nxt_mem <= mem[this_addr];

	wire	[19:0]	full_holdoff;
	assign full_holdoff[(HOLDOFFBITS-1):0] = br_holdoff;
	generate if (HOLDOFFBITS < 20)
		assign full_holdoff[19:(HOLDOFFBITS)] = 0;
	endgenerate

	reg	[31:0]	o_bus_data;
	wire	[4:0]	bw_lgmem;
	assign		bw_lgmem = LGMEM;
	always @(posedge bus_clock)
		if (!read_address) // Control register read
			o_bus_data <= { bw_reset_request,
					bw_stopped,
					bw_triggered,
					bw_primed,
					bw_manual_trigger,
					bw_disable_trigger,
					(raddr == {(LGMEM){1'b0}}),
					bw_lgmem,
					full_holdoff  };
		else if (!bw_stopped) // read, prior to stopping
			o_bus_data <= i_data;
		else // if (i_wb_addr) // Read from FIFO memory
			o_bus_data <= nxt_mem; // mem[raddr+waddr];

	assign	S_AXI_RDATA = o_bus_data;

	reg	br_level_interrupt;
	initial	br_level_interrupt = 1'b0;
	assign	o_interrupt = (bw_stopped)&&(!bw_disable_trigger)
					&&(!br_level_interrupt);
	always @(posedge bus_clock)
		if ((bw_reset_complete)||(bw_reset_request))
			br_level_interrupt<= 1'b0;
		else
			br_level_interrupt<= (bw_stopped)&&(!bw_disable_trigger);

	// verilator lint_off UNUSED
	// Make verilator happy
	wire	[44:0]	unused;
	assign unused = { S_AXI_WSTRB, S_AXI_ARPROT, S_AXI_AWPROT,
		axi_awaddr[3:1], axi_araddr[3:1],
		i_wb_data[30:28], i_wb_data[25:0] };
	// verilator lint_on UNUSED
endmodule
