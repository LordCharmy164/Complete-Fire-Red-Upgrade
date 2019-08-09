#include "../defines.h"
#include "../../include/random.h"

#include "../../include/new/AI_Helper_Functions.h"
#include "../../include/new/ai_master.h"
#include "../../include/new/AI_scripts.h"
#include "../../include/new/battle_controller_opponent.h"
#include "../../include/new/battle_start_turn_start.h"
#include "../../include/new/damage_calc.h"
#include "../../include/new/frontier.h"
#include "../../include/new/general_bs_commands.h"
#include "../../include/new/Helper_Functions.h"
#include "../../include/new/mega.h"
#include "../../include/new/multi.h"
#include "../../include/new/move_tables.h"
#include "../../include/new/switching.h"

// AI states
enum
{
	AIState_SettingUp,
	AIState_Processing,
	AIState_FinishedProcessing,
	AIState_DoNotProcess,
};

struct SmartWildMons
{
	u16 species;
	u32 aiFlags;
};

struct SmartWildMons sSmartWildAITable[] =
{
	{SPECIES_ARTICUNO, AI_SCRIPT_CHECK_BAD_MOVE},
	{SPECIES_ZAPDOS, AI_SCRIPT_CHECK_BAD_MOVE},
	{SPECIES_MOLTRES, AI_SCRIPT_CHECK_BAD_MOVE},
	{SPECIES_MEWTWO, AI_SCRIPT_CHECK_BAD_MOVE},
	{SPECIES_MEW, AI_SCRIPT_CHECK_BAD_MOVE},
	{SPECIES_ENTEI, AI_SCRIPT_CHECK_BAD_MOVE},
	{SPECIES_RAIKOU, AI_SCRIPT_CHECK_BAD_MOVE},
	{SPECIES_SUICUNE, AI_SCRIPT_CHECK_BAD_MOVE},
	{SPECIES_LUGIA, AI_SCRIPT_CHECK_BAD_MOVE},
	{SPECIES_HO_OH, AI_SCRIPT_CHECK_BAD_MOVE},
	{SPECIES_CELEBI, AI_SCRIPT_CHECK_BAD_MOVE},
	{SPECIES_REGIROCK, AI_SCRIPT_CHECK_BAD_MOVE},
	{SPECIES_REGICE, AI_SCRIPT_CHECK_BAD_MOVE},
	{SPECIES_REGISTEEL, AI_SCRIPT_CHECK_BAD_MOVE},
	{SPECIES_LATIAS, AI_SCRIPT_CHECK_BAD_MOVE},
	{SPECIES_LATIOS, AI_SCRIPT_CHECK_BAD_MOVE},
	{SPECIES_KYOGRE, AI_SCRIPT_CHECK_BAD_MOVE},
	{SPECIES_GROUDON, AI_SCRIPT_CHECK_BAD_MOVE},
	{SPECIES_RAYQUAZA, AI_SCRIPT_CHECK_BAD_MOVE},
	{SPECIES_JIRACHI, AI_SCRIPT_CHECK_BAD_MOVE},
	{SPECIES_DEOXYS, AI_SCRIPT_CHECK_BAD_MOVE},
	{0xFFFF, 0}
};

static u8 (*const sBattleAIScriptTable[])(const u8, const u8, const u16, const u8) =
{
	[0] = AI_Script_Negatives,
	[1] = AI_Script_Positives,
	
	[29] = AI_Script_Roaming,
	[30] = AI_Script_Safari,
	[31] = AI_Script_FirstBattle,
};

//This file's functions:
static u8 ChooseMoveOrAction_Singles(void);
static u8 ChooseMoveOrAction_Doubles(void);
static void BattleAI_DoAIProcessing(void);
static bool8 ShouldSwitch(void);
static void LoadBattlersAndFoes(u8* battlerIn1, u8* battlerIn2, u8* foe1, u8* foe2);
static bool8 ShouldSwitchIfOnlyBadMovesLeft(void);
static bool8 FindMonThatAbsorbsOpponentsMove(void);
static bool8 ShouldSwitchIfNaturalCureOrRegenerator(void);
static bool8 PassOnWish(void);
static bool8 SemiInvulnerableTroll(void);
static u8 GetBestPartyNumberForSemiInvulnerableLockedMoveCalcs(u8 opposingBattler1, u8 opposingBattler2, bool8 checkLockedMoves);
static bool8 RunAllSemiInvulnerableLockedMoveCalcs(u8 opposingBattler1, u8 opposingBattler2, bool8 checkLockedMoves);
static bool8 TheCalcForSemiInvulnerableTroll(u8 bankAtk, u8 flags, bool8 checkLockedMoves);
static bool8 CanStopLockedMove(void);
static bool8 IsYawned(void);
static bool8 IsTakingAnnoyingSecondaryDamage(void);
static bool8 ShouldSwitchIfWonderGuard(void);
static void PredictMovesForBanks(void);
static void UpdateStrongestMoves(void);
static void UpdateBestDoublesKillingMoves(void);
static u32 GetMaxByteIndexInList(const u8 array[], const u32 size);

void __attribute__((long_call)) RecordLastUsedMoveByTarget(void);

void BattleAI_HandleItemUseBeforeAISetup(u8 defaultScoreMoves)
{
	u32 i;
	u8 *data = (u8*)BATTLE_HISTORY;

	for (i = 0; i < sizeof(struct BattleHistory); i++)
		data[i] = 0;

	// Items are allowed to use in ONLY trainer battles.
	if ((gBattleTypeFlags & BATTLE_TYPE_TRAINER)
		&& !(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_SAFARI | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TWO_OPPONENTS | BATTLE_TYPE_INGAME_PARTNER | BATTLE_TYPE_EREADER_TRAINER))
		&& gTrainerBattleOpponent_A != 0x400)
	{
		for (i = 0; i < 4; i++)
		{
			if (gTrainers[gTrainerBattleOpponent_A].items[i] != 0)
			{
				BATTLE_HISTORY->trainerItems[BATTLE_HISTORY->itemsNo] = gTrainers[gTrainerBattleOpponent_A].items[i];
				BATTLE_HISTORY->itemsNo++;
			}
		}
	}

	BattleAI_SetupAIData(defaultScoreMoves);
}

void BattleAI_SetupAIData(u8 defaultScoreMoves)
{
	u32 i;
	u8* data = (u8*) AI_THINKING_STRUCT;
	u8 moveLimitations;

	// Clear AI data.
	for (i = 0; i < sizeof(struct AI_ThinkingStruct); ++i)
		data[i] = 0;

	// Conditional score reset, unlike Ruby.
	for (i = 0; i < MAX_MON_MOVES; ++i)
	{
		if (defaultScoreMoves & 1)
			AI_THINKING_STRUCT->score[i] = 100;
		else
			AI_THINKING_STRUCT->score[i] = 0;

		defaultScoreMoves >>= 1;
	}

	moveLimitations = CheckMoveLimitations(gActiveBattler, 0, 0xFF);

	// Ignore moves that aren't possible to use.
	for (i = 0; i < MAX_MON_MOVES; i++)
	{
		if (gBitTable[i] & moveLimitations)
			AI_THINKING_STRUCT->score[i] = 0;

		AI_THINKING_STRUCT->simulatedRNG[i] = 100 - umodsi(Random(), 16);
	}

	gBattleResources->AI_ScriptsStack->size = 0;
	gBankAttacker = gActiveBattler;

	// Decide a random target battlerId in doubles.
	if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
	{
		gBankTarget = (Random() & BIT_FLANK) + (SIDE(gActiveBattler) ^ BIT_SIDE);
		if (gAbsentBattlerFlags & gBitTable[gBankTarget])
			gBankTarget ^= BIT_FLANK;
	}
	// There's only one choice in single battles.
	else
		gBankTarget = gBankAttacker ^ BIT_SIDE;

	// Choose proper trainer ai scripts.
	AI_THINKING_STRUCT->aiFlags = GetAIFlags();

	//if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
		//AI_THINKING_STRUCT->aiFlags |= AI_SCRIPT_DOUBLE_BATTLE; // act smart in doubles and don't attack your partner
}

u32 GetAIFlags(void)
{
	u32 flags;

	if (gBattleTypeFlags & BATTLE_TYPE_SAFARI)
		flags = AI_SCRIPT_SAFARI;
	else if (gBattleTypeFlags & BATTLE_TYPE_ROAMER)
		flags = AI_SCRIPT_ROAMING | WildMonIsSmart(gActiveBattler);
	else if (gBattleTypeFlags & BATTLE_TYPE_OAK_TUTORIAL)
		flags = AI_SCRIPT_FIRST_BATTLE;
	else if (gBattleTypeFlags & (BATTLE_TYPE_FRONTIER))
		flags = GetAIFlagsInBattleFrontier(gActiveBattler);
	else if (gBattleTypeFlags & (BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_TRAINER_TOWER) && gTrainerBattleOpponent_A != 0x400)
		flags = AI_SCRIPT_CHECK_BAD_MOVE | AI_SCRIPT_CHECK_GOOD_MOVE;
	else if (gBattleTypeFlags & BATTLE_TYPE_SCRIPTED_WILD_2) //No idea how these two work
		flags = AI_SCRIPT_CHECK_BAD_MOVE;
	else if (gBattleTypeFlags & BATTLE_TYPE_SCRIPTED_WILD_3) 
		flags = AI_SCRIPT_CHECK_BAD_MOVE | AI_SCRIPT_CHECK_GOOD_MOVE;
	else if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS)
		flags = gTrainers[gTrainerBattleOpponent_A].aiFlags | gTrainers[VarGet(SECOND_OPPONENT_VAR)].aiFlags;
	else
	   flags = gTrainers[gTrainerBattleOpponent_A].aiFlags;

	return flags;
}

u8 BattleAI_ChooseMoveOrAction(void)
{
	u16 savedCurrentMove = gCurrentMove;
	u8 ret;

	if (!(gBattleTypeFlags & BATTLE_TYPE_DOUBLE))
		ret = ChooseMoveOrAction_Singles();
	else
		ret = ChooseMoveOrAction_Doubles();

	gCurrentMove = savedCurrentMove;
	return ret;
}

