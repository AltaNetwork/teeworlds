// #include "tournament.h"
// #include "gamecontext.h"
// #include <engine/shared/config.h>

// CLMB::CLMB()
// {
// 	m_State = STATE_STANDBY;
// 	m_LastLMB = 0;
// }

// CLMB::~CLMB()
// {
//     CGameContext *m_pGameServer;
// 	CPlayer *m_pPlayer;
// }

// void CLMB::OpenRegistration()
// {
// 	if (m_State > STATE_STANDBY)
// 		return;

// 	m_Participants.clear();
// 	m_LastLMB = m_pGameServer->Server()->Tick();

// 	dbg_msg("LMB", "opened registration for lmb");

// 	m_State = STATE_REGISTRATION;
// 	m_StartTick = m_pGameServer->Server()->Tick() + SERVER_TICK_SPEED * g_Config.m_SvLMBRegDuration;
// }

// void CLMB::Tick()
// {
// 	if (m_State == STATE_REGISTRATION)
// 	{
//     	if (m_pGameServer->Server()->Tick() % SERVER_TICK_SPEED == 0)
//     	{
//     		char aBuf[512];
//     		str_format(aBuf, sizeof(aBuf), "Tournament is starting in %ds\n%d subscribed (use /sub to subscribe)                                                     ", (m_StartTick-m_pGameServer->Server()->Tick())/SERVER_TICK_SPEED , ParticipantNum());
//     		m_pGameServer->SendBroadcast(aBuf, -1);
//     	}

// 		int Diff = m_StartTick - m_pGameServer->Server()->Tick();

// 		if (Diff % 1500 == 0 && Diff > 0 && Diff != m_pGameServer->Server()->Tick() + SERVER_TICK_SPEED * g_Config.m_SvLMBRegDuration)
// 			m_pGameServer->SendChatTarget(-1, "Tournament starts soon use /sub if you wish to participate!");
// 		else if (Diff <= 0)
// 		{
// 			if (m_Participants.size() >= (unsigned)2)
// 			{
// 				m_State = STATE_RUNNING;
// 				m_StartTick = m_pGameServer->Server()->Tick();
// 				TeleportParticipants();
// 				m_pGameServer->SendBroadcast("Tournament has started!", -1);
// 			}
// 			else
// 			{
// 				dbg_msg("LMB", "lack of players to start tournament");
// 				m_pGameServer->SendBroadcast("Not enough players to start tournament", -1 );

// 				m_State = STATE_STANDBY;
// 				for (unsigned int i = 0; i < m_Participants.size(); i++)
// 				{
// 					if (m_pGameServer->m_apPlayers[m_Participants[i]])
// 						m_pGameServer->m_apPlayers[m_Participants[i]]->m_LMBState = LMB_NONREGISTERED;
// 				}

// 				m_Participants.clear();
// 			}
// 		}
// 	}
// 	else if (m_Participants.size() && m_State == STATE_STANDBY && m_pGameServer->Server()->Tick() > m_EndTick + 500)
// 	{
// 		dbg_msg("LMB", "Removing winner from arena (%d)", m_Participants[0]);

// 		m_pGameServer->m_apPlayers[m_Participants[0]]->m_LMBState = LMB_NONREGISTERED;
// 		if (m_pGameServer->m_apPlayers[m_Participants[0]]->GetCharacter())
// 			m_pGameServer->m_apPlayers[m_Participants[0]]->KillCharacter();

// 		m_State = STATE_STANDBY;
// 		m_Participants.clear();

// 	}
// 	else if (m_State == STATE_RUNNING)
// 	{
// 	    int TimeLeft = abs(((m_pGameServer->Server()->Tick() - m_StartTick) - SERVER_TICK_SPEED * g_Config.m_SvLMBTournamentTime))/SERVER_TICK_SPEED;

// 		if (m_pGameServer->Server()->Tick() % SERVER_TICK_SPEED == 0)
// 	    {
//     	    char aBuf[512];
//        	    str_format(aBuf, sizeof(aBuf), "Opponents left: %d\nTime Left: %ds                                                     ", ParticipantNum(), TimeLeft);
//             m_pGameServer->SendBroadcast(aBuf, -1);
// 		}
// 		if (m_pGameServer->Server()->Tick() >= m_StartTick + SERVER_TICK_SPEED * g_Config.m_SvLMBTournamentTime)	//force cancel
// 		{
// 			m_State = STATE_STANDBY;

// 			while (m_Participants.size())
// 			{		//killed player will get removed from the list; so we just have to kill ID 0 all the time!
// 					//m_pGameServer->m_apPlayers[m_Participants[0]]->m_LMBState = 0;
// 				m_pGameServer->m_apPlayers[m_Participants[0]]->m_LMBState = LMB_NONREGISTERED;
// 				if (m_pGameServer->m_apPlayers[m_Participants[0]] && m_pGameServer->m_apPlayers[m_Participants[0]]->GetCharacter())
// 				{
// 					dbg_msg("LMB", "Killing %d because tournament-time is over!", m_Participants[0]);
// 					m_pGameServer->m_apPlayers[m_Participants[0]]->m_LMBState = 0;
// 					m_pGameServer->m_apPlayers[m_Participants[0]]->KillCharacter();
// 					m_pGameServer->m_apPlayers[m_Participants[0]]->m_DieTick = m_pGameServer->Server()->Tick() - m_pGameServer->Server()->TickSpeed() * 3;
// 				}
// 				m_Participants.erase(m_Participants.begin());
// 			}

