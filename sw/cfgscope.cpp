///////////////////////////////////////////////////////////////////////////
//
// Filename: 	cfgscope.cpp
//
// Project:	FPGA library development (Basys-3 development board)
//
// Purpose:	To read out, and decompose, the results of the wishbone scope
//		as applied to the ICAPE2 interaction.
//
//		This is provided together with the wbscope project as an
//		example of what might be done with the wishbone scope.
//		The intermediate details, though, between this and the
//		wishbone scope are not part of the wishbone scope project.
//
//		Using this particular scope made it a *lot* easier to get the
//		ICAPE2 interface up and running, since I was able to see what
//		was going right (or wrong) with the interface as I was 
//		developing it.  Sure, it would've been better to get it to work
//		under a simulator instead of with the scope, but not being
//		certain of how the interface was supposed to work made building
//		a simulator difficult.
//
// Creator:	Dan Gisselquist, Ph.D.
//		Gisselquist Technology, LLC
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2015, Gisselquist Technology, LLC
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
// with this program.  (It's in the $(ROOT)/doc directory, run make with no
// target there if the PDF file isn't present.)  If not, see
// <http://www.gnu.org/licenses/> for a copy.
//
// License:	GPL, v3, as defined and found on www.gnu.org,
//		http://www.gnu.org/licenses/gpl.html
//
//
///////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include "port.h"
#include "llcomms.h"	// This defines how we talk to the device over wishbone
#include "regdefs.h"

// Here are the two registers needed for accessing our scope: A control register
// and a data register.  
#define	WBSCOPE		R_CFGSCOPE
#define	WBSCOPEDATA	R_CFGSCOPED

//
// The DEVBUS structure encapsulates wishbone accesses, so that this code can
// access the wishbone bus on the FPGA.
DEVBUS	*m_fpga;
void	closeup(int v) {
	m_fpga->kill();
	exit(0);
}

int main(int argc, char **argv) {
	// Open up a port to talk to the FPGA ...
#ifndef	FORCE_UART
	m_fpga = new FPGA(new NETCOMMS("lazarus",PORT));
#else
	m_fpga = new FPGA(new TTYCOMMS("/dev/ttyUSB2"));
#endif

	signal(SIGSTOP, closeup);
	signal(SIGHUP, closeup);

	// Check to see whether or not the scope has captured the data we need
	// yet or not.  If not, exit kindly.
	unsigned	v, lgln, scoplen;
	v = m_fpga->readio(WBSCOPE);
	if (0x60000000 != (v & 0x60000000)) {
		printf("Scope is not yet ready:\n");
		printf("\tRESET:\t\t%s\n", (v&0x80000000)?"Ongoing":"Complete");
		printf("\tSTOPPED:\t%s\n", (v&0x40000000)?"Yes":"No");
		printf("\tTRIGGERED:\t%s\n", (v&0x20000000)?"Yes":"No");
		printf("\tPRIMED:\t\t%s\n", (v&0x10000000)?"Yes":"No");
		printf("\tMANUAL:\t\t%s\n", (v&0x08000000)?"Yes":"No");
		printf("\tDISABLED:\t%s\n", (v&0x04000000)?"Yes":"No");
		printf("\tZERO:\t\t%s\n", (v&0x02000000)?"Yes":"No");
		exit(0);
	}

	// Since the length of the scope memory is a configuration parameter
	// internal to the scope, we read it here to find out how it was
	// configured.
	lgln = (v>>20) & 0x1f;
	scoplen = (1<<lgln);

	DEVBUS::BUSW	*buf;
	buf = new DEVBUS::BUSW[scoplen];

	// There are two means of reading from a DEVBUS interface: The first
	// is a vector read, optimized so that the address and read command
	// only needs to be sent once.  This is the optimal means.  However,
	// if the bus isn't (yet) trustworthy, it may be more reliable to access
	// the port by reading one register at a time--hence the second method.
	// If the bus works, you'll want to use readz(): read scoplen values
	// into the buffer, from the address WBSCOPEDATA, without incrementing
	// the address each time (hence the 'z' in readz--for zero increment).
	if (true) {
		m_fpga->readz(WBSCOPEDATA, scoplen, buf);

		printf("Vector read complete\n");
	} else {
		for(int i=0; i<scoplen; i++)
			buf[i] = m_fpga->readio(WBSCOPEDATA);
	}

	// Now, let's decompose our 32-bit wires into something ... meaningful.
	// This section will change from project to project, scope to scope,
	// depending on what wires are placed into the scope.
	for(int i=0; i<scoplen; i++) {
		if ((i>0)&&(buf[i] == buf[i-1])&&
				(i<scoplen-1)&&(buf[i] == buf[i+1]))
			continue;
		printf("%6d %08x:", i, buf[i]);
		printf("%s %s ", (buf[i]&0x80000000)?"  ":"CS",
				 (buf[i]&0x40000000)?"RD":"WR");
		unsigned cw = (buf[i]>>24)&0x03f;
		switch(cw) {
			case	0x20: printf("DUMMY"); break;
			case	0x10: printf("NOOP "); break;
			case	0x08: printf("SYNC "); break;
			case	0x04: printf("CMD  "); break;
			case	0x02: printf("IPROG"); break;
			case	0x01: printf("DSYNC"); break;
			default:      printf("OTHER"); break;
		}
		printf(" -> %02x\n", buf[i] & 0x0ffffff);
	}

	// Clean up our interface, now, and we're done.
	delete	m_fpga;
}

