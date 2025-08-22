

#ifndef GAME_SERVER_PLAYER_H
#define GAME_SERVER_PLAYER_H

// this include should perhaps be removed
#include "entities/character.h"
#include "gamecontext.h"
#include <algorithm>

// player object
class CPlayer
{
	MACRO_ALLOC_POOL_ID()

public:
	CPlayer(CGameContext *pGameServer, int ClientID, int Team);
	~CPlayer();

	void Init(int CID);

	void TryRespawn();
	void Respawn();
	void SetTeam(int Team, bool DoChatMsg = true, bool KillChr = true);
	int GetTeam() const { return m_Team; };
	int GetCID() const { return m_ClientID; };

	void Tick();
	void PostTick();
	void Snap(int SnappingClient);

	void FakeSnap(int SnappingClient);

	void OnDirectInput(CNetObj_PlayerInput *NewInput);
	void OnPredictedInput(CNetObj_PlayerInput *NewInput);
	void OnDisconnect(const char *pReason);

	void KillCharacter(int Weapon = WEAPON_GAME, int Flags = 0);
	CCharacter *GetCharacter();

	const char* GetLanguage();
	void SetLanguage(const char* pLanguage);

	int PlayerEvent(); // Returns

	bool OnVote(int Vote);
	bool OnCallVote(const char* pVote, const char* pReason);
	void SendVoteMenu();

	bool m_ToSendVoteMenu;
	// int m_VoteMenu;

	enum
	{
		EVENT_NONE=0,

		COSM_RAINBOW=1,
		COSM_RAINBOWFEET=2,
		COSM_RANDOMSKIN=4,
		COSM_STARTRAIL=8,
		COSM_STARGLOW=16,
		COSM_LOVELY=32,
		COSM_BLOODY=64,

		SETTINGS_PREDICTVANILLA=1,
		SETTINGS_BEYONDZOOM=2,
		SETTINGS_TRANSPARENTPASSIVE=4,

		LMB_STANDBY=0,
		LMB_REG,
		LMB_PLAYING,
		LMB_WON,

		DUEL_ACCEPT = 1,
		DUEL_DENY = 2,
		DUEL_INDUEL = 4,
		DUEL_LEAVE = 8,
		DUEL_DIED = 16,
		DUEL_INVITED = 32,

		EFFECT_BLIND = 1,
		EFFECT_FORCENATURALPREDICTION = 2,

		INVENTORY_VIP = 1,
	};

	//---------------------------------------------------------
	// this is used for snapping so we know how we can clip the view for the player
	vec2 m_ViewPos;

	// states if the client is chatting, accessing a menu etc.
	int m_PlayerFlags;

	// used for snapping to just update latency if the scoreboard is active
	int m_aActLatency[MAX_CLIENTS];

	// used for spectator mode
	int m_SpectatorID;

	bool m_IsReady;
	bool m_Mute = false; // muted on entering

	int m_LMBState;
	int m_DuelFlags;

	int m_Effects;

	int m_SpawnVTeam;
	//
	int m_Vote;
	int m_VotePos;
	//
	int m_LastVoteCall;
	int m_LastVoteTry;
	int m_LastChat;
	int m_LastSetTeam;
	int m_LastSetSpectatorMode;
	int m_LastChangeInfo;
	int m_LastEmote;
	int m_LastKill;

	// ITEMS

	int m_DeathNotes;
	int m_LastDeathNote;

	int m_Inventory;
	int m_Hat;
	int m_GunDesign;

	vec2 m_SavePos;

	// ITEMS END

	struct
	{
		// Main
		int m_UserID;
		char m_Username[32];
		char m_Password[32];
		char m_Clan[32];
		int m_ClanStatus;
		// char m_RconPassword[32];

		// basic
		unsigned long long m_BPWager;
		int m_Health;
		int m_Armor;
		int m_Elo;
		int m_IsConnected;

		// levels
		unsigned int m_Level;
		unsigned long long int m_eXPerience;
		unsigned int m_LvlWeapon[5];
		unsigned int m_ExpWeapon[5];

		// Rank
		int m_Donor;
		int m_VIP;

		// Event
		long long m_Bounty;

		// Punishments
		int m_Arrested;

		// Player
		int m_WeaponsKit;
		int m_HealthRegen;
		int m_InfinityAmmo;
		int m_InfinityJumps;
		int m_FastReload;
		int m_NoSelfDMG;
		int m_Portal;

