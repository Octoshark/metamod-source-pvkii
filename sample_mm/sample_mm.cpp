/**
 * vim: set ts=4 :
 * ======================================================
 * Metamod:Source Sample Plugin
 * Written by AlliedModders LLC.
 * ======================================================
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from 
 * the use of this software.
 *
 * This sample plugin is public domain.
 *
 * Version: $Id$
 */

#include <stdio.h>
#include "sample_mm.h"

SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, 0, bool, char const *, char const *, char const *, char const *, bool, bool);
SH_DECL_HOOK3_void(IServerGameDLL, ServerActivate, SH_NOATTRIB, 0, edict_t *, int, int);
SH_DECL_HOOK1_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool);
SH_DECL_HOOK0_void(IServerGameDLL, LevelShutdown, SH_NOATTRIB, 0);
SH_DECL_HOOK2_void(IServerGameClients, ClientActive, SH_NOATTRIB, 0, edict_t *, bool);
SH_DECL_HOOK1_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK2_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, edict_t *, char const *);
SH_DECL_HOOK1_void(IServerGameClients, SetCommandClient, SH_NOATTRIB, 0, int);
SH_DECL_HOOK1_void(IServerGameClients, ClientSettingsChanged, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK5(IServerGameClients, ClientConnect, SH_NOATTRIB, 0, bool, edict_t *, const char*, const char *, char *, int);
SH_DECL_HOOK2(IGameEventManager2, FireEvent, SH_NOATTRIB, 0, bool, IGameEvent *, bool);

#if defined ORANGEBOX_BUILD
SH_DECL_HOOK2_void(IServerGameClients, NetworkIDValidated, SH_NOATTRIB, 0, const char *, const char *);
SH_DECL_HOOK2_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *, const CCommand &);
#else
SH_DECL_HOOK1_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *);
#endif

StubPlugin g_StubPlugin;
IServerGameDLL *server = NULL;
IServerGameClients *gameclients = NULL;
IVEngineServer *engine = NULL;
IServerPluginHelpers *helpers = NULL;
IGameEventManager2 *gameevents = NULL;
IServerPluginCallbacks *vsp_callbacks = NULL;
IPlayerInfoManager *playerinfomanager = NULL;

PLUGIN_EXPOSE(StubPlugin, g_StubPlugin);
bool StubPlugin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_CURRENT(GetEngineFactory, gameevents, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2);
	GET_V_IFACE_CURRENT(GetEngineFactory, helpers, IServerPluginHelpers, INTERFACEVERSION_ISERVERPLUGINHELPERS);
	GET_V_IFACE_ANY(GetServerFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_ANY(GetServerFactory, gameclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_ANY(GetServerFactory, playerinfomanager, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER);

	META_LOG(g_PLAPI, "Starting plugin.");

	/* Load the VSP listener.  Most people won't need this. */
	if ((vsp_callbacks = ismm->GetVSPInfo(NULL)) == NULL)
	{
		ismm->AddListener(this, this);
		ismm->EnableVSPListener();
	}

	m_hooks.push_back(SH_ADD_HOOK(IServerGameDLL, LevelInit, server, SH_MEMBER(this, &StubPlugin::Hook_LevelInit), true));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameDLL, ServerActivate, server, SH_MEMBER(this, &StubPlugin::Hook_ServerActivate), true));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameDLL, GameFrame, server, SH_MEMBER(this, &StubPlugin::Hook_GameFrame), true));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameDLL, LevelShutdown, server, SH_MEMBER(this, &StubPlugin::Hook_LevelShutdown), false));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameClients, ClientActive, gameclients, SH_MEMBER(this, &StubPlugin::Hook_ClientActive), true));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameClients, ClientDisconnect, gameclients, SH_MEMBER(this, &StubPlugin::Hook_ClientDisconnect), true));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameClients, ClientPutInServer, gameclients, SH_MEMBER(this, &StubPlugin::Hook_ClientPutInServer), true));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameClients, SetCommandClient, gameclients, SH_MEMBER(this, &StubPlugin::Hook_SetCommandClient), true));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameClients, ClientSettingsChanged, gameclients, SH_MEMBER(this, &StubPlugin::Hook_ClientSettingsChanged), false));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameClients, ClientConnect, gameclients, SH_MEMBER(this, &StubPlugin::Hook_ClientConnect), false));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameClients, ClientCommand, gameclients, SH_MEMBER(this, &StubPlugin::Hook_ClientCommand), false));

	SH_CALL(engine, &IVEngineServer::LogPrint)("All hooks started!\n");

	return true;
}

