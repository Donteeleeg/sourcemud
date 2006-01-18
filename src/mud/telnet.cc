/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define TELCMDS
#define TELOPTS
#define SLC_NAMES

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdarg.h>

#include "error.h"
#include "server.h"
#include "network.h"
#include "parse.h"
#include "command.h"
#include "room.h"
#include "zmp.h"
#include "streams.h"
#include "color.h"
#include "message.h"
#include "telnet.h"
#include "settings.h"

#define TELOPT_MCCP2 86

// otput spacing in put()
#define OUTPUT_INDENT() \
	if (cur_col < margin) { \
		while (margin - cur_col >= 16) { \
			cur_col += 16; \
			add_output("                ", 16); \
		} \
		add_output("                ", margin - cur_col); \
		cur_col = margin; \
	}


// ---- BEGIN COLOURS ----

// color names
const char *color_value_names[] = {
	"normal",
	"black",
	"red",
	"green",
	"brown",
	"blue",
	"magenta",
	"cyan",
	"grey",
	"lightblack",
	"lightred",
	"lightgreen",
	"yellow",
	"lightblue",
	"lightmagenta",
	"lightcyan",
	"white",
	"darkred",
	"darkgreen",
	"darkyellow",
	"darkblue",
	"darkmagenta",
	"darkcyan",
	"darkgrey",
	NULL
};
// colour ansi values
const char *color_values[] = {
	ANSI_NORMAL,
	ANSI_BLACK,
	ANSI_RED,
	ANSI_GREEN,
	ANSI_BROWN,
	ANSI_BLUE,
	ANSI_MAGENTA,
	ANSI_CYAN,
	ANSI_GREY,
	ANSI_LIGHTBLACK,
	ANSI_LIGHTRED,
	ANSI_LIGHTGREEN,
	ANSI_YELLOW,
	ANSI_LIGHTBLUE,
	ANSI_LIGHTMAGENTA,
	ANSI_LIGHTCYAN,
	ANSI_WHITE,
	ANSI_DARKRED,
	ANSI_DARKGREEN,
	ANSI_DARKYELLOW,
	ANSI_DARKBLUE,
	ANSI_DARKMAGENTA,
	ANSI_DARKCYAN,
	ANSI_DARKGREY,
};
// colour type names
const char *color_type_names[] = {
	"normal",
	"title",
	"desc",
	"player",
	"npc",
	"item",
	"special",
	"admin",
	"exit",
	"stat",
	"statvbad",
	"statbad",
	"statgood",
	"statvgood",
	"bold",
	"talk",
	NULL
};
// default colour type mappings
const int color_type_defaults[] = {
	COLOR_NORMAL,
	COLOR_GREEN,
	COLOR_NORMAL,
	COLOR_MAGENTA,
	COLOR_BROWN,
	COLOR_LIGHTBLUE,
	COLOR_BROWN,
	COLOR_RED,
	COLOR_CYAN,
	COLOR_GREY,
	COLOR_LIGHTRED,
	COLOR_YELLOW,
	COLOR_LIGHTCYAN,
	COLOR_LIGHTGREEN,
	COLOR_BROWN,
	COLOR_CYAN
};
// colour type RGB values
const char* color_type_rgb[] = {
	"",
	"#0A0",
	"",
	"#A05",
	"#A50",
	"#0A0",
	"#A50",
	"#500",
	"#0AF",
	"",
	"#A00",
	"#AA5",
	"#5AF",
	"#5FA",
	"#A50",
	"#05A",
};

// ---- END COLOURS ----

TextBufferList TextBuffer::lists[] = {
	TextBufferList(2048),
	TextBufferList(4096),
	TextBufferList(8192),
	TextBufferList(16384),
};

char*
TextBufferList::alloc (void) {
	++ allocs;
	++ out;
	if (!list.empty()) {
		char* buf = list.back();
		list.pop_back();
		return buf;
	} else {
		char* buf = new char[size];
		return buf;
	}
}

