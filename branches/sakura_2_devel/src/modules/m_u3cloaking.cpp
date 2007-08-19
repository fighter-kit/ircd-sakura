/*       +------------------------------------+
 *       | UnrealIRCd v4.0                    |
 *       +------------------------------------+
 *
 * UnrealIRCd 4.0 (C) 2007 Carsten Valdemar Munk 
 * This program is free but copyrighted software; see
 *            the file COPYING for details.
 *
*/

#include "inspircd.h"
#include "users.h"
#include "channels.h"
#include "modules.h"
#include "m_hash.h"

/* $ModDesc: Provides masking of user hostnames */
/* $ModDep: m_hash.h */

/** Handles user mode +x
 */
class U3CloakUser : public ModeHandler
{
	
	std::string prefix;
	std::string key1;
	std::string key2;
	std::string key3;
	Module* Sender;
	Module* HashProvider;
	/** Downsamples a 128bit result to 32bits (md5 -> unsigned int) */
	unsigned int downsample(char *i)
	{
		unsigned char r[4];
	
	        r[0] = i[0] ^ i[1] ^ i[2] ^ i[3];
	        r[1] = i[4] ^ i[5] ^ i[6] ^ i[7];
	        r[2] = i[8] ^ i[9] ^ i[10] ^ i[11];
	        r[3] = i[12] ^ i[13] ^ i[14] ^ i[15];
	
	        return ( ((unsigned int)r[0] << 24) +
	                 ((unsigned int)r[1] << 16) +
       		          ((unsigned int)r[2] << 8) +
       		          (unsigned int)r[3]);
	}
	
