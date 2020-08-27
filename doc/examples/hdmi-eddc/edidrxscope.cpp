////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	edidrxscope.cpp
//
// Project:	OpenArty, an entirely open SoC based upon the Arty platform
//
// Purpose:	To communicate with a generic scope, specifically the one for
//		testing the I2C communication path associated with an EDID
//	data set.  Further, this file defines what the various wires are
//	on that path, as well as the fact that the scope is compressed.
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include "port.h"
#include "regdefs.h"
#include "scopecls.h"
#include "ttybus.h"

#define	WBSCOPE		R_EDID_SCOPC
#define	WBSCOPEDATA	R_EDID_SCOPD

FPGA	*m_fpga;

class	EDIDRXSCOPE : public SCOPE {
public:
	EDIDRXSCOPE(FPGA *fpga, unsigned addr, bool vecread=true)
		: SCOPE(fpga, addr, true, vecread) {};
	~EDIDRXSCOPE(void) {}

	virtual	void	define_traces(void) {
		register_trace("i_scl", 1, 3);
		register_trace("i_sda", 1, 2);
		register_trace("o_scl", 1, 1);
		register_trace("o_sda", 1, 0);
	}

	virtual	void	decode(DEVBUS::BUSW val) const {
		int	i_sck, i_sda, o_sck, o_sda;

		i_sck = (val>>3)&1;
		i_sda = (val>>2)&1;
		o_sck = (val>>1)&1;
		o_sda = (val   )&1;

		printf("CMD[%s %s] RCVD[%s %s]",
			(o_sck)?"SCK":"   ", (o_sda)?"SDA":"   ",
			(i_sck)?"SCK":"   ", (i_sda)?"SDA":"   ");
	}
};

int main(int argc, char **argv) {
	// Open and connect to our FPGA.  This macro needs to be defined in the
	// include files above.
	FPGAOPEN(m_fpga);

	// Here, we open a scope.  An EDIDRXSCOPE specifically.  The difference
	// between an EDIDRXSCOPE and any other scope is ... that the
	// EDIDRXSCOPE has particular things wired to particular bits, whereas
	// a generic scope ... just has data.  Well, that and the EDIDRXSCOPE
	// is a compressed scope, whereas a generic scope could be either.
	EDIDRXSCOPE *scope = new EDIDRXSCOPE(m_fpga, WBSCOPE);

	if (!scope->ready()) {
		// If we get here, then ... nothing started the scope.
		// It either hasn't primed, hasn't triggered, or hasn't finished
		// recording yet.  Trying to read data would do nothing but
		// read garbage, so we don't try.
		printf("Scope is not yet ready:\n");
		scope->decode_control();
	} else {
		// The scope has been primed, triggered, the holdoff wait 
		// period has passed, and the scope has now stopped.
		//
		// Hence we can read from our scope the values we need.
		scope->print();
		// If we want, we can also write out a VCD file with the data
		// we just read.
		scope->writevcd("edid.vcd");
	}

	// Now, we're all done.  Let's be nice to our interface and shut it
	// down gracefully, rather than letting the O/S do it in ... whatever
	// manner it chooses.
	delete	m_fpga;
}
