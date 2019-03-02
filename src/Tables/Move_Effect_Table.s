.thumb
.text
.align 2
.global gBattleScriptsForMoveEffects

.include "..\\defines"

gBattleScriptsForMoveEffects:
.word 0x81D6926		@EFFECT_HIT
.word BS_001_SetSleep
.word BS_002_SetPoisonChance
.word BS_003_DrainHP
.word BS_004_SetBurnChance
.word BS_005_SetFreezeChance
.word BS_006_SetParalyzeChance
.word BS_007_Explode
.word BS_008_DreamEater
.word BS_009_MirrorMove
.word BS_010_RaiseUserAtk1
.word BS_011_RaiseUserDef1
.word BS_012_RaiseUserSpd1
.word BS_013_RaiseUserSpAtk1
.word BS_014_RaiseUserSpDef1
.word BS_015_RaiseUserAcc1
.word BS_016_RaiseUserEvsn1
.word BS_017_NeverMiss 	@Regular Hit
.word 0x81d6be1		@EFFECT_ATTACK_DOWN
.word 0x81d6bec		@EFFECT_DEFENSE_DOWN
.word 0x81d6bf7		@EFFECT_SPEED_DOWN
.word 0x81D6926		@EFFECT_SPECIAL_ATTACK_DOWN
.word 0x81D6926		@EFFECT_SPECIAL_DEFENSE_DOWN
.word 0x81d6c02		@EFFECT_ACCURACY_DOWN
.word 0x81d6c0d		@EFFECT_EVASION_DOWN
.word 0x81d6c72		@EFFECT_HAZE
.word 0x81d6c83		@EFFECT_BIDE
.word 0x81d6c97		@EFFECT_RAMPAGE
.word 0x81d6cb1		@EFFECT_ROAR
.word 0x81d6cd9		@EFFECT_MULTI_HIT
.word 0x81d6d9d		@EFFECT_CONVERSION
.word 0x81d6db2		@EFFECT_FLINCH_HIT
.word 0x81d6dbd		@EFFECT_RESTORE_HP
.word 0x81d6de0		@EFFECT_TOXIC
.word 0x81d6e69		@EFFECT_PAY_DAY
.word 0x81d6e74		@EFFECT_LIGHT_SCREEN
.word 0x81d6e7d		@EFFECT_TRI_ATTACK
.word 0x81d6e88		@EFFECT_REST
.word 0x81d6ed0		@EFFECT_0HKO
.word 0x81d6f01		@EFFECT_RAZOR_WIND
.word 0x81d6f82		@EFFECT_SUPER_FANG
.word 0x81d6f99		@EFFECT_DRAGON_RAGE
.word 0x81d6fc8		@EFFECT_TRAP
.word 0x81D6926		@EFFECT_HIGH_CRITICAL
.word 0x81d6926		@EFFECT_DOUBLE_HIT
.word 0x81d7011		@EFFECT_RECOIL_IF_MISS
.word 0x81d7062		@EFFECT_MIST
.word 0x81d7075		@EFFECT_FOCUS_ENERGY
.word 0x81d7092		@EFFECT_RECOIL
.word 0x81d70ab		@EFFECT_CONFUSE
.word 0x81d70f8		@EFFECT_ATTACK_UP_2
.word 0x81d7103		@EFFECT_DEFENSE_UP_2
.word 0x81d710e		@EFFECT_SPEED_UP_2
.word 0x81d7119		@EFFECT_SPECIAL_ATTACK_UP_2
.word 0x81d7124		@EFFECT_SPECIAL_DEFENSE_UP_2
.word 0x81D6926		@EFFECT_ACCURACY_UP_2
.word 0x81D6926		@EFFECT_EVASION_UP_2
.word 0x81d712f		@EFFECT_TRANSFORM
.word 0x81d7142		@EFFECT_ATTACK_DOWN_2
.word 0x81d714d		@EFFECT_DEFENSE_DOWN_2
.word 0x81d7158		@EFFECT_SPEED_DOWN_2
.word 0x81D6926		@EFFECT_SPECIAL_ATTACK_DOWN_2
.word 0x81d7163		@EFFECT_SPECIAL_DEFENSE_DOWN_2
.word 0x81D6926		@EFFECT_ACCURACY_DOWN_2
.word 0x81D6926		@EFFECT_EVASION_DOWN_2
.word 0x81d716e		@EFFECT_REFLECT
.word 0x81d7181		@EFFECT_POISON
.word 0x81d71e2		@EFFECT_PARALYZE
.word 0x81d725f		@EFFECT_ATTACK_DOWN_HIT
.word 0x81d726a		@EFFECT_DEFENSE_DOWN_HIT
.word 0x81d7275		@EFFECT_SPEED_DOWN_HIT
.word 0x81d7280		@EFFECT_SPECIAL_ATTACK_DOWN_HIT
.word 0x81d728b		@EFFECT_SPECIAL_DEFENSE_DOWN_HIT
.word 0x81d7296		@EFFECT_ACCURACY_DOWN_HIT
.word 0x81D6926		@EFFECT_EVASION_DOWN_HIT
.word 0x81d72a1		@EFFECT_SKY_ATTACK
.word 0x81d72c9		@EFFECT_CONFUSE_HIT
.word 0x81d72d4		@EFFECT_TWINEEDLE
.word 0x81D6926		@EFFECT_VITAL_THROW
.word 0x81d72ec		@EFFECT_SUBSTITUTE
.word 0x81d732f		@EFFECT_RECHARGE
.word 0x81d734d		@EFFECT_RAGE
.word 0x81d7374		@EFFECT_MIMIC
.word 0x81d739a		@EFFECT_METRONOME
.word 0x81d73ae		@EFFECT_LEECH_SEED
.word 0x81d73d5		@EFFECT_SPLASH
.word 0x81d73e7		@EFFECT_DISABLE
.word 0x81d7403		@EFFECT_LEVEL_DAMAGE
.word 0x81d741b		@EFFECT_PSYWAVE
.word 0x81d7433		@EFFECT_COUNTER
.word 0x81d7449		@EFFECT_ENCORE
.word 0x81d7465		@EFFECT_PAIN_SPLIT
.word 0x81d749c		@EFFECT_SNORE
.word 0x81d74d6		@EFFECT_CONVERSION_2
.word 0x81d74eb		@EFFECT_LOCK_ON
.word 0x81d750d		@EFFECT_SKETCH
.word 0x81D6926		@EFFECT_UNUSED_60
.word 0x81d752c		@EFFECT_SLEEP_TALK
.word 0x81d756e		@EFFECT_DESTINY_BOND
.word 0x81d757f		@EFFECT_FLAIL
.word 0x81d7585		@EFFECT_SPITE
.word 0x81D6926		@EFFECT_FALSE_SWIPE
.word 0x81d75a1		@EFFECT_HEAL_BELL
.word 0x81D6926		@EFFECT_QUICK_ATTACK
.word 0x81d75e6		@EFFECT_TRIPLE_KICK
.word 0x81d76c9		@EFFECT_THIEF
.word 0x81d76d4		@EFFECT_MEAN_LOOK
.word 0x81d7706		@EFFECT_NIGHTMARE
.word 0x81d7740		@EFFECT_MINIMIZE
.word 0x81d774d		@EFFECT_CURSE
.word 0x81D6926		@EFFECT_UNUSED_6E
.word 0x81d7816		@EFFECT_PROTECT
.word 0x81d7829		@EFFECT_SPIKES
.word 0x81d783e		@EFFECT_FORESIGHT
.word 0x81d7856		@EFFECT_PERISH_SONG
.word 0x81d7897		@EFFECT_SANDSTORM
.word 0x81d7816		@EFFECT_ENDURE
.word 0x81d78a0		@EFFECT_ROLLOUT
.word 0x81d78bb		@EFFECT_SWAGGER
.word 0x81d7919		@EFFECT_FURY_CUTTER
.word 0x81d7938		@EFFECT_ATTRACT
.word 0x81d7954		@EFFECT_RETURN
.word 0x81d7962		@EFFECT_PRESENT
.word 0x81d7954		@EFFECT_FRUSTRATION
.word 0x81d796e		@EFFECT_SAFEGUARD
.word 0x81d7977		@EFFECT_THAW_HIT
.word 0x81d7982		@EFFECT_MAGNITUDE
.word 0x81d7995		@EFFECT_BATON_PASS
.word 0x81D6926		@EFFECT_PURSUIT
.word 0x81d79c2		@EFFECT_RAPID_SPIN
.word 0x81d79cd		@EFFECT_SONICBOOM
.word 0x81D6926		@EFFECT_UNUSED_83
.word 0x81d79fc		@EFFECT_MORNING_SUN
.word 0x81d79fc		@EFFECT_SYNTHESIS
.word 0x81d79fc		@EFFECT_MOONLIGHT
.word 0x81d7a09		@EFFECT_HIDDEN_POWER
.word 0x81d7a10		@EFFECT_RAIN_DANCE
.word 0x81d7a28		@EFFECT_SUNNY_DAY
.word 0x81d7a31		@EFFECT_DEFENSE_UP_HIT
.word 0x81d7a3c		@EFFECT_ATTACK_UP_HIT
.word 0x81d7a47		@EFFECT_ALL_STATS_UP_HIT
.word 0x81D6926		@EFFECT_UNUSED_8D
.word 0x81d7a52		@EFFECT_BELLY_DRUM
.word 0x81d7a74		@EFFECT_PSYCH_UP
.word 0x81d7a89		@EFFECT_MIRROR_COAT
.word 0x81d7a9f		@EFFECT_SKULL_BASH
.word 0x81d7aee		@EFFECT_TWISTER
.word 0x81d7b13		@EFFECT_EARTHQUAKE
.word 0x81d7b97		@EFFECT_FUTURE_SIGHT
.word 0x81d7bae		@EFFECT_GUST
.word 0x81d7bcd		@EFFECT_FLINCH_HIT_2
.word 0x81d7be3		@EFFECT_SOLARBEAM
.word 0x81d7c39		@EFFECT_THUNDER
.word 0x81d7c4d		@EFFECT_TELEPORT
.word 0x81d7c8a		@EFFECT_BEAT_UP
.word 0x81d7ce1		@EFFECT_SEMI_INVULNERABLE
.word 0x81d7d8c		@EFFECT_DEFENSE_CURL
.word 0x81d7dae		@EFFECT_SOFTBOILED
.word 0x81d7ddf		@EFFECT_FAKE_OUT
.word 0x81d7e16		@EFFECT_UPROAR
.word 0x81d7e36		@EFFECT_STOCKPILE
.word 0x81d7e49		@EFFECT_SPIT_UP
.word 0x81d7e8b		@EFFECT_SWALLOW
.word 0x81D6926		@EFFECT_UNUSED_A3
.word 0x81d7ea8		@EFFECT_HAIL
.word 0x81d7eb1		@EFFECT_TORMENT
.word 0x81d7ecd		@EFFECT_FLATTER
.word 0x81d7f2b		@EFFECT_WILL_O_WISP
.word 0x81d7f9f		@EFFECT_MEMENTO
.word 0x81d8042		@EFFECT_FACADE
.word 0x81d805c		@EFFECT_FOCUS_PUNCH
.word 0x81d806e		@EFFECT_SMELLINGSALT
.word 0x81d8098		@EFFECT_FOLLOW_ME
.word 0x81d80a9		@EFFECT_NATURE_POWER
.word 0x81d80b6		@EFFECT_CHARGE
.word 0x81d80c7		@EFFECT_TAUNT
.word 0x81d80e3		@EFFECT_HELPING_HAND
.word 0x81d80f8		@EFFECT_TRICK
.word 0x81d8126		@EFFECT_ROLE_PLAY
.word 0x81d8142		@EFFECT_WISH
.word 0x81d8152		@EFFECT_ASSIST
.word 0x81d8169		@EFFECT_INGRAIN
.word 0x81d817e		@EFFECT_SUPERPOWER
.word 0x81d8189		@EFFECT_MAGIC_COAT
.word 0x81d819e		@EFFECT_RECYCLE
.word 0x81d81b3		@EFFECT_REVENGE
.word 0x81d81b9		@EFFECT_BRICK_BREAK
.word 0x81d820a		@EFFECT_YAWN
.word 0x81d8263		@EFFECT_KNOCK_OFF
.word 0x81d826e		@EFFECT_ENDEAVOR
.word 0x81d82a9		@EFFECT_ERUPTION
.word 0x81d82af		@EFFECT_SKILL_SWAP
.word 0x81d82cb		@EFFECT_IMPRISON
.word 0x81d82e0		@EFFECT_REFRESH
.word 0x81d82f7		@EFFECT_GRUDGE
.word 0x81d830c		@EFFECT_SNATCH
.word 0x81d8324		@EFFECT_LOW_KICK
.word 0x81d8334		@EFFECT_SECRET_POWER
.word 0x81d833a		@EFFECT_DOUBLE_EDGE
.word 0x81d8345		@EFFECT_TEETER_DANCE
.word 0x81d6a55		@EFFECT_BLAZE_KICK
.word 0x81d83f3		@EFFECT_MUD_SPORT
.word 0x81d840a		@EFFECT_POISON_FANG
.word 0x81d8415		@EFFECT_WEATHER_BALL
.word 0x81d841b		@EFFECT_OVERHEAT
.word 0x81d8426		@EFFECT_TICKLE
.word 0x81d84ad		@EFFECT_COSMIC_POWER
.word 0x81d8511		@EFFECT_SKY_UPPERCUT
.word 0x81d851f		@EFFECT_BULK_UP
.word 0x81d69dc		@EFFECT_POISON_TAIL
.word 0x81d83f3		@EFFECT_WATER_SPORT
.word 0x81d8583		@EFFECT_CALM_MIND
.word 0x81d85fb		@EFFECT_DRAGON_DANCE
.word 0x81d865f		@EFFECT_CAMOUFLAGE