	void DoMD5(const char *result, const char *buf, size_t len)
	{
		unsigned int iv[] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476 };
		HashResetRequest(Sender, HashProvider).Send();
		HashKeyRequest(Sender, HashProvider, iv).Send();
		unsigned char *hashres = (unsigned char*) HashRawRequest(Sender, HashProvider, buf).Send();
		memcpy((void *)result, (void *)hashres, 16);
	} 
 public:
	U3CloakUser(InspIRCd* Instance, Module* Source, Module* Hash) : ModeHandler(Instance, 'x', 0, 0, false, MODETYPE_USER, false), Sender(Source), HashProvider(Hash)
	{
	}

	ModeAction OnModeChange(userrec* source, userrec* dest, chanrec* channel, std::string &parameter, bool adding)
	{
		if (source != dest)
			return MODEACTION_DENY;

		/* For remote clients, we dont take any action, we just allow it.
		 * The local server where they are will set their cloak instead.
		 */
		if (!IS_LOCAL(dest))
			return MODEACTION_ALLOW;

		if (adding)
		{
			if(!dest->IsModeSet('x'))
			{
				/* The mode is being turned on - so attempt to
				 * allocate the user a cloaked host using a non-reversible
				 * algorithm (its simple, but its non-reversible so the
				 * simplicity doesnt really matter). This algorithm
				 * will not work if the user has only one level of domain
				 * naming in their hostname (e.g. if they are on a lan or
				 * are connecting via localhost) -- this doesnt matter much.
				 */

				char* n1 = strchr(dest->host,'.');
				char* n2 = strchr(dest->host,':');
				std::string b;
				if (n1 || n2)
				{
#ifdef IPV6
					in6_addr testaddr;
					in_addr testaddr2;
					if ((dest->GetProtocolFamily() == AF_INET6) && (inet_pton(AF_INET6,dest->host,&testaddr) < 1))
						/* Invalid ipv6 address, and ipv6 user (resolved host) */
						b = CloakHost(dest->host);
					else if ((dest->GetProtocolFamily() == AF_INET) && (inet_aton(dest->host,&testaddr2) < 1))
						/* Invalid ipv4 address, and ipv4 user (resolved host) */
						b = CloakHost(dest->host);
					else
						/* Valid ipv6 or ipv4 address (not resolved) ipv4 or ipv6 user */
						b = ((!strchr(dest->host,':')) ? Cloak4(dest->host) : Cloak6(dest->host));
#else
					in_addr testaddr;
					if ((inet_aton(dest->host,&testaddr) < 1))
						/* Invalid ipv4 address, and ipv4 user (resolved host) */
						b = CloakHost(dest->host);
					else
						/* Valid ipv4 address (not resolved) ipv4 user */
						b = Cloak4(dest->host);
#endif
					
					dest->ChangeDisplayedHost(b.c_str());
				}
				
				dest->SetMode('x',true);
				return MODEACTION_ALLOW;
			}
		}
		else
  		{
			if (dest->IsModeSet('x'))
			{
  				/* User is removing the mode, so just restore their real host
  				 * and make it match the displayed one.
				 */
				dest->ChangeDisplayedHost(dest->host);
				dest->SetMode('x',false);
				return MODEACTION_ALLOW;
			}
		}

		return MODEACTION_DENY;
	}

	std::string Cloak4(const char *host)
	{
		unsigned int a, b, c, d;
		static char buf[512], res[512], res2[512], result[128];
		unsigned long n;
		unsigned int alpha, beta, gamma;
		sscanf(host, "%u.%u.%u.%u", &a, &b, &c, &d);
		/*
		 * Output: ALPHA.BETA.GAMMA.IP
		 * ALPHA is unique for a.b.c.d
		 * BETA  is unique for a.b.c.*
		 * GAMMA is unique for a.b.*
		 * We cloak like this:
		 * ALPHA = downsample(md5(md5("KEY2:A.B.C.D:KEY3")+"KEY1"));
		 * BETA  = downsample(md5(md5("KEY3:A.B.C:KEY1")+"KEY2"));
		 * GAMMA = downsample(md5(md5("KEY1:A.B:KEY2")+"KEY3"));
		 */
		/* ALPHA... */
		sprintf(buf, "%s:%s:%s", key2.c_str(), host, key3.c_str());
		DoMD5(res, buf, strlen(buf));
		strcpy(res+16, key1.c_str()); /* first 16 bytes are filled, append our key.. */
		n = strlen(res+16) + 16;
		DoMD5(res2, res, n);
		alpha = downsample(res2);
		
		/* BETA... */
		sprintf(buf, "%s:%d.%d.%d:%s", key3.c_str(), a, b, c, key1.c_str());
		DoMD5(res, buf, strlen(buf));
		strcpy(res+16, key2.c_str()); /* first 16 bytes are filled, append our key.. */
		n = strlen(res+16) + 16;
		DoMD5(res2, res, n);
		beta = downsample(res2);

		/* GAMMA... */
		sprintf(buf, "%s:%d.%d:%s", key1.c_str(), a, b, key2.c_str());
		DoMD5(res, buf, strlen(buf));
		strcpy(res+16, key3.c_str()); /* first 16 bytes are filled, append our key.. */
		n = strlen(res+16) + 16;
		DoMD5(res2, res, n);
		gamma = downsample(res2);

		sprintf(result, "%X.%X.%X.IP", alpha, beta, gamma);
		return result;
	}
	
	std::string Cloak6(const char *host)
	{
#ifdef IPV6
		unsigned int a, b, c, d, e, f, g, h;
		static char buf[512], res[512], res2[512], result[128];
		unsigned long n;
		unsigned int alpha, beta, gamma;

		/* 
		 * Output: ALPHA:BETA:GAMMA:IP
		 * ALPHA is unique for a:b:c:d:e:f:g:h
		 * BETA  is unique for a:b:c:d:e:f:g
		 * GAMMA is unique for a:b:c:d
		 * We cloak like this:
		 * ALPHA = downsample(md5(md5("KEY2:a:b:c:d:e:f:g:h:KEY3")+"KEY1"));
		 * BETA  = downsample(md5(md5("KEY3:a:b:c:d:e:f:g:KEY1")+"KEY2"));
		 * GAMMA = downsample(md5(md5("KEY1:a:b:c:d:KEY2")+"KEY3"));
		 */
		in6_addr addr;
		// Unfold
		inet_pton(AF_INET6, host,&addr);
		inet_ntop(AF_INET6, &addr, buf, sizeof(buf));
		sscanf(buf, "%x:%x:%x:%x:%x:%x:%x:%x",
			&a, &b, &c, &d, &e, &f, &g, &h);

		/* ALPHA... */
		sprintf(buf, "%s:%s:%s", key2.c_str(), host, key3.c_str());
		DoMD5(res, buf, strlen(buf));
		strcpy(res+16, key1.c_str()); /* first 16 bytes are filled, append our key.. */
		n = strlen(res+16) + 16;
		DoMD5(res2, res, n);
		alpha = downsample(res2);
		
		/* BETA... */
		sprintf(buf, "%s:%x:%x:%x:%x:%x:%x:%x:%s", key3.c_str(), a, b, c, d, e, f, g, key1.c_str());
		DoMD5(res, buf, strlen(buf));
		strcpy(res+16, key2.c_str()); /* first 16 bytes are filled, append our key.. */
		n = strlen(res+16) + 16;
		DoMD5(res2, res, n);
		beta = downsample(res2);
		
		/* GAMMA... */
		sprintf(buf, "%s:%x:%x:%x:%x:%s", key1.c_str(), a, b, c, d, key2.c_str());
		DoMD5(res, buf, strlen(buf));
		strcpy(res+16, key3.c_str()); /* first 16 bytes are filled, append our key.. */
		n = strlen(res+16) + 16;
		DoMD5(res2, res, n);
		gamma = downsample(res2);
		
		sprintf(result, "%X:%X:%X:IP", alpha, beta, gamma);
		return result;
#else
		return host;
#endif
	}
	
	std::string CloakHost(const char *host)
	{
		const char *p;
		static char buf[512], res[512], res2[512], result[64];
		unsigned int alpha, n;

		sprintf(buf, "%s:%s:%s", key1.c_str(), host, key2.c_str());
		DoMD5(res, buf, strlen(buf));
		strcpy(res+16, key3.c_str()); /* first 16 bytes are filled, append our key.. */
		n = strlen(res+16) + 16;
		DoMD5(res2, res, n);
		alpha = downsample(res2);
		
		for (p = host; *p; p++)
			if (*p == '.')
				if (isalpha(*(p + 1)))
					break;

		if (*p)
		{
			unsigned int len;
			p++;
			sprintf(result, "%s-%X.", prefix.c_str(), alpha);
			len = strlen(result) + strlen(p);
			if (len <= 63)
				strcat(result, p);
			else
				strcat(result, p + (len - 63));
		} else
			sprintf(result,  "%s-%X", prefix.c_str(), alpha);
			
		return result;
	}
	
	void DoRehash()
	{
		ConfigReader Conf(ServerInstance);
		key1 = key2 = key3 = "";
		key1 = Conf.ReadValue("cloak","key1",0,true);
		key2 = Conf.ReadValue("cloak","key2",0,true);
		key3 = Conf.ReadValue("cloak","key3",0,true);
		prefix = Conf.ReadValue("cloak","prefix",0);

		if (prefix.empty())
			prefix = ServerInstance->Config->Network;

		if (key1.empty() || key2.empty() || key3.empty())
		{
			std::string detail;
			if (key1.empty())
				detail = "<cloak:key1> is not valid, it may be set to a too high/low value, or it may not exist.";
			else if (key2.empty())
				detail = "<cloak:key2> is not valid, it may be set to a too high/low value, or it may not exist.";
			else if (key3.empty())
				detail = "<cloak:key3> is not valid, it may be set to a too high/low value, or it may not exist.";
			throw ModuleException("You have not defined cloak keys for m_u3cloaking!!! THIS IS INSECURE AND SHOULD BE CHECKED! - " + detail);
		}
	}
};


