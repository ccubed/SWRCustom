#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "mud.h"

/* global variables */
int top_sn = 0;

SKILLTYPE *skill_table[MAX_SKILL];

const char *const skill_tname[] = { "unknown", "Spell", "Skill", "Weapon" };

#ifdef SWR2_USE_DLSYM

SPELL_FUN *spell_function( const char *name )
{
  SPELL_FUN *fun_handle = (SPELL_FUN*) dlsym( sysdata.dl_handle, name );

  if( !fun_handle )
  {
    bug( "Could not find symbol '%s': %s", name, dlerror() );
    return spell_notfound;
  }

  return fun_handle;
}

DO_FUN *skill_function( const char *name )
{
  DO_FUN *fun_handle = (DO_FUN*) dlsym( sysdata.dl_handle, name );

  if( !fun_handle )
  {
    bug( "Could not find symbol '%s': %s", name, dlerror() );
    return skill_notfound;
  }

  return fun_handle;
}

#else

typedef struct spell_fun_entry
{
  const char *fun_name;
  SPELL_FUN *fun_ptr;
} SPELL_FUN_ENTRY;

static const SPELL_FUN_ENTRY spell_fun_table[] = {
  { "spell_blindness",      spell_blindness },
  { "spell_charm_person",   spell_charm_person },
  { "spell_fireball",       spell_fireball },
  { "spell_identify",       spell_identify },
  { "spell_invis",          spell_invis },
  { "spell_lightning_bolt", spell_lightning_bolt },
  { "spell_notfound",       spell_notfound },
  { "spell_null",           spell_null },
  { "spell_poison",         spell_poison },
  { "spell_possess",        spell_possess },
  { "spell_sleep",          spell_sleep },
  { "spell_smaug",          spell_smaug }
};

static size_t spell_fun_table_size( void )
{
  return sizeof( spell_fun_table ) / sizeof( *spell_fun_table );
}

SPELL_FUN *spell_function( const char *name )
{
  SPELL_FUN *fun_ptr = spell_notfound;
  size_t i = 0;

  for( i = 0; i < spell_fun_table_size(); ++i )
    {
      if( !str_cmp( name, spell_fun_table[ i ].fun_name ) )
	{
	  fun_ptr = spell_fun_table[ i ].fun_ptr;
	  break;
	}
    }

  return fun_ptr;
}

typedef struct do_fun_entry
{
  const char *fun_name;
  DO_FUN *fun_ptr;
} DO_FUN_ENTRY;

