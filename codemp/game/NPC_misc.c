/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

//
// NPC_misc.cpp
//
#include "b_local.h"
#include "qcommon/q_shared.h"

/*
Debug_Printf
*/
void Debug_Printf (vmCvar_t *cv, int debugLevel, char *fmt, ...)
{
	char		*color;
	va_list		argptr;
	char		msg[1024];

	if (cv->value < debugLevel)
		return;

	if (debugLevel == DEBUG_LEVEL_DETAIL)
		color = S_COLOR_WHITE;
	else if (debugLevel == DEBUG_LEVEL_INFO)
		color = S_COLOR_GREEN;
	else if (debugLevel == DEBUG_LEVEL_WARNING)
		color = S_COLOR_YELLOW;
	else // if (debugLevel == DEBUG_LEVEL_ERROR)
		color = S_COLOR_RED;

	va_start (argptr,fmt);
	Q_vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	Com_Printf("%s%5i:%s", color, level.time, msg);
}


/*
Debug_NPCPrintf
*/
void Debug_NPCPrintf (gentity_t *printNPC, vmCvar_t *cv, int debugLevel, char *fmt, ...)
{
	int			color;
	va_list		argptr;
	char		msg[1024];

	if (cv->value < debugLevel)
	{
		return;
	}

//	if ( debugNPCName.string[0] && Q_stricmp( debugNPCName.string, printNPC->targetname) != 0 )
//	{
//		return;
//	}

	if (debugLevel == DEBUG_LEVEL_DETAIL)
		color = COLOR_WHITE;
	else if (debugLevel == DEBUG_LEVEL_INFO)
		color = COLOR_GREEN;
	else if (debugLevel == DEBUG_LEVEL_WARNING)
		color = COLOR_YELLOW;
	else // if (debugLevel == DEBUG_LEVEL_ERROR)
		color = COLOR_RED;

	va_start (argptr,fmt);
	Q_vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	Com_Printf ("%c%c%5i (%s) %s", Q_COLOR_ESCAPE, color, level.time, printNPC->targetname, msg);
}

//ClanMod - NPC Order
orderNPC_t orderNPCTable[] = {

		{ "follow",
		"Call back your NPCs. When they are following you they adopt your behavior. So if you are crouched they will try to pass between enemies stealthly, and otherwise they try to attack enemies around with following you.",
		&NPCF_O_Follow },

		{ "attack",
		"Make your NPCs attack the target that you point out. If the target is not a character or cannot take damage then your NPCs will defend the position.",
		&NPCF_O_Attack },

		{ "defend",
		"Order your NPCs to defend your actual position. They will stay there until you give them an other order.",
		&NPCF_O_StayHere },

		{
			NULL,
			NULL,
			NULL
		}

};

NPCORDER_FUNC *NPCF_GetOrderForName(const char *name)
{
	orderNPC_t *type = orderNPCTable;

	while (type->order)
	{
		if (Q_stricmp(name, type->name) == 0)
			return type->order;

		type++;
	}

	return NULL;
}

int NPCF_MaxFollowers()
{
	int max3 = g_NPCmaxFollowers.integer;

	if (max3 > MAX_FOLLOWERS)
		max3 = MAX_FOLLOWERS;
	else if (max3 < 0)
		max3 = 0;

	return max3;
}

