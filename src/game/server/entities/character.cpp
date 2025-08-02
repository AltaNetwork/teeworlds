#include <algorithm>
#include <new>
#include <optional>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/mapitems.h>

#include "character.h"
#include "laser.h"
#include "pickup.h"
#include "special/indicator.h"
#include "special/soul.h"
#include "special/hat.h"
#include "projectile.h"

//input count
struct CInputCount
{
	int m_Presses;
	int m_Releases;
};

CInputCount CountInput(int Prev, int Cur)
{
	CInputCount c = {0, 0};
	Prev &= INPUT_STATE_MASK;
	Cur &= INPUT_STATE_MASK;
	int i = Prev;

	while(i != Cur)
	{
		i = (i+1)&INPUT_STATE_MASK;
		if(i&1)
			c.m_Presses++;
		else
			c.m_Releases++;
	}

	return c;
}


MACRO_ALLOC_POOL_ID_IMPL(CCharacter, MAX_CLIENTS)

// Character, "physical" player's part
CCharacter::CCharacter(CGameWorld *pWorld)
: CEntity(pWorld, CGameWorld::ENTTYPE_CHARACTER)
{
	m_ProximityRadius = ms_PhysSize;
}

void CCharacter::Reset()
{
	Destroy();
}

bool CCharacter::Spawn(CPlayer *pPlayer, vec2 Pos)
{
	m_EmoteStop = -1;
	m_LastAction = -1;
	m_LastNoAmmoSound = -1;
	m_ActiveWeapon = WEAPON_GUN;
	m_LastWeapon = WEAPON_HAMMER;
	m_QueuedWeapon = -1;
	if(pPlayer->PlayerEvent() == CPlayer::EVENT_NONE)
	    m_PassiveTicks = SERVER_TICK_SPEED * g_Config.m_SvSpawnPassive;

	m_pPlayer = pPlayer;
	m_Pos = Pos;
	m_ChrViewPos = m_Pos;

	m_Core.Reset();
	m_Core.Init(&GameServer()->m_World.m_Core, GameServer()->Collision());
	m_Core.m_Pos = m_Pos;
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = &m_Core;

	m_ReckoningTick = 0;
	mem_zero(&m_SendCore, sizeof(m_SendCore));
	mem_zero(&m_ReckoningCore, sizeof(m_ReckoningCore));

	GameServer()->m_World.InsertEntity(this);
	m_Alive = true;
	m_CheckPoint = 0;

	GameServer()->m_pController->OnCharacterSpawn(this);

	m_Indicator = false;
	m_Hat = false;
	m_SentFlags = false;

	m_FreezeStart = 0;
	m_FreezeEnd = 0;
	m_DeepFrozen = false;

	m_RaceTick = -1;

	if(pPlayer->PlayerEvent() == CPlayer::EVENT_DUEL)
	{
        m_Core.m_VTeam = GameServer()->m_pController->VTeamDuel(m_pPlayer->GetCID(),m_pPlayer->m_DuelPlayer);
        Freeze(3);
	}

	GameServer()->CreatePlayerSpawn(m_Pos, m_Core.m_VTeam);

	return true;
}

void CCharacter::Destroy()
{
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	m_Alive = false;
}

void CCharacter::SetWeapon(int W)
{
	if(W == m_ActiveWeapon)
		return;
	m_LastWeapon = m_ActiveWeapon;
	m_QueuedWeapon = -1;
	m_ActiveWeapon = W;
	GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SWITCH);
	if(m_ActiveWeapon < 0 || m_ActiveWeapon >= NUM_WEAPONS)
		m_ActiveWeapon = 0;
}

bool CCharacter::IsGrounded() {
	if(GameServer()->Collision()->CheckPoint(m_Pos.x+m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5)) return true;
	if(GameServer()->Collision()->CheckPoint(m_Pos.x-m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5)) return true;
	return false;
}

bool CCharacter::IsFrozen() {
    if(m_FreezeEnd > Server()->Tick())
        return true;
    return false;
}

