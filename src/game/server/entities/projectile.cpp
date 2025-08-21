

#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "projectile.h"
#include "loltext.h"

CProjectile::CProjectile(CGameWorld *pGameWorld, int Type, int Owner, vec2 Pos, vec2 Dir, int Span,
		int Damage, bool Explosive, float Force, int SoundImpact, int Weapon, int VTeam, int SubType)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
	m_Type = Type;
	m_Pos = Pos;
	m_Direction = Dir;
	m_LifeSpan = Span;
	m_Owner = Owner;
	m_Force = Force;
	m_Damage = Damage;
	m_SoundImpact = SoundImpact;
	m_Weapon = Weapon;
	m_StartTick = Server()->Tick();
	m_Explosive = Explosive;

	m_SubType = SubType;

	m_WTeam = VTeam;
	m_VTeam = VTeam;

	GameWorld()->InsertEntity(this);
}

void CProjectile::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

vec2 CProjectile::GetPos(float Time)
{
	float Curvature = 0;
	float Speed = 0;

	switch(m_Type)
	{
		case WEAPON_GRENADE:
			Curvature = GameServer()->Tuning()->m_GrenadeCurvature;
			Speed = GameServer()->Tuning()->m_GrenadeSpeed;
			break;

		case WEAPON_SHOTGUN:
			Curvature = GameServer()->Tuning()->m_ShotgunCurvature;
			Speed = GameServer()->Tuning()->m_ShotgunSpeed;
			break;

		case WEAPON_GUN:
		    switch(m_SubType)
           	{
           	    case 0:
                case 5:
             		Curvature = GameServer()->Tuning()->m_GunCurvature;
                    Speed = GameServer()->Tuning()->m_GunSpeed;
          		break;
                case 1:
                case 2:
                case 3:
                case 4:
              		Curvature = 0;
                    Speed = 750;
                break;
           	}
			break;
	}

	return CalcPos(m_Pos, m_Direction, Curvature, Speed, Time);
}


void CProjectile::Tick()
{
	float Pt = (Server()->Tick()-m_StartTick-1)/(float)Server()->TickSpeed();
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	vec2 PrevPos = GetPos(Pt);
	vec2 CurPos = GetPos(Ct);
	int Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &CurPos, 0);
	CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *TargetChr = GameServer()->m_World.IntersectCharacter(PrevPos, CurPos, 6.0f, CurPos, OwnerChar);
	m_LifeSpan--;

	if((TargetChr && TargetChr->GetVTeam() == m_VTeam) || Collide || m_LifeSpan < 0 || GameLayerClipped(CurPos))
	{
		if(m_LifeSpan >= 0 || m_Weapon == WEAPON_GRENADE)
			GameServer()->CreateSound(CurPos, m_SoundImpact);

		if(m_Explosive)
			GameServer()->CreateExplosion(CurPos, m_Owner, m_Weapon, false, m_VTeam);

		else if(TargetChr)
			TargetChr->TakeDamage(m_Direction * max(0.001f, m_Force), m_Owner, m_Weapon);

		if(m_SubType == 0 && m_Type == WEAPON_GUN)
    		GameServer()->CreateDamageInd(GetPos(Ct), -std::atan2(m_Direction.x, m_Direction.y), 6, m_VTeam);

		// CLoltext::Create(GameWorld(), PrevPos, SERVER_TICK_SPEED/5, "PEW", true, false, 1);

        GameServer()->m_World.DestroyEntity(this);
	}
}

void CProjectile::TickPaused()
{
	++m_StartTick;
}

void CProjectile::FillInfo(CNetObj_Projectile *pProj)
{
	pProj->m_X = (int)m_Pos.x;
	pProj->m_Y = (int)m_Pos.y;
	pProj->m_VelX = (int)(m_Direction.x*100.0f);
	pProj->m_VelY = (int)(m_Direction.y*100.0f);
	pProj->m_StartTick = m_StartTick;
	pProj->m_Type = m_Type;
}

void CProjectile::Snap(int SnappingClient)
{
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	vec2 Position = GetPos(Ct);

	if(NetworkClipped(SnappingClient, Position))
		return;

	if(m_SubType == 0)
	{
	    CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
    	if(pProj)
            FillInfo(pProj);
	} else if (m_SubType == 1)
    {
        CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
        if(pObj)
        {
      		pObj->m_X = (int)Position.x;
      		pObj->m_Y = (int)Position.y;
      		pObj->m_FromX = (int)(Position.x);
      		pObj->m_FromY = (int)(Position.y);
      		pObj->m_StartTick = Server()->Tick();
        }
    } else if (m_SubType > 1 && m_SubType < 5)
	{
	    CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
		if(pP)
		{
            pP->m_X = (int)Position.x;
           	pP->m_Y = (int)Position.y;
           	pP->m_Type = m_SubType < 4 ? m_SubType == 3 ? POWERUP_HEALTH : POWERUP_ARMOR : POWERUP_WEAPON;
           	pP->m_Subtype = m_SubType > 2 ? 1 : 0;
		}
	}

	// 0 = NORMAL
	// 1 = PLASMA
	// 2 = GHOST
	// 3 = LOVE
	// 4 = GUNGUN
	// 5 = STAR
}
