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
#include "wildcard.h"
#include "mode.h"
#include "xline.h"
#include "socket.h"
#include "socketengine.h"
#include "command_parse.h"
#include "dns.h"

// version is a simple class for holding a modules version number
Version::Version(int major, int minor, int revision, int build, int flags, int api_ver)
: Major(major), Minor(minor), Revision(revision), Build(build), Flags(flags), API(api_ver)
{
}

Request::Request(char* anydata, Module* src, Module* dst)
: data(anydata), source(src), dest(dst)
{
	/* Ensure that because this module doesnt support ID strings, it doesnt break modules that do
	 * by passing them uninitialized pointers (could happen)
	 */
	id = '\0';
}

Request::Request(Module* src, Module* dst, const char* idstr)
: id(idstr), source(src), dest(dst)
{
}

char* Request::GetData()
{
	return this->data;
}

const char* Request::GetId()
{
	return this->id;
}

Module* Request::GetSource()
{
	return this->source;
}

Module* Request::GetDest()
{
	return this->dest;
}

char* Request::Send()
{
	if (this->dest)
	{
		return dest->OnRequest(this);
	}
	else
	{
		return NULL;
	}
}

Event::Event(char* anydata, Module* src, const std::string &eventid) : data(anydata), source(src), id(eventid) { }

char* Event::GetData()
{
	return (char*)this->data;
}

Module* Event::GetSource()
{
	return this->source;
}

char* Event::Send(InspIRCd* ServerInstance)
{
	FOREACH_MOD(I_OnEvent,OnEvent(this));
	return NULL;
}

std::string Event::GetEventID()
{
	return this->id;
}


// These declarations define the behavours of the base class Module (which does nothing at all)

		Module::Module(InspIRCd* Me) : ServerInstance(Me) { }
		Module::~Module() { }
