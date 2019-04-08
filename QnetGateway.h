/*
 *   Copyright (C) 2018,2019 by Thomas Early N7TAE
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

#include <map>
#include <set>

#include "QnetTypeDefs.h"
#include "SEcho.h"
#include "UnixDgramSocket.h"
#include "aprs.h"
#include "SockAddress.h"

#define MAXHOSTNAMELEN 64
#define CALL_SIZE 8
#define MAX_DTMF_BUF 32

typedef struct to_remote_g2_tag {
	unsigned short streamid;
	CSockAddress toDstar;
	time_t last_time;
} STOREMOTEG2;

typedef struct torepeater_tag {
	// help with header re-generation
	SDSVT saved_hdr; // repeater format
	CSockAddress saved_addr;

	unsigned short streamid;
	CSockAddress addr;
	time_t last_time;
	unsigned char sequence;
} STOREPEATER;

typedef struct band_txt_tag {
	unsigned short streamID;
	unsigned char flags[3];
	char lh_mycall[CALL_SIZE + 1];
	char lh_sfx[5];
	char lh_yrcall[CALL_SIZE + 1];
	char lh_rpt1[CALL_SIZE + 1];
	char lh_rpt2[CALL_SIZE + 1];
	time_t last_time;
	char txt[64];   // Only 20 are used
	unsigned short txt_cnt;
	bool sent_key_on_msg;

	char dest_rptr[CALL_SIZE + 1];

	// try to process GPS mode: GPRMC and ID
	char temp_line[256];
	unsigned short temp_line_cnt;
	char gprmc[256];
	char gpid[256];
	bool is_gps_sent;
	time_t gps_last_time;

	int num_dv_frames;
	int num_dv_silent_frames;
	int num_bit_errors;
} SBANDTXT;

class CQnetGateway {
public:
	CQnetGateway();
	~CQnetGateway();
	void Process();
	bool Init(char *cfgfile);

private:
	// network type
	int af_family;
	// text stuff
	bool new_group[3] = { true, true, true };
	unsigned char header_type = 0;
	short to_print[3] = { 0, 0, 0 };
	bool ABC_grp[3] = { false, false, false };
	bool C_seen[3] = { false, false, false };

	SPORTIP g2_external, ircddb;

	CUnixDgramReader Link2Gate, Modem2Gate;
	CUnixDgramWriter Gate2Link, Gate2Modem[3];

	std::string gate2link, link2gate, gate2modem[3], modem2gate;

	std::string OWNER, owner, FILE_STATUS, FILE_DTMF, FILE_ECHOTEST, IRCDDB_PASSWORD, FILE_QNVOICE_FILE;

	bool GATEWAY_SEND_QRGS_MAP, GATEWAY_HEADER_REGEN, APRS_ENABLE, playNotInCache;
	bool LOG_DEBUG, LOG_IRC, LOG_DTMF, LOG_QSO;

	int TIMING_PLAY_WAIT, TIMING_PLAY_DELAY, TIMING_TIMEOUT_ECHO, TIMING_TIMEOUT_VOICEMAIL, TIMING_TIMEOUT_REMOTE_G2, TIMING_TIMEOUT_LOCAL_RPTR, dtmf_digit;

	unsigned int vPacketCount;

	std::set<std::string> findRoute;

	// data needed for aprs login and aprs beacon
	// RPTR defined in aprs.h
	SRPTR rptr;

	// local repeater modules being recorded
	// This is for echotest and voicemail
	SECHO recd[3], vm[3];
	SDSVT recbuf; // 56 or 27, max is 56

	// the streamids going to remote Gateways from each local module
	STOREMOTEG2 to_remote_g2[3]; // 0=A, 1=B, 2=C

	// input from remote G2 gateway
	int g2_sock = -1;
	CSockAddress fromDstar;

	// Incoming data from remote systems
	// must be fed into our local repeater modules.
	STOREPEATER toRptr[3]; // 0=A, 1=B, 2=C

	SDSVT end_of_audio;

	// send packets to g2_link
	struct sockaddr_in plug;

	// for talking with the irc server
	CIRCDDB *ii;
	// for handling APRS stuff
	CAPRS *aprs;

	// text coming from local repeater bands
	SBANDTXT band_txt[3]; // 0=A, 1=B, 2=C

	/* Used to validate MYCALL input */
	regex_t preg;

	// CACHE used to cache users, repeaters,
	// gateways, IP numbers coming from the irc server

	std::map<std::string, std::string> user2rptr_map, rptr2gwy_map, gwy2ip_map;

	pthread_mutex_t irc_data_mutex = PTHREAD_MUTEX_INITIALIZER;

	// dtmf stuff
	int dtmf_buf_count[3];
	char dtmf_buf[3][MAX_DTMF_BUF + 1];
	int dtmf_last_frame[3];
	unsigned int dtmf_counter[3];

	bool VoicePacketIsSync(const unsigned char *text);
	void AddFDSet(int &max, int newfd, fd_set *set);
	int open_port(const SPORTIP &pip);
	void calcPFCS(unsigned char *packet, int len);
	void GetIRCDataThread();
	int get_yrcall_rptr_from_cache(const std::string &call, std::string &arearp_cs, std::string &zonerp_cs, char *mod, std::string &ip, char RoU);
	bool get_yrcall_rptr(const std::string &call, std::string &arearp_cs, std::string &zonerp_cs, char *mod, std::string &ip, char RoU);
	void PlayFileThread(SECHO &edata);
	void compute_aprs_hash();
	void APRSBeaconThread();
	void ProcessTimeouts();
	void ProcessSlowData(unsigned char *data, unsigned short sid);
	void ProcessG2(const ssize_t g2buflen, const SDSVT &g2buf, const bool is_from_g2);
	void ProcessModem();
	bool Flag_is_ok(unsigned char flag);
	void UnpackCallsigns(const std::string &str, std::set<std::string> &set, const std::string &delimiters = ",");
	void PrintCallsigns(const std::string &key, const std::set<std::string> &set);

	// read configuration file
	bool ReadConfig(char *);

/* aprs functions, borrowed from my retired IRLP node 4201 */
	void gps_send(short int rptr_idx);
	bool verify_gps_csum(char *gps_text, char *csum_text);
	void build_aprs_from_gps_and_send(short int rptr_idx);

	void qrgs_and_maps();

	void set_dest_rptr(int mod_ndx, char *dest_rptr);
	bool validate_csum(SBANDTXT &bt, bool is_gps);
};