int
TextBuffer::alloc (SizeType size)
{
	if (size == EMPTY)
		return -1;

	release();

	// do allocation
	bdata = lists[size].alloc();
	if (!bdata)
		return -1;

	bsize = size;
	return 0;
}
int
TextBuffer::grow (void)
{
	if (bsize == EMPTY) {
		// base allocation
		return alloc(SIZE_2048);
	} else if (bsize < COUNT - 1) {
		// upgrade
		char* bdatanew = lists[bsize + 1].alloc();
		if (bdatanew == NULL) {
			return -1;
		}

		// copy
		memcpy(bdatanew, bdata, size());
		lists[bsize].release(bdata);
		bsize = (SizeType)(bsize + 1);
		bdata = bdatanew;
		return 0;
	} else {
		// can't grow any more
		return -1;
	}
}
void
TextBuffer::release (void)
{
	if (bdata != NULL) {
		lists[bsize].release(bdata);
		bsize = EMPTY;
		bdata = NULL;
	}
}

SCRIPT_TYPE(Telnet);
TelnetHandler::TelnetHandler (int s_sock, const SockStorage& s_addr) : Scriptix::Native (AweMUD_TelnetType), SocketUser(s_sock)
{
	addr = s_addr;

	// various state settings
	in_cnt = out_cnt = sb_cnt = outchunk_cnt = esc_cnt = 0;
	istate = ISTATE_TEXT;
	ostate = OSTATE_TEXT;
	margin = 0;
	width = 70; // good default?
	chunk_size = 0;
	cur_col = 0;
	mode = NULL;
	memset(&io_flags, 0, sizeof(IOFlags));
	timeout = SettingsManager.get_activity_timeout();

	// initial telnet options
	io_flags.want_echo = true;
	io_flags.do_echo = false;
	io_flags.do_eor = false;
	io_flags.use_ansi = true;

	// send our initial telnet state and support options
	send_iac (2, WONT, TELOPT_ECHO);
	send_iac (2, WILL, TELOPT_EOR);
	send_iac (2, WILL, TELOPT_ZMP);
	send_iac (2, DO, TELOPT_NEW_ENVIRON);
	send_iac (2, DO, TELOPT_TTYPE);
	send_iac (2, DO, TELOPT_NAWS);
	send_iac (2, DO, TELOPT_LFLOW);

	// have MCCP support?
#ifdef HAVE_LIBZ
	zstate = NULL;
	send_iac (2, WILL, TELOPT_MCCP2);
#endif // HAVE_LIBZ

	// colors
	for (int i = 0; i < NUM_CTYPES; ++ i) {
		color_set[i] = -1;
	}

	// in stamp
	time (&in_stamp);
}

// ----- COMPRESSION -----
#ifdef HAVE_LIBZ

// initialize compression
bool
TelnetHandler::begin_mccp (void)
{
	if (!zstate) {
		// allocte
		zstate = new (UseGC) z_stream;
		if (zstate == NULL) {
			Log::Error << "Failed to allocate z_stream";
			return false;
		}

		// initialize
		memset(zstate, 0, sizeof(z_stream));
		if (deflateInit(zstate, Z_DEFAULT_COMPRESSION) != Z_OK) {
			Log::Error << "Failed to initialize z_stream: " << zstate->msg;
			return false;
		}

		// send the "all data will be compressed now message"
		send_iac(2, SB, TELOPT_MCCP2);
		send_iac(1, SE);

		return true;
	}

	return false;
}

// end compressed stream
void
TelnetHandler::end_mccp (void)
{
	if (zstate) {
		// free
		deflateEnd(zstate);
		zstate = NULL;
	}
}

#endif // HAVE_LIBZ

// disconnect
void
TelnetHandler::disconnect (void)
{
	// log
	Log::Network << "Telnet client disconnected: " << Network::get_addr_name(addr);

	// reduce count
	NetworkManager.connections.remove(addr);

#ifdef HAVE_LIBZ
	end_mccp();
#endif // HAVE_LIBZ

	// shutdown current mode
	if (mode) {
		mode->shutdown();
		mode = NULL;
	}

	// close socket
	if (sock != -1) {
		close(sock);
		sock = -1;
	}
}

