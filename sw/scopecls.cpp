////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	scopecls.cpp
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <time.h>

#include "devbus.h"
#include "scopecls.h"

bool	SCOPE::ready() {
	unsigned v;
	v = m_fpga->readio(m_addr);
	if (m_scoplen == 0) {
		m_scoplen = (1<<((v>>20)&0x01f));
		m_holdoff = (v & ((1<<20)-1));
	} v = (v>>28)&6;
	return (v==6);
}

void	SCOPE::decode_control(void) {
	unsigned	v;

	v = m_fpga->readio(m_addr);
	printf("\t31. RESET:\t%s\n", (v&0x80000000)?"Ongoing":"Complete");
	printf("\t30. STOPPED:\t%s\n", (v&0x40000000)?"Yes":"No");
	printf("\t29. TRIGGERED:\t%s\n", (v&0x20000000)?"Yes":"No");
	printf("\t28. PRIMED:\t%s\n", (v&0x10000000)?"Yes":"No");
	printf("\t27. MANUAL:\t%s\n", (v&0x08000000)?"Yes":"No");
	printf("\t26. DISABLED:\t%s\n", (v&0x04000000)?"Yes":"No");
	printf("\t25. ZERO:\t%s\n", (v&0x02000000)?"Yes":"No");
	printf("\tSCOPLEN:\t%08x (%d)\n", m_scoplen, m_scoplen);
	printf("\tHOLDOFF:\t%08x\n", (v&0x0fffff));
	printf("\tTRIGLOC:\t%d\n", m_scoplen-(v&0x0fffff));
}

int	SCOPE::scoplen(void) {
	unsigned	v, lgln;

	// If the scope length is zero, then the scope isn't present.
	// We use a length of zero here to also represent whether or not we've
	// looked up the length by reading from the scope.
	if (m_scoplen == 0) {
		v = m_fpga->readio(m_addr);
		m_holdoff = (v & ((1<<20)-1));

		// Since the length of the scope memory is a configuration
		// parameter internal to the scope, we read it here to find
		// out how the scope was configured.
		lgln = (v>>20) & 0x1f;

		// If the length is still zero, then there is no scope installed
		if (lgln != 0) {
			// Otherwise, the scope length contained in the device
			// control register is the log base 2 of the actual
			// length of what's in the FPGA.  Here, we just convert
			// that to the actual length of the scope.
			m_scoplen = (1<<lgln);
		}
	// else we already know the length of the scope, and don't need to
	// slow down to read that length from the device a second time.
	} return m_scoplen;
}

//
// rawread
//
// Read the scope data from the scope.
void	SCOPE::rawread(void) {
	// If we've already read the data from the scope, then we don't need
	// to read it a second time.
	if (m_data)
		return;

	// Let's get the length of the scope, and check that it is a valid
	// length
	if (scoplen() <= 4) {
		printf("ERR: Scope has less than a minimum length.  Is it truly a scope?\n");
		return;
	}

	// Now that we know the size of the scopes buffer, let's allocate a
	// buffer to hold all this data
	m_data = new DEVBUS::BUSW[m_scoplen];

	// There are two means of reading from a DEVBUS interface: The first
	// is a vector read, optimized so that the address and read command
	// only needs to be sent once.  This is the optimal means.  However,
	// if the bus isn't (yet) trustworthy, it may be more reliable to access
	// the port by reading one register at a time--hence the second method.
	// If the bus works, you'll want to use readz(): read scoplen values
	// into the buffer, from the address WBSCOPEDATA, without incrementing
	// the address each time (hence the 'z' in readz--for zero increment).
	if (m_vector_read) {
		m_fpga->readz(m_addr+4, m_scoplen, m_data);
	} else {
		for(unsigned int i=0; i<m_scoplen; i++)
			m_data[i] = m_fpga->readio(m_addr+4);
	}
}