void		Module::OnUserConnect(userrec* user) { }
void		Module::OnUserQuit(userrec* user, const std::string& message, const std::string &oper_message) { }
void		Module::OnUserDisconnect(userrec* user) { }
void		Module::OnUserJoin(userrec* user, chanrec* channel, bool &silent) { }
void		Module::OnPostJoin(userrec* user, chanrec* channel) { }
void		Module::OnUserPart(userrec* user, chanrec* channel, const std::string &partmessage, bool &silent) { }
void		Module::OnRehash(userrec* user, const std::string &parameter) { }
void		Module::OnServerRaw(std::string &raw, bool inbound, userrec* user) { }
int		Module::OnUserPreJoin(userrec* user, chanrec* chan, const char* cname, std::string &privs) { return 0; }
void		Module::OnMode(userrec* user, void* dest, int target_type, const std::string &text) { }
Version		Module::GetVersion() { return Version(1,0,0,0,VF_VENDOR,-1); }
void		Module::OnOper(userrec* user, const std::string &opertype) { }
void		Module::OnPostOper(userrec* user, const std::string &opertype) { }
void		Module::OnInfo(userrec* user) { }
void		Module::OnWhois(userrec* source, userrec* dest) { }
int		Module::OnUserPreInvite(userrec* source,userrec* dest,chanrec* channel) { return 0; }
int		Module::OnUserPreMessage(userrec* user,void* dest,int target_type, std::string &text,char status, CUList &exempt_list) { return 0; }
int		Module::OnUserPreNotice(userrec* user,void* dest,int target_type, std::string &text,char status, CUList &exempt_list) { return 0; }
int		Module::OnUserPreNick(userrec* user, const std::string &newnick) { return 0; }
void		Module::OnUserPostNick(userrec* user, const std::string &oldnick) { }
int		Module::OnAccessCheck(userrec* source,userrec* dest,chanrec* channel,int access_type) { return ACR_DEFAULT; }
void		Module::On005Numeric(std::string &output) { }
int		Module::OnKill(userrec* source, userrec* dest, const std::string &reason) { return 0; }
void		Module::OnLoadModule(Module* mod,const std::string &name) { }
void		Module::OnUnloadModule(Module* mod,const std::string &name) { }
void		Module::OnBackgroundTimer(time_t curtime) { }
int		Module::OnPreCommand(const std::string &command, const char** parameters, int pcnt, userrec *user, bool validated, const std::string &original_line) { return 0; }
void		Module::OnPostCommand(const std::string &command, const char** parameters, int pcnt, userrec *user, CmdResult result, const std::string &original_line) { }
bool		Module::OnCheckReady(userrec* user) { return true; }
int		Module::OnUserRegister(userrec* user) { return 0; }
int		Module::OnUserPreKick(userrec* source, userrec* user, chanrec* chan, const std::string &reason) { return 0; }
void		Module::OnUserKick(userrec* source, userrec* user, chanrec* chan, const std::string &reason, bool &silent) { }
int		Module::OnCheckInvite(userrec* user, chanrec* chan) { return 0; }
int		Module::OnCheckKey(userrec* user, chanrec* chan, const std::string &keygiven) { return 0; }
int		Module::OnCheckLimit(userrec* user, chanrec* chan) { return 0; }
int		Module::OnCheckBan(userrec* user, chanrec* chan) { return 0; }
int		Module::OnStats(char symbol, userrec* user, string_list &results) { return 0; }
int		Module::OnChangeLocalUserHost(userrec* user, const std::string &newhost) { return 0; }
int		Module::OnChangeLocalUserGECOS(userrec* user, const std::string &newhost) { return 0; }
int		Module::OnLocalTopicChange(userrec* user, chanrec* chan, const std::string &topic) { return 0; }
void		Module::OnEvent(Event* event) { return; }
char*		Module::OnRequest(Request* request) { return NULL; }
int		Module::OnOperCompare(const std::string &password, const std::string &input, int tagnumber) { return 0; }
void		Module::OnGlobalOper(userrec* user) { }
void		Module::OnPostConnect(userrec* user) { }
int		Module::OnAddBan(userrec* source, chanrec* channel,const std::string &banmask) { return 0; }
int		Module::OnDelBan(userrec* source, chanrec* channel,const std::string &banmask) { return 0; }
void		Module::OnRawSocketAccept(int fd, const std::string &ip, int localport) { }
int		Module::OnRawSocketWrite(int fd, const char* buffer, int count) { return 0; }
void		Module::OnRawSocketClose(int fd) { }
void		Module::OnRawSocketConnect(int fd) { }
int		Module::OnRawSocketRead(int fd, char* buffer, unsigned int count, int &readresult) { return 0; }
void		Module::OnUserMessage(userrec* user, void* dest, int target_type, const std::string &text, char status, const CUList &exempt_list) { }
void		Module::OnUserNotice(userrec* user, void* dest, int target_type, const std::string &text, char status, const CUList &exempt_list) { }
void 		Module::OnRemoteKill(userrec* source, userrec* dest, const std::string &reason, const std::string &operreason) { }
void		Module::OnUserInvite(userrec* source,userrec* dest,chanrec* channel) { }
void		Module::OnPostLocalTopicChange(userrec* user, chanrec* chan, const std::string &topic) { }
void		Module::OnGetServerDescription(const std::string &servername,std::string &description) { }
void		Module::OnSyncUser(userrec* user, Module* proto, void* opaque) { }
void		Module::OnSyncChannel(chanrec* chan, Module* proto, void* opaque) { }
void		Module::ProtoSendMode(void* opaque, int target_type, void* target, const std::string &modeline) { }
void		Module::OnSyncChannelMetaData(chanrec* chan, Module* proto,void* opaque, const std::string &extname, bool displayable) { }
void		Module::OnSyncUserMetaData(userrec* user, Module* proto,void* opaque, const std::string &extname, bool displayable) { }
void		Module::OnSyncOtherMetaData(Module* proto, void* opaque, bool displayable) { }
void		Module::OnDecodeMetaData(int target_type, void* target, const std::string &extname, const std::string &extdata) { }
void		Module::ProtoSendMetaData(void* opaque, int target_type, void* target, const std::string &extname, const std::string &extdata) { }
void		Module::OnWallops(userrec* user, const std::string &text) { }
void		Module::OnChangeHost(userrec* user, const std::string &newhost) { }
void		Module::OnChangeName(userrec* user, const std::string &gecos) { }
void		Module::OnAddGLine(long duration, userrec* source, const std::string &reason, const std::string &hostmask) { }
void		Module::OnAddZLine(long duration, userrec* source, const std::string &reason, const std::string &ipmask) { }
void		Module::OnAddKLine(long duration, userrec* source, const std::string &reason, const std::string &hostmask) { }
void		Module::OnAddQLine(long duration, userrec* source, const std::string &reason, const std::string &nickmask) { }
void		Module::OnAddELine(long duration, userrec* source, const std::string &reason, const std::string &hostmask) { }
void		Module::OnDelGLine(userrec* source, const std::string &hostmask) { }
void		Module::OnDelZLine(userrec* source, const std::string &ipmask) { }
void		Module::OnDelKLine(userrec* source, const std::string &hostmask) { }
void		Module::OnDelQLine(userrec* source, const std::string &nickmask) { }
void		Module::OnDelELine(userrec* source, const std::string &hostmask) { }
void 		Module::OnCleanup(int target_type, void* item) { }
void		Module::Implements(char* Implements) { for (int j = 0; j < 255; j++) Implements[j] = 0; }
void		Module::OnChannelDelete(chanrec* chan) { }
Priority	Module::Prioritize() { return PRIORITY_DONTCARE; }
void		Module::OnSetAway(userrec* user) { }
void		Module::OnCancelAway(userrec* user) { }
int		Module::OnUserList(userrec* user, chanrec* Ptr, CUList* &userlist) { return 0; }
int		Module::OnWhoisLine(userrec* user, userrec* dest, int &numeric, std::string &text) { return 0; }
void		Module::OnBuildExemptList(MessageType message_type, chanrec* chan, userrec* sender, char status, CUList &exempt_list) { }
void		Module::OnGarbageCollect() { }
void		Module::OnBufferFlushed(userrec* user) { }