		// Weapons
		int m_GrenadeSpread;
		int m_GrenadeBounce;
		int m_GrenadeMine;
		int m_FlameThrower;

		int m_ShotgunSpread;
		int m_ShotgunExplode;
		int m_ShotgunStars;

		int m_RifleSpread;
		int m_RifleSwap;
		int m_RiflePlasma;

		int m_GunSpread;
		int m_GunExplode;
		int m_GunFreeze;

		int m_HammerWalls;
		int m_HammerShot;
		int m_HammerKill;
		int m_HammerExplode;

		int m_NinjaPermanent;
		int m_NinjaStart;
		int m_NinjaSwitch;
		int m_NinjaFly;
		int m_NinjaBomber;

		int m_EndlessHook;
		int m_HealHook;
		int m_BoostHook;

		int m_PushAura;
		int m_PullAura;
	} m_AccData;

	// TODO: clean this up
	struct
	{
		char m_SkinName[64];
		int m_UseCustomColor;
		int m_ColorBody;
		int m_ColorFeet;
	} m_TeeInfos;

	int	m_SpawnTeam;

	int m_Cosmetics;
	int m_Settings;
	int m_WTeam;

	int m_DieTick;
	int m_Score;
	int m_ScoreStartTick;
	bool m_ForceBalanced;
	int m_LastActionTick;
	int m_TeamChangeTick;
	struct
	{
		int m_TargetX;
		int m_TargetY;
	} m_LatestActivity;

	// network latency calculations
	struct
	{
		int m_Accum;
		int m_AccumMin;
		int m_AccumMax;
		int m_Avg;
		int m_Min;
		int m_Max;
	} m_Latency;

	int m_Authed;

private:
	CCharacter *m_pCharacter;
	CGameContext *m_pGameServer;

	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const;

	bool m_Spawning;
	int m_ClientID;
	int m_Team;

	char m_aLanguage[16];

	private:
	CTuningParams m_PrevTuningParams;
	CTuningParams m_NextTuningParams;

	void HandleTuningParams(); //This function will send the new parameters if needed

	const char* ProccessName() const;
	const char* ProccessClan() const;
	const char* ProccessSkin() const;

public:
	CTuningParams* GetNextTuningParams() { return &m_NextTuningParams; };
};

[[maybe_unused]]
static const char *aSkins[] = {
	"bluekitty",
	"bluestripe",
	"brownbear",
	"cammo",
	"cammostripes",
	"coala",
	"default",
	"limekitty",
	"pinky",
	"redbopp",
	"redstripe",
	"saddo",
	"toptri",
	"twinbop",
	"twintri",
	"warpaint"
};

// static const char *aSkinsCoala[] = {
// 	"coala_bluekitty",
// 	"coala_bluestripe",
// 	"coala_cammo",
// 	"coala_cammostripes",
// 	"coala",
// 	"coala_default",
// 	"coala_limekitty",
// 	"coala_pinky",
// 	"coala_redbopp",
// 	"coala_redstripe",
// 	"coala_saddo",
// 	"coala_toptri",
// 	"coala_twinbop",
// 	"coala_twintri",
// 	"coala_warpaint"
// };

// static const char *aSkinsSanta[] = {
// 	"santa_bluekitty",
// 	"santa_bluestripe",
// 	"santa_brownbear",
// 	"santa_cammo",
// 	"santa_cammostripes",
// 	"santa_coala",
// 	"santa_default",
// 	"santa_limekitty",
// 	"santa_pinky",
// 	"santa_redbopp",
// 	"santa_redstripe",
// 	"santa_saddo",
// 	"santa_toptri",
// 	"santa_twinbop",
// 	"santa_twintri",
// 	"santa_warpaint"
// };

// static const char *aSkinsKitty[] = {
// 	"bluekitty",
// 	"kitty_bluestripe",
// 	"kitty_brownbear",
// 	"kitty_cammo",
// 	"kitty_cammostripes",
// 	"kitty_coala",
// 	"kitty_default",
// 	"limekitty",
// 	"kitty_pinky",
// 	"kitty_redbopp",
// 	"kitty_redstripe",
// 	"kitty_saddo",
// 	"kitty_toptri",
// 	"kitty_twinbop",
// 	"kitty_twintri",
// 	"kitty_warpaint"
// };




#endif
