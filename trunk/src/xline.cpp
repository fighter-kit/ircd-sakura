/*       +------------------------------------+
 *       | Inspire Internet Relay Chat Daemon |
 *       +------------------------------------+
 *
 *  InspIRCd: (C) 2002-2007 InspIRCd Development Team
 * See: http://www.inspircd.org/wiki/index.php/Credits
 *
 * This program is free but copyrighted software; see
 *            the file COPYING for details.
 *
 * ---------------------------------------------------
 */

#include "inspircd.h"
#include "users.h"
#include "modules.h"
#include "wildcard.h"
#include "xline.h"

/* Version two, now with optimized expiry!
 *
 * Because the old way was horrendously slow, the new way of expiring xlines is very
 * very efficient. I have improved the efficiency of the algorithm in two ways:
 *
 * (1) There are now two lists of items for each linetype. One list holds temporary
 *     items, and the other list holds permanent items (ones which will expire).
 *     Items which are on the permanent list are NEVER checked at all by the
 *     expire_lines() function.
 * (2) The temporary xline lists are always kept in strict numerical order, keyed by
 *     current time + duration. This means that the line which is due to expire the
 *     soonest is always pointed at by vector::begin(), so a simple while loop can
 *     very efficiently, very quickly and above all SAFELY pick off the first few
 *     items in the vector which need zapping.
 *
 *     -- Brain
 */

bool InitXLine(ServerConfig* conf, const char* tag)
{
	return true;
}

bool DoneZLine(ServerConfig* conf, const char* tag)
{
	conf->GetInstance()->XLines->apply_lines(APPLY_ZLINES|APPLY_PERM_ONLY);
	return true;
}

bool DoneQLine(ServerConfig* conf, const char* tag)
{
	conf->GetInstance()->XLines->apply_lines(APPLY_QLINES|APPLY_PERM_ONLY);
	return true;
}

bool DoneKLine(ServerConfig* conf, const char* tag)
{
	conf->GetInstance()->XLines->apply_lines(APPLY_KLINES|APPLY_PERM_ONLY);
	return true;
}

bool DoneELine(ServerConfig* conf, const char* tag)
{
	/* Yes, this is supposed to do nothing, we dont 'apply' these */
	return true;
}

bool DoZLine(ServerConfig* conf, const char* tag, char** entries, ValueList &values, int* types)
{
	const char* reason = values[0].GetString();
	const char* ipmask = values[1].GetString();

	conf->GetInstance()->XLines->add_zline(0,"<Config>",reason,ipmask);
	return true;
}

bool DoQLine(ServerConfig* conf, const char* tag, char** entries, ValueList &values, int* types)
{
	const char* reason = values[0].GetString();
	const char* nick = values[1].GetString();

	conf->GetInstance()->XLines->add_qline(0,"<Config>",reason,nick);
	return true;
}

bool DoKLine(ServerConfig* conf, const char* tag, char** entries, ValueList &values, int* types)
{
	const char* reason = values[0].GetString();
	const char* host = values[1].GetString();

	conf->GetInstance()->XLines->add_kline(0,"<Config>",reason,host);
	return true;
}

bool DoELine(ServerConfig* conf, const char* tag, char** entries, ValueList &values, int* types)
{
	const char* reason = values[0].GetString();
	const char* host = values[1].GetString();

	conf->GetInstance()->XLines->add_eline(0,"<Config>",reason,host);
	return true;
}

IdentHostPair XLineManager::IdentSplit(const std::string &ident_and_host)
{
	IdentHostPair n = std::make_pair<std::string,std::string>("*","*");
	std::string::size_type x = ident_and_host.find('@');
	if (x != std::string::npos)
	{
		n.second = ident_and_host.substr(x + 1,ident_and_host.length());
		n.first = ident_and_host.substr(0, x);
		if (!n.first.length())
			n.first.assign("*");
		if (!n.second.length())
			n.second.assign("*");
	}
	else
	{
		n.second = ident_and_host;
	}

	return n;
}