// 			m_pGameServer->SendChatTarget(-1, "Tournament has been closed due to the timelimit.");

// 			m_LastLMB = m_pGameServer->Server()->Tick();
// 		}
// 	}
// }

// bool CLMB::RegisterPlayer(int ID)
// {
// 	std::vector<int>::iterator it = FindParticipant(ID);

// 	if (it == m_Participants.end() && m_pGameServer->m_apPlayers[ID])	//register him!
// 	{
// 		m_Participants.push_back(ID);
// 		m_pGameServer->m_apPlayers[ID]->m_LMBState = LMB_REGISTERED;
// 		return true;
// 	}

// 	if (m_pGameServer->m_apPlayers[ID])
// 		m_pGameServer->m_apPlayers[ID]->m_LMBState = LMB_NONREGISTERED;

// 	m_Participants.erase(it);
// 	return false;
// }

// bool CLMB::IsParticipant(int ID)
// {
// 	return !(FindParticipant(ID) == m_Participants.end());
// }

// std::vector<int>::iterator CLMB::FindParticipant(int ID)
// {
// 	std::vector<int>::iterator it = std::find(m_Participants.begin(), m_Participants.end(), ID);
// 	return it;
// }

// void CLMB::TeleportParticipants()
// {
// 	for (unsigned int i = 0; i < m_Participants.size(); i++)
// 	{
// 		vec2 ActPos; bool CanSpawn;

// 		// if ((CanSpawn = m_pGameServer->m_pController->CanSpawn(2, &ActPos)) && m_pGameServer->m_apPlayers[m_Participants[i]])
// 		// {
// 			if (m_pGameServer->m_apPlayers[m_Participants[i]]->GetCharacter() && m_pGameServer->m_apPlayers[m_Participants[i]]->m_LMBState == LMB_REGISTERED)
// 			{
// 				Reset(m_Participants[i]);
// 				m_pGameServer->m_apPlayers[m_Participants[i]]->m_SpawnTeam = 2;
// 				m_pGameServer->m_apPlayers[m_Participants[i]]->KillCharacter();
// 				m_pGameServer->m_apPlayers[m_Participants[i]]->m_LMBState = LMB_PARTICIPATE;
// 			}
// 			else
// 			{
// 				m_pGameServer->SendChatTarget(m_Participants[i], "Oops! Something went wrong! Please assure that you are alive when the tournament starts.");
// 				m_pGameServer->m_apPlayers[m_Participants[i]]->m_LMBState = LMB_NONREGISTERED;
// 				RemoveParticipant(m_Participants[i]);
// 			}
// 		// }
// 		// else
// 		// {
// 		// 	m_pGameServer->SendChatTarget(m_Participants[i], "Oops, some strange error happened. We're sorry. If you have information for us what exactly happened, please tell the team.");
// 		// 	if (m_pGameServer->m_apPlayers[m_Participants[i]])
// 		// 	{
// 		// 		dbg_msg("LMB/ERROR", "Error while joining the tournament: ID=%d, SpawnPos=(%.0f %.0f; can%sspawn here)", m_Participants[i], ActPos.x, ActPos.y, CanSpawn ? " " : " NOT ");
// 		// 		m_pGameServer->m_apPlayers[m_Participants[i]]->m_LMBState = LMB_NONREGISTERED;
// 		// 	}
// 		// 	else
// 		// 		dbg_msg("LMB/ERROR", "Player with ID=%d failed to join the tournament: Pointer not valid anymore.");
// 		// 	RemoveParticipant(m_Participants[i]);
// 		// }
// 	}
// }

// void CLMB::RemoveParticipant(int CID)
// {
// 	std::vector<int>::iterator it = FindParticipant(CID);
// 	m_Participants.erase(it);

// 	dbg_msg("LMB", "Removing %d!", CID);
// 	char aBuf[256];

// 	if (m_Participants.size() == 1 && m_State == STATE_RUNNING)
// 	{
// 		int ID = m_Participants[0];
// 		CPlayer *Winner = m_pGameServer->m_apPlayers[ID];
// 		// str_format(aBuf, sizeof(aBuf), "'%s' won!", m_pGameServer->Server()->ClientName(ID));
// 		// m_pGameServer->SendBroadcast(aBuf, -1);

// 		char aBuf2[256];
// 		str_format(aBuf2, sizeof(aBuf2), "'%s' has won the tournament!", m_pGameServer->Server()->ClientName(ID));
// 		m_pGameServer->SendChatTarget(-1, aBuf2);

// 		dbg_msg("LMB", "%d has won!", ID);

// 		m_EndTick = m_pGameServer->Server()->Tick();
// 		m_State = STATE_STANDBY;
// 		m_LastLMB = m_pGameServer->Server()->Tick();
// 	}
// }

// void CLMB::Reset(int ClientID)
// {
// 	CPlayer* pPlayer = m_pGameServer->m_apPlayers[ClientID];
// 	pPlayer->m_SpawnTeam = 0;
// }
