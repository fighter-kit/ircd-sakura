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

/* $ModDesc: Enters ghost (0 users) channels into /LIST if official-channel config directive added */

class ModuleOfficialChannels : public Module
{
	ConfigReader *conf;
 public:
	ModuleOfficialChannels(InspIRCd* Me)
		: Module(Me)
	{
		conf = new ConfigReader(ServerInstance);
	}

	void Implements(char* List)
	{
		List[I_OnChannelList] = 1;
	}

	virtual void OnChannelList(userrec* user, const char** parameters, int pcnt, int minusers, int maxusers)
	{
		if (minusers > 0)
			return; // does not want official chans
		int acount = conf->Enumerate("official-channel");
		
		for (int i = 0; i < acount; i++)
		{
			std::string name = conf->ReadValue("official-channel", "name", "", i);
			std::string topic = conf->ReadValue("official-channel", "topic", "", i);
			
			if (name.size() == 0)
				continue;
			if (ServerInstance->FindChan(name))
				continue; // will appear later as it already exists
			if (pcnt)
				if (!ServerInstance->MatchText(name, parameters[0]))
					continue;
			user->WriteServ("322 %s %s %ld :[+] %s", user->nick, name.c_str(), 0, topic.c_str());
		}
	}
	
	virtual ~ModuleOfficialChannels()
	{
	}
	
	virtual Version GetVersion()
	{
		return Version(1, 1, 0, 0, VF_VENDOR, API_VERSION);
	}
};

MODULE_INIT(ModuleOfficialChannels)