static u8 ChooseMoveOrAction_Singles(void)
{
	u8 currentMoveArray[4];
	u8 consideredMoveArray[4];
	u8 numOfBestMoves;
	s32 i;
	u32 flags = AI_THINKING_STRUCT->aiFlags;
	
	RecordLastUsedMoveByTarget();

	while (flags != 0)
	{
		if (flags & 1)
		{
			AI_THINKING_STRUCT->aiState = AIState_SettingUp;
			BattleAI_DoAIProcessing();
		}
		flags >>= 1;
		AI_THINKING_STRUCT->aiLogicId++;
		AI_THINKING_STRUCT->movesetIndex = 0;
	}

	// Check special AI actions.
	if (AI_THINKING_STRUCT->aiAction & AI_ACTION_FLEE)
		return AI_CHOICE_FLEE;
	if (AI_THINKING_STRUCT->aiAction & AI_ACTION_WATCH)
		return AI_CHOICE_WATCH;

	numOfBestMoves = 1;
	currentMoveArray[0] = AI_THINKING_STRUCT->score[0];
	consideredMoveArray[0] = 0;

	for (i = 1; i < MAX_MON_MOVES; i++)
	{
		if (gBattleMons[gBankAttacker].moves[i] != MOVE_NONE)
		{
			// In ruby, the order of these if statements is reversed.
			if (currentMoveArray[0] == AI_THINKING_STRUCT->score[i])
			{
				currentMoveArray[numOfBestMoves] = AI_THINKING_STRUCT->score[i];
				consideredMoveArray[numOfBestMoves++] = i;
			}
			if (currentMoveArray[0] < AI_THINKING_STRUCT->score[i])
			{
				numOfBestMoves = 1;
				currentMoveArray[0] = AI_THINKING_STRUCT->score[i];
				consideredMoveArray[0] = i;
			}
		}
	}

	return consideredMoveArray[Random() % numOfBestMoves];
}

static u8 ChooseMoveOrAction_Doubles(void)
{
	s32 i;
	s32 j;
	u32 flags;
	s16 bestMovePointsForTarget[4];
	s8 mostViableTargetsArray[4];
	u8 actionOrMoveIndex[4];
	u8 mostViableMovesScores[4];
	u8 mostViableMovesIndices[4];
	s32 mostViableTargetsNo;
	s32 mostViableMovesNo;
	s16 mostMovePoints;

	for (i = 0; i < MAX_BATTLERS_COUNT; i++)
	{
		if (i == gBankAttacker || gBattleMons[i].hp == 0)
		{
			actionOrMoveIndex[i] = 0xFF;
			bestMovePointsForTarget[i] = -1;
		}
		else
		{
			BattleAI_SetupAIData(0xF);

			gBattlerTarget = i;

			if ((i & BIT_SIDE) != (gBankAttacker & BIT_SIDE))
				RecordLastUsedMoveByTarget();

			AI_THINKING_STRUCT->aiLogicId = 0;
			AI_THINKING_STRUCT->movesetIndex = 0;
			flags = AI_THINKING_STRUCT->aiFlags;
			while (flags != 0)
			{
				if (flags & 1)
				{
					AI_THINKING_STRUCT->aiState = AIState_SettingUp;
					BattleAI_DoAIProcessing();
				}

				flags >>= 1;
				AI_THINKING_STRUCT->aiLogicId++;
				AI_THINKING_STRUCT->movesetIndex = 0;
			}

			if (AI_THINKING_STRUCT->aiAction & AI_ACTION_FLEE)
			{
				actionOrMoveIndex[i] = AI_CHOICE_FLEE;
			}
			else if (AI_THINKING_STRUCT->aiAction & AI_ACTION_WATCH)
			{
				actionOrMoveIndex[i] = AI_CHOICE_WATCH;
			}
			else
			{
				mostViableMovesScores[0] = AI_THINKING_STRUCT->score[0];
				mostViableMovesIndices[0] = 0;
				mostViableMovesNo = 1;
				for (j = 1; j < MAX_MON_MOVES; j++)
				{
					if (gBattleMons[gBankAttacker].moves[j] != 0)
					{
						if (mostViableMovesScores[0] == AI_THINKING_STRUCT->score[j])
						{
							mostViableMovesScores[mostViableMovesNo] = AI_THINKING_STRUCT->score[j];
							mostViableMovesIndices[mostViableMovesNo] = j;
							mostViableMovesNo++;
						}
						if (mostViableMovesScores[0] < AI_THINKING_STRUCT->score[j])
						{
							mostViableMovesScores[0] = AI_THINKING_STRUCT->score[j];
							mostViableMovesIndices[0] = j;
							mostViableMovesNo = 1;
						}
					}
				}
				
				actionOrMoveIndex[i] = mostViableMovesIndices[Random() % mostViableMovesNo];
				bestMovePointsForTarget[i] = mostViableMovesScores[0];

				// Don't use a move against ally if it has less than 101 points.
				if (i == (gBankAttacker ^ BIT_FLANK) && bestMovePointsForTarget[i] < 101)
				{
					bestMovePointsForTarget[i] = -1;
				}
			}
		}
	}

	mostMovePoints = bestMovePointsForTarget[0];
	mostViableTargetsArray[0] = 0;
	mostViableTargetsNo = 1;

	for (i = 1; i < MAX_MON_MOVES; i++)
	{
		if (mostMovePoints == bestMovePointsForTarget[i])
		{
			mostViableTargetsArray[mostViableTargetsNo] = i;
			mostViableTargetsNo++;
		}
		if (mostMovePoints < bestMovePointsForTarget[i])
		{
			mostMovePoints = bestMovePointsForTarget[i];
			mostViableTargetsArray[0] = i;
			mostViableTargetsNo = 1;
		}
	}

	gBankTarget = mostViableTargetsArray[Random() % mostViableTargetsNo];
	return actionOrMoveIndex[gBankTarget];
}