static const DO_FUN_ENTRY command_fun_table[] = {
  { "do_aassign", do_aassign },
  { "do_accelerate", do_accelerate },
  { "do_addpilot", do_addpilot },
  { "do_advance", do_advance },
  { "do_affected", do_affected },
  { "do_afk", do_afk },
  { "do_aid", do_aid },
  { "do_allow", do_allow },
  { "do_allsave", do_allsave },
  { "do_allships", do_allships },
  { "do_allspeeders", do_allspeeders },
  { "do_ammo", do_ammo },
  { "do_ansi", do_ansi },
  { "do_appoint", do_appoint },
  { "do_appraise", do_appraise },
  { "do_areas", do_areas },
  { "do_arrest", do_arrest },
  { "do_aset", do_aset },
  { "do_astat", do_astat },
  { "do_at", do_at },
  { "do_auction", do_auction },
  { "do_authorize", do_authorize },
  { "do_autopilot", do_autopilot },
  { "do_autorecharge", do_autorecharge },
  { "do_autotrack", do_autotrack },
  { "do_backstab", do_backstab },
  { "do_balzhur", do_balzhur },
  { "do_bamfin", do_bamfin },
  { "do_bamfout", do_bamfout },
  { "do_ban", do_ban },
  { "do_bank", do_bank },
  { "do_bashdoor", do_bashdoor },
  { "do_beep", do_beep },
  { "do_berserk", do_berserk },
  { "do_bestow", do_bestow },
  { "do_bestowarea", do_bestowarea },
  { "do_bio", do_bio },
  { "do_board", do_board },
  { "do_boards", do_boards },
  { "do_bodybag", do_bodybag },
  { "do_bomb", do_bomb },
  { "do_bridge", do_bridge },
  { "do_bset", do_bset },
  { "do_bstat", do_bstat },
  { "do_bug", do_bug },
  { "do_bury", do_bury },
  { "do_buy", do_buy },
  { "do_buyhome", do_buyhome },
  { "do_buyship", do_buyship },
  { "do_calculate", do_calculate },
  { "do_capture", do_capture },
  { "do_cast", do_cast },
  { "do_cedit", do_cedit },
  { "do_chaff", do_chaff },
  { "do_channels", do_channels },
  { "do_circle", do_circle },
  { "do_clan_donate", do_clan_donate },
  { "do_clan_withdraw", do_clan_withdraw },
  { "do_clanbuyship", do_clanbuyship },
  { "do_clans", do_clans },
  { "do_clantalk", do_clantalk },
  { "do_climb", do_climb },
  { "do_clone", do_clone },
  { "do_close", do_close },
  { "do_closebay", do_closebay },
  { "do_closehatch", do_closehatch },
  { "do_cmdtable", do_cmdtable },
  { "do_commands", do_commands },
  { "do_compare", do_compare },
  { "do_config", do_config },
  { "do_consider", do_consider },
  { "do_construction", do_construction },
  { "do_copyover", do_copyover },
  { "do_credits", do_credits },
  { "do_cset", do_cset },
  { "do_demote", do_demote },
  { "do_deny", do_deny },
  { "do_description", do_description },
  { "do_designship", do_designship },
  { "do_destroy", do_destroy },
  { "do_dig", do_dig },
  { "do_disarm", do_disarm },
  { "do_disconnect", do_disconnect },
  { "do_disguise", do_disguise },
  { "do_dismount", do_dismount },
  { "do_dmesg", do_dmesg },
  { "do_down", do_down },
  { "do_drag", do_drag },
  { "do_drink", do_drink },
  { "do_drive", do_drive },
  { "do_drop", do_drop },
  { "do_east", do_east },
  { "do_eat", do_eat },
  { "do_echo", do_echo },
  { "do_emote", do_emote },
  { "do_empower", do_empower },
  { "do_empty", do_empty },
  { "do_enter", do_enter },
  { "do_equipment", do_equipment },
  { "do_examine", do_examine },
  { "do_exits", do_exits },
  { "do_fill", do_fill },
  { "do_fire", do_fire },
  { "do_first_aid", do_first_aid },
  { "do_fixchar", do_fixchar },
  { "do_flee", do_flee },
  { "do_fly", do_fly },
  { "do_foldarea", do_foldarea },
  { "do_follow", do_follow },
  { "do_for", do_for },
  { "do_force", do_force },
  { "do_forceclose", do_forceclose },
  { "do_forget", do_forget },
  { "do_form_password", do_form_password },
  { "do_freeze", do_freeze },
  { "do_get", do_get },
  { "do_give", do_give },
  { "do_glance", do_glance },
  { "do_gnet", do_gnet },
  { "do_gold", do_gold },
  { "do_goto", do_goto },
  { "do_gouge", do_gouge },
  { "do_group", do_group },
  { "do_gtell", do_gtell },
  { "do_hail", do_hail },
  { "do_hedit", do_hedit },
  { "do_help", do_help },
  { "do_hide", do_hide },
  { "do_hijack", do_hijack },
  { "do_hitall", do_hitall },
  { "do_hlist", do_hlist },
  { "do_holylight", do_holylight },
  { "do_homepage", do_homepage },
  { "do_hset", do_hset },
  { "do_hyperspace", do_hyperspace },
  { "do_immortalize", do_immortalize },
  { "do_immtalk", do_immtalk },
  { "do_induct", do_induct },
  { "do_info", do_info },
  { "do_inventory", do_inventory },
  { "do_invis", do_invis },
  { "do_kick", do_kick },
  { "do_kill", do_kill },
  { "do_land", do_land },
  { "do_landscape", do_landscape },
  { "do_last", do_last },
  { "do_launch", do_launch },
  { "do_leave", do_leave },
  { "do_leaveship", do_leaveship },
  { "do_list", do_list },
  { "do_loadup", do_loadup },
  { "do_lock", do_lock },
  { "do_log", do_log },
  { "do_look", do_look },
  { "do_low_purge", do_low_purge },
  { "do_mailroom", do_mailroom },
  { "do_makearmor", do_makearmor },
  { "do_makeblade", do_makeblade },
  { "do_makeblaster", do_makeblaster },
  { "do_makeboard", do_makeboard },
  { "do_makeclan", do_makeclan },
  { "do_makecontainer", do_makecontainer },
  { "do_makejewelry", do_makejewelry },
  { "do_makelightsaber", do_makelightsaber },
  { "do_makeplanet", do_makeplanet },
  { "do_makerepair", do_makerepair },
  { "do_makeshield", do_makeshield },
  { "do_makeship", do_makeship },
  { "do_makeshop", do_makeshop },
  { "do_mcreate", do_mcreate },
  { "do_memory", do_memory },
  { "do_mfind", do_mfind },
  { "do_minvoke", do_minvoke },
  { "do_mlist", do_mlist },
  { "do_mount", do_mount },
  { "do_mp_close_passage", do_mp_close_passage },
  { "do_mp_damage", do_mp_damage },
  { "do_mp_deposit", do_mp_deposit },
  { "do_mp_offer_job", do_mp_offer_job },
  { "do_mp_open_passage", do_mp_open_passage },
  { "do_mp_practice", do_mp_practice },
  { "do_mp_restore", do_mp_restore },
  { "do_mp_slay", do_mp_slay },
  { "do_mp_withdraw", do_mp_withdraw },
  { "do_mpadvance", do_mpadvance },
  { "do_mpapply", do_mpapply },
  { "do_mpapplyb", do_mpapplyb },
  { "do_mpasound", do_mpasound },
  { "do_mpat", do_mpat },
  { "do_mpdream", do_mpdream },
  { "do_mpecho", do_mpecho },
  { "do_mpechoaround", do_mpechoaround },
  { "do_mpechoat", do_mpechoat },
  { "do_mpedit", do_mpedit },
  { "do_mpforce", do_mpforce },
  { "do_mpgain", do_mpgain },
  { "do_mpgoto", do_mpgoto },
  { "do_mpinvis", do_mpinvis },
  { "do_mpjunk", do_mpjunk },
  { "do_mpkill", do_mpkill },
  { "do_mpmload", do_mpmload },
  { "do_mpnothing", do_mpnothing },
  { "do_mpoload", do_mpoload },
  { "do_mppkset", do_mppkset },
  { "do_mppurge", do_mppurge },
  { "do_mpstat", do_mpstat },
  { "do_mptransfer", do_mptransfer },
  { "do_mset", do_mset },
  { "do_mstat", do_mstat },
  { "do_murder", do_murder },
  { "do_mwhere", do_mwhere },
  { "do_newbiechat", do_newbiechat },
  { "do_newbieset", do_newbieset },
  { "do_noemote", do_noemote },
  { "do_noresolve", do_noresolve },
  { "do_north", do_north },
  { "do_northeast", do_northeast },
  { "do_northwest", do_northwest },
  { "do_notell", do_notell },
  { "do_noteroom", do_noteroom },
  { "do_notitle", do_notitle },
  { "do_ocreate", do_ocreate },
  { "do_ofind", do_ofind },
  { "do_oinvoke", do_oinvoke },
  { "do_oldscore", do_oldscore },
  { "do_olist", do_olist },
  { "do_ooc", do_ooc },
  { "do_opedit", do_opedit },
  { "do_open", do_open },
  { "do_openbay", do_openbay },
  { "do_openhatch", do_openhatch },
  { "do_opstat", do_opstat },
  { "do_order", do_order },
  { "do_oset", do_oset },
  { "do_ostat", do_ostat },
  { "do_outcast", do_outcast },
  { "do_overthrow", do_overthrow },
  { "do_owhere", do_owhere },
  { "do_pager", do_pager },
  { "do_password", do_password },
  { "do_peace", do_peace },
  { "do_pick", do_pick },
  { "do_pickshiplock", do_pickshiplock },
  { "do_planets", do_planets },
  { "do_pnet", do_pnet },
  { "do_poison_weapon", do_poison_weapon },
  { "do_postguard", do_postguard },
  { "do_prompt", do_prompt },
  { "do_propeganda", do_propeganda },
  { "do_prototypes", do_prototypes },
  { "do_purge", do_purge },
  { "do_put", do_put },
  { "do_quicktalk", do_quicktalk },
  { "do_quit", do_quit },
  { "do_radar", do_radar },
  { "do_rank", do_rank },
  { "do_rat", do_rat },
  { "do_reboot", do_reboot },
  { "do_recall", do_recall },
  { "do_recharge", do_recharge },
  { "do_recho", do_recho },
  { "do_redit", do_redit },
  { "do_regoto", do_regoto },
  { "do_reinforcements", do_reinforcements },
  { "do_remove", do_remove },
  { "do_rempilot", do_rempilot },
  { "do_rentship", do_rentship },
  { "do_repair", do_repair },
  { "do_repairset", do_repairset },
  { "do_repairship", do_repairship },
  { "do_repairshops", do_repairshops },
  { "do_repairstat", do_repairstat },
  { "do_reply", do_reply },
  { "do_rescue", do_rescue },
  { "do_reset", do_reset },
  { "do_resetship", do_resetship },
  { "do_resign", do_resign },
  { "do_rest", do_rest },
  { "do_restore", do_restore },
  { "do_restoretime", do_restoretime },
  { "do_restrict", do_restrict },
  { "do_retire", do_retire },
  { "do_retran", do_retran },
  { "do_return", do_return },
  { "do_revert", do_revert },
  { "do_rlist", do_rlist },
  { "do_rpedit", do_rpedit },
  { "do_rpstat", do_rpstat },
  { "do_rstat", do_rstat },
  { "do_save", do_save },
  { "do_say", do_say },
  { "do_scan", do_scan },
  { "do_score", do_score },
  { "do_search", do_search },
  { "do_sedit", do_sedit },
  { "do_sell", do_sell },
  { "do_sellship", do_sellship },
  { "do_set_boot_time", do_set_boot_time },
  { "do_setblaster", do_setblaster },
  { "do_setclan", do_setclan },
  { "do_setplanet", do_setplanet },
  { "do_setprototype", do_setprototype },
  { "do_setship", do_setship },
  { "do_setstarsystem", do_setstarsystem },
  { "do_setwages", do_setwages },
  { "do_ships", do_ships },
  { "do_shiptalk", do_shiptalk },
  { "do_shops", do_shops },
  { "do_shopset", do_shopset },
  { "do_shopstat", do_shopstat },
  { "do_shove", do_shove },
  { "do_showclan", do_showclan },
  { "do_showplanet", do_showplanet },
  { "do_showprototype", do_showprototype },
  { "do_showship", do_showship },
  { "do_showstarsystem", do_showstarsystem },
  { "do_shutdown", do_shutdown },
  { "do_silence", do_silence },
  { "do_sit", do_sit },
  { "do_skills", do_skills },
  { "do_slay", do_slay },
  { "do_sleep", do_sleep },
  { "do_slice", do_slice },
  { "do_slist", do_slist },
  { "do_slookup", do_slookup },
  { "do_sneak", do_sneak },
  { "do_snipe", do_snipe },
  { "do_snoop", do_snoop },
  { "do_sober", do_sober },
  { "do_socials", do_socials },
  { "do_south", do_south },
  { "do_southeast", do_southeast },
  { "do_southwest", do_southwest },
  { "do_speeders", do_speeders },
  { "do_split", do_split },
  { "do_sset", do_sset },
  { "do_stand", do_stand },
  { "do_starsystems", do_starsystems },
  { "do_status", do_status },
  { "do_steal", do_steal },
  { "do_suicide", do_suicide },
  { "do_survey", do_survey },
  { "do_switch", do_switch },
  { "do_systemtalk", do_systemtalk },
  { "do_target", do_target },
  { "do_teach", do_teach },
  { "do_tell", do_tell },
  { "do_throw", do_throw },
  { "do_time", do_time },
  { "do_timecmd", do_timecmd },
  { "do_title", do_title },
  { "do_torture", do_torture },
  { "do_track", do_track },
  { "do_tractorbeam", do_tractorbeam },
  { "do_trajectory", do_trajectory },
  { "do_transfer", do_transfer },
  { "do_transship", do_transship },
  { "do_trust", do_trust },
  { "do_typo", do_typo },
  { "do_unfoldarea", do_unfoldarea },
  { "do_unlock", do_unlock },
  { "do_unsilence", do_unsilence },
  { "do_up", do_up },
  { "do_use", do_use },
  { "do_users", do_users },
  { "do_value", do_value },
  { "do_visible", do_visible },
  { "do_vnums", do_vnums },
  { "do_vsearch", do_vsearch },
  { "do_wake", do_wake },
  { "do_war", do_war },
  { "do_wear", do_wear },
  { "do_weather", do_weather },
  { "do_west", do_west },
  { "do_where", do_where },
  { "do_who", do_who },
  { "do_whois", do_whois },
  { "do_wimpy", do_wimpy },
  { "do_wizhelp", do_wizhelp },
  { "do_wizlist", do_wizlist },
  { "do_wizlock", do_wizlock },
  { "do_yell", do_yell },
  { "do_zones", do_zones }
};

