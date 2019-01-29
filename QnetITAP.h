/*
 *   Copyright (C) 2018 by Thomas A. Early N7TAE
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#pragma once

#include <atomic>
#include <string>

#include <netinet/in.h>
#include "Random.h"	// for streamid generation
#include "UnixDgramSocket.h"

#define CALL_SIZE 8
#define IP_SIZE 15

enum REPLY_TYPE {
	RT_TIMEOUT,
	RT_ERROR,
	RT_UNKNOWN,
	RT_HEADER,
	RT_DATA,
	RT_HEADER_ACK,
	RT_DATA_ACK,
	RT_PONG
};

// Icom Terminal and Access Point Mode data structure
#pragma pack(push, 1)
typedef struct itap_tag {
	unsigned char length;
		// 41 for header (42 for writing)
		// 16 for voice  (17 for writing)
	unsigned char type;
		// 0x03U pong
		// 0x10U header from icom
		// 0x11U acknowledgment
		// 0x12U data   from icom (it's EOT if voice.sequence bit 0x40 is set)
		// 0x13U acknowledgment
		// 0x20U header to icom
		// 0x21U header acknowledgment
		// 0x22U   data to icom
		// 0x23U   data acknowledgment
	union {
		struct {
			unsigned char flag[3];
			unsigned char r2[8];
			unsigned char r1[8];
			unsigned char ur[8];
			unsigned char my[8];
			unsigned char nm[4];
		} header;
		struct {
			unsigned char counter;	// ordinal counter is reset with each header
			unsigned char sequence;	// is modulo 21
			unsigned char ambe[9];
			unsigned char text[3];
		} voice;
	};
} SITAP;
#pragma pack(pop)

class CQnetITAP
{
public:
	// functions
	CQnetITAP(int mod);
	~CQnetITAP();
	void Run(const char *cfgfile);

	// data
	static std::atomic<bool> keep_running;

private:
	int assigned_module;
	unsigned short COUNTER;
	// functions
	bool Initialize(const char *cfgfile);
	static void SignalCatch(const int signum);
	bool ProcessGateway(const int len, const unsigned char *raw);
	bool ProcessITAP(const unsigned char *raw);
	int OpenITAP();
	int SendTo(const unsigned char *buf);
	REPLY_TYPE GetITAPData(unsigned char *buf);
	void calcPFCS(const unsigned char *packet, unsigned char *pfcs);

	// read configuration file
	bool ReadConfig(const char *);

	// config data
	char RPTR_MOD;
	std::string ITAP_DEVICE, RPTR;
	bool log_qso;

	// parameters
	int serfd;
	unsigned char tapcounter;

	// helpers
	CRandom random;

	// unix sockets
	std::string modem2gate, gate2modem;
	CUnixDgramWriter Modem2Gate;
	CUnixDgramReader Gate2Modem;
};