void NPCF_Add(gentity_t *self, gentity_t *ent)
{
	if (!ent || !self)
	{
		return;
	}

	if (!ent->client || !self->client)
	{
		return;
	}

	if (self->client->numPlFollowers >= NPCF_MaxFollowers())
	{
		trap->SendServerCommand(self->client->ps.clientNum, va("print \"No more than %d followers...\n\"", g_NPCmaxFollowers.integer));
		G_EntitySound(self, CHAN_VOICE, G_SoundIndex("*nomore.wav"));
		TIMER_Set(ent, "followUse", 1750);
		return;
	}

	if (ent->client->playerTeam == NPCTEAM_NEUTRAL)
	{
		ent->NPC->keepTeam = 1 | (ent->client->enemyTeam << 4);

		ent->client->playerTeam = ent->alliedTeam = ent->s.teamowner = self->client->playerTeam;
		ent->client->enemyTeam = self->client->enemyTeam;
	}
	else
	{
		ent->NPC->keepTeam = 0;
	}

	self->client->plFollower[self->client->numPlFollowers] = ent;
	self->client->numPlFollowers++;

	ent->NPC->defaultBehavior = BS_FOLLOW_LEADER;
	ent->client->leader = self;
	ent->client->playerLeader = self;

	ent->NPC->behaviorState = BS_FOLLOW_LEADER;
	ent->NPC->defendEnt = self;
	ent->NPC->eventualGoal = self;

	NPC_RestoreStats(ent);

	//NPC_Respond( ent, self->s.number );

	TIMER_Set(ent, "dynamicBehavior", 10000);
}

qboolean NPCF_Remove(gentity_t *self, gentity_t *ent)
{
	int i;
	qboolean found = qfalse;

	if (!self)
	{
		return qfalse;
	}

	if (!self->client)
	{
		return qfalse;
	}

	if (!self->client->numPlFollowers)
	{
		return qfalse;
	}

	for (i = 0; i < self->client->numPlFollowers; i++)
	{
		if (self->client->plFollower[i] == ent)
		{
			found = qtrue;
			break;
		}
	}

	if (found)
	{
		if (i != (self->client->numPlFollowers - 1))
		{
			memmove(&(self->client->plFollower[i]), &(self->client->plFollower[i + 1]), ((self->client->numPlFollowers - 1) - i) * sizeof(gentity_t *));
			self->client->plFollower[self->client->numPlFollowers - 1] = NULL;
		}
		else
		{
			self->client->plFollower[i] = NULL;
		}

		self->client->numPlFollowers--;

		return qtrue;
	}

	return qfalse;
}

void NPCF_Drop(gentity_t *self, gentity_t *ent)
{
	if (!ent || !self)
	{
		return;
	}

	if (!ent->client || !self->client)
	{
		return;
	}

	if (!ent->NPC)
	{
		return;
	}

	if (!NPCF_Remove(self, ent))
	{
#ifdef _DEBUG
		Com_Printf(S_COLOR_CYAN"DEBUG WARNING ( NPCF_Drop ) : ent wasn't part of self team\n");
#endif
	}

	if (ent->NPC->keepTeam != 0)
	{
		ent->client->playerTeam = ent->alliedTeam = ent->s.teamowner = NPCTEAM_NEUTRAL;
		ent->client->enemyTeam = (ent->NPC->keepTeam >> 4);
		ent->NPC->keepTeam = 0;
	}

	if (ent->NPC->goalEntity == ent->client->playerLeader)
		ent->NPC->goalEntity = NULL;
	if (ent->NPC->eventualGoal == ent->client->playerLeader)
		ent->NPC->eventualGoal = NULL;
	if (ent->NPC->defendEnt == ent->client->playerLeader)
		ent->NPC->defendEnt = NULL;

	ent->NPC->captureGoal = NULL;

	if (ent->client->leader == self)
		ent->client->leader = NULL;
	ent->client->playerLeader = NULL;

	ent->NPC->defaultBehavior = ent->NPC->behaviorState = BS_DEFAULT;

	NPC_RestoreStats(ent);

	ent->NPC->scriptFlags &= ~SCF_CROUCHED;	//FIXME?
}

static void NPCF_CheckSafe(gentity_t *self)	//Cleans up
{
	int i;

	for (i = 0; i < self->client->numPlFollowers; i++)
	{
		gentity_t *ent = self->client->plFollower[i];

		if (!ent)
		{
			NPCF_Remove(self, ent);
			continue;
		}

		if (!ent->client || !ent->NPC)
		{
			NPCF_Remove(self, ent);
			break;
		}

		if (DistanceSquared(self->r.currentOrigin, self->client->plFollower[i]->r.currentOrigin) > (2048 * 2048))
		{
			NPCF_Drop(self, ent);
			break;
		}
	}
}

