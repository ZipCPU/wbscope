////////////////////////////////////////////////////////////////////////////////
//
// Filename:	wbscopc_tb.cpp
// {{{
// Project:	WBScope, a wishbone hosted scope
//
// Purpose:	A quick test bench to determine if the run-length encoded
//		wbscopc module works.
//
// Creator:	Dan Gisselquist, Ph.D.
//		Gisselquist Technology, LLC
//
////////////////////////////////////////////////////////////////////////////////
// }}}
// Copyright (C) 2015-2023, Gisselquist Technology, LLC
// {{{
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
// }}}
// License:	GPL, v3, as defined and found on www.gnu.org,
// {{{
//		http://www.gnu.org/licenses/gpl.html
//
////////////////////////////////////////////////////////////////////////////////
//
// }}}
#include <stdio.h>

#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Vwbscopc_tb.h"
#include "testb.h"
#include "devbus.h"
#define	INTERRUPTWIRE	o_interrupt
#include "wb_tb.h"

const int LGMEMSIZE = 15;

class	WBSCOPC_TB : public WB_TB<Vwbscopc_tb> {
	bool		m_debug;
public:
	// {{{
	WBSCOPC_TB(void) {
		m_debug = true;
	}

	void	tick(void) {

		WB_TB<Vwbscopc_tb>::tick();

		bool	writeout = true;
		if ((m_debug)&&(writeout)) {}
	}

	void reset(void) {
		m_core->i_reset    = 1;
		m_core->i_wb_cyc = 0;
		m_core->i_wb_stb = 0;
		tick();
		m_core->i_reset  = 0;
	}

	unsigned	trigger(void) {
		m_core->i_trigger = 1;
		idle();
		m_core->i_trigger = 0;
		printf("TRIGGERED AT %08x\n", m_core->o_data);
		return m_core->o_data;
	}

	bool	debug(void) const { return m_debug; }
	bool	debug(bool nxtv) { return m_debug = nxtv; }
	// }}}
};

int main(int  argc, char **argv) {
	Verilated::commandArgs(argc, argv);
	WBSCOPC_TB	*tb = new WBSCOPC_TB;
	unsigned	v, addr, trigger_addr;
	unsigned *buf;
	int	trigpt;

	tb->opentrace("wbscopc_tb.vcd");
	printf("Giving the core 2 cycles to start up\n");
	// Before testing, let's give the unit time enough to warm up
	tb->reset();
	tb->idle(2);

#define	WBSCOPE_STATUS	0
#define	WBSCOPE_DATA	4
#define	WBSCOPE_NORESET	0x80000000
#define	WBSCOPE_TRIGGER	(WBSCOPE_NO_RESET|0x08000000)
#define	WBSCOPE_MANUAL	(WBSCOPE_TRIGGER)
#define	WBSCOPE_PRIMED	0x10000000
#define	WBSCOPE_TRIGGERED 0x20000000
#define	WBSCOPE_STOPPED 0x40000000
#define	WBSCOPE_DISABLED  0x04000000
#define	WBSCOPE_LGLEN(A)	((A>>20)&0x01f)
#define	WBSCOPE_LENGTH(A)	(1<<(LGLEN(A)))

	// First test ... read the status register
	v = tb->readio(WBSCOPE_STATUS);
	int ln = WBSCOPE_LGLEN(v);
	printf("V   = %08x\n", v);
	printf("LN  = %d, or %d entries\n", ln, (1<<ln));
	printf("DLY = %d\n", (v&0xfffff));
	if (((1<<ln) < tb->m_tickcount)&&(v&0x10000000)) {
		printf("SCOPE is already triggered! ??\n");
		goto test_failure;
	}
	buf = new unsigned[(1<<ln)];

	tb->idle((1<<(12+4)) + (1<<ln) +240);

	v = tb->readio(WBSCOPE_STATUS);
	if ((v&WBSCOPE_PRIMED)==0) {
		printf("v = %08x\n", v);
		printf("SCOPE hasn\'t primed! ??\n");
		goto test_failure;
	}

	tb->trigger();
	v = tb->readio(WBSCOPE_STATUS);
	if ((v&WBSCOPE_TRIGGERED)==0) {
		printf("v = %08x\n", v);
		printf("SCOPE hasn\'t triggered! ??\n");
		goto test_failure;
	}

	while((v & WBSCOPE_STOPPED)==0)
		v = tb->readio(WBSCOPE_STATUS);
	printf("SCOPE has stopped, reading data\n");
	// {{{
	tb->readz(WBSCOPE_DATA, (1<<ln), buf);
	addr = 0;
	trigger_addr = 0xffffffff;
	for(int i=0; i<(1<<ln); i++) {
		if (buf[i] & 0x80000000)
			addr += (buf[i]&0x7fffffff) + 1;
		else {
			if ((i > 0)&&(buf[i-1]&0x80000000))
				printf("     [*****]:\n");
			printf("%5d[%5d]: %08x", addr, i, buf[i]);
			if (buf[i] & 0x40000000) {
				printf(" <<--- TRIGGER!");
				trigger_addr = addr;
			} printf("\n");

			addr++;
		}
	} if ((buf[(1<<ln)-1]&0x80000000))
		printf("     [*****]:\n");
	// }}}

	// Error checking
	// {{{
	if (buf[(1<<ln)-1] & 0x80000000) {
		printf("ERR: LAST VALUE IS A RUN, 0x%08x\n", buf[(1<<ln)-1]);
		goto test_failure;
	}

	if (trigger_addr == 0xffffffff) {
		printf("ERR: TRIGGER NOT FOUND IN THE DATA!\n");
		goto test_failure;
	}
	// }}}

	// Trigger check
	// {{{
	printf("TRIGGER ADDRESS = %08x (%5d)\n", trigger_addr, trigger_addr);
	printf("V               = %08x\n", v & 0x0fffff);
	printf("Difference      = %08x (%5d)\n", addr - trigger_addr,
			addr - trigger_addr);
	if (addr - 1 - trigger_addr != (v & 0x0fffff)) {
		printf("TRIGGER AT THE WRONG LOCATION!\n");
		goto test_failure;
	}
	// }}}

	printf("SUCCESS!!\n");
	delete tb;
	exit(0);
test_failure:
	printf("FAIL-HERE\n");
	for(int i=0; i<4; i++)
		tb->tick();
	printf("TEST FAILED\n");
	delete tb;
	exit(-1);
}
