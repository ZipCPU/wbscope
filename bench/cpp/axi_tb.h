////////////////////////////////////////////////////////////////////////////////
//
// Filename:	axi_tb.cpp
// {{{
// Project:	WBScope, a wishbone hosted scope
//
// Purpose:	To provide a fairly generic interface wrapper to an AXI bus,
//		that can then be used to create a test-bench class.
//
// Creator:	Dan Gisselquist, Ph.D.
//		Gisselquist Technology, LLC
//
////////////////////////////////////////////////////////////////////////////////
// }}}
// Copyright (C) 2017-2023, Gisselquist Technology, LLC
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
//
////////////////////////////////////////////////////////////////////////////////
//
// }}}
#include <stdio.h>
#include <stdlib.h>

#include <verilated.h>
#include <verilated_vcd_c.h>
#include "testb.h"
#include "devbus.h"

const int	BOMBCOUNT = 32;

template <class VA>	class	AXI_TB : public DEVBUS {
	// {{{
#ifdef	AXIERR
	bool	m_buserr;
#endif
#ifdef	INTERRUPTWIRE
	bool	m_interrupt;
#endif
	VA	*m_core;
	VerilatedVcdC	*m_trace;
	unsigned long	m_tickcount;
public:
	typedef	uint32_t	BUSW;
	
	bool	m_bomb;

	AXI_TB(void) {
		// {{{
		m_core = new VA;
		Verilated::traceEverOn(true);

		m_bomb = false;
		m_core->i_wb_cyc = 0;
		m_core->i_wb_stb = 0;
#ifdef	AXIERR
		m_buserr = false;
#endif
#ifdef	INTERRUPTWIRE
		m_interrupt = false;
#endif
		// }}}
	}

	virtual	~AXI_TB(void) {
		// {{{
		if (m_trace) {
			m_trace->close();
			delete m_trace;
		}
		delete m_core;
		m_core  = NULL;
		m_trace = NULL;
		// }}}
	}

	virtual	void	opentrace(const char *vcdname) {
		// {{{
		m_trace = new VerilatedVcdC;
		m_core->trace(m_trace, 99);
		m_trace->open(vcdname);
		// }}}
	}

	virtual	void	closetrace(void) {
		if (m_trace) {
			m_trace->close();
			m_trace = NULL;
		}
	}

	virtual	void	close(void) {
		TESTB<VA>::closetrace();
	}

	virtual	void	kill(void) {
		close();
	}

	virtual	void	eval(void) {
		m_core->eval();
	}

	virtual	void	tick(void) {
		// {{{
		m_tickcount++

		eval();
		if (m_trace) m_trace->dump(10*m_tickcount-2);
		m_core->S_AXI_ACLK = 1;
		eval();
		if (m_trace) m_trace->dump(10*m_tickcount);
		m_core->S_AXI_ACLK = 0;
		eval();
		if (m_trace) {
			m_trace->dump(10*m_tickcount+5);
			m_trace->flush();
		}
#ifdef	INTERRUPTWIRE
		if (TESTB<VA>::m_core->INTERRUPTWIRE)
			m_interrupt = true;
#endif
		// }}}
	}

	virtual	void	reset(void) {
		// {{{
		m_core->S_AXI_ARESET = 0;
		tick();
		m_core->S_AXI_ARESET = 1;
		// }}}
	}

	unsigned long	tickcount(void) {
		return m_tickcount;
	}

	void	idle(const unsigned counts = 1) {
		// {{{
		m_core->S_AXI_AWVALID = 0;
		m_core->S_AXI_WVALID  = 0;
		m_core->S_AXI_BREADY  = 0;
		m_core->S_AXI_ARVALID = 0;
		m_core->S_AXI_RREADY  = 0;
		for(unsigned k=0; k<counts; k++) {
			this->tick();
			assert(!TESTB<VA>::m_core->o_wb_ack);
		}
		// }}}
	}

	BUSW readio(BUSW a) {
		// {{{
		int		errcount = 0;
		BUSW		result;

		// printf("AXI-READM(%08x)\n", a);

		m_core->S_AXI_ = 1;
		m_core->S_AXI_ARVALID = 1;
		m_core->S_AXI_ARADDR  = a;
		//
		m_core->S_AXI_ARPROT  = 0; // Not priveleged or secure, data access
		//
		m_core->S_AXI_ARREADY = 1;

		while(!m_core->S_AXI_ARREADY)
			TICK();

		m_core->S_AXI_ARVALID = 0;

		while((errcount++ <  BOMBCOUNT)&&(
				((!m_core->S_AXI_RVALID)
				||(!m_core->S_AXI_RREADY))))
			TICK();

		result = m_core->S_AXI_RDATA;

		if(errcount >= BOMBCOUNT) {
			printf("AXI/SR-BOMB: NO RESPONSE AFTER %d CLOCKS\n", errcount);
			m_bomb = true;
		} else if (m_core->S_AXI_RRESP != 0) {
			printf("AXI/SR-BOMB: NO ACK, NO TIMEOUT, INVALID RESPONSE (%d)\n", m_core->S_AXI_RRESP);
			m_bomb = true;
		}
		TICK();

		assert(m_core->S_AXI_RRESP == 0);
		assert(!m_core->S_AXI_RVALID);

		return result;
		// }}}
	}

	void	readv(const BUSW a, int len, BUSW *buf, const int inc=1) {
		// {{{
		int		errcount = 0;
		int		THISBOMBCOUNT = BOMBCOUNT * len;
		int		cnt, rdidx;

		printf("AXI-READM(%08x, %d)\n", a, len);
		m_core->S_AXI_ = 1;
		m_core->S_AXI_ARVALID = 1;
		m_core->S_AXI_ARADDR  = a;
		//
		m_core->S_AXI_ARPROT  = 0; // Not priveleged or secure, data access
		//
		m_core->S_AXI_ARREADY = 1;


		errcount = 0;
		
		rdidx =0; cnt = 0;

		do {
			int	s;
			m_core->S_AXI_ARVALID = 1; // ((rand()&7)!=0) ? 1:0;
			s = ((m_core->S_AXI_ARVALID)
				&&(m_core->S_AXI_ARREADY==0))?0:1;
			TICK();
			m_core->S_AXI_ARADDR += (inc&(s^1))?4:0;
			cnt += (s^1);
			if (m_core->S_AXI_RVALID)
				buf[rdidx++] = m_core->S_AXI_RDATA;
			if (m_core->S_AXI_RRESP != 0) {
				m_buserr = true;
			}
		} while((cnt < len)&&(errcount++ < THISBOMBCOUNT));

		m_core->S_AXI_ARVALID = 0;

		while((rdidx < len)&&(errcount++ < THISBOMBCOUNT)) {
			TICK();
			if ((m_core->S_AXI_RVALID)&&(m_core->S_AXI_RREADY))
				buf[rdidx++] = m_core->S_AXI_RDATA;
			if (m_core->S_AXI_RRESP != 0)
				m_buserr = true;
		}

		if(errcount >= THISBOMBCOUNT) {
			printf("AXI/PR-BOMB: NO RESPONSE AFTER %d CLOCKS\n", errcount);
			m_bomb = true;
		} else if (!TESTB<VA>::m_core->o_wb_ack) {
			printf("AXI/PR-BOMB: NO ACK, NO TIMEOUT\n");
			m_bomb = true;
		}
		TICK();
		m_core->S_AXI_RREADY = 0;
		assert(!m_core->S_AXI_RVALID);
		// }}}
	}

	void	readi(const BUSW a, const int len, BUSW *buf) {
		return readv(a, len, buf, 1);
	}

	void	readz(const BUSW a, const int len, BUSW *buf) {
		return readv(a, len, buf, 0);
	}

	void	writeio(const BUSW a, const BUSW v) {
		// {{{
		int errcount = 0;

		printf("AXI-WRITEM(%08x) <= %08x\n", a, v);
		TESTB<VA>::m_core->i_wb_cyc = 1;
		TESTB<VA>::m_core->i_wb_stb = 1;
		TESTB<VA>::m_core->i_wb_we  = 1;
		TESTB<VA>::m_core->i_wb_addr= (a>>2);
		TESTB<VA>::m_core->i_wb_data= v;
		// TESTB<VA>::m_core->i_wb_sel = 0x0f;

		if (TESTB<VA>::m_core->o_wb_stall)
			while((errcount++ < BOMBCOUNT)&&(TESTB<VA>::m_core->o_wb_stall)) {
				printf("Stalled, so waiting, errcount=%d\n", errcount);
				TICK();
#ifdef	AXIERR
				if (m_core->AXIERR) {
					m_buserr = true;
					TESTB<VA>::m_core->i_wb_cyc = 0;
					TESTB<VA>::m_core->i_wb_stb = 0;
					return;
				}
#endif
			}
		TICK();
#ifdef	AXIERR
		if (m_core->AXIERR) {
			m_buserr = true;
			TESTB<VA>::m_core->i_wb_cyc = 0;
			TESTB<VA>::m_core->i_wb_stb = 0;
			return;
		}
#endif

		TESTB<VA>::m_core->i_wb_stb = 0;

		while((errcount++ <  BOMBCOUNT)&&(!TESTB<VA>::m_core->o_wb_ack)) {
			TICK();
#ifdef	AXIERR
			if (m_core->AXIERR) {
				m_buserr = true;
				TESTB<VA>::m_core->i_wb_cyc = 0;
				TESTB<VA>::m_core->i_wb_stb = 0;
				return;
			}
#endif
		}
		TICK();

		// Release the bus?
		TESTB<VA>::m_core->i_wb_cyc = 0;
		TESTB<VA>::m_core->i_wb_stb = 0;

		if(errcount >= BOMBCOUNT) {
			printf("AXI/SW-BOMB: NO RESPONSE AFTER %d CLOCKS (LINE=%d)\n",errcount, __LINE__);
			m_bomb = true;
		} TICK();
#ifdef	AXIERR
		if (m_core->AXIERR) {
			m_buserr = true;
			TESTB<VA>::m_core->i_wb_cyc = 0;
			TESTB<VA>::m_core->i_wb_stb = 0;
			return;
		}
#endif
		assert(!TESTB<VA>::m_core->o_wb_ack);
		assert(!TESTB<VA>::m_core->o_wb_stall);
		// }}}
	}

	void	writev(const BUSW a, const int ln, const BUSW *buf, const int inc=1) {
		// {{{
		unsigned errcount = 0, nacks = 0;

		printf("AXI-WRITEM(%08x, %d, ...)\n", a, ln);
		TESTB<VA>::m_core->i_wb_cyc = 1;
		TESTB<VA>::m_core->i_wb_stb = 1;
		TESTB<VA>::m_core->i_wb_we  = 1;
		TESTB<VA>::m_core->i_wb_addr= (a>>2);
		// TESTB<VA>::m_core->i_wb_sel = 0x0f;
		for(unsigned stbcnt=0; stbcnt<ln; stbcnt++) {
			// m_core->i_wb_addr= a+stbcnt;
			TESTB<VA>::m_core->i_wb_data= buf[stbcnt];
			errcount = 0;

			while((errcount++ < BOMBCOUNT)&&(TESTB<VA>::m_core->o_wb_stall)) {
				TICK();
				if (TESTB<VA>::m_core->o_wb_ack)
					nacks++;
#ifdef	AXIERR
				if (m_core->AXIERR) {
					m_buserr = true;
					TESTB<VA>::m_core->i_wb_cyc = 0;
					TESTB<VA>::m_core->i_wb_stb = 0;
					return;
				}
#endif
			}
			// Tick, now that we're not stalled.  This is the tick
			// that gets accepted.
			TICK();
			if (TESTB<VA>::m_core->o_wb_ack) nacks++;
#ifdef	AXIERR
			if (m_core->AXIERR) {
				m_buserr = true;
				TESTB<VA>::m_core->i_wb_cyc = 0;
				TESTB<VA>::m_core->i_wb_stb = 0;
				return;
			}
#endif

			// Now update the address
			TESTB<VA>::m_core->i_wb_addr += (inc)?4:0;
		}

		TESTB<VA>::m_core->i_wb_stb = 0;

		errcount = 0;
		while((nacks < ln)&&(errcount++ < BOMBCOUNT)) {
			TICK();
			if (TESTB<VA>::m_core->o_wb_ack) {
				nacks++;
				errcount = 0;
			}
#ifdef	AXIERR
			if (m_core->AXIERR) {
				m_buserr = true;
				TESTB<VA>::m_core->i_wb_cyc = 0;
				TESTB<VA>::m_core->i_wb_stb = 0;
				return;
			}
#endif
		}

		// Release the bus
		TESTB<VA>::m_core->i_wb_cyc = 0;
		TESTB<VA>::m_core->i_wb_stb = 0;

		if(errcount >= BOMBCOUNT) {
			printf("AXI/PW-BOMB: NO RESPONSE AFTER %d CLOCKS (LINE=%d)\n",errcount,__LINE__);
			m_bomb = true;
		}
		TICK();
		assert(!TESTB<VA>::m_core->o_wb_ack);
		assert(!TESTB<VA>::m_core->o_wb_stall);
		// }}}
	}

	void	writei(const BUSW a, const int ln, const BUSW *buf) {
		writev(a, ln, buf, 1);
	}

	void	writez(const BUSW a, const int ln, const BUSW *buf) {
		writev(a, ln, buf, 0);
	}


	bool	bombed(void) const { return m_bomb; }

	// bool	debug(void) const	{ return m_debug; }
	// bool	debug(bool nxtv)	{ return m_debug = nxtv; }

	bool	poll(void) {
		// {{{
#ifdef	INTERRUPTWIRE
		return (m_interrupt)||(TESTB<VA>::m_core->INTERRUPTWIRE != 0);
#else
		return false;
#endif
		// }}}
	}

	bool	bus_err(void) const {
		// {{{
#ifdef	AXIERR
		return m_buserr;
#else
		return false;
#endif
		// }}}
	}

	void	reset_err(void) {
		// {{{
#ifdef	AXIERR
		m_buserr = false;;
#endif
		// }}}
	}

	void	usleep(unsigned msec) {
		// {{{
#ifdef	CLKRATEHZ
		unsigned count = CLKRATEHZ / 1000 * msec;
#else
		// Assume 100MHz if no clockrate is given
		unsigned count = 1000*100 * msec;
#endif
		while(count-- != 0)
#ifdef	INTERRUPTWIRE
			if (poll()) return; else
#endif
			TICK();
		// }}}
	}

	void	clear(void) {
		// {{{
#ifdef	INTERRUPTWIRE
		m_interrupt = false;
#endif
		// }}}
	}

	void	wait(void) {
		// {{{
#ifdef	INTERRUPTWIRE
		while(!poll())
			TICK();
#else
		assert(("No interrupt defined",0));
#endif
		// }}}
	}
	// }}}
};

