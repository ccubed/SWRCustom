#include <stdio.h>
#include "mud.h"

static int get_in_array( const char *name, const char * const * array,
			 size_t sz,
			 bool (*compare_string)( const char*, const char* ) )
{
  size_t x = 0;

  for( x = 0; x < sz; ++x )
    if( !compare_string( name, array[x] ) )
      return x;

  return -1;
}

/*
 * Attribute bonus tables.
 */
const struct str_app_type str_app[26] = {
  {-5, -4, 0, 0},		/* 0  */
  {-5, -4, 3, 2},		/* 1  */
  {-3, -2, 3, 4},
  {-3, -1, 10, 6},		/* 3  */
  {-2, -1, 25, 8},
  {-2, -1, 55, 10},		/* 5  */
  {-1, 0, 80, 12},
  {-1, 0, 90, 14},
  {0, 0, 100, 16},
  {0, 0, 100, 18},
  {0, 0, 115, 20},		/* 10  */
  {0, 0, 115, 22},
  {0, 0, 140, 24},
  {0, 0, 140, 26},		/* 13  */
  {0, 1, 170, 28},
  {1, 1, 170, 30},		/* 15  */
  {1, 2, 195, 32},
  {2, 3, 220, 34},
  {2, 4, 250, 36},		/* 18  */
  {3, 5, 400, 38},
  {3, 6, 500, 40},		/* 20  */
  {4, 7, 600, 42},
  {5, 7, 700, 44},
  {6, 8, 800, 46},
  {8, 10, 900, 48},
  {10, 12, 999, 50}		/* 25   */
};

const struct int_app_type int_app[26] = {
  {3},				/*  0 */
  {5},				/*  1 */
  {7},
  {8},				/*  3 */
  {9},
  {10},				/*  5 */
  {11},
  {12},
  {13},
  {15},
  {17},				/* 10 */
  {19},
  {22},
  {25},
  {28},
  {31},				/* 15 */
  {34},
  {37},
  {40},				/* 18 */
  {44},
  {49},				/* 20 */
  {55},
  {60},
  {70},
  {85},
  {99}				/* 25 */
};

const struct wis_app_type wis_app[26] = {
  {0},				/*  0 */
  {0},				/*  1 */
  {0},
  {0},				/*  3 */
  {0},
  {1},				/*  5 */
  {1},
  {1},
  {1},
  {2},
  {2},				/* 10 */
  {2},
  {2},
  {2},
  {2},
  {3},				/* 15 */
  {3},
  {4},
  {5},				/* 18 */
  {5},
  {5},				/* 20 */
  {6},
  {6},
  {6},
  {6},
  {7}				/* 25 */
};

const struct dex_app_type dex_app[26] = {
  {60},				/* 0 */
  {50},				/* 1 */
  {50},
  {40},
  {30},
  {20},				/* 5 */
  {10},
  {0},
  {0},
  {0},
  {0},				/* 10 */
  {0},
  {0},
  {0},
  {0},
  {-10},			/* 15 */
  {-15},
  {-20},
  {-30},
  {-40},
  {-50},			/* 20 */
  {-60},
  {-75},
  {-90},
  {-105},
  {-120}			/* 25 */
};

const struct con_app_type con_app[26] = {
  {-4, 20, 95},			/*  0 */
  {-3, 25, 95},			/*  1 */
  {-2, 30, 95},
  {-2, 35, 95},			/*  3 */
  {-1, 40, 90},
  {-1, 45, 85},			/*  5 */
  {-1, 50, 80},
  {0, 55, 75},
  {0, 60, 70},
  {0, 65, 65},
  {0, 70, 60},			/* 10 */
  {0, 75, 55},
  {0, 80, 50},
  {0, 85, 50},
  {0, 88, 50},
  {1, 90, 45},			/* 15 */
  {2, 95, 40},
  {2, 97, 35},
  {3, 99, 30},			/* 18 */
  {3, 99, 25},
  {4, 99, 20},			/* 20 */
  {4, 99, 20},
  {5, 99, 10},
  {6, 99, 10},
  {7, 99, 5},
  {8, 99, 5}			/* 25 */
};