void CCharacter::HandleNinja()
{
	if(m_ActiveWeapon != WEAPON_NINJA)
		return;

	if ((Server()->Tick() - m_Ninja.m_ActivationTick) > (g_pData->m_Weapons.m_Ninja.m_Duration * Server()->TickSpeed() / 1000))
	{
		// time's up, return
		m_aWeapons[WEAPON_NINJA].m_Got = false;
		m_ActiveWeapon = m_LastWeapon;

		SetWeapon(m_ActiveWeapon);
		return;
	}

	// force ninja Weapon
	SetWeapon(WEAPON_NINJA);

	m_Ninja.m_CurrentMoveTime--;

	if (m_Ninja.m_CurrentMoveTime == 0)
	{
		// reset velocity
		m_Core.m_Vel = m_Ninja.m_ActivationDir*m_Ninja.m_OldVelAmount;
	}

	if (m_Ninja.m_CurrentMoveTime > 0)
	{
		// Set velocity
		m_Core.m_Vel = m_Ninja.m_ActivationDir * g_pData->m_Weapons.m_Ninja.m_Velocity;
		vec2 OldPos = m_Pos;
		GameServer()->Collision()->MoveBox(&m_Core.m_Pos, &m_Core.m_Vel, vec2(m_ProximityRadius, m_ProximityRadius), 0.f);

		// reset velocity so the client doesn't predict stuff
		m_Core.m_Vel = vec2(0.f, 0.f);

		// check if we Hit anything along the way
		{
			CCharacter *aEnts[MAX_CLIENTS];
			vec2 Dir = m_Pos - OldPos;
			float Radius = m_ProximityRadius * 2.0f;
			vec2 Center = OldPos + Dir * 0.5f;
			int Num = GameServer()->m_World.FindEntities(Center, Radius, (CEntity**)aEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

			for (int i = 0; i < Num; ++i)
			{
				if (aEnts[i] == this)
					continue;

				// make sure we haven't Hit this object before
				bool bAlreadyHit = false;
				for (int j = 0; j < m_NumObjectsHit; j++)
				{
					if (m_apHitObjects[j] == aEnts[i])
						bAlreadyHit = true;
				}
				if (bAlreadyHit)
					continue;

				// check so we are sufficiently close
				if (distance(aEnts[i]->m_Pos, m_Pos) > (m_ProximityRadius * 2.0f))
					continue;

				// Hit a player, give him damage and stuffs...
				GameServer()->CreateSound(aEnts[i]->m_Pos, SOUND_NINJA_HIT);
				// set his velocity to fast upward (for now)
				if(m_NumObjectsHit < 10)
					m_apHitObjects[m_NumObjectsHit++] = aEnts[i];

				aEnts[i]->TakeDamage(vec2(0, -10.0f), m_pPlayer->GetCID(), WEAPON_NINJA);
			}
		}

		return;
	}

	return;
}


void CCharacter::DoWeaponSwitch()
{
	// make sure we can switch
	if(m_ReloadTimer != 0 || m_QueuedWeapon == -1 || m_aWeapons[WEAPON_NINJA].m_Got)
		return;

	// switch Weapon
	SetWeapon(m_QueuedWeapon);
}

void CCharacter::HandleWeaponSwitch()
{
	int WantedWeapon = m_ActiveWeapon;
	if(m_QueuedWeapon != -1)
		WantedWeapon = m_QueuedWeapon;

	// select Weapon
	int Next = CountInput(m_LatestPrevInput.m_NextWeapon, m_LatestInput.m_NextWeapon).m_Presses;
	int Prev = CountInput(m_LatestPrevInput.m_PrevWeapon, m_LatestInput.m_PrevWeapon).m_Presses;

	if(Next < 128) // make sure we only try sane stuff
	{
		while(Next) // Next Weapon selection
		{
			WantedWeapon = (WantedWeapon+1)%NUM_WEAPONS;
			if(m_aWeapons[WantedWeapon].m_Got)
				Next--;
		}
	}

	if(Prev < 128) // make sure we only try sane stuff
	{
		while(Prev) // Prev Weapon selection
		{
			WantedWeapon = (WantedWeapon-1)<0?NUM_WEAPONS-1:WantedWeapon-1;
			if(m_aWeapons[WantedWeapon].m_Got)
				Prev--;
		}
	}

	// Direct Weapon selection
	if(m_LatestInput.m_WantedWeapon)
		WantedWeapon = m_Input.m_WantedWeapon-1;
	// char aBuf[256];
	// str_format(aBuf, sizeof(aBuf), "                         \nINPUT: %d\nTICK:%d",m_LatestInput.m_WantedWeapon ,Server()->Tick(), aBuf);
	// GameServer()->SendBroadcast(aBuf, m_pPlayer->GetCID());

	// check for insane values
	if(WantedWeapon >= 0 && WantedWeapon < NUM_WEAPONS && WantedWeapon != m_ActiveWeapon && m_aWeapons[WantedWeapon].m_Got)
		m_QueuedWeapon = WantedWeapon;

	DoWeaponSwitch();
}

void CCharacter::FireWeapon()
{
	if(m_ReloadTimer != 0 || IsFrozen())
		return;

	DoWeaponSwitch();
	vec2 Direction = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));

	bool FullAuto = false;
	if(m_ActiveWeapon == WEAPON_GRENADE || m_ActiveWeapon == WEAPON_SHOTGUN || m_ActiveWeapon == WEAPON_RIFLE)
		FullAuto = true;


	// check if we gonna fire
	bool WillFire = false;
	if(CountInput(m_LatestPrevInput.m_Fire, m_LatestInput.m_Fire).m_Presses)
		WillFire = true;

	if(FullAuto && (m_LatestInput.m_Fire&1) && m_aWeapons[m_ActiveWeapon].m_Ammo)
		WillFire = true;

	if(!WillFire)
		return;

	// check for ammo
	if(!m_aWeapons[m_ActiveWeapon].m_Ammo)
	{
		// 125ms is a magical limit of how fast a human can click
		m_ReloadTimer = 125 * Server()->TickSpeed() / 1000;
		if(m_LastNoAmmoSound+Server()->TickSpeed() <= Server()->Tick())
		{
			GameServer()->CreateSound(m_Pos, SOUND_WEAPON_NOAMMO);
			m_LastNoAmmoSound = Server()->Tick();
		}
		return;
	}

	vec2 ProjStartPos = m_Pos+Direction*m_ProximityRadius*0.75f;

	switch(m_ActiveWeapon)
	{
		case WEAPON_HAMMER:
		{
			// reset objects Hit
			m_NumObjectsHit = 0;
			GameServer()->CreateSound(m_Pos, SOUND_HAMMER_FIRE);

			CCharacter *apEnts[MAX_CLIENTS];
			int Hits = 0;
			int Num = GameServer()->m_World.FindEntities(ProjStartPos, m_ProximityRadius*0.5f, (CEntity**)apEnts,
														MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
			for (int i = 0; i < Num; ++i)    {
			    CCharacter *pTarget = apEnts[i];
				if ((pTarget == this) /*|| GameServer()->Collision()->IntersectLine(ProjStartPos, pTarget->m_Pos, NULL, NULL)*/ || (pTarget->GetCore().m_VTeam != m_Core.m_VTeam))
					continue;

				// set his velocity to fast upward (for now)
				if(length(pTarget->m_Pos-ProjStartPos) > 0.0f)
					GameServer()->CreateHammerHit(pTarget->m_Pos-normalize(pTarget->m_Pos-ProjStartPos)*m_ProximityRadius*0.5f, m_Core.m_VTeam);
				else
					GameServer()->CreateHammerHit(ProjStartPos, m_Core.m_VTeam);
				vec2 Dir;
				if (length(pTarget->m_Pos - m_Pos) > 0.0f)
					Dir = normalize(pTarget->m_Pos - m_Pos);
				else
					Dir = vec2(0.f, -1.f);
				pTarget->TakeDamage(vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f,
					m_pPlayer->GetCID(), m_ActiveWeapon);
				Hits++;
			}

			// if we Hit anything, we have to wait for the reload
			if(Hits)
				m_ReloadTimer = Server()->TickSpeed()/3;

		} break;

		case WEAPON_GUN:
		{
			new CProjectile(GameWorld(), WEAPON_GUN,
				m_pPlayer->GetCID(),
				ProjStartPos,
				Direction,
				(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GunLifetime),
				1, 0, 0, -1, WEAPON_GUN, m_Core.m_VTeam);
			GameServer()->CreateSound(m_Pos, SOUND_GUN_FIRE);
		} break;

		case WEAPON_SHOTGUN:
		{
			new CLaser(GameWorld(), m_Pos, Direction, GameServer()->Tuning()->m_LaserReach, m_pPlayer->GetCID(), m_Core.m_VTeam, 10);
			GameServer()->CreateSound(m_Pos, SOUND_RIFLE_FIRE);
		} break;

		case WEAPON_GRENADE:
		{
			new CProjectile(GameWorld(), WEAPON_GRENADE,
				m_pPlayer->GetCID(),
				ProjStartPos,
				Direction,
				(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeLifetime),
				1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE, m_Core.m_VTeam);
			GameServer()->CreateSound(m_Pos, SOUND_GRENADE_FIRE);
		} break;

		case WEAPON_RIFLE:
		{
			new CLaser(GameWorld(), m_Pos, Direction, GameServer()->Tuning()->m_LaserReach, m_pPlayer->GetCID(), m_Core.m_VTeam);
			GameServer()->CreateSound(m_Pos, SOUND_RIFLE_FIRE);
		} break;

		case WEAPON_NINJA:
		{
			// reset Hit objects
			m_NumObjectsHit = 0;

			m_Ninja.m_ActivationDir = Direction;
			m_Ninja.m_CurrentMoveTime = g_pData->m_Weapons.m_Ninja.m_Movetime * Server()->TickSpeed() / 1000;
			m_Ninja.m_OldVelAmount = length(m_Core.m_Vel);

			GameServer()->CreateSound(m_Pos, SOUND_NINJA_FIRE);
		} break;

	}

	m_AttackTick = Server()->Tick();

	if(m_aWeapons[m_ActiveWeapon].m_Ammo > 0) // -1 == unlimited
		m_aWeapons[m_ActiveWeapon].m_Ammo--;

	if(!m_ReloadTimer)
		m_ReloadTimer = g_pData->m_Weapons.m_aId[m_ActiveWeapon].m_Firedelay * Server()->TickSpeed() / 1000;
}

