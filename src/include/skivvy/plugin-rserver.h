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

namespace skivvy { namespace ircbot {

/**
 *
 */
class RServerIrcBotPlugin
: public BasicIrcBotPlugin
{
	typedef int socket;
public:
	typedef long port;

private:
	socket ss; // server socket
	std::future<void> con;
	bool done;

	bool bind();//port p, const std::string& iface = "0.0.0.0");
	bool listen();//port p);
	void process(socket cs);

	void on(const message& msg);
	void off(const message& msg);

public:
	RServerIrcBotPlugin(IrcBot& bot);
	virtual ~RServerIrcBotPlugin();

	// INTERFACE: BasicIrcBotPlugin

	virtual bool initialize();

	// INTERFACE: IrcBotPlugin

	virtual std::string get_id() const;
	virtual std::string get_name() const;
	virtual std::string get_version() const;

	virtual void exit();
};

}} // sookee::ircbot

#endif // _SOOKEE_IRCBOT_RSERVER_H_
