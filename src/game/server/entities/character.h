#ifndef GAME_SERVER_ENTITIES_CHARACTER_H
#define GAME_SERVER_ENTITIES_CHARACTER_H

#include <game/server/entity.h>
#include <game/generated/server_data.h>
#include <game/generated/protocol.h>

#include <game/gamecore.h>

enum
{
	WEAPON_GAME = -3, // team switching etc
	WEAPON_SELF = -2, // console kill command
	WEAPON_WORLD = -1, // death tiles etc

    FTUNE_NOMOVE = 1, // All 3 used by "freeze"; flag "0" is used on connection
    FTUNE_NOHOOK = 2,
    FTUNE_NOJUMP = 4,
    FTUNE_NOCOLL = 8,
    FTUNE_CANTHOOK = 16,

    FLAG_NOKILLMSG = 1,
    FLAG_ENDDUEL = 2,

    INDFLAG_PASSIVE = 1,
    INDFLAG_HAT = 2,
    INDFLAG_FREEZEHIDE = 4,
    INDFLAG_HAMMER = 8,
    INDFLAG_GUN = 16,
    INDFLAG_SHOTGUN = 32,
    INDFLAG_GRENADE = 64,
    INDFLAG_LASER = 128,
    INDFLAG_NINJA = 256,

};

static const char *aStoreNames[] = {
	"Weapons Kit",
	"Passive Mode 2 Hours",
	"Deathnote Page",
	"VIP+ Room access",
};

static const int aStorePrices[] = {
	25,
	30,
	15,
	50,
};

class CCharacter : public CEntity
{
	MACRO_ALLOC_POOL_ID()

public:
	//character's size
	static const int ms_PhysSize = 28;

	CCharacter(CGameWorld *pWorld);

	virtual void Reset();
	virtual void Destroy();
	virtual void Tick();
	virtual void TickDefered();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

	bool IsGrounded();
	bool IsFrozen();

	void SetWeapon(int W);
	void HandleWeaponSwitch();
	void DoWeaponSwitch();
	void HandleZones();

	void HandleWeapons();
	void HandleNinja();

	void HandleHouse();

	void OnPredictedInput(CNetObj_PlayerInput *pNewInput);
	void OnDirectInput(CNetObj_PlayerInput *pNewInput);
	void ResetInput();
	void FireWeapon();

	void Die(int Killer, int Weapon, int Flags = 0);
	bool TakeDamage(vec2 Force, int From, int Weapon);

	bool Spawn(class CPlayer *pPlayer, vec2 Pos);
	bool Remove();

	bool GiveWeapon(int Weapon, int Ammo);
	bool TakeWeapon(int Weapon);
	void GiveNinja();

	void Freeze(int Length = 0);

	void SetEmote(int Emote, int Tick);

	void TeleCursor();

	bool OnVote(int Vote);

	bool IsAlive() const { return m_Alive; }
	class CPlayer *GetPlayer() { return m_pPlayer; }
	class CCharacterCore& GetCore() { return m_Core; }
	int GetVTeam() { return m_Core.m_VTeam; }
	void SetVTeam(int VTeam) { m_Core.m_VTeam = VTeam; }

	int m_FreezeEnd;
	int m_FreezeStart;
	bool m_DeepFrozen;

	int m_PassiveTicks;
	vec2 m_ChrViewPos;

	bool m_Indicator;
	bool m_Hat;
	bool m_SentFlags;

	int m_EmoteType;
	int m_EmoteStop;

	int m_RaceTick;

	int m_House;

private:
	// player controlling this character
	class CPlayer *m_pPlayer;

	int m_StoreItem;

	bool m_Alive;

	// weapon info
	CEntity *m_apHitObjects[10];
	int m_NumObjectsHit;

	struct WeaponStat
	{
		int m_AmmoRegenStart;
		int m_Ammo;
		int m_Ammocost;
		bool m_Got;

	} m_aWeapons[NUM_WEAPONS];

	int m_ActiveWeapon;
	int m_LastWeapon;
	int m_QueuedWeapon;

	int m_ReloadTimer;
	int m_AttackTick;

	int m_DamageTaken;

	// last tick that the player took any action ie some input
	int m_LastAction;
	int m_LastNoAmmoSound;

	// these are non-heldback inputs
	CNetObj_PlayerInput m_LatestPrevInput;
	CNetObj_PlayerInput m_LatestInput;

	// input
	CNetObj_PlayerInput m_PrevInput;
	CNetObj_PlayerInput m_Input;
	int m_NumInputs;
	int m_Jumped;

	int m_DamageTakenTick;

	// int m_Health;
	// int m_Armor;

	int m_CheckPoint;

	// ninja
	struct
	{
		vec2 m_ActivationDir;
		int m_ActivationTick;
		int m_CurrentMoveTime;
		int m_OldVelAmount;
	} m_Ninja;

	// the player core for the physics
	CCharacterCore m_Core;

	// info for dead reckoning
	int m_ReckoningTick; // tick that we are performing dead reckoning From
	CCharacterCore m_SendCore; // core that we should send
	CCharacterCore m_ReckoningCore; // the dead reckoning core

};

#endif