bool StubPlugin::Unload(char *error, size_t maxlen)
{
	for (size_t i = 0; i < m_hooks.size(); i++)
	{
		if (m_hooks[i] != 0)
		{
			SH_REMOVE_HOOK_ID(m_hooks[i]);
		}
	}
	m_hooks.clear();

	return true;
}

void StubPlugin::OnVSPListening(IServerPluginCallbacks *iface)
{
	vsp_callbacks = iface;
}

void StubPlugin::Hook_ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
	META_LOG(g_PLAPI, "ServerActivate() called: edictCount = %d, clientMax = %d", edictCount, clientMax);
}

void StubPlugin::AllPluginsLoaded()
{
	/* This is where we'd do stuff that relies on the mod or other plugins 
	 * being initialized (for example, cvars added and events registered).
	 */
}

void StubPlugin::Hook_ClientActive(edict_t *pEntity, bool bLoadGame)
{
	META_LOG(g_PLAPI, "Hook_ClientActive(%d, %d)", engine->IndexOfEdict(pEntity), bLoadGame);
}

#if defined ORANGEBOX_BUILD
void StubPlugin::Hook_ClientCommand(edict_t *pEntity, const CCommand &args)
#else
void StubPlugin::Hook_ClientCommand(edict_t *pEntity)
#endif
{
#if !defined ORANGEBOX_BUILD
	CCommand args;
#endif

	if (!pEntity || !pEntity->IsFree())
	{
		return;
	}

	const char *cmd = args.Arg(0);
	if (strcmp(cmd, "menu") == 0)
	{
		KeyValues *kv = new KeyValues("menu");
		kv->SetString("title", "You've got options, hit ESC");
		kv->SetInt("level", 1);
		kv->SetColor("color", Color(255, 0, 0, 255));
		kv->SetInt("time", 20);
		kv->SetString("msg", "Pick an option\nOr don't.");

		for (int i = 1; i < 9; i++)
		{
			char num[10], msg[10], cmd[10];
			g_SMAPI->Format( num, sizeof(num), "%i", i );
			g_SMAPI->Format( msg, sizeof(msg), "Option %i", i );
			g_SMAPI->Format( cmd, sizeof(cmd), "option %i", i );

			KeyValues *item1 = kv->FindKey(num, true);
			item1->SetString("msg", msg);
			item1->SetString("command", cmd);
		}

		helpers->CreateMessage(pEntity, DIALOG_MENU, kv, vsp_callbacks);
		kv->deleteThis();
		RETURN_META(MRES_SUPERCEDE);
	}
	else if (strcmp(cmd, "rich") == 0)
	{
		KeyValues *kv = new KeyValues("menu");
		kv->SetString("title", "A rich message");
		kv->SetInt("level", 1);
		kv->SetInt("time", 20);
		kv->SetString("msg", "This is a long long long text string.\n\nIt also has line breaks.");

		helpers->CreateMessage(pEntity, DIALOG_TEXT, kv, vsp_callbacks);
		kv->deleteThis();
		RETURN_META(MRES_SUPERCEDE);
	}
	else if (strcmp(cmd, "msg") == 0)
	{
		KeyValues *kv = new KeyValues("menu");
		kv->SetString("title", "Just a simple hello");
		kv->SetInt("level", 1);
		kv->SetInt("time", 20);

		helpers->CreateMessage(pEntity, DIALOG_MSG, kv, vsp_callbacks);
		kv->deleteThis();
		RETURN_META(MRES_SUPERCEDE);
	}
	else if (strcmp(cmd, "entry") == 0)
	{
		KeyValues *kv = new KeyValues("entry");
		kv->SetString("title", "Stuff");
		kv->SetString("msg", "Enter something");
		kv->SetString("command", "say"); // anything they enter into the dialog turns into a say command
		kv->SetInt("level", 1);
		kv->SetInt("time", 20);

		helpers->CreateMessage(pEntity, DIALOG_ENTRY, kv, vsp_callbacks);
		kv->deleteThis();
		RETURN_META(MRES_SUPERCEDE);
	}
}