static size_t command_fun_table_size( void )
{
  return sizeof( command_fun_table ) / sizeof( *command_fun_table );
}

DO_FUN *skill_function( const char *name )
{
  DO_FUN *fun_ptr = skill_notfound;
  size_t i = 0;

  for( i = 0; i < command_fun_table_size(); ++i )
    {
      if( !str_cmp( name, command_fun_table[ i ].fun_name ) )
        {
          fun_ptr = command_fun_table[ i ].fun_ptr;
	  break;
        }
    }

  return fun_ptr;
}

#endif // SWR2_USE_DLSYM

/*
 * Function used by qsort to sort skills
 */
int skill_comp( const SKILLTYPE ** sk1, const SKILLTYPE ** sk2 )
{
  const SKILLTYPE *skill1 = ( *sk1 );
  const SKILLTYPE *skill2 = ( *sk2 );

  if( !skill1 && skill2 )
    return 1;
  if( skill1 && !skill2 )
    return -1;
  if( !skill1 && !skill2 )
    return 0;
  if( skill1->type < skill2->type )
    return -1;
  if( skill1->type > skill2->type )
    return 1;

  return strcmp( skill1->name, skill2->name );
}

/*
 * Sort the skill table with qsort
 */
void sort_skill_table()
{
  log_string( "Sorting skill table..." );
  qsort( &skill_table[1], top_sn - 1, sizeof( SKILLTYPE * ),
      ( int ( * )( const void *, const void * ) ) skill_comp );
}