void NPCF_DropAll(gentity_t *self)
{
	int i;

	if (!self)
	{
		return;
	}

	if (!self->client)
	{
		return;
	}

	NPCF_CheckSafe(self);

	for (i = 0; i < self->client->numPlFollowers; i++)
	{
		NPCF_Drop(self, self->client->plFollower[i]);
	}
}

void NPCF_Recruit(gentity_t *self, gentity_t *ent)
{
	if (!ent || !self)
	{
		return;
	}

	if (!ent->client || !self->client)
	{
		return;
	}

	if (ent->client->NPC_class == CLASS_VEHICLE
		|| !ent->NPC
		|| ent->s.eType != ET_NPC
		|| ent->enemy == self
		|| (ent->client->playerTeam != self->client->playerTeam && ent->client->playerTeam != NPCTEAM_NEUTRAL)
		|| !TIMER_Done(ent, "followUse")
		)
	{
		return;
	}

	NPCF_CheckSafe(self);

	if (!ent->client->playerLeader)
	{
		NPCF_Add(self, ent);
	}
	else if (ent->client->playerLeader == self)
	{
		NPCF_Drop(self, ent);
	}

	TIMER_Set(ent, "followUse", 1750);
}

//ClanMod - Order NPCs
extern void WP_DeactivateSaber(gentity_t *self, qboolean clearLength);
void NPCF_DynamicBehavior(void)
{
	gentity_t *leader = NPCS.NPC->client->playerLeader;


	if (!leader)
		return;

	if (NPCS.NPCInfo->defaultBehavior == BS_FOLLOW_LEADER)
	{
		if ( /*( NPCInfo->special&NSP_CLOAKING ) &&*/
			leader->client->ps.powerups[PW_CLOAKED] &&
			TIMER_Done(NPCS.NPC, "cloakRafale"))
		{
			if (NPCS.NPC->enemy)
				G_ClearEnemy(NPCS.NPC);

			WP_DeactivateSaber(NPCS.NPC, qtrue);

			NPCS.NPCInfo->jediAggression = 1;
			//if ( !NPC->client->ps.powerups[PW_CLOAKED] )
			//	Jedi_Cloak( NPC );
		}

		if (TIMER_Done(NPCS.NPC, "followDuck") && !(NPCS.NPC->client->ps.eFlags2 & EF2_FLYING))
		{
			NPCS.NPCInfo->scriptFlags &= ~SCF_CROUCHED;

			if (leader->client->ps.pm_flags & PMF_DUCKED)
			{
				if (Q_irand(0, 10))
					NPCS.NPCInfo->scriptFlags |= SCF_CROUCHED;
			}
			else
			{
				if (!Q_irand(0, 4))
					NPCS.NPCInfo->scriptFlags |= SCF_CROUCHED;
			}

			TIMER_Set(NPCS.NPC, "followDuck", Q_irand(2500, 4500));
		}

		if ((leader->client->ps.pm_flags & PMF_DUCKED) && !(NPCS.NPC->client->ps.eFlags2 & EF2_FLYING))
		{
			NPCS.NPCInfo->stats.vfov = NPCS.NPCInfo->rstats.vfov / 5;
			NPCS.NPCInfo->stats.hfov = NPCS.NPCInfo->rstats.hfov / 5;
			NPCS.NPCInfo->stats.earshot = NPCS.NPCInfo->rstats.earshot / 5;
			NPCS.NPCInfo->stats.visrange = NPCS.NPCInfo->rstats.visrange / 5;
		}
		else
		{
			NPCS.NPCInfo->stats.vfov = NPCS.NPCInfo->rstats.vfov;
			NPCS.NPCInfo->stats.hfov = NPCS.NPCInfo->rstats.hfov;
			NPCS.NPCInfo->stats.earshot = NPCS.NPCInfo->rstats.earshot;
			NPCS.NPCInfo->stats.visrange = NPCS.NPCInfo->rstats.visrange;
		}

		if (TIMER_Done(NPCS.NPC, "dynamicBehavior"))
		{
			qboolean mweap = IsMeleeWeapon(NPCS.NPC->client->ps.weapon);

			int	r = Q_irand(0, 3);

			if (NPCS.NPC->enemy
				&& (NPCS.NPC->enemy->health > 0)
				&& ((r && mweap) || (!r && !mweap))
				&& (!(leader->client->ps.pm_flags & PMF_DUCKED) || !mweap)
				)
			{
				NPCS.NPCInfo->stats.vfov = NPCS.NPCInfo->rstats.vfov;
				NPCS.NPCInfo->stats.hfov = NPCS.NPCInfo->rstats.hfov;
				NPCS.NPCInfo->stats.earshot = NPCS.NPCInfo->rstats.earshot;
				NPCS.NPCInfo->stats.visrange = NPCS.NPCInfo->rstats.visrange;

				NPCS.NPCInfo->behaviorState = BS_HUNT_AND_KILL;
				NPCS.NPC->client->leader = NULL;

				if (mweap)
				{
					if (NPCS.NPC->enemy->client && NPCS.NPC->enemy->client->ps.weapon == WP_SABER)
						TIMER_Set(NPCS.NPC, "dynamicBehavior", Q_irand(4000, 6500));
					else
						TIMER_Set(NPCS.NPC, "dynamicBehavior", Q_irand(2500, 4500));
				}
				else
				{
					TIMER_Set(NPCS.NPC, "dynamicBehavior", Q_irand(1500, 3000));
				}

			}
			else
			{
				NPCS.NPCInfo->behaviorState = BS_FOLLOW_LEADER;
				NPCS.NPC->client->leader = leader;

				TIMER_Set(NPCS.NPC, "dynamicBehavior", Q_irand(1500, 3500));
			}
		}

		if (!(NPCS.NPC->client->ps.eFlags2 & EF2_FLYING)
			&& NPCS.NPC->client->NPC_class == CLASS_BOBAFETT
			&& leader->client->jetPackOn
			&& !NPCS.NPCInfo->shouldJetOn)
		{
			TIMER_Set(NPCS.NPC, "jetpackGoGo", Q_irand(300, 2000));
			NPCS.NPCInfo->shouldJetOn = qtrue;
		}
	}
	else if (NPCS.NPCInfo->defaultBehavior == BS_HUNT_AND_KILL)
	{

	}
	else if (NPCS.NPCInfo->defaultBehavior == BS_PATROL)
	{
		if (NPCS.NPC->enemy && NPCS.NPC->enemy->health > 0)
		{
			if (TIMER_Done(NPCS.NPC, "dynamicBehavior"))
			{
				NPCS.NPCInfo->scriptFlags &= ~SCF_CROUCHED;
				NPCS.NPCInfo->behaviorState = BS_HUNT_AND_KILL;
				NPCS.NPCInfo->scriptFlags |= SCF_CHASE_ENEMIES;
				TIMER_Set(NPCS.NPC, "dynamicBehavior", Q_irand(3500, 7500));
			}
		}
		else
		{
			if (TIMER_Done(NPCS.NPC, "dynamicBehavior"))
			{
				if (NPCS.NPCInfo->scriptFlags & SCF_CHASE_ENEMIES)
				{
					if (Q_irand(qfalse, qtrue))
						NPCS.NPCInfo->scriptFlags &= ~SCF_CROUCHED;
					else
						NPCS.NPCInfo->scriptFlags |= SCF_CROUCHED;
				}
				NPCS.NPCInfo->behaviorState = BS_PATROL;
				NPCS.NPCInfo->scriptFlags &= ~SCF_CHASE_ENEMIES;
				TIMER_Set(NPCS.NPC, "dynamicBehavior", Q_irand(1500, 3500));
			}
		}
	}

	//endHelpLeader:

	// FIX: NPCs sometimes behave in a strange manner (allied attack, attack blocked), this fixes it 
	// although with a small speed
#if 1
	if (NPCS.NPC->enemy)
	{
		if (!NPCS.NPC->enemy->inuse || NPCS.NPC->enemy->health <= 0 || NPCS.NPC->enemy == leader)
		{
			G_ClearEnemy(NPCS.NPC);
		}
		else
		{
			int i;

			for (i = 0; i < leader->client->numPlFollowers; i++)
			{
				if (NPCS.NPC->enemy == leader->client->plFollower[i])
				{
					G_ClearEnemy(NPCS.NPC);
					break;
				}
			}
		}

	}
#endif
}

