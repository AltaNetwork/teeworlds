/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "pickup.h"
#include "projectile.h"

CPickup::CPickup(CGameWorld *pGameWorld, int Type, int SubType, bool remove_on_pickup)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP)
{
	m_Type = Type;
	m_Subtype = SubType;
	m_ProximityRadius = PickupPhysSize;
	// m_ID2 = Server()->SnapNewID();
	m_Remove_on_pickup = remove_on_pickup;

	Reset();
	if (m_Remove_on_pickup)
		m_SpawnTick = -1;

	GameWorld()->InsertEntity(this);
}

void CPickup::Reset() { m_SpawnTick = -1; }

void CPickup::Tick()
{
	// wait for respawn
	if(m_SpawnTick > 0 && Server()->Tick() > m_SpawnTick) {   m_SpawnTick = -1;  }

	// Check if a player intersected us
	CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, 20.0f, 0);
	if(pChr && pChr->IsAlive()) {
		// player picked us up, is someone was hooking us, let them go
		int RespawnTime = -1;
		int amount = 1; // amount of heart/shields
		bool success = false; // to reset if its picked up
		switch (m_Type) {
			case POWERUP_HEALTH:
				if(pChr->IncreaseHealth(amount))    {
                    success = true;
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_HEALTH);
				}
				break;

			case POWERUP_ARMOR:
				if(pChr->IncreaseArmor(amount)) {
				    success = true;
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR);
				}
				break;

			case POWERUP_WEAPON:
				if(m_Subtype >= 0 && m_Subtype < NUM_WEAPONS+NUM_WEAPONS_EXTRA) {
					if(pChr->GiveWeapon(m_Subtype, 10)) {
	                    success = true;
						if(m_Subtype == WEAPON_GRENADE)   GameServer()->CreateSound(m_Pos, SOUND_PICKUP_GRENADE);
						else if(m_Subtype == WEAPON_SHOTGUN || m_Subtype == WEAPON_RIFLE)   GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN);
						if(pChr->GetPlayer())
							GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCID(), m_Subtype);
				}   }
				break;
			case POWERUP_NINJA: {
					success = true;
					pChr->GiveNinja();
					pChr->SetEmote(EMOTE_ANGRY, Server()->Tick() + 1200 * Server()->TickSpeed() / 1000);
					break;
				}
			default:
				break;
		};
		if(success) { RespawnTime = g_pData->m_aPickups[m_Type].m_Respawntime; } // only remove itself if player can recieve pickup
		if(RespawnTime >= 0) {   m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * RespawnTime; // respawn it
    		if (m_Remove_on_pickup) {   GameWorld()->DestroyEntity(this);   }   } // if its to be removed ( used in add pickup cmd )
	}
}

void CPickup::TickPaused()  {
	if(m_SpawnTick != -1)
		++m_SpawnTick;
}

void CPickup::Snap(int SnappingClient)
{
	if(m_SpawnTick != -1 || NetworkClipped(SnappingClient))
	    return;
	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
	if(!pP)
		return;
	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y;
	pP->m_Type = m_Type;
	pP->m_Subtype = m_Subtype;
}
