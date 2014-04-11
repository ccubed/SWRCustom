#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "mud.h"

#ifndef _WIN32
extern int _filbuf( FILE * );
#endif

void init_supermob( void );

/*
 * Globals.
 */

time_t last_restore_all_time = 0;

HELP_DATA *first_help = NULL;
HELP_DATA *last_help = NULL;

SHOP_DATA *first_shop = NULL;
SHOP_DATA *last_shop = NULL;

REPAIR_DATA *first_repair = NULL;
REPAIR_DATA *last_repair = NULL;

TELEPORT_DATA *first_teleport = NULL;
TELEPORT_DATA *last_teleport = NULL;

OBJ_DATA *extracted_obj_queue = NULL;
EXTRACT_CHAR_DATA *extracted_char_queue = NULL;

char bug_buf[2 * MAX_INPUT_LENGTH];
CHAR_DATA *first_char = NULL;
CHAR_DATA *last_char = NULL;
char *help_greeting = NULL;
char log_buf[2 * MAX_INPUT_LENGTH];

OBJ_DATA *first_object = NULL;
OBJ_DATA *last_object = NULL;
TIME_INFO_DATA time_info;
WEATHER_DATA weather_info;

int cur_qobjs = 0;
int cur_qchars = 0;
int nummobsloaded = 0;
int numobjsloaded = 0;
int physicalobjects = 0;

AUCTION_DATA *auction = NULL;	/* auctions */
OBJ_DATA *supermob_obj = NULL;

/* criminals */
short gsn_torture;
short gsn_disguise;
short gsn_pickshiplock;
short gsn_hijack;

/* soldiers and officers */
short gsn_reinforcements;
short gsn_postguard;
short gsn_first_aid;
short gsn_throw;

short gsn_quicktalk;
short gsn_propeganda;

/* pilots and smugglers */
short gsn_spacecraft;
short gsn_weaponsystems;
short gsn_shipmaintenance;
short gsn_shipdesign;
short gsn_spacecombat;

/* player building skills */
short gsn_lightsaber_crafting;
short gsn_spice_refining;
short gsn_makeblade;
short gsn_makeblaster;
short gsn_makelight;
short gsn_makecomlink;
short gsn_makearmor;
short gsn_makeshield;
short gsn_makecontainer;
short gsn_makejewelry;

short gsn_bridge;
short gsn_survey;
short gsn_landscape;
short gsn_construction;

/* weaponry */
short gsn_blasters;
short gsn_bowcasters;
short gsn_force_pikes;
short gsn_lightsabers;
short gsn_vibro_blades;
short gsn_flexible_arms;
short gsn_talonous_arms;
short gsn_bludgeons;

/* thief */
short gsn_backstab;
short gsn_circle;
short gsn_dodge;
short gsn_hide;
short gsn_peek;
short gsn_pick_lock;
short gsn_sneak;
short gsn_steal;
short gsn_gouge;
short gsn_poison_weapon;

/* thief & warrior */
short gsn_enhanced_damage;
short gsn_kick;
short gsn_parry;
short gsn_rescue;
short gsn_second_attack;
short gsn_third_attack;
short gsn_dual_wield;
short gsn_bashdoor;
short gsn_grip;
short gsn_berserk;
short gsn_hitall;
short gsn_disarm;

/* other   */
short gsn_aid;
short gsn_track;
short gsn_mount;
short gsn_climb;
short gsn_slice;

/* spells */
short gsn_aqua_breath;
short gsn_blindness;
short gsn_charm_person;
short gsn_invis;
short gsn_mass_invis;
short gsn_poison;
short gsn_sleep;
short gsn_stun;
short gsn_possess;
short gsn_fireball;
short gsn_lightning_bolt;

/* for searching */
short gsn_first_spell;
short gsn_first_skill;
short gsn_first_weapon;
short gsn_top_sn;

/*
 * Locals.
 */
MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];

AREA_DATA *first_area = NULL;
AREA_DATA *last_area = NULL;
AREA_DATA *first_build = NULL;
AREA_DATA *last_build = NULL;

SYSTEM_DATA sysdata;

int top_affect = 0;
int top_area = 0;
int top_ed = 0;
int top_exit = 0;
int top_help = 0;
int top_mob_index = 0;
int top_obj_index = 0;
int top_room = 0;
int top_r_vnum = 0;
int top_shop = 0;
int top_repair = 0;
int top_vroom = 0;

/*
 * Semi-locals.
 */
bool fBootDb = FALSE;
FILE *fpArea = NULL;
char strArea[MAX_INPUT_LENGTH];

bool MOBtrigger = TRUE;

/*
 * Local booting procedures.
 */
#ifdef __cplusplus
extern "C"
#endif
void init_mm( void );

static void boot_log( const char *str, ... );
static void load_area( FILE * fp );
static void load_flags( AREA_DATA * tarea, FILE * fp );
static void boot_load_helps( void );
static void load_mobiles( AREA_DATA * tarea, FILE * fp );
static void load_objects( AREA_DATA * tarea, FILE * fp );
static void load_rooms( AREA_DATA * tarea, FILE * fp );
static void load_shops( AREA_DATA * tarea, FILE * fp );
static void load_repairs( AREA_DATA * tarea, FILE * fp );
static void load_specials( AREA_DATA * tarea, FILE * fp );
static bool load_systemdata( SYSTEM_DATA * sys );
static void load_banlist( void );
static void fix_exits( void );

/*
 * External booting function
 */
void load_corpses( void );

/*
 * MUDprogram locals
 */

static int mprog_name_to_type( const char *name );
static void mprog_read_programs( FILE * fp, MOB_INDEX_DATA * pMobIndex );
static void oprog_read_programs( FILE * fp, OBJ_INDEX_DATA * pObjIndex );
static void rprog_read_programs( FILE * fp, ROOM_INDEX_DATA * pRoomIndex );
void unlink_social( SOCIALTYPE * social );

#ifdef __cplusplus
extern "C"
#endif
void shutdown_mud( const char *reason )
{
  FILE *fp;

  if( ( fp = fopen( SHUTDOWN_FILE, "a" ) ) != NULL )
  {
    fprintf( fp, "%s\n", reason );
    fclose( fp );
  }
}

static void boot_init_globals( void )
{
  size_t wear = 0, x = 0;

  top_r_vnum = 0;
  nummobsloaded = 0;
  numobjsloaded = 0;
  physicalobjects = 0;
  sysdata.maxplayers = 0;
  first_object = NULL;
  last_object = NULL;
  first_char = NULL;
  last_char = NULL;
  first_area = NULL;
  last_area = NULL;
  first_build = NULL;
  last_area = NULL;
  first_shop = NULL;
  last_shop = NULL;
  first_repair = NULL;
  last_repair = NULL;
  first_teleport = NULL;
  last_teleport = NULL;
  extracted_obj_queue = NULL;
  extracted_char_queue = NULL;
  cur_qobjs = 0;
  cur_qchars = 0;
  cur_char = NULL;
  cur_obj = 0;
  cur_obj_serial = 0;
  cur_char_died = FALSE;
  cur_obj_extracted = FALSE;
  cur_room = NULL;
  quitting_char = NULL;
  loading_char = NULL;
  saving_char = NULL;
  CREATE( auction, AUCTION_DATA, 1 );
  auction->item = NULL;

  for( wear = 0; wear < MAX_WEAR; wear++ )
  {
    for( x = 0; x < MAX_LAYERS; x++ )
    {
      save_equipment[wear][x] = NULL;
    }
  }
}

static void boot_assign_global_skillnumbers( void )
{
  ASSIGN_GSN( gsn_survey, "surveying" );
  ASSIGN_GSN( gsn_landscape, "landscape and design" );
  ASSIGN_GSN( gsn_construction, "construction" );
  ASSIGN_GSN( gsn_quicktalk, "quicktalk" );
  ASSIGN_GSN( gsn_bridge, "bridges and exits" );
  ASSIGN_GSN( gsn_propeganda, "propeganda" );
  ASSIGN_GSN( gsn_hijack, "hijack" );
  ASSIGN_GSN( gsn_makejewelry, "makejewelry" );
  ASSIGN_GSN( gsn_makeblade, "makeblade" );
  ASSIGN_GSN( gsn_makeblaster, "makeblaster" );
  ASSIGN_GSN( gsn_makelight, "makeflashlight" );
  ASSIGN_GSN( gsn_makecomlink, "makecomlink" );
  ASSIGN_GSN( gsn_makearmor, "makearmor" );
  ASSIGN_GSN( gsn_makeshield, "makeshield" );
  ASSIGN_GSN( gsn_makecontainer, "makecontainer" );
  ASSIGN_GSN( gsn_reinforcements, "reinforcements" );
  ASSIGN_GSN( gsn_postguard, "post guard" );
  ASSIGN_GSN( gsn_torture, "torture" );
  ASSIGN_GSN( gsn_throw, "throw" );
  ASSIGN_GSN( gsn_disguise, "disguise" );
  ASSIGN_GSN( gsn_first_aid, "first aid" );
  ASSIGN_GSN( gsn_lightsaber_crafting, "lightsaber crafting" );
  ASSIGN_GSN( gsn_spice_refining, "spice refining" );
  ASSIGN_GSN( gsn_spacecombat, "space combat" );
  ASSIGN_GSN( gsn_weaponsystems, "weapon systems" );
  ASSIGN_GSN( gsn_spacecraft, "spacecraft" );
  ASSIGN_GSN( gsn_shipdesign, "ship design" );
  ASSIGN_GSN( gsn_shipmaintenance, "ship maintenance" );
  ASSIGN_GSN( gsn_blasters, "blasters" );
  ASSIGN_GSN( gsn_bowcasters, "bowcasters" );
  ASSIGN_GSN( gsn_force_pikes, "force pikes" );
  ASSIGN_GSN( gsn_lightsabers, "lightsabers" );
  ASSIGN_GSN( gsn_vibro_blades, "vibro-blades" );
  ASSIGN_GSN( gsn_flexible_arms, "flexible arms" );
  ASSIGN_GSN( gsn_talonous_arms, "talonous arms" );
  ASSIGN_GSN( gsn_bludgeons, "bludgeons" );
  ASSIGN_GSN( gsn_backstab, "backstab" );
  ASSIGN_GSN( gsn_circle, "circle" );
  ASSIGN_GSN( gsn_dodge, "dodge" );
  ASSIGN_GSN( gsn_hide, "hide" );
  ASSIGN_GSN( gsn_peek, "peek" );
  ASSIGN_GSN( gsn_pick_lock, "picklock" );
  ASSIGN_GSN( gsn_pickshiplock, "pickshiplock" );
  ASSIGN_GSN( gsn_sneak, "sneak" );
  ASSIGN_GSN( gsn_steal, "steal" );
  ASSIGN_GSN( gsn_gouge, "gouge" );
  ASSIGN_GSN( gsn_poison_weapon, "poison weapon" );
  ASSIGN_GSN( gsn_disarm, "disarm" );
  ASSIGN_GSN( gsn_enhanced_damage, "enhanced damage" );
  ASSIGN_GSN( gsn_kick, "kick" );
  ASSIGN_GSN( gsn_parry, "parry" );
  ASSIGN_GSN( gsn_rescue, "rescue" );
  ASSIGN_GSN( gsn_second_attack, "second attack" );
  ASSIGN_GSN( gsn_third_attack, "third attack" );
  ASSIGN_GSN( gsn_dual_wield, "dual wield" );
  ASSIGN_GSN( gsn_bashdoor, "doorbash" );
  ASSIGN_GSN( gsn_grip, "grip" );
  ASSIGN_GSN( gsn_berserk, "berserk" );
  ASSIGN_GSN( gsn_hitall, "hitall" );
  ASSIGN_GSN( gsn_aid, "aid" );
  ASSIGN_GSN( gsn_track, "track" );
  ASSIGN_GSN( gsn_mount, "mount" );
  ASSIGN_GSN( gsn_climb, "climb" );
  ASSIGN_GSN( gsn_slice, "slice" );
  ASSIGN_GSN( gsn_fireball, "fireball" );
  ASSIGN_GSN( gsn_lightning_bolt, "lightning bolt" );
  ASSIGN_GSN( gsn_aqua_breath, "aqua breath" );
  ASSIGN_GSN( gsn_blindness, "blindness" );
  ASSIGN_GSN( gsn_charm_person, "affect mind" );
  ASSIGN_GSN( gsn_invis, "mask" );
  ASSIGN_GSN( gsn_mass_invis, "group masking" );
  ASSIGN_GSN( gsn_poison, "poison" );
  ASSIGN_GSN( gsn_sleep, "sleep" );
  ASSIGN_GSN( gsn_stun, "stun" );
  ASSIGN_GSN( gsn_possess, "possess" );
}

static void boot_set_time_weather( void )
{
  long lday = 0, lmonth = 0, lhour = 0;

  lhour = ( current_time - 650336715 ) / ( PULSE_TICK / PULSE_PER_SECOND );
  time_info.hour = lhour % 24;
  lday = lhour / 24;
  time_info.day = lday % 35;
  lmonth = lday / 35;
  time_info.month = lmonth % 17;
  time_info.year = lmonth / 17;

  if( time_info.hour < 5 )
    weather_info.sunlight = SUN_DARK;
  else if( time_info.hour < 6 )
    weather_info.sunlight = SUN_RISE;
  else if( time_info.hour < 19 )
    weather_info.sunlight = SUN_LIGHT;
  else if( time_info.hour < 20 )
    weather_info.sunlight = SUN_SET;
  else
    weather_info.sunlight = SUN_DARK;

  weather_info.change = 0;
  weather_info.mmhg = 960;

  if( time_info.month >= 7 && time_info.month <= 12 )
    weather_info.mmhg += number_range( 1, 50 );
  else
    weather_info.mmhg += number_range( 1, 80 );

  if( weather_info.mmhg <= 980 )
    weather_info.sky = SKY_LIGHTNING;
  else if( weather_info.mmhg <= 1000 )
    weather_info.sky = SKY_RAINING;
  else if( weather_info.mmhg <= 1020 )
    weather_info.sky = SKY_CLOUDY;
  else
    weather_info.sky = SKY_CLOUDLESS;
}

static void boot_read_area_files( void )
{
  FILE *fpList = NULL;

  if( ( fpList = fopen( AREA_DIR AREA_LIST, "r" ) ) == NULL )
  {
    shutdown_mud( "Unable to open area list" );
    exit( 1 );
  }

  for( ;; )
  {
    strcpy( strArea, fread_word( fpList ) );

    if( strArea[0] == '$' )
      break;

    load_area_file( last_area, strArea );
  }

  fclose( fpList );
}

/*
 * Big mama top level function.
 */
