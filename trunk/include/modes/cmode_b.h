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

#include "mode.h"
#include "channels.h"

class InspIRCd;

/** Channel mode +b
 */
class ModeChannelBan : public ModeHandler
{
 private:
	BanItem b;
 public:
	ModeChannelBan(InspIRCd* Instance);
	ModeAction OnModeChange(userrec* source, userrec* dest, chanrec* channel, std::string &parameter, bool adding);
	std::string& AddBan(userrec *user,std::string& dest,chanrec *chan,int status);
	std::string& DelBan(userrec *user,std::string& dest,chanrec *chan,int status);
	void DisplayList(userrec* user, chanrec* channel);
	ModePair ModeSet(userrec* source, userrec* dest, chanrec* channel, const std::string &parameter);
	void RemoveMode(userrec* user);
	void RemoveMode(chanrec* channel);
};

