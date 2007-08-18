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

/* $ModDesc: Restricts banned users in a channel. May not speak, etc. */

class ModuleRestrictBanned : public Module
{
 private:
 public:
	ModuleRestrictBanned(InspIRCd* Me) : Module(Me)
	{
	}
	
	virtual ~ModuleRestrictBanned()
	{
	}
	
	virtual Version GetVersion()
	{
		return Version(1,1,0,1,VF_VENDOR,API_VERSION);
	}

	void Implements(char* List)
	{
		List[I_OnLocalTopicChange] = List[I_OnUserPreNick] = List[I_OnUserPreNotice] = List[I_OnUserPreMessage] = 1;
	}

	int CheckRestricted(userrec *user, chanrec *channel, const std::string &action)
	{
		/* aren't local? we don't care. */
		if (!IS_LOCAL(user))
			return 0;

		if (channel->GetStatus(user) < STATUS_VOICE && channel->IsBanned(user))
		{
			/* banned, boned. drop the message. */
			user->WriteServ("NOTICE "+std::string(user->nick)+" :*** You may not " + action + ", as you are banned on channel " + channel->name);
			return 1;
		}

		return 0;
	}

	virtual int OnUserPreNick(userrec *user, const std::string &newnick)
	{
		/* if they aren't local, we don't care */
		if (!IS_LOCAL(user))
			return 0;

		/* bit of a special case. */
		for (UCListIter i = user->chans.begin(); i != user->chans.end(); i++)
		{
			if (CheckRestricted(user, i->first, "change your nickname") == 1)
				return 1;
		}

		return 0;
	}

	virtual int OnLocalTopicChange(userrec *user, chanrec *channel, const std::string &topic)
	{
		return CheckRestricted(user, channel, "change the topic");
	}
	
	virtual int OnUserPreMessage(userrec* user,void* dest,int target_type, std::string &text, char status, CUList &exempt_list)
	{
		return OnUserPreNotice(user,dest,target_type,text,status,exempt_list);
	}

	virtual int OnUserPreNotice(userrec* user,void* dest,int target_type, std::string &text, char status, CUList &exempt_list)
	{
		if (target_type == TYPE_CHANNEL)
		{
			chanrec *channel = (chanrec *)dest;

			return CheckRestricted(user, channel, "message the channel");
		}

		return 0;
	}
};

MODULE_INIT(ModuleRestrictBanned)