void CCharacter::HandleWeapons()
{
	//ninja
	HandleNinja();

	// check reload timer
	if(m_ReloadTimer)
	{
		m_ReloadTimer--;
		return;
	}

	// fire Weapon, if wanted
	FireWeapon();

	// ammo regen
	int AmmoRegenTime = g_pData->m_Weapons.m_aId[WEAPON_GUN].m_Ammoregentime;
	if(AmmoRegenTime && m_aWeapons[m_ActiveWeapon].m_Ammo != -1)
	{
		// If equipped and not active, regen ammo?
		if (m_ReloadTimer <= 0)
		{
			if (m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart < 0)
				m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart = Server()->Tick();

			if ((Server()->Tick() - m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart) >= AmmoRegenTime * Server()->TickSpeed() / 1000)
			{
				// Add some ammo
				m_aWeapons[m_ActiveWeapon].m_Ammo = min(m_aWeapons[m_ActiveWeapon].m_Ammo + 1, 10);
				m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart = -1;
			}
		}
		else
		{
			m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart = -1;
		}
	}

	return;
}

bool CCharacter::GiveWeapon(int Weapon, int Ammo)
{
	if(/*m_aWeapons[Weapon].m_Ammo < g_pData->m_Weapons.m_aId[Weapon].m_Maxammo ||*/ !m_aWeapons[Weapon].m_Got)
	{
		m_aWeapons[Weapon].m_Got = true;
		m_aWeapons[Weapon].m_Ammo = -1;//min(g_pData->m_Weapons.m_aId[Weapon].m_Maxammo, Ammo);
		return true;
	}
	return false;
}

bool CCharacter::TakeWeapon(int Weapon)
{
    if(m_aWeapons[Weapon].m_Got == false)
        return false;
    if(m_ActiveWeapon>WEAPON_GUN)
        SetWeapon(WEAPON_GUN);
    m_aWeapons[Weapon].m_Got = false;
    m_aWeapons[Weapon].m_Ammo = 0;
    return true;
}