long InspIRCd::PriorityAfter(const std::string &modulename)
{
	for (unsigned int j = 0; j < this->Config->module_names.size(); j++)
	{
		if (this->Config->module_names[j] == modulename)
		{
			return ((j << 8) | PRIORITY_AFTER);
		}
	}
	return PRIORITY_DONTCARE;
}

long InspIRCd::PriorityBefore(const std::string &modulename)
{
	for (unsigned int j = 0; j < this->Config->module_names.size(); j++)
	{
		if (this->Config->module_names[j] == modulename)
		{
			return ((j << 8) | PRIORITY_BEFORE);
		}
	}
	return PRIORITY_DONTCARE;
}

bool InspIRCd::PublishFeature(const std::string &FeatureName, Module* Mod)
{
	if (Features.find(FeatureName) == Features.end())
	{
		Features[FeatureName] = Mod;
		return true;
	}
	return false;
}

bool InspIRCd::UnpublishFeature(const std::string &FeatureName)
{
	featurelist::iterator iter = Features.find(FeatureName);
	
	if (iter == Features.end())
		return false;

	Features.erase(iter);
	return true;
}

Module* InspIRCd::FindFeature(const std::string &FeatureName)
{
	featurelist::iterator iter = Features.find(FeatureName);

	if (iter == Features.end())
		return NULL;

	return iter->second;
}

bool InspIRCd::PublishInterface(const std::string &InterfaceName, Module* Mod)
{
	interfacelist::iterator iter = Interfaces.find(InterfaceName);

	if (iter == Interfaces.end())
	{
		modulelist ml;
		ml.push_back(Mod);
		Interfaces[InterfaceName] = std::make_pair(0, ml);
		return true;
	}
	else
	{
		iter->second.second.push_back(Mod);
		return true;
	}
	return false;
}

bool InspIRCd::UnpublishInterface(const std::string &InterfaceName, Module* Mod)
{
	interfacelist::iterator iter = Interfaces.find(InterfaceName);

	if (iter == Interfaces.end())
		return false;

	for (modulelist::iterator x = iter->second.second.begin(); x != iter->second.second.end(); x++)
	{
		if (*x == Mod)
		{
			iter->second.second.erase(x);
			if (iter->second.second.empty())
				Interfaces.erase(InterfaceName);
			return true;
		}
	}
	return false;
}

