////////////////////////////////////////////////////////////////////////////////
//
// Filename:	wb_tb.cpp
// {{{
// Project:	WBScope, a wishbone hosted scope
//
// Purpose:	To provide a fairly generic interface wrapper to a wishbone bus,
//		that can then be used to create a test-bench class.
//
// Creator:	Dan Gisselquist, Ph.D.
//		Gisselquist Technology, LLC
//
////////////////////////////////////////////////////////////////////////////////
// }}}
// Copyright (C) 2015-2021, Gisselquist Technology, LLC
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
#include <stdlib.h>

#include <verilated.h>
#include <verilated_vcd_c.h>
#include "testb.h"
#include "devbus.h"

const int	BOMBCOUNT = 32;

template <class VA>	class	WB_TB : public TESTB<VA>, public DEVBUS {
	// {{{
#ifdef	WBERR
	bool	m_buserr;
#endif
#ifdef	INTERRUPTWIRE
	bool	m_interrupt;
#endif
	// }}}
public:
	typedef	uint32_t	BUSW;
	
	bool	m_bomb;

	WB_TB(void) {
		// {{{
		m_bomb = false;
		TESTB<VA>::m_core->i_wb_cyc = 0;
		TESTB<VA>::m_core->i_wb_stb = 0;
#ifdef	WBERR
		m_buserr = false;
#endif
#ifdef	INTERRUPTWIRE
		m_interrupt = false;
#endif
	}

	// close()
	// {{{
	virtual	void	close(void) {
		TESTB<VA>::closetrace();
	}
	// }}}

	// kill()
	// {{{
	virtual	void	kill(void) {
		close();
	}
	// }}}

	// tick()
	// {{{
#ifdef	INTERRUPTWIRE
	virtual	void	tick(void) {
		TESTB<VA>::tick();
		if (TESTB<VA>::m_core->INTERRUPTWIRE)
			m_interrupt = true;
	}
#endif
#define	TICK	this->tick
	// }}}

	// idle
	// {{{
	void	idle(const unsigned counts = 1) {
		TESTB<VA>::m_core->i_wb_cyc = 0;
		TESTB<VA>::m_core->i_wb_stb = 0;
		for(unsigned k=0; k<counts; k++) {
			this->tick();
			assert(!TESTB<VA>::m_core->o_wb_ack);
		}
	}
	// }}}

	// readio()
	// {{{
	BUSW readio(BUSW a) {
		int		errcount = 0;
		BUSW		result;

		// printf("WB-READM(%08x)\n", a);

		TESTB<VA>::m_core->i_wb_cyc = 1;
		TESTB<VA>::m_core->i_wb_stb = 1;
		TESTB<VA>::m_core->i_wb_we  = 0;
		TESTB<VA>::m_core->i_wb_addr= (a>>2);

		if (TESTB<VA>::m_core->o_wb_stall) {
			while((errcount++ < BOMBCOUNT)&&(TESTB<VA>::m_core->o_wb_stall)) {
				TICK();
#ifdef	WBERR
				if (TESTB<VA>::m_core->WBERR) {
					m_buserr = true;
					TESTB<VA>::m_core->i_wb_cyc = 0;
					TESTB<VA>::m_core->i_wb_stb = 0;
					return -1;
				}
#endif
			}
		} TICK();

		TESTB<VA>::m_core->i_wb_stb = 0;

		while((errcount++ <  BOMBCOUNT)&&(!TESTB<VA>::m_core->o_wb_ack)) {
			TICK();
#ifdef	WBERR
			if (TESTB<VA>::m_core->WBERR) {
				m_buserr = true;
				TESTB<VA>::m_core->i_wb_cyc = 0;
				TESTB<VA>::m_core->i_wb_stb = 0;
				return -1;
			}
#endif
		}


		result = TESTB<VA>::m_core->o_wb_data;

		// Release the bus
		TESTB<VA>::m_core->i_wb_cyc = 0;
		TESTB<VA>::m_core->i_wb_stb = 0;

		if(errcount >= BOMBCOUNT) {
			printf("WB/SR-BOMB: NO RESPONSE AFTER %d CLOCKS\n", errcount);
			m_bomb = true;
		} else if (!TESTB<VA>::m_core->o_wb_ack) {
			printf("WB/SR-BOMB: NO ACK, NO TIMEOUT\n");
			m_bomb = true;
		}
		TICK();

		assert(!TESTB<VA>::m_core->o_wb_ack);
		assert(!TESTB<VA>::m_core->o_wb_stall);

		return result;
	}
	// }}}

	// readv()
	// {{{
	void	readv(const BUSW a, int len, BUSW *buf, const int inc=1) {
		int		errcount = 0;
		int		THISBOMBCOUNT = BOMBCOUNT * len;
		int		cnt, rdidx;

		printf("WB-READM(%08x, %d)\n", a, len);
		TESTB<VA>::m_core->i_wb_cyc  = 0;
		TESTB<VA>::m_core->i_wb_stb  = 0;

		while((errcount++ < BOMBCOUNT)&&(TESTB<VA>::m_core->o_wb_stall))
			TICK();

		if (errcount >= BOMBCOUNT) {
			printf("WB-READ(%d): Setting bomb to true (errcount = %d)\n", __LINE__, errcount);
			m_bomb = true;
			return;
		}

		errcount = 0;
		
		TESTB<VA>::m_core->i_wb_cyc  = 1;
		TESTB<VA>::m_core->i_wb_stb  = 1;
		TESTB<VA>::m_core->i_wb_we   = 0;
		TESTB<VA>::m_core->i_wb_addr = (a>>2);

		rdidx =0; cnt = 0;

		do {
			int	s;
			TESTB<VA>::m_core->i_wb_stb = ((rand()&7)!=0) ? 1:0;
			s = ((TESTB<VA>::m_core->i_wb_stb)
				&&(TESTB<VA>::m_core->o_wb_stall==0))?0:1;
			TICK();
			TESTB<VA>::m_core->i_wb_addr += (inc&(s^1))?4:0;
			cnt += (s^1);
			if (TESTB<VA>::m_core->o_wb_ack)
				buf[rdidx++] = TESTB<VA>::m_core->o_wb_data;
#ifdef	WBERR
			if (TESTB<VA>::m_core->WBERR) {
				m_buserr = true;
				TESTB<VA>::m_core->i_wb_cyc = 0;
				TESTB<VA>::m_core->i_wb_stb = 0;
				return -1;
			}
#endif
		} while((cnt < len)&&(errcount++ < THISBOMBCOUNT));

		TESTB<VA>::m_core->i_wb_stb = 0;

		while((rdidx < len)&&(errcount++ < THISBOMBCOUNT)) {
			TICK();
			if (TESTB<VA>::m_core->o_wb_ack)
				buf[rdidx++] = TESTB<VA>::m_core->o_wb_data;
#ifdef	WBERR
			if (TESTB<VA>::m_core->WBERR) {
				m_buserr = true;
				TESTB<VA>::m_core->i_wb_cyc = 0;
				TESTB<VA>::m_core->i_wb_stb = 0;
				return -1;
			}
#endif
		}

		// Release the bus
		TESTB<VA>::m_core->i_wb_cyc = 0;

		if(errcount >= THISBOMBCOUNT) {
			printf("WB/PR-BOMB: NO RESPONSE AFTER %d CLOCKS\n", errcount);
			m_bomb = true;
		} else if (!TESTB<VA>::m_core->o_wb_ack) {
			printf("WB/PR-BOMB: NO ACK, NO TIMEOUT\n");
			m_bomb = true;
		}
		TICK();
		assert(!TESTB<VA>::m_core->o_wb_ack);
	}
	// }}}

	// readi()
	// {{{
	void	readi(const BUSW a, const int len, BUSW *buf) {
		return readv(a, len, buf, 1);
	}
	// }}}

	// readz()
	// {{{
	void	readz(const BUSW a, const int len, BUSW *buf) {
		return readv(a, len, buf, 0);
	}
	// }}}

	// writeio()
	// {{{
	void	writeio(const BUSW a, const BUSW v) {
		int errcount = 0;

		printf("WB-WRITEM(%08x) <= %08x\n", a, v);
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
#ifdef	WBERR
				if (m_core->WBERR) {
					m_buserr = true;
					TESTB<VA>::m_core->i_wb_cyc = 0;
					TESTB<VA>::m_core->i_wb_stb = 0;
					return;
				}
#endif
			}
		TICK();
#ifdef	WBERR
		if (m_core->WBERR) {
			m_buserr = true;
			TESTB<VA>::m_core->i_wb_cyc = 0;
			TESTB<VA>::m_core->i_wb_stb = 0;
			return;
		}
#endif

		TESTB<VA>::m_core->i_wb_stb = 0;

		while((errcount++ <  BOMBCOUNT)&&(!TESTB<VA>::m_core->o_wb_ack)) {
			TICK();
#ifdef	WBERR
			if (m_core->WBERR) {
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
			printf("WB/SW-BOMB: NO RESPONSE AFTER %d CLOCKS (LINE=%d)\n",errcount, __LINE__);
			m_bomb = true;
		} TICK();
#ifdef	WBERR
		if (m_core->WBERR) {
			m_buserr = true;
			TESTB<VA>::m_core->i_wb_cyc = 0;
			TESTB<VA>::m_core->i_wb_stb = 0;
			return;
		}
#endif
		assert(!TESTB<VA>::m_core->o_wb_ack);
		assert(!TESTB<VA>::m_core->o_wb_stall);
	}

	// }}}

	// writev()
	// {{{
	void	writev(const BUSW a, const int ln, const BUSW *buf, const int inc=1) {
		unsigned errcount = 0, nacks = 0;

		printf("WB-WRITEM(%08x, %d, ...)\n", a, ln);
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
#ifdef	WBERR
				if (m_core->WBERR) {
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
#ifdef	WBERR
			if (m_core->WBERR) {
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
#ifdef	WBERR
			if (m_core->WBERR) {
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
			printf("WB/PW-BOMB: NO RESPONSE AFTER %d CLOCKS (LINE=%d)\n",errcount,__LINE__);
			m_bomb = true;
		}
		TICK();
		assert(!TESTB<VA>::m_core->o_wb_ack);
		assert(!TESTB<VA>::m_core->o_wb_stall);
	}
	// }}}

	// writei()
	// {{{
	void	writei(const BUSW a, const int ln, const BUSW *buf) {
		writev(a, ln, buf, 1);
	}
	// }}}

	// writez()
	// {{{
	void	writez(const BUSW a, const int ln, const BUSW *buf) {
		writev(a, ln, buf, 0);
	}
	// }}}

	bool	bombed(void) const { return m_bomb; }

	// bool	debug(void) const	{ return m_debug; }
	// bool	debug(bool nxtv)	{ return m_debug = nxtv; }

	// poll()
	// {{{
	bool	poll(void) {
#ifdef	INTERRUPTWIRE
		return (m_interrupt)||(TESTB<VA>::m_core->INTERRUPTWIRE != 0);
#else
		return false;
#endif
	}

	// }}}

	// bus_err()
	// {{{
	bool	bus_err(void) const {
#ifdef	WBERR
		return m_buserr;
#else
		return false;
#endif
	}
	// }}}

	// reset_err()
	// {{{
	void	reset_err(void) {
#ifdef	WBERR
		m_buserr = false;;
#endif
	}
	// }}}

	// usleep()
	// {{{
	void	usleep(unsigned msec) {
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
	}
	// }}}

	// clear()
	// {{{
	void	clear(void) {
#ifdef	INTERRUPTWIRE
		m_interrupt = false;
#endif
	}
	// }}}

	// wait()
	// {{{
	void	wait(void) {
#ifdef	INTERRUPTWIRE
		while(!poll())
			TICK();
#else
		assert(("No interrupt defined",0));
#endif
	}
	// }}}
	// }}}
};