void	SCOPE::print(void) {
	DEVBUS::BUSW	addrv = 0;

	rawread();

	if(m_compressed) {
		for(int i=0; i<(int)m_scoplen; i++) {
			if ((m_data[i]>>31)&1) {
				addrv += (m_data[i]&0x7fffffff);
				printf(" ** (+0x%08x = %8d)\n",
					(m_data[i]&0x07fffffff),
					(m_data[i]&0x07fffffff));
				continue;
			}
			printf("%10d %08x: ", addrv++, m_data[i]);
			decode(m_data[i]);
			printf("\n");
		}
	} else {
		for(int i=0; i<(int)m_scoplen; i++) {
			if ((i>0)&&(m_data[i] == m_data[i-1])&&(i<(int)(m_scoplen-1))) {
				if ((i>2)&&(m_data[i] != m_data[i-2]))
					printf(" **** ****\n");
				continue;
			} printf("%9d %08x: ", i, m_data[i]);
			decode(m_data[i]);
			printf("\n");
		}
	}
}

void	SCOPE::write_trace_timescale(FILE *fp) {
	fprintf(fp, "$timescale 1ns $end\n\n");
}

void	SCOPE::write_trace_timezero(FILE *fp, int offset) {
	fprintf(fp, "$timescale %d $end\n\n", offset);
}

// $dumpoff and $dumpon
void	SCOPE::write_trace_header(FILE *fp, int offset) {
	time_t	now;

	time(&now);
	fprintf(fp, "$version Generated by WBScope $end\n");
	fprintf(fp, "$date %s\n $end\n", ctime(&now));
	write_trace_timescale(fp);
	if (offset != 0)
		write_trace_timezero(fp, offset);

	fprintf(fp, " $scope module WBSCOPE $end\n");
	// Print out all of the various values
	if (m_compressed) {
		fprintf(fp, "  $var wire %2d \'R _raw_data [%d:0] $end\n", 31,
			30);
	} else {
		fprintf(fp, "  $var wire %2d \'C clk $end\n", 1);
		fprintf(fp, "  $var wire %2d \'R _raw_data [%d:0] $end\n", 32,
			31);
	}

	for(unsigned i=0; i<m_traces.size(); i++) {
		TRACEINFO *info = m_traces[i];
		fprintf(fp, "  $var wire %2d %s %s",
			info->m_nbits, info->m_key, info->m_name);
		if ((info->m_nbits > 0)&&(NULL == strchr(info->m_name, '[')))
			fprintf(fp, "[%d:0] $end\n", info->m_nbits-1);
		else
			fprintf(fp, " $end\n");
	}

	fprintf(fp, " $upscope $end\n");
	fprintf(fp, "$enddefinitions $end\n");
}

void	SCOPE::write_binary_trace(FILE *fp, const int nbits, unsigned val,
		const char *str) {
	if (nbits <= 1) {
		fprintf(fp, "%d%s\n", val&1, str);
		return;
	}
	if ((unsigned)nbits < sizeof(val)*8)
		val &= ~(-1<<nbits);
	fputs("b", fp);
	for(int i=0; i<nbits; i++)
		fprintf(fp, "%d", (val>>(nbits-1-i))&1);
	fprintf(fp, " %s\n", str);
}

void	SCOPE::write_binary_trace(FILE *fp, TRACEINFO *info, unsigned value) {
	write_binary_trace(fp, info->m_nbits, (value>>info->m_nshift),
		info->m_key);
}

void	SCOPE::register_trace(const char *name,
		unsigned nbits, unsigned shift) {
	TRACEINFO	*info = new TRACEINFO;
	int	nkey = m_traces.size();

	info->m_name   = name;
	info->m_nbits  = nbits;
	info->m_nshift = shift;

	info->m_key[0] = 'v';
	if (nkey < 26)
		info->m_key[1] = 'a'+nkey;
	else if (nkey < 26+26)
		info->m_key[1] = 'A'+nkey-26;
	else // if (nkey < 26+26+10)	// Should never happen
		info->m_key[1] = '0'+nkey-26-26;
	info->m_key[2] = '\0';
	info->m_key[3] = '\0';

	m_traces.push_back(info);
}

