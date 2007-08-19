/*       +------------------------------------+
 *       | UnrealIRCd v4.0                    |
 *       +------------------------------------+
 *
 * UnrealIRCd 4.0 (C) 2007 Carsten Valdemar Munk 
 * This program is free but copyrighted software; see
 *            the file COPYING for details.
 *
 * ---------------------------------------------------
 * Based off m_regonlycreate
 */

#include "inspircd.h"
#include "users.h"
#include "channels.h"
#include "modules.h"

/* $ModDesc: Prevents users that are non-opers for creating denied channels */

class ModuleAllowDenyChans : public Module
{
	ConfigReader *conf;
 public:
	ModuleAllowDenyChans(InspIRCd* Me)
		: Module(Me)
	{
		conf = new ConfigReader(ServerInstance);
	}

	void Implements(char* List)
	{
		List[I_OnUserPreJoin] = 1;
	}

	virtual int OnUserPreJoin(userrec* user, chanrec* chan, const char* cname, std::string &privs)
	{
		if (chan)
			return 0;

		if (IS_OPER(user))
			return 0;
		int count = conf->Enumerate("deny-channel");
		for (int i = 0; i < count; i++)
		{
			if (ServerInstance->MatchText(cname, conf->ReadValue("deny-channel", "mask", "", i)))
			{
				// Check for exceptions
				int acount = conf->Enumerate("allow-channel");
				for (int j = 0; j < acount; j++)
				{
					if (ServerInstance->MatchText(cname, conf->ReadValue("allow-channel", "mask", "", j)))
						return 0;
				}
				std::string reason = conf->ReadValue("deny-channel", "reason", "Channel is disallowed", i);
				user->WriteServ("NOTICE %s :*** Cannot create channel %s (%s)", user->nick, cname, reason.c_str());
				user->WriteServ("474 %s %s :Cannot join channel (%s)", user->nick, cname, reason.c_str());
				return 1;	
			}
		}
		return 0;
	}
	
	virtual ~ModuleAllowDenyChans()
	{
	}
	
	virtual Version GetVersion()
	{
		return Version(1, 1, 0, 0, VF_VENDOR, API_VERSION);
	}
};

MODULE_INIT(ModuleAllowDenyChans)
