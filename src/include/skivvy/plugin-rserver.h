#pragma once
#ifndef _SOOKEE_IRCBOT_RSERVER_H_
#define _SOOKEE_IRCBOT_RSERVER_H_
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
#include <sys/poll.h>

namespace skivvy { namespace ircbot {

typedef std::set<in_addr_t> ipv4addr_set;

/**
 *
 */
class RServerIrcBotPlugin
: public BasicIrcBotPlugin
, public IrcBotMonitor
{
	typedef int socket;
public:
	typedef long port;

private:
	socket ss; // server socket
	std::future<void> con;
	std::atomic<bool> done;
	std::atomic<bool> dusted;

	struct pollfd ufd;

	ipv4addr_set accepts;

	// client OOB update info
	std::mutex mtx;
	str_vec status;
	uns status_max = 128;

	void add_status_msg(const str& s)
	{
		if(status.size() < status_max)
			status.push_back(s);
		else
			log("WARN: Dropping status message: " << s);
	}

	bool bind();
	bool listen();
	void process(socket cs);

	void on(const message& msg);
	void off(const message& msg);

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
};

}} // sookee::ircbot

#endif // _SOOKEE_IRCBOT_RSERVER_H_