void CCharacter::GiveNinja()
{
	m_Ninja.m_ActivationTick = Server()->Tick();
	m_aWeapons[WEAPON_NINJA].m_Got = true;
	m_aWeapons[WEAPON_NINJA].m_Ammo = -1;
	if (m_ActiveWeapon != WEAPON_NINJA)
		m_LastWeapon = m_ActiveWeapon;
	m_ActiveWeapon = WEAPON_NINJA;

	GameServer()->CreateSound(m_Pos, SOUND_PICKUP_NINJA);
}

void CCharacter::SetEmote(int Emote, int Tick)
{
	m_EmoteType = Emote;
	m_EmoteStop = Tick;
}

void CCharacter::TeleCursor()
{
    m_Core.m_Pos.x = m_Pos.x + m_Input.m_TargetX;
    m_Core.m_Pos.y = m_Pos.y + m_Input.m_TargetY;
    m_Core.m_Vel = vec2(0,0);
    Freeze(0);
}

void CCharacter::OnPredictedInput(CNetObj_PlayerInput *pNewInput)
{
	// check for changes
	if(mem_comp(&m_Input, pNewInput, sizeof(CNetObj_PlayerInput)) != 0)
		m_LastAction = Server()->Tick();

	// copy new input
	mem_copy(&m_Input, pNewInput, sizeof(m_Input));
	m_NumInputs++;

	// it is not allowed to aim in the center
	if(m_Input.m_TargetX == 0 && m_Input.m_TargetY == 0)
		m_Input.m_TargetY = -1;
}

void CCharacter::OnDirectInput(CNetObj_PlayerInput *pNewInput)
{
	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
	mem_copy(&m_LatestInput, pNewInput, sizeof(m_LatestInput));

	// it is not allowed to aim in the center
	if(m_LatestInput.m_TargetX == 0 && m_LatestInput.m_TargetY == 0)
		m_LatestInput.m_TargetY = -1;

	if(m_NumInputs > 2 && m_pPlayer->GetTeam() != TEAM_SPECTATORS)
	{
		HandleWeaponSwitch();
		FireWeapon();
	}

	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
}

void CCharacter::ResetInput()
{
	m_Input.m_Direction = 0;
	m_Input.m_Hook = 0;
	// simulate releasing the fire button
	if((m_Input.m_Fire&1) != 0)
		m_Input.m_Fire++;
	m_Input.m_Fire &= INPUT_STATE_MASK;
	m_Input.m_Jump = 0;
	m_LatestPrevInput = m_LatestInput = m_Input;
}

void CCharacter::Tick()
{
    m_WTeam = m_Core.m_VTeam;
	if(m_PassiveTicks != 0)
	{
    	if(m_PassiveTicks > 0)
            m_PassiveTicks--;
        if(m_PassiveTicks == (SERVER_TICK_SPEED*3)-1)
       	    GameServer()->SendChatTarget(m_pPlayer->GetCID(), _("Passive will be disabled in 3 secs"));
		if(m_PassiveTicks == 0)
		{
		    m_Core.m_VTeam = 0;
			m_SentFlags = false;
		}
		else
            m_Core.m_VTeam = -m_pPlayer->GetCID()-1;
	}

	if(m_Core.m_VTeam < 0 && !m_Indicator)
	{
    	new CIndicator(GameWorld(), m_Pos, m_pPlayer->GetCID(), INDFLAG_PASSIVE);
        m_Indicator = true;
	}

	if(!m_Hat)
	{
    	new CHat(GameWorld(), m_Pos, m_pPlayer->GetCID());
        m_Hat = true;
	}


	if(IsFrozen())
	{
		ResetInput();
		m_DeepFrozen = false;
	}

	m_Core.m_Input = m_Input;
	m_Core.Tick(true, m_pPlayer->GetNextTuningParams());

	// handle weapons and game zones
	HandleZones();
	HandleWeapons();

	if(m_Core.m_HookedPlayer != -1 && GameServer()->m_apPlayers[m_Core.m_HookedPlayer]->GetCharacter())
	{
    	GameServer()->m_apPlayers[m_Core.m_HookedPlayer]->GetCharacter()->GetCore().m_LastContact = m_pPlayer->GetCID();
        GameServer()->m_apPlayers[m_Core.m_HookedPlayer]->GetCharacter()->GetCore().m_LastContactTicks = SERVER_TICK_SPEED;
	}
    // Previnput
	m_PrevInput = m_Input;
	return;
}