// adds a g:line

bool XLineManager::add_gline(long duration, const char* source,const char* reason,const char* hostmask)
{
	IdentHostPair ih = IdentSplit(hostmask);

	if (del_gline(hostmask, true))
		return false;

	GLine* item = new GLine(ServerInstance->Time(), duration, source, reason, ih.first.c_str(), ih.second.c_str());

	if (duration)
	{
		glines.push_back(item);
		sort(glines.begin(), glines.end(),XLineManager::GSortComparison);
	}
	else
	{
		pglines.push_back(item);
	}

	return true;
}

// adds an e:line (exception to bans)

bool XLineManager::add_eline(long duration, const char* source, const char* reason, const char* hostmask)
{
	IdentHostPair ih = IdentSplit(hostmask);

	if (del_eline(hostmask, true))
		return false;

	ELine* item = new ELine(ServerInstance->Time(), duration, source, reason, ih.first.c_str(), ih.second.c_str());

	if (duration)
	{
		elines.push_back(item);
		sort(elines.begin(), elines.end(),XLineManager::ESortComparison);
	}
	else
	{
		pelines.push_back(item);
	}
	return true;
}

// adds a q:line

bool XLineManager::add_qline(long duration, const char* source, const char* reason, const char* nickname)
{
	if (del_qline(nickname, true))
		return false;

	QLine* item = new QLine(ServerInstance->Time(), duration, source, reason, nickname);

	if (duration)
	{
		qlines.push_back(item);
		sort(qlines.begin(), qlines.end(),XLineManager::QSortComparison);
	}
	else
	{
		pqlines.push_back(item);
	}
	return true;
}

// adds a z:line

bool XLineManager::add_zline(long duration, const char* source, const char* reason, const char* ipaddr)
{
	if (strchr(ipaddr,'@'))
	{
		while (*ipaddr != '@')
			ipaddr++;
		ipaddr++;
	}

	if (del_zline(ipaddr, true))
		return false;

	ZLine* item = new ZLine(ServerInstance->Time(), duration, source, reason, ipaddr);

	if (duration)
	{
		zlines.push_back(item);
		sort(zlines.begin(), zlines.end(),XLineManager::ZSortComparison);
	}
	else
	{
		pzlines.push_back(item);
	}
	return true;
}

// adds a k:line

bool XLineManager::add_kline(long duration, const char* source, const char* reason, const char* hostmask)
{
	IdentHostPair ih = IdentSplit(hostmask);

	if (del_kline(hostmask, true))
		return false;

	KLine* item = new KLine(ServerInstance->Time(), duration, source, reason, ih.first.c_str(), ih.second.c_str());

	if (duration)
	{
		klines.push_back(item);
		sort(klines.begin(), klines.end(),XLineManager::KSortComparison);
	}
	else
	{
		pklines.push_back(item);
	}
	return true;
}

// deletes a g:line, returns true if the line existed and was removed

bool XLineManager::del_gline(const char* hostmask, bool simulate)
{
	IdentHostPair ih = IdentSplit(hostmask);
	for (std::vector<GLine*>::iterator i = glines.begin(); i != glines.end(); i++)
	{
		if (!strcasecmp(ih.first.c_str(),(*i)->identmask) && !strcasecmp(ih.second.c_str(),(*i)->hostmask))
		{
			if (!simulate)
			{
				delete *i;
				glines.erase(i);
			}
			return true;
		}
	}
	for (std::vector<GLine*>::iterator i = pglines.begin(); i != pglines.end(); i++)
	{
		if (!strcasecmp(ih.first.c_str(),(*i)->identmask) && !strcasecmp(ih.second.c_str(),(*i)->hostmask))
		{
			if (!simulate)
			{
				delete *i;
				pglines.erase(i);
			}
			return true;
		}
	}
	return false;
}

// deletes a e:line, returns true if the line existed and was removed