const struct cha_app_type cha_app[26] = {
  {-60},			/* 0 */
  {-50},			/* 1 */
  {-50},
  {-40},
  {-30},
  {-20},			/* 5 */
  {-10},
  {-5},
  {-1},
  {0},
  {0},				/* 10 */
  {0},
  {0},
  {0},
  {1},
  {5},				/* 15 */
  {10},
  {20},
  {30},
  {40},
  {50},				/* 20 */
  {60},
  {70},
  {80},
  {90},
  {99}				/* 25 */
};

/* Have to fix this up - not exactly sure how it works (Scryn) */
const struct lck_app_type lck_app[26] = {
  {60},				/* 0 */
  {50},				/* 1 */
  {50},
  {40},
  {30},
  {20},				/* 5 */
  {10},
  {0},
  {0},
  {0},
  {0},				/* 10 */
  {0},
  {0},
  {0},
  {0},
  {-10},			/* 15 */
  {-15},
  {-20},
  {-30},
  {-40},
  {-50},			/* 20 */
  {-60},
  {-75},
  {-90},
  {-105},
  {-120}			/* 25 */
};

const struct frc_app_type frc_app[26] = {
  {0},				/* 0 */
  {0},				/* 1 */
  {0},
  {0},
  {0},
  {0},				/* 5 */
  {0},
  {0},
  {0},
  {0},
  {0},				/* 10 */
  {0},
  {0},
  {0},
  {0},
  {0},				/* 15 */
  {0},
  {0},
  {0},
  {0},
  {0},				/* 20 */
  {0},
  {0},
  {0},
  {0},
  {0}				/* 25 */
};

/*
 * Liquid properties.
 * Used in #OBJECT section of area file.
 */
const struct liq_type liq_table[LIQ_MAX] = {
  {"water", "clear", {0, 1, 10}},	/*  0 */
  {"beer", "amber", {3, 2, 5}},
  {"wine", "rose", {5, 2, 5}},
  {"ale", "brown", {2, 2, 5}},
  {"dark ale", "dark", {1, 2, 5}},

  {"whisky", "golden", {6, 1, 4}},	/*  5 */
  {"lemonade", "pink", {0, 1, 8}},
  {"firebreather", "boiling", {10, 0, 0}},
  {"local specialty", "everclear", {3, 3, 3}},
  {"slime mold juice", "green", {0, 4, -8}},

  {"milk", "white", {0, 3, 6}},	/* 10 */
  {"tea", "tan", {0, 1, 6}},
  {"coffee", "black", {0, 1, 6}},
  {"blood", "red", {0, 2, -1}},
  {"salt water", "clear", {0, 1, -2}},

  {"cola", "cherry", {0, 1, 5}},	/* 15 */
  {"mead", "honey color", {4, 2, 5}},	/* 16 */
  {"grog", "thick brown", {3, 2, 5}},	/* 17 */
  {"milkshake", "creamy", {0, 8, 5}}	/* 18 */
};

const char *const attack_table[] = {
  "hit",
  "slice", "stab", "slash", "whip", "claw",
  "blast", "pound", "crush", "shot", "bite",
  "pierce", "suction"
};

size_t attack_table_size( void )
{
  return sizeof( attack_table ) / sizeof( attack_table[0] );
}

const char *const where_name[] = {
  "<used as light>     ",
  "<worn on finger>    ",
  "<worn on finger>    ",
  "<worn around neck>  ",
  "<worn around neck>  ",
  "<worn on body>      ",
  "<worn on head>      ",
  "<worn on legs>      ",
  "<worn on feet>      ",
  "<worn on hands>     ",
  "<worn on arms>      ",
  "<energy shield>     ",
  "<worn about body>   ",
  "<worn about waist>  ",
  "<worn around wrist> ",
  "<worn around wrist> ",
  "<wielded>           ",
  "<held>              ",
  "<dual wielded>      ",
  "<worn on ears>      ",
  "<worn on eyes>      ",
  "<missile wielded>   "
};

