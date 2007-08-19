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

#ifdef WIN32
#include <sys/types.h>
#include <sys/timeb.h>
#endif

/* $ModDesc: Provides /RPING functionality */

/** Handle /RPING
 */
class cmd_rping : public command_t
{
 public:
	/* Command 'rping', needs operator and takes a parameter */
	cmd_rping (InspIRCd* Instance) : command_t(Instance,"RPING", 'o', 1)
	{
		this->source = "m_rping.so";
	}

	CmdResult Handle (const char** parameters, int pcnt, userrec *user)
	{
		std::string starttime = "<No client start time>";
		if (pcnt == 2)
			starttime = parameters[1];

		// Wire: ENCAP target RPING return-to current_usec current_usec :remark
		std::deque<std::string> params;
		params.push_back(parameters[0]);
		params.push_back("RPING");
		params.push_back(user->nick);
		char timebuf[18];
#ifndef WIN32
		struct timeval tv;
		gettimeofday(&tv, NULL);
		sprintf(timebuf, "%ld", 
			tv.tv_sec);
		params.push_back(timebuf);
		sprintf(timebuf, "%ld", 
			tv.tv_usec);
		params.push_back(timebuf);
#else
		struct _timeb tv;
		_ftime(&tv);
		sprintf(timebuf, "%ld",
			tv.time);
		params.push_back(timebuf);
		sprintf(timebuf, "%ld",
			tv.millitm * 1000);
		params.push_back(timebuf);
#endif
		params.push_back(":" + starttime);
		Event ev((char *) &params, NULL, "send_encap");
		ev.Send(ServerInstance);
		return CMD_LOCALONLY;
	}
};

class ModuleRping : public Module
{
	cmd_rping* newcommand;
 public:
	ModuleRping(InspIRCd* Me)
		: Module(Me)
	{
		// Create a new command
		newcommand = new cmd_rping(ServerInstance);
		ServerInstance->AddCommand(newcommand);
	}

	void Implements(char* List)
	{
		List[I_OnEncapReceived] = 1;
	}

	int OnEncapReceived(const std::string& from, std::deque<std::string>& parameters)
	{
		
		if (parameters[1] == "RPING")
		{
			// Wire: ENCAP target RPING return-to current_usec current_usec :remark

			if (parameters.size() < 6)
				return 0;
			std::deque<std::string> params;
			params.push_back(from);
			params.push_back("RPONG");
			params.push_back(parameters[2]);
			params.push_back(parameters[3]);
			params.push_back(parameters[4]);
			params.push_back(":" + parameters[5]);
			Event ev((char *) &params, NULL, "send_encap");
			ev.Send(ServerInstance);
			return 1;
		}
		/* :S2 ENCAP S1 RPONG target sec usec :remark */
		else if (parameters[1] == "RPONG")
		{
			if (parameters.size() < 6)
				return 0;
			userrec* target = ServerInstance->FindNick(parameters[2]);
			if (target == NULL)
				return 0;
			if (IS_LOCAL(target))
			{
				char timebuf[18];
#ifndef WIN32
				struct timeval tv;
				gettimeofday(&tv, NULL);
				sprintf(timebuf, "%ld", 
					(tv.tv_sec - atoi(parameters[3].c_str()))
					 * 1000 + (tv.tv_usec - atoi(parameters[4].c_str()))
					 / 1000);
#else
				struct _timeb tv;
				_ftime(&tv);
				sprintf(timebuf, "%ld",
					(tv.time - atoi(parameters[3].c_str())) * 
					1000 + (tv.millitm - 
					(atoi(parameters[4].c_str())/1000)));
#endif
				target->WriteServ("RPONG "
					+ parameters[2]
					+ " " + from + " " + std::string(timebuf) + " :" + parameters[5]);
				return 1;
			}
			else // fake direction?
				return 0;
		}
		else return 0;
	}

	
	virtual ~ModuleRping()
	{
	}

	
	virtual Version GetVersion()
	{
		return Version(1, 1, 0, 0, VF_VENDOR, API_VERSION);
	}
};

MODULE_INIT(ModuleRping)