void boot_db( bool fCopyOver )
{
  int x = 0;
  show_hash( 32 );
  remove( BOOTLOG_FILE );
  boot_log( "---------------------[ Boot Log ]--------------------" );

  log_string( "Loading commands" );
  load_commands();

  log_string( "Loading sysdata configuration..." );

  /* default values */
  sysdata.save_frequency = 20;	/* minutes */
  sysdata.save_flags = SV_DEATH | SV_PASSCHG | SV_AUTO
    | SV_PUT | SV_DROP | SV_GIVE | SV_AUCTION | SV_ZAPDROP | SV_IDLE;
  if( !load_systemdata( &sysdata ) )
  {
    log_string( "Not found.  Creating new configuration." );
    sysdata.alltimemax = 0;
  }

  log_string( "Loading socials" );
  load_socials();

  log_string( "Loading skill table" );
  load_skill_table();
  sort_skill_table();

  gsn_first_spell = 0;
  gsn_first_skill = 0;
  gsn_first_weapon = 0;
  gsn_top_sn = top_sn;

  for( x = 0; x < top_sn; x++ )
  {
    if( !gsn_first_spell && skill_table[x]->type == SKILL_SPELL )
    {
      gsn_first_spell = x;
    }
    else if( !gsn_first_skill && skill_table[x]->type == SKILL_SKILL )
    {
      gsn_first_skill = x;
    }
    else if( !gsn_first_weapon && skill_table[x]->type == SKILL_WEAPON )
    {
      gsn_first_weapon = x;
    }
  }

  fBootDb = TRUE;

  boot_init_globals();

  /*
   * Init random number generator.
   */
  log_string( "Initializing random number generator" );
  init_mm();

  /*
   * Set time and weather.
   */
  log_string( "Setting time and weather" );
  boot_set_time_weather();

  /*
   * Assign gsn's for skills which need them.
   */
  log_string( "Assigning gsn's" );
  boot_assign_global_skillnumbers();

  /*
   * Read help file.
   */
  log_string( "Reading help entries..." );
  boot_load_helps();

  /*
   * Read in all the area files.
   */
  log_string( "Reading in area files..." );
  boot_read_area_files();

  init_supermob();


  /*
   * Fix up exits.
   * Declare db booting over.
   * Reset all areas once.
   * Load up the notes file.
   */
  log_string( "Fixing exits" );
  fix_exits();
  fBootDb = FALSE;
  log_string( "Loading boards" );
  load_boards();
  log_string( "Loading clans" );
  load_clans();
  log_string( "Loading bans" );
  load_banlist();
  log_string( "Loading corpses" );
  load_corpses();
  log_string( "Loading space" );
  load_space();
  log_string( "Loading ship prototypes" );
  load_prototypes();
  log_string( "Loading ships" );
  load_ships();
  log_string( "Loading planet data" );
  load_planets();
  log_string( "Resetting areas" );
  reset_all();

  MOBtrigger = TRUE;

  if( fCopyOver )
  {
    log_string( "Running copyover_recover." );
    copyover_recover();
  }
}

/*
 * Load an 'area' header line.
 */
void load_area( FILE * fp )
{
  AREA_DATA *pArea = 0;

  CREATE( pArea, AREA_DATA, 1 );
  pArea->first_room = NULL;
  pArea->last_room = NULL;
  pArea->planet = NULL;
  pArea->name = fread_string_nohash( fp );
  pArea->filename = str_dup( strArea );

  LINK( pArea, first_area, last_area, next, prev );
  top_area++;
}

/*
 * Load area flags. Narn, Mar/96 
 */
static void load_flags( AREA_DATA * tarea, FILE * fp )
{
  const char *ln = NULL;
  int x1 = 0, x2 = 0;

  if( !tarea )
  {
    bug( "Load_flags: no #AREA seen yet." );

    if( fBootDb )
    {
      shutdown_mud( "No #AREA" );
      exit( 1 );
    }
    else
    {
      return;
    }
  }

  ln = fread_line( fp );
  sscanf( ln, "%d %d", &x1, &x2 );
  tarea->flags = x1;
}

/*
 * Adds a help page to the list if it is not a duplicate of an existing page.
 * Page is insert-sorted by keyword.			-Thoric
 * (The reason for sorting is to keep do_hlist looking nice)
 */
void add_help( HELP_DATA * pHelp )
{
  HELP_DATA *tHelp = NULL;
  int match = 0;

  for( tHelp = first_help; tHelp; tHelp = tHelp->next )
  {
    if( pHelp->level == tHelp->level
	&& strcmp( pHelp->keyword, tHelp->keyword ) == 0 )
    {
      bug( "add_help: duplicate: %s.  Deleting.", pHelp->keyword );
      free_help( pHelp );
      return;
    }
    else
      if( ( match =
	    strcmp( pHelp->keyword[0] ==
	      '\'' ? pHelp->keyword + 1 : pHelp->keyword,
	      tHelp->keyword[0] ==
	      '\'' ? tHelp->keyword + 1 : tHelp->keyword ) ) < 0
	  || ( match == 0 && pHelp->level > tHelp->level ) )
      {
	if( !tHelp->prev )
	  first_help = pHelp;
	else
	  tHelp->prev->next = pHelp;

	pHelp->prev = tHelp->prev;
	pHelp->next = tHelp;
	tHelp->prev = pHelp;
	break;
      }
  }

  if( !tHelp )
    LINK( pHelp, first_help, last_help, next, prev );

  top_help++;
}

/*
 * Load a help section.
 */
void boot_load_helps( void )
{
  FILE *fp = NULL;
  HELP_DATA *pHelp = NULL;

  if( !( fp = fopen( HELP_FILE, "r" ) ) )
    {
      log_printf( "Unable to open %s", HELP_FILE );
      return;
    }

  for( ;; )
  {
    CREATE( pHelp, HELP_DATA, 1 );
    pHelp->level = fread_number( fp );
    pHelp->keyword = fread_string( fp );

    if( pHelp->keyword[0] == '$' )
    {
      free_help( pHelp );
      break;
    }

    pHelp->text = fread_string( fp );

    if( pHelp->keyword[0] == '\0' )
    {
      free_help( pHelp );
      continue;
    }

    if( !str_cmp( pHelp->keyword, "greeting" ) )
      help_greeting = pHelp->text;

    add_help( pHelp );
  }
}


/*
 * Add a character to the list of all characters		-Thoric
 */
void add_char( CHAR_DATA * ch )
{
  LINK( ch, first_char, last_char, next, prev );
}


/*
 * Load a mob section.
 */
void load_mobiles( AREA_DATA * tarea, FILE * fp )
{
  MOB_INDEX_DATA *pMobIndex;
  char *ln;
  int x1, x2, x3, x4, x5, x6, x7, x8;

  for( ;; )
  {
    char buf[MAX_STRING_LENGTH];
    long vnum;
    char letter;
    int iHash;
    bool oldmob;
    bool tmpBootDb;

    letter = fread_letter( fp );
    if( letter != '#' )
    {
      bug( "Load_mobiles: # not found." );
      if( fBootDb )
      {
	shutdown_mud( "# not found" );
	exit( 1 );
      }
      else
	return;
    }

    vnum = fread_number( fp );
    if( vnum == 0 )
      break;

    tmpBootDb = fBootDb;
    fBootDb = FALSE;
    if( get_mob_index( vnum ) )
    {
      if( tmpBootDb )
      {
	bug( "Load_mobiles: vnum %ld duplicated.", vnum );
	shutdown_mud( "duplicate vnum" );
	exit( 1 );
      }
      else
      {
	pMobIndex = get_mob_index( vnum );
	sprintf( buf, "Cleaning mobile: %ld", vnum );
	log_string_plus( buf, LOG_BUILD );
	clean_mob( pMobIndex );
	oldmob = TRUE;
      }
    }
    else
    {
      oldmob = FALSE;
      CREATE( pMobIndex, MOB_INDEX_DATA, 1 );
    }
    fBootDb = tmpBootDb;

    pMobIndex->vnum = vnum;
    pMobIndex->player_name = fread_string( fp );
    pMobIndex->short_descr = fread_string( fp );
    pMobIndex->long_descr = fread_string( fp );
    pMobIndex->description = fread_string( fp );

    pMobIndex->long_descr[0] = UPPER( pMobIndex->long_descr[0] );
    pMobIndex->description[0] = UPPER( pMobIndex->description[0] );

    pMobIndex->act = fread_number( fp ) | ACT_IS_NPC;
    pMobIndex->affected_by = fread_number( fp );
    pMobIndex->pShop = NULL;
    pMobIndex->rShop = NULL;
    pMobIndex->alignment = fread_number( fp );
    letter = fread_letter( fp );
    /*pMobIndex->level              = */ fread_number( fp );

    /*pMobIndex->mobthac0 =*/ fread_number( fp );
    pMobIndex->ac = fread_number( fp );
    pMobIndex->hitnodice = fread_number( fp );
    /* 'd'          */ fread_letter( fp );
    pMobIndex->hitsizedice = fread_number( fp );
    /* '+'          */ fread_letter( fp );
    pMobIndex->hitplus = fread_number( fp );
    pMobIndex->damnodice = fread_number( fp );
    /* 'd'          */ fread_letter( fp );
    pMobIndex->damsizedice = fread_number( fp );
    /* '+'          */ fread_letter( fp );
    pMobIndex->damplus = fread_number( fp );
    pMobIndex->gold = fread_number( fp );
    /*pMobIndex->exp = */ fread_number( fp );
    pMobIndex->position = fread_number( fp );
    pMobIndex->defposition = fread_number( fp );

    /*
     * Back to meaningful values.
     */
    pMobIndex->sex = fread_number( fp );

    if( letter != 'S' && letter != 'C' && letter != 'Z' )
    {
      bug( "Load_mobiles: vnum %ld: letter '%c' not Z, S or C.", vnum,
	  letter );
      shutdown_mud( "bad mob data" );
      exit( 1 );
    }
    if( letter == 'C' || letter == 'Z' )	/* Realms complex mob     -Thoric  */
    {
      pMobIndex->perm_str = fread_number( fp );
      pMobIndex->perm_int = fread_number( fp );
      pMobIndex->perm_wis = fread_number( fp );
      pMobIndex->perm_dex = fread_number( fp );
      pMobIndex->perm_con = fread_number( fp );
      pMobIndex->perm_cha = fread_number( fp );
      pMobIndex->perm_lck = fread_number( fp );
      pMobIndex->saving_poison_death = fread_number( fp );
      pMobIndex->saving_wand = fread_number( fp );
      pMobIndex->saving_para_petri = fread_number( fp );
      pMobIndex->saving_breath = fread_number( fp );
      pMobIndex->saving_spell_staff = fread_number( fp );
      ln = fread_line( fp );
      x1 = x2 = x3 = x4 = x5 = x6 = x7 = 0;
      sscanf( ln, "%d %d %d %d %d %d %d",
	  &x1, &x2, &x3, &x4, &x5, &x6, &x7 );
      pMobIndex->height = x3;
      pMobIndex->weight = x4;
      pMobIndex->numattacks = x7;

      ln = fread_line( fp );
      x1 = x2 = x3 = x4 = x5 = x6 = x7 = x8 = 0;
      sscanf( ln, "%d %d %d %d %d %d %d %d",
	  &x1, &x2, &x3, &x4, &x5, &x6, &x7, &x8 );
      pMobIndex->hitroll = x1;
      pMobIndex->damroll = x2;
      /*pMobIndex->xflags = x3;*/
      pMobIndex->resistant = x4;
      pMobIndex->immune = x5;
      pMobIndex->susceptible = x6;
      pMobIndex->attacks = x7;
      pMobIndex->defenses = x8;
    }
    else
    {
      pMobIndex->perm_str = 10;
      pMobIndex->perm_dex = 10;
      pMobIndex->perm_int = 10;
      pMobIndex->perm_wis = 10;
      pMobIndex->perm_cha = 10;
      pMobIndex->perm_con = 10;
      pMobIndex->perm_lck = 10;
      pMobIndex->resistant = 0;
      pMobIndex->immune = 0;
      pMobIndex->susceptible = 0;
      pMobIndex->numattacks = 0;
      pMobIndex->attacks = 0;
      pMobIndex->defenses = 0;
    }
    if( letter == 'Z' )	/*  STar Wars Reality Complex Mob  */
    {
      ln = fread_line( fp );
      x1 = x2 = x3 = x4 = x5 = x6 = x7 = x8 = 0;
      sscanf( ln, "%d %d %d %d %d %d %d %d",
	  &x1, &x2, &x3, &x4, &x5, &x6, &x7, &x8 );
    }

    letter = fread_letter( fp );
    if( letter == '>' )
    {
      ungetc( letter, fp );
      mprog_read_programs( fp, pMobIndex );
    }
    else
      ungetc( letter, fp );

    if( !oldmob )
    {
      iHash = vnum % MAX_KEY_HASH;
      pMobIndex->next = mob_index_hash[iHash];
      mob_index_hash[iHash] = pMobIndex;
      top_mob_index++;
    }
  }

  return;
}



/*
 * Load an obj section.
 */