void CCharacter::TickDefered()
{
	// advance the dummy
	{
		CWorldCore TempWorld;
		m_ReckoningCore.Init(&TempWorld, GameServer()->Collision());
		m_ReckoningCore.Tick(false, &TempWorld.m_Tuning);
		m_ReckoningCore.Move(&TempWorld.m_Tuning);
		m_ReckoningCore.Quantize();
		m_ReckoningCore.m_Jumped = 1;
	}

    //	lastsentcore
	m_Core.Move(m_pPlayer->GetNextTuningParams());
	m_Core.Quantize();
	m_Pos = m_Core.m_Pos;

	int Events = m_Core.m_TriggeredEvents;
	int Mask = CmaskAllExceptOne(m_pPlayer->GetCID());

	if(Events&COREEVENT_GROUND_JUMP) GameServer()->CreateSound(m_Pos, SOUND_PLAYER_JUMP, Mask);
	if(Events&COREEVENT_HOOK_ATTACH_PLAYER) GameServer()->CreateSound(m_Pos, SOUND_HOOK_ATTACH_PLAYER, CmaskAll());
	if(Events&COREEVENT_HOOK_ATTACH_GROUND) GameServer()->CreateSound(m_Pos, SOUND_HOOK_ATTACH_GROUND, Mask);
	if(Events&COREEVENT_HOOK_HIT_NOHOOK) GameServer()->CreateSound(m_Pos, SOUND_HOOK_NOATTACH, Mask);

	if(m_pPlayer->GetTeam() == TEAM_SPECTATORS)
	{
		m_ChrViewPos.x = m_Input.m_TargetX+m_Pos.x;
		m_ChrViewPos.y = m_Input.m_TargetY+m_Pos.y;
	} else m_ChrViewPos = m_Pos;

	// update the m_SendCore if needed
	{
		CNetObj_Character Predicted;
		CNetObj_Character Current;
		mem_zero(&Predicted, sizeof(Predicted));
		mem_zero(&Current, sizeof(Current));
		m_ReckoningCore.Write(&Predicted);
		m_Core.Write(&Current);

		// only allow dead reackoning for a top of 3 seconds
		if(m_ReckoningTick+Server()->TickSpeed()*3 < Server()->Tick() || mem_comp(&Predicted, &Current, sizeof(CNetObj_Character)) != 0)
		{
			m_ReckoningTick = Server()->Tick();
			m_SendCore = m_Core;
			m_ReckoningCore = m_Core;
		}
	}
}

void CCharacter::TickPaused()
{
	++m_AttackTick;
	++m_DamageTakenTick;
	++m_Ninja.m_ActivationTick;
	++m_ReckoningTick;
	if(m_LastAction != -1)
		++m_LastAction;
	if(m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart > -1)
		++m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart;
	if(m_EmoteStop > -1)
		++m_EmoteStop;
}

void CCharacter::Die(int Killer, int Weapon, int Flags)
{

    if(m_Core.m_LastContact != -1 && m_Core.m_LastContactTicks > 0)
           Killer = m_Core.m_LastContact;

    if(m_pPlayer->PlayerEvent() == CPlayer::EVENT_DUEL && Weapon != WEAPON_GAME)
	{
	    Killer = m_pPlayer->m_DuelPlayer;
        GameServer()->m_apPlayers[m_pPlayer->m_DuelPlayer]->KillCharacter(WEAPON_GAME, FLAG_NOKILLMSG);
	}

    if(Killer != m_pPlayer->GetCID())
        new CSoul(GameWorld(), m_Pos, Killer);

    int ModeSpecial = GameServer()->m_pController->OnCharacterDeath(this, GameServer()->m_apPlayers[Killer], Weapon, Flags);

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "kill killer='%d:%s' victim='%d:%s' weapon=%d special=%d, flags=%d",
		Killer, Server()->ClientName(Killer),
		m_pPlayer->GetCID(), Server()->ClientName(m_pPlayer->GetCID()), Weapon, ModeSpecial, Flags);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	// send the kill message
	if(~Flags&FLAG_NOKILLMSG)
	{
    	CNetMsg_Sv_KillMsg Msg;
    	Msg.m_Killer = Killer;
    	Msg.m_Victim = m_pPlayer->GetCID();
    	Msg.m_Weapon = Weapon;
    	Msg.m_ModeSpecial = ModeSpecial;
    	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
	}
	// a nice sound
	GameServer()->CreateSound(m_Pos, SOUND_PLAYER_DIE);

	// this is for auto respawn after 3 secs
	m_pPlayer->m_DieTick = Server()->Tick();

	m_Alive = false;
	GameServer()->m_World.RemoveEntity(this);
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	GameServer()->CreateDeath(m_Pos, m_pPlayer->GetCID(), m_Core.m_VTeam);
}

bool CCharacter::TakeDamage(vec2 Force, int From, int Weapon)
{
    if((GameServer()->m_apPlayers[From]->GetCharacter()->GetCore().m_VTeam != m_Core.m_VTeam))
        return false;

    m_Core.m_Vel += Force;

	if(Weapon == WEAPON_HAMMER || Weapon == WEAPON_RIFLE)
        Freeze(0);

	m_EmoteType = EMOTE_PAIN;
	m_EmoteStop = Server()->Tick() + 500 * Server()->TickSpeed() / 1000;

	return true;
}