/*
 * Write skill data to a file
 */
void fwrite_skill( FILE * fpout, SKILLTYPE * skill )
{
  SMAUG_AFF *aff = NULL;

  fprintf( fpout, "Name         %s~\n", skill->name );
  fprintf( fpout, "Type         %s\n", skill_tname[skill->type] );
  fprintf( fpout, "Flags        %d\n", skill->flags );

  if( skill->target )
    fprintf( fpout, "Target       %d\n", skill->target );

  if( skill->minimum_position )
    fprintf( fpout, "Minpos       %d\n", skill->minimum_position );

  if( skill->saves )
    fprintf( fpout, "Saves        %d\n", skill->saves );

  if( skill->slot )
    fprintf( fpout, "Slot         %d\n", skill->slot );

  if( skill->min_mana )
    fprintf( fpout, "Mana         %d\n", skill->min_mana );

  if( skill->beats )
    fprintf( fpout, "Rounds       %d\n", skill->beats );

  if( skill->skill_fun )
    fprintf( fpout, "Code         %s\n", skill->fun_name );
  else if( skill->spell_fun )
    fprintf( fpout, "Code         %s\n", skill->fun_name );

  fprintf( fpout, "Dammsg       %s~\n", skill->noun_damage );

  if( skill->msg_off && skill->msg_off[0] != '\0' )
    fprintf( fpout, "Wearoff      %s~\n", skill->msg_off );

  if( skill->hit_char && skill->hit_char[0] != '\0' )
    fprintf( fpout, "Hitchar      %s~\n", skill->hit_char );

  if( skill->hit_vict && skill->hit_vict[0] != '\0' )
    fprintf( fpout, "Hitvict      %s~\n", skill->hit_vict );

  if( skill->hit_room && skill->hit_room[0] != '\0' )
    fprintf( fpout, "Hitroom      %s~\n", skill->hit_room );

  if( skill->miss_char && skill->miss_char[0] != '\0' )
    fprintf( fpout, "Misschar     %s~\n", skill->miss_char );

  if( skill->miss_vict && skill->miss_vict[0] != '\0' )
    fprintf( fpout, "Missvict     %s~\n", skill->miss_vict );

  if( skill->miss_room && skill->miss_room[0] != '\0' )
    fprintf( fpout, "Missroom     %s~\n", skill->miss_room );

  if( skill->die_char && skill->die_char[0] != '\0' )
    fprintf( fpout, "Diechar      %s~\n", skill->die_char );

  if( skill->die_vict && skill->die_vict[0] != '\0' )
    fprintf( fpout, "Dievict      %s~\n", skill->die_vict );

  if( skill->die_room && skill->die_room[0] != '\0' )
    fprintf( fpout, "Dieroom      %s~\n", skill->die_room );

  if( skill->imm_char && skill->imm_char[0] != '\0' )
    fprintf( fpout, "Immchar      %s~\n", skill->imm_char );

  if( skill->imm_vict && skill->imm_vict[0] != '\0' )
    fprintf( fpout, "Immvict      %s~\n", skill->imm_vict );

  if( skill->imm_room && skill->imm_room[0] != '\0' )
    fprintf( fpout, "Immroom      %s~\n", skill->imm_room );

  if( skill->dice && skill->dice[0] != '\0' )
    fprintf( fpout, "Dice         %s~\n", skill->dice );

  if( skill->value )
    fprintf( fpout, "Value        %d\n", skill->value );

  if( skill->difficulty )
    fprintf( fpout, "Difficulty   %d\n", skill->difficulty );

  if( skill->participants )
    fprintf( fpout, "Participants %d\n", skill->participants );

  if( skill->components && skill->components[0] != '\0' )
    fprintf( fpout, "Components   %s~\n", skill->components );

  for( aff = skill->affects; aff; aff = aff->next )
    fprintf( fpout, "Affect       '%s' %d '%s' %d\n",
	aff->duration, aff->location, aff->modifier, aff->bitvector );

  if( skill->alignment )
    fprintf( fpout, "Alignment   %d\n", skill->alignment );

  fprintf( fpout, "End\n\n" );
}