void load_objects( AREA_DATA * tarea, FILE * fp )
{
  OBJ_INDEX_DATA *pObjIndex;
  char letter;
  char *ln;
  int x1, x2, x3, x4, x5, x6;

  for( ;; )
  {
    char buf[MAX_STRING_LENGTH];
    long vnum;
    int iHash;
    bool tmpBootDb;
    bool oldobj;

    letter = fread_letter( fp );
    if( letter != '#' )
    {
      bug( "Load_objects: # not found." );
      if( fBootDb )
      {
	shutdown_mud( "# not found" );
	exit( 1 );
      }
      else
	return;
    }

    vnum = fread_number( fp );
    if( vnum == 0 )
      break;

    tmpBootDb = fBootDb;
    fBootDb = FALSE;
    if( get_obj_index( vnum ) )
    {
      if( tmpBootDb )
      {
	bug( "Load_objects: vnum %ld duplicated.", vnum );
	shutdown_mud( "duplicate vnum" );
	exit( 1 );
      }
      else
      {
	pObjIndex = get_obj_index( vnum );
	sprintf( buf, "Cleaning object: %ld", vnum );
	log_string_plus( buf, LOG_BUILD );
	clean_obj( pObjIndex );
	oldobj = TRUE;
      }
    }
    else
    {
      oldobj = FALSE;
      CREATE( pObjIndex, OBJ_INDEX_DATA, 1 );
    }
    fBootDb = tmpBootDb;

    pObjIndex->vnum = vnum;
    pObjIndex->name = fread_string( fp );
    pObjIndex->short_descr = fread_string( fp );
    pObjIndex->description = fread_string( fp );
    pObjIndex->action_desc = fread_string( fp );

    /* Commented out by Narn, Apr/96 to allow item short descs like 
       Bonecrusher and Oblivion */
    /*pObjIndex->short_descr[0]     = LOWER(pObjIndex->short_descr[0]); */
    pObjIndex->description[0] = UPPER( pObjIndex->description[0] );

    ln = fread_line( fp );
    x1 = x2 = x3 = x4 = 0;
    sscanf( ln, "%d %d %d %d", &x1, &x2, &x3, &x4 );
    pObjIndex->item_type = x1;
    pObjIndex->extra_flags = x2;
    pObjIndex->wear_flags = x3;
    pObjIndex->layers = x4;

    ln = fread_line( fp );
    x1 = x2 = x3 = x4 = x5 = x6 = 0;
    sscanf( ln, "%d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6 );
    pObjIndex->value[0] = x1;
    pObjIndex->value[1] = x2;
    pObjIndex->value[2] = x3;
    pObjIndex->value[3] = x4;
    pObjIndex->value[4] = x5;
    pObjIndex->value[5] = x6;
    pObjIndex->weight = fread_number( fp );
    pObjIndex->weight = UMAX( 1, pObjIndex->weight );
    pObjIndex->cost = fread_number( fp );

    fread_number( fp );	/*extra */

    for( ;; )
    {
      letter = fread_letter( fp );

      if( letter == 'A' )
      {
	AFFECT_DATA *paf;

	CREATE( paf, AFFECT_DATA, 1 );
	paf->type = -1;
	paf->duration = -1;
	paf->location = fread_number( fp );
	if( paf->location == APPLY_WEAPONSPELL
	    || paf->location == APPLY_WEARSPELL
	    || paf->location == APPLY_REMOVESPELL
	    || paf->location == APPLY_STRIPSN )
	  paf->modifier = slot_lookup( fread_number( fp ) );
	else
	  paf->modifier = fread_number( fp );
	paf->bitvector = 0;
	LINK( paf, pObjIndex->first_affect, pObjIndex->last_affect,
	    next, prev );
	top_affect++;
      }

      else if( letter == 'E' )
      {
	EXTRA_DESCR_DATA *ed;

	CREATE( ed, EXTRA_DESCR_DATA, 1 );
	ed->keyword = fread_string( fp );
	ed->description = fread_string( fp );
	LINK( ed, pObjIndex->first_extradesc, pObjIndex->last_extradesc,
	    next, prev );
	top_ed++;
      }

      else if( letter == '>' )
      {
	ungetc( letter, fp );
	oprog_read_programs( fp, pObjIndex );
      }

      else
      {
	ungetc( letter, fp );
	break;
      }
    }

    /*
     * Translate spell "slot numbers" to internal "skill numbers."
     */
    switch ( pObjIndex->item_type )
    {
      case ITEM_DEVICE:
	pObjIndex->value[3] = slot_lookup( pObjIndex->value[3] );
	break;
    }

    if( !oldobj )
    {
      iHash = vnum % MAX_KEY_HASH;
      pObjIndex->next = obj_index_hash[iHash];
      obj_index_hash[iHash] = pObjIndex;
      top_obj_index++;
    }
  }

  return;
}



/*
 * Load a room section.
 */
void load_rooms( AREA_DATA * tarea, FILE * fp )
{
  ROOM_INDEX_DATA *pRoomIndex = NULL;
  char buf[MAX_STRING_LENGTH];
  char *ln = NULL;

  if( !tarea )
  {
    bug( "Load_rooms: no #AREA seen yet." );
    shutdown_mud( "No #AREA" );
    exit( 1 );
  }

  for( ;; )
  {
    long vnum = 0;
    char letter = 0;
    int door = 0;
    int iHash = 0;
    bool tmpBootDb = FALSE;
    bool oldroom = FALSE;
    int x1 = 0, x2 = 0, x3 = 0, x4 = 0, x5 = 0, x6 = 0;

    letter = fread_letter( fp );

    if( letter != '#' )
    {
      bug( "Load_rooms: # not found." );

      if( fBootDb )
      {
	shutdown_mud( "# not found" );
	exit( 1 );
      }
      else
      {
	return;
      }
    }

    vnum = fread_number( fp );

    if( vnum == 0 )
      break;

    tmpBootDb = fBootDb;
    fBootDb = FALSE;

    if( get_room_index( vnum ) != NULL )
    {
      if( tmpBootDb )
      {
	bug( "Load_rooms: vnum %d duplicated.", vnum );
	shutdown_mud( "duplicate vnum" );
	exit( 1 );
      }
      else
      {
	pRoomIndex = get_room_index( vnum );
	sprintf( buf, "Cleaning room: %ld", vnum );
	log_string_plus( buf, LOG_BUILD );

	if( pRoomIndex->area )
	  UNLINK( pRoomIndex, pRoomIndex->area->first_room,
	      pRoomIndex->area->last_room,
	      next_in_area, prev_in_area );

	clean_room( pRoomIndex );
	oldroom = TRUE;
      }
    }
    else
    {
      oldroom = FALSE;
      CREATE( pRoomIndex, ROOM_INDEX_DATA, 1 );
      pRoomIndex->first_person = NULL;
      pRoomIndex->last_person = NULL;
      pRoomIndex->first_content = NULL;
      pRoomIndex->last_content = NULL;
      pRoomIndex->next_in_area = NULL;
      pRoomIndex->prev_in_area = NULL;
      pRoomIndex->next_in_ship = NULL;
      pRoomIndex->prev_in_ship = NULL;
    }

    fBootDb = tmpBootDb;
    pRoomIndex->area = tarea;
    pRoomIndex->vnum = vnum;
    pRoomIndex->first_extradesc = NULL;
    pRoomIndex->last_extradesc = NULL;

    pRoomIndex->name = fread_string( fp );
    pRoomIndex->description = fread_string( fp );

    /* Area number                      fread_number( fp ); */
    ln = fread_line( fp );
    x1 = x2 = x3 = x4 = x5 = x6 = 0;
    sscanf( ln, "%d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6 );

    pRoomIndex->room_flags = x2;
    pRoomIndex->sector_type = x3;
    pRoomIndex->tele_delay = x4;
    pRoomIndex->tele_vnum = x5;
    pRoomIndex->tunnel = x6;

    if( pRoomIndex->sector_type < 0 || pRoomIndex->sector_type == SECT_MAX )
    {
      bug( "Fread_rooms: vnum %d has bad sector_type %d.", vnum,
	  pRoomIndex->sector_type );
      pRoomIndex->sector_type = 1;
    }

    pRoomIndex->light = 0;
    pRoomIndex->first_exit = NULL;
    pRoomIndex->last_exit = NULL;

    for( ;; )
    {
      letter = fread_letter( fp );

      if( letter == 'S' )
	break;

      if( letter == 'D' )
      {
	EXIT_DATA *pexit = NULL;
	int locks = 0;

	door = fread_number( fp );

	if( door < 0 || door > 10 )
	{
	  bug( "Fread_rooms: vnum %d has bad door number %d.", vnum,
	      door );

	  if( fBootDb )
	  {
	    exit( 1 );
	  }
	}
	else
	{
	  pexit = make_exit( pRoomIndex, NULL, door );
	  pexit->description = fread_string( fp );
	  pexit->keyword = fread_string( fp );
	  pexit->exit_info = 0;
	  ln = fread_line( fp );
	  x1 = x2 = x3 = x4 = 0;
	  sscanf( ln, "%d %d %d %d", &x1, &x2, &x3, &x4 );

	  locks = x1;
	  pexit->key = x2;
	  pexit->vnum = x3;
	  pexit->vdir = door;
	  pexit->distance = x4;

	  switch ( locks )
	  {
	    case 1:
	      pexit->exit_info = EX_ISDOOR;
	      break;
	    case 2:
	      pexit->exit_info = EX_ISDOOR | EX_PICKPROOF;
	      break;
	    default:
	      pexit->exit_info = locks;
	  }
	}
      }
      else if( letter == 'E' )
      {
	EXTRA_DESCR_DATA *ed = NULL;

	CREATE( ed, EXTRA_DESCR_DATA, 1 );
	ed->keyword = fread_string( fp );
	ed->description = fread_string( fp );
	LINK( ed, pRoomIndex->first_extradesc,
	    pRoomIndex->last_extradesc, next, prev );
	top_ed++;
      }
      else if( letter == '>' )
      {
	ungetc( letter, fp );
	rprog_read_programs( fp, pRoomIndex );
      }
      else
      {
	bug( "Load_rooms: vnum %d has flag '%c' not 'DES'.", vnum,
	    letter );
	shutdown_mud( "Room flag not DES" );
	exit( 1 );
      }
    }

    if( !oldroom )
    {
      iHash = vnum % MAX_KEY_HASH;
      pRoomIndex->next = room_index_hash[iHash];
      room_index_hash[iHash] = pRoomIndex;
      top_room++;
    }

    if( vnum > top_r_vnum )
      top_r_vnum = vnum;

    LINK( pRoomIndex, tarea->first_room, tarea->last_room, next_in_area,
	prev_in_area );
  }
}



/*
 * Load a shop section.
 */
void load_shops( AREA_DATA * tarea, FILE * fp )
{
  SHOP_DATA *pShop;

  for( ;; )
  {
    MOB_INDEX_DATA *pMobIndex;
    int iTrade;
    int keeper = fread_number( fp );

    if( keeper == 0 )
    {
      break;
    }

    CREATE( pShop, SHOP_DATA, 1 );
    pShop->keeper = keeper;

    for( iTrade = 0; iTrade < MAX_TRADE; iTrade++ )
      pShop->buy_type[iTrade] = fread_number( fp );

    pShop->profit_buy = fread_number( fp );
    pShop->profit_sell = fread_number( fp );
    pShop->profit_buy =
      URANGE( pShop->profit_sell + 5, pShop->profit_buy, 1000 );
    pShop->profit_sell =
      URANGE( 0, pShop->profit_sell, pShop->profit_buy - 5 );
    pShop->open_hour = fread_number( fp );
    pShop->close_hour = fread_number( fp );
    fread_to_eol( fp );
    pMobIndex = get_mob_index( pShop->keeper );
    pMobIndex->pShop = pShop;

    if( !first_shop )
      first_shop = pShop;
    else
      last_shop->next = pShop;

    pShop->next = NULL;
    pShop->prev = last_shop;
    last_shop = pShop;
    top_shop++;
  }
}

/*
 * Load a repair shop section.					-Thoric
 */
void load_repairs( AREA_DATA * tarea, FILE * fp )
{
  REPAIR_DATA *rShop;

  for( ;; )
  {
    MOB_INDEX_DATA *pMobIndex;
    int iFix;
    int keeper = fread_number( fp );

    if( keeper == 0 )
      break;

    CREATE( rShop, REPAIR_DATA, 1 );
    rShop->keeper = keeper;

    for( iFix = 0; iFix < MAX_FIX; iFix++ )
      rShop->fix_type[iFix] = fread_number( fp );

    rShop->profit_fix = fread_number( fp );
    rShop->shop_type = fread_number( fp );
    rShop->open_hour = fread_number( fp );
    rShop->close_hour = fread_number( fp );
    fread_to_eol( fp );
    pMobIndex = get_mob_index( rShop->keeper );
    pMobIndex->rShop = rShop;

    if( !first_repair )
      first_repair = rShop;
    else
      last_repair->next = rShop;
    rShop->next = NULL;
    rShop->prev = last_repair;
    last_repair = rShop;
    top_repair++;
  }
  return;
}


/*
 * Load spec proc declarations.
 */
void load_specials( AREA_DATA * tarea, FILE * fp )
{
  for( ;; )
  {
    MOB_INDEX_DATA *pMobIndex;
    char letter;

    switch ( letter = fread_letter( fp ) )
    {
      default:
	bug( "Load_specials: letter '%c' not *MS.", letter );
	exit( 1 );

      case 'S':
	return;

      case '*':
	break;

      case 'M':
	pMobIndex = get_mob_index( fread_number( fp ) );
	if( !pMobIndex->spec_fun )
	{
	  pMobIndex->spec_fun = spec_lookup( fread_word( fp ) );

	  if( pMobIndex->spec_fun == 0 )
	  {
	    bug( "Load_specials: 'M': vnum %ld.", pMobIndex->vnum );
	    exit( 1 );
	  }
	}
	else if( !pMobIndex->spec_2 )
	{
	  pMobIndex->spec_2 = spec_lookup( fread_word( fp ) );

	  if( pMobIndex->spec_2 == 0 )
	  {
	    bug( "Load_specials: 'M': vnum %ld.", pMobIndex->vnum );
	    exit( 1 );
	  }
	}

	break;
    }

    fread_to_eol( fp );
  }
}



/*
 * Translate all room exits from virtual to real.
 * Has to be done after all rooms are read in.
 * Check for bad reverse exits.
 */
void fix_exits( void )
{
  ROOM_INDEX_DATA *pRoomIndex;
  EXIT_DATA *pexit, *pexit_next, *rev_exit;
  int iHash;

  for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
  {
    for( pRoomIndex = room_index_hash[iHash];
	pRoomIndex; pRoomIndex = pRoomIndex->next )
    {
      bool fexit;

      fexit = FALSE;
      for( pexit = pRoomIndex->first_exit; pexit; pexit = pexit_next )
      {
	pexit_next = pexit->next;
	pexit->rvnum = pRoomIndex->vnum;
	if( pexit->vnum <= 0
	    || ( pexit->to_room =
	      get_room_index( pexit->vnum ) ) == NULL )
	{
	  if( fBootDb )
	    boot_log
	      ( "Fix_exits: room %d, exit %s leads to bad vnum (%d)",
		pRoomIndex->vnum, dir_name[pexit->vdir],
		pexit->vnum );

	  bug( "Deleting %s exit in room %ld", dir_name[pexit->vdir],
	      pRoomIndex->vnum );
	  extract_exit( pRoomIndex, pexit );
	}
	else
	  fexit = TRUE;
      }
      if( !fexit )
	SET_BIT( pRoomIndex->room_flags, ROOM_NO_MOB );
    }
  }

  /* Set all the rexit pointers       -Thoric */
  for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
  {
    for( pRoomIndex = room_index_hash[iHash];
	pRoomIndex; pRoomIndex = pRoomIndex->next )
    {
      for( pexit = pRoomIndex->first_exit; pexit; pexit = pexit->next )
      {
	if( pexit->to_room && !pexit->rexit )
	{
	  rev_exit =
	    get_exit_to( pexit->to_room, rev_dir[pexit->vdir],
		pRoomIndex->vnum );
	  if( rev_exit )
	  {
	    pexit->rexit = rev_exit;
	    rev_exit->rexit = pexit;
	  }
	}
      }
    }
  }

  return;
}


/*
 * Get diku-compatable exit by number				-Thoric
 */
EXIT_DATA *get_exit_number( ROOM_INDEX_DATA * room, int xit )
{
  EXIT_DATA *pexit;
  int count;

  count = 0;
  for( pexit = room->first_exit; pexit; pexit = pexit->next )
    if( ++count == xit )
      return pexit;
  return NULL;
}

/*
 * (prelude...) This is going to be fun... NOT!
 * (conclusion) QSort is f*cked!
 */
int exit_comp( EXIT_DATA ** xit1, EXIT_DATA ** xit2 )
{
  int d1, d2;

  d1 = ( *xit1 )->vdir;
  d2 = ( *xit2 )->vdir;

  if( d1 < d2 )
    return -1;
  if( d1 > d2 )
    return 1;
  return 0;
}

void sort_exits( ROOM_INDEX_DATA * room )
{
  EXIT_DATA *pexit;		/* *texit *//* Unused */
  EXIT_DATA *exits[MAX_REXITS];
  int x, nexits;

  nexits = 0;
  for( pexit = room->first_exit; pexit; pexit = pexit->next )
  {
    exits[nexits++] = pexit;
    if( nexits > MAX_REXITS )
    {
      bug( "sort_exits: more than %d exits in room... fatal", nexits );
      return;
    }
  }
  qsort( &exits[0], nexits, sizeof( EXIT_DATA * ),
      ( int ( * )( const void *, const void * ) ) exit_comp );
  for( x = 0; x < nexits; x++ )
  {
    if( x > 0 )
      exits[x]->prev = exits[x - 1];
    else
    {
      exits[x]->prev = NULL;
      room->first_exit = exits[x];
    }
    if( x >= ( nexits - 1 ) )
    {
      exits[x]->next = NULL;
      room->last_exit = exits[x];
    }
    else
      exits[x]->next = exits[x + 1];
  }
}

void randomize_exits( ROOM_INDEX_DATA * room, short maxdir )
{
  EXIT_DATA *pexit;
  int nexits, /* maxd, */ d0, d1, count, door;	/* Maxd unused */
  int vdirs[MAX_REXITS];

  nexits = 0;
  for( pexit = room->first_exit; pexit; pexit = pexit->next )
    vdirs[nexits++] = pexit->vdir;

  for( d0 = 0; d0 < nexits; d0++ )
  {
    if( vdirs[d0] > maxdir )
      continue;
    count = 0;
    while( vdirs[( d1 = number_range( d0, nexits - 1 ) )] > maxdir
	|| ++count > 5 );
    if( vdirs[d1] > maxdir )
      continue;
    door = vdirs[d0];
    vdirs[d0] = vdirs[d1];
    vdirs[d1] = door;
  }
  count = 0;
  for( pexit = room->first_exit; pexit; pexit = pexit->next )
    pexit->vdir = vdirs[count++];

  sort_exits( room );
}


/*
 * Create an instance of a mobile.
 */
CHAR_DATA *create_mobile( MOB_INDEX_DATA * pMobIndex )
{
  CHAR_DATA *mob;

  if( !pMobIndex )
  {
    bug( "Create_mobile: NULL pMobIndex." );
    exit( 1 );
  }

  CREATE( mob, CHAR_DATA, 1 );
  clear_char( mob );
  mob->pIndexData = pMobIndex;

  mob->editor = NULL;
  mob->name = QUICKLINK( pMobIndex->player_name );
  mob->short_descr = QUICKLINK( pMobIndex->short_descr );
  mob->long_descr = QUICKLINK( pMobIndex->long_descr );
  mob->description = QUICKLINK( pMobIndex->description );
  mob->spec_fun = pMobIndex->spec_fun;
  mob->spec_2 = pMobIndex->spec_2;
  mob->mpscriptpos = 0;
  mob->top_level = number_fuzzy( pMobIndex->level );
  mob->act = pMobIndex->act;
  mob->affected_by = pMobIndex->affected_by;
  mob->alignment = pMobIndex->alignment;
  mob->sex = pMobIndex->sex;
  mob->mob_clan = NULL;
  mob->was_sentinel = NULL;
  mob->plr_home = NULL;
  mob->guard_data = NULL;

  if( !pMobIndex->ac )
    mob->armor = pMobIndex->ac;
  else
    mob->armor = 100 - mob->top_level * 2;
  mob->armor = URANGE( -100, mob->armor, 100 );
  if( !pMobIndex->hitnodice )
    mob->max_hit =
      10 + mob->top_level * number_range( 1, 5 ) + number_range( 1,
	  mob->
	  top_level );
  else
    mob->max_hit =
      pMobIndex->hitnodice * number_range( 1,
	  pMobIndex->hitsizedice ) +
      pMobIndex->hitplus;
  mob->max_hit = URANGE( 1, mob->max_hit, 1000 );
  mob->hit = mob->max_hit;
  /* lets put things back the way they used to be! -Thoric */
  mob->gold = pMobIndex->gold;
  mob->position = pMobIndex->position;
  mob->defposition = pMobIndex->defposition;
  mob->barenumdie = pMobIndex->damnodice;
  mob->baresizedie = pMobIndex->damsizedice;
  mob->hitplus = pMobIndex->hitplus;
  mob->damplus = pMobIndex->damplus;

  mob->perm_str = pMobIndex->perm_str;
  mob->perm_dex = pMobIndex->perm_dex;
  mob->perm_wis = pMobIndex->perm_wis;
  mob->perm_int = pMobIndex->perm_int;
  mob->perm_con = pMobIndex->perm_con;
  mob->perm_cha = pMobIndex->perm_cha;
  mob->perm_lck = pMobIndex->perm_lck;
  mob->hitroll = pMobIndex->hitroll;
  mob->damroll = pMobIndex->damroll;
  mob->saving_poison_death = pMobIndex->saving_poison_death;
  mob->saving_wand = pMobIndex->saving_wand;
  mob->saving_para_petri = pMobIndex->saving_para_petri;
  mob->saving_breath = pMobIndex->saving_breath;
  mob->saving_spell_staff = pMobIndex->saving_spell_staff;
  mob->height = pMobIndex->height;
  mob->weight = pMobIndex->weight;
  mob->resistant = pMobIndex->resistant;
  mob->immune = pMobIndex->immune;
  mob->susceptible = pMobIndex->susceptible;
  mob->attacks = pMobIndex->attacks;
  mob->defenses = pMobIndex->defenses;
  mob->numattacks = pMobIndex->numattacks;

  /*
   * Insert in list.
   */
  add_char( mob );
  pMobIndex->count++;
  nummobsloaded++;
  return mob;
}

static void create_object_default_values_food( OBJ_DATA * obj )
{
  /*
   * optional food condition (rotting food)         -Thoric
   * value1 is the max condition of the food
   * value4 is the optional initial condition
   */
  if( obj->value[4] )
    obj->timer = obj->value[4];
  else
    obj->timer = obj->value[1];
}

static void create_object_default_values_device( OBJ_DATA * obj )
{
  obj->value[0] = number_fuzzy( obj->value[0] );
  obj->value[1] = number_fuzzy( obj->value[1] );
  obj->value[2] = obj->value[1];
}

static void create_object_default_values_battery( OBJ_DATA * obj )
{
  if( obj->value[0] <= 0 )
    obj->value[0] = number_fuzzy( 95 );
}

static void create_object_default_values_ammo( OBJ_DATA * obj )
{
  if( obj->value[0] <= 0 )
    obj->value[0] = number_fuzzy( 495 );
}

static void create_object_default_values_weapon( OBJ_DATA * obj )
{
  if( obj->value[1] && obj->value[2] )
  {
    obj->value[2] *= obj->value[1];
  }
  else
  {
    obj->value[1] = number_fuzzy( 6 );
    obj->value[2] = number_fuzzy( 12 );
  }

  if( obj->value[1] > obj->value[2] )
    obj->value[1] = obj->value[2] / 3;

  if( obj->value[0] == 0 )
    obj->value[0] = INIT_WEAPON_CONDITION;

  switch ( obj->value[3] )
  {
    case WEAPON_BLASTER:
    case WEAPON_LIGHTSABER:
    case WEAPON_VIBRO_BLADE:

      if( obj->value[5] <= 0 )
	obj->value[5] = number_fuzzy( 1000 );
  }

  obj->value[4] = obj->value[5];
}

static void create_object_default_values_armor( OBJ_DATA * obj )
{
  if( obj->value[0] == 0 )
    obj->value[0] = obj->value[1];

  obj->timer = obj->value[3];
}

static void create_object_default_values_money( OBJ_DATA * obj )
{
  obj->value[0] = obj->cost;
}

/*
 * Create an instance of an object.
 */
OBJ_DATA *create_object( OBJ_INDEX_DATA * pObjIndex )
{
  OBJ_DATA *obj = 0;

  if( !pObjIndex )
  {
    bug( "Create_object: NULL pObjIndex." );
    exit( 1 );
  }

  CREATE( obj, OBJ_DATA, 1 );

  obj->pIndexData = pObjIndex;
  obj->in_room = NULL;
  obj->wear_loc = -1;
  obj->count = 1;
  cur_obj_serial = UMAX( ( cur_obj_serial + 1 ) & ( BV30 - 1 ), 1 );
  obj->serial = obj->pIndexData->serial = cur_obj_serial;

  obj->name = QUICKLINK( pObjIndex->name );
  obj->short_descr = QUICKLINK( pObjIndex->short_descr );
  obj->description = QUICKLINK( pObjIndex->description );
  obj->action_desc = QUICKLINK( pObjIndex->action_desc );
  obj->item_type = pObjIndex->item_type;
  obj->extra_flags = pObjIndex->extra_flags;
  obj->wear_flags = pObjIndex->wear_flags;
  obj->value[0] = pObjIndex->value[0];
  obj->value[1] = pObjIndex->value[1];
  obj->value[2] = pObjIndex->value[2];
  obj->value[3] = pObjIndex->value[3];
  obj->value[4] = pObjIndex->value[4];
  obj->value[5] = pObjIndex->value[5];
  obj->weight = pObjIndex->weight;
  obj->cost = pObjIndex->cost;

  /*
   * Mess with object properties.
   */
  switch ( obj->item_type )
  {
    default:
      bug( "Read_object: vnum %ld bad type.", pObjIndex->vnum );
      bug( "------------------------>     ", obj->item_type );
      break;

    case ITEM_NONE:
      break;

    case ITEM_LENS:
    case ITEM_RESOURCE:
    case ITEM_CRYSTAL:
    case ITEM_PLASTIC:
    case ITEM_METAL:
    case ITEM_SUPERCONDUCTOR:
    case ITEM_COMLINK:
    case ITEM_MEDPAC:
    case ITEM_FABRIC:
    case ITEM_RARE_METAL:
    case ITEM_MAGNET:
    case ITEM_THREAD:
    case ITEM_OVEN:
    case ITEM_MIRROR:
    case ITEM_CIRCUIT:
    case ITEM_TOOLKIT:
    case ITEM_LIGHT:
    case ITEM_FURNITURE:
    case ITEM_TRASH:
    case ITEM_CONTAINER:
    case ITEM_DRINK_CON:
      break;

    case ITEM_FOOD:
      create_object_default_values_food( obj );
      break;

    case ITEM_DROID_CORPSE:
    case ITEM_CORPSE_NPC:
    case ITEM_CORPSE_PC:
    case ITEM_FOUNTAIN:
    case ITEM_SCRAPS:
    case ITEM_PAPER:
    case ITEM_PEN:
    case ITEM_LOCKPICK:
    case ITEM_SHOVEL:
      break;

    case ITEM_DEVICE:
      create_object_default_values_device( obj );
      break;

    case ITEM_BATTERY:
      create_object_default_values_battery( obj );
      break;


    case ITEM_AMMO:
      create_object_default_values_ammo( obj );
      break;

    case ITEM_WEAPON:
      create_object_default_values_weapon( obj );
      break;

    case ITEM_ARMOR:
      create_object_default_values_armor( obj );
      break;

    case ITEM_MONEY:
      create_object_default_values_money( obj );
      break;
  }

  LINK( obj, first_object, last_object, next, prev );
  ++pObjIndex->count;
  ++numobjsloaded;
  ++physicalobjects;

  return obj;
}

/*
 * Clear a new character.
 */
void clear_char( CHAR_DATA * ch )
{
  ch->editor = NULL;
  ch->hunting = NULL;
  ch->fearing = NULL;
  ch->hating = NULL;
  ch->name = NULL;
  ch->short_descr = NULL;
  ch->long_descr = NULL;
  ch->description = NULL;
  ch->next = NULL;
  ch->prev = NULL;
  ch->first_carrying = NULL;
  ch->last_carrying = NULL;
  ch->next_in_room = NULL;
  ch->prev_in_room = NULL;
  ch->fighting = NULL;
  ch->switched = NULL;
  ch->first_affect = NULL;
  ch->last_affect = NULL;
  ch->last_cmd = NULL;
  ch->dest_buf = NULL;
  ch->dest_buf_2 = NULL;
  ch->spare_ptr = NULL;
  ch->mount = NULL;
  ch->affected_by = 0;
  ch->logon = current_time;
  ch->armor = 100;
  ch->position = POS_STANDING;
  ch->hit = 100;
  ch->max_hit = 100;
  ch->mana = 1000;
  ch->max_mana = 0;
  ch->move = 500;
  ch->max_move = 500;
  ch->height = 72;
  ch->weight = 180;
  ch->barenumdie = 1;
  ch->baresizedie = 4;
  ch->substate = 0;
  ch->tempnum = 0;
  ch->perm_str = 10;
  ch->perm_dex = 10;
  ch->perm_int = 10;
  ch->perm_wis = 10;
  ch->perm_cha = 10;
  ch->perm_con = 10;
  ch->perm_lck = 10;
  ch->mod_str = 0;
  ch->mod_dex = 0;
  ch->mod_int = 0;
  ch->mod_wis = 0;
  ch->mod_cha = 0;
  ch->mod_con = 0;
  ch->mod_lck = 0;
}

/*
 * Free a character.
 */
void free_char( CHAR_DATA * ch )
{
  AFFECT_DATA *paf = NULL;
  TIMER *timer = NULL;
  MPROG_ACT_LIST *mpact = NULL, *mpact_next = NULL;

  if( !ch )
  {
    bug( "Free_char: null ch!" );
    return;
  }

  if( ch->desc )
    bug( "Free_char: char still has descriptor." );

  character_extract_carried_objects( ch );

  while( ( paf = ch->last_affect ) != NULL )
    affect_remove( ch, paf );

  while( ( timer = ch->first_timer ) != NULL )
    extract_timer( ch, timer );

  STRFREE( ch->name );
  STRFREE( ch->short_descr );
  STRFREE( ch->long_descr );
  STRFREE( ch->description );

  if( ch->editor )
    stop_editing( ch );

  stop_hunting( ch );
  stop_hating( ch );
  stop_fearing( ch );
  free_fight( ch );

  if( ch->pnote )
    free_note( ch->pnote );

  if( ch->pcdata )
  {
    DISPOSE( ch->pcdata->clan_permissions );
    DISPOSE( ch->pcdata->pwd );	/* no hash */
    DISPOSE( ch->pcdata->email );	/* no hash */
    DISPOSE( ch->pcdata->bamfin );	/* no hash */
    DISPOSE( ch->pcdata->bamfout );	/* no hash */
    DISPOSE( ch->pcdata->rank );
    STRFREE( ch->pcdata->title );
    STRFREE( ch->pcdata->bio );
    DISPOSE( ch->pcdata->bestowments );	/* no hash */
    DISPOSE( ch->pcdata->homepage );	/* no hash */
    STRFREE( ch->pcdata->authed_by );
    STRFREE( ch->pcdata->prompt );

    if( ch->pcdata->subprompt )
      STRFREE( ch->pcdata->subprompt );

#ifdef SWR2_USE_IMC
    imc_freechardata( ch );
#endif

    DISPOSE( ch->pcdata );
  }

  for( mpact = ch->mpact; mpact; mpact = mpact_next )
  {
    mpact_next = mpact->next;
    DISPOSE( mpact->buf );
    DISPOSE( mpact );
  }

  DISPOSE( ch );
}



/*
 * Get an extra description from a list.
 */
char *get_extra_descr( const char *name, EXTRA_DESCR_DATA * ed )
{
  for( ; ed; ed = ed->next )
    if( is_name( name, ed->keyword ) )
      return ed->description;

  return NULL;
}



/*
 * Translates mob virtual number to its mob index struct.
 * Hash table lookup.
 */
MOB_INDEX_DATA *get_mob_index( long vnum )
{
  MOB_INDEX_DATA *pMobIndex;

  if( vnum < 0 )
    vnum = 0;

  for( pMobIndex = mob_index_hash[vnum % MAX_KEY_HASH];
      pMobIndex; pMobIndex = pMobIndex->next )
    if( pMobIndex->vnum == vnum )
      return pMobIndex;

  if( fBootDb )
    bug( "Get_mob_index: bad vnum %ld.", vnum );

  return NULL;
}

/*
 * Translates obj virtual number to its obj index struct.
 * Hash table lookup.
 */
OBJ_INDEX_DATA *get_obj_index( long vnum )
{
  OBJ_INDEX_DATA *pObjIndex;

  if( vnum < 0 )
    vnum = 0;

  for( pObjIndex = obj_index_hash[vnum % MAX_KEY_HASH];
      pObjIndex; pObjIndex = pObjIndex->next )
    if( pObjIndex->vnum == vnum )
      return pObjIndex;

  if( fBootDb )
    bug( "Get_obj_index: bad vnum %ld.", vnum );

  return NULL;
}

/*
 * Translates room virtual number to its room index struct.
 * Hash table lookup.
 */
ROOM_INDEX_DATA *get_room_index( long vnum )
{
  ROOM_INDEX_DATA *pRoomIndex;

  if( vnum < 0 )
    vnum = 0;

  for( pRoomIndex = room_index_hash[vnum % MAX_KEY_HASH];
      pRoomIndex; pRoomIndex = pRoomIndex->next )
    if( pRoomIndex->vnum == vnum )
      return pRoomIndex;

  if( fBootDb )
    bug( "Get_room_index: bad vnum %ld.", vnum );

  return NULL;
}

void do_memory( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  int hash;

  argument = one_argument( argument, arg );
  ch_printf( ch, "Exe     %s\r\n", sysdata.exe_filename );
  ch_printf( ch, "Affects %5d    Areas   %5d\r\n", top_affect, top_area );
  ch_printf( ch, "ExtDes  %5d    Exits   %5d\r\n", top_ed, top_exit );
  ch_printf( ch, "Helps   %5d\r\n", top_help );
  ch_printf( ch, "IdxMobs %5d    Mobs    %5d\r\n", top_mob_index,
      nummobsloaded );
  ch_printf( ch, "IdxObjs %5d    Objs    %5d (%d)\r\n", top_obj_index,
      numobjsloaded, physicalobjects );
  ch_printf( ch, "Rooms   %5d    VRooms  %5d\r\n", top_room, top_vroom );
  ch_printf( ch, "Shops   %5d    RepShps %5d\r\n", top_shop, top_repair );
  ch_printf( ch, "CurOq's %5d    CurCq's %5d\r\n", cur_qobjs, cur_qchars );
  ch_printf( ch, "Players %5d    Maxplrs %5d\r\n", num_descriptors,
      sysdata.maxplayers );
  ch_printf( ch, "MaxEver %5d    Topsn   %5d (%d)\r\n", sysdata.alltimemax,
      top_sn, MAX_SKILL );
  ch_printf( ch, "MaxEver time recorded at:   %s\r\n", sysdata.time_of_max );
  if( !str_cmp( arg, "check" ) )
  {
#ifdef HASHSTR
    send_to_char( check_hash( argument ), ch );
#else
    send_to_char( "Hash strings not enabled.\r\n", ch );
#endif
    return;
  }
  if( !str_cmp( arg, "showhigh" ) )
  {
#ifdef HASHSTR
    show_high_hash( atoi( argument ) );
#else
    send_to_char( "Hash strings not enabled.\r\n", ch );
#endif
    return;
  }
  if( argument[0] != '\0' )
    hash = atoi( argument );
  else
    hash = -1;
  if( !str_cmp( arg, "hash" ) )
  {
#ifdef HASHSTR
    ch_printf( ch, "Hash statistics:\r\n%s", hash_stats() );
    if( hash != -1 )
      hash_dump( hash );
#else
    send_to_char( "Hash strings not enabled.\r\n", ch );
#endif
  }
  return;
}

/*
 * Append a string to a file.
 */
void append_file( CHAR_DATA * ch, const char *file, const char *str )
{
  FILE *fp;

  if( IS_NPC( ch ) || str[0] == '\0' )
    return;

  if( ( fp = fopen( file, "a" ) ) == NULL )
  {
    send_to_char( "Could not open the file!\r\n", ch );
  }
  else
  {
    fprintf( fp, "[%5ld] %s: %s\n",
	ch->in_room ? ch->in_room->vnum : 0, ch->name, str );
    fclose( fp );
  }
}

/*
 * Reports a bug.
 */
void bug( const char *str, ... )
{
  char buf[MAX_STRING_LENGTH];
  FILE *fp;
  struct stat fst;

  if( fpArea != NULL )
  {
    int iLine;
    int iChar;

    if( fpArea == out_stream )
    {
      iLine = 0;
    }
    else
    {
      iChar = ftell( fpArea );
      fseek( fpArea, 0, 0 );

      for( iLine = 0; ftell( fpArea ) < iChar; iLine++ )
      {
	int letter;

	while( ( letter = fgetc( fpArea ) )
	    && letter != EOF && letter != '\n' )
	  ;
      }
      fseek( fpArea, iChar, 0 );
    }

    sprintf( buf, "[*****] FILE: %s LINE: %d", strArea, iLine );
    log_string( buf );

    if( stat( SHUTDOWN_FILE, &fst ) != -1 )	/* file exists */
    {
      if( ( fp = fopen( SHUTDOWN_FILE, "a" ) ) != NULL )
      {
	fprintf( fp, "[*****] %s\n", buf );
	fclose( fp );
      }
    }
  }

  strcpy( buf, "[*****] BUG: " );

  {
    va_list param;

    va_start( param, str );
    vsprintf( buf + strlen( buf ), str, param );
    va_end( param );
  }

  log_string( buf );

  if( ( fp = fopen( BUG_FILE, "a" ) ) != NULL )
  {
    fprintf( fp, "%s\n", buf );
    fclose( fp );
  }
}

/*
 * Add a string to the boot-up log				-Thoric
 */
void boot_log( const char *str, ... )
{
  char buf[MAX_STRING_LENGTH];
  FILE *fp;
  va_list param;

  strcpy( buf, "[*****] BOOT: " );
  va_start( param, str );
  vsprintf( buf + strlen( buf ), str, param );
  va_end( param );
  log_string( buf );

  if( ( fp = fopen( BOOTLOG_FILE, "a" ) ) != NULL )
  {
    fprintf( fp, "%s\n", buf );
    fclose( fp );
  }
}

/*
 * Dump a text file to a player, a line at a time		-Thoric
 */
void show_file( const CHAR_DATA * ch, const char *filename )
{
  FILE *fp = NULL;
  signed char buf[MAX_STRING_LENGTH];
  int c = 0;
  int num = 0;

  if( ( fp = fopen( filename, "r" ) ) != NULL )
  {
    while( !feof( fp ) )
    {
      while( ( buf[num] = fgetc( fp ) ) != EOF
	  && buf[num] != '\n'
	  && buf[num] != '\r' && num < ( MAX_STRING_LENGTH - 2 ) )
	num++;
      c = fgetc( fp );
      if( ( c != '\n' && c != '\r' ) || c == buf[num] )
	ungetc( c, fp );
      buf[num++] = '\n';
      buf[num++] = '\r';
      buf[num] = '\0';
      send_to_pager( ( const char * ) buf, ch );
      num = 0;
    }

    fclose( fp );
  }
}

/*
 * Show the boot log file					-Thoric
 */
void do_dmesg( CHAR_DATA * ch, char *argument )
{
  set_pager_color( AT_LOG, ch );
  show_file( ch, BOOTLOG_FILE );
}

/*
 * Writes a string to the log, extended version			-Thoric
 */
void log_string_plus( const char *str, short log_type )
{
  int offset = 0;
  char *strtime = ctime( &current_time );

  strtime[strlen( strtime ) - 1] = '\0';

  fprintf( out_stream, "%s :: %s\n", strtime, str );

  if( strncmp( str, "Log ", 4 ) == 0 )
    offset = 4;
  else
    offset = 0;

  switch ( log_type )
  {
    default:
      to_channel( str + offset, CHANNEL_LOG, "Log", 2 );
      break;
    case LOG_BUILD:
      to_channel( str + offset, CHANNEL_BUILD, "Build", 2 );
      break;
    case LOG_COMM:
      to_channel( str + offset, CHANNEL_COMM, "Comm", 2 );
      break;
    case LOG_ALL:
      break;
  }
  return;
}

/* From Erwin */
void log_printf( const char *fmt, ... )
{
  char buf[MAX_STRING_LENGTH * 2];
  va_list args;
  va_start( args, fmt );
  vsprintf( buf, fmt, args );
  va_end( args );

  log_string( buf );
}

/* mud prog functions */

/* This routine reads in scripts of MUDprograms from a file */

int mprog_name_to_type( const char *name )
{
  if( !str_cmp( name, "in_file_prog" ) )
    return IN_FILE_PROG;
  if( !str_cmp( name, "act_prog" ) )
    return ACT_PROG;
  if( !str_cmp( name, "speech_prog" ) )
    return SPEECH_PROG;
  if( !str_cmp( name, "rand_prog" ) )
    return RAND_PROG;
  if( !str_cmp( name, "fight_prog" ) )
    return FIGHT_PROG;
  if( !str_cmp( name, "hitprcnt_prog" ) )
    return HITPRCNT_PROG;
  if( !str_cmp( name, "death_prog" ) )
    return DEATH_PROG;
  if( !str_cmp( name, "entry_prog" ) )
    return ENTRY_PROG;
  if( !str_cmp( name, "greet_prog" ) )
    return GREET_PROG;
  if( !str_cmp( name, "all_greet_prog" ) )
    return ALL_GREET_PROG;
  if( !str_cmp( name, "give_prog" ) )
    return GIVE_PROG;
  if( !str_cmp( name, "bribe_prog" ) )
    return BRIBE_PROG;
  if( !str_cmp( name, "time_prog" ) )
    return TIME_PROG;
  if( !str_cmp( name, "hour_prog" ) )
    return HOUR_PROG;
  if( !str_cmp( name, "wear_prog" ) )
    return WEAR_PROG;
  if( !str_cmp( name, "remove_prog" ) )
    return REMOVE_PROG;
  if( !str_cmp( name, "sac_prog" ) )
    return SAC_PROG;
  if( !str_cmp( name, "look_prog" ) )
    return LOOK_PROG;
  if( !str_cmp( name, "exa_prog" ) )
    return EXA_PROG;
  if( !str_cmp( name, "zap_prog" ) )
    return ZAP_PROG;
  if( !str_cmp( name, "get_prog" ) )
    return GET_PROG;
  if( !str_cmp( name, "drop_prog" ) )
    return DROP_PROG;
  if( !str_cmp( name, "damage_prog" ) )
    return DAMAGE_PROG;
  if( !str_cmp( name, "repair_prog" ) )
    return REPAIR_PROG;
  if( !str_cmp( name, "greet_prog" ) )
    return GREET_PROG;
  if( !str_cmp( name, "randiw_prog" ) )
    return RANDIW_PROG;
  if( !str_cmp( name, "speechiw_prog" ) )
    return SPEECHIW_PROG;
  if( !str_cmp( name, "pull_prog" ) )
    return PULL_PROG;
  if( !str_cmp( name, "push_prog" ) )
    return PUSH_PROG;
  if( !str_cmp( name, "sleep_prog" ) )
    return SLEEP_PROG;
  if( !str_cmp( name, "rest_prog" ) )
    return REST_PROG;
  if( !str_cmp( name, "rfight_prog" ) )
    return FIGHT_PROG;
  if( !str_cmp( name, "enter_prog" ) )
    return ENTRY_PROG;
  if( !str_cmp( name, "leave_prog" ) )
    return LEAVE_PROG;
  if( !str_cmp( name, "rdeath_prog" ) )
    return DEATH_PROG;
  if( !str_cmp( name, "script_prog" ) )
    return SCRIPT_PROG;
  if( !str_cmp( name, "use_prog" ) )
    return USE_PROG;
  return ( ERROR_PROG );
}

void mobprog_file_read( MOB_INDEX_DATA * mob, char *f )
{
  MPROG_DATA *mprg = NULL;
  char MUDProgfile[256];
  FILE *progfile;
  char letter;

  sprintf( MUDProgfile, "%s%s", PROG_DIR, f );

  if( !( progfile = fopen( MUDProgfile, "r" ) ) )
  {
    bug( "%s: couldn't open mudprog file", __FUNCTION__ );
    return;
  }

  for( ;; )
  {
    letter = fread_letter( progfile );

    if( letter == '|' )
      break;

    if( letter != '>' )
    {
      bug( "%s: MUDPROG char", __FUNCTION__ );
      break;
    }

    CREATE( mprg, MPROG_DATA, 1 );
    mprg->type = mprog_name_to_type( fread_word( progfile ) );
    switch ( mprg->type )
    {
      case ERROR_PROG:
	bug( "%s: mudprog file type error", __FUNCTION__ );
	DISPOSE( mprg );
	continue;

      case IN_FILE_PROG:
	bug( "%s: Nested file programs are not allowed.", __FUNCTION__ );
	DISPOSE( mprg );
	continue;

      default:
	mprg->arglist = fread_string( progfile );
	mprg->comlist = fread_string( progfile );
	mprg->fileprog = TRUE;
	SET_BIT( mob->progtypes, mprg->type );
	mprg->next = mob->mudprogs;
	mob->mudprogs = mprg;
	break;
    }
  }
  fclose( progfile );
  progfile = NULL;
  return;
}

/* This procedure is responsible for reading any in_file MUDprograms.
*/
void mprog_read_programs( FILE * fp, MOB_INDEX_DATA * mob )
{
  MPROG_DATA *mprg;
  char letter;
  char *word;

  for( ;; )
  {
    letter = fread_letter( fp );

    if( letter == '|' )
      return;

    if( letter != '>' )
    {
      bug( "%s: vnum %d MUDPROG char", __FUNCTION__, mob->vnum );
      exit( 1 );
    }
    CREATE( mprg, MPROG_DATA, 1 );
    mprg->next = mob->mudprogs;
    mob->mudprogs = mprg;

    word = fread_word( fp );
    mprg->type = mprog_name_to_type( word );

    switch ( mprg->type )
    {
      case ERROR_PROG:
	bug( "%s: vnum %d MUDPROG type.", __FUNCTION__, mob->vnum );
	exit( 1 );

      case IN_FILE_PROG:
	mprg->arglist = fread_string( fp );
	mprg->fileprog = FALSE;
	mobprog_file_read( mob, mprg->arglist );
	break;

      default:
	SET_BIT( mob->progtypes, mprg->type );
	mprg->fileprog = FALSE;
	mprg->arglist = fread_string( fp );
	mprg->comlist = fread_string( fp );
	break;
    }
  }
  return;
}

/*************************************************************/
/* obj prog functions */
/* This routine transfers between alpha and numeric forms of the
 *  mob_prog bitvector types. This allows the use of the words in the
 *  mob/script files.
 */

/* This routine reads in scripts of OBJprograms from a file */
void objprog_file_read( OBJ_INDEX_DATA * obj, char *f )
{
  MPROG_DATA *mprg = NULL;
  char MUDProgfile[256];
  FILE *progfile;
  char letter;

  sprintf( MUDProgfile, "%s%s", PROG_DIR, f );

  if( !( progfile = fopen( MUDProgfile, "r" ) ) )
  {
    bug( "%s: couldn't open mudprog file", __FUNCTION__ );
    return;
  }

  for( ;; )
  {
    letter = fread_letter( progfile );

    if( letter == '|' )
      break;

    if( letter != '>' )
    {
      bug( "%s: MUDPROG char", __FUNCTION__ );
      break;
    }

    CREATE( mprg, MPROG_DATA, 1 );
    mprg->type = mprog_name_to_type( fread_word( progfile ) );
    switch ( mprg->type )
    {
      case ERROR_PROG:
	bug( "%s: mudprog file type error", __FUNCTION__ );
	DISPOSE( mprg );
	continue;

      case IN_FILE_PROG:
	bug( "%s: Nested file programs are not allowed.", __FUNCTION__ );
	DISPOSE( mprg );
	continue;

      default:
	mprg->arglist = fread_string( progfile );
	mprg->comlist = fread_string( progfile );
	mprg->fileprog = TRUE;
	SET_BIT( obj->progtypes, mprg->type );
	mprg->next = obj->mudprogs;
	obj->mudprogs = mprg;
	break;
    }
  }
  fclose( progfile );
  progfile = NULL;
  return;
}

/* This procedure is responsible for reading any in_file OBJprograms.
*/
void oprog_read_programs( FILE * fp, OBJ_INDEX_DATA * obj )
{
  MPROG_DATA *mprg;
  char letter;
  char *word;

  for( ;; )
  {
    letter = fread_letter( fp );

    if( letter == '|' )
      return;

    if( letter != '>' )
    {
      bug( "%s: vnum %d MUDPROG char", __FUNCTION__, obj->vnum );
      exit( 1 );
    }
    CREATE( mprg, MPROG_DATA, 1 );
    mprg->next = obj->mudprogs;
    obj->mudprogs = mprg;

    word = fread_word( fp );
    mprg->type = mprog_name_to_type( word );

    switch ( mprg->type )
    {
      case ERROR_PROG:
	bug( "%s: vnum %d MUDPROG type.", __FUNCTION__, obj->vnum );
	exit( 1 );

      case IN_FILE_PROG:
	mprg->arglist = fread_string( fp );
	mprg->fileprog = FALSE;
	objprog_file_read( obj, mprg->arglist );
	break;

      default:
	SET_BIT( obj->progtypes, mprg->type );
	mprg->fileprog = FALSE;
	mprg->arglist = fread_string( fp );
	mprg->comlist = fread_string( fp );
	break;
    }
  }
  return;
}


/*************************************************************/
/* room prog functions */
/* This routine transfers between alpha and numeric forms of the
 *  mob_prog bitvector types. This allows the use of the words in the
 *  mob/script files.
 */

/* This routine reads in scripts of OBJprograms from a file */
void roomprog_file_read( ROOM_INDEX_DATA * room, char *f )
{
  MPROG_DATA *mprg = NULL;
  char MUDProgfile[256];
  FILE *progfile;
  char letter;

  sprintf( MUDProgfile, "%s%s", PROG_DIR, f );

  if( !( progfile = fopen( MUDProgfile, "r" ) ) )
  {
    bug( "%s: couldn't open mudprog file", __FUNCTION__ );
    return;
  }

  for( ;; )
  {
    letter = fread_letter( progfile );

    if( letter == '|' )
      break;

    if( letter != '>' )
    {
      bug( "%s: MUDPROG char", __FUNCTION__ );
      break;
    }

    CREATE( mprg, MPROG_DATA, 1 );
    mprg->type = mprog_name_to_type( fread_word( progfile ) );
    switch ( mprg->type )
    {
      case ERROR_PROG:
	bug( "%s: mudprog file type error", __FUNCTION__ );
	DISPOSE( mprg );
	continue;

      case IN_FILE_PROG:
	bug( "%s: Nested file programs are not allowed.", __FUNCTION__ );
	DISPOSE( mprg );
	continue;

      default:
	mprg->arglist = fread_string( progfile );
	mprg->comlist = fread_string( progfile );
	mprg->fileprog = TRUE;
	SET_BIT( room->progtypes, mprg->type );
	mprg->next = room->mudprogs;
	room->mudprogs = mprg;
	break;
    }
  }
  fclose( progfile );
  progfile = NULL;
  return;
}


/* This procedure is responsible for reading any in_file ROOMprograms.
*/
void rprog_read_programs( FILE * fp, ROOM_INDEX_DATA * room )
{
  MPROG_DATA *mprg;
  char letter;
  char *word;

  for( ;; )
  {
    letter = fread_letter( fp );

    if( letter == '|' )
      return;

    if( letter != '>' )
    {
      bug( "%s: vnum %d MUDPROG char", __FUNCTION__, room->vnum );
      exit( 1 );
    }
    CREATE( mprg, MPROG_DATA, 1 );
    mprg->next = room->mudprogs;
    room->mudprogs = mprg;

    word = fread_word( fp );
    mprg->type = mprog_name_to_type( word );

    switch ( mprg->type )
    {
      case ERROR_PROG:
	bug( "%s: vnum %d MUDPROG type.", __FUNCTION__, room->vnum );
	exit( 1 );

      case IN_FILE_PROG:
	mprg->arglist = fread_string( fp );
	mprg->fileprog = FALSE;
	roomprog_file_read( room, mprg->arglist );
	break;

      default:
	SET_BIT( room->progtypes, mprg->type );
	mprg->fileprog = FALSE;
	mprg->arglist = fread_string( fp );
	mprg->comlist = fread_string( fp );
	break;
    }
  }
  return;
}

/*************************************************************/
/* Function to delete a room index.  Called from do_rdelete in build.c
   Narn, May/96
   */
bool delete_room( ROOM_INDEX_DATA * room )
{
  ROOM_INDEX_DATA *tmp = NULL, *prev = NULL;
  int iHash = room->vnum % MAX_KEY_HASH;

  /* Take the room index out of the hash list. */
  for( tmp = room_index_hash[iHash]; tmp && tmp != room; tmp = tmp->next )
  {
    prev = tmp;
  }

  if( !tmp )
  {
    bug( "Delete_room: room not found" );
    return FALSE;
  }

  if( prev )
  {
    prev->next = room->next;
  }
  else
  {
    room_index_hash[iHash] = room->next;
  }

  /* Free up the ram for all strings attached to the room. */
  STRFREE( room->name );
  STRFREE( room->description );

  /* Free up the ram held by the room index itself. */
  DISPOSE( room );

  top_room--;
  return TRUE;
}

/* See comment on delete_room. */
bool delete_obj( OBJ_INDEX_DATA * obj )
{
  return TRUE;
}

/* See comment on delete_room. */
bool delete_mob( MOB_INDEX_DATA * mob )
{
  return TRUE;
}

/*
 * Creat a new room (for online building)			-Thoric
 */
ROOM_INDEX_DATA *make_room( long vnum )
{
  ROOM_INDEX_DATA *pRoomIndex;
  int iHash;

  CREATE( pRoomIndex, ROOM_INDEX_DATA, 1 );
  pRoomIndex->first_person = NULL;
  pRoomIndex->last_person = NULL;
  pRoomIndex->first_content = NULL;
  pRoomIndex->last_content = NULL;
  pRoomIndex->first_extradesc = NULL;
  pRoomIndex->last_extradesc = NULL;
  pRoomIndex->first_ship = NULL;
  pRoomIndex->last_ship = NULL;
  pRoomIndex->next_in_area = NULL;
  pRoomIndex->prev_in_area = NULL;
  pRoomIndex->next_in_ship = NULL;
  pRoomIndex->prev_in_ship = NULL;
  pRoomIndex->area = NULL;
  pRoomIndex->vnum = vnum;
  pRoomIndex->name = STRALLOC( "Floating in a void" );
  pRoomIndex->description = STRALLOC( "" );
  pRoomIndex->room_flags = 0;
  pRoomIndex->sector_type = 1;
  pRoomIndex->light = 0;
  pRoomIndex->first_exit = NULL;
  pRoomIndex->last_exit = NULL;

  iHash = vnum % MAX_KEY_HASH;
  pRoomIndex->next = room_index_hash[iHash];
  room_index_hash[iHash] = pRoomIndex;
  top_room++;

  return pRoomIndex;
}

ROOM_INDEX_DATA *make_ship_room( SHIP_DATA * ship )
{
  ROOM_INDEX_DATA *pRoomIndex;

  CREATE( pRoomIndex, ROOM_INDEX_DATA, 1 );
  pRoomIndex->first_person = NULL;
  pRoomIndex->last_person = NULL;
  pRoomIndex->first_content = NULL;
  pRoomIndex->last_content = NULL;
  pRoomIndex->first_extradesc = NULL;
  pRoomIndex->last_extradesc = NULL;
  pRoomIndex->first_ship = NULL;
  pRoomIndex->last_ship = NULL;
  pRoomIndex->next_in_area = NULL;
  pRoomIndex->prev_in_area = NULL;
  pRoomIndex->next_in_ship = NULL;
  pRoomIndex->prev_in_ship = NULL;
  pRoomIndex->area = NULL;
  pRoomIndex->vnum = -1;
  pRoomIndex->name = STRALLOC( ship->name );
  pRoomIndex->description = STRALLOC( "" );
  pRoomIndex->room_flags = 0;
  pRoomIndex->sector_type = 0;
  pRoomIndex->light = 0;
  pRoomIndex->first_exit = NULL;
  pRoomIndex->last_exit = NULL;

  SET_BIT( pRoomIndex->room_flags, ROOM_SPACECRAFT );
  SET_BIT( pRoomIndex->room_flags, ROOM_INDOORS );

  LINK( pRoomIndex, ship->first_room, ship->last_room, next_in_ship,
      prev_in_ship );

  top_room++;

  return pRoomIndex;
}

/*
 * Create a new INDEX object (for online building)		-Thoric
 * Option to clone an existing index object.
 */
OBJ_INDEX_DATA *make_object( long vnum, long cvnum, char *name )
{
  OBJ_INDEX_DATA *pObjIndex, *cObjIndex;
  char buf[MAX_STRING_LENGTH];
  int iHash;

  if( cvnum > 0 )
    cObjIndex = get_obj_index( cvnum );
  else
    cObjIndex = NULL;

  CREATE( pObjIndex, OBJ_INDEX_DATA, 1 );
  pObjIndex->vnum = vnum;
  pObjIndex->name = STRALLOC( name );
  pObjIndex->first_affect = NULL;
  pObjIndex->last_affect = NULL;
  pObjIndex->first_extradesc = NULL;
  pObjIndex->last_extradesc = NULL;

  if( !cObjIndex )
  {
    sprintf( buf, "A %s", name );
    pObjIndex->short_descr = STRALLOC( buf );
    sprintf( buf, "A %s is here.", name );
    pObjIndex->description = STRALLOC( buf );
    pObjIndex->action_desc = STRALLOC( "" );
    pObjIndex->short_descr[0] = LOWER( pObjIndex->short_descr[0] );
    pObjIndex->description[0] = UPPER( pObjIndex->description[0] );
    pObjIndex->item_type = ITEM_TRASH;
    pObjIndex->extra_flags = ITEM_PROTOTYPE;
    pObjIndex->wear_flags = 0;
    pObjIndex->value[0] = 0;
    pObjIndex->value[1] = 0;
    pObjIndex->value[2] = 0;
    pObjIndex->value[3] = 0;
    pObjIndex->weight = 1;
    pObjIndex->cost = 0;
  }
  else
  {
    EXTRA_DESCR_DATA *ed, *ced;
    AFFECT_DATA *paf, *cpaf;

    pObjIndex->short_descr = QUICKLINK( cObjIndex->short_descr );
    pObjIndex->description = QUICKLINK( cObjIndex->description );
    pObjIndex->action_desc = QUICKLINK( cObjIndex->action_desc );
    pObjIndex->item_type = cObjIndex->item_type;
    pObjIndex->extra_flags = cObjIndex->extra_flags | ITEM_PROTOTYPE;
    pObjIndex->wear_flags = cObjIndex->wear_flags;
    pObjIndex->value[0] = cObjIndex->value[0];
    pObjIndex->value[1] = cObjIndex->value[1];
    pObjIndex->value[2] = cObjIndex->value[2];
    pObjIndex->value[3] = cObjIndex->value[3];
    pObjIndex->weight = cObjIndex->weight;
    pObjIndex->cost = cObjIndex->cost;

    for( ced = cObjIndex->first_extradesc; ced; ced = ced->next )
    {
      CREATE( ed, EXTRA_DESCR_DATA, 1 );
      ed->keyword = QUICKLINK( ced->keyword );
      ed->description = QUICKLINK( ced->description );
      LINK( ed, pObjIndex->first_extradesc, pObjIndex->last_extradesc,
	  next, prev );
      top_ed++;
    }

    for( cpaf = cObjIndex->first_affect; cpaf; cpaf = cpaf->next )
    {
      CREATE( paf, AFFECT_DATA, 1 );
      paf->type = cpaf->type;
      paf->duration = cpaf->duration;
      paf->location = cpaf->location;
      paf->modifier = cpaf->modifier;
      paf->bitvector = cpaf->bitvector;
      LINK( paf, pObjIndex->first_affect, pObjIndex->last_affect,
	  next, prev );
      top_affect++;
    }
  }

  pObjIndex->count = 0;
  iHash = vnum % MAX_KEY_HASH;
  pObjIndex->next = obj_index_hash[iHash];
  obj_index_hash[iHash] = pObjIndex;
  top_obj_index++;

  return pObjIndex;
}

/*
 * Create a new INDEX mobile (for online building)		-Thoric
 * Option to clone an existing index mobile.
 */
MOB_INDEX_DATA *make_mobile( long vnum, long cvnum, char *name )
{
  MOB_INDEX_DATA *pMobIndex, *cMobIndex;
  char buf[MAX_STRING_LENGTH];
  int iHash;

  if( cvnum > 0 )
    cMobIndex = get_mob_index( cvnum );
  else
    cMobIndex = NULL;

  CREATE( pMobIndex, MOB_INDEX_DATA, 1 );
  pMobIndex->vnum = vnum;
  pMobIndex->count = 0;
  pMobIndex->killed = 0;
  pMobIndex->player_name = STRALLOC( name );

  if( !cMobIndex )
  {
    sprintf( buf, "A newly created %s", name );
    pMobIndex->short_descr = STRALLOC( buf );
    sprintf( buf, "Some god abandoned a newly created %s here.\r\n", name );
    pMobIndex->long_descr = STRALLOC( buf );
    pMobIndex->description = STRALLOC( "" );
    pMobIndex->short_descr[0] = LOWER( pMobIndex->short_descr[0] );
    pMobIndex->long_descr[0] = UPPER( pMobIndex->long_descr[0] );
    pMobIndex->description[0] = UPPER( pMobIndex->description[0] );
    pMobIndex->act = ACT_IS_NPC | ACT_PROTOTYPE;
    pMobIndex->affected_by = 0;
    pMobIndex->pShop = NULL;
    pMobIndex->rShop = NULL;
    pMobIndex->spec_fun = NULL;
    pMobIndex->spec_2 = NULL;
    pMobIndex->mudprogs = NULL;
    pMobIndex->progtypes = 0;
    pMobIndex->alignment = 0;
    pMobIndex->level = 1;
    pMobIndex->ac = 0;
    pMobIndex->hitnodice = 0;
    pMobIndex->hitsizedice = 0;
    pMobIndex->hitplus = 0;
    pMobIndex->damnodice = 0;
    pMobIndex->damsizedice = 0;
    pMobIndex->damplus = 0;
    pMobIndex->gold = 0;
    pMobIndex->position = 8;
    pMobIndex->defposition = 8;
    pMobIndex->sex = 0;
    pMobIndex->perm_str = 10;
    pMobIndex->perm_dex = 10;
    pMobIndex->perm_int = 10;
    pMobIndex->perm_wis = 10;
    pMobIndex->perm_cha = 10;
    pMobIndex->perm_con = 10;
    pMobIndex->perm_lck = 10;
    pMobIndex->resistant = 0;
    pMobIndex->immune = 0;
    pMobIndex->susceptible = 0;
    pMobIndex->numattacks = 1;
    pMobIndex->attacks = 0;
    pMobIndex->defenses = 0;
  }
  else
  {
    pMobIndex->short_descr = QUICKLINK( cMobIndex->short_descr );
    pMobIndex->long_descr = QUICKLINK( cMobIndex->long_descr );
    pMobIndex->description = QUICKLINK( cMobIndex->description );
    pMobIndex->act = cMobIndex->act | ACT_PROTOTYPE;
    pMobIndex->affected_by = cMobIndex->affected_by;
    pMobIndex->pShop = NULL;
    pMobIndex->rShop = NULL;
    pMobIndex->spec_fun = cMobIndex->spec_fun;
    pMobIndex->spec_2 = cMobIndex->spec_2;
    pMobIndex->mudprogs = NULL;
    pMobIndex->progtypes = 0;
    pMobIndex->alignment = cMobIndex->alignment;
    pMobIndex->level = cMobIndex->level;
    pMobIndex->ac = cMobIndex->ac;
    pMobIndex->hitnodice = cMobIndex->hitnodice;
    pMobIndex->hitsizedice = cMobIndex->hitsizedice;
    pMobIndex->hitplus = cMobIndex->hitplus;
    pMobIndex->damnodice = cMobIndex->damnodice;
    pMobIndex->damsizedice = cMobIndex->damsizedice;
    pMobIndex->damplus = cMobIndex->damplus;
    pMobIndex->gold = cMobIndex->gold;
    pMobIndex->position = cMobIndex->position;
    pMobIndex->defposition = cMobIndex->defposition;
    pMobIndex->sex = cMobIndex->sex;
    pMobIndex->perm_str = cMobIndex->perm_str;
    pMobIndex->perm_dex = cMobIndex->perm_dex;
    pMobIndex->perm_int = cMobIndex->perm_int;
    pMobIndex->perm_wis = cMobIndex->perm_wis;
    pMobIndex->perm_cha = cMobIndex->perm_cha;
    pMobIndex->perm_con = cMobIndex->perm_con;
    pMobIndex->perm_lck = cMobIndex->perm_lck;
    pMobIndex->resistant = cMobIndex->resistant;
    pMobIndex->immune = cMobIndex->immune;
    pMobIndex->susceptible = cMobIndex->susceptible;
    pMobIndex->numattacks = cMobIndex->numattacks;
    pMobIndex->attacks = cMobIndex->attacks;
    pMobIndex->defenses = cMobIndex->defenses;
  }

  iHash = vnum % MAX_KEY_HASH;
  pMobIndex->next = mob_index_hash[iHash];
  mob_index_hash[iHash] = pMobIndex;
  top_mob_index++;

  return pMobIndex;
}

/*
 * Creates a simple exit with no fields filled but rvnum and optionally
 * to_room and vnum.						-Thoric
 * Exits are inserted into the linked list based on vdir.
 */
EXIT_DATA *make_exit( ROOM_INDEX_DATA * pRoomIndex, ROOM_INDEX_DATA * to_room,
    short door )
{
  EXIT_DATA *pexit = NULL, *texit = NULL;
  bool broke = FALSE;

  CREATE( pexit, EXIT_DATA, 1 );
  pexit->vdir = door;
  pexit->rvnum = pRoomIndex->vnum;
  pexit->to_room = to_room;
  pexit->distance = 1;

  if( to_room )
    {
      pexit->vnum = to_room->vnum;
      texit = get_exit_to( to_room, rev_dir[door], pRoomIndex->vnum );

      if( texit )		/* assign reverse exit pointers */
	{
	  texit->rexit = pexit;
	  pexit->rexit = texit;
	}
    }

  for( texit = pRoomIndex->first_exit; texit; texit = texit->next )
    {
      if( door < texit->vdir )
	{
	  broke = TRUE;
	  break;
	}
    }

  if( !pRoomIndex->first_exit )
    {
      pRoomIndex->first_exit = pexit;
    }
  else
    {
      /* keep exits in incremental order - insert exit into list */
      if( broke && texit )
	{
	  if( !texit->prev )
	    pRoomIndex->first_exit = pexit;
	  else
	    texit->prev->next = pexit;

	  pexit->prev = texit->prev;
	  pexit->next = texit;
	  texit->prev = pexit;
	  top_exit++;
	  return pexit;
	}

      pRoomIndex->last_exit->next = pexit;
    }

  pexit->next = NULL;
  pexit->prev = pRoomIndex->last_exit;
  pRoomIndex->last_exit = pexit;
  top_exit++;
  return pexit;
}

void make_bexit( ROOM_INDEX_DATA *location, ROOM_INDEX_DATA *troom,
		 int direction )
{
  EXIT_DATA *xit = make_exit( location, troom, direction );
  xit->keyword = STRALLOC( "" );
  xit->description = STRALLOC( "" );
  xit->key = -1;
  xit->exit_info = 0;

  xit = make_exit( troom, location, rev_dir[direction] );
  xit->keyword = STRALLOC( "" );
  xit->description = STRALLOC( "" );
  xit->key = -1;
  xit->exit_info = 0;
}

void fix_area_exits( AREA_DATA * tarea )
{
  ROOM_INDEX_DATA *pRoomIndex = NULL;
  EXIT_DATA *pexit = NULL, *rev_exit = NULL;

  for( pRoomIndex = tarea->first_room; pRoomIndex;
      pRoomIndex = pRoomIndex->next_in_area )
  {
    bool fexit = FALSE;


    for( pexit = pRoomIndex->first_exit; pexit; pexit = pexit->next )
    {
      fexit = TRUE;
      pexit->rvnum = pRoomIndex->vnum;

      if( pexit->vnum <= 0 )
	pexit->to_room = NULL;
      else
	pexit->to_room = get_room_index( pexit->vnum );
    }

    if( !fexit )
      SET_BIT( pRoomIndex->room_flags, ROOM_NO_MOB );
  }


  for( pRoomIndex = tarea->first_room; pRoomIndex;
      pRoomIndex = pRoomIndex->next_in_area )
  {
    for( pexit = pRoomIndex->first_exit; pexit; pexit = pexit->next )
    {
      if( pexit->to_room && !pexit->rexit )
      {
	rev_exit =
	  get_exit_to( pexit->to_room, rev_dir[pexit->vdir],
	      pRoomIndex->vnum );
	if( rev_exit )
	{
	  pexit->rexit = rev_exit;
	  rev_exit->rexit = pexit;
	}
      }
    }
  }
}

void load_area_file( AREA_DATA * tarea, const char *filename )
{
  char realfilename[MAX_STRING_LENGTH];
  sprintf( realfilename, "%s%s", AREA_DIR, filename );

  /*    FILE *fpin;
	what intelligent person stopped using fpArea?????
	if fpArea isn't being used, then no filename or linenumber
	is printed when an error occurs during loading the area..
	(bug uses fpArea)
	--TRI  */

  log_printf( "Reading %s", realfilename );

  if( fBootDb )
    tarea = last_area;

  if( !fBootDb && !tarea )
  {
    bug( "Load_area: null area!" );
    return;
  }

  if( ( fpArea = fopen( realfilename, "r" ) ) == NULL )
  {
    bug( "load_area: error loading file (can't open)" );
    bug( filename );
    return;
  }

  for( ;; )
  {
    const char *word;

    if( fread_letter( fpArea ) != '#' )
    {
      bug( filename );
      bug( "load_area: # not found." );
      exit( 1 );
    }

    word = fread_word( fpArea );

    if( word[0] == '$' )
    {
      break;
    }
    else if( !str_cmp( word, "AREA" ) )
    {
      if( fBootDb )
      {
	load_area( fpArea );
	tarea = last_area;
      }
      else
      {
	DISPOSE( tarea->name );
	tarea->name = fread_string_nohash( fpArea );
      }
    }
    else if( !str_cmp( word, "FLAGS" ) )
      load_flags( tarea, fpArea );
    else if( !str_cmp( word, "MOBILES" ) )
      load_mobiles( tarea, fpArea );
    else if( !str_cmp( word, "OBJECTS" ) )
      load_objects( tarea, fpArea );
    else if( !str_cmp( word, "ROOMS" ) )
      load_rooms( tarea, fpArea );
    else if( !str_cmp( word, "SHOPS" ) )
      load_shops( tarea, fpArea );
    else if( !str_cmp( word, "REPAIRS" ) )
      load_repairs( tarea, fpArea );
    else if( !str_cmp( word, "SPECIALS" ) )
      load_specials( tarea, fpArea );
    else
    {
      bug( filename );
      bug( "load_area: bad section name." );

      if( fBootDb )
      {
	exit( 1 );
      }
      else
      {
	fclose( fpArea );
	return;
      }
    }
  }

  fclose( fpArea );
  /*
     if ( tarea )
     {
     fprintf( out_stream, "%s\n",
     filename);
     }
     else
     fprintf( out_stream, "(%s)\n", filename );
     */
}

/*
 * Shows prototype vnums ranges, and if loaded
 */

void do_vnums( CHAR_DATA * ch, char *argument )
{
  AREA_DATA *pArea = NULL;
  size_t counter = 1;

  for( pArea = first_area; pArea; pArea = pArea->next, ++counter )
  {
    ch_printf( ch, "%2d) %-10s : %5d - %-5d\r\n",
	counter, pArea->filename,
	pArea->first_room->vnum, pArea->last_room->vnum );
  }
}

/*
 * Shows installed areas, sorted.  Mark unloaded areas with an X
 */
void do_zones( CHAR_DATA * ch, char *argument )
{
  AREA_DATA *pArea;

  set_pager_color( AT_PLAIN, ch );

  pager_printf( ch, "Zones\r\n" );
  pager_printf( ch, "----------------------------------------------\r\n" );

  for( pArea = first_area; pArea; pArea = pArea->next )
    pager_printf( ch, "%s\r\n", pArea->filename );

  pager_printf( ch, "----------------------------------------------\r\n" );
  return;

}


/*
 * Save system info to data file
 */
void save_sysdata( void )
{
  FILE *fp;
  char filename[MAX_INPUT_LENGTH];
  log_string( "Saving systemdata..." );

  sprintf( filename, "%ssysdata.dat", SYSTEM_DIR );

  if( ( fp = fopen( filename, "w" ) ) == NULL )
  {
    bug( "save_sysdata: fopen" );
  }
  else
  {
    fprintf( fp, "#SYSTEM\n" );
    fprintf( fp, "Highplayers    %d\n", sysdata.alltimemax );
    fprintf( fp, "Highplayertime %s~\n", sysdata.time_of_max );
    fprintf( fp, "Nameresolving  %d\n", sysdata.NO_NAME_RESOLVING );
    fprintf( fp, "Waitforauth    %d\n", sysdata.WAIT_FOR_AUTH );
    fprintf( fp, "Saveflags      %d\n", sysdata.save_flags );
    fprintf( fp, "Savefreq       %d\n", sysdata.save_frequency );
    fprintf( fp, "Officials      %s~\n", sysdata.officials );
    fprintf( fp, "End\n\n" );
    fprintf( fp, "#END\n" );
  }
  fclose( fp );
}


void fread_sysdata( SYSTEM_DATA * sys, FILE * fp )
{
  const char *word;
  bool fMatch;

  sys->time_of_max = NULL;
  sys->officials = NULL;

  for( ;; )
  {
    word = feof( fp ) ? "End" : fread_word( fp );
    fMatch = FALSE;

    switch ( UPPER( word[0] ) )
    {
      case '*':
	fMatch = TRUE;
	fread_to_eol( fp );
	break;

      case 'E':
	if( !str_cmp( word, "End" ) )
	{
	  if( !sys->time_of_max )
	    sys->time_of_max = str_dup( "(not recorded)" );
	  if( !sys->officials )
	    sys->officials = STRALLOC( "" );
	  return;
	}
	break;

      case 'H':
	KEY( "Highplayers", sys->alltimemax, fread_number( fp ) );
	KEY( "Highplayertime", sys->time_of_max,
	    fread_string_nohash( fp ) );
	break;

      case 'N':
	KEY( "Nameresolving", sys->NO_NAME_RESOLVING, fread_number( fp ) );
	break;

      case 'O':
	KEY( "Officials", sys->officials, fread_string( fp ) );
	break;

      case 'S':
	KEY( "Saveflags", sys->save_flags, fread_number( fp ) );
	KEY( "Savefreq", sys->save_frequency, fread_number( fp ) );
	break;


      case 'W':
	KEY( "Waitforauth", sys->WAIT_FOR_AUTH, fread_number( fp ) );
	break;
    }


    if( !fMatch )
    {
      bug( "Fread_sysdata: no match: %s", word );
    }
  }
}



/*
 * Load the sysdata file
 */
bool load_systemdata( SYSTEM_DATA * sys )
{
  char filename[MAX_INPUT_LENGTH];
  FILE *fp;
  bool found;

  found = FALSE;
  sprintf( filename, "%ssysdata.dat", SYSTEM_DIR );

  if( ( fp = fopen( filename, "r" ) ) != NULL )
  {

    found = TRUE;
    for( ;; )
    {
      char letter;
      char *word;

      letter = fread_letter( fp );
      if( letter == '*' )
      {
	fread_to_eol( fp );
	continue;
      }

      if( letter != '#' )
      {
	bug( "Load_sysdata_file: # not found." );
	break;
      }

      word = fread_word( fp );
      if( !str_cmp( word, "SYSTEM" ) )
      {
	fread_sysdata( sys, fp );
	break;
      }
      else if( !str_cmp( word, "END" ) )
	break;
      else
      {
	bug( "Load_sysdata_file: bad section." );
	break;
      }
    }
    fclose( fp );
  }

  return found;
}


void load_banlist( void )
{
  BAN_DATA *pban = NULL;
  FILE *fp = NULL;
  int number = 0;
  char letter = 0;

  if( !( fp = fopen( SYSTEM_DIR BAN_LIST, "r" ) ) )
    return;

  for( ;; )
  {
    if( feof( fp ) )
    {
      bug( "Load_banlist: no -1 found." );
      fclose( fp );
      return;
    }
    number = fread_number( fp );
    if( number == -1 )
    {
      fclose( fp );
      return;
    }
    CREATE( pban, BAN_DATA, 1 );
    pban->level = number;
    pban->name = fread_string_nohash( fp );
    if( ( letter = fread_letter( fp ) ) == '~' )
      pban->ban_time = fread_string_nohash( fp );
    else
    {
      ungetc( letter, fp );
      pban->ban_time = str_dup( "(unrecorded)" );
    }
    LINK( pban, first_ban, last_ban, next, prev );
  }
}

bool is_valid_filename( const CHAR_DATA * ch, const char *direct,
    const char *filename )
{
  char newfilename[256];
  struct stat fst;

  /* Length restrictions */
  if( !filename || filename[0] == '\0' || strlen( filename ) < 3 )
  {
    if( !filename || !str_cmp( filename, "" ) )
      send_to_char( "Empty filename is not valid.\r\n", ch );
    else
      ch_printf( ch, "%s: Filename is too short.\r\n", filename );

    return FALSE;
  }

  /* Illegal characters */
  if( strstr( filename, ".." ) || strstr( filename, "/" )
      || strstr( filename, "\\" ) )
  {
    send_to_char
      ( "A filename may not contain a '..', '/', or '\\' in it.\r\n", ch );
    return FALSE;
  }

  /* If that filename is already being used lets not allow it now to be on the safe side */
  sprintf( newfilename, "%s%s", direct, filename );

  if( stat( newfilename, &fst ) != -1 )
  {
    ch_printf( ch, "%s is already an existing filename.\r\n", newfilename );
    return FALSE;
  }

  /* If we got here assume its valid */
  return TRUE;
}

void free_ban( BAN_DATA * ban )
{
  DISPOSE( ban->name );
  DISPOSE( ban->ban_time );
  DISPOSE( ban );
}

static void free_ban_list( void )
{
  while( first_ban )
  {
    BAN_DATA *ban = first_ban;
    first_ban = ban->next;
    free_ban( ban );
  }

  first_ban = NULL;
  last_ban = NULL;
}

static void free_all_descriptors( void )
{
  DESCRIPTOR_DATA *d = NULL;
  DESCRIPTOR_DATA *d_next = NULL;

  for( d = first_descriptor; d; d = d_next )
  {
    d_next = d->next;
    close_socket( d, TRUE );
  }
}

void free_help( HELP_DATA * help )
{
  if( help->text )
    STRFREE( help->text );

  if( help->keyword )
    STRFREE( help->keyword );

  DISPOSE( help );
}

static void free_help_list( void )
{
  while( first_help )
  {
    HELP_DATA *help = first_help;
    first_help = help->next;
    UNLINK( help, first_help, last_help, next, prev );
    free_help( help );
  }
}

void free_repair( REPAIR_DATA * repair )
{
  DISPOSE( repair );
}

void free_shop( SHOP_DATA * shop )
{
  DISPOSE( shop );
}

static void free_shop_list( void )
{
  while( first_shop )
  {
    SHOP_DATA *shop = first_shop;
    first_shop = shop->next;
    free_shop( shop );
  }

  while( first_repair )
  {
    REPAIR_DATA *repair = first_repair;
    first_repair = repair->next;
    free_repair( repair );
  }
}

void free_space_data( SPACE_DATA * starsystem )
{
  if( starsystem->filename )
  {
    STRFREE( starsystem->filename );
  }

  if( starsystem->name )
  {
    STRFREE( starsystem->name );
  }

  if( starsystem->star1 )
  {
    STRFREE( starsystem->star1 );
  }

  if( starsystem->star2 )
  {
    STRFREE( starsystem->star2 );
  }

  DISPOSE( starsystem );
}

static void free_space_data_list( void )
{
  while( first_starsystem )
  {
    SPACE_DATA *s = first_starsystem;
    first_starsystem = s->next;
    UNLINK( s, first_starsystem, last_starsystem, next, prev );
    free_space_data( s );
  }
}

static void free_command_list( void )
{
  int hash = 0;

  for( hash = 0; hash < 126; hash++ )
  {
    CMDTYPE *command = NULL;
    CMDTYPE *cmd_next = NULL;

    for( command = command_hash[hash]; command; command = cmd_next )
    {
      cmd_next = command->next;
      unlink_command( command );
      free_command( command );
    }
  }
}

static void free_social_list( void )
{
  int hash = 0;

  for( hash = 0; hash < 27; hash++ )
  {
    SOCIALTYPE *social = NULL;
    SOCIALTYPE *s_next = NULL;

    for( social = social_index[hash]; social; social = s_next )
    {
      s_next = social->next;
      unlink_social( social );
      free_social( social );
    }
  }
}

static void free_sysdata( void )
{
  if( sysdata.time_of_max )
  {
    DISPOSE( sysdata.time_of_max );
  }

  if( sysdata.officials )
  {
    STRFREE( sysdata.officials );
  }

  if( sysdata.exe_filename )
  {
    STRFREE( sysdata.exe_filename );
  }
}

void free_smaug_affect( SMAUG_AFF * aff )
{
  if( aff->duration )
    DISPOSE( aff->duration );

  if( aff->modifier )
    DISPOSE( aff->modifier );

  DISPOSE( aff );
}

void free_skill( SKILLTYPE * skill )
{
  while( skill->affects )
  {
    SMAUG_AFF *aff = skill->affects;
    skill->affects = aff->next;
    free_smaug_affect( aff );
  }

  if( skill->components )
    DISPOSE( skill->components );

  if( skill->noun_damage )
    DISPOSE( skill->noun_damage );

  if( skill->dice )
    DISPOSE( skill->dice );

  if( skill->die_char )
    DISPOSE( skill->die_char );

  if( skill->die_room )
    DISPOSE( skill->die_room );

  if( skill->die_vict )
    DISPOSE( skill->die_vict );

  if( skill->hit_char )
    DISPOSE( skill->hit_char );

  if( skill->hit_room )
    DISPOSE( skill->hit_room );

  if( skill->hit_vict )
    DISPOSE( skill->hit_vict );

  if( skill->imm_char )
    DISPOSE( skill->imm_char );

  if( skill->imm_room )
    DISPOSE( skill->imm_room );

  if( skill->imm_vict )
    DISPOSE( skill->imm_vict );

  if( skill->miss_char )
    DISPOSE( skill->miss_char );

  if( skill->miss_room )
    DISPOSE( skill->miss_room );

  if( skill->miss_vict )
    DISPOSE( skill->miss_vict );

  if( skill->name )
    DISPOSE( skill->name );

  if( skill->msg_off )
    DISPOSE( skill->msg_off );

  if( skill->fun_name )
    DISPOSE( skill->fun_name );

  DISPOSE( skill );
}

static void free_skill_list( void )
{
  int sn = 0;

  for( sn = 0; sn < top_sn && skill_table[sn] && skill_table[sn]->name; sn++ )
  {
    SKILLTYPE *skill = skill_table[sn];

    if( skill )
    {
      free_skill( skill );
      skill_table[sn] = NULL;
    }
  }
}

void free_extra_descr( EXTRA_DESCR_DATA * ed )
{
  STRFREE( ed->keyword );
  STRFREE( ed->description );
  DISPOSE( ed );
  top_ed--;
}

void free_room( ROOM_INDEX_DATA * room )
{
  STRFREE( room->name );
  STRFREE( room->description );
  room->name = NULL;
  room->description = NULL;

  while( room->first_extradesc )
  {
    EXTRA_DESCR_DATA *ed = room->first_extradesc;
    room->first_extradesc = ed->next;
    free_extra_descr( ed );
  }

  room->first_extradesc = NULL;
  room->last_extradesc = NULL;

  while( room->first_exit )
  {
    extract_exit( room, room->first_exit );
  }

  room->first_exit = NULL;
  room->last_exit = NULL;

  while( room->mpact )
  {
    MPROG_ACT_LIST *mpact = room->mpact;
    room->mpact = mpact->next;
    DISPOSE( mpact->buf );
    DISPOSE( mpact );
  }

  while( room->mudprogs )
  {
    MPROG_DATA *mprog = room->mudprogs;
    room->mudprogs = mprog->next;
    STRFREE( mprog->arglist );
    STRFREE( mprog->comlist );
    DISPOSE( mprog );
  }

  DISPOSE( room );
}

void free_turret( TURRET_DATA * turret )
{
  DISPOSE( turret );
}

void free_hangar( HANGAR_DATA * hangar )
{
  DISPOSE( hangar );
}

void free_ship( SHIP_DATA * ship )
{
  size_t n = 0;

  while( ship->first_turret )
  {
    TURRET_DATA *turret = ship->first_turret;
    ship->first_turret = turret->next;
    free_turret( turret );
  }

  while( ship->first_hangar )
  {
    HANGAR_DATA *hangar = ship->first_hangar;
    ship->first_hangar = hangar->next;
    free_hangar( hangar );
  }

  if( ship->filename )
    DISPOSE( ship->filename );

  if( ship->name )
    STRFREE( ship->name );

  if( ship->home )
    STRFREE( ship->home );

  for( n = 0; n < MAX_SHIP_ROOMS; ++n )
  {
    if( ship->description[n] )
      STRFREE( ship->description[n] );
  }

  if( ship->pilot )
    STRFREE( ship->pilot );

  if( ship->copilot )
    STRFREE( ship->copilot );

  if( ship->dest )
    STRFREE( ship->dest );

  if( ship->owner )
    STRFREE( ship->owner );

  while( ship->first_room )
  {
    ROOM_INDEX_DATA *room = ship->first_room;
    ship->first_room = room->next;
    free_room( room );
  }

  DISPOSE( ship );
}

static void free_ship_list( void )
{
  while( first_ship )
  {
    SHIP_DATA *ship = first_ship;
    first_ship = ship->next;
    free_ship( ship );
  }
}

void free_ship_prototype( SHIP_PROTOTYPE * proto )
{
  if( proto->filename )
    DISPOSE( proto->filename );

  if( proto->name )
    STRFREE( proto->name );;

  if( proto->description )
    STRFREE( proto->description );

  DISPOSE( proto );
}

static void free_ship_prototype_list( void )
{
  while( first_ship_prototype )
  {
    SHIP_PROTOTYPE *proto = first_ship_prototype;
    first_ship_prototype = proto->next;
    free_ship_prototype( proto );
  }
}

void free_clan( CLAN_DATA * clan )
{
  if( clan->filename )
    DISPOSE( clan->filename );

  if( clan->name )
    STRFREE( clan->name );

  if( clan->description )
    STRFREE( clan->description );

  if( clan->leaders )
    STRFREE( clan->leaders );

  if( clan->atwar )
    STRFREE( clan->atwar );

  DISPOSE( clan );
}

static void free_clan_list( void )
{
  while( first_clan )
  {
    CLAN_DATA *clan = first_clan;
    first_clan = clan->next;
    free_clan( clan );
  }
}

void free_guard( GUARD_DATA * guard )
{
  DISPOSE( guard );
}

void free_planet( PLANET_DATA * planet )
{
  while( planet->first_guard )
  {
    GUARD_DATA *guard = planet->first_guard;
    planet->first_guard = guard->next;
    UNLINK( guard, first_guard, last_guard, next, prev );
    free_guard( guard );
  }

  if( planet->name )
    STRFREE( planet->name );

  if( planet->filename )
    DISPOSE( planet->filename );

  DISPOSE( planet );
}

static void free_planet_list( void )
{
  while( first_planet )
  {
    PLANET_DATA *planet = first_planet;
    first_planet = planet->next;
    free_planet( planet );
  }
}

static void free_character_list( void )
{
  while( first_char )
  {
    CHAR_DATA *ch = first_char;
    first_char = ch->next;
    free_char( ch );
  }

  clean_char_queue();
}

void dispose_area( AREA_DATA * area )
{
  while( area->first_room )
  {
    ROOM_INDEX_DATA *room = area->first_room;
    UNLINK( room, area->first_room, area->last_room,
	next_in_area, prev_in_area );
    free_room( room );
  }

  free_area( area );
}

static void free_area_list( void )
{
  while( first_area )
  {
    AREA_DATA *area = first_area;
    UNLINK( area, first_area, last_area, next, prev );
    dispose_area( area );
  }
}

static void free_obj_list( void )
{
  while( first_object )
  {
    OBJ_DATA *obj = first_object;
    extract_obj( obj );
  }

  clean_obj_queue();
}

void free_obj_index( OBJ_INDEX_DATA * obj )
{
  while( obj->first_affect )
  {
    AFFECT_DATA *aff = obj->first_affect;
    obj->first_affect = aff->next;
    DISPOSE( aff );
  }

  while( obj->first_extradesc )
  {
    EXTRA_DESCR_DATA *ed = obj->first_extradesc;
    obj->first_extradesc = ed;
    free_extra_descr( ed );
  }

  while( obj->mudprogs )
  {
    MPROG_DATA *mprog = obj->mudprogs;
    obj->mudprogs = mprog->next;
    STRFREE( mprog->arglist );
    STRFREE( mprog->comlist );
    DISPOSE( mprog );
  }

  if( obj->name )
    STRFREE( obj->name );

  if( obj->short_descr )
    STRFREE( obj->short_descr );

  if( obj->description )
    STRFREE( obj->description );

  if( obj->action_desc )
    STRFREE( obj->action_desc );

  DISPOSE( obj );
}

static void free_obj_index_list( void )
{
  int hash = 0;

  for( hash = 0; hash < MAX_KEY_HASH; hash++ )
  {
    OBJ_INDEX_DATA *obj = NULL;
    OBJ_INDEX_DATA *obj_next = NULL;

    for( obj = obj_index_hash[hash]; obj; obj = obj_next )
    {
      obj_next = obj->next;
      free_obj_index( obj );
    }
  }
}

void free_mob_index( MOB_INDEX_DATA * mob )
{
  while( mob->mudprogs )
  {
    MPROG_DATA *mprog = mob->mudprogs;
    mob->mudprogs = mprog->next;
    STRFREE( mprog->arglist );
    STRFREE( mprog->comlist );
    DISPOSE( mprog );
  }

  if( mob->player_name )
    STRFREE( mob->player_name );

  if( mob->short_descr )
    STRFREE( mob->short_descr );

  if( mob->long_descr )
    STRFREE( mob->long_descr );

  if( mob->description )
    STRFREE( mob->description );

  DISPOSE( mob );
}

static void free_mob_index_list( void )
{
  int hash = 0;

  for( hash = 0; hash < MAX_KEY_HASH; ++hash )
  {
    MOB_INDEX_DATA *mob = NULL;
    MOB_INDEX_DATA *mob_next = NULL;

    for( mob = mob_index_hash[hash]; mob; mob = mob_next )
    {
      mob_next = mob->next;
      free_mob_index( mob );
    }
  }
}

void free_board( BOARD_DATA * board )
{
  while( board->first_note )
  {
    NOTE_DATA *note = board->first_note;
    board->first_note = note->next;
    free_note( note );
  }

  if( board->extra_readers )
    DISPOSE( board->extra_readers );

  if( board->extra_removers )
    DISPOSE( board->extra_removers );

  if( board->read_group )
    DISPOSE( board->read_group );

  if( board->post_group )
    DISPOSE( board->post_group );

  if( board->note_file )
    DISPOSE( board->note_file );

  DISPOSE( board );
}

void free_board_list( void )
{
  while( first_board )
  {
    BOARD_DATA *board = first_board;
    first_board = board->next;
    free_board( board );
  }
}

static void free_string_literals( void );

void free_memory( void )
{
  free_ban_list();
  free_help_list();
  free_space_data_list();
  free_command_list();
  free_social_list();
  free_skill_list();
  free_ship_list();
  free_ship_prototype_list();
  free_clan_list();
  free_planet_list();
  free_all_descriptors();
  free_obj_list();
  free_character_list();
  free_obj_index_list();
  free_mob_index_list();
  free_area_list();
  free_shop_list();
  free_board_list();

  if( auction )
    DISPOSE( auction );

  free_sysdata();
  free_string_literals();
}

/* Non-const string literals */

char *STRLIT_EMPTY = NULL;
char *STRLIT_AUTO = NULL;
char *STRLIT_FULL = NULL;

void allocate_string_literals( void )
{
  STRLIT_EMPTY = str_dup( "" );
  STRLIT_AUTO  = str_dup( "auto" );
  STRLIT_FULL  = str_dup( "full" );
}

static void free_string_literals( void )
{
  DISPOSE( STRLIT_FULL );
  DISPOSE( STRLIT_AUTO );
  DISPOSE( STRLIT_EMPTY );
}