// toggle echo
bool
TelnetHandler::toggle_echo (bool v)
{
	io_flags.want_echo = v;

	// force echoing?
	if (io_flags.force_echo) {
		io_flags.do_echo = v;
	// normal negotiation
	} else {
		if (v == false)
			io_flags.do_echo = false;
		send_iac (2, v ? WONT : WILL, TELOPT_ECHO);
	}

	return v;
}

/* output a data of text -
 * deal with formatting new-lines and such, and also
 * escaping/removing/translating AweMUD commands
 */
void
TelnetHandler::stream_put (const char *text, size_t len) 
{
	assert(text != NULL);

	// output a newline if we need one, such as after a prompt
	if (io_flags.need_newline) {
		add_output ("\n\r", 2);
		io_flags.soft_break = false;
		cur_col = 0;
	}
	io_flags.need_newline = false;

	// do loop
	char c;
	for (size_t ti = 0; ti < len; ++ti) {
		c = text[ti];
		switch (ostate) {
			// normal text
			case OSTATE_TEXT:
				switch (c) {
					// space?
					case ' ':
						end_chunk();

						// not soft-wrapped?
						if (!io_flags.soft_break) {
							// word wrap?
							if (width && cur_col + 1 >= width - 2) {
								add_output("\n\r", 2);
								cur_col = 0;
								io_flags.soft_break = true;
							} else {
								OUTPUT_INDENT()
								add_output(" ", 1);
								++cur_col;
							}
						}
						break;
					// newline?
					case '\n':
						end_chunk();
						
						// not after a soft-break
						if (!io_flags.soft_break) {
							add_output("\n\r", 2);
							cur_col = 0;
						}

						// this _is_ a hard break
						io_flags.soft_break = false;
						break;
					// escape sequence?
					case '\033':
						ostate = OSTATE_ESCAPE;
						esc_buf[0] = '\033';
						esc_cnt = 1;
						break;
					// tab?
					case '\t':
						end_chunk();
						OUTPUT_INDENT()
						add_output("    ", 4 % cur_col);
						cur_col += 4 % cur_col;
						break;
					// IAC byte
					case '\255':
						add_to_chunk("\255\255", 2); // escape IAC
						++chunk_size;
						break;
					// just data
					default:
						add_to_chunk(&c, 1);
						++chunk_size;
						break;
				}
				break;
			// escape
			case OSTATE_ESCAPE:
				// awecode?
				if (c == '!') {
					// we just want data
					esc_cnt = 0;
					ostate = OSTATE_AWECODE;
				// ansi
				} else if (c == '[') {
					// we keep whole code
					esc_buf[1] = c;
					esc_cnt = 2;
					ostate = OSTATE_ANSI;
				// unsupported/invalid
				} else {
					ostate = OSTATE_TEXT;
				}
				break;
			// awecode
			case OSTATE_AWECODE:
				// end?
				if (c != '!') {
					// add data
					if (esc_cnt < TELNET_MAX_ESCAPE_SIZE - 1)
						esc_buf[esc_cnt++] = c;
					break;
				}
				esc_buf[esc_cnt] = 0;

				// have we anything?
				if (esc_cnt == 0) {
					ostate = OSTATE_TEXT;
					break;
				}

				// process
				switch (esc_buf[0]) {
					// color command
					case 'C':
						// zmp color?
						if (io_flags.zmp_color) {
							const char* argv[2] = {"color.use", &esc_buf[1]};
							add_zmp(2, argv);
						}
						
						// ansi color?
						else if (io_flags.use_ansi) {
							int color = atoi(&esc_buf[1]);

							// reset color?
							if (color == 0) {
								// normalize colors
								add_to_chunk (ANSI_NORMAL, strlen(ANSI_NORMAL));

								// eat last color
								if (!colors.empty())
									colors.pop_back();

								// old color?
								if (!colors.empty())
									add_to_chunk (color_values[colors.back()], strlen (color_values[colors.back()]));

							// other color
							} else if (color > 0 && color < NUM_CTYPES) {
								// put color
								int cvalue = get_color (color);
								colors.push_back(cvalue);
								add_to_chunk (color_values[cvalue], strlen (color_values[cvalue]));
							}
						}

						break;
					// indent
					case 'I':
					{
						long mlen = strtol(&esc_buf[1], NULL, 10);
						if (mlen >= 0)
							set_indent(mlen);
						break;
					}
					// auto-indent
					case 'A':
						io_flags.auto_indent = (esc_buf[1] == '1');
						break;
				}

				// done
				ostate = OSTATE_TEXT;
				break;
			// ansi code
			case OSTATE_ANSI:
				// add data
				if (esc_cnt < TELNET_MAX_ESCAPE_SIZE)
					esc_buf[esc_cnt++] = c;

				// end?
				if (isalpha(c)) {
					if (io_flags.use_ansi) {
						add_to_chunk (esc_buf, esc_cnt);
					}
					ostate = OSTATE_TEXT;
				}
				break;
		}
	}

	// set output needs
	io_flags.need_prompt = true;
}