bool XLineManager::del_eline(const char* hostmask, bool simulate)
{
	IdentHostPair ih = IdentSplit(hostmask);
	for (std::vector<ELine*>::iterator i = elines.begin(); i != elines.end(); i++)
	{
		if (!strcasecmp(ih.first.c_str(),(*i)->identmask) && !strcasecmp(ih.second.c_str(),(*i)->hostmask))
		{
			if (!simulate)
			{
				delete *i;
				elines.erase(i);
			}
			return true;
		}
	}
	for (std::vector<ELine*>::iterator i = pelines.begin(); i != pelines.end(); i++)
	{
		if (!strcasecmp(ih.first.c_str(),(*i)->identmask) && !strcasecmp(ih.second.c_str(),(*i)->hostmask))
		{
			if (!simulate)
			{
				delete *i;
				pelines.erase(i);
			}
			return true;
		}
	}
	return false;
}

// deletes a q:line, returns true if the line existed and was removed

bool XLineManager::del_qline(const char* nickname, bool simulate)
{
	for (std::vector<QLine*>::iterator i = qlines.begin(); i != qlines.end(); i++)
	{
		if (!strcasecmp(nickname,(*i)->nick))
		{
			if (!simulate)
			{
				delete *i;
				qlines.erase(i);
			}
			return true;
		}
	}
	for (std::vector<QLine*>::iterator i = pqlines.begin(); i != pqlines.end(); i++)
	{
		if (!strcasecmp(nickname,(*i)->nick))
		{
			if (!simulate)
			{
				delete *i;
				pqlines.erase(i);
			}
			return true;
		}
	}
	return false;
}

// deletes a z:line, returns true if the line existed and was removed

bool XLineManager::del_zline(const char* ipaddr, bool simulate)
{
	for (std::vector<ZLine*>::iterator i = zlines.begin(); i != zlines.end(); i++)
	{
		if (!strcasecmp(ipaddr,(*i)->ipaddr))
		{
			if (!simulate)
			{
				delete *i;
				zlines.erase(i);
			}
			return true;
		}
	}
	for (std::vector<ZLine*>::iterator i = pzlines.begin(); i != pzlines.end(); i++)
	{
		if (!strcasecmp(ipaddr,(*i)->ipaddr))
		{
			if (!simulate)
			{
				delete *i;
				pzlines.erase(i);
			}
			return true;
		}
	}
	return false;
}

// deletes a k:line, returns true if the line existed and was removed

bool XLineManager::del_kline(const char* hostmask, bool simulate)
{
	IdentHostPair ih = IdentSplit(hostmask);
	for (std::vector<KLine*>::iterator i = klines.begin(); i != klines.end(); i++)
	{
		if (!strcasecmp(ih.first.c_str(),(*i)->identmask) && !strcasecmp(ih.second.c_str(),(*i)->hostmask))
		{
			if (!simulate)
			{
				delete *i;
				klines.erase(i);
			}
			return true;
		}
	}
	for (std::vector<KLine*>::iterator i = pklines.begin(); i != pklines.end(); i++)
	{
		if (!strcasecmp(ih.first.c_str(),(*i)->identmask) && !strcasecmp(ih.second.c_str(),(*i)->hostmask))
		{
			if (!simulate)
			{
				delete *i;
				pklines.erase(i);
			}
			return true;
		}
	}
	return false;
}

// returns a pointer to the reason if a nickname matches a qline, NULL if it didnt match

QLine* XLineManager::matches_qline(const char* nick, bool permonly)
{
	if ((qlines.empty()) && (pqlines.empty()))
		return NULL;
	if (!permonly)
	{
		for (std::vector<QLine*>::iterator i = qlines.begin(); i != qlines.end(); i++)
			if (match(nick,(*i)->nick))
				return (*i);
	}
	for (std::vector<QLine*>::iterator i = pqlines.begin(); i != pqlines.end(); i++)
		if (match(nick,(*i)->nick))
			return (*i);
	return NULL;
}