/*
 * define_traces
 *
 * This is a user stub.  User programs should define this function.
 */
void	SCOPE::define_traces(void) {}

void	SCOPE::writevcd(FILE *fp) {
	int	offset = 0;

	if (!m_data)
		rawread();

	// If the traces haven't yet been defined, then define them now.
	if (m_traces.size()==0)
		define_traces();

	// Find the offset to the trigger
	if (m_compressed) {
		offset = 0;
	} else
		offset = m_scoplen - m_holdoff;

	// Write the file header.
	write_trace_header(fp, offset);

	// And split into two paths--one for compressed scopes (wbscopc), and
	// the other for the more normal scopes (wbscope).
	if(m_compressed) {
		// With compressed scopes, you need to track the address
		// relative to the beginning.
		unsigned	addrv = 0;
		unsigned 	now_ns;
		double		dnow;

		// Loop over each data word read from the scope
		for(int i=0; i<(int)m_scoplen; i++) {
			// If the high bit is set, the address jumps by more
			// than an increment
			if ((m_data[i]>>31)&1) {
				// But ... with nothing to write out.
				addrv += (m_data[i]&0x7fffffff);
				continue;
			}

			// Produce a line identifying the time associated with
			// this piece of data.
			dnow = 1.0/((double)m_clkfreq_hz) * addrv;
			now_ns = (unsigned)(dnow * 1e9);
			fprintf(fp, "#%d\n", now_ns);

			// For compressed data, only the lower 31 bits are
			// valid.  Write those bits to the VCD file as a raw
			// value.
			write_binary_trace(fp, 31, m_data[i], "\'R\n");

			// Finally, walk through all of the user defined traces,
			// writing each to the VCD file.
			for(unsigned k=0; k<m_traces.size(); k++) {
				TRACEINFO *info = m_traces[k];
				write_binary_trace(fp, info, m_data[i]);
			}
		}
	} else {
		//
		// Uncompressed scope.
		//
		unsigned now_ns;
		double	dnow;

		// We assume a clock signal, and set it to one and zero.
		// We also assume everything changes on the positive edge of
		// that clock within here.

		// Loop over all data words
		for(int i=0; i<(int)m_scoplen; i++) {
			// Positive edge of the clock (everything is assumed to
			// be on the positive edge)


			//
			// Clock goes high
			//

			// Write the current (relative) time of this data word
			dnow = 1.0/((double)m_clkfreq_hz) * i;
			now_ns = (unsigned)(dnow * 1e9 + 0.5);
			fprintf(fp, "#%d\n", now_ns);

			fprintf(fp, "1\'C\n");
			write_binary_trace(fp, (m_compressed)?31:32,
				m_data[i], "\'R\n");

			for(unsigned k=0; k<m_traces.size(); k++) {
				TRACEINFO *info = m_traces[k];
				write_binary_trace(fp, info, m_data[i]);
			}

			//
			// Clock goes to zero
			//

			// Add half a clock period to our time
			dnow += 1.0/((double)m_clkfreq_hz)/2.;
			now_ns = (unsigned)(dnow * 1e9 + 0.5);
			fprintf(fp, "#%d\n", now_ns);

			// Now finally write the clock as zero.
			fprintf(fp, "0\'C\n");
		}
	}
}

/*
 * writevcd
 *
 * Main user entry point for VCD file creation.  This just opens a file of the
 * given name, and writes the VCD info to it.  If the file cannot be opened,
 * an error is written to the standard error stream, and the routine returns.
 */
void	SCOPE::writevcd(const char *trace_file_name) {
	FILE	*fp = fopen(trace_file_name, "w");

	if (fp == NULL) {
		fprintf(stderr, "ERR: Cannot open %s for writing!\n", trace_file_name);
		fprintf(stderr, "ERR: Trace file not written\n");
		return;
	}

	writevcd(fp);

	fclose(fp);
}