const char *const halucinated_object_short[] = {
  "a sword",
  "a stick",
  "something shiny",
  "something",
  "something interesting",
  "something colorful",
  "something that looks cool",
  "a nifty thing",
  "a cloak of flowing colors",
  "a mystical flaming sword",
  "a swarm of insects",
  "a deathbane",
  "a figment of your imagination",
  "your gravestone",
  "the long lost boots of Ranger Thoric",
  "a glowing tome of arcane knowledge",
  "a long sought secret",
  "the meaning of it all",
  "the answer",
  "the key to life, the universe and everything"
};

size_t halucinated_object_short_size( void )
{
  return sizeof halucinated_object_short / sizeof *halucinated_object_short;
}

const char *const halucinated_object_long[] = {
  "A nice looking sword catches your eye.",
  "The ground is covered in small sticks.",
  "Something shiny catches your eye.",
  "Something catches your attention.",
  "Something interesting catches your eye.",
  "Something colorful flows by.",
  "Something that looks cool calls out to you.",
  "A nifty thing of great importance stands here.",
  "A cloak of flowing colors asks you to wear it.",
  "A mystical flaming sword awaits your grasp.",
  "A swarm of insects buzzes in your face!",
  "The extremely rare Deathbane lies at your feet.",
  "A figment of your imagination is at your command.",
  "You notice a gravestone here... upon closer examination, it reads your name.",
  "The long lost boots of Ranger Thoric lie off to the side.",
  "A glowing tome of arcane knowledge hovers in the air before you.",
  "A long sought secret of all mankind is now clear to you.",
  "The meaning of it all, so simple, so clear... of course!",
  "The answer.  One.  It's always been One.",
  "The key to life, the universe and everything awaits your hand."
};

size_t halucinated_object_long_size( void )
{
  return sizeof halucinated_object_long / sizeof *halucinated_object_long;
}

const char *const day_name[] = {
  "the Moon", "the Bull", "Deception", "Thunder", "Freedom",
  "the Great Gods", "the Sun"
};

const char *const month_name[] = {
  "Winter", "the Winter Wolf", "the Frost Giant", "the Old Forces",
  "the Grand Struggle", "the Spring", "Nature", "Futility", "the Dragon",
  "the Sun", "the Heat", "the Battle", "the Dark Shades", "the Shadows",
  "the Long Shadows", "the Ancient Darkness", "the Great Evil"
};

const short movement_loss[SECT_MAX] = {
  1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6, 5, 7, 4, 6, 4, 2, 3, 6, 3, 3, 5, 4, 3, 2,
  3, 7
};

const char *const dir_name[] = {
  "north", "east", "south", "west", "up", "down",
  "northeast", "northwest", "southeast", "southwest", "somewhere"
};

const short rev_dir[] = {
  DIR_SOUTH, DIR_WEST, DIR_NORTH, DIR_EAST, DIR_DOWN,
  DIR_UP, DIR_SOUTHWEST, DIR_SOUTHEAST, DIR_NORTHWEST, DIR_NORTHEAST,
  DIR_SOMEWHERE
};

const char *const sect_names[SECT_MAX] = {
  "Inside",        /* SECT_INSIDE */
  "City Street",   /* SECT_CITY */
  "Field",         /* SECT_FIELD */
  "Forest",        /* SECT_FOREST */
  "Hill",          /* SECT_HILLS */
  "Mountain",      /* SECT_MOUNTAIN */
  "Water",         /* SECT_WATER_SWIM */
  "Rough Water",   /* SECT_WATER_NOSWIM */
  "Underwater",    /* SECT_UNDERWATER */
  "In the air",    /* SECT_AIR */
  "Sesert",        /* SECT_DESERT */
  "Somewhere",     /* SECT_DUNNO */
  "Ocean floor",   /* SECT_OCEANFLOOR */
  "Underground",   /* SECT_UNDERGROUND */
  "Scrub",         /* SECT_SCRUB */
  "Rocky Terrain", /* SECT_ROCKY */
  "Savanna",       /* SECT_SAVANNA */
  "Tundra",        /* SECT_TUNDRA */
  "Glacier",       /* SECT_GLACIAL */
  "Rainforest",    /* SECT_RAINFOREST */
  "Jungle",        /* SECT_JUNGLE */
  "Swamp",         /* SECT_SWAMP */
  "Wetlands",      /* SECT_WETLANDS */
  "Brush",         /* SECT_BRUSH */
  "Steppe",        /* SECT_STEPPE */
  "Farmland",      /* SECT_FARMLAND */
  "Volcano"        /* SECT_VOLCANIC */
};