// clear the screen
void
TelnetHandler::clear_scr (void)
{
	// try clear screen sequence
	if (io_flags.use_ansi) {
		// ansi code, try both ways...
		*this << "\e[2J\e[H";
	} else {
		// cheap way
		for (uint i = 0; i < height; ++i)
			*this << "\n";
	}
}

// set indentation/margin
void
TelnetHandler::set_indent (uint amount)
{
	end_chunk();
	margin = amount;
}

// draw a progress bar
void
TelnetHandler::draw_bar (uint percent)
{
	// 20 part bar
	static const char* bar = "============";
	static const char* space = "            ";

	// clip percent
	if (percent > 100)
		percent = 100;

	// draw
	int parts = 12 * percent / 100;
	*this << "[";
	if (parts > 0)
		*this << StreamChunk(bar, parts);
	if (parts < 12)
		*this << StreamChunk(space, 12 - parts);
	*this << "]";
}

// process input
void
TelnetHandler::in_handle (char* buffer, size_t size)
{
	// deal with telnet options
	unsigned char c;
	size_t in_size = input.size();
	for (size_t i = 0; i < size; i ++) {
		c = buffer[i];
		switch (istate) {
			case ISTATE_TEXT:
				if (c == IAC) {
					istate = ISTATE_IAC;
					break;
				}

				// only printable characters thank you
				if (c == '\n' || isprint (c)) {
					// need to grow?
					if (in_cnt + 2 >= in_size) {
						if (input.grow()) {
							// input growth failure
							break;
						}
						in_size = input.size();
					}

					// do add
					input.data()[in_cnt ++] = c;
					input.data()[in_cnt] = '\0';

					// echo back normal characters
					if (c != '\n' && io_flags.do_echo)
						send_data (1, c);
				// basic backspace support
				} else if (c == 127) {
					if (in_cnt > 0 && input.data()[in_cnt - 1] != '\n') {
						input.data()[--in_cnt] = '\0';

						if (io_flags.do_echo)
							send_data (3, 127, ' ', 127);
					}
				}

				// get a newline in?
				if (c == '\n') {
					// handle the input data
					if (io_flags.do_echo)
						send_data (2, '\r', '\n');
					io_flags.need_newline = false;
					io_flags.need_prompt = true;

					// count current lines
					uint8 lines = 0;
					for (size_t i = 0; i < in_cnt; ++i) {
						// too many lines?
						if (lines == TELNET_BUFFER_LINES) {
							*this << CADMIN "You only have " << TELNET_BUFFER_LINES << " type-ahead lines." CNORMAL "\n";
							input.data()[i] = '\0';
							in_cnt = i;
							break;
						}

						// increment data count
						if (input.data()[i] == '\n')
							++lines;
					}
				}
				break;
			case ISTATE_IAC:
				istate = ISTATE_TEXT;
				switch (c) {
					case WILL:
						istate = ISTATE_WILL;
						break;
					case WONT:
						istate = ISTATE_WONT;
						break;
					case DO:
						istate = ISTATE_DO;
						break;
					case DONT:
						istate = ISTATE_DONT;
						break;
					case IAC:
						break;
					case SB:
						istate = ISTATE_SB;
						sb_cnt = 0;
						break;
					case EC:
						break;
					case EL:
					{
						if (in_size) {
							// find last newline
							char* nl = strrchr(input.data(), '\n');
							if (nl == NULL) {
								input.release();
								in_cnt = 0;
								in_size = 0;
							} else {
								// cut data
								in_cnt = nl - input.data();
								input.data()[in_cnt] = '\0';
							}
						}
						break;
					}
				}
				break;
			case ISTATE_SB:
				if (c == IAC) {
					istate = ISTATE_SE;
				} else {
					if (sb_cnt >= subrequest.size()) {
						if (subrequest.grow()) {
							// damn, growth failure
							break;
						}
					}
					subrequest.data()[sb_cnt++] = c;
				}
				break;
			case ISTATE_SE:
				if (c == SE) {
					process_sb ();
					subrequest.release();
					sb_cnt = 0;
					istate = ISTATE_TEXT;
				} else {
					istate = ISTATE_SB;

					if (sb_cnt >= subrequest.size()) {
						if (subrequest.grow()) {
							// damn, growth failure
							break;
						}
					}
					subrequest.data()[sb_cnt++] = IAC;
				}
				break;
			case ISTATE_WILL:
				istate = ISTATE_TEXT;
				switch (c) {
					case TELOPT_NAWS:
						// ignore
						break;
					case TELOPT_TTYPE:
						send_iac (3, SB, TELOPT_TTYPE, 1); // 1 is 'SEND'
						send_iac (1, SE);
						break;
					case TELOPT_NEW_ENVIRON:
						send_iac (4, SB, TELOPT_NEW_ENVIRON, 1, 0); // 1 is 'SEND', 0 is 'VAR'
						send_data (10, 'S', 'Y', 'S', 'T', 'E', 'M', 'T', 'Y', 'P', 'E');
						send_iac (1, SE);
						break;
					default:
						send_iac (2, DONT, c);
						break;
				}
				break;
			case ISTATE_WONT:
				istate = ISTATE_TEXT;
				switch (c) {
					case TELOPT_NAWS:
						// reset to default width
						width = 70;
						break;
				}
				break;
			case ISTATE_DO:
				istate = ISTATE_TEXT;
				switch (c) {
					case TELOPT_ECHO:
						if (io_flags.want_echo)
							io_flags.do_echo = true;
						break;
					case TELOPT_EOR:
						if (!io_flags.do_eor) {
							io_flags.do_eor = true;
							send_iac (2, WILL, TELOPT_EOR);
						}
						break;
#ifdef HAVE_LIBZ
					case TELOPT_MCCP2:
						begin_mccp();
						break;
#endif // HAVE_LIBZ
					case TELOPT_ZMP: {
						// enable ZMP support
						io_flags.zmp = true;
						// send zmp.ident command
						const char* argv[4];
						argv[0] = "zmp.ident"; // command
						argv[1] = "AweMUD NG"; // name
						argv[2] = VERSION; // version
						argv[3] = "Powerful C++ MUD server software"; // about
						send_zmp(4, argv);
						// check for net.awemud package
						argv[0] = "zmp.check";
						argv[1] = "net.awemud.";
						send_zmp(2, argv);
						// check for color.define command
						argv[0] = "zmp.check";
						argv[1] = "color.define";
						send_zmp(2, argv);
						break;
					}
					default:
						send_iac (2, WONT, c);
						break;
				}
				break;
			case ISTATE_DONT:
				istate = ISTATE_TEXT;
				switch (c) {
					case TELOPT_ECHO:
						if (!io_flags.force_echo)
							io_flags.do_echo = false;
						break;
					case TELOPT_EOR:
						if (io_flags.do_eor) {
							io_flags.do_eor = false;
							send_iac (2, WONT, TELOPT_EOR);
						}
						break;
					default:
						send_iac (2, WONT, c);
						break;
				}
				break;
			// NEWS negotiation
			default:
				istate = ISTATE_TEXT;
				break;
		}
	}

	process_input();
}

