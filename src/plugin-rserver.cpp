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

#include <hol/string_utils.h>
#include <hol/bug.h>
#include <hol/simple_logger.h>
#include <hol/small_types.h>
#include <skivvy/logrep.h>
#include <skivvy/utils.h>
#include <skivvy/socketstream.h>
#include <skivvy/irc-constants.h>

#include <boost/asio.hpp>

// TODO: Fix loging (remove this)
#define log(m) LOG::A << m

namespace skivvy {
namespace ircbot {
namespace rserver {

IRC_BOT_PLUGIN(RServerIrcBotPlugin);
PLUGIN_INFO("rserver", "Remote Server", "0.1");

using namespace skivvy::utils;
using namespace hol::small_types::basic;
using namespace hol::simple_logger;
using namespace boost;

using clock = std::chrono::system_clock;

const str PORT_KEY = "rserver.port";
const unsigned short PORT_VAL = 7334;
const str BIND_KEY = "rserver.bind";
const str BIND_VAL = "0.0.0.0";

const auto BIND_RETRIES_KEY = "rserver.bind.retries";
const auto BIND_RETRIES_VAL = 10U;
const auto BIND_RETRY_PAUSE_KEY = "rserver.bind.retry.pause.ms";
const auto BIND_RETRY_PAUSE_VAL = 10000U;

const auto ACCEPT_IPV4_KEY = "rserver.accept.ipv4";
const auto ACCEPT_IPV6_KEY = "rserver.accept.ipv6";

RServerIrcBotPlugin::RServerIrcBotPlugin(IrcBot& bot)
: BasicIrcBotPlugin(bot)
, done(false)
, dusted(false)
{
}

bool RServerIrcBotPlugin::accept()
{
	auto port = bot.get(PORT_KEY, PORT_VAL);
	auto host = bot.get(BIND_KEY, BIND_VAL);

	unsigned retry_pause_millis = bot.get(BIND_RETRY_PAUSE_KEY, BIND_RETRY_PAUSE_VAL);

	if(retry_pause_millis < 500)
		retry_pause_millis = 500;

	if(retry_pause_millis > 60000)
		retry_pause_millis = 60000;

	asio::io_service io_service;
	asio::ip::tcp::acceptor acceptor{io_service, {asio::ip::tcp::v4(), port}};

	while(!done)
	{
		asio::ip::tcp::socket cs(io_service);
		acceptor.accept(cs);

		if(cs.local_endpoint().address().is_v6())
		{
			auto ip = cs.local_endpoint().address().to_v6();
			if(ipv6accepts.empty() || ipv6accepts.count(ip))
				std::thread(&RServerIrcBotPlugin::process, this, std::move(cs)).detach();
			else
			{
				cs.close();
				LOG::S << "rejecting unauthorized connection attempt from: " << ip.to_string();
			}
		}
		else if(cs.local_endpoint().address().is_v4())
		{
			auto ip = cs.local_endpoint().address().to_v4();
			if(ipv6accepts.empty() || ipv4accepts.count(ip))
				std::thread(&RServerIrcBotPlugin::process, this, std::move(cs)).detach();
			else
			{
				cs.close();
				LOG::S << "rejecting unauthorized connection attempt from: " << ip.to_string();
			}
		}
		else
		{
			cs.close();
			LOG::W << "only accepting connections for ipv4 & ipv6";
			continue;
		}
	}
	acceptor.close();
	dusted = true;
	return true;
}

void RServerIrcBotPlugin::process(asio::ip::tcp::socket cs)
{
	lock_guard lock(mtx);

	str cmd;

//	sgl(ss, cmd, '\0');

	std::array<char, 1024> buf;
	boost::system::error_code ec;
	while(auto len = cs.read_some(asio::buffer(buf), ec))
	{
		cmd.append(buf.data(), len);
		if(len < buf.size())
			break;
	}

	if(hol::trim_mute(cmd, "\0\r\n\t ").empty())
		return;

	if(cmd == "/get_status")
	{
		for(const auto& s: status)
		{
//			cs << s << '\n';
			auto buf = s + "\n";
			auto pos = buf.data();
			auto end = pos + buf.size();
			while((pos += cs.write_some(asio::buffer(pos, end - pos), ec)) < end)
				if(ec)
					break;
		}
		cs.write_some(asio::buffer("\0", 1), ec);
		status.clear();
	}
	else
	{
		soss oss;
		bot.exec(cmd, &oss);
//		cs << oss.str() << '\0' << std::flush;
		auto buf = oss.str() + "\0";
		auto pos = buf.data();
		auto end = pos + buf.size();
		while((pos += cs.write_some(asio::buffer(pos, end - pos), ec)) < end)
			if(ec)
				break;
	}
}

void RServerIrcBotPlugin::on(const message& msg)
{
	(void) msg;
//	BUG_COMMAND(msg);
}

void RServerIrcBotPlugin::off(const message& msg)
{
	(void) msg;
//	BUG_COMMAND(msg);
}

bool RServerIrcBotPlugin::initialize()
{
	bug_fun();

	boost::system::error_code ec;

	{
		str_vec ips = bot.get_vec(ACCEPT_IPV4_KEY);

		for(const str& ip: ips)
		{
			auto ipv4 = asio::ip::address_v4::from_string(ip, ec);

			if(ec)
				LOG::E << ec.message();
			else
			{
				ipv4accepts.insert(ipv4);
				LOG::I << "ADDING ALLOWED IPv4: " << ip;
			}
		}
	}

	{
		str_vec ips = bot.get_vec(ACCEPT_IPV6_KEY);

		for(const str& ip: ips)
		{
			auto ipv6 = asio::ip::address_v6::from_string(ip, ec);

			if(ec)
				LOG::E << ec.message();
			else
			{
				ipv6accepts.insert(ipv6);
				LOG::I << "ADDING ALLOWED IPv6: " << ip;
			}
		}
	}

	LOG::I << "RSERVER: " << ((ipv4accepts.empty() && ipv6accepts.empty())?"INSECURE":"SECURE") << " MODE";

	con = std::async(std::launch::async, &RServerIrcBotPlugin::accept, this);

	bot.add_monitor(*this);
	return true;
}

// INTERFACE: IrcBotMonitor

static const str_vec event_commands
{
	  irc::NOTICE
	, irc::MODE
};

void RServerIrcBotPlugin::event(const message& msg)
{
//	bug_fun();
	if(std::find(event_commands.begin(), event_commands.end(), msg.command) == event_commands.end())
		return;

//	BUG_COMMAND(msg);
	if(msg.command == irc::NOTICE)
	{
		lock_guard lock(mtx);
		add_status_msg(msg.get_trailing());
	}
	if(msg.command == irc::MODE)
	{
		auto params = msg.get_params();
		if(params.size() == 3 && params[2] == bot.nick)
		{
			//---------------------------------------------------
			//                  line: :Galik!~galik@unaffiliated/galik MODE #skivvy +o Skivvy
			//                prefix: Galik!~galik@unaffiliated/galik
			//               command: MODE
			//                params:  #skivvy +o Skivvy
			// get_servername()     :
			// get_nickname()       : Galik
			// get_user()           : ~galik
			// get_host()           : unaffiliated/galik
			// param                : #skivvy
			// param                : +o
			// param                : Skivvy
			// middle               : #skivvy
			// middle               : +o
			// middle               : Skivvy
			// trailing             :
			// get_nick()           : Galik
			// get_chan()           : #skivvy
			// get_user_cmd()       :
			// get_user_params()    :
			//---------------------------------------------------
			if(params[1] == "+o")
			{
				lock_guard lock(mtx);
				add_status_msg(msg.get_nickname() + " gives channel operator status to "
					+ bot.nick);
			}
		}
	}
}

// INTERFACE: IrcBotPlugin

str RServerIrcBotPlugin::get_id() const { return ID; }
std::string RServerIrcBotPlugin::get_name() const { return NAME; }
std::string RServerIrcBotPlugin::get_version() const { return VERSION; }

void RServerIrcBotPlugin::exit()
{
	this->done = true;

	clock::time_point timeout = clock::now() + std::chrono::seconds(30);

	while(!dusted && clock::now() < timeout)
		std::this_thread::sleep_for(std::chrono::seconds(1));

	if(!dusted)
		log("ERROR: server failed to stop");

	if(con.valid())
		con.get();
}

} // rserver
} // ircbot
} // skivvy