// returns a pointer to the reason if a host matches a gline, NULL if it didnt match

GLine* XLineManager::matches_gline(userrec* user, bool permonly)
{
	if ((glines.empty()) && (pglines.empty()))
		return NULL;
	if (!permonly)
	{
		for (std::vector<GLine*>::iterator i = glines.begin(); i != glines.end(); i++)
		{
			if ((match(user->ident,(*i)->identmask)))
			{
				if ((match(user->host,(*i)->hostmask, true)) || (match(user->GetIPString(),(*i)->hostmask, true)))
				{
					return (*i);
				}
			}
		}
	}
	for (std::vector<GLine*>::iterator i = pglines.begin(); i != pglines.end(); i++)
	{
		if ((match(user->ident,(*i)->identmask)))
		{
			if ((match(user->host,(*i)->hostmask, true)) || (match(user->GetIPString(),(*i)->hostmask, true)))
			{
				return (*i);
			}
		}
	}
	return NULL;
}

ELine* XLineManager::matches_exception(userrec* user, bool permonly)
{
	if ((elines.empty()) && (pelines.empty()))
		return NULL;
	char host2[MAXBUF];
	snprintf(host2,MAXBUF,"*@%s",user->host);
	if (!permonly)
	{
		for (std::vector<ELine*>::iterator i = elines.begin(); i != elines.end(); i++)
		{
			if ((match(user->ident,(*i)->identmask)))
			{
				if ((match(user->host,(*i)->hostmask, true)) || (match(user->GetIPString(),(*i)->hostmask, true)))
				{
					return (*i);
				}
			}
		}
	}
	for (std::vector<ELine*>::iterator i = pelines.begin(); i != pelines.end(); i++)
	{
		if ((match(user->ident,(*i)->identmask)))
		{
			if ((match(user->host,(*i)->hostmask, true)) || (match(user->GetIPString(),(*i)->hostmask, true)))
			{
				return (*i);
			}
		}
	}
	return NULL;
}


void XLineManager::gline_set_creation_time(const char* host, time_t create_time)
{
	for (std::vector<GLine*>::iterator i = glines.begin(); i != glines.end(); i++)
	{
		if (!strcasecmp(host,(*i)->hostmask))
		{
			(*i)->set_time = create_time;
			(*i)->expiry = create_time + (*i)->duration;
			return;
		}
	}
	for (std::vector<GLine*>::iterator i = pglines.begin(); i != pglines.end(); i++)
	{
		if (!strcasecmp(host,(*i)->hostmask))
		{
			(*i)->set_time = create_time;
			return;
		}
	}
	return ;
}

void XLineManager::eline_set_creation_time(const char* host, time_t create_time)
{
	for (std::vector<ELine*>::iterator i = elines.begin(); i != elines.end(); i++)
	{
		if (!strcasecmp(host,(*i)->hostmask))
		{
			(*i)->set_time = create_time;
			(*i)->expiry = create_time + (*i)->duration;
			return;
		}
	}
	for (std::vector<ELine*>::iterator i = pelines.begin(); i != pelines.end(); i++)
	{
		if (!strcasecmp(host,(*i)->hostmask))
		{
			(*i)->set_time = create_time;
			return;
		}
	}
	return;
}

void XLineManager::qline_set_creation_time(const char* nick, time_t create_time)
{
	for (std::vector<QLine*>::iterator i = qlines.begin(); i != qlines.end(); i++)
	{
		if (!strcasecmp(nick,(*i)->nick))
		{
			(*i)->set_time = create_time;
			(*i)->expiry = create_time + (*i)->duration;
			return;
		}
	}
	for (std::vector<QLine*>::iterator i = pqlines.begin(); i != pqlines.end(); i++)
	{
		if (!strcasecmp(nick,(*i)->nick))
		{
			(*i)->set_time = create_time;
			return;
		}
	}
	return;
}