// handle entered commands
void
TelnetHandler::process_input (void)
{
	// have we any input?
	if (!in_cnt)
		return;

	// get one data of data
	char* data = input.data();
	char* nl = (char*)memchr(input.data(), '\n', in_cnt);
	if (nl == NULL)
		return;
	*nl = '\0';

	// do process
	process_command(data);

	// consume command data
	size_t len = nl - input.data() + 1;
	in_cnt -= len;
	memmove(input.data(), input.data() + len, in_cnt);
}

// handle a specific command
void
TelnetHandler::process_command (char* cbuffer)
{
	// time stamp
	time (&in_stamp);

	// force output update
	io_flags.need_prompt = true;

	// general input?
	if (cbuffer[0] != '!' && mode) {
		mode->process(cbuffer);
	// if it starts with !, process as a telnet layer command
	} else {
		process_telnet_command(&cbuffer[1]);
	}
}

void
TelnetHandler::set_mode (ITelnetMode* new_mode)
{
	// close old mode
	if (mode)
		mode->shutdown();

	mode = new_mode;

	// initialize new mode
	if (mode && mode->initialize()) {
		mode = NULL;
		disconnect();
	}
}

void
TelnetHandler::check_mode (void)
{
	if (mode)
		mode->check();
}

void
TelnetHandler::process_telnet_command(char* data)
{
	StringList args = explode(data, ' ');
	// enable/disable color
	if (args.size() == 2 && args.front() == "color") {
		if (args[1] == "on") {
			io_flags.use_ansi = true;
			*this << CADMIN "ANSI Color Enabled" CNORMAL "\n";
			return;
		} else if (args[1] == "off") {
			io_flags.use_ansi = false;
			*this << "ANSI Color Disabled\n";
			return;
		}
	}

	// set screen width and height
	else if (args.size() == 3 && args.front() ==  "screen") {
		int new_width = tolong(args[1]);
		int new_height = tolong(args[2]);
		if (new_width >= 20 && new_height >= 10) {
			width = new_width;
			height = new_height;
			*this << CADMIN "Screen: " << width << 'x' << height << CNORMAL "\n";
			return;
		}
	}

	// enable/disable server echo
	else if (args.size() == 2 && args.front() == "echo") {
		if (args[1] == "on") {
			io_flags.force_echo = true;
			if (io_flags.want_echo)
				io_flags.do_echo = true;
			*this << CADMIN "Echo Enabled" CNORMAL "\n";
			return;
		} else if (args[1] == "off") {
			io_flags.force_echo = false;
			if (io_flags.want_echo)
				send_iac(WONT, TELOPT_ECHO);
			*this << CADMIN "Echo Disabled" CNORMAL "\n";
			return;
		}
	}

	// if we reached this, all the above parsing must have failed
	*this << "Telnet commands:\n";
	*this << " !color <on|off> -- Enable or disable ANSI color.\n";
	*this << " !screen [w] [h] -- Set the column width of your display.\n";
	*this << " !echo <on|off>  -- Enable or disable forced server echoing.\n";
	*this << " !help           -- Show this message.\n";
}