/*
 * Save the skill table to disk
 */
void save_skill_table()
{
  int x = 0;
  FILE *fpout = NULL;

  if( ( fpout = fopen( SKILL_FILE, "w" ) ) == NULL )
  {
    bug( "Cannot open skills.dat for writting", 0 );
    perror( SKILL_FILE );
    return;
  }

  for( x = 0; x < top_sn; x++ )
  {
    if( !skill_table[x]->name || skill_table[x]->name[0] == '\0' )
      break;
    fprintf( fpout, "#SKILL\n" );
    fwrite_skill( fpout, skill_table[x] );
  }
  fprintf( fpout, "#END\n" );
  fclose( fpout );
}


/*
 * Save the socials to disk
 */
void save_socials()
{
  FILE *fpout = NULL;
  SOCIALTYPE *social = NULL;
  int x = 0;

  if( ( fpout = fopen( SOCIAL_FILE, "w" ) ) == NULL )
  {
    bug( "Cannot open socials.dat for writting", 0 );
    perror( SOCIAL_FILE );
    return;
  }

  for( x = 0; x < 27; x++ )
  {
    for( social = social_index[x]; social; social = social->next )
    {
      if( !social->name || social->name[0] == '\0' )
      {
	bug( "Save_socials: blank social in hash bucket %d", x );
	continue;
      }
      fprintf( fpout, "#SOCIAL\n" );
      fprintf( fpout, "Name        %s~\n", social->name );
      if( social->char_no_arg )
	fprintf( fpout, "CharNoArg   %s~\n", social->char_no_arg );
      else
	bug( "Save_socials: NULL char_no_arg in hash bucket %d", x );
      if( social->others_no_arg )
	fprintf( fpout, "OthersNoArg %s~\n", social->others_no_arg );
      if( social->char_found )
	fprintf( fpout, "CharFound   %s~\n", social->char_found );
      if( social->others_found )
	fprintf( fpout, "OthersFound %s~\n", social->others_found );
      if( social->vict_found )
	fprintf( fpout, "VictFound   %s~\n", social->vict_found );
      if( social->char_auto )
	fprintf( fpout, "CharAuto    %s~\n", social->char_auto );
      if( social->others_auto )
	fprintf( fpout, "OthersAuto  %s~\n", social->others_auto );
      fprintf( fpout, "End\n\n" );
    }
  }
  fprintf( fpout, "#END\n" );
  fclose( fpout );
}

