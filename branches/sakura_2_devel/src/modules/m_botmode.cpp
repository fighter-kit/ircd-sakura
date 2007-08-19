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
#include <stdio.h>
#include <string>
#include "users.h"
#include "channels.h"
#include "modules.h"
#include "configreader.h"

/* $ModDesc: Provides support for unreal-style umode +B (including BOTMOTD) */

static FileReader* botmotd;

CmdResult ShowBotMOTD(userrec* user)
{
	if(!botmotd->FileSize())
	{
		user->WriteServ(std::string("NOTICE ") + user->nick + std::string(" :BOTMOTD File not found"));
		return CMD_FAILURE;
	}

	user->WriteServ(std::string("NOTICE ") + user->nick + std::string(" :- ") + user->server + std::string(" Bot Message of the Day -"));

	for(int i=0; i != botmotd->FileSize(); i++)
	{
		user->WriteServ(std::string("NOTICE ") + user->nick + std::string(" :- ") + botmotd->GetLine(i));
	}

	user->WriteServ(std::string("NOTICE ") + user->nick + std::string(" :End of /BOTMOTD command."));

	/* don't route me */
	return CMD_LOCALONLY;
}

/** Handle /BOTMOTD
 */
class cmd_botmotd : public command_t
{
 public:
	cmd_botmotd (InspIRCd* Instance) : command_t(Instance,"BOTMOTD",0,0)
	{
		this->source = "m_botmode.so";
		syntax = "[<servername>]";
	}

	CmdResult Handle (const char** parameters, int pcnt, userrec* user)
	{
		return ShowBotMOTD(user);
	}
};

/** Handles user mode +B
 */
class BotMode : public ModeHandler
{
 public:
	BotMode(InspIRCd* Instance) : ModeHandler(Instance, 'B', 0, 0, false, MODETYPE_USER, false) { }

	ModeAction OnModeChange(userrec* source, userrec* dest, chanrec* channel, std::string &parameter, bool adding)
	{
		if (adding)
		{
			if (!dest->IsModeSet('B'))
			{
				dest->SetMode('B',true);
				ShowBotMOTD(dest);
				return MODEACTION_ALLOW;
			}
		}
		else
		{
			if (dest->IsModeSet('B'))
			{
				dest->SetMode('B',false);
				return MODEACTION_ALLOW;
			}
		}

		return MODEACTION_DENY;
	}
};

class ModuleBotMode : public Module
{
	cmd_botmotd* mycommand;	
	BotMode* bm;
 public:

	void LoadBotMOTD()
	{
		ConfigReader* conf = new ConfigReader(ServerInstance);
		std::string filename;
		filename = conf->ReadValue("files","botmotd",0);
		if (botmotd)
		{
			delete botmotd;
			botmotd = NULL;
		}
		botmotd = new FileReader(ServerInstance, filename);
		DELETE(conf);
	}

	ModuleBotMode(InspIRCd* Me)
		: Module(Me)
	{
		botmotd = NULL;
		mycommand = new cmd_botmotd(ServerInstance);
		ServerInstance->AddCommand(mycommand);
		botmotd = new FileReader(ServerInstance);
		LoadBotMOTD();		
		bm = new BotMode(ServerInstance);
		if (!ServerInstance->AddMode(bm, 'B'))
			throw ModuleException("Could not add new modes!");
	}

	virtual ~ModuleBotMode()
	{
		ServerInstance->Modes->DelMode(bm);
		DELETE(bm);
	}

	void Implements(char* List)
	{
		List[I_OnWhois] = List[I_OnRehash] = 1;
	}

	virtual Version GetVersion()
	{
		return Version(1,1,0,0,VF_COMMON|VF_VENDOR,API_VERSION);
	}

	virtual void OnWhois(userrec* src, userrec* dst)
	{
		if (dst->IsModeSet('B'))
		{
			ServerInstance->SendWhoisLine(src, dst, 335, std::string(src->nick)+" "+std::string(dst->nick)+" :is a Bot on "+ServerInstance->Config->Network);
		}
	}

	virtual void OnRehash(userrec* user, const std::string &parameter)
	{
		LoadBotMOTD();
	}
};

MODULE_INIT(ModuleBotMode)