// process a telnet sub command
void
TelnetHandler::process_sb (void)
{
	char* data = subrequest.data();
	if (data == NULL || sb_cnt == 0)
		return;

	switch (data[0]) {
		// handle ZMP
		case TELOPT_ZMP:
			if (has_zmp())
				process_zmp(sb_cnt - 1, (char*)&data[1]);
			break;
		// resize of telnet window
		case TELOPT_NAWS:
			width = ntohs(*(uint16*)&data[1]);
			height = ntohs(*(uint16*)&data[3]);
			break;
		// handle terminal type
		case TELOPT_TTYPE:
			// proper input?
			if (sb_cnt > 2 && data[1] == 0) {
				// xterm?
				if (sb_cnt == 7 && !memcmp(&data[2], "XTERM", 5))
					io_flags.xterm = true;
				// ansi 
				if (sb_cnt == 6 && !memcmp(&data[2], "ANSI", 4))
					io_flags.ansi_term = true;

				// set xterm title
				if (io_flags.xterm) {
					send_data (3, '\033', ']', ';');
					add_output ("AweMUD NG", 9);
					send_data (1, '\a');
				}
			}
			break;
		// handle new environ
		case TELOPT_NEW_ENVIRON:
			// proper input - IS, VAR
			if (sb_cnt > 3 && data[1] == 0 && data[2] == 0) {
				// system type?
				if (sb_cnt >= 13 && !memcmp(&data[3], "SYSTEMTYPE", 10)) {
					// value is windows?
					if (sb_cnt >= 19 && data[13] == 1 && !memcmp(&data[14], "WIN32", 5)) {
						// we're running windows telnet, most likely
						*this << "\n---\n" CADMIN "Warning:" CNORMAL " AweMUD has detected that "
							"you are using the standard Windows telnet program.  AweMUD will "
							"enable the slower server-side echoing.  You may disable this by "
							"typing " CADMIN "!echo off" CNORMAL " at any time.\n---\n";
						io_flags.force_echo = true;
						if (io_flags.want_echo)
							io_flags.do_echo = true;
					}
				}
			}
			break;
	}
}