size_t sector_names_size( void )
{
  return sizeof( sect_names ) / sizeof( *sect_names );
}

const char *get_sector_name( int sect )
{
  if( sect >= (int) sector_names_size() || sect < 0 )
    return "*out of bounds*";

  return sect_names[sect];
}

int get_sector_type( const char *sect )
{
  return get_in_array( sect, sect_names, sector_names_size(), str_prefix );
}

const char *const save_flag[] = {
  "death", "kill", "passwd", "drop", "put", "give", "auto", "zap",
  "auction", "get", "receive", "idle", "backup", "r13", "r14", "r15", "r16",
  "r17", "r18", "r19", "r20", "r21", "r22", "r23", "r24", "r25", "r26",
  "r27", "r28", "r29", "r30", "r31"
};

const char *const weapon_table[] = {
  "none",
  "w1", "vibro-blade", "lightsaber", "w4", "w5",
  "blaster", "w7", "w8", "w9", "w10",
  "w11", "w12"
};

size_t weapon_table_size( void )
{
  return sizeof( weapon_table ) / sizeof( weapon_table[0] );
}

const char *const ex_flags[] = {
  "isdoor", "closed", "locked", "secret", "swim", "pickproof", "fly", "climb",
  "dig", "r1", "nopassdoor", "hidden", "passage", "portal", "r2", "r3",
  "can_climb", "can_enter", "can_leave", "auto", "r4", "searchable",
  "bashed", "bashproof", "nomob", "window", "can_look"
};

const char *const r_flags[] = {
  "dark", "reserved", "nomob", "indoors", "can_land", "garage", "control",
  "trade", "bank", "r9", "safe", "mail", "information", "r13",
  "shipyard", "barracks", "r16", "no_edit", "r18", "restaurant",
  "plr_home", "empty_home", "r22", "hotel", "nofloor", "pawn", "supply",
  "bar", "employment", "spacecraft", "r30", "auction"
};

const char *const o_flags[] = {
  "glow", "hum", "dark", "hutt_size", "i4", "invis", "magic", "nodrop",
  "bless",
  "i9", "i10", "i11", "noremove", "inventory",
  "i14", "i15", "i16", "i17", "small_size", "large_size",
  "donation", "clanobject", "i22", "i23", "i24",
  "hidden", "poisoned", "covering", "deathrot", "burried", "prototype",
  "human_size"
};

const char *const mag_flags[] = {
  "returning", "backstabber", "bane", "loyal", "haste", "drain",
  "lightning_blade"
};

const char *const w_flags[] = {
  "take", "finger", "neck", "body", "head", "legs", "feet", "hands", "arms",
  "shield", "about", "waist", "wrist", "wield", "hold", "_dual_", "ears",
  "eyes",
  "_missile_", "r1", "r2", "r3", "r4", "r5", "r6",
  "r7", "r8", "r9", "r10", "r11", "r12", "r13"
};

const char *const area_flags[] = {
  "nopkill", "modified", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
  "r9", "r10", "r11", "r12", "r13", "r14", "r15", "r16", "r17",
  "r18", "r19", "r20", "r21", "r22", "r23", "r24",
  "r25", "r26", "r27", "r28", "r29", "r30", "r31"
};

const char *const o_types[] = {
  "none", "light", "weapon", "armor", "furniture",
  "trash", "container", "paper", "drinkcon", "food",
  "money", "pen", "corpse", "corpse_pc", "fountain",
  "scraps", "lockpick", "ammo", "shovel", "lens",
  "crystal", "plastic", "battery", "toolkit", "metal",
  "oven", "mirror", "circuit", "superconductor", "comlink",
  "medpac", "fabric", "rare_metal", "magnet", "thread",
  "device", "droid_corpse", "resource"
};