void CCharacter::Snap(int SnappingClient)
{
	int Id = m_pPlayer->GetCID();

	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(Server()->SnapNewItem(NETOBJTYPE_CHARACTER, Id, sizeof(CNetObj_Character)));
	if(!pCharacter)
		return;

	// write down the m_Core
	if(!m_ReckoningTick || GameServer()->m_World.m_Paused)
	{
		// no dead reckoning when paused because the client doesn't know
		// how far to perform the reckoning
		pCharacter->m_Tick = 0;
		m_Core.Write(pCharacter);
	}
	else
	{
		pCharacter->m_Tick = m_ReckoningTick;
		m_SendCore.Write(pCharacter);
	}

	// set emote
	if (m_EmoteStop < Server()->Tick()) {
	    m_EmoteType = EMOTE_NORMAL;
		m_EmoteStop = -1;
	}

	pCharacter->m_Emote = m_EmoteType == EMOTE_NORMAL && IsFrozen() ? EMOTE_BLINK : m_EmoteType;
	pCharacter->m_Emote = ((pCharacter->m_Emote == EMOTE_NORMAL && (250 - ((Server()->Tick() - m_LastAction)%(250)) < 5)) || m_pPlayer->GetTeam() == TEAM_SPECTATORS) ? EMOTE_BLINK:pCharacter->m_Emote;
	pCharacter->m_PlayerFlags = GetPlayer()->m_PlayerFlags;
	pCharacter->m_Weapon = m_ActiveWeapon;
	pCharacter->m_AttackTick = m_AttackTick;
	pCharacter->m_Direction = m_Input.m_Direction;
	pCharacter->m_Health = 0;
	pCharacter->m_Armor = 0;
	pCharacter->m_AmmoCount = 10;

	// if(m_pPlayer->GetCID() == SnappingClient || SnappingClient == -1 || m_pPlayer->GetCID() == GameServer()->m_apPlayers[SnappingClient]->m_SpectatorID)

	CNetObj_DDNetCharacter *pDDNetCharacter = (CNetObj_DDNetCharacter *)Server()->SnapNewItem(32764, m_pPlayer->GetCID(), 40);
	if(!pDDNetCharacter)
		return;

	pDDNetCharacter->m_TargetX = m_LatestInput.m_TargetX;
	pDDNetCharacter->m_TargetY = m_LatestInput.m_TargetY;
	pDDNetCharacter->m_JumpedTotal = m_Core.m_AirJumps - m_Core.m_AirJumped;
	pDDNetCharacter->m_Jumps = m_Core.m_AirJumps + 1;

    if(IsFrozen())
    {
        pDDNetCharacter->m_FreezeStart = m_DeepFrozen ? -1 : m_FreezeStart;
        pDDNetCharacter->m_FreezeEnd = m_FreezeEnd+1;
    }

	if (m_aWeapons[0].m_Got)    pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_HAMMER;
	if (m_aWeapons[1].m_Got)    pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_GUN;
	if (m_aWeapons[2].m_Got)    pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_SHOTGUN;
	if (m_aWeapons[3].m_Got)    pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_GRENADE;
	if (m_aWeapons[4].m_Got)    pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_LASER;
	if (m_aWeapons[5].m_Got)    pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_NINJA;
	if (m_Core.m_VTeam < 0)     pDDNetCharacter->m_Flags |= CHARACTERFLAG_SOLO;
	pDDNetCharacter->m_NinjaActivationTick = m_Ninja.m_ActivationTick;

	// pDDNetCharacter->m_Flags |= CHARACTERFLAG_INVINCIBLE;
}

void CCharacter::Freeze(int Length)
{
    m_FreezeEnd = Server()->Tick() + Length * SERVER_TICK_SPEED;
    if(Length != 0)
    {
        m_FreezeStart = Server()->Tick();
        m_DeepFrozen = true;
    }

}