// flush out the output, write prompt
void
TelnetHandler::prepare (void)
{
	// fix up color
	if (!colors.empty()) {
		if (io_flags.use_ansi)
			add_to_chunk(ANSI_NORMAL, strlen(ANSI_NORMAL));
		colors.resize(0);
	}

	// end chunk
	end_chunk();

	// if we need an update to prompt, do so
	if (io_flags.need_prompt) {
		// prompt
		if (mode)
			mode->prompt();
		else
			*this << ">";

		// clean output
		end_chunk();
		add_output (" ", 1);

		// GOAHEAD telnet command
		if (io_flags.do_eor)
			send_iac (1, EOR);
		
		io_flags.need_prompt = false;
		io_flags.need_newline = true;
		cur_col = 0;
	}
}

// send out a telnet command
void
TelnetHandler::send_iac (uint count, ...)
{
	va_list va;
	unsigned char buffer[16]; // simple buffer
	uint bc = 1; // buffer index
	uint byte;
	buffer[0] = IAC; // we need to send an IAC

	va_start (va, count);
	// loop thru args
	for (uint i = 0; i < count; i ++) {
		byte = va_arg (va, uint);

		// add byte
		buffer[bc ++] = byte;
		if (bc >= 16) {
			add_output ((char *)buffer, 16);
			bc = 0;
		}

		// doube up on IAC character
		if (byte == IAC)
			buffer[bc ++] = IAC;
		if (bc >= 16) {
			add_output ((char *)buffer, 16);
			bc = 0;
		}
	}

	// write out rest of buffer
	if (bc)
		add_output ((char *)buffer, bc);
}

// send out telnet data
void
TelnetHandler::send_data (uint count, ...)
{
	va_list va;
	unsigned char buffer[16]; // simple buffer
	uint bc = 0; // buffer index
	uint byte;

	va_start (va, count);
	// loop thru args
	for (uint i = 0; i < count; i ++) {
		byte = va_arg (va, uint);

		// add byte
		buffer[bc ++] = byte;
		if (bc >= 16) {
			add_output ((char *)buffer, 16);
			bc = 0;
		}

		// doube up on IAC character
		if (byte == IAC)
			buffer[bc ++] = IAC;
		if (bc >= 16) {
			add_output ((char *)buffer, 16);
			bc = 0;
		}
	}

	// write out rest of buffer
	if (bc)
		add_output ((char *)buffer, bc);
}

void
TelnetHandler::add_to_chunk (const char *data, size_t len) {
	// output indenting
	OUTPUT_INDENT()

	// grow chunk buffer if needed
	while (len + outchunk_cnt >= outchunk.size()) {
		// chunk buffer overflow - force dump
		if (outchunk.grow()) {
			end_chunk();
			break;
		}
	}

	// still too big? cut excess chunks
	while (len >= outchunk.size()) {
		add_output(data, outchunk.size());
		data += outchunk.size();
		len -= outchunk.size();
	}

	// append remaining data
	if (len > 0) {
		memcpy(outchunk.data() + outchunk_cnt, data, len * sizeof(char));
		outchunk_cnt += len;
	}
}