static void BattleAI_DoAIProcessing(void)
{
	while (AI_THINKING_STRUCT->aiState != AIState_FinishedProcessing)
	{
		switch (AI_THINKING_STRUCT->aiState)
		{
			case AIState_DoNotProcess: // Needed to match.
				break;
			case AIState_SettingUp:
				if (gBattleMons[gBankAttacker].pp[AI_THINKING_STRUCT->movesetIndex] == 0)
					AI_THINKING_STRUCT->moveConsidered = MOVE_NONE;
				else
					AI_THINKING_STRUCT->moveConsidered = gBattleMons[gBankAttacker].moves[AI_THINKING_STRUCT->movesetIndex];

				AI_THINKING_STRUCT->aiState++;
				break;

			case AIState_Processing:
				if (AI_THINKING_STRUCT->moveConsidered != MOVE_NONE
				&&  AI_THINKING_STRUCT->score[AI_THINKING_STRUCT->movesetIndex] > 0)
				{
					if (AI_THINKING_STRUCT->aiLogicId < ARRAY_COUNT(sBattleAIScriptTable) //AI Script exists
					&&  sBattleAIScriptTable[AI_THINKING_STRUCT->aiLogicId] != NULL)
					{
						AI_THINKING_STRUCT->score[AI_THINKING_STRUCT->movesetIndex] = 
							sBattleAIScriptTable[AI_THINKING_STRUCT->aiLogicId](gBankAttacker, 
																				gBankTarget, 
																				AI_THINKING_STRUCT->moveConsidered, 
																				AI_THINKING_STRUCT->score[AI_THINKING_STRUCT->movesetIndex]); //Run AI script
					}
				}
				else
				{
					AI_THINKING_STRUCT->score[AI_THINKING_STRUCT->movesetIndex] = 0;
				}

				AI_THINKING_STRUCT->movesetIndex++;

				if (AI_THINKING_STRUCT->movesetIndex < MAX_MON_MOVES && !(AI_THINKING_STRUCT->aiAction & AI_ACTION_DO_NOT_ATTACK))
					AI_THINKING_STRUCT->aiState = AIState_SettingUp;
				else
					AI_THINKING_STRUCT->aiState++;

			break;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AI_TrySwitchOrUseItem(void)
{
	struct Pokemon* party;
	u8 battlerIn1, battlerIn2;
	u8 firstId, lastId;

	//Calulate everything important now to save as much processing time as possible later
	if (!gNewBS->calculatedAIPredictions) //Only calculate these things once per turn
	{
		UpdateStrongestMoves();
		UpdateBestDoublesKillingMoves();
		PredictMovesForBanks();
		
		gNewBS->calculatedAIPredictions = TRUE;
	}

	if (!gNewBS->calculatedAISwitchings[gActiveBattler]) //So Multi Battles still work properly
	{
		CalcMostSuitableMonToSwitchInto();
		gNewBS->calculatedAISwitchings[gActiveBattler] = TRUE;
		
		if ((SIDE(gActiveBattler) == B_SIDE_OPPONENT && !IsTwoOpponentBattle())
		||  (SIDE(gActiveBattler) == B_SIDE_PLAYER && !IsTagBattle()))
			gNewBS->calculatedAISwitchings[PARTNER(gActiveBattler)] = TRUE;
	}

	party = LoadPartyRange(gActiveBattler, &firstId, &lastId);

	if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
	{
		if (ShouldSwitch()) //0x8039A80
		{
			if (gBattleStruct->switchoutIndex[SIDE(gActiveBattler)] == PARTY_SIZE)
			{
				u8 monToSwitchId = GetMostSuitableMonToSwitchInto();
				if (monToSwitchId == PARTY_SIZE)
				{
					if (!(gBattleTypeFlags & BATTLE_TYPE_DOUBLE))
					{
						battlerIn1 = gActiveBattler;
						battlerIn2 = battlerIn1;
					}
					else
					{
						battlerIn1 = gActiveBattler;
						if (gAbsentBattlerFlags & gBitTable[PARTNER(gActiveBattler)])
							battlerIn2 = battlerIn1;
						else
							battlerIn2 = PARTNER(battlerIn1);
					}

					for (monToSwitchId = firstId; monToSwitchId < lastId; ++monToSwitchId)
					{
						if (party[monToSwitchId].hp == 0
						||  GetMonData(&party[monToSwitchId], MON_DATA_IS_EGG, 0)
						||  monToSwitchId == gBattlerPartyIndexes[battlerIn1]
						||	monToSwitchId == gBattlerPartyIndexes[battlerIn2]
						||	monToSwitchId == gBattleStruct->monToSwitchIntoId[battlerIn1]
						||	monToSwitchId == gBattleStruct->monToSwitchIntoId[battlerIn2])
							continue;
						
						break;
					}
				}

				gBattleStruct->switchoutIndex[SIDE(gActiveBattler)] = monToSwitchId;
			}

			gBattleStruct->monToSwitchIntoId[gActiveBattler] = gBattleStruct->switchoutIndex[SIDE(gActiveBattler)];				
			return;
		}
		else if (ShouldUseItem()) //0x803A1F4
			return;
	}

	EmitTwoReturnValues(1, ACTION_USE_MOVE, (gActiveBattler ^ BIT_SIDE) << 8);
}

static bool8 ShouldSwitch(void)
{
	u8 battlerIn1, battlerIn2;
	u8 firstId, lastId;
	u8 availableToSwitch;
	struct Pokemon *party;
	int i;

	if (IsTrapped(gActiveBattler, TRUE))
		return FALSE;
	
	availableToSwitch = 0;
	if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
	{
		battlerIn1 = gActiveBattler;
		if (gAbsentBattlerFlags & gBitTable[GetBattlerAtPosition(GetBattlerPosition(gActiveBattler) ^ BIT_FLANK)])
			battlerIn2 = gActiveBattler;
		else
			battlerIn2 = GetBattlerAtPosition(GetBattlerPosition(gActiveBattler) ^ BIT_FLANK);
	}
	else
	{
		battlerIn1 = gActiveBattler;
		battlerIn2 = gActiveBattler;
	}

	party = LoadPartyRange(gActiveBattler, &firstId, &lastId);

	for (i = firstId; i < lastId; ++i)
	{
		if (party[i].hp == 0
		||	GetMonData(&party[i], MON_DATA_SPECIES2, 0) == SPECIES_NONE
		|| 	GetMonData(&party[i], MON_DATA_IS_EGG, 0)
		||	i == gBattlerPartyIndexes[battlerIn1]
		||	i == gBattlerPartyIndexes[battlerIn2]
		||	i == gBattleStruct->monToSwitchIntoId[battlerIn1]
		||  i == gBattleStruct->monToSwitchIntoId[battlerIn2])
			continue;

		++availableToSwitch;
	}
	
	if (availableToSwitch == 0)
		return FALSE;
	if (ShouldSwitchIfOnlyBadMovesLeft())
		return TRUE;
	if (ShouldSwitchIfPerishSong())
		return TRUE;
	if (ShouldSwitchIfWonderGuard())
		return TRUE;
	if (FindMonThatAbsorbsOpponentsMove())
		return TRUE;
	if (ShouldSwitchIfNaturalCureOrRegenerator())
		return TRUE;
	if (PassOnWish())
		return TRUE;
	if (SemiInvulnerableTroll())
		return TRUE;
	if (CanStopLockedMove())
		return TRUE;
	if (IsYawned())
		return TRUE;
	if (IsTakingAnnoyingSecondaryDamage())
		return TRUE;
	//if (CanKillAFoe(gActiveBattler))
	//	return FALSE;
	//if (AreStatsRaised())
	//	return FALSE;

	return FALSE;
}

static void LoadBattlersAndFoes(u8* battlerIn1, u8* battlerIn2, u8* foe1, u8* foe2)
{
	if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
	{
		*battlerIn1 = gActiveBattler;
		if (gAbsentBattlerFlags & gBitTable[PARTNER(gActiveBattler)])
			*battlerIn2 = gActiveBattler;
		else
			*battlerIn2 = PARTNER(gActiveBattler);
			
		if (gAbsentBattlerFlags & gBitTable[FOE(gActiveBattler)])
			*foe1 = *foe2 = PARTNER(FOE(gActiveBattler));
		else if (gAbsentBattlerFlags & gBitTable[PARTNER(FOE(gActiveBattler))])
			*foe1 = *foe2 = FOE(gActiveBattler);
		else
		{
			*foe1 = FOE(gActiveBattler);
			*foe2 = PARTNER(FOE(gActiveBattler));
		}
	}
	else
	{
		*battlerIn1 = gActiveBattler;
		*battlerIn2 = gActiveBattler;
		*foe1 = FOE(gActiveBattler);
		*foe2 = *foe1;
	}
}

static bool8 ShouldSwitchIfOnlyBadMovesLeft(void)
{
	u8 battlerIn1, battlerIn2;
	u8 foe1, foe2;
	LoadBattlersAndFoes(&battlerIn1, &battlerIn2, &foe1, &foe2);

	if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
	{
		if ((!BATTLER_ALIVE(foe1) || OnlyBadMovesLeftInMoveset(battlerIn1, foe1))
		&&  (!BATTLER_ALIVE(foe2) || OnlyBadMovesLeftInMoveset(battlerIn1, foe2)))
		{
			gBattleStruct->switchoutIndex[SIDE(gActiveBattler)] = PARTY_SIZE;
			EmitTwoReturnValues(1, ACTION_SWITCH, 0);
			return TRUE;
		}
	}
	else
	{
		if (OnlyBadMovesLeftInMoveset(battlerIn1, foe1))
		{
			gBattleStruct->switchoutIndex[SIDE(gActiveBattler)] = PARTY_SIZE;
			EmitTwoReturnValues(1, ACTION_SWITCH, 0);
			return TRUE;
		}
	}
	
	return FALSE;
}

static bool8 FindMonThatAbsorbsOpponentsMove(void)
{
	u8 battlerIn1, battlerIn2;
	u8 foe1, foe2;
	u16 predictedMove1, predictedMove2;
	u8 absorbingTypeAbility1, absorbingTypeAbility2, absorbingTypeAbility3;
	u8 firstId, lastId;
	struct Pokemon *party;
	int i;

	LoadBattlersAndFoes(&battlerIn1, &battlerIn2, &foe1, &foe2);

	predictedMove1 = IsValidMovePrediction(foe1, gActiveBattler);
	predictedMove2 = IsValidMovePrediction(foe2, gActiveBattler);

	if ((CanKillAFoe(gActiveBattler) && (Random() & 1) != 0)
	|| ((predictedMove1 == MOVE_NONE || predictedMove1 == MOVE_PREDICTION_SWITCH) && (predictedMove2 == MOVE_NONE || predictedMove2 == MOVE_PREDICTION_SWITCH))
	|| (SPLIT(predictedMove1) == SPLIT_STATUS && SPLIT(predictedMove2) == SPLIT_STATUS))
		return FALSE;

	u8 moveType;
	if (predictedMove1 != MOVE_NONE && predictedMove1 != MOVE_PREDICTION_SWITCH)
		moveType = GetMoveTypeSpecial(foe1, predictedMove1);
	else
		moveType = GetMoveTypeSpecial(foe2, predictedMove2);

	switch (moveType) {
		case TYPE_FIRE:
			absorbingTypeAbility1 = ABILITY_FLASHFIRE;
			absorbingTypeAbility2 = ABILITY_FLASHFIRE;
			absorbingTypeAbility3 = ABILITY_FLASHFIRE;
			break;
		case TYPE_ELECTRIC:
			absorbingTypeAbility1 = ABILITY_VOLTABSORB;
			absorbingTypeAbility2 = ABILITY_LIGHTNINGROD;
			absorbingTypeAbility3 = ABILITY_MOTORDRIVE;
			break;
		case TYPE_WATER:
			absorbingTypeAbility1 = ABILITY_WATERABSORB;
			absorbingTypeAbility2 = ABILITY_DRYSKIN;
			absorbingTypeAbility3 = ABILITY_STORMDRAIN;
			break;
		case TYPE_GRASS:
			absorbingTypeAbility1 = ABILITY_SAPSIPPER;
			absorbingTypeAbility2 = ABILITY_SAPSIPPER;
			absorbingTypeAbility3 = ABILITY_SAPSIPPER;
			break;
		default:
			return FALSE;
	}
	
	
	u8 atkAbility = GetPredictedAIAbility(gActiveBattler, foe1);
	if (atkAbility == absorbingTypeAbility1
	||  atkAbility == absorbingTypeAbility2
	||  atkAbility == absorbingTypeAbility3)
		return FALSE;

	party = LoadPartyRange(gActiveBattler, &firstId, &lastId);

	for (i = firstId; i < lastId; i++)
	{
		u16 species = party[i].species;
		u8 monAbility = GetPartyAbility(&party[i]);

		if (party[i].hp == 0
		||  species == SPECIES_NONE
		||	GetMonData(&party[i], MON_DATA_IS_EGG, 0)
		||	i == gBattlerPartyIndexes[battlerIn1]
		||	i == gBattlerPartyIndexes[battlerIn2]
		||	i == gBattleStruct->monToSwitchIntoId[battlerIn1]
		||	i == gBattleStruct->monToSwitchIntoId[battlerIn2])
			continue;

		if (monAbility == absorbingTypeAbility1
		||  monAbility == absorbingTypeAbility2
		||  monAbility == absorbingTypeAbility3)
		{
			// we found a mon.
			gBattleStruct->switchoutIndex[SIDE(gActiveBattler)] = i;
			EmitTwoReturnValues(1, ACTION_SWITCH, 0);
			return TRUE;
		}
	}

	return FALSE;
}

static bool8 ShouldSwitchIfNaturalCureOrRegenerator(void)
{
	u8 battlerIn1, battlerIn2;
	u8 foe1, foe2;
	u16 aiMovePrediction;
	u16 foe1MovePrediction, foe2MovePrediction;

	LoadBattlersAndFoes(&battlerIn1, &battlerIn2, &foe1, &foe2);

	switch (ABILITY(gActiveBattler)) {
		case ABILITY_NATURALCURE:
			if (WillTakeSignificantDamageFromEntryHazards(gActiveBattler, 4))
				return FALSE; //Don't switch out if you'll take a lot of damage of switch in

			if (gBattleMons[gActiveBattler].status1 & (STATUS1_SLEEP | STATUS1_FREEZE))
				break;
			if (gBattleMons[gActiveBattler].status1 //Has regular status and over half health
			&& gBattleMons[gActiveBattler].hp >= gBattleMons[gActiveBattler].maxHP / 2)
				break;
			return FALSE;

		//Try switch if less than half health, enemy can kill, and mon can't kill enemy first
		case ABILITY_REGENERATOR:
			if (gBattleMons[gActiveBattler].hp > gBattleMons[gActiveBattler].maxHP / 2)
				return FALSE;
				
			if (WillTakeSignificantDamageFromEntryHazards(gActiveBattler, 3))
				return FALSE; //Don't switch out if you'll lose more than you gain by switching out here
				
			foe1MovePrediction = IsValidMovePrediction(foe1, gActiveBattler);
			foe2MovePrediction = IsValidMovePrediction(foe2, gActiveBattler);

			if ((BATTLER_ALIVE(foe1) && foe1MovePrediction != MOVE_NONE && MoveKnocksOutXHits(foe1MovePrediction, foe1, gActiveBattler, 1)) //Foe can kill AI
			|| (IsDoubleBattle() && BATTLER_ALIVE(foe2) && foe2MovePrediction != MOVE_NONE && MoveKnocksOutXHits(foe2MovePrediction, foe2, gActiveBattler, 1)))
			{
				if (BATTLER_ALIVE(foe1))
				{
					aiMovePrediction = IsValidMovePrediction(gActiveBattler, foe1);
					if (aiMovePrediction != MOVE_NONE && MoveWouldHitFirst(aiMovePrediction, gActiveBattler, foe1) && MoveKnocksOutXHits(aiMovePrediction, gActiveBattler, foe1, 1))
						return FALSE; //Don't switch if can knock out enemy first or enemy can't kill
					else
						break;
				}

				if (IsDoubleBattle() && BATTLER_ALIVE(foe2))
				{
					aiMovePrediction = IsValidMovePrediction(gActiveBattler, foe2);
					if (aiMovePrediction != MOVE_NONE && MoveWouldHitFirst(aiMovePrediction, gActiveBattler, foe2) && MoveKnocksOutXHits(aiMovePrediction, gActiveBattler, foe2, 1))
						return FALSE; //Don't switch if can knock out enemy first or enemy can't kill
					else
						break;
				}
			}

			return FALSE;
		
		default:
			return FALSE;
	}

	gBattleStruct->switchoutIndex[SIDE(gActiveBattler)] = PARTY_SIZE;
	EmitTwoReturnValues(1, ACTION_SWITCH, 0);
	return TRUE;
}

bool8 HasSuperEffectiveMoveAgainstOpponents(bool8 noRng)
{
	u8 opposingBattler;
	int i;
	u8 moveFlags;
	u16 move;

	opposingBattler = FOE(gActiveBattler);
	if (!(gAbsentBattlerFlags & gBitTable[opposingBattler]))
	{
		for (i = 0; i < MAX_MON_MOVES; ++i)
		{
			move = gBattleMons[gActiveBattler].moves[i];
			if (move == MOVE_NONE || SPLIT(move) == SPLIT_STATUS)
				continue;

			moveFlags = AI_SpecialTypeCalc(move, gActiveBattler, opposingBattler);
			
			if (moveFlags & MOVE_RESULT_SUPER_EFFECTIVE)
			{
				if (noRng)
					return TRUE;
				if (umodsi(Random(), 10) != 0)
					return TRUE;
			}
		}
	}

	if (!(gBattleTypeFlags & BATTLE_TYPE_DOUBLE))
		return FALSE;

	opposingBattler = PARTNER(FOE(gActiveBattler));
	if (!(gAbsentBattlerFlags & gBitTable[opposingBattler]))
	{	
		for (i = 0; i < MAX_MON_MOVES; ++i)
		{
			move = gBattleMons[gActiveBattler].moves[i];
			if (move == MOVE_NONE || SPLIT(move) == SPLIT_STATUS)
				continue;

			moveFlags = AI_SpecialTypeCalc(move, gActiveBattler, opposingBattler);
			
			if (moveFlags & MOVE_RESULT_SUPER_EFFECTIVE)
			{
				if (noRng)
					return TRUE;
				if (umodsi(Random(), 10) != 0)
					return TRUE;
			}
		}
	}

	return FALSE;
}

static bool8 PassOnWish(void)
{
	pokemon_t* party;
	u8 firstId, lastId;
	u8 bank = gActiveBattler;
	u8 battlerIn1, battlerIn2;
	u8 opposingBattler1, opposingBattler2;
	int i;
	
	if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
	{
		opposingBattler1 = FOE(bank);
		opposingBattler2 = PARTNER(opposingBattler1);
		battlerIn1 = gActiveBattler;
		if (gAbsentBattlerFlags & gBitTable[PARTNER(gActiveBattler)])
			battlerIn2 = gActiveBattler;
		else
			battlerIn2 = PARTNER(gActiveBattler);
	}
	else
	{
		opposingBattler1 = FOE(bank);
		opposingBattler2 = opposingBattler1;
		battlerIn1 = gActiveBattler;
		battlerIn2 = gActiveBattler;
	}
		
	if (gWishFutureKnock->wishCounter[bank])
	{	
		//Don't switch if your health is less than half and you can survive an opponent's hit
		if (gBattleMons[bank].hp < gBattleMons[bank].maxHP / 2
		&& ((!CanKnockOut(opposingBattler1, bank) && !(gBattleTypeFlags & BATTLE_TYPE_DOUBLE && CanKnockOut(opposingBattler2, bank))) || HasProtectionMoveInMoveset(bank, CHECK_MAT_BLOCK)))
			return FALSE;
			
		party = LoadPartyRange(gActiveBattler, &firstId, &lastId);
		
		for (i = firstId; i < lastId; ++i)
		{
			if (party[i].hp == 0
			||  party[i].species == SPECIES_NONE
			||	GetMonData(&party[i], MON_DATA_IS_EGG, 0)
			||	i == gBattlerPartyIndexes[battlerIn1]
			||	i == gBattlerPartyIndexes[battlerIn2]
			||	i == gBattleStruct->monToSwitchIntoId[battlerIn1]
			||	i == gBattleStruct->monToSwitchIntoId[battlerIn2])
				continue;

			if (party[i].hp < party[i].maxHP / 2)
			{
				gBattleStruct->switchoutIndex[SIDE(bank)] = i;
				EmitTwoReturnValues(1, ACTION_SWITCH, 0);
				return TRUE;
			}
		}
	}
	return FALSE;
}

static bool8 SemiInvulnerableTroll(void)
{
	u8 opposingBattler1, opposingBattler2;

	if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
	{
		opposingBattler1 = FOE(gActiveBattler);
		opposingBattler2 = PARTNER(opposingBattler1);
	}
	else
	{
		opposingBattler1 = FOE(gActiveBattler);
		opposingBattler2 = opposingBattler1;
	}
	
	if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
	{
		if (!(gStatuses3[opposingBattler1] & STATUS3_SEMI_INVULNERABLE) && !(gStatuses3[opposingBattler2] & STATUS3_SEMI_INVULNERABLE))
			return FALSE;
		#ifndef REALLY_SMART_AI
			if (ViableMonCountFromBank(gActiveBattler) > 1)
				return FALSE; //AI doesn't know which mon is being targeted if there's more than one on the field
		#endif
	}
	else if (!(gStatuses3[opposingBattler1] & STATUS3_SEMI_INVULNERABLE))
		return FALSE;
	
	if (RunAllSemiInvulnerableLockedMoveCalcs(opposingBattler1, opposingBattler2, FALSE))
		return TRUE;
	
	return FALSE;
}

static u8 GetBestPartyNumberForSemiInvulnerableLockedMoveCalcs(u8 opposingBattler1, u8 opposingBattler2, bool8 checkLockedMoves)
{
	u8 option1 = TheCalcForSemiInvulnerableTroll(opposingBattler1, MOVE_RESULT_DOESNT_AFFECT_FOE, checkLockedMoves);
	if (option1 != PARTY_SIZE)
	{
		return option1;
	}
	
	if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
	{
		u8 option2 = TheCalcForSemiInvulnerableTroll(opposingBattler2, MOVE_RESULT_DOESNT_AFFECT_FOE, checkLockedMoves);
		if (option2 != PARTY_SIZE)
		{
			return option2;
		}	
	}
	
	u8 option3 = TheCalcForSemiInvulnerableTroll(opposingBattler1, MOVE_RESULT_NOT_VERY_EFFECTIVE, checkLockedMoves);
	if (option3 != PARTY_SIZE)
	{
		return option3;
	}
	
	if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
	{
		u8 option4 = TheCalcForSemiInvulnerableTroll(opposingBattler2, MOVE_RESULT_NOT_VERY_EFFECTIVE, checkLockedMoves);
		if (option4 != PARTY_SIZE)
		{
			return option4;
		}
	}
	
	return PARTY_SIZE;
}

static bool8 RunAllSemiInvulnerableLockedMoveCalcs(u8 opposingBattler1, u8 opposingBattler2, bool8 checkLockedMoves)
{
	u8 option = GetBestPartyNumberForSemiInvulnerableLockedMoveCalcs(opposingBattler1, opposingBattler2, checkLockedMoves);
	if (option != PARTY_SIZE)
	{
		gBattleStruct->switchoutIndex[SIDE(gActiveBattler)] = option;
		EmitTwoReturnValues(1, ACTION_SWITCH, 0);
	}

	return FALSE;
}

static u8 TheCalcForSemiInvulnerableTroll(u8 bankAtk, u8 flags, bool8 checkLockedMoves)
{
	u8 firstId, lastId;
	u8 battlerIn1, battlerIn2;
	int i;

	if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
	{
		battlerIn1 = gActiveBattler;
		if (gAbsentBattlerFlags & gBitTable[PARTNER(gActiveBattler)])
			battlerIn2 = gActiveBattler;
		else
			battlerIn2 = PARTNER(gActiveBattler);
	}
	else
	{
		battlerIn1 = gActiveBattler;
		battlerIn2 = gActiveBattler;
	}
	
	pokemon_t* party = LoadPartyRange(gActiveBattler, &firstId, &lastId);

	if (((gStatuses3[bankAtk] & STATUS3_SEMI_INVULNERABLE) || (checkLockedMoves && gLockedMoves[bankAtk] != MOVE_NONE)) 
	&&	gBattleStruct->moveTarget[bankAtk] == gActiveBattler)
	{
		u8 newFlags = AI_SpecialTypeCalc(gLockedMoves[bankAtk], bankAtk, gActiveBattler);
		if (BATTLER_ALIVE(gActiveBattler)) //Not end turn switch
		{
			if (newFlags & flags) //Target already has these type flags so why switch?
				return PARTY_SIZE;
		}

		for (i = firstId; i < lastId; ++i)
		{
			if (party[i].hp == 0
			||  party[i].species == SPECIES_NONE
			||	GetMonData(&party[i], MON_DATA_IS_EGG, 0)
			||	i == gBattlerPartyIndexes[battlerIn1]
			||	i == gBattlerPartyIndexes[battlerIn2]
			||	i == gBattleStruct->monToSwitchIntoId[battlerIn1]
			||	i == gBattleStruct->monToSwitchIntoId[battlerIn2])
				continue;

			if (AI_TypeCalc(gLockedMoves[bankAtk], bankAtk, &party[i]) & flags)
			{
				return i;
			}
		}
	}

	return PARTY_SIZE;
}

static bool8 CanStopLockedMove(void)
{
	u8 opposingBattler1, opposingBattler2;

	if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
	{
		opposingBattler1 = FOE(gActiveBattler);
		opposingBattler2 = PARTNER(opposingBattler1);
	}
	else
	{
		opposingBattler1 = FOE(gActiveBattler);
		opposingBattler2 = opposingBattler1;
	}
	
	if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
	{
		if (!(gLockedMoves[opposingBattler1] && SPLIT(gLockedMoves[opposingBattler1]) != SPLIT_STATUS) && !(gLockedMoves[opposingBattler2] && SPLIT(gLockedMoves[opposingBattler2]) != SPLIT_STATUS))
			return FALSE;
		#ifndef REALLY_SMART_AI
			if (ViableMonCountFromBank(gActiveBattler) > 1)
				return FALSE; //AI doesn't know which mon is being targeted if there's more than one on the field
		#endif
	}
	else if (!(gLockedMoves[opposingBattler1] && SPLIT(gLockedMoves[opposingBattler1]) != SPLIT_STATUS))
		return FALSE;
	
	if (RunAllSemiInvulnerableLockedMoveCalcs(opposingBattler1, opposingBattler2, TRUE))
		return TRUE;

	return FALSE;
}

static bool8 IsYawned(void)
{
	if (ABILITY(gActiveBattler) != ABILITY_NATURALCURE
	&& gStatuses3[gActiveBattler] & STATUS3_YAWN
	&& CanBePutToSleep(gActiveBattler, FALSE) //Could have been yawned and then afflicted with another status condition
	&& gBattleMons[gActiveBattler].hp > gBattleMons[gActiveBattler].maxHP / 4)
	{
		for (int i = 0; i < gBattlersCount; ++i)
		{
			if (SIDE(i) != SIDE(gActiveBattler))
			{
				if (gBattleMons[i].status2 & STATUS2_WRAPPED
				&& ABILITY(i) != ABILITY_MAGICGUARD //Taking trap damage
				&& gBattleStruct->wrappedBy[i] == gActiveBattler)
					return FALSE; //Don't switch if there's an enemy taking trap damage from this mon
			}
		}
	
		gBattleStruct->switchoutIndex[SIDE(gActiveBattler)] = PARTY_SIZE;
		EmitTwoReturnValues(1, ACTION_SWITCH, 0);
		return TRUE;
	}
	return FALSE;
}

static bool8 IsTakingAnnoyingSecondaryDamage(void)
{
	if (GetPredictedAIAbility(gActiveBattler, FOE(gActiveBattler)) != ABILITY_MAGICGUARD
	&& !CanKillAFoe(gActiveBattler)
	&& AI_THINKING_STRUCT->aiFlags > AI_SCRIPT_CHECK_BAD_MOVE) //Has smart AI
	{
		if (gStatuses3[gActiveBattler] & STATUS3_LEECHSEED
		|| ((gBattleMons[gActiveBattler].status1 & STATUS1_SLEEP) > 1 && gBattleMons[gActiveBattler].status2 & STATUS2_NIGHTMARE)
		||  gBattleMons[gActiveBattler].status2 & (STATUS2_CURSED))
		{
			gBattleStruct->switchoutIndex[SIDE(gActiveBattler)] = PARTY_SIZE;
			EmitTwoReturnValues(1, ACTION_SWITCH, 0);
			return TRUE;
		}
	}

	return FALSE;
}

//Most likely this function won't get used anymore. GetMostSuitableMonToSwitchInto
//now handles all of the switching checks.
bool8 FindMonWithFlagsAndSuperEffective(u8 flags, u8 moduloPercent)
{
	u8 battlerIn1, battlerIn2;
	u8 foe1, foe2;
	int i, j;
	u8 start, end;
	u16 move;
	u8 moveFlags;

	if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
	{
		battlerIn1 = gActiveBattler;
		if (gAbsentBattlerFlags & gBitTable[PARTNER(gActiveBattler)])
			battlerIn2 = gActiveBattler;
		else
			battlerIn2 = PARTNER(gActiveBattler);
			
		if (gAbsentBattlerFlags & gBitTable[FOE(gActiveBattler)])
			foe1 = foe2 = PARTNER(FOE(gActiveBattler));
		else if (gAbsentBattlerFlags & gBitTable[PARTNER(FOE(gActiveBattler))])
			foe1 = foe2 = FOE(gActiveBattler);
		else
		{
			foe1 = FOE(gActiveBattler);
			foe2 = PARTNER(FOE(gActiveBattler));
		}
	}
	else
	{
		battlerIn1 = gActiveBattler;
		battlerIn2 = gActiveBattler;
		foe1 = FOE(gActiveBattler);
		foe2 = foe1;
	}

	u16 predictedMove1 = IsValidMovePrediction(foe1, gActiveBattler);
	u16 predictedMove2 = IsValidMovePrediction(foe2, gActiveBattler);
	
	if (((predictedMove1 == MOVE_NONE || predictedMove1 == MOVE_PREDICTION_SWITCH) && (predictedMove2 == MOVE_NONE || predictedMove2 == MOVE_PREDICTION_SWITCH))
	|| (SPLIT(predictedMove1) == SPLIT_STATUS && SPLIT(predictedMove2) == SPLIT_STATUS))
		return FALSE;

	pokemon_t* party = LoadPartyRange(gActiveBattler, &start, &end);

//Party Search			
	for (i = start; i < end; ++i)
	{
		if (party[i].hp == 0
		|| party[i].species == SPECIES_NONE
		|| GetMonData(&party[i], MON_DATA_IS_EGG, NULL)
		|| i == gBattlerPartyIndexes[battlerIn1]
		|| i == gBattlerPartyIndexes[battlerIn2]
		|| i == gBattleStruct->monToSwitchIntoId[battlerIn1]
		|| i == gBattleStruct->monToSwitchIntoId[battlerIn2])
			continue;

		if (predictedMove1 != MOVE_NONE && predictedMove1 != MOVE_PREDICTION_SWITCH)
			moveFlags = AI_TypeCalc(predictedMove1, foe1, &party[i]);
		else
			moveFlags = AI_TypeCalc(predictedMove2, foe2, &party[i]);
		
		if (moveFlags & flags)
		{
			if (predictedMove1 != MOVE_NONE && predictedMove1 != MOVE_PREDICTION_SWITCH)
				battlerIn1 = foe1;
			else
				battlerIn1 = foe2;

			for (j = 0; j < 4; j++) {
				move = party[i].moves[j];
				if (move == 0 || SPLIT(move) == SPLIT_STATUS)
					continue;
							
				moveFlags = TypeCalc(move, gActiveBattler, battlerIn1, &party[i], TRUE);
				
				if (moveFlags & MOVE_RESULT_SUPER_EFFECTIVE && umodsi(Random(), moduloPercent) == 0) {
					gBattleStruct->switchoutIndex[SIDE(gActiveBattler)] = i; //There was a GetBattlerPosition here but its a pretty useless function
					EmitTwoReturnValues(1, ACTION_SWITCH, 0);
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

static bool8 ShouldSwitchIfWonderGuard(void)
{
	u8 battlerIn1;
	u8 opposingBattler;
	u8 moveFlags;
	int i, j;
	u8 start, end;
	opposingBattler = FOE(gActiveBattler);

	if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
		return FALSE;

	if (ABILITY(opposingBattler) != ABILITY_WONDERGUARD)
		return FALSE;
	
	if (WillFaintFromSecondaryDamage(opposingBattler))
		return FALSE;

	//Check if pokemon has a super effective move, Mold Breaker Move, or move that can hurt Shedinja
	u8 moveLimitations = CheckMoveLimitations(gActiveBattler, 0, 0xFF);
	u8 bankAtk = gActiveBattler;
	u8 bankDef = opposingBattler;
	for (i = 0; i < MAX_MON_MOVES; ++i)
	{
		u16 move = GetBattleMonMove(gActiveBattler, i);
		if (move == MOVE_NONE)
			break;
		
		if (!(gBitTable[i] & moveLimitations))
		{
			if (CheckTableForMove(move, MoldBreakerMoves))
				return FALSE;

			if (SPLIT(move) != SPLIT_STATUS)
			{
				u8 atkAbility = GetAIAbility(bankAtk, bankDef, move);
				if (atkAbility == ABILITY_MOLDBREAKER
				||  atkAbility == ABILITY_TERAVOLT
				||  atkAbility == ABILITY_TURBOBLAZE)
					return FALSE;
					
				moveFlags = AI_SpecialTypeCalc(move, bankAtk, bankDef);
				if (moveFlags & MOVE_RESULT_SUPER_EFFECTIVE)
					return FALSE;
			}
			else if (!MoveBlockedBySubstitute(move, bankAtk, bankDef))
			{
				switch (gBattleMoves[move].effect) {
					case EFFECT_SLEEP:
					case EFFECT_YAWN:
						if (CanBePutToSleep(bankDef, TRUE))
							return FALSE;
						break;
					case EFFECT_ROAR:
						if (ViableMonCountFromBankLoadPartyRange(bankDef) > 1)
							return FALSE;
						break;
					case EFFECT_TOXIC:
					case EFFECT_POISON:
					PSN_CHECK:
						if (CanBePoisoned(bankDef, bankAtk, TRUE))
							return FALSE;
						break;
					case EFFECT_WILL_O_WISP:
					BRN_CHECK:
						if (CanBeBurned(bankDef, TRUE))
							return FALSE;
						break;
					case EFFECT_CONFUSE:
					case EFFECT_SWAGGER:
					case EFFECT_FLATTER:
						if (CanBeConfused(bankDef))
							return FALSE;
						break;
					case EFFECT_PARALYZE:
						if (CanBeParalyzed(bankDef, TRUE))
							return FALSE;
						break;
					case EFFECT_LEECH_SEED:
						if (!IsOfType(bankDef, TYPE_GRASS))
							return FALSE;
						break;
					case EFFECT_NIGHTMARE:
						if (gBattleMons[bankDef].status1 & STATUS_SLEEP)
							return FALSE;
						break;
					case EFFECT_CURSE:
						if (IsOfType(bankAtk, TYPE_GHOST))
							return FALSE;
						break;
					case EFFECT_SPIKES:
						switch (move) {
							case MOVE_STEALTHROCK:
								if (gSideTimers[SIDE(bankDef)].srAmount == 0)
									return FALSE;
								break;
							
							case MOVE_TOXICSPIKES:
								if (gSideTimers[SIDE(bankDef)].tspikesAmount < 2)
									return FALSE;
								break;
								
							case MOVE_STICKYWEB:
								if (gSideTimers[SIDE(bankDef)].stickyWeb == 0)
									return FALSE;
								break;
							
							default: //Spikes
								if (gSideTimers[SIDE(bankDef)].spikesAmount < 3)
									return FALSE;
								break;
						}
						break;
					case EFFECT_PERISH_SONG:
						if (!(gStatuses3[bankDef] & STATUS3_PERISH_SONG))
							return FALSE;
						break;
					case EFFECT_SANDSTORM:
						if (SandstormHurts(bankDef))
							return FALSE;
						break;
					case EFFECT_HAIL:
						if (HailHurts(bankDef))
							return FALSE;
						break;
					case EFFECT_BATON_PASS:
						return FALSE;
					case EFFECT_MEMENTO:
						if (SPLIT(move) == SPLIT_STATUS)
							return FALSE;
						break;
					case EFFECT_TRICK:
						if (move == MOVE_TRICK)
						{
							if (CanTransferItem(bankAtk, ITEM(bankAtk), GetBankPartyData(bankAtk))
							&& CanTransferItem(bankAtk, ITEM(bankDef), GetBankPartyData(bankAtk))
							&& CanTransferItem(bankDef, ITEM(bankAtk), GetBankPartyData(bankDef))
							&& CanTransferItem(bankDef, ITEM(bankDef), GetBankPartyData(bankDef)))
							{
								switch (ITEM_EFFECT(bankAtk)) {
									case ITEM_EFFECT_TOXIC_ORB:
										goto PSN_CHECK;
									case ITEM_EFFECT_FLAME_ORB:
										goto BRN_CHECK;
									case ITEM_EFFECT_BLACK_SLUDGE:
										if (!IsOfType(bankDef, TYPE_POISON))
											return FALSE;
										break;
									case ITEM_EFFECT_STICKY_BARB:
										return FALSE;
								}
							}
						}
						break;
					case EFFECT_WISH:
						if (gWishFutureKnock->wishCounter[bankAtk] == 0)
							return FALSE;
						break;
					case EFFECT_SKILL_SWAP:
						if (move != MOVE_SKILLSWAP)
							return FALSE;
						break;

					case EFFECT_TYPE_CHANGES:
						switch (move) {
							case MOVE_TRICKORTREAT:
								if (!IsOfType(bankDef, TYPE_GHOST))
									return FALSE;
								break;
							case MOVE_FORESTSCURSE:
								if (!IsOfType(bankDef, TYPE_GRASS))
									return FALSE;
								break;
						}
						break;
					case EFFECT_TEAM_EFFECTS:
						switch (move) {
							case MOVE_TAILWIND:
								if (gNewBS->TailwindTimers[SIDE(bankAtk)] == 0)
									return FALSE;
								break;
							case MOVE_LUCKYCHANT:
								if (gNewBS->LuckyChantTimers[SIDE(bankAtk)] == 0)
									return FALSE;
								break;
						}
						break;
				}
			}
		}
	}


	battlerIn1 = gActiveBattler;
	pokemon_t* party = LoadPartyRange(gActiveBattler, &start, &end);
	
	//Find a pokemon in the party that has a super effective move
	for (i = start; i < end; ++i)
	{
		if (party[i].hp == 0
			|| party[i].species == SPECIES_NONE
			|| GetMonData(&party[i], MON_DATA_IS_EGG, 0)
			|| i == gBattlerPartyIndexes[battlerIn1]
			|| i == gBattleStruct->monToSwitchIntoId[battlerIn1])
			continue;

		for (j = 0; j < MAX_MON_MOVES; j++) {
			u16 move = party[i].moves[j];
			if (move == MOVE_NONE || SPLIT(move) == SPLIT_STATUS)
				continue;

			moveFlags = TypeCalc(move, gActiveBattler, opposingBattler, &party[i], TRUE);
			if (moveFlags & MOVE_RESULT_SUPER_EFFECTIVE)
			{
				// we found a mon
				gBattleStruct->switchoutIndex[SIDE(gActiveBattler)] = i;
				EmitTwoReturnValues(1, ACTION_SWITCH, 0);
				return TRUE;
			}
		}
	}

	return FALSE; // at this point there is not a single pokemon in the party that has a super effective move against a pokemon with wonder guard
}

/*
10. KO Foe + Resist/Immune to All Moves + Revenge Kill
9.
8.  KO Foe + Resist/Immune to All Moves
7.
6.  KO Foe + Revenge Kill
5.  Resist/Immune to All Moves + Has Super-Effective Move, KO Foe + Revenge Kill + Weak to Move
4.  KO Foe, Resist/Immune to All Moves
3.  KO Foe + Weak to Move
2.
1.  Has Super-Effective Move
*/

#define SWITCHING_INCREASE_KO_FOE 4 //If changing these 4's, make sure to modify the corresponding value in OnlyBadMovesLeftInMoveset
#define SWITCHING_INCREASE_RESIST_ALL_MOVES 4
#define SWITCHING_INCREASE_REVENGE_KILL 2 //Can only happen if can KO in the first place
#define SWITCHING_INCREASE_WALLS_FOE 2 //Can only wall if no Super-Effective moves against foe
//#define SWITCHING_INCREASE_HAS_SUPER_EFFECTIVE_MOVE 1
#define SWITCHING_INCREASE_CAN_REMOVE_HAZARDS 10

#define SWITCHING_DECREASE_WEAK_TO_MOVE 1

#define SWITCHING_SCORE_MAX (SWITCHING_INCREASE_KO_FOE + SWITCHING_INCREASE_RESIST_ALL_MOVES + SWITCHING_INCREASE_REVENGE_KILL)

//Add logic about switching in a partner to resist spread move in doubles
u8 GetMostSuitableMonToSwitchInto(void)
{
	u8 battlerIn1, battlerIn2;
	u8 foe1, foe2;
	LoadBattlersAndFoes(&battlerIn1, &battlerIn2, &foe1, &foe2);
	
	u8 option1 = gNewBS->bestMonIdToSwitchInto[gActiveBattler][0];
	u8 option2 = gNewBS->bestMonIdToSwitchInto[gActiveBattler][1];

	if (option1 == gBattleStruct->monToSwitchIntoId[battlerIn1]
	||  option1 == gBattleStruct->monToSwitchIntoId[battlerIn2])
		return option2;
		
	return option1;
}

u8 GetMostSuitableMonToSwitchIntoScore(void)
{
	u8 battlerIn1, battlerIn2;
	u8 foe1, foe2;
	LoadBattlersAndFoes(&battlerIn1, &battlerIn2, &foe1, &foe2);
	
	u8 option1 = gNewBS->bestMonIdToSwitchInto[gActiveBattler][0];

	if (option1 == gBattleStruct->monToSwitchIntoId[battlerIn1]
	||  option1 == gBattleStruct->monToSwitchIntoId[battlerIn2])
		return gNewBS->bestMonIdToSwitchIntoScores[gActiveBattler][1];

	return gNewBS->bestMonIdToSwitchIntoScores[gActiveBattler][0];
}

u8 CalcMostSuitableMonToSwitchInto(void)
{
	//u8 opposingBattler;
	u32 i, j, k;
	u8 bestMonId, secondBestMonId;
	u8 battlerIn1, battlerIn2;
	u8 foe, foe1, foe2;
	u8 firstId, lastId; // + 1
	u16 move;
	struct Pokemon* party;
	
	u8 lastValidMon = PARTY_SIZE;
	u8 secondLastValidMon = PARTY_SIZE;
	s8 scores[PARTY_SIZE] = {0};
	bool8 canNegateToxicSpikes[PARTY_SIZE] = {FALSE};
	bool8 canRemoveHazards[PARTY_SIZE] = {FALSE};

	if (gBattleStruct->monToSwitchIntoId[gActiveBattler] != PARTY_SIZE)
		return gBattleStruct->monToSwitchIntoId[gActiveBattler];

	LoadBattlersAndFoes(&battlerIn1, &battlerIn2, &foe1, &foe2);
	party = LoadPartyRange(gActiveBattler, &firstId, & lastId);

	//Check if point even running calcs
	u8 count = 0;
	for (i = firstId; i < lastId; ++i)
	{
		if (party[i].species != SPECIES_NONE
		&& !GetMonData(&party[i], MON_DATA_IS_EGG, NULL)
		&& party[i].hp > 0
		&& gBattlerPartyIndexes[battlerIn1] != i
		&& gBattlerPartyIndexes[battlerIn2] != i
		&& i != gBattleStruct->monToSwitchIntoId[battlerIn1]
		&& i != gBattleStruct->monToSwitchIntoId[battlerIn2])
		{
			++count;
		}
	}

	if (count == 0)
	{
		ResetBestMonToSwitchInto(gActiveBattler);
		return PARTY_SIZE;
	}

	//Try to counter a locked move
	//u8 option = GetBestPartyNumberForSemiInvulnerableLockedMoveCalcs(opposingBattler, opposingBattler, TRUE);
	//if (option != PARTY_SIZE)
	//	return option;

	//Find the mon who is most suitable
	bestMonId = PARTY_SIZE;
	secondBestMonId = PARTY_SIZE;
	for (i = firstId; i < lastId; ++i)
	{
		if (party[i].species != SPECIES_NONE
		&& party[i].hp > 0
		&& i != gBattlerPartyIndexes[battlerIn1]
		&& i != gBattlerPartyIndexes[battlerIn2]
		&& i != gBattleStruct->monToSwitchIntoId[battlerIn1]
		&& i != gBattleStruct->monToSwitchIntoId[battlerIn2])
		{
			u8 foes[] = {foe1, foe2};
			u8 moveLimitations = CheckMoveLimitationsFromParty(&party[i], 0, 0xFF);
			secondLastValidMon = lastValidMon;
			lastValidMon = i;
			
			u16 species = GetMonData(&party[i], MON_DATA_SPECIES, NULL);
			canNegateToxicSpikes[i] = CheckGroundingFromPartyData(&party[i])
									&& (gBaseStats[species].type1 == TYPE_POISON || gBaseStats[species].type2 == TYPE_POISON);
									
			if (WillFaintFromEntryHazards(&party[i], SIDE(gActiveBattler)))
				continue; //Don't switch in the mon if it'll faint on reentry

			for (j = 0, foe = foes[j]; j < gBattlersCount / 2; ++j) //Loop through all enemies on field
			{
				if (BATTLER_ALIVE(foe)
				&& (j == 0 || foes[0] != foes[j])) //Don't check same opponent twice
				{
					u8 typeEffectiveness = 0;
					u8 ability = GetPartyAbility(&party[i]);
					bool8 isWeakToMove = FALSE;
					bool8 isNormalEffectiveness = FALSE;

					//Check Offensive Capabilities
					if (CanKnockOutFromParty(&party[i], foe))
					{
						scores[i] += SWITCHING_INCREASE_KO_FOE;
						
						if (ability == ABILITY_MOXIE
						||  ability == ABILITY_SOULHEART
						||  ability == ABILITY_BEASTBOOST)
							scores[i] += SWITCHING_INCREASE_REVENGE_KILL;
						else
						{
							for (k = 0; k < MAX_MON_MOVES; ++k)
							{
								move = GetMonData(&party[i], MON_DATA_MOVE1 + k, 0);

								if (gBattleMoves[move].effect == EFFECT_RAPID_SPIN //Includes Defog
								&&  gSideAffecting[SIDE(gActiveBattler)] & SIDE_STATUS_SPIKES)
								{
									if (!IS_DOUBLE_BATTLE) //Single Battle
										canRemoveHazards[i] = ViableMonCountFromBank(gActiveBattler) >= 2; //There's a point in removing the hazards
									else //Double Battle
										canRemoveHazards[i] = ViableMonCountFromBank(gActiveBattler) >= 3; //There's a point in removing the hazards								
								}

								if (move == MOVE_FELLSTINGER
								&&  !(gBitTable[k] & moveLimitations))
								{
									if (MoveKnocksOutXHitsFromParty(move, &party[i], foe, 1))
									{
										scores[i] += SWITCHING_INCREASE_REVENGE_KILL;
										break;
									}
								}
								else if (SPLIT(move) != SPLIT_STATUS
								&& PriorityCalcForParty(&party[i], move) > 0
								&& MoveKnocksOutXHitsFromParty(move, &party[i], foe, 1))
								{
									//Priority move that KOs
									scores[i] += SWITCHING_INCREASE_REVENGE_KILL;
									break;
								}
							}
						}
					}
					else //This mon can't KO the foe
					{
						for (k = 0; k < MAX_MON_MOVES; ++k)
						{
							move = GetMonData(&party[i], MON_DATA_MOVE1 + k, 0);

							if (gBattleMoves[move].effect == EFFECT_RAPID_SPIN //Includes Defog
							&&  gSideAffecting[SIDE(gActiveBattler)] & SIDE_STATUS_SPIKES)
							{
								if (!IS_DOUBLE_BATTLE) //Single Battle
									canRemoveHazards[i]= ViableMonCountFromBank(gActiveBattler) >= 2; //There's a point in removing the hazards
								else //Double Battle
									canRemoveHazards[i]= ViableMonCountFromBank(gActiveBattler) >= 3; //There's a point in removing the hazards								
							}

							if (move != MOVE_NONE 
							&& SPLIT(move) != SPLIT_STATUS 
							&& !(gBitTable[k] & moveLimitations)
							&& TypeCalc(move, gActiveBattler, foe, &party[i], TRUE) & MOVE_RESULT_SUPER_EFFECTIVE)
							{
								scores[i] += SWITCHING_INCREASE_HAS_SUPER_EFFECTIVE_MOVE; //Only checked if can't KO
								break;
							}
						}
					}

					//Check Defensive Capabilities
					u8 foeMoveLimitations = CheckMoveLimitations(foe, 0, 0xFF);
					for (k = 0; k < MAX_MON_MOVES; ++k)
					{
						move = GetBattleMonMove(foe, k);

						if (move == MOVE_NONE)
							break;
							
						if (gBattleMons[foe].status2 & STATUS2_MULTIPLETURNS
						&&  MoveInMoveset(gLockedMoves[foe], foe)
						&&  move != gLockedMoves[foe])
							continue; //Skip non-locked moves
							
						if (SPLIT(move) == SPLIT_STATUS)
							continue;

						if (!(gBitTable[k] & foeMoveLimitations))
						{
							typeEffectiveness = AI_TypeCalc(move, foe, &party[i]);
							
							if (typeEffectiveness & MOVE_RESULT_SUPER_EFFECTIVE)
								isWeakToMove = TRUE;
							else if (typeEffectiveness == 0)
								isNormalEffectiveness = TRUE;
							//By default it either resists or is immune
						}
					}
					
					if (isWeakToMove)
						scores[i] -= SWITCHING_DECREASE_WEAK_TO_MOVE;
					else if (!isNormalEffectiveness) //Foe has no Super-Effective or Normal-Effectiveness moves
						scores[i] += SWITCHING_INCREASE_RESIST_ALL_MOVES;
					else
					{
						//Wall checks
					
					
					}
				}
			}

			if (!IS_DOUBLE_BATTLE || ViableMonCountFromBank(foe1) == 1) //Single Battle or 1 target left
			{
				if (scores[i] >= SWITCHING_SCORE_MAX && canRemoveHazards[i]) //This mon is perfect
				{
					if (IS_SINGLE_BATTLE)
					{
						bestMonId = i;
						break;
					}
					else //Double Battle
					{
						if (bestMonId == PARTY_SIZE)
							bestMonId = i;
						else
						{
							secondBestMonId = i;
							break;
						}
					}
				}
			}
			else //Double Battle
			{
				if (scores[i] >= SWITCHING_SCORE_MAX * 2 && canRemoveHazards[i]) //This mon is perfect
				{
					if (IS_SINGLE_BATTLE)
					{
						bestMonId = i;
						break;
					}
					else //Double Battle
					{
						if (bestMonId == PARTY_SIZE)
							bestMonId = i;
						else
						{
							secondBestMonId = i;
							break;
						}
					}
				}
			}

			if (bestMonId == PARTY_SIZE)
				bestMonId = i;
			else if (scores[i] > scores[bestMonId])
			{
				if (IS_DOUBLE_BATTLE)
					secondBestMonId = bestMonId;
					
				bestMonId = i;
			}
			else if (IS_DOUBLE_BATTLE && secondBestMonId == PARTY_SIZE)
				secondBestMonId = i;
		}
	}

	if (bestMonId != PARTY_SIZE)
	{
		if (scores[bestMonId] < 8)
		{
			bool8 tSpikesActive = gSideTimers[SIDE(gActiveBattler)].tspikesAmount > 0;
			for (i = firstId; i < lastId; ++i)
			{
				if (canRemoveHazards[i]
				|| (tSpikesActive && canNegateToxicSpikes[i]))
				{
					if (IS_DOUBLE_BATTLE)
					{
						gNewBS->bestMonIdToSwitchIntoScores[gActiveBattler][0] = SWITCHING_INCREASE_CAN_REMOVE_HAZARDS * 2;
						
						if ((SIDE(gActiveBattler) == B_SIDE_OPPONENT && !IsTwoOpponentBattle())
						||  (SIDE(gActiveBattler) == B_SIDE_PLAYER && !IsTagBattle()))
						{
							gNewBS->bestMonIdToSwitchIntoScores[PARTNER(gActiveBattler)][0] = SWITCHING_INCREASE_CAN_REMOVE_HAZARDS * 2;
						}
					}
					else
						gNewBS->bestMonIdToSwitchIntoScores[gActiveBattler][0] = SWITCHING_INCREASE_CAN_REMOVE_HAZARDS;

					gNewBS->bestMonIdToSwitchInto[gActiveBattler][0] = i;
					if ((SIDE(gActiveBattler) == B_SIDE_OPPONENT && !IsTwoOpponentBattle())
					||  (SIDE(gActiveBattler) == B_SIDE_PLAYER && !IsTagBattle()))
					{
						gNewBS->bestMonIdToSwitchInto[PARTNER(gActiveBattler)][0] = i;
					}
					
					if (IS_SINGLE_BATTLE)
						return i; //Just send out the Pokemon that can remove the hazards
					else
					{
						secondBestMonId = bestMonId;
						bestMonId = i;
						break;
					}
				}
			}
		}

		if (IS_DOUBLE_BATTLE)
		{
			gNewBS->bestMonIdToSwitchInto[gActiveBattler][0] = bestMonId;
			gNewBS->bestMonIdToSwitchInto[gActiveBattler][1] = secondBestMonId;
			gNewBS->bestMonIdToSwitchIntoScores[gActiveBattler][0] = scores[bestMonId];
			gNewBS->bestMonIdToSwitchIntoScores[gActiveBattler][1] = scores[secondBestMonId];
			
			if ((SIDE(gActiveBattler) == B_SIDE_OPPONENT && !IsTwoOpponentBattle())
			||  (SIDE(gActiveBattler) == B_SIDE_PLAYER && !IsTagBattle()))
			{
				gNewBS->bestMonIdToSwitchInto[PARTNER(gActiveBattler)][0] = bestMonId;
				gNewBS->bestMonIdToSwitchInto[PARTNER(gActiveBattler)][1] = secondBestMonId;
				gNewBS->bestMonIdToSwitchIntoScores[PARTNER(gActiveBattler)][0] = scores[bestMonId];
				gNewBS->bestMonIdToSwitchIntoScores[PARTNER(gActiveBattler)][1] = scores[secondBestMonId];
			}
		}
		else
		{
			gNewBS->bestMonIdToSwitchInto[gActiveBattler][0] = bestMonId;
			gNewBS->bestMonIdToSwitchIntoScores[gActiveBattler][0] = scores[bestMonId];
		}

		return bestMonId;
	}

	//If for some reason we weren't able to find a mon above,
	//pick the last checked available mon now.
	gNewBS->bestMonIdToSwitchIntoScores[gActiveBattler][0] = 0;
	gNewBS->bestMonIdToSwitchInto[gActiveBattler][0] = lastValidMon;
	gNewBS->bestMonIdToSwitchIntoScores[gActiveBattler][0] = 0;
	gNewBS->bestMonIdToSwitchInto[gActiveBattler][0] = secondLastValidMon;
	
	if ((SIDE(gActiveBattler) == B_SIDE_OPPONENT && !IsTwoOpponentBattle())
	||  (SIDE(gActiveBattler) == B_SIDE_PLAYER && !IsTagBattle()))
	{
		gNewBS->bestMonIdToSwitchIntoScores[PARTNER(gActiveBattler)][0] = 0;
		gNewBS->bestMonIdToSwitchInto[PARTNER(gActiveBattler)][0] = lastValidMon;
		gNewBS->bestMonIdToSwitchIntoScores[PARTNER(gActiveBattler)][0] = 0;
		gNewBS->bestMonIdToSwitchInto[PARTNER(gActiveBattler)][0] = secondLastValidMon;
	}

	return lastValidMon; //If lastValidMon is still PARTY_SIZE when reaching here, there is a bigger problem
}

void ResetBestMonToSwitchInto(u8 bank)
{
	gNewBS->bestMonIdToSwitchIntoScores[bank][0] = 0;
	gNewBS->bestMonIdToSwitchInto[bank][0] = PARTY_SIZE;
	gNewBS->bestMonIdToSwitchIntoScores[bank][1] = 0;
	gNewBS->bestMonIdToSwitchInto[bank][1] = PARTY_SIZE;
	
	if ((SIDE(bank) == B_SIDE_OPPONENT && !IsTwoOpponentBattle())
	||  (SIDE(bank) == B_SIDE_PLAYER && !IsTagBattle()))
	{
		gNewBS->bestMonIdToSwitchIntoScores[PARTNER(bank)][0] = 0;
		gNewBS->bestMonIdToSwitchInto[PARTNER(bank)][0] = PARTY_SIZE;
		gNewBS->bestMonIdToSwitchIntoScores[PARTNER(bank)][1] = 0;
		gNewBS->bestMonIdToSwitchInto[PARTNER(bank)][1] = PARTY_SIZE;
	}
}

void RemoveBestMonToSwitchInto(u8 bank)
{
	//secondBestMonId -> bestMonId
	gNewBS->bestMonIdToSwitchIntoScores[SIDE(bank)][0] = gNewBS->bestMonIdToSwitchIntoScores[SIDE(bank)][1];
	gNewBS->bestMonIdToSwitchInto[SIDE(bank)][0] = gNewBS->bestMonIdToSwitchInto[SIDE(bank)][1];

	gNewBS->bestMonIdToSwitchIntoScores[SIDE(bank)][1] = 0;
	gNewBS->bestMonIdToSwitchInto[SIDE(bank)][1] = PARTY_SIZE;
	
	if ((SIDE(bank) == B_SIDE_OPPONENT && !IsTwoOpponentBattle())
	||  (SIDE(bank) == B_SIDE_PLAYER && !IsTagBattle()))
	{
		gNewBS->bestMonIdToSwitchIntoScores[PARTNER(bank)][0] = gNewBS->bestMonIdToSwitchIntoScores[PARTNER(bank)][1];
		gNewBS->bestMonIdToSwitchInto[PARTNER(bank)][0] = gNewBS->bestMonIdToSwitchInto[PARTNER(bank)][1];

		gNewBS->bestMonIdToSwitchIntoScores[PARTNER(bank)][1] = 0;
		gNewBS->bestMonIdToSwitchInto[PARTNER(bank)][1] = PARTY_SIZE;
	}
}

u32 WildMonIsSmart(u8 bank)
{
	#ifdef WILD_ALWAYS_SMART
		bank++; //So no compiler warnings
		return TRUE;
	#else
		u16 species = gBattleMons[bank].species;
		for (u32 i = 0; sSmartWildAITable[i].species != 0xFFFF; ++i) {
			if (sSmartWildAITable[i].species == species)
				return sSmartWildAITable[i].aiFlags;
		}
		return FALSE;
	#endif
}

static void PredictMovesForBanks(void)
{
	int i, j;
	u8 viabilities[MAX_MON_MOVES] = {0};
	u8 bestMoves[MAX_MON_MOVES] = {0};

	for (u8 bankAtk = 0; bankAtk < gBattlersCount; ++bankAtk)
	{
		u32 moveLimitations = CheckMoveLimitations(bankAtk, 0, 0xFF);

		for (u8 bankDef = 0; bankDef < gBattlersCount; ++bankDef)
		{
			if (bankAtk == bankDef) continue;
			
			if (gBattleMons[bankAtk].status2 & STATUS2_RECHARGE
			||  gDisableStructs[bankAtk].truantCounter != 0)
			{
				StoreMovePrediction(bankAtk, bankDef, MOVE_NONE);
			}
			else if (IsBankAsleep(bankAtk)
			&& !MoveEffectInMoveset(EFFECT_SLEEP_TALK, bankAtk) && !MoveEffectInMoveset(EFFECT_SNORE, bankAtk))
			{
				StoreMovePrediction(bankAtk, bankDef, MOVE_NONE);
			}
			else if (gBattleMons[bankAtk].status2 & STATUS2_MULTIPLETURNS
			&& FindMovePositionInMoveset(gLockedMoves[bankAtk], bankAtk) < 4)
			{
				StoreMovePrediction(bankAtk, bankDef, gLockedMoves[bankAtk]);
			}
			else
			{
				u32 backupFlags = AI_THINKING_STRUCT->aiFlags; //Backup flags so killing in negatives is ignored
				AI_THINKING_STRUCT->aiFlags = 7;
				
				for (i = 0; i < MAX_MON_MOVES && gBattleMons[bankAtk].moves[i] != MOVE_NONE; ++i)
				{
					viabilities[i] = 0;
					bestMoves[i] = 0;

					if (gBitTable[i] & moveLimitations) continue;

					u16 move = gBattleMons[bankAtk].moves[i];
					move = TryReplaceMoveWithZMove(bankAtk, bankDef, move);	
					viabilities[i] = AI_Script_Negatives(bankAtk, bankDef, move, 100);
					viabilities[i] = AI_Script_Positives(bankAtk, bankDef, move, viabilities[i]);
				}

				AI_THINKING_STRUCT->aiFlags = backupFlags;
				
				bestMoves[j = 0] = GetMaxByteIndexInList(viabilities, MAX_MON_MOVES) + 1;
				for (i = 0; i < MAX_MON_MOVES; ++i)
				{
					if (i + 1 != bestMoves[0] //i is not the index returned from GetMaxByteIndexInList
					&& viabilities[i] == viabilities[bestMoves[j] - 1])
						bestMoves[++j] = i + 1;
				}

				if (viabilities[GetMaxByteIndexInList(viabilities, MAX_MON_MOVES)] < 100) //Best move has viability < 100
					StoreSwitchPrediction(bankAtk, bankDef);
				else
					StoreMovePrediction(bankAtk, bankDef, gBattleMons[bankAtk].moves[bestMoves[Random() % (j + 1)] - 1]);
					
				Memset(viabilities, 0, sizeof(viabilities));
			}
		}
	}
}

static void UpdateStrongestMoves(void)
{
	u8 bankAtk, bankDef;

	for (bankAtk = 0; bankAtk < gBattlersCount; ++bankAtk)
	{
		for (bankDef = 0; bankDef < gBattlersCount; ++bankDef)
		{
			if (bankAtk == bankDef || bankDef == PARTNER(bankAtk))
				continue; //Don't bother calculating for these Pokemon. Never used

			gNewBS->strongestMove[bankAtk][bankDef] = GetStrongestMove(bankAtk, bankDef, FALSE);
			gNewBS->canKnockOut[bankAtk][bankDef] = MoveKnocksOutXHits(gNewBS->strongestMove[bankAtk][bankDef], bankAtk, bankDef, 1);
			gNewBS->can2HKO[bankAtk][bankDef] = (gNewBS->canKnockOut[bankAtk][bankDef]) ? TRUE
												: MoveKnocksOutXHits(gNewBS->strongestMove[bankAtk][bankDef], bankAtk, bankDef, 2); //If you can KO in 1 hit you can KO in 2

			if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
				gNewBS->strongestSpreadMove[bankAtk][bankDef] = GetStrongestMove(bankAtk, bankDef, FALSE);
		}
	}
}

static void UpdateBestDoublesKillingMoves(void)
{
	if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
	{
		u8 bankAtk, bankDef;

		for (bankAtk = 0; bankAtk < gBattlersCount; ++bankAtk)
		{
			for (bankDef = 0; bankDef < gBattlersCount; ++bankDef)
			{
				if (bankAtk == bankDef || bankDef == PARTNER(bankAtk))
					continue; //Don't bother calculating for these Pokemon. Never used

				UpdateBestDoublesKillingScore(bankAtk, bankDef);
				
				if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
					gNewBS->strongestSpreadMove[bankAtk][bankDef] = GetStrongestMove(bankAtk, bankDef, FALSE);
			}
		}
	}
}


void UpdateBestDoublesKillingScore(u8 bankAtk, u8 bankDef)
{
	gNewBS->bestDoublesKillingScores[bankAtk][bankDef] = GetBestDoubleKillingMoveScore(bankAtk, bankDef, PARTNER(bankAtk), PARTNER(bankDef), &gNewBS->bestDoublesKillingMoves[bankAtk][bankDef]);
}

static u32 GetMaxByteIndexInList(const u8 array[], const u32 size)
{
	u8 maxIndex = 0;
	
	for (u32 i = 0; i < size; ++i)
	{
		if (array[i] > array[maxIndex])
			maxIndex = i;
	}

	return maxIndex;
}