const char *const a_types[] = {
  "none", "strength", "dexterity", "intelligence", "wisdom", "constitution",
  "sex", "null", "level", "age", "height", "weight", "force", "hit", "move",
  "credits", "experience", "armor", "hitroll", "damroll", "save_poison",
  "save_rod",
  "save_para", "save_breath", "save_spell", "charisma", "affected",
  "resistant",
  "immune", "susceptible", "weaponspell", "luck", "backstab", "pick", "track",
  "steal", "sneak", "hide", "palm", "detrap", "dodge", "peek", "scan",
  "gouge",
  "search", "mount", "disarm", "kick", "parry", "bash", "stun", "punch",
  "climb",
  "grip", "scribe", "brew", "wearspell", "removespell", "mentalstate",
  "emotion",
  "stripsn", "remove", "dig", "full", "thirst", "drunk", "blood"
};

const char *const a_flags[] = {
  "blind", "invisible", "detect_evil", "detect_invis", "detect_magic",
  "detect_hidden", "weaken", "sanctuary", "faerie_fire", "infrared", "curse",
  "_flaming", "poison", "protect", "paralysis", "sneak", "hide", "sleep",
  "charm", "flying", "pass_door", "floating", "truesight", "detect_traps",
  "scrying", "fireshield", "shockshield", "r1", "iceshield", "possess",
  "berserk", "aqua_breath"
};

const char *const act_flags[] = {
  "npc", "sentinel", "scavenger", "r3", "r4", "aggressive", "stayarea",
  "wimpy", "pet", "train", "practice", "immortal", "deadly", "polyself",
  "meta_aggr", "guardian", "running", "nowander", "mountable", "mounted",
  "citizen",
  "secretive", "polymorphed", "mobinvis", "noassist", "nokill", "droid",
  "nocorpse",
  "r28", "r29", "prototype", "r31"
};

const char *const pc_flags[] = {
  "r1", "deadly", "unauthed", "norecall", "nointro", "gag", "retired",
  "guest",
  "nosummon", "pageron", "notitled", "room", "r6", "r7", "r8", "r9", "r10",
  "r11", "r12", "r13",
  "r14", "r15", "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23", "r24",
  "r25"
};

const char *const plr_flags[] = {
  "npc", "boughtpet", "shovedrag", "autoexits", "autoloot", "autosac",
  "blank",
  "outcast", "brief", "combine", "prompt", "telnet_ga", "holylight",
  "wizinvis", "roomvnum", "silence", "noemote", "attacker", "notell", "log",
  "deny", "freeze", "killer", "pf_3", "litterbug", "ansi", "rip", "nice",
  "flee", "autocred", "automap", "afk"
};

const char *const trap_flags[] = {
  "room", "obj", "enter", "leave", "open", "close", "get", "put", "pick",
  "unlock", "north", "south", "east", "west", "up", "down", "examine",
  "northeast", "northwest", "southeast", "southwest", "r6", "r7", "r8",
  "r9", "r10", "r11", "r12", "r13", "r14", "r15"
};

const char *const wear_locs[] = {
  "light", "finger1", "finger2", "neck1", "neck2", "body", "head", "legs",
  "feet", "hands", "arms", "shield", "about", "waist", "wrist1", "wrist2",
  "wield", "hold", "dual_wield", "ears", "eyes", "missile_wield"
};

const char *const ris_flags[] = {
  "fire", "cold", "electricity", "energy", "blunt", "pierce", "slash", "acid",
  "poison", "drain", "sleep", "charm", "hold", "nonmagic", "plus1", "plus2",
  "plus3", "plus4", "plus5", "plus6", "magic", "paralysis", "r1", "r2", "r3",
  "r4", "r5", "r6", "r7", "r8", "r9", "r10"
};

