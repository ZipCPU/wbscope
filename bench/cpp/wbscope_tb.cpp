////////////////////////////////////////////////////////////////////////////////
//
// Filename:	wbscope_tb.cpp
//
// Project:	WBScope, a wishbone hosted scope
//
// Purpose:	A quick test bench to determine if the wbscope module works.
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
#include <stdio.h>

#include <verilated.h>
#include <verilated_vcd_c.h>
#include "testb.h"
#include "Vwbscope_tb.h"

#define	MMUFLAG_RONW	8 // Read only (not writeable)
#define	MMUFLAG_ACCS	4 // Accessed
#define	MMUFLAG_CCHE	2 // Cachable
#define	MMUFLAG_THSP	1 // Page has this context

const int	BOMBCOUNT = 32,
		LGMEMSIZE = 15;

class	WBSCOPE_TB : public TESTB<Vwbscope_tb> {
	bool		m_bomb, m_miss, m_err, m_debug;
	int		m_last_tlb_index;
public:

	WBSCOPE_TB(void) {
		m_debug = true;
		m_last_tlb_index = 0;
	}

	void	tick(void) {

		TESTB<Vwbscope_tb>::tick();

		bool	writeout = true;
		if ((m_debug)&&(writeout)) {}
	}

	void reset(void) {
		m_core->i_rst    = 1;
		m_core->i_wb_cyc = 0;
		m_core->i_wb_stb = 0;
		tick();
		m_core->i_rst  = 0;
	}

	void wb_tick(void) {
		m_core->i_wb_cyc  = 0;
		m_core->i_wb_stb  = 0;
		tick();
		assert(!m_core->o_wb_ack);
	}

	unsigned wb_read(unsigned a) {
		unsigned	result;

		printf("WB-READM(%08x)\n", a);

		m_core->i_wb_cyc = 1;
		m_core->i_wb_stb = 1;
		m_core->i_wb_we  = 0;
		m_core->i_wb_addr= (a>>2)&1;

		// Dont need to check for stalls, since the wbscope never stalls
		tick();

		m_core->i_wb_stb = 0;

		while(!m_core->o_wb_ack)
			tick();

		result = m_core->o_wb_data;

		// Release the bus?
		m_core->i_wb_cyc = 0;
		m_core->i_wb_stb = 0;

		// Let the bus idle for one cycle
		tick();

		return result;
	}

	void	wb_read(unsigned a, int len, unsigned *buf) {
		int		cnt, rdidx;

		printf("WB-READM(%08x, %d)\n", a, len);

		m_core->i_wb_cyc = 1;
		m_core->i_wb_stb = 1;
		m_core->i_wb_we   = 0;
		m_core->i_wb_addr = (a>>2)&1;

		rdidx =0; cnt = 0;

		do {
			tick();
			// Normally, we'd increment the address here.  For the
			// scope, multiple reads only make sense if they are 
			// from the same address, hence we don't increment the
			// address here
			// m_core->i_wb_addr += inc;
			cnt += 1;
			if (m_core->o_wb_ack)
				buf[rdidx++] = m_core->o_wb_data;
		} while(cnt < len);

		m_core->i_wb_stb = 0;

		while(rdidx < len) {
			tick();
			if (m_core->o_wb_ack)
				buf[rdidx++] = m_core->o_wb_data;
		}

		// Release the bus?
		m_core->i_wb_cyc = 0;

		tick();
		assert(!m_core->o_wb_ack);
	}

	void	wb_write(unsigned a, unsigned v) {
		int errcount = 0;

		printf("WB-WRITEM(%08x) <= %08x\n", a, v);
		m_core->i_wb_cyc = 1;
		m_core->i_wb_stb = 1;
		m_core->i_wb_we  = 1;
		m_core->i_wb_addr= (a>>2)&1;
		m_core->i_wb_data= v;

		tick();
		m_core->i_wb_stb = 0;

		while(!m_core->o_wb_ack) {
			tick();
		}

		tick();

		// Release the bus?
		m_core->i_wb_cyc = 0;
		m_core->i_wb_stb = 0;

		assert(!m_core->o_wb_ack);
	}

	unsigned	trigger(void) {
		m_core->i_trigger = 1;
		wb_tick();
		m_core->i_trigger = 0;
		printf("TRIGGERED AT %08x\n", m_core->o_data);
		return m_core->o_data;
	}

	bool	debug(void) const { return m_debug; }
	bool	debug(bool nxtv) { return m_debug = nxtv; }
};

int main(int  argc, char **argv) {
	Verilated::commandArgs(argc, argv);
	WBSCOPE_TB	*tb = new WBSCOPE_TB;
	unsigned	v;
	unsigned *buf;
	int	trigpt;

	tb->opentrace("wbscope_tb.vcd");
	printf("Giving the core 2 cycles to start up\n");
	// Before testing, let's give the unit time enough to warm up
	tb->reset();
	for(int i=0; i<2; i++)
		tb->wb_tick();

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
	v = tb->wb_read(WBSCOPE_STATUS);
	int ln = WBSCOPE_LGLEN(v);
	printf("V   = %08x\n", v);
	printf("LN  = %d, or %d entries\n", ln, (1<<ln));
	printf("DLY = %d\n", (v&0xfffff));
	if (((1<<ln) < tb->m_tickcount)&&(v&0x10000000)) {
		printf("SCOPE is already triggered! ??\n");
		goto test_failure;
	}
	buf = new unsigned[(1<<ln)];

	for(int i=0; i<(1<<ln); i++)
		tb->wb_tick();

	v = tb->wb_read(WBSCOPE_STATUS);
	if ((v&WBSCOPE_PRIMED)==0) {
		printf("v = %08x\n", v);
		printf("SCOPE hasn\'t primed! ??\n");
		goto test_failure;
	}

	tb->trigger();
	v = tb->wb_read(WBSCOPE_STATUS);
	if ((v&WBSCOPE_TRIGGERED)==0) {
		printf("v = %08x\n", v);
		printf("SCOPE hasn\'t triggered! ??\n");
		goto test_failure;
	}

	while((v & WBSCOPE_STOPPED)==0)
		v = tb->wb_read(WBSCOPE_STATUS);
	printf("SCOPE has stopped, reading data\n");

	tb->wb_read(WBSCOPE_DATA, (1<<ln), buf);
	for(int i=0; i<(1<<ln); i++) {
		printf("%4d: %08x\n", i, buf[i]);
		if ((i>0)&&(((buf[i]&0x7fffffff)-(buf[i-1]&0x7fffffff))!=1))
			goto test_failure;
	}

	trigpt = (1<<ln)-v&(0x0fffff);
	if ((trigpt >= 0)&&(trigpt < (1<<ln))) {
		printf("Trigger value = %08x\n", buf[trigpt]);
		if (((0x80000000 & buf[trigpt])==0)&&(trigpt>0)) {
			printf("Pre-Trigger value = %08x\n", buf[trigpt-1]);
			if ((buf[trigpt-1]&0x80000000)==0) {
				printf("TRIGGER NOT FOUND\n");
				goto test_failure;
			}
		}
	}

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