modulelist* InspIRCd::FindInterface(const std::string &InterfaceName)
{
	interfacelist::iterator iter = Interfaces.find(InterfaceName);
	if (iter == Interfaces.end())
		return NULL;
	else
		return &(iter->second.second);
}

void InspIRCd::UseInterface(const std::string &InterfaceName)
{
	interfacelist::iterator iter = Interfaces.find(InterfaceName);
	if (iter != Interfaces.end())
		iter->second.first++;

}

void InspIRCd::DoneWithInterface(const std::string &InterfaceName)
{
	interfacelist::iterator iter = Interfaces.find(InterfaceName);
	if (iter != Interfaces.end())
		iter->second.first--;
}

std::pair<int,std::string> InspIRCd::GetInterfaceInstanceCount(Module* m)
{
	for (interfacelist::iterator iter = Interfaces.begin(); iter != Interfaces.end(); iter++)
	{
		for (modulelist::iterator x = iter->second.second.begin(); x != iter->second.second.end(); x++)
		{
			if (*x == m)
			{
				return std::make_pair(iter->second.first, iter->first);
			}
		}
	}
	return std::make_pair(0, "");
}

const std::string& InspIRCd::GetModuleName(Module* m)
{
	static std::string nothing; /* Prevent compiler warning */

	if (!this->GetModuleCount())
		return nothing;

	for (int i = 0; i <= this->GetModuleCount(); i++)
	{
		if (this->modules[i] == m)
		{
			return this->Config->module_names[i];
		}
	}
	return nothing; /* As above */
}

void InspIRCd::RehashServer()
{
	this->WriteOpers("*** Rehashing config file");
	this->RehashUsersAndChans();
	this->Config->Read(false,NULL);
	this->ResetMaxBans();
	this->Res->Rehash();
}

/* This is ugly, yes, but hash_map's arent designed to be
 * addressed in this manner, and this is a bit of a kludge.
 * Luckily its a specialist function and rarely used by
 * many modules (in fact, it was specially created to make
 * m_safelist possible, initially).
 */

chanrec* InspIRCd::GetChannelIndex(long index)
{
	int target = 0;
	for (chan_hash::iterator n = this->chanlist->begin(); n != this->chanlist->end(); n++, target++)
	{
		if (index == target)
			return n->second;
	}
	return NULL;
}

bool InspIRCd::MatchText(const std::string &sliteral, const std::string &spattern)
{
	return match(sliteral.c_str(),spattern.c_str());
}

CmdResult InspIRCd::CallCommandHandler(const std::string &commandname, const char** parameters, int pcnt, userrec* user)
{
	return this->Parser->CallHandler(commandname,parameters,pcnt,user);
}

bool InspIRCd::IsValidModuleCommand(const std::string &commandname, int pcnt, userrec* user)
{
	return this->Parser->IsValidCommand(commandname, pcnt, user);
}

void InspIRCd::AddCommand(command_t *f)
{
	if (!this->Parser->CreateCommand(f))
	{
		ModuleException err("Command "+std::string(f->command)+" already exists.");
		throw (err);
	}
}

void InspIRCd::SendMode(const char** parameters, int pcnt, userrec *user)
{
	this->Modes->Process(parameters,pcnt,user,true);
}

void InspIRCd::DumpText(userrec* User, const std::string &LinePrefix, stringstream &TextStream)
{
	std::string CompleteLine = LinePrefix;
	std::string Word;
	while (TextStream >> Word)
	{
		if (CompleteLine.length() + Word.length() + 3 > 500)
		{
			User->WriteServ(CompleteLine);
			CompleteLine = LinePrefix;
		}
		CompleteLine = CompleteLine + Word + " ";
	}
	User->WriteServ(CompleteLine);
}

userrec* InspIRCd::FindDescriptor(int socket)
{
	return reinterpret_cast<userrec*>(this->SE->GetRef(socket));
}

bool InspIRCd::AddMode(ModeHandler* mh, const unsigned char mode)
{
	return this->Modes->AddMode(mh,mode);
}

bool InspIRCd::AddModeWatcher(ModeWatcher* mw)
{
	return this->Modes->AddModeWatcher(mw);
}