extern qboolean NPC_TargetVisible(gentity_t *ent);
void NPCF_ProtectLeader(gentity_t *self, gentity_t *attacker)
{
	int			i, choice;
	qboolean	isPlayer;
	gentity_t	*target;

	if (!self->client)
		return;

	if (!self->client->numPlFollowers)
		return;

	if (self == attacker)
		return;

	if (!attacker->client)
		return;	//FIXME? check for turrent or other stuff

	if (attacker->client->playerLeader == self)
		return;

	isPlayer = (attacker->client && attacker->client->ps.clientNum < MAX_CLIENTS) ? qtrue : qfalse;

	if (!isPlayer && attacker->client->playerLeader)
	{
		target = attacker->client->playerLeader;
	}
	else
	{
		target = attacker;
	}

	NPCF_CheckSafe(self);

	for (i = 0; i < self->client->numPlFollowers; i++)
	{
		gentity_t *fwr = self->client->plFollower[i];

		SaveNPCGlobals();
		SetNPCGlobals(fwr);

		if (!NPC_ValidEnemy(fwr->enemy) ||
			!NPC_TargetVisible(fwr->enemy))
		{
			G_ClearEnemy(fwr);
		}

		if (!fwr->enemy)
		{
			choice = Q_irand(-1, target->client->numPlFollowers - 1);

			if (choice == -1)
			{
				G_SetEnemy(fwr, target);
			}
			else
			{
				gentity_t *tfwr = target->client->plFollower[choice];

				if (tfwr->health > 0 && NPC_ClearLOS4(tfwr))
					G_SetEnemy(fwr, tfwr);
				else
					G_SetEnemy(fwr, tfwr);
			}
		}

		RestoreNPCGlobals();
	}
}

