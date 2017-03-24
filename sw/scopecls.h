////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	scopecls.h
//
// Project:	WBScope, a wishbone hosted scope
//
// Purpose:	After rebuilding the same code over and over again for every
//		"scope" I tried to interact with, I thought it would be simpler
//	to try to make a more generic interface, that other things could plug
//	into.  This is that more generic interface.
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
#ifndef	SCOPECLS_H
#define	SCOPECLS_H

#include <vector>
#include "devbus.h"

class	TRACEINFO {
public:
	const char	*m_name;
	char		m_key[4];
	unsigned	m_nbits, m_nshift;
};

class	SCOPE {
	DEVBUS		*m_fpga;
	DEVBUS::BUSW	m_addr;
	bool		m_compressed, m_vector_read;
	unsigned	m_scoplen;
	unsigned	*m_data;
	std::vector<TRACEINFO *> m_traces;

public:
	SCOPE(DEVBUS *fpga, unsigned addr,
			bool compressed=false, bool vecread=true)
		: m_fpga(fpga), m_addr(addr),
			m_compressed(compressed), m_vector_read(vecread),
			m_scoplen(0), m_data(NULL) {}
	~SCOPE(void) { if (m_data) delete[] m_data; }

	bool	ready();
	void	decode_control(void);
	int	scoplen(void);
	virtual	void	rawread(void);
		void	print(void);
	virtual void	write_trace_timescale(FILE *fp);
	virtual void	write_trace_header(FILE *fp);
		void	write_binary_trace(FILE *fp, const int nbits,
				unsigned val, const char *str);
		void	write_binary_trace(FILE *fp, TRACEINFO *info,
				unsigned value);
		void	writevcd(const char *trace_file_name);
		void	register_trace(const char *varname,
				unsigned nbits, unsigned shift);
	virtual	void	decode(DEVBUS::BUSW v) const = 0;
	virtual	void	define_traces(void);
};

#endif	// SCOPECLS_H
