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
#include "commands/cmd_loadmodule.h"
#ifndef WIN32
#include <dirent.h>
#endif
extern "C" DllExport command_t* init_command(InspIRCd* Instance)
{
	return new cmd_loadmodule(Instance);
}

/** Handle /LOADMODULE
 */
CmdResult cmd_loadmodule::Handle (const char** parameters, int pcnt, userrec *user)
{
	/* Do we have a glob pattern in the filename?
	 * The user wants to load multiple modules which
	 * match the pattern.
	 */
	if (strchr(parameters[0],'*') || (strchr(parameters[0],'?')))
	{
		int n_match = 0;
		DIR* library = opendir(ServerInstance->Config->ModPath);
		if (library)
		{
			/* Try and locate and load all modules matching the pattern */
			dirent* entry = NULL;
			while ((entry = readdir(library)))
			{
				if (ServerInstance->MatchText(entry->d_name, parameters[0]))
				{
					if (ServerInstance->LoadModule(entry->d_name))
					{
						ServerInstance->WriteOpers("*** NEW MODULE: %s loaded %s",user->nick, entry->d_name);
						user->WriteServ("975 %s %s :Module successfully loaded.",user->nick, entry->d_name);
					}
					else 
					{
						user->WriteServ("974 %s %s :Failed to load module: %s",user->nick, entry->d_name, ServerInstance->ModuleError());
						n_match++;
					}
				}
			}
			closedir(library);
		}
		/* Loadmodule will now return false if any one of the modules failed
		 * to load (but wont abort when it encounters a bad one) and when 1 or
		 * more modules were actually loaded.
		 */
		return (n_match > 0 ? CMD_SUCCESS : CMD_FAILURE);
	}
	else
	{
		if (ServerInstance->LoadModule(parameters[0]))
		{
			ServerInstance->WriteOpers("*** NEW MODULE: %s loaded %s",user->nick, parameters[0]);
			user->WriteServ("975 %s %s :Module successfully loaded.",user->nick, parameters[0]);
			return CMD_SUCCESS;
		}
		else
		{
			user->WriteServ("974 %s %s :Failed to load module: %s",user->nick, parameters[0],ServerInstance->ModuleError());
			return CMD_FAILURE;
		}
	}
}

