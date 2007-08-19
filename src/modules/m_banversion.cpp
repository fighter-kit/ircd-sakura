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
#include "channels.h"
#include "modules.h"
#include "xline.h"

/* $ModDesc: Sends a numeric on connect which cripples a common type of trojan/spambot */

class ModuleBanVersion : public Module
{
 private:
 	ConfigReader *conf;
 public:
	ModuleBanVersion(InspIRCd* Me) : Module(Me)
	{
		conf = new ConfigReader(ServerInstance);	
	}
	
	virtual ~ModuleBanVersion()
	{
	}
	
	virtual Version GetVersion()
	{
		return Version(1,1,0,0,VF_VENDOR,API_VERSION);
	}

	void Implements(char* List)
	{
		List[I_OnUserRegister] = List[I_OnPreCommand] = 1;
	}

	virtual int OnPreCommand(const std::string &command, const char** parameters, int pcnt, userrec *user, bool validated, const std::string &original_line)
	{
		if (command == "NOTICE" && !validated && pcnt > 1 && user->GetExt("banversion_check"))
		{
			int acount = conf->Enumerate("ban-version");
			if (!strncmp(parameters[1], "\1VERSION ", 9))
			{	
				for (int i = 0; i < acount; i++)
				{
					if (ServerInstance->MatchText(parameters[1] + 9,
							conf->ReadValue("ban-version", "mask", "", i)))
					{
						std::string reason = conf->ReadValue("ban-version", "reason", "Your version is disallowed", i);
						if (ServerInstance->XLines->add_zline(3600, ServerInstance->Config->ServerName, reason.c_str(), user->MakeHostIP()))
						{
							ServerInstance->XLines->apply_lines(APPLY_ZLINES);
							FOREACH_MOD(I_OnAddGLine,OnAddZLine(3600, NULL, reason, user->MakeHostIP()));
							return 1;
						}	
					}
				}
			}
			
			user->Shrink("banversion_check");
			// Block the command, so the user doesn't receive a no such nick notice
			return 1;
		}
		
		return 0;
	}

	virtual int OnUserRegister(userrec* user)
	{
		user->WriteServ("PRIVMSG %s :\1VERSION\1", user->nick);
		user->Extend("banversion_check");
		return 0;
	}
};

MODULE_INIT(ModuleBanVersion)