void XLineManager::zline_set_creation_time(const char* ip, time_t create_time)
{
	for (std::vector<ZLine*>::iterator i = zlines.begin(); i != zlines.end(); i++)
	{
		if (!strcasecmp(ip,(*i)->ipaddr))
		{
			(*i)->set_time = create_time;
			(*i)->expiry = create_time + (*i)->duration;
			return;
		}
	}
	for (std::vector<ZLine*>::iterator i = pzlines.begin(); i != pzlines.end(); i++)
	{
		if (!strcasecmp(ip,(*i)->ipaddr))
		{
			(*i)->set_time = create_time;
			return;
		}
	}
	return;
}

// returns a pointer to the reason if an ip address matches a zline, NULL if it didnt match

ZLine* XLineManager::matches_zline(const char* ipaddr, bool permonly)
{
	if ((zlines.empty()) && (pzlines.empty()))
		return NULL;
	if (!permonly)
	{
		for (std::vector<ZLine*>::iterator i = zlines.begin(); i != zlines.end(); i++)
			if (match(ipaddr,(*i)->ipaddr, true))
				return (*i);
	}
	for (std::vector<ZLine*>::iterator i = pzlines.begin(); i != pzlines.end(); i++)
		if (match(ipaddr,(*i)->ipaddr, true))
			return (*i);
	return NULL;
}

// returns a pointer to the reason if a host matches a kline, NULL if it didnt match

KLine* XLineManager::matches_kline(userrec* user, bool permonly)
{
	if ((klines.empty()) && (pklines.empty()))
		return NULL;
	if (!permonly)
	{
		for (std::vector<KLine*>::iterator i = klines.begin(); i != klines.end(); i++)
		{
			if ((match(user->ident,(*i)->identmask)))
			{
				if ((match(user->host,(*i)->hostmask, true)) || (match(user->GetIPString(),(*i)->hostmask, true)))
				{
					return (*i);
				}
			}
		}
	}
	for (std::vector<KLine*>::iterator i = pklines.begin(); i != pklines.end(); i++)
	{
		if ((match(user->ident,(*i)->identmask)))
		{
			if ((match(user->host,(*i)->hostmask, true)) || (match(user->GetIPString(),(*i)->hostmask, true)))
			{
				return (*i);
			}
		}
	}
	return NULL;
}

bool XLineManager::GSortComparison ( const GLine* one, const GLine* two )
{
	return (one->expiry) < (two->expiry);
}

bool XLineManager::ESortComparison ( const ELine* one, const ELine* two )
{
	return (one->expiry) < (two->expiry);
}

bool XLineManager::ZSortComparison ( const ZLine* one, const ZLine* two )
{
	return (one->expiry) < (two->expiry);
}

bool XLineManager::KSortComparison ( const KLine* one, const KLine* two )
{
	return (one->expiry) < (two->expiry);
}

bool XLineManager::QSortComparison ( const QLine* one, const QLine* two )
{
	return (one->expiry) < (two->expiry);
}

// removes lines that have expired

