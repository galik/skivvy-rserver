#pragma once
#ifndef SKIVVY_IRCBOT_RSERVER_H
#define SKIVVY_IRCBOT_RSERVER_H
/*
 * ircbot-rserver.h
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

#include <skivvy/ircbot.h>

#include <mutex>
#include <future>
#include <set>

#include <netinet/in.h>
//#include <sys/poll.h>
#include <hol/simple_logger.h>

#include <boost/asio.hpp>

namespace skivvy {
namespace ircbot {
namespace rserver {

using namespace boost;
using namespace hol::simple_logger;

struct in6_addr_cmp
{
	bool operator()(const in6_addr& ip1, const in6_addr& ip2) const
	{
		char* ip1d = (char*) &ip1;
		char* ip2d = (char*) &ip2;
		return std::lexicographical_compare(ip1d, ip1d + sizeof(ip1), ip2d, ip2d + sizeof(ip2));
	}
};

using ipv4addr_set = std::set<asio::ip::address_v4>;
using ipv6addr_set = std::set<asio::ip::address_v6>;

/**
 *
 */
class RServerIrcBotPlugin
: public BasicIrcBotPlugin
, public IrcBotMonitor
{
public:
	RServerIrcBotPlugin(IrcBot& bot);
	//virtual ~RServerIrcBotPlugin() {}

	// INTERFACE: BasicIrcBotPlugin

	bool initialize() override;

	// INTERFACE: IrcBotPlugin

	std::string get_id() const override;
	std::string get_name() const override;
	std::string get_version() const override;

	void exit() override;

	// INTERFACE: IrcBotMonitor

	void event(const message& msg) override;

private:
	void add_status_msg(const str& s)
	{
		if(status.size() < status_max)
			status.push_back(s);
		else
			LOG::W << "WARN: Dropping status message: " << s;
	}

//	void close() { if(ss.is_open()) ss.close(); }

//	bool bind();
	bool accept();
	void process(asio::ip::tcp::socket cs);

	void on(const message& msg);
	void off(const message& msg);

//	asio::io_service io_service;
//	asio::ip::tcp::acceptor acceptor;

	std::future<bool> con;
	std::atomic<bool> done;
	std::atomic<bool> dusted;

	ipv4addr_set ipv4accepts;
	ipv6addr_set ipv6accepts;

	// client OOB update info
	mutable std::mutex mtx;
	str_vec status;
	uns status_max = 128;
};

} // rserver
} // ircbot
} // skivvy

#endif // SKIVVY_IRCBOT_RSERVER_H