void NPCF_Order(gentity_t *self, gentity_t *ent, NPCORDER_FUNC *order, qboolean init)
{
	if (!ent || !self || !order)
	{
		return;
	}

	if (!ent->client || !self->client)
	{
		return;
	}

#ifdef _DEBUG
	if (ent->client->playerLeader != self)
	{
		Com_Printf(S_COLOR_CYAN"DEBUG WARNING ( NPCF_Order ) : Giving order to a NPC who isn't in the team of the player 'self'\n'");
	}
#endif

	(*order)(ent, init);
}

void NPCF_OrderToAll(gentity_t *self, NPCORDER_FUNC *order)
{
	int i;

	if (!self || !order)
	{
		return;
	}

	if (!self->client)
	{
		return;
	}

	if (!self->client->numPlFollowers)
	{
		return;
	}

	for (i = 0; i < self->client->numPlFollowers; i++)
	{
		(*order)(self->client->plFollower[i], (i == 0) ? qtrue : qfalse);
	}
}

void NPCF_O_Follow(gentity_t *ent, qboolean init)
{
	if (init)
	{
		G_SetAnim(ent->client->playerLeader, NULL, SETANIM_BOTH, BOTH_COME_ON1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART, 0);
		G_EntitySound(ent->client->playerLeader, CHAN_VOICE, G_SoundIndex(va("*follow%d.wav", Q_irand(1, 2))));
	}

	SaveNPCGlobals();
	SetNPCGlobals(ent);

	ent->client->leader = ent->NPC->defendEnt = ent->client->playerLeader;
	ent->NPC->eventualGoal = ent->NPC->captureGoal = NULL;

	//NPC_ReachedGoal();
	VectorClear(NPCS.NPCInfo->tempGoal->r.currentOrigin);

	NPC_RestoreStats(ent);

	if (ent->NPC->defaultBehavior == BS_FOLLOW_LEADER) //related to jedi not listening
	{
		ent->NPC->behaviorState = BS_FOLLOW_LEADER;
		ent->NPC->scriptFlags &= ~SCF_CROUCHED;
		G_ClearEnemy(ent);
		ent->NPC->enemyCheckDebounceTime = level.time + ((ent->client->ps.weapon == WP_SABER) ? 6000 : 3500);
		TIMER_Set(ent, "dynamicBehavior", 10000);
		RestoreNPCGlobals();
		return;	//FIXME?
	}

	ent->NPC->defaultBehavior = ent->NPC->behaviorState = BS_FOLLOW_LEADER;

	if (!NPC_ValidEnemy(ent->enemy))
	{
		G_ClearEnemy(ent);
	}

	//mm..
	ent->NPC->scriptFlags &= ~(SCF_CROUCHED | SCF_WALKING | SCF_RUNNING | SCF_LOOK_FOR_ENEMIES | SCF_CHASE_ENEMIES | SCF_USE_CP_NEAREST);
	ent->NPC->scriptFlags |= (SCF_DONT_FLEE);

	RestoreNPCGlobals();
}

