/*       +------------------------------------+
 *       | UnrealIRCd v4.0                    |
 *       +------------------------------------+
 *
 * UnrealIRCd 4.0 (C) 2007 Carsten Valdemar Munk 
 * This program is free but copyrighted software; see
 *            the file COPYING for details.
 *
 * ---------------------------------------------------
 */

#include "inspircd.h"
#include "users.h"
#include "channels.h"
#include "modules.h"

/* $ModDesc: Provides /CLOSE functionality */

/** Handle /CLOSE
 */
class cmd_close : public command_t
{
 public:
	/* Command 'close', needs operator */
	cmd_close (InspIRCd* Instance) : command_t(Instance,"CLOSE", 'o', 0)
	{
		this->source = "m_close.so";
	}

	CmdResult Handle (const char** parameters, int pcnt, userrec *user)
	{
		int num = 0;
		for (std::vector<userrec*>::const_iterator u = ServerInstance->local_users.begin();
			u != ServerInstance->local_users.end(); u++)
		{
			if ((*u)->registered != REG_ALL)
			{
				userrec::QuitUser(ServerInstance, *u, "Closing all unknown connections per request");
				user->WriteServ("NOTICE %s :*** Closing unknown connection [%s.%i]",
					user->nick,
					(*u)->GetIPString(), (*u)->GetPort());
				num++;
			}
		}
		user->WriteServ("NOTICE %s :*** %i unknown connection(s) closed", user->nick, num);
		return CMD_LOCALONLY;
	}
};

class ModuleClose : public Module
{
	cmd_close* newcommand;
 public:
	ModuleClose(InspIRCd* Me)
		: Module(Me)
	{
		// Create a new command
		newcommand = new cmd_close(ServerInstance);
		ServerInstance->AddCommand(newcommand);
	}

	void Implements(char* List)
	{
	}


	virtual ~ModuleClose()
	{
	}

	
	virtual Version GetVersion()
	{
		return Version(1, 1, 0, 0, VF_VENDOR, API_VERSION);
	}
};

MODULE_INIT(ModuleClose)