int get_skill( const char *skilltype )
{
  if( !str_cmp( skilltype, "Spell" ) )
    return SKILL_SPELL;
  if( !str_cmp( skilltype, "Skill" ) )
    return SKILL_SKILL;
  if( !str_cmp( skilltype, "Weapon" ) )
    return SKILL_WEAPON;
  return SKILL_UNKNOWN;
}

/*
 * Save the commands to disk
 */
void save_commands()
{
  FILE *fpout = NULL;
  CMDTYPE *command = NULL;
  int x = 0;

  if( ( fpout = fopen( COMMAND_FILE, "w" ) ) == NULL )
  {
    bug( "Cannot open commands.dat for writing", 0 );
    perror( COMMAND_FILE );
    return;
  }

  for( x = 0; x < 126; x++ )
  {
    for( command = command_hash[x]; command; command = command->next )
    {
      if( !command->name || command->name[0] == '\0' )
      {
	bug( "Save_commands: blank command in hash bucket %d", x );
	continue;
      }
      fprintf( fpout, "#COMMAND\n" );
      fprintf( fpout, "Name        %s~\n", command->name );
      fprintf( fpout, "Code        %s\n", command->fun_name );
      fprintf( fpout, "Position    %d\n", command->position );
      fprintf( fpout, "Level       %d\n", command->level );
      fprintf( fpout, "Log         %d\n", command->log );
      fprintf( fpout, "End\n\n" );
    }
  }
  fprintf( fpout, "#END\n" );
  fclose( fpout );
}