class ModuleU3Cloaking : public Module
{
 private:
	
 	U3CloakUser* cu;
	Module* HashModule;

 public:
	ModuleU3Cloaking(InspIRCd* Me)
		: Module(Me)
	{
		ServerInstance->UseInterface("HashRequest");

		/* Attempt to locate the md5 service provider, bail if we can't find it */
		HashModule = ServerInstance->FindModule("m_md5.so");
		if (!HashModule)
			throw ModuleException("Can't find m_md5.so. Please load m_md5.so before m_cloaking.so.");

		/* Create new mode handler object */
		cu = new U3CloakUser(ServerInstance, this, HashModule);

		/* Register it with the core */		
		if (!ServerInstance->AddMode(cu, 'x'))
			throw ModuleException("Could not add new modes!");

		OnRehash(NULL,"");
	}
	
	virtual ~ModuleU3Cloaking()
	{
		ServerInstance->Modes->DelMode(cu);
		DELETE(cu);
		ServerInstance->DoneWithInterface("HashRequest");
	}
	
	virtual Version GetVersion()
	{
		// returns the version number of the module to be
		// listed in /MODULES
		return Version(4,0,0,0,VF_COMMON|VF_VENDOR,API_VERSION);
	}

	virtual void OnRehash(userrec* user, const std::string &parameter)
	{
		cu->DoRehash();
	}

	void Implements(char* List)
	{
		List[I_OnRehash] = 1;
	}
};

MODULE_INIT(ModuleU3Cloaking)