void CCharacter::HandleZones()
{
   	// handle death-tiles and leaving gamelayer
	if(GameServer()->Collision()->GetCollisionAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH || GameServer()->Collision()->GetCollisionAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH || GameServer()->Collision()->GetCollisionAt(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH || GameServer()->Collision()->GetCollisionAt(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius)&CCollision::COLFLAG_DEATH || GameLayerClipped(m_Pos)) 	{		Die(m_pPlayer->GetCID(), WEAPON_SELF); 	}
	/* DEATH ( GAMELAYER ) */
	if(GameServer()->Collision()->GetCollisionAt(m_Pos.x+m_ProximityRadius/100.f, m_Pos.y-m_ProximityRadius/100.f)&CCollision::COLFLAG_UNFREEZE ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x+m_ProximityRadius/100.f, m_Pos.y+m_ProximityRadius/100.f)&CCollision::COLFLAG_UNFREEZE ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x-m_ProximityRadius/100.f, m_Pos.y-m_ProximityRadius/100.f)&CCollision::COLFLAG_UNFREEZE ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x-m_ProximityRadius/100.f, m_Pos.y+m_ProximityRadius/100.f)&CCollision::COLFLAG_UNFREEZE)
    /* UNFREEZE ( GAMELAYER ) */	{	    Freeze(0);		}
	if(GameServer()->Collision()->GetCollisionAt(m_Pos.x+m_ProximityRadius/100.f, m_Pos.y-m_ProximityRadius/100.f)&CCollision::COLFLAG_FREEZE ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x+m_ProximityRadius/100.f, m_Pos.y+m_ProximityRadius/100.f)&CCollision::COLFLAG_FREEZE ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x-m_ProximityRadius/100.f, m_Pos.y-m_ProximityRadius/100.f)&CCollision::COLFLAG_FREEZE ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x-m_ProximityRadius/100.f, m_Pos.y+m_ProximityRadius/100.f)&CCollision::COLFLAG_FREEZE)
	/* FREEZE ( GAMELAYER ) */	{	    Freeze(g_Config.m_FreezeLength);		}
	if(GameServer()->Collision()->GetZoneValueAt(GameServer()->m_ZoneHandle_TeeWorlds, m_Pos.x+m_ProximityRadius/100.f, m_Pos.y-m_ProximityRadius/100.f) == 1 ||
		GameServer()->Collision()->GetZoneValueAt(GameServer()->m_ZoneHandle_TeeWorlds, m_Pos.x+m_ProximityRadius/100.f, m_Pos.y+m_ProximityRadius/100.f) == 1 ||
		GameServer()->Collision()->GetZoneValueAt(GameServer()->m_ZoneHandle_TeeWorlds, m_Pos.x-m_ProximityRadius/100.f, m_Pos.y-m_ProximityRadius/100.f) == 1 ||
		GameServer()->Collision()->GetZoneValueAt(GameServer()->m_ZoneHandle_TeeWorlds, m_Pos.x-m_ProximityRadius/100.f, m_Pos.y+m_ProximityRadius/100.f) == 1)
	/* FREEZE ( #ZONES>TEEWORLDS ) */	{	    Freeze(g_Config.m_FreezeLength);		}
 if(GameServer()->Collision()->GetZoneValueAt(GameServer()->m_ZoneHandle_Death, m_Pos.x+m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f) == 1 ||
    GameServer()->Collision()->GetZoneValueAt(GameServer()->m_ZoneHandle_Death, m_Pos.x+m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f) == 1 ||
    GameServer()->Collision()->GetZoneValueAt(GameServer()->m_ZoneHandle_Death, m_Pos.x-m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f) == 1 ||
    GameServer()->Collision()->GetZoneValueAt(GameServer()->m_ZoneHandle_Death, m_Pos.x-m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f) == 1)
    /* DEATH ( #ZONES>DEATH ) */   {    Die(m_pPlayer->GetCID(), WEAPON_SELF);    }
	if(GameServer()->Collision()->GetZoneValueAt(GameServer()->m_ZoneHandle_TeeWorlds, m_Pos.x+m_ProximityRadius/100.f, m_Pos.y-m_ProximityRadius/2.f) == 4 ||
		GameServer()->Collision()->GetZoneValueAt(GameServer()->m_ZoneHandle_TeeWorlds, m_Pos.x+m_ProximityRadius/100.f, m_Pos.y+m_ProximityRadius/2.f) == 4 ||
		GameServer()->Collision()->GetZoneValueAt(GameServer()->m_ZoneHandle_TeeWorlds, m_Pos.x-m_ProximityRadius/100.f, m_Pos.y-m_ProximityRadius/2.f) == 4 ||
		GameServer()->Collision()->GetZoneValueAt(GameServer()->m_ZoneHandle_TeeWorlds, m_Pos.x-m_ProximityRadius/100.f, m_Pos.y+m_ProximityRadius/2.f) == 4 )
	/* DOWNSTOPPER ( #ZONES>TEEWORLDS ) */   {  if(m_Core.m_Vel.y < 0)  m_Core.m_Vel.y = 0;	}

 // ▒█▀▀▀█ ▒█▀▀█ ▒█▀▀▀ ▒█▀▀▀ ▒█▀▀▄ 　 ▒█░▒█ ▒█▀▀█
 // ░▀▀▀▄▄ ▒█▄▄█ ▒█▀▀▀ ▒█▀▀▀ ▒█░▒█ 　 ▒█░▒█ ▒█▄▄█
 // ▒█▄▄▄█ ▒█░░░ ▒█▄▄▄ ▒█▄▄▄ ▒█▄▄▀ 　 ░▀▄▄▀ ▒█░░░
    int Index = GameServer()->Collision()->GetMapIndex(m_Pos);
   	if(GameServer()->Collision()->IsSpeedup(Index))
	{
		vec2 Direction, TempVel = m_Core.m_Vel;
		int Force, MaxSpeed, Type = 0;
		float TeeAngle, SpeederAngle, DiffAngle, SpeedLeft, TeeSpeed;
		GameServer()->Collision()->GetSpeedup(Index, &Direction, &Force, &MaxSpeed, &Type);
		if(Type == 1) // PASSIVE CONTROL //
		{
		    if(MaxSpeed == 0) // SET PASSIVE TO ANY SECOUND
                m_PassiveTicks = Force*SERVER_TICK_SPEED;
            else if(MaxSpeed == 1 && m_PassiveTicks == -1) // SET PASSIVE TO ANY SECOUND IF HAD BEFORE
                m_PassiveTicks = Force*SERVER_TICK_SPEED;
            else if(MaxSpeed == 2) // DISABLE PASSIVE WHENEVER
                m_PassiveTicks = 1;
            else if(MaxSpeed == 3) // SET PASSIVE INFINITE
                m_PassiveTicks = -1;
		}
		else if (Type == 3 && m_pPlayer->PlayerEvent() != CPlayer::EVENT_NONE)
		{
				Die(m_pPlayer->GetCID(), WEAPON_SELF); // DEATH IN ANY EVENT //
		}
        // Blocks movement based on angle //
        else if (Type == 30)
        {
            float DirLen = std::sqrt(Direction.x * Direction.x + Direction.y * Direction.y);
            if (DirLen < 0.000001f) // Avoid division by zero if Direction is (0,0)
                return; // Or handle as an error/do nothing

            vec2 NormalizedDirection = Direction / DirLen;

            SpeederAngle = std::atan2(NormalizedDirection.y, NormalizedDirection.x);
            if (std::abs(TempVel.x) < 0.000001f && std::abs(TempVel.y) < 0.000001f)
                return;

            TeeAngle = std::atan2(TempVel.y, TempVel.x);

            DiffAngle = SpeederAngle - TeeAngle;

            while (DiffAngle <= -std::asin(1.0f) * 2.0f) DiffAngle += std::asin(1.0f) * 4.0f; // Add 2*PI
            while (DiffAngle > std::asin(1.0f) * 2.0f) DiffAngle -= std::asin(1.0f) * 4.0f;  // Subtract 2*PI
            float DotProduct = TempVel.x * NormalizedDirection.x + TempVel.y * NormalizedDirection.y;

            // Define a small epsilon for comparison
            const float EPSILON = 0.0001f;

            if (DotProduct > EPSILON)
                m_Core.m_Vel -= NormalizedDirection * DotProduct;

        }

        //### CLASSIC SPEEDUP###//

        else if(Type == 29 || Type == 28)
        {
    		if(Force == 255 && MaxSpeed)
    		{
    			m_Core.m_Vel = Direction * (MaxSpeed / 5);
    		}
    		else
    		{
    			if(MaxSpeed > 0 && MaxSpeed < 5)
    				MaxSpeed = 5;
    			if(MaxSpeed > 0)
    			{
    				if(Direction.x > 0.0000001f)
    					SpeederAngle = -std::atan(Direction.y / Direction.x);
    				else if(Direction.x < 0.0000001f)
    					SpeederAngle = std::atan(Direction.y / Direction.x) + 2.0f * std::asin(1.0f);
    				else if(Direction.y > 0.0000001f)
    					SpeederAngle = std::asin(1.0f);
    				else
    					SpeederAngle = std::asin(-1.0f);

    				if(SpeederAngle < 0)
    					SpeederAngle = 4.0f * std::asin(1.0f) + SpeederAngle;

    				if(TempVel.x > 0.0000001f)
    					TeeAngle = -std::atan(TempVel.y / TempVel.x);
    				else if(TempVel.x < 0.0000001f)
    					TeeAngle = std::atan(TempVel.y / TempVel.x) + 2.0f * std::asin(1.0f);
    				else if(TempVel.y > 0.0000001f)
    					TeeAngle = std::asin(1.0f);
    				else
    					TeeAngle = std::asin(-1.0f);

    				if(TeeAngle < 0)
    					TeeAngle = 4.0f * std::asin(1.0f) + TeeAngle;

    				TeeSpeed = std::sqrt(std::pow(TempVel.x, 2) + std::pow(TempVel.y, 2));

    				DiffAngle = SpeederAngle - TeeAngle;
    				SpeedLeft = MaxSpeed / 5.0f - std::cos(DiffAngle) * TeeSpeed;
    				if(absolute((int)SpeedLeft) > Force && SpeedLeft > 0.0000001f)
    					TempVel += Direction * Force;
    				else if(absolute((int)SpeedLeft) > Force)
    					TempVel += Direction * -Force;
    				else
    					TempVel += Direction * SpeedLeft;
    			}
    			else
    				TempVel += Direction * Force;

    			m_Core.m_Vel = TempVel;
    		}
        }
	}

