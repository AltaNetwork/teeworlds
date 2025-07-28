

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
	enum
	{
		EVENT_NONE=0,
		EVENT_DUEL,
		EVENT_TOURNAMENT,

		COSM_RAINBOW=1,
		COSM_RAINBOWFEET=2,
		COSM_PULSEREDFEET=4,
		COSM_RANDOMSKIN=8,
		COSM_RANDOMSKINCOALA=16,
		COSM_RANDOMSKINSANTA=32,
		COSM_RANDOMSKINKITTY=64,

		SETTINGS_PREDICTVANILLA=16,
		SETTINGS_BEYONDZOOM=32,

		MENU_VOTES=1,
		MENU_MAIN=2,
		MENU_SETTINGS=4,
	};
	void DuelTick();

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

	// TODO: clean this up
	struct
	{
		char m_SkinName[64];
		int m_UseCustomColor;
		int m_ColorBody;
		int m_ColorFeet;
	} m_TeeInfos;

	// int m_RespawnTick;
	int	m_SpawnTeam;
	int m_DuelPlayer;
	int m_DuelScore;
	int m_InvitedBy;

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

// static const char *aSkins[] = {
// 	"bluekitty",
// 	"bluestripe",
// 	"brownbear",
// 	"cammo",
// 	"cammostripes",
// 	"coala",
// 	"default",
// 	"limekitty",
// 	"pinky",
// 	"redbopp",
// 	"redstripe",
// 	"saddo",
// 	"toptri",
// 	"twinbop",
// 	"twintri",
// 	"warpaint"
// };

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
