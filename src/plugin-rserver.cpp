/*
 * ircbot-rserver.cpp
 *
 *  Created on: 10 Dec 2011
 *      Author: oaskivvy@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2011 SooKee oaskivvy@gmail.com               |
'------------------------------------------------------------------'

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.

http://www.gnu.org/licenses/gpl-2.0.html

'-----------------------------------------------------------------*/

#include <skivvy/plugin-rserver.h>

#include <ctime>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>

#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <skivvy/str.h>
#include <skivvy/logrep.h>
#include <skivvy/types.h>
#include <skivvy/socketstream.h>

namespace skivvy { namespace ircbot {

IRC_BOT_PLUGIN(RServerIrcBotPlugin);
PLUGIN_INFO("rserver", "Remote Server", "0.1");

using namespace skivvy::types;
using namespace skivvy::utils;
using namespace skivvy::string;

const str RSERVER_PORT = "rserver.port";
const RServerIrcBotPlugin::port RSERVER_PORT_DEFAULT = 7334L;
const str RSERVER_HOST = "rserver.host";
const str RSERVER_HOST_DEFAULT = "0.0.0.0";

RServerIrcBotPlugin::RServerIrcBotPlugin(IrcBot& bot)
: BasicIrcBotPlugin(bot), ss(::socket(PF_INET, SOCK_STREAM, 0))
{
	fcntl(ss, F_SETFL, fcntl(ss, F_GETFL, 0) | O_NONBLOCK);
}

RServerIrcBotPlugin::~RServerIrcBotPlugin() {}

bool RServerIrcBotPlugin::bind()//port p, const std::string& iface)
{
	bug_func();
	port p = bot.get(RSERVER_PORT, RSERVER_PORT_DEFAULT);
	str host = bot.get(RSERVER_HOST, RSERVER_HOST_DEFAULT);
	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(p);
	addr.sin_addr.s_addr = inet_addr(host.c_str());
	return ::bind(ss, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != -1;
}

bool RServerIrcBotPlugin::listen()//port p)
{
	bug_func();
	if(!bind())
	{
		log("ERROR: " << std::strerror(errno));
		return false;
	}
	if(::listen(ss, 10) == -1)
	{
		log("ERROR: " << std::strerror(errno));
		return false;
	}
	while(!done)
	{
		sockaddr connectee;
		socklen_t connectee_len = sizeof(sockaddr);
		socket cs;

		while(!done)
		{
			while(!done && (cs = ::accept4(ss, &connectee, &connectee_len, SOCK_NONBLOCK)) == -1)
			{
				if(!done)
				{
					if(errno ==  EAGAIN || errno == EWOULDBLOCK)
						std::this_thread::sleep_for(std::chrono::seconds(1));
					else
					{
						log("ERROR: " << strerror(errno));
						return false;
					}
				}
			}

			if(!done)
				std::async(std::launch::async, [&]{ process(cs); });
		}
	}
	return true;
}

void RServerIrcBotPlugin::process(socket cs)
{
	net::socketstream ss(cs);
	// receive null terminated string
	std::string cmd;
	char c;
	while(ss.get(c) && c)
		cmd.append(1, c);
	bug("cmd: " << cmd);
	if(!trim(cmd).empty())
	{
		soss oss;
		bot.exec(cmd, &oss);
		ss << oss.str() << '\0' << std::flush;
	}
}

void RServerIrcBotPlugin::on(const message& msg)
{
	BUG_COMMAND(msg);
}

void RServerIrcBotPlugin::off(const message& msg)
{
	BUG_COMMAND(msg);
}

bool RServerIrcBotPlugin::initialize()
{
	bug_func();
	con = std::async(std::launch::async, [&]{ listen(); });
	return true;
}

// INTERFACE: IrcBotPlugin

str RServerIrcBotPlugin::get_id() const { return ID; }
std::string RServerIrcBotPlugin::get_name() const { return NAME; }
std::string RServerIrcBotPlugin::get_version() const { return VERSION; }

void RServerIrcBotPlugin::exit()
{
	done = true;
	close(ss);
	// TODO: find a way to get past block...
	//if(con.valid()) con.get();
	if(con.valid())
		if(con.wait_for(std::chrono::seconds(10)) == std::future_status::ready)
			con.get();
}

}} // sookee::ircbot