bool InspIRCd::DelModeWatcher(ModeWatcher* mw)
{
	return this->Modes->DelModeWatcher(mw);
}

bool InspIRCd::AddResolver(Resolver* r, bool cached)
{
	if (!cached)
		return this->Res->AddResolverClass(r);
	else
	{
		r->TriggerCachedResult();
		delete r;
		return true;
	}
}

void InspIRCd::AddGLine(long duration, const std::string &source, const std::string &reason, const std::string &hostmask)
{
	XLines->add_gline(duration, source.c_str(), reason.c_str(), hostmask.c_str());
	XLines->apply_lines(APPLY_GLINES);
}

void InspIRCd::AddQLine(long duration, const std::string &source, const std::string &reason, const std::string &nickname)
{
	XLines->add_qline(duration, source.c_str(), reason.c_str(), nickname.c_str());
	XLines->apply_lines(APPLY_QLINES);
}

void InspIRCd::AddZLine(long duration, const std::string &source, const std::string &reason, const std::string &ipaddr)
{
	XLines->add_zline(duration, source.c_str(), reason.c_str(), ipaddr.c_str());
	XLines->apply_lines(APPLY_ZLINES);
}

void InspIRCd::AddKLine(long duration, const std::string &source, const std::string &reason, const std::string &hostmask)
{
	XLines->add_kline(duration, source.c_str(), reason.c_str(), hostmask.c_str());
	XLines->apply_lines(APPLY_KLINES);
}

void InspIRCd::AddELine(long duration, const std::string &source, const std::string &reason, const std::string &hostmask)
{
	XLines->add_eline(duration, source.c_str(), reason.c_str(), hostmask.c_str());
}

bool InspIRCd::DelGLine(const std::string &hostmask)
{
	return XLines->del_gline(hostmask.c_str());
}

bool InspIRCd::DelQLine(const std::string &nickname)
{
	return XLines->del_qline(nickname.c_str());
}

bool InspIRCd::DelZLine(const std::string &ipaddr)
{
	return XLines->del_zline(ipaddr.c_str());
}

bool InspIRCd::DelKLine(const std::string &hostmask)
{
	return XLines->del_kline(hostmask.c_str());
}

bool InspIRCd::DelELine(const std::string &hostmask)
{
	return XLines->del_eline(hostmask.c_str());
}

/*
 * XXX why on *earth* is this in modules.cpp...? I think
 * perhaps we need a server.cpp for InspIRCd:: stuff where possible. -- w00t
 */
bool InspIRCd::IsValidMask(const std::string &mask)
{
	char* dest = (char*)mask.c_str();
	if (strchr(dest,'!')==0)
		return false;
	if (strchr(dest,'@')==0)
		return false;
	for (char* i = dest; *i; i++)
		if (*i < 32)
			return false;
	for (char* i = dest; *i; i++)
		if (*i > 126)
			return false;
	unsigned int c = 0;
	for (char* i = dest; *i; i++)
		if (*i == '!')
			c++;
	if (c>1)
		return false;
	c = 0;
	for (char* i = dest; *i; i++)
		if (*i == '@')
			c++;
	if (c>1)
		return false;

	return true;
}

Module* InspIRCd::FindModule(const std::string &name)
{
	for (int i = 0; i <= this->GetModuleCount(); i++)
	{
		if (this->Config->module_names[i] == name)
		{
			return this->modules[i];
		}
	}
	return NULL;
}

ConfigReader::ConfigReader(InspIRCd* Instance) : ServerInstance(Instance)
{
	/* Is there any reason to load the entire config file again here?
	 * it's needed if they specify another config file, but using the
	 * default one we can just use the global config data - pre-parsed!
	 */
	this->errorlog = new std::ostringstream(std::stringstream::in | std::stringstream::out);
	
	this->data = &ServerInstance->Config->config_data;
	this->privatehash = false;
}


ConfigReader::~ConfigReader()
{
	if (this->errorlog)
		DELETE(this->errorlog);
	if(this->privatehash)
		DELETE(this->data);
}


