/* (c) heinrich5991 */

#ifndef GAME_SERVER_GAMEMODES_BOMB_H
#define GAME_SERVER_GAMEMODES_BOMB_H

#include <engine/shared/protocol.h>
#include <game/server/gamecontroller.h>

class CGameControllerBOMB : public IGameController
{
public: CGameControllerBOMB(class CGameContext *pGameServer); 
	virtual bool IsBomb() const { return true; }

	void MakeBomb(int ClientID);
	void MakeRandomBomb();

	void StartBombRound();
	void EndBombRound();

	virtual void DoWincheck();

	virtual void PostReset();
	virtual void Tick();

	// virtual bool CanJoinTeam(int Team, int NotThisID);
	// virtual void OnPlayerInfoChange(class CPlayer *pPlayer);
	virtual void OnCharacterSpawn(class CCharacter *pChr);
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon, int Flags = 0);

	struct
	{
		int m_ClientID;
		int m_Tick;
	} m_Bomb;

	bool m_Running;
	int m_BombEndTick;
};

#endif
