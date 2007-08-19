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
#include "configreader.h"
#include "users.h"
#include "modules.h"
#include "commands/cmd_info.h"

extern "C" DllExport command_t* init_command(InspIRCd* Instance)
{
	return new cmd_info(Instance);
}

/** Handle /INFO
 */
CmdResult cmd_info::Handle (const char** parameters, int pcnt, userrec *user)
{
	user->WriteServ( "371 %s :=-=-=-= %s =-=-=-=", user->nick, VERSION);
	user->WriteServ( "371 %s :Based on the original code by the InspIRCd Development Team", user->nick);
	user->WriteServ( "371 %s :(C) 2002-2007 InspIRCd Development Team", user->nick);
	user->WriteServ( "371 %s :               http://www.inspircd.org/wiki/index.php/Credits", user->nick);
	user->WriteServ( "371 %s :(C) 2007 UnrealIRCd Development Team", user->nick);
	user->WriteServ( "371 %s :", user->nick);
	user->WriteServ( "371 %s :This version was brought to you by:", user->nick);
	user->WriteServ( "371 %s :* Stskeeps     <stskeeps@unrealircd.com>", user->nick);
	user->WriteServ( "371 %s :* aquanight    <aquanight@unrealircd.com>", user->nick);
	user->WriteServ( "371 %s :* WolfSage     <wolfsage@unrealircd.com>", user->nick);
	user->WriteServ( "371 %s :", user->nick);
	FOREACH_MOD(I_OnInfo,OnInfo(user));
	user->WriteServ( "374 %s :End of /INFO list", user->nick);
	return CMD_SUCCESS;
}