SKILLTYPE *fread_skill( FILE * fp )
{
  char buf[MAX_STRING_LENGTH];
  SKILLTYPE *skill = create_skill();

  for( ;; )
  {
    const char *word = feof( fp ) ? "End" : fread_word( fp );
    bool fMatch = FALSE;

    switch ( UPPER( word[0] ) )
    {
      case '*':
	fMatch = TRUE;
	fread_to_eol( fp );
	break;

      case 'A':
	KEY( "Alignment", skill->alignment, fread_number( fp ) );
	if( !str_cmp( word, "Affect" ) )
	{
	  SMAUG_AFF *aff;

	  CREATE( aff, SMAUG_AFF, 1 );
	  aff->duration = str_dup( fread_word( fp ) );
	  aff->location = fread_number( fp );
	  aff->modifier = str_dup( fread_word( fp ) );
	  aff->bitvector = fread_number( fp );
	  aff->next = skill->affects;
	  skill->affects = aff;
	  fMatch = TRUE;
	  break;
	}
	break;

      case 'C':
	if( !str_cmp( word, "Code" ) )
	{
	  SPELL_FUN *spellfun = NULL;
	  DO_FUN *dofun = NULL;
	  const char *w = fread_word( fp );

	  fMatch = TRUE;

	  if( ( spellfun = spell_function( w ) ) != spell_notfound
	      && !str_prefix( "spell_", w ) )
	  {
	    skill->spell_fun = spellfun;
	    skill->fun_name = str_dup( w );
	  }
	  else if( ( dofun = skill_function( w ) ) != skill_notfound
		   && !str_prefix( "do_", w ) )
	  {
	    skill->skill_fun = dofun;
	    skill->fun_name = str_dup( w );
	  }
	  else
	  {
	    bug( "fread_skill: unknown skill/spell %s", w );
	    skill->fun_name = str_dup( "" );
	  }
	  break;
	}
	KEY( "Components", skill->components, fread_string_nohash( fp ) );
	break;

      case 'D':
	KEY( "Dammsg", skill->noun_damage, fread_string_nohash( fp ) );
	KEY( "Dice", skill->dice, fread_string_nohash( fp ) );
	KEY( "Diechar", skill->die_char, fread_string_nohash( fp ) );
	KEY( "Dieroom", skill->die_room, fread_string_nohash( fp ) );
	KEY( "Dievict", skill->die_vict, fread_string_nohash( fp ) );
	KEY( "Difficulty", skill->difficulty, fread_number( fp ) );
	break;

      case 'E':
	if( !str_cmp( word, "End" ) )
	  return skill;
	break;

      case 'F':
	KEY( "Flags", skill->flags, fread_number( fp ) );
	break;

      case 'H':
	KEY( "Hitchar", skill->hit_char, fread_string_nohash( fp ) );
	KEY( "Hitroom", skill->hit_room, fread_string_nohash( fp ) );
	KEY( "Hitvict", skill->hit_vict, fread_string_nohash( fp ) );
	break;

      case 'I':
	KEY( "Immchar", skill->imm_char, fread_string_nohash( fp ) );
	KEY( "Immroom", skill->imm_room, fread_string_nohash( fp ) );
	KEY( "Immvict", skill->imm_vict, fread_string_nohash( fp ) );
	break;

      case 'M':
	KEY( "Mana", skill->min_mana, fread_number( fp ) );
	KEY( "Minpos", skill->minimum_position, fread_number( fp ) );
	KEY( "Misschar", skill->miss_char, fread_string_nohash( fp ) );
	KEY( "Missroom", skill->miss_room, fread_string_nohash( fp ) );
	KEY( "Missvict", skill->miss_vict, fread_string_nohash( fp ) );
	break;

      case 'N':
	KEY( "Name", skill->name, fread_string_nohash( fp ) );
	break;

      case 'P':
	KEY( "Participants", skill->participants, fread_number( fp ) );
	break;

      case 'R':
	KEY( "Rounds", skill->beats, fread_number( fp ) );
	break;

      case 'S':
	KEY( "Slot", skill->slot, fread_number( fp ) );
	KEY( "Saves", skill->saves, fread_number( fp ) );
	break;

      case 'T':
	KEY( "Target", skill->target, fread_number( fp ) );
	KEY( "Type", skill->type, get_skill( fread_word( fp ) ) );
	break;

      case 'V':
	KEY( "Value", skill->value, fread_number( fp ) );
	break;

      case 'W':
	KEY( "Wearoff", skill->msg_off, fread_string_nohash( fp ) );
	break;
    }

    if( !fMatch )
    {
      sprintf( buf, "Fread_skill: no match: %s", word );
      bug( buf, 0 );
    }
  }
}

void load_skill_table()
{
  FILE *fp = NULL;

  if( ( fp = fopen( SKILL_FILE, "r" ) ) != NULL )
  {
    top_sn = 0;
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
	bug( "Load_skill_table: # not found.", 0 );
	break;
      }

      word = fread_word( fp );
      if( !str_cmp( word, "SKILL" ) )
      {
	if( top_sn >= MAX_SKILL )
	{
	  bug( "load_skill_table: more skills than MAX_SKILL %d",
	      MAX_SKILL );
	  fclose( fp );
	  return;
	}
	skill_table[top_sn++] = fread_skill( fp );
	continue;
      }
      else if( !str_cmp( word, "END" ) )
	break;
      else
      {
	bug( "Load_skill_table: bad section.", 0 );
	continue;
      }
    }
    fclose( fp );
  }
  else
  {
    bug( "Cannot open skills.dat", 0 );
    exit( 0 );
  }
}