const char *const trig_flags[] = {
  "up", "unlock", "lock", "d_north", "d_south", "d_east", "d_west", "d_up",
  "d_down", "door", "container", "open", "close", "passage", "oload", "mload",
  "teleport", "teleportall", "teleportplus", "death", "cast", "fakeblade",
  "rand4", "rand6", "trapdoor", "anotherroom", "usedial", "absolutevnum",
  "showroomdesc", "autoreturn", "r2", "r3"
};

const char *const attack_flags[] = {
  "bite", "claws", "tail", "sting", "punch", "kick",
  "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "r16",
  "r17",
  "r18", "r19", "r20", "r21", "r22", "r23", "r24", "r25", "r26", "r27", "r28",
  "r29",
  "r30", "r31"
};

const char *const defense_flags[] = {
  "parry", "dodge", "r2", "r3", "r4", "r5",
  "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "r16",
  "r17",
  "r18", "r19", "r20", "r21", "r22", "r23", "r24", "r25", "r26", "r27", "r28",
  "r29",
  "r30", "r31"
};

/*
 * Note: I put them all in one big set of flags since almost all of these
 * can be shared between mobs, objs and rooms for the exception of
 * bribe and hitprcnt, which will probably only be used on mobs.
 * ie: drop -- for an object, it would be triggered when that object is
 * dropped; -- for a room, it would be triggered when anything is dropped
 *          -- for a mob, it would be triggered when anything is dropped
 *
 * Something to consider: some of these triggers can be grouped together,
 * and differentiated by different arguments... for example:
 *  hour and time, rand and randiw, speech and speechiw
 *
 */
const char *const mprog_flags[] = {
  "act", "speech", "rand", "fight", "death", "hitprcnt", "entry", "greet",
  "allgreet", "give", "bribe", "hour", "time", "wear", "remove", "sac",
  "look", "exa", "zap", "get", "drop", "damage", "repair", "randiw",
  "speechiw", "pull", "push", "sleep", "rest", "leave", "script", "use"
};

int get_otype( const char *type )
{
  return get_in_array( type, o_types,
		       sizeof( o_types ) / sizeof( *o_types ), str_cmp );
}

int get_aflag( const char *flag )
{
  return get_in_array( flag, a_flags,
		       sizeof( a_flags ) / sizeof( *a_flags ), str_cmp );
}

int get_trapflag( const char *flag )
{
  return get_in_array( flag, trap_flags,
		       sizeof( trap_flags ) / sizeof( *trap_flags ), str_cmp );
}

int get_atype( const char *type )
{
  return get_in_array( type, a_types, MAX_APPLY_TYPE, str_cmp );
}

int get_wearloc( const char *type )
{
  return get_in_array( type, wear_locs, MAX_WEAR, str_cmp );
}

int get_exflag( const char *flag )
{
  return get_in_array( flag, ex_flags, MAX_EXFLAG, str_cmp );
}

int get_rflag( const char *flag )
{
  return get_in_array( flag, r_flags,
		       sizeof( r_flags ) / sizeof( *r_flags ), str_cmp );
}

int get_mpflag( const char *flag )
{
  return get_in_array( flag, mprog_flags,
		       sizeof( mprog_flags ) / sizeof( *mprog_flags ),
		       str_cmp );
}

int get_oflag( const char *flag )
{
  return get_in_array( flag, o_flags,
		       sizeof( o_flags ) / sizeof( *o_flags ), str_cmp );
}

int get_areaflag( const char *flag )
{
  return get_in_array( flag, area_flags,
		       sizeof( area_flags ) / sizeof( *area_flags ), str_cmp );
}

int get_wflag( const char *flag )
{
  return get_in_array( flag, w_flags,
		       sizeof( w_flags ) / sizeof( *w_flags ), str_cmp );
}

int get_actflag( const char *flag )
{
  return get_in_array( flag, act_flags,
		       sizeof( act_flags ) / sizeof( *act_flags ), str_cmp );
}

int get_pcflag( const char *flag )
{
  return get_in_array( flag, pc_flags,
		       sizeof( pc_flags ) / sizeof( *pc_flags ), str_cmp );
}
int get_plrflag( const char *flag )
{
  return get_in_array( flag, plr_flags,
		       sizeof( plr_flags ) / sizeof( *plr_flags ), str_cmp );
}