void XLineManager::expire_lines()
{
	time_t current = ServerInstance->Time();

	/* Because we now store all our XLines in sorted order using ((*i)->duration + (*i)->set_time) as a key, this
	 * means that to expire the XLines we just need to do a while, picking off the top few until there are
	 * none left at the head of the queue that are after the current time.
	 */

	while ((glines.size()) && (current > (*glines.begin())->expiry))
	{
		std::vector<GLine*>::iterator i = glines.begin();
		ServerInstance->SNO->WriteToSnoMask('x',"Expiring timed G-Line %s@%s (set by %s %d seconds ago)",(*i)->identmask,(*i)->hostmask,(*i)->source,(*i)->duration);
		glines.erase(i);
	}

	while ((elines.size()) && (current > (*elines.begin())->expiry))
	{
		std::vector<ELine*>::iterator i = elines.begin();
		ServerInstance->SNO->WriteToSnoMask('x',"Expiring timed E-Line %s@%s (set by %s %d seconds ago)",(*i)->identmask,(*i)->hostmask,(*i)->source,(*i)->duration);
		elines.erase(i);
	}

	while ((zlines.size()) && (current > (*zlines.begin())->expiry))
	{
		std::vector<ZLine*>::iterator i = zlines.begin();
		ServerInstance->SNO->WriteToSnoMask('x',"Expiring timed Z-Line %s (set by %s %d seconds ago)",(*i)->ipaddr,(*i)->source,(*i)->duration);
		zlines.erase(i);
	}

	while ((klines.size()) && (current > (*klines.begin())->expiry))
	{
		std::vector<KLine*>::iterator i = klines.begin();
		ServerInstance->SNO->WriteToSnoMask('x',"Expiring timed K-Line %s@%s (set by %s %d seconds ago)",(*i)->identmask,(*i)->hostmask,(*i)->source,(*i)->duration);
		klines.erase(i);
	}

	while ((qlines.size()) && (current > (*qlines.begin())->expiry))
	{
		std::vector<QLine*>::iterator i = qlines.begin();
		ServerInstance->SNO->WriteToSnoMask('x',"Expiring timed Q-Line %s (set by %s %d seconds ago)",(*i)->nick,(*i)->source,(*i)->duration);
		qlines.erase(i);
	}

}

// applies lines, removing clients and changing nicks etc as applicable