static trace_t	tr;
static gentity_t	*traceEnt;
static vec3_t	goalOrg;
static int	moveType;	//0-all,1-flying,2-none

static void NPCF_Init_Attack(gentity_t *ent)
{
	vec3_t	start, end, dir;
	const vec3_t	groundNormal = { 0, 0, 1 };

	gentity_t	*leader = ent->client->playerLeader;

	AngleVectors(leader->client->ps.viewangles, dir, NULL, NULL);

	memset(&tr, 0, sizeof(tr));

	VectorCopy(leader->client->ps.origin, start);
	start[2] += leader->client->ps.viewheight;

	VectorMA(start, 32768, dir, end);

	trap->Trace(&tr, start, NULL, NULL, end, leader->s.number, MASK_SHOT, qfalse, 0, 0);

	if (tr.surfaceFlags & SURF_SKY)
	{
		moveType = 2;
		return;
	}

	if (tr.entityNum < ENTITYNUM_WORLD && tr.entityNum != ENTITYNUM_NONE)
	{
		traceEnt = &g_entities[tr.entityNum];

		if (!traceEnt->inuse || !traceEnt->takedamage)
			traceEnt = NULL;
	}
	else
	{
		traceEnt = NULL;
	}

	if (!traceEnt)
	{
		VectorMA(tr.endpos, 12.0f, tr.plane.normal, goalOrg);
		moveType = (DotProduct(groundNormal, tr.plane.normal)<0.2f) ? 1 : 0;
	}
	else
	{
		moveType = 0;
	}

	if (!tr.allsolid)
		G_PlayEffectID(G_EffectIndex("misc/npcattack"), tr.endpos, tr.plane.normal);
}