int get_risflag( const char *flag )
{
  return get_in_array( flag, ris_flags,
		       sizeof( ris_flags ) / sizeof( *ris_flags ), str_cmp );
}

int get_trigflag( const char *flag )
{
  return get_in_array( flag, trig_flags,
		       sizeof( trig_flags ) / sizeof( *trig_flags ), str_cmp );
}

int get_attackflag( const char *flag )
{
  return get_in_array( flag, attack_flags,
		       sizeof( attack_flags ) / sizeof( *attack_flags ),
		       str_cmp );
}

int get_defenseflag( const char *flag )
{
  return get_in_array( flag, defense_flags,
		       sizeof( defense_flags ) / sizeof( *defense_flags ),
		       str_cmp );
}

const char *const spell_flag[] =
{ "water", "earth", "air", "astral", "area", "distant", "reverse",
  "save_half_dam", "save_negates", "accumulative", "recastable", "noscribe",
  "nobrew", "group", "object", "character", "secretskill", "pksensitive"
};

const char *const spell_saves[] =
{ "none", "poison_death", "wands", "para_petri", "breath", "spell_staff" };

const char *const spell_damage[] =
{ "none", "fire", "cold", "electricity", "energy", "acid", "poison",
  "drain" };

const char *const spell_action[] =
{ "none", "create", "destroy", "resist", "suscept", "divinate", "obscure",
  "change"
};

const char *const spell_power[] = { "none", "minor", "greater", "major" };

const char *const spell_class[] =
{ "none", "lunar", "solar", "travel", "summon", "life", "death",
  "illusion" };

const char *const target_type[] =
{ "ignore", "offensive", "defensive", "self", "objinv" };

int get_ssave( const char *name )
{
  return get_in_array( name, spell_saves,
		       sizeof( spell_saves ) / sizeof( spell_saves[0] ),
		       str_cmp );
}

int get_starget( const char *name )
{
  return get_in_array( name, target_type,
		       sizeof( target_type ) / sizeof( target_type[0] ),
		       str_cmp );
}

int get_sflag( const char *name )
{
  return get_in_array( name, spell_flag,
		       sizeof( spell_flag ) / sizeof( spell_flag[0] ),
		       str_cmp );
}

int get_sdamage( const char *name )
{
  return get_in_array( name, spell_damage,
		       sizeof( spell_damage ) / sizeof( spell_damage[0] ),
		       str_cmp );
}

int get_saction( const char *name )
{
  return get_in_array( name, spell_action,
		       sizeof( spell_action ) / sizeof( spell_action[0] ),
		       str_cmp );
}

int get_spower( const char *name )
{
  return get_in_array( name, spell_power,
		       sizeof( spell_power ) / sizeof( spell_power[0] ),
		       str_cmp );
}

int get_sclass( const char *name )
{
  return get_in_array( name, spell_class,
		       sizeof( spell_class ) / sizeof( spell_class[0] ),
		       str_cmp );
}

const char *const corpse_descs[] = {
  "The corpse of %s will soon be gone.",
  "The corpse of %s lies here.",
  "The corpse of %s lies here.",
  "The corpse of %s lies here.",
  "The corpse of %s lies here."
};

const char *const d_corpse_descs[] = {
  "The shattered remains %s will soon be gone.",
  "The shattered remains %s are here.",
  "The shattered remains %s are here.",
  "The shattered remains %s are here.",
  "The shattered remains %s are here."
};

const char * const position_name[] = {
  "dead",
  "mortally wounded",
  "incapacitated",
  "sleeping",
  "resting",
  "sitting",
  "fighting",
  "standing",
  "mounted",
  "shoved",
  "dragged"
};

size_t position_name_size( void )
{
  return sizeof( position_name ) / sizeof( *position_name );
}

int get_position( const char *pos )
{
  return get_in_array( pos, position_name, position_name_size(), str_prefix );
}

const char *get_position_name( int pos )
{
  if( pos >= (int) position_name_size() || pos < 0 )
    return "*out of bounds*";

  return position_name[pos];
}