void XLineManager::apply_lines(const int What)
{
	if (!What)
		return;

	if (What & APPLY_PERM_ONLY)
	{
		char reason[MAXBUF];

		if ((!pglines.size()) && (!pklines.size()) && (!pzlines.size()) && (!pqlines.size()))
			return;

		XLine* check = NULL;
		for (std::vector<userrec*>::const_iterator u2 = ServerInstance->local_users.begin(); u2 != ServerInstance->local_users.end(); u2++)
		{
			userrec* u = (userrec*)(*u2);

			if (elines.size() || pelines.size())
				if (matches_exception(u))
					continue;

			if ((What & APPLY_GLINES) && pglines.size())
			{
				if ((check = matches_gline(u,true)))
				{
					snprintf(reason,MAXBUF,"G-Lined: %s",check->reason);
					if (*ServerInstance->Config->MoronBanner)
						u->WriteServ("NOTICE %s :*** %s", u->nick, ServerInstance->Config->MoronBanner);
					if (ServerInstance->Config->HideBans)
						userrec::QuitUser(ServerInstance, u, "G-Lined", reason);
					else
						userrec::QuitUser(ServerInstance, u, reason);
				}
			}

			if ((What & APPLY_KLINES) && pklines.size())
			{
				if ((check = matches_kline(u,true)))
				{
					snprintf(reason,MAXBUF,"K-Lined: %s",check->reason);
					if (*ServerInstance->Config->MoronBanner)
						u->WriteServ("NOTICE %s :*** %s", u->nick, ServerInstance->Config->MoronBanner);
					if (ServerInstance->Config->HideBans)
						userrec::QuitUser(ServerInstance, u, "K-Lined", reason);
					else
						userrec::QuitUser(ServerInstance, u, reason);
				}
			}

			if ((What & APPLY_QLINES) && pqlines.size())
			{
				if ((check = matches_qline(u->nick,true)))
				{
					snprintf(reason,MAXBUF,"Q-Lined: %s",check->reason);
					if (*ServerInstance->Config->MoronBanner)
						u->WriteServ("NOTICE %s :*** %s", u->nick, ServerInstance->Config->MoronBanner);
					if (ServerInstance->Config->HideBans)
						userrec::QuitUser(ServerInstance, u, "Q-Lined", reason);
					else
						userrec::QuitUser(ServerInstance, u, reason);
				}
			}

			if ((What & APPLY_ZLINES) && pzlines.size())
			{
				if ((check = matches_zline(u->GetIPString(),true)))
				{
					snprintf(reason,MAXBUF,"Z-Lined: %s",check->reason);
					if (*ServerInstance->Config->MoronBanner)
						u->WriteServ("NOTICE %s :*** %s", u->nick, ServerInstance->Config->MoronBanner);
					if (ServerInstance->Config->HideBans)
						userrec::QuitUser(ServerInstance, u, "Z-Lined", reason);
					else
						userrec::QuitUser(ServerInstance, u, reason);
				}
			}
		}
	}
	else
	{
		char reason[MAXBUF];

		if ((!glines.size()) && (!klines.size()) && (!zlines.size()) && (!qlines.size()) &&
		(!pglines.size()) && (!pklines.size()) && (!pzlines.size()) && (!pqlines.size()))
			return;

		XLine* check = NULL;
		for (std::vector<userrec*>::const_iterator u2 = ServerInstance->local_users.begin(); u2 != ServerInstance->local_users.end(); u2++)
		{
			userrec* u = (userrec*)(*u2);

			if (elines.size() || pelines.size())
			{
				// ignore people matching exempts
				if (matches_exception(u))
					continue;
			}
			if ((What & APPLY_GLINES) && (glines.size() || pglines.size()))
			{
				if ((check = matches_gline(u)))
				{
					snprintf(reason,MAXBUF,"G-Lined: %s",check->reason);
					if (*ServerInstance->Config->MoronBanner)
						u->WriteServ("NOTICE %s :*** %s", u->nick, ServerInstance->Config->MoronBanner);
					if (ServerInstance->Config->HideBans)
						userrec::QuitUser(ServerInstance, u, "G-Lined", reason);
					else
						userrec::QuitUser(ServerInstance, u, reason);
				}
			}
			if ((What & APPLY_KLINES) && (klines.size() || pklines.size()))
			{
				if ((check = matches_kline(u)))
				{
					snprintf(reason,MAXBUF,"K-Lined: %s",check->reason);
					if (*ServerInstance->Config->MoronBanner)
						u->WriteServ("NOTICE %s :*** %s", u->nick, ServerInstance->Config->MoronBanner);
					if (ServerInstance->Config->HideBans)
						userrec::QuitUser(ServerInstance, u, "K-Lined", reason);
					else
						userrec::QuitUser(ServerInstance, u, reason);
				}
			}
			if ((What & APPLY_QLINES) && (qlines.size() || pqlines.size()))
			{
				if ((check = matches_qline(u->nick)))
				{
					snprintf(reason,MAXBUF,"Q-Lined: %s",check->reason);
					if (*ServerInstance->Config->MoronBanner)
						u->WriteServ("NOTICE %s :*** %s", u->nick, ServerInstance->Config->MoronBanner);
					if (ServerInstance->Config->HideBans)
						userrec::QuitUser(ServerInstance, u, "Q-Lined", reason);
					else
						userrec::QuitUser(ServerInstance, u, reason);
				}
			}
			if ((What & APPLY_ZLINES) && (zlines.size() || pzlines.size()))
			{
				if ((check = matches_zline(u->GetIPString())))
				{
					snprintf(reason,MAXBUF,"Z-Lined: %s", check->reason);
					if (*ServerInstance->Config->MoronBanner)
						u->WriteServ("NOTICE %s :*** %s", u->nick, ServerInstance->Config->MoronBanner);
					if (ServerInstance->Config->HideBans)
						userrec::QuitUser(ServerInstance, u, "Z-Lined", reason);
					else
						userrec::QuitUser(ServerInstance, u, reason);
				}
			}
		}
	}
}

