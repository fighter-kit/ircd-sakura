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

static FileReader *quotes = NULL;

std::string q_file;
std::string prefix;
std::string suffix;

/* $ModDesc: Provides random Quotes on Connect. */

/** Handle /RANDQUOTE
 */
class cmd_randquote : public command_t
{
 public:
	cmd_randquote (InspIRCd* Instance) : command_t(Instance,"RANDQUOTE", 0, 0)
	{
		this->source = "m_randquote.so";
	}

	CmdResult Handle (const char** parameters, int pcntl, userrec *user)
	{
		std::string str;
		int fsize;

		if (q_file.empty() || quotes->Exists())
		{
			fsize = quotes->FileSize();
			str = quotes->GetLine(rand() % fsize);
			user->WriteServ("NOTICE %s :%s%s%s",user->nick,prefix.c_str(),str.c_str(),suffix.c_str());
		}
		else
		{
			user->WriteServ("NOTICE %s :Your administrator specified an invalid quotes file, please bug them about this.", user->nick);
			return CMD_FAILURE;
		}

		return CMD_LOCALONLY;
	}
};

/** Thrown by m_randquote
 */
class RandquoteException : public ModuleException
{
 private:
	const std::string err;
 public:
	RandquoteException(const std::string &message) : err(message) { }

	~RandquoteException() throw () { }

	virtual const char* GetReason()
	{
		return err.c_str();
	}
};

class ModuleRandQuote : public Module
{
 private:
	cmd_randquote* mycommand;
	ConfigReader *conf;
 public:
	ModuleRandQuote(InspIRCd* Me)
		: Module(Me)
	{
		
		conf = new ConfigReader(ServerInstance);
		// Sort the Randomizer thingie..
		srand(time(NULL));

		q_file = conf->ReadValue("randquote","file",0);
		prefix = conf->ReadValue("randquote","prefix",0);
		suffix = conf->ReadValue("randquote","suffix",0);

		mycommand = NULL;

		if (q_file.empty())
		{
			RandquoteException e("m_randquote: Quotefile not specified - Please check your config.");
			throw(e);
		}

		quotes = new FileReader(ServerInstance, q_file);
		if(!quotes->Exists())
		{
			RandquoteException e("m_randquote: QuoteFile not Found!! Please check your config - module will not function.");
			throw(e);
		}
		else
		{
			/* Hidden Command -- Mode clients assume /quote sends raw data to an IRCd >:D */
			mycommand = new cmd_randquote(ServerInstance);
			ServerInstance->AddCommand(mycommand);
		}
	}

	void Implements(char* List)
	{
		List[I_OnUserConnect] = 1;
	}
	
	virtual ~ModuleRandQuote()
	{
		DELETE(conf);
		DELETE(quotes);
	}
	
	virtual Version GetVersion()
	{
		return Version(1,1,0,1,VF_VENDOR,API_VERSION);
	}
	
	virtual void OnUserConnect(userrec* user)
	{
		if (mycommand)
			mycommand->Handle(NULL, 0, user);
	}
};

MODULE_INIT(ModuleRandQuote)
