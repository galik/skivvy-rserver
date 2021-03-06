/*
 * File:   PKI.cpp
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 3, 2013, 7:40 AM
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2013 SooKee oasookee@gmail.com               |
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

#include "PKI.h"

#include <fstream>
#include <gcrypt.h>

#include <sookee/log.h>
#include <sookee/types/basic.h>
#include <sookee/types/stream.h>
//#include <katina/utils.h>

namespace skivvy { namespace pki {

using namespace katina::log;
using namespace katina::types;
using namespace katina::utils;

PKI::PKI()
: skey(0)
, pkey(0)
{
	if(!gcry_check_version(GCRYPT_VERSION))
		log("PKI: ERROR: version check failed: " << GCRYPT_VERSION);
	gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
}

PKI::~PKI()
{
	gcry_sexp_release(skey);
	gcry_sexp_release(pkey);
	for(key_map_iter i = pkeys.begin(); i != pkeys.end(); ++i)
		gcry_sexp_release(i->second);
}

bool PKI::generate_keypair(siz bits)
{
	gcry_sexp_t params = 0;

	soss oss;
	oss << bits;
	str sbits = oss.str();

	oss.str("");
	oss << "(genkey (rsa (nbits " << sbits.size() << ":" << sbits << ")))";

	//con("genkey param: " << oss.str() << "(" << bits << ")");

	if(gcry_error_t e = gcry_sexp_build(&params, 0, oss.str().c_str()))
	{
		log("PKI: ERROR: setting parameters: " << gcry_strerror(e));
		log("PKI:      : " << oss.str());
		return false;
	}

	gcry_sexp_t keypair;
	if(gcry_error_t e = gcry_pk_genkey(&keypair, params))
	{
		gcry_sexp_release(params);
		log("PKI: ERROR: generating keys: " << gcry_strerror(e));
		log("PKI:      : " << oss.str());
		return false;
	}

	// DEBUG
//	str s;
//	if(get_sexp_as_text(keypair, s))
//		con("XXX: " << s);

	if(!set_keys(keypair))
	{
		gcry_sexp_release(keypair);
		return false;
	}

	gcry_sexp_release(keypair);
	return true;
}

bool PKI::set_keys(const gcry_sexp_t keypair)
{
	gcry_sexp_t skey = 0;

	if(!(skey = gcry_sexp_find_token(keypair, "private-key", 0)))
	{
		log("PKI: ERROR: retrieving private key");
		return false;
	}

	gcry_sexp_t pkey = 0;

	if(!(pkey = gcry_sexp_find_token(keypair, "public-key", 0)))
	{
		gcry_sexp_release(skey);
		log("PKI: ERROR: retrieving public key");
		return false;
	}

	gcry_sexp_release(this->skey);
	this->skey = skey;

	gcry_sexp_release(this->pkey);
	this->pkey = pkey;

	return true;
}

bool PKI::load_keypair(const str& file)
{
	std::ifstream ifs(file.c_str());
	if(!ifs.is_open())
	{
		log("PKI: ERROR: opening file; " << file);
		return false;
	}
	return read_keypair(ifs);
}

bool PKI::read_keypair(std::istream& is)
{
	soss oss;
	if(!(oss << is.rdbuf()))
	{
		log("PKI: ERROR: reading keypair from stream");
		return false;
	}
	return read_keypair(oss.str());
}

bool PKI::read_keypair(const str& s)
{
	gcry_sexp_t keypair = 0;

	if(gcry_error_t e = gcry_sexp_sscan(&keypair, 0, s.c_str(), s.size()))
	{
		log("PKI: ERROR: parsing keypair data: " << gcry_strerror(e));
		log("PKI: " << s);
		return false;
	}

	if(!set_keys(keypair))
	{
		gcry_sexp_release(keypair);
		return false;
	}

	gcry_sexp_release(keypair);
	return true;
}

bool PKI::set_key_file(const str& file, key_t& key)
{
	std::ifstream ifs(file.c_str());
	if(!ifs.is_open())
	{
		log("PKI: ERROR: opening file; " << file);
		return false;
	}
	return set_key(ifs, key);
}

bool PKI::set_key(std::istream& is, key_t& key)
{
	soss oss;
	if(!(oss << is.rdbuf()))
	{
		log("PKI: ERROR: reading keypair from stream");
		return false;
	}
	return set_key(oss.str(), key);
}

bool PKI::set_key(const str& s, key_t& key)
{
	if(key)
		gcry_sexp_release(key);
	key = 0;

	if(gcry_error_t e = gcry_sexp_sscan(&key, 0, s.c_str(), s.size()))
	{
		log("PKI: ERROR: parsing public key data: " << gcry_strerror(e));
		log("PKI: " << s);
		return false;
	}

	return true;
}

//

bool PKI::add_client_key_file(const str& id, const str& file)
{
	std::ifstream ifs(file.c_str());
	if(!ifs.is_open())
	{
		log("PKI: ERROR: opening file; " << file);
		return false;
	}
	return add_client_key(id, ifs);
}

bool PKI::add_client_key(const str& id, std::istream& is)
{
	soss oss;
	if(!(oss << is.rdbuf()))
	{
		log("PKI: ERROR: reading keypair from stream");
		return false;
	}
	return add_client_key(id, oss.str());
}

bool PKI::add_client_key(const str& id, const str& s)
{
	gcry_sexp_t pkey = 0;

	if(gcry_error_t e = gcry_sexp_sscan(&pkey, 0, s.c_str(), s.size()))
	{
		log("PKI: ERROR: parsing public key data: " << gcry_strerror(e));
		log("PKI: " << s);
		return false;
	}

	pkeys[id] = pkey;

	return true;
}

// =======

bool PKI::get_sexp_as_text(const gcry_sexp_t& sexp, str& text) const
{
	siz len = 0;

	char* buff = new char[(len = gcry_sexp_sprint(sexp, GCRYSEXP_FMT_ADVANCED, 0, 0))];

	if(!buff)
	{
		log("PKI: ERROR: creating buffer");
		return false;
	}

	if(!(len = gcry_sexp_sprint(sexp, GCRYSEXP_FMT_ADVANCED, buff, len)))
	{
		log("PKI: ERROR: converting sexp to string");
		delete[] buff;
		return false;
	}

	text.assign(buff, len);
	delete[] buff;

	return true;
}

bool PKI::get_keypair_as_text(str& keypair) const
{
	gcry_sexp_t keys = 0;
	if(gcry_error_t e = gcry_sexp_build (&keys, 0, "(key-data\n  %S\n  %S\n )", pkey, skey))
	{
		log("PKI: ERROR: concatenating keys: " << gcry_strerror(e));
		return false;
	}

	if(!get_sexp_as_text(keys, keypair))
	{
		gcry_sexp_release(keys);
		return false;
	}

	gcry_sexp_release(keys);
//	key_t key = gcry_sexp_append(pkey, skey);

	return true;
}

bool PKI::get_public_key_as_text(str& text) const
{
	return get_sexp_as_text(pkey, text);
}

bool PKI::get_private_key_as_text(str& text) const
{
	return get_sexp_as_text(skey, text);
}

bool PKI::encrypt(const str& id, const str& text, str& code) const
{
	gcry_sexp_t data = 0;

	if(pkeys.find(id) == pkeys.end())
	{
		log("PKI: ERROR: Unknown id: " << id);
		return false;
	}

	if(gcry_error_t e = gcry_sexp_build(&data, 0, "%s", text.c_str()))
	{
		log("PKI: ERROR: converting text to sexp: " << gcry_strerror(e));
		return false;
	}

	gcry_sexp_t result = 0;

	if(gcry_error_t e = gcry_pk_encrypt(&result, data, pkeys.at(id)))
	{
		gcry_sexp_release(data);
		log("PKI: encrypt error: " << gcry_strerror(e));
		return false;
	}

	gcry_sexp_release(data);

	if(!get_sexp_as_text(result, code))
	{
		gcry_sexp_release(result);
		return false;
	}

	gcry_sexp_release(result);

	return true;
}

bool PKI::decrypt(const str& code, str& text) const
{
	gcry_sexp_t data = 0;

	// convert code into data

	if(gcry_error_t e = gcry_sexp_new(&data, code.c_str(), code.size(), 1))
	{
		log("PKI: ERROR: converting code to sexp: " << gcry_strerror(e));
		return false;
	}

	gcry_sexp_t result = 0;

	if(gcry_error_t e = gcry_pk_decrypt(&result, data, skey))
	{
		gcry_sexp_release(data);
		log("PKI: decrypt error: " << gcry_strerror(e));
		return false;
	}

	gcry_sexp_release(data);

	if(!get_sexp_as_text(result, text))
	{
		gcry_sexp_release(result);
		return false;
	}

	text.erase(0, text.find_first_not_of("\n\" "));
	text.erase(text.find_last_not_of("\n\" ") + 1);

	gcry_sexp_release(result);

	return true;
}

bool PKI::create_sexp_from_text(const str& exp, gcry_sexp_t& sexp) const
{
	gcry_sexp_t data = 0;

	if(gcry_error_t e = gcry_sexp_build(&data, 0, "%s", exp.c_str()))
	{
		log("PKI: ERROR: converting text to sexp: " << gcry_strerror(e));
		return false;
	}

	sexp = data;

	return true;
}

bool PKI::get_signature(str& signature) const
{
	return get_signature(pkey, signature);
}

bool PKI::get_signature(const str& text, str& signature) const
{
	gcry_sexp_t data = 0;

	if(!create_sexp_from_text(text, data))
		return false;

	if(!get_signature(data, signature))
	{
		gcry_sexp_release(data);
		return false;
	}

	gcry_sexp_release(data);

	return true;
}

bool PKI::get_signature(const gcry_sexp_t& sexp, str& signature) const
{
	gcry_sexp_t result = 0;

	if(gcry_error_t e = gcry_pk_sign(&result, sexp, skey))
	{
		log("PKI: signing error: " << gcry_strerror(e));
		return false;
	}

	if(!get_sexp_as_text(result, signature))
	{
		gcry_sexp_release(result);
		return false;
	}

	gcry_sexp_release(result);
	
	return true;
}

bool PKI::verify_signature(const str& id, const str& signature, bool& is_good) const
{
	if(pkeys.find(id) == pkeys.end())
	{
		log("PKI: ERROR: Unknown id: " << id);
		return false;
	}

	gcry_sexp_t sigval = 0;

	if(gcry_error_t e = gcry_sexp_sscan(&sigval, 0, signature.c_str(), signature.size()))
	{
		log("PKI: ERROR: parsing signature data: " << gcry_strerror(e));
		log("PKI: " << signature);
		return false;
	}

	is_good = gcry_pk_verify(sigval, pkeys.at(id), pkeys.at(id)) == 0;

	gcry_sexp_release(sigval);

	return true;
}

bool PKI::verify_signature(const str& id, const str& signature, const str& text, bool& is_good) const
{
	if(pkeys.find(id) == pkeys.end())
	{
		log("PKI: ERROR: Unknown id: " << id);
		return false;
	}

	gcry_sexp_t sigval = 0;

	if(gcry_error_t e = gcry_sexp_sscan(&sigval, 0, signature.c_str(), signature.size()))
	{
		log("PKI: ERROR: parsing signature data: " << gcry_strerror(e));
		log("PKI: " << signature);
		return false;
	}

	gcry_sexp_t data = 0;

	if(!create_sexp_from_text(text, data))
	{
		gcry_sexp_release(sigval);
		return false;
	}

	is_good = gcry_pk_verify(sigval, data, pkeys.at(id)) == 0;

	gcry_sexp_release(sigval);
	gcry_sexp_release(data);

	return true;
}

}} // skivvy::pki