ConfigReader::ConfigReader(InspIRCd* Instance, const std::string &filename) : ServerInstance(Instance)
{
	ServerInstance->Config->ClearStack();

	this->data = new ConfigDataHash;
	this->privatehash = true;
	this->errorlog = new std::ostringstream(std::stringstream::in | std::stringstream::out);
	this->readerror = ServerInstance->Config->LoadConf(*this->data, filename, *this->errorlog);
	if (!this->readerror)
		this->error = CONF_FILE_NOT_FOUND;
}


std::string ConfigReader::ReadValue(const std::string &tag, const std::string &name, const std::string &default_value, int index, bool allow_linefeeds)
{
	/* Don't need to strlcpy() tag and name anymore, ReadConf() takes const char* */ 
	std::string result;
	
	if (!ServerInstance->Config->ConfValue(*this->data, tag, name, default_value, index, result, allow_linefeeds))
	{
		this->error = CONF_VALUE_NOT_FOUND;
	}
	return result;
}

std::string ConfigReader::ReadValue(const std::string &tag, const std::string &name, int index, bool allow_linefeeds)
{
	return ReadValue(tag, name, "", index, allow_linefeeds);
}

bool ConfigReader::ReadFlag(const std::string &tag, const std::string &name, const std::string &default_value, int index)
{
	return ServerInstance->Config->ConfValueBool(*this->data, tag, name, default_value, index);
}

bool ConfigReader::ReadFlag(const std::string &tag, const std::string &name, int index)
{
	return ReadFlag(tag, name, "", index);
}


long ConfigReader::ReadInteger(const std::string &tag, const std::string &name, const std::string &default_value, int index, bool needs_unsigned)
{
	int result;
	
	if(!ServerInstance->Config->ConfValueInteger(*this->data, tag, name, default_value, index, result))
	{
		this->error = CONF_VALUE_NOT_FOUND;
		return 0;
	}
	
	if ((needs_unsigned) && (result < 0))
	{
		this->error = CONF_NOT_UNSIGNED;
		return 0;
	}
	
	return result;
}

long ConfigReader::ReadInteger(const std::string &tag, const std::string &name, int index, bool needs_unsigned)
{
	return ReadInteger(tag, name, "", index, needs_unsigned);
}

long ConfigReader::GetError()
{
	long olderr = this->error;
	this->error = 0;
	return olderr;
}

void ConfigReader::DumpErrors(bool bail, userrec* user)
{
	ServerInstance->Config->ReportConfigError(this->errorlog->str(), bail, user);
}


int ConfigReader::Enumerate(const std::string &tag)
{
	return ServerInstance->Config->ConfValueEnum(*this->data, tag);
}

int ConfigReader::EnumerateValues(const std::string &tag, int index)
{
	return ServerInstance->Config->ConfVarEnum(*this->data, tag, index);
}

bool ConfigReader::Verify()
{
	return this->readerror;
}


FileReader::FileReader(InspIRCd* Instance, const std::string &filename) : ServerInstance(Instance)
{
	LoadFile(filename);
}

FileReader::FileReader(InspIRCd* Instance) : ServerInstance(Instance)
{
}

std::string FileReader::Contents()
{
	std::string x;
	for (file_cache::iterator a = this->fc.begin(); a != this->fc.end(); a++)
	{
		x.append(*a);
		x.append("\r\n");
	}
	return x;
}

unsigned long FileReader::ContentSize()
{
	return this->contentsize;
}

void FileReader::CalcSize()
{
	unsigned long n = 0;
	for (file_cache::iterator a = this->fc.begin(); a != this->fc.end(); a++)
		n += (a->length() + 2);
	this->contentsize = n;
}

void FileReader::LoadFile(const std::string &filename)
{
	file_cache c;
	c.clear();
	if (ServerInstance->Config->ReadFile(c,filename.c_str()))
	{
		this->fc = c;
		this->CalcSize();
	}
}


FileReader::~FileReader()
{
}

bool FileReader::Exists()
{
	return (!(fc.size() == 0));
}

std::string FileReader::GetLine(int x)
{
	if ((x<0) || ((unsigned)x>fc.size()))
		return "";
	return fc[x];
}

int FileReader::FileSize()
{
	return fc.size();
}