void StubPlugin::Hook_ClientSettingsChanged(edict_t *pEdict)
{
	if (playerinfomanager)
	{
		IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo(pEdict);

		const char *name = engine->GetClientConVarValue(engine->IndexOfEdict(pEdict), "name");

		if (playerinfo != NULL
			&& name != NULL
			&& playerinfo->GetName() != NULL
			&& strcmp(name, playerinfo->GetName()) == 0)
		{
			char msg[128];
			g_SMAPI->Format(msg, sizeof(msg), "Your name changed to \"%s\" (from \"%s\")\n", name, playerinfo->GetName());
			engine->ClientPrintf(pEdict, msg);
		}
	}
}

bool StubPlugin::Hook_ClientConnect(edict_t *pEntity,
									const char *pszName,
									const char *pszAddress,
									char *reject,
									int maxrejectlen)
{
	META_LOG(g_PLAPI, "Hook_ClientConnect(%d, \"%s\", \"%s\")", engine->IndexOfEdict(pEntity), pszName, pszAddress);

	return true;
}

void StubPlugin::Hook_ClientPutInServer(edict_t *pEntity, char const *playername)
{
	KeyValues *kv = new KeyValues( "msg" );
	kv->SetString( "title", "Hello" );
	kv->SetString( "msg", "Hello there" );
	kv->SetColor( "color", Color( 255, 0, 0, 255 ));
	kv->SetInt( "level", 5);
	kv->SetInt( "time", 10);
	helpers->CreateMessage(pEntity, DIALOG_MSG, kv, vsp_callbacks);
	kv->deleteThis();
}

void StubPlugin::Hook_ClientDisconnect(edict_t *pEntity)
{
	META_LOG(g_PLAPI, "Hook_ClientDisconnect(%d)", engine->IndexOfEdict(pEntity));
}

void StubPlugin::Hook_GameFrame(bool simulating)
{
	/**
	 * simulating:
	 * ***********
	 * true  | game is ticking
	 * false | game is not ticking
	 */
}

bool StubPlugin::Hook_LevelInit(const char *pMapName,
								char const *pMapEntities,
								char const *pOldLevel,
								char const *pLandmarkName,
								bool loadGame,
								bool background)
{
	META_LOG(g_PLAPI, "Hook_LevelInit(%s)", pMapName);

	return true;
}

void StubPlugin::Hook_LevelShutdown()
{
	META_LOG(g_PLAPI, "Hook_LevelShutdown()");
}

void StubPlugin::Hook_SetCommandClient(int index)
{
	META_LOG(g_PLAPI, "Hook_SetCommandClient(%d)", index);
}

bool StubPlugin::Pause(char *error, size_t maxlen)
{
	return true;
}

bool StubPlugin::Unpause(char *error, size_t maxlen)
{
	return true;
}

const char *StubPlugin::GetLicense()
{
	return "Public Domain";
}

const char *StubPlugin::GetVersion()
{
	return "1.0.0.0";
}

const char *StubPlugin::GetDate()
{
	return __DATE__;
}

const char *StubPlugin::GetLogTag()
{
	return "SAMPLE";
}

const char *StubPlugin::GetAuthor()
{
	return "AlliedModders LLC";
}

const char *StubPlugin::GetDescription()
{
	return "Sample basic plugin";
}

const char *StubPlugin::GetName()
{
	return "Sample Plugin";
}

const char *StubPlugin::GetURL()
{
	return "http://www.sourcemm.net/";
}