void
TelnetHandler::add_output (const char *data, size_t len)
{
#ifdef HAVE_LIBZ
	if (zstate) {
		char buffer[512]; // good enough for now

		// setup
		zstate->next_in = (Bytef*)bytes;
		zstate->avail_in = len;
		zstate->next_out = (Bytef*)buffer;
		zstate->avail_out = sizeof(buffer);
		
		// keep compressing until we have no more input
		do {
			deflate(zstate, Z_NO_FLUSH);

			// out of output?
			if (!zstate->avail_out) {
				add_to_out_buffer(buffer, sizeof(buffer));
				zstate->next_out = (Bytef*)buffer;
				zstate->avail_out = sizeof(buffer);
			}
		} while (zstate->avail_in);

		// have we any remaining output?
		deflate(zstate, Z_SYNC_FLUSH);
		if (zstate->avail_out < sizeof(buffer))
			add_to_out_buffer(buffer, sizeof(buffer) - zstate->avail_out);
	} else
#endif // HAVE_LIBZ

	add_to_out_buffer(data, len);
}

void
TelnetHandler::add_to_out_buffer (const char* data, size_t len)
{
	// empty?  why both...
	if (len == 0)
		return;
	// grow output buffer if needed
	while (len + out_cnt >= output.size()) {
		// can't grow enough?
		if (output.grow()) {
			// overflow message
			const char* overflow_msg = "\r\n\r\n[[ ** OUTPUT OVERFLOW ** ]\r\n\r\n]";
			strncpy(output.data(), overflow_msg, output.size());
			out_cnt = strlen(overflow_msg);

			// copy as much of _end_ of new buffer as is possible
			size_t remain = output.size() - out_cnt;
			if (len > remain) {
				data += len - remain;
				len = remain;
			}

			// copy to buffer
			memcpy(output.data() + out_cnt, data, len);
			out_cnt += len;

			// log it and return
			Log::Warning << "Telnet output buffer overflow"; // FIXME: log which client, somehow
			return;
		}
	}

	// append remaining data
	memcpy(output.data() + out_cnt, data, len * sizeof(char));
	out_cnt += len;
}

void
TelnetHandler::end_chunk (void)
{
	// only if we have data
	if (outchunk_cnt > 0) {
		// need to word-wrap?
		if (width > 0 && chunk_size + cur_col >= width - 2) {
			add_output("\n\r", 2);
			cur_col = 0;
			OUTPUT_INDENT()
		}

		// do output
		add_output(outchunk.data(), outchunk_cnt);
		outchunk_cnt = 0;
		cur_col += chunk_size;
		chunk_size = 0;

		io_flags.soft_break = false;
	}
}

// check various timeouts
int
TelnetHandler::check_time (void)
{
	// get time diff
	time_t now;
	time (&now);
	unsigned long diff = (unsigned long)difftime(now, in_stamp);

	// check time..
	if (diff >= (timeout * 60)) {
		// disconnect the dink
		*this << CADMIN "You are being disconnected for lack of activity." CNORMAL "\n";
		Log::Info << "Connection timeout (" << timeout << " minutes of no input).";
		disconnect();
		return 1;
	}

	// no disconnections... yet
	return 0;
}

char
TelnetHandler::get_poll_flags (void)
{
	char flags = POLLSYS_READ;
	if (out_cnt > 0)
		flags |= POLLSYS_WRITE;
	return flags;
}

void
TelnetHandler::out_ready (void)
{
	if (out_cnt == 0)
		return;

	int ret = send(sock, output.data(), out_cnt, MSG_DONTWAIT);
	if (ret == -1 && errno != EAGAIN && errno != EINTR) {
		Log::Error << "send() failed: " << strerror(errno);
		return;
	}

	if (ret <= 0)
		return;

	if (out_cnt - ret > 0) {
		memmove(output.data(), output.data() + out_cnt, out_cnt - ret);
	} else if (out_cnt == (uint)ret) {
		output.release();
		out_cnt = 0;
	}
}

void
TelnetHandler::hangup (void)
{
	disconnect();
}