void XLineManager::stats_k(userrec* user, string_list &results)
{
	std::string sn = ServerInstance->Config->ServerName;
	for (std::vector<KLine*>::iterator i = klines.begin(); i != klines.end(); i++)
		results.push_back(sn+" 216 "+user->nick+" :"+(*i)->identmask+"@"+(*i)->hostmask+" "+ConvToStr((*i)->set_time)+" "+ConvToStr((*i)->duration)+" "+(*i)->source+" :"+(*i)->reason);
	for (std::vector<KLine*>::iterator i = pklines.begin(); i != pklines.end(); i++)
		results.push_back(sn+" 216 "+user->nick+" :"+(*i)->identmask+"@"+(*i)->hostmask+" "+ConvToStr((*i)->set_time)+" "+ConvToStr((*i)->duration)+" "+(*i)->source+" :"+(*i)->reason);
}

void XLineManager::stats_g(userrec* user, string_list &results)
{
	std::string sn = ServerInstance->Config->ServerName;
	for (std::vector<GLine*>::iterator i = glines.begin(); i != glines.end(); i++)
		results.push_back(sn+" 223 "+user->nick+" :"+(*i)->identmask+"@"+(*i)->hostmask+" "+ConvToStr((*i)->set_time)+" "+ConvToStr((*i)->duration)+" "+(*i)->source+" :"+(*i)->reason);
	for (std::vector<GLine*>::iterator i = pglines.begin(); i != pglines.end(); i++)
		results.push_back(sn+" 223 "+user->nick+" :"+(*i)->identmask+"@"+(*i)->hostmask+" "+ConvToStr((*i)->set_time)+" "+ConvToStr((*i)->duration)+" "+(*i)->source+" :"+(*i)->reason);
}

void XLineManager::stats_q(userrec* user, string_list &results)
{
	std::string sn = ServerInstance->Config->ServerName;
	for (std::vector<QLine*>::iterator i = qlines.begin(); i != qlines.end(); i++)
		results.push_back(sn+" 217 "+user->nick+" :"+(*i)->nick+" "+ConvToStr((*i)->set_time)+" "+ConvToStr((*i)->duration)+" "+(*i)->source+" :"+(*i)->reason);
	for (std::vector<QLine*>::iterator i = pqlines.begin(); i != pqlines.end(); i++)
		results.push_back(sn+" 217 "+user->nick+" :"+(*i)->nick+" "+ConvToStr((*i)->set_time)+" "+ConvToStr((*i)->duration)+" "+(*i)->source+" :"+(*i)->reason);
}

void XLineManager::stats_z(userrec* user, string_list &results)
{
	std::string sn = ServerInstance->Config->ServerName;
	for (std::vector<ZLine*>::iterator i = zlines.begin(); i != zlines.end(); i++)
		results.push_back(sn+" 223 "+user->nick+" :"+(*i)->ipaddr+" "+ConvToStr((*i)->set_time)+" "+ConvToStr((*i)->duration)+" "+(*i)->source+" :"+(*i)->reason);
	for (std::vector<ZLine*>::iterator i = pzlines.begin(); i != pzlines.end(); i++)
		results.push_back(sn+" 223 "+user->nick+" :"+(*i)->ipaddr+" "+ConvToStr((*i)->set_time)+" "+ConvToStr((*i)->duration)+" "+(*i)->source+" :"+(*i)->reason);
}

void XLineManager::stats_e(userrec* user, string_list &results)
{
	std::string sn = ServerInstance->Config->ServerName;
	for (std::vector<ELine*>::iterator i = elines.begin(); i != elines.end(); i++)
		results.push_back(sn+" 223 "+user->nick+" :"+(*i)->identmask+"@"+(*i)->hostmask+" "+ConvToStr((*i)->set_time)+" "+ConvToStr((*i)->duration)+" "+(*i)->source+" :"+(*i)->reason);
	for (std::vector<ELine*>::iterator i = pelines.begin(); i != pelines.end(); i++)
		results.push_back(sn+" 223 "+user->nick+" :"+(*i)->identmask+"@"+(*i)->hostmask+" "+ConvToStr((*i)->set_time)+" "+ConvToStr((*i)->duration)+" "+(*i)->source+" :"+(*i)->reason);
}

XLineManager::XLineManager(InspIRCd* Instance) : ServerInstance(Instance)
{
}