extern void Boba_ForceFlyStart(gentity_t *self, int jetTime);
void NPCF_O_Attack(gentity_t *ent, qboolean init)
{
	if (init)
	{
		NPCF_Init_Attack(ent);

		G_SetAnim(ent->client->playerLeader, NULL, SETANIM_BOTH, BOTH_THERMAL_THROW, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART, 0);
		G_EntitySound(ent->client->playerLeader, CHAN_VOICE, G_SoundIndex(va("*attack%d.wav", Q_irand(1, 2))));
	}

	if (moveType == 2)
		return;

	if (moveType == 1 && !(ent->client->ps.eFlags2&EF2_FLYING))
	{
		if (ent->client->NPC_class == CLASS_BOBAFETT)
		{
			Boba_ForceFlyStart(ent, 20000); //calculate intelligently
		}
		else
		{
			return;
		}
	}

	SaveNPCGlobals();
	SetNPCGlobals(ent);

	if (!traceEnt)
	{
		if (!NPC_ValidEnemy(ent->enemy) || !NPC_TargetVisible(ent->enemy))
		{
			G_ClearEnemy(ent);
		}

		NPC_CheckEnemyExt(qfalse);

		NPC_SetMoveGoal(ent, goalOrg, 8, qtrue, ent->NPC->combatPoint, ent->enemy);
		ent->NPC->eventualGoal = ent->NPC->goalEntity;
		ent->NPC->defendEnt = NULL;
	}
	else
	{
		G_SetEnemy(ent, traceEnt);

		ent->NPC->defendEnt = ent->NPC->eventualGoal = NULL;
		ent->NPC->goalEntity = traceEnt;

		NPC_KyleChooseWeapon();
	}

	ent->NPC->defaultBehavior = ent->NPC->behaviorState = BS_HUNT_AND_KILL;

	ent->client->leader = ent->NPC->captureGoal = NULL;

	//big hack
	ent->NPC->scriptFlags &= ~(SCF_CROUCHED | SCF_WALKING);
	ent->NPC->scriptFlags |= (SCF_RUNNING | SCF_CHASE_ENEMIES | SCF_LOOK_FOR_ENEMIES | SCF_DONT_FLEE | SCF_USE_CP_NEAREST);

	NPC_RestoreStats(ent);

	RestoreNPCGlobals();
}
// add orders npc
void NPCF_O_StayHere(gentity_t *ent, qboolean init)
{
	if (init)
	{
		G_SetAnim(ent->client->playerLeader, NULL, SETANIM_BOTH, BOTH_FORCEGRIP3, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART, 0);
		G_EntitySound(ent->client->playerLeader, CHAN_VOICE, G_SoundIndex(va("*defense%d.wav", Q_irand(1, 2))));
	}

	NPC_SetMoveGoal(ent, ent->client->playerLeader->client->ps.origin, 8, qtrue, ent->NPC->combatPoint, ent->enemy);

	ent->NPC->defaultBehavior = ent->NPC->behaviorState = BS_PATROL;
	ent->NPC->defendEnt = ent->NPC->eventualGoal = ent->NPC->captureGoal = NULL;
	ent->client->leader = NULL;

	if (Q_irand(0, 2))
		ent->NPC->scriptFlags &= ~SCF_CROUCHED;
	else
		ent->NPC->scriptFlags |= SCF_CROUCHED;

	ent->NPC->scriptFlags &= ~(SCF_WALKING | SCF_DONT_FLEE);
	ent->NPC->scriptFlags |= (SCF_RUNNING | SCF_CHASE_ENEMIES | SCF_LOOK_FOR_ENEMIES | SCF_USE_CP_NEAREST);

	NPC_RestoreStats(ent);
}
// NPCMOD Jetpack - Bobafett still has to be fixed (Ineedblood)
void NPCF_O_JetpackGo(gentity_t *ent, qboolean init)
{
	if (ent->client->NPC_class != CLASS_BOBAFETT)
		return;

	if (!(ent->client->ps.eFlags2 & EF2_FLYING))
	{
		TIMER_Set(ent, "jetpackGoGo", Q_irand(300, 2000));
		ent->NPC->shouldJetOn = qtrue;
	}
	else
	{
		int t = ent->client->jetPackTime - (level.time + (FRAMETIME * 2));

		if (t < FRAMETIME)
			t = FRAMETIME;

		TIMER_Set(ent, "jetpackGoGo", t);
	}
}