void fread_social( FILE * fp )
{
  char buf[MAX_STRING_LENGTH];
  const char *word = NULL;
  bool fMatch = FALSE;
  SOCIALTYPE *social = NULL;

  CREATE( social, SOCIALTYPE, 1 );

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

      case 'C':
	KEY( "CharNoArg", social->char_no_arg, fread_string_nohash( fp ) );
	KEY( "CharFound", social->char_found, fread_string_nohash( fp ) );
	KEY( "CharAuto", social->char_auto, fread_string_nohash( fp ) );
	break;

      case 'E':
	if( !str_cmp( word, "End" ) )
	{
	  if( !social->name )
	  {
	    bug( "Fread_social: Name not found", 0 );
	    free_social( social );
	    return;
	  }
	  if( !social->char_no_arg )
	  {
	    bug( "Fread_social: CharNoArg not found", 0 );
	    free_social( social );
	    return;
	  }
	  add_social( social );
	  return;
	}
	break;

      case 'N':
	KEY( "Name", social->name, fread_string_nohash( fp ) );
	break;

      case 'O':
	KEY( "OthersNoArg", social->others_no_arg,
	    fread_string_nohash( fp ) );
	KEY( "OthersFound", social->others_found,
	    fread_string_nohash( fp ) );
	KEY( "OthersAuto", social->others_auto, fread_string_nohash( fp ) );
	break;

      case 'V':
	KEY( "VictFound", social->vict_found, fread_string_nohash( fp ) );
	break;
    }

    if( !fMatch )
    {
      sprintf( buf, "Fread_social: no match: %s", word );
      bug( buf, 0 );
    }
  }
}

void load_socials()
{
  FILE *fp = NULL;

  if( ( fp = fopen( SOCIAL_FILE, "r" ) ) != NULL )
  {
    top_sn = 0;
    for( ;; )
    {
      const char *word = NULL;
      char letter = fread_letter( fp );

      if( letter == '*' )
      {
	fread_to_eol( fp );
	continue;
      }

      if( letter != '#' )
      {
	bug( "Load_socials: # not found.", 0 );
	break;
      }

      word = fread_word( fp );
      if( !str_cmp( word, "SOCIAL" ) )
      {
	fread_social( fp );
	continue;
      }
      else if( !str_cmp( word, "END" ) )
	break;
      else
      {
	bug( "Load_socials: bad section.", 0 );
	continue;
      }
    }
    fclose( fp );
  }
  else
  {
    bug( "Cannot open socials.dat", 0 );
    exit( 0 );
  }
}

void fread_command( FILE * fp )
{
  char buf[MAX_STRING_LENGTH];
  CMDTYPE *command = NULL;

  CREATE( command, CMDTYPE, 1 );

  for( ;; )
  {
    const char *word = feof( fp ) ? "End" : fread_word( fp );
    bool fMatch = FALSE;

    switch ( UPPER( word[0] ) )
    {
      case '*':
	fMatch = TRUE;
	fread_to_eol( fp );
	break;

      case 'C':
	if( !str_cmp( "Code", word ) )
	{
	  const char *symbol_name = fread_word( fp );
	  command->do_fun = skill_function( symbol_name );
	  fMatch = TRUE;

	  if( command->do_fun != skill_notfound )
	  {
	    command->fun_name = str_dup( symbol_name );
	  }
	  else
	  {
	    command->fun_name = str_dup( "" );
	  }

	  break;
	}
 
	break;

      case 'E':
	if( !str_cmp( word, "End" ) )
	{
	  if( !command->name )
	  {
	    bug( "Fread_command: Name not found", 0 );
	    free_command( command );
	    return;
	  }
	  if( !command->do_fun )
	  {
	    bug( "Fread_command: Function not found", 0 );
	    free_command( command );
	    return;
	  }
	  add_command( command );
	  return;
	}
	break;

      case 'L':
	KEY( "Level", command->level, fread_number( fp ) );
	KEY( "Log", command->log, fread_number( fp ) );
	break;

      case 'N':
	KEY( "Name", command->name, fread_string_nohash( fp ) );
	break;

      case 'P':
	KEY( "Position", command->position, fread_number( fp ) );
	break;
    }

    if( !fMatch )
    {
      sprintf( buf, "Fread_command: no match: %s", word );
      bug( buf, 0 );
    }
  }
}

void load_commands()
{
  FILE *fp = NULL;

  if( ( fp = fopen( COMMAND_FILE, "r" ) ) != NULL )
  {
    top_sn = 0;
    for( ;; )
    {
      const char *word = NULL;
      char letter = fread_letter( fp );

      if( letter == '*' )
      {
	fread_to_eol( fp );
	continue;
      }

      if( letter != '#' )
      {
	bug( "Load_commands: # not found.", 0 );
	break;
      }

      word = fread_word( fp );
      if( !str_cmp( word, "COMMAND" ) )
      {
	fread_command( fp );
	continue;
      }
      else if( !str_cmp( word, "END" ) )
	break;
      else
      {
	bug( "Load_commands: bad section.", 0 );
	continue;
      }
    }
    fclose( fp );
  }
  else
  {
    bug( "Cannot open commands.dat", 0 );
    exit( 0 );
  }
}

