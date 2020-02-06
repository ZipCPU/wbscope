////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	cfgscope.cpp
//
// Project:	WBScope, a wishbone hosted scope
//
// Purpose:	To read out, and decompose, the results of the wishbone scope
//		as applied to the ICAPE2 interaction.
//
//	This is provided together with the wbscope project as an example of
//	what might be done with the wishbone scope.  The intermediate details,
//	though, between this and the wishbone scope are not part of the
//	wishbone scope project.
//
//	Using this particular scope made it a *lot* easier to get the ICAPE2
//	interface up and running, since I was able to see what was going right
//	(or wrong) with the interface as I was developing it.  Sure, it
//	would've been better to get it to work under a simulator instead of
//	with the scope, but not being certain of how the interface was
//	supposed to work made building a simulator difficult.
//
// Creator:	Dan Gisselquist, Ph.D.
//		Gisselquist Technology, LLC
//
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2015-2020, Gisselquist Technology, LLC
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
#include "devbus.h"
#include "scopecls.h"

//
// CFGSCOPE
//
// When you wish to build your own scope, you'll need to build a version of this
// class to do so.  This class has two particular functions to it: one
// (optional) one to define the traces used incase we wish to split these apart
// for output to a VCD file.  The other function is for use with debug-by-printf
// approaches.  As a result, it provides for a more flexible (textual) output.
//
class	CFGSCOPE : public SCOPE {

	virtual	void	define_traces(void) {
		// Heres the interface for VCD files: We need to tell the VCD
		// writer the names of all of our traces, how many bits each
		// trace uses, and where the location of the value exists within
		// the 32-bit trace word.
		register_trace("cs_n",   1, 31);
		register_trace("we_n",   1, 30);
		register_trace("code",   6, 24);
		register_trace("value", 24,  0);
	}

	//
	// decode
	//
	// Decode the value to the standard-output stream.  How you decode this
	// value is up to you.  Prior to the value being printed, a prefix
	// identifying the clock number (as counted by the scope, with the
	// internal clock enable on), and the raw value will be printed out.
	// Further, after doing whatever printing you wish to do here, a newline
	// will be printed before going to the next value.
	//
	virtual	void	decode(DEVBUS::BUSW v) const {
		// Now, let's decompose our 32-bit wires into something ...
		// meaningful and dump it to stdout.  This section will change
		// from project to project, scope to scope, depending on what
		// wires are placed into the scope.
		printf("%s %s ", (v&0x80000000)?"  ":"CS",
			 	(v&0x40000000)?"RD":"WR");

		unsigned cw = (v>>24)&0x03f;
		switch(cw) {
			case	0x20: printf("DUMMY"); break;
			case	0x10: printf("NOOP "); break;
			case	0x08: printf("SYNC "); break;
			case	0x04: printf("CMD  "); break;
			case	0x02: printf("IPROG"); break;
			case	0x01: printf("DSYNC"); break;
			default:      printf("OTHER"); break;
		}
		printf(" -> %02x", v & 0x0ffffff);
	}
};

int main(int argc, char **argv) {
	// The DEVBUS structure encapsulates wishbone accesses, so that this
	// code can access the wishbone bus on the FPGA.
	DEVBUS	*m_fpga;

	// Open up a port to talk to the FPGA ...
	//
	// This may be unique to your FPGA, so feel free to adjust these lines
	// for your setup.  The result, though, must be a DEVBUS structure
	// giving you access to the FPGA.
#ifndef	FORCE_UART
	m_fpga = new FPGA(new NETCOMMS("lazarus",PORT));
#else
	m_fpga = new FPGA(new TTYCOMMS("/dev/ttyUSB2"));
#endif

	// 
	CFGSCOPE *scope = new CFGSCOPE(m_fpga, WBSCOPE);

	// Check to see whether or not the scope has captured the data we need
	// yet or not.
	if (scope->ready()) {
		// If the data has been captured, we call print().  This
		// function will print all our values to the standard output,
		// and it will call the decode() function above to do it.
		scope->print();

		// You can also write the results to a VCD trace file.  To do
		// this, just call writevcd and pass it the name you wish your
		// VCD file to have.
		scope->writevcd("cfgtrace.vcd");
	} else {
		// If the scope isnt yet ready, print a message, decode its
		// current state, and exit kindly.
		printf("Scope is not (yet) ready:\n");
		scope->decode_control();
	}

	// Clean up our interface, now, and we're done.
	delete	m_fpga;
}
