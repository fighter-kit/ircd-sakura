// m_whoismodes.cpp - ViaraiX
// Updated for new API - LeaChim
// Updated for even newer API - Om
// Updated for 1.1-beta5 - AnMaster
// Cosmetic changes for Unreal4.0 - CuttingEdge

#include "users.h"
#include "channels.h"
#include "modules.h"
#include "inspircd.h"

/* $ModDesc: Allows opers to see users modes on whois */
class ModuleWhoismodes : public Module
{
public:
	ModuleWhoismodes(InspIRCd* Me)
	: Module(Me)
	{
	}
	
	virtual Version GetVersion()
	{
		return Version(1,1,0,0,0,API_VERSION);
	}
	
	virtual void Implements(char* List)
	{
		List[I_OnWhois] = 1;
	}

	virtual void OnWhois(userrec* source, userrec* dest)
	{
		if(*source->oper) {
			if (dest->modes[UM_SNOMASK] != 0) {
				ServerInstance->SendWhoisLine(source, dest, 379, "%s %s :is using modes +%s +%s",
				                              source->nick, dest->nick, dest->FormatModes(),
				                              dest->FormatNoticeMasks());
			} else {
				ServerInstance->SendWhoisLine(source, dest, 379, "%s %s :is using modes +%s",
				                              source->nick, dest->nick, dest->FormatModes());
			}
		}
	}
};

class ModuleWhoismodesFactory : public ModuleFactory
{
	public: ModuleWhoismodesFactory()
	{
	}

	~ModuleWhoismodesFactory()
	{
	}

	virtual Module* CreateModule(InspIRCd* Me)
	{
		return new ModuleWhoismodes(Me);
	}
}; 

extern "C" void* init_module() 
{ 
	return new ModuleWhoismodesFactory; 
}
