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

/** Channel mode +h
 */
class ModeChannelHalfOp : public ModeHandler
{
 private:
 public:
	ModeChannelHalfOp(InspIRCd* Instance);
	ModeAction OnModeChange(userrec* source, userrec* dest, chanrec* channel, std::string &parameter, bool adding);
	std::string AddHalfOp(userrec *user,const char *dest,chanrec *chan,int status);
	std::string DelHalfOp(userrec *user,const char *dest,chanrec *chan,int status);
	ModePair ModeSet(userrec* source, userrec* dest, chanrec* channel, const std::string &parameter);
	unsigned int GetPrefixRank();
	void RemoveMode(chanrec* channel);
	void RemoveMode(userrec* user);
};