// ▀▀█▀▀ ▒█▀▀▀ ▒█░░░ ▒█▀▀▀ ▒█▀▀█ ▒█▀▀▀█ ▒█▀▀█ ▀▀█▀▀
// ░▒█░░ ▒█▀▀▀ ▒█░░░ ▒█▀▀▀ ▒█▄▄█ ▒█░░▒█ ▒█▄▄▀ ░▒█░░
// ░▒█░░ ▒█▄▄▄ ▒█▄▄█ ▒█▄▄▄ ▒█░░░ ▒█▄▄▄█ ▒█░▒█ ░▒█░░

	const CTeleTile *pTeleLayer = GameServer()->Collision()->TeleLayer();
	if(!pTeleLayer)
		return;
	int TeleNumber = pTeleLayer[Index].m_Number;
	int TeleType = pTeleLayer[Index].m_Type;

	if(TeleNumber < 1 || TeleType == TILE_TELEOUT || TeleType == 30)
	    return;

	if(TeleType == 29 )
	{ // CHECKPOINT
        m_CheckPoint = TeleNumber;
        return;
	}

	int DestTeleNumber = -1;
	vec2 DestPosition;

	if(TeleType == 63 || TeleType == 32) // CFRM
	{
	    const std::vector<vec2> &CheckOuts = GameServer()->Collision()->TeleCheckOuts(m_CheckPoint);
	    if(CheckOuts.empty())
            return;
        DestTeleNumber = clamp(static_cast<int>(random()), 0, static_cast<int>(CheckOuts.size()) - 1);
        DestPosition = CheckOuts.at(DestTeleNumber);
	}
	if(TeleType == 10 || TeleType == 26 || TeleType == 14 ) // NORMAL TELE IN
    {
        const std::vector<vec2> &Outs = GameServer()->Collision()->TeleOuts(TeleNumber);
        if(Outs.empty())
            return;
        DestTeleNumber = clamp(static_cast<int>(random()), 0, static_cast<int>(Outs.size()) - 1);
        DestPosition = Outs.at(DestTeleNumber);
        if(TeleType == 14 && (m_ActiveWeapon != WEAPON_HAMMER || m_AttackTick != Server()->Tick()-SERVER_TICK_SPEED/8))
            DestPosition = vec2(0,0);
    }
    if(DestTeleNumber != -1)
    {
        m_Core.m_Pos = DestPosition;
        if((TeleType == 63) || (TeleType == 10))
        {
           	m_Core.m_Vel = vec2(0, 0);
            m_Core.ResetHook();
    	}
    }

    //### CUSTOM TELEPORT ITEMS###//

    switch(TeleType)
    {
        case 198:
            break;
        case 33:
            m_RaceTick = Server()->Tick();
            break;
        case 34:
            if(m_RaceTick != -1)
            {
            	int Time = (Server()->Tick()-m_RaceTick)/SERVER_TICK_SPEED;
				char aTime[128];
				str_format(aTime, sizeof(aTime), "%02d:%02d", Time/60, Time%60);
                GameServer()->CreateDeath(m_Pos, m_pPlayer->GetCID(), m_Core.m_VTeam);
                GameServer()->SendChatTarget(-1, _("{str:PlayerName} finished race in {str:Time}"), "PlayerName", Server()->ClientName(m_pPlayer->GetCID()), "Time", aTime);
                m_RaceTick = -1;
            }
            break;
        case 69:
            break;
    }
}
