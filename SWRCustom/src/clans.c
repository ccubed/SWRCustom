#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "mud.h"

CLAN_DATA *first_clan = NULL;
CLAN_DATA *last_clan = NULL;

/* local routines */
static void fread_clan( CLAN_DATA * clan, FILE * fp );
static bool load_clan_file( const char *clanfile );
static void write_clan_list( void );
static size_t clan_permission_table_size( void );
static const char *get_clan_permission_name( int id );
static const char *get_clan_permission_description( int id );
static int get_clan_permission_id( const char *name );
static void clan_toggle_permission( const CLAN_DATA *clan, CHAR_DATA *ch, int id );
static void clan_clear_permissions( const CLAN_DATA *clan, CHAR_DATA *ch );
static const char *get_clan_permission_string( const CHAR_DATA *ch );
static void clan_add_leader( CLAN_DATA *clan, const char *name );

/*
 * Get pointer to clan structure from clan name.
 */
CLAN_DATA *get_clan( const char *name )
{
  CLAN_DATA *clan = NULL;

  for( clan = first_clan; clan; clan = clan->next )
    if( !str_cmp( name, clan->name ) )
      return clan;

  for( clan = first_clan; clan; clan = clan->next )
    if( !str_prefix( name, clan->name ) )
      return clan;

  for( clan = first_clan; clan; clan = clan->next )
    if( nifty_is_name( name, clan->name ) )
      return clan;

  for( clan = first_clan; clan; clan = clan->next )
    if( nifty_is_name_prefix( name, clan->name ) )
      return clan;

  return NULL;
}

void write_clan_list()
{
  const CLAN_DATA *tclan = NULL;
  FILE *fpout = NULL;
  char filename[256];

  sprintf( filename, "%s%s", CLAN_DIR, CLAN_LIST );
  fpout = fopen( filename, "w" );

  if( !fpout )
  {
    bug( "FATAL: cannot open clan.lst for writing!\r\n", 0 );
    return;
  }

  for( tclan = first_clan; tclan; tclan = tclan->next )
    fprintf( fpout, "%s\n", tclan->filename );

  fprintf( fpout, "$\n" );
  fclose( fpout );
}

/*
 * Save a clan's data to its data file
 */
void save_clan( const CLAN_DATA * clan )
{
  FILE *fp = NULL;
  char filename[256];
  char buf[MAX_STRING_LENGTH];

  if( !clan )
  {
    bug( "save_clan: null clan pointer!", 0 );
    return;
  }

  if( !clan->filename || clan->filename[0] == '\0' )
  {
    sprintf( buf, "save_clan: %s has no filename", clan->name );
    bug( buf, 0 );
    return;
  }

  sprintf( filename, "%s%s", CLAN_DIR, clan->filename );

  if( ( fp = fopen( filename, "w" ) ) == NULL )
  {
    bug( "save_clan: fopen", 0 );
    perror( filename );
  }
  else
  {
    fprintf( fp, "#CLAN\n" );
    fprintf( fp, "Name         %s~\n", clan->name );
    fprintf( fp, "Filename     %s~\n", clan->filename );
    fprintf( fp, "Description  %s~\n", clan->description );
    fprintf( fp, "Leaders      %s~\n", clan->leaders );
    fprintf( fp, "Atwar        %s~\n", clan->atwar );
    fprintf( fp, "Members      %d\n",  clan->members );
    fprintf( fp, "Funds        %ld\n", clan->funds );
    fprintf( fp, "Salary       %d\n",  clan->salary );
    fprintf( fp, "End\n\n" );
    fprintf( fp, "#END\n" );
  }

  fclose( fp );
}

/*
 * Read in actual clan data.
 */

void fread_clan( CLAN_DATA * clan, FILE * fp )
{
  char buf[MAX_STRING_LENGTH];

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
	  KEY( "Atwar", clan->atwar, fread_string( fp ) );
	  break;

	case 'D':
	  KEY( "Description", clan->description, fread_string( fp ) );
	  break;

	case 'E':
	  if( !str_cmp( word, "End" ) )
	    {
	      if( !clan->name )
		clan->name = STRALLOC( "" );
	      if( !clan->leaders )
		clan->leaders = STRALLOC( "" );
	      if( !clan->atwar )
		clan->atwar = STRALLOC( "" );
	      if( !clan->description )
		clan->description = STRALLOC( "" );
	      return;
	    }
	  break;

	case 'F':
	  KEY( "Funds", clan->funds, fread_number( fp ) );
	  KEY( "Filename", clan->filename, fread_string_nohash( fp ) );
	  break;

	case 'L':
	  KEY( "Leaders", clan->leaders, fread_string( fp ) );
	  break;

	case 'M':
	  KEY( "Members", clan->members, fread_number( fp ) );
	  break;

	case 'N':
	  KEY( "Name", clan->name, fread_string( fp ) );
	  break;

	case 'S':
	  KEY( "Salary", clan->salary, fread_number( fp ) );
	  break;
	}

      if( !fMatch )
	{
	  sprintf( buf, "Fread_clan: no match: %s", word );
	  bug( buf, 0 );
	}
    }
}

/*
 * Load a clan file
 */
bool load_clan_file( const char *clanfile )
{
  char filename[256];
  CLAN_DATA *clan = NULL;
  FILE *fp = NULL;
  bool found = FALSE;

  CREATE( clan, CLAN_DATA, 1 );

  sprintf( filename, "%s%s", CLAN_DIR, clanfile );

  if( ( fp = fopen( filename, "r" ) ) != NULL )
  {
    found = TRUE;

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
	bug( "Load_clan_file: # not found.", 0 );
	break;
      }

      word = fread_word( fp );
      if( !str_cmp( word, "CLAN" ) )
      {
	fread_clan( clan, fp );
	break;
      }
      else if( !str_cmp( word, "END" ) )
	break;
      else
      {
	char buf[MAX_STRING_LENGTH];

	sprintf( buf, "Load_clan_file: bad section: %s.", word );
	bug( buf, 0 );
	break;
      }
    }
    fclose( fp );
  }

  if( found )
  {
    LINK( clan, first_clan, last_clan, next, prev );
    return found;
  }
  else
    DISPOSE( clan );

  return found;
}

/*
 * Load in all the clan files.
 */
void load_clans()
{
  FILE *fpList = NULL;
  char clanlist[256];

  sprintf( clanlist, "%s%s", CLAN_DIR, CLAN_LIST );

  if( ( fpList = fopen( clanlist, "r" ) ) == NULL )
  {
    perror( clanlist );
    exit( 1 );
  }

  for( ;; )
  {
    const char *filename = feof( fpList ) ? "$" : fread_word( fpList );

    if( filename[0] == '$' )
      break;

    if( !load_clan_file( filename ) )
    {
      bug( "Cannot load clan file: %s", filename );
    }
  }

  fclose( fpList );
}

void do_induct( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim = NULL;
  CLAN_DATA *clan = NULL;

  if( IS_NPC( ch ) || !ch->pcdata->clan )
  {
    send_to_char( "Huh?\r\n", ch );
    return;
  }

  clan = ch->pcdata->clan;

  if( !clan_char_has_permission( clan, ch, CLAN_PERM_INDUCT ) )
    {
      ch_printf( ch, "You don't have permission to %s.\r\n",
		 get_clan_permission_description( CLAN_PERM_INDUCT ) );
      return;
    }

  argument = one_argument( argument, arg );

  if( arg[0] == '\0' )
  {
    send_to_char( "Induct whom?\r\n", ch );
    return;
  }

  if( ( victim = get_char_room( ch, arg ) ) == NULL )
  {
    send_to_char( "That player is not here.\r\n", ch );
    return;
  }

  if( IS_NPC( victim ) )
  {
    send_to_char( "Not on NPC's.\r\n", ch );
    return;
  }

  if( victim->pcdata->clan )
  {
    if( victim->pcdata->clan == clan )
      send_to_char( "This player already belongs to your organization!\r\n",
	  ch );
    else
      send_to_char( "This player already belongs to an organization!\r\n",
	  ch );
    return;
  }

  clan->members++;

  victim->pcdata->clan = clan;

  act( AT_MAGIC, "You induct $N into $t", ch, clan->name, victim, TO_CHAR );
  act( AT_MAGIC, "$n inducts $N into $t", ch, clan->name, victim,
      TO_NOTVICT );
  act( AT_MAGIC, "$n inducts you into $t", ch, clan->name, victim, TO_VICT );
  save_char_obj( victim );
}

void do_outcast( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim = NULL;
  CLAN_DATA *clan = NULL;
  char buf[MAX_STRING_LENGTH];

  if( IS_NPC( ch ) || !ch->pcdata->clan )
  {
    send_to_char( "Huh?\r\n", ch );
    return;
  }

  clan = ch->pcdata->clan;

  if( !clan_char_has_permission( clan, ch, CLAN_PERM_OUTCAST ) )
    {
      ch_printf( ch, "You don't have permission to %s.\r\n",
                 get_clan_permission_description( CLAN_PERM_OUTCAST ) );
      return;
    }

  argument = one_argument( argument, arg );

  if( arg[0] == '\0' )
  {
    send_to_char( "Outcast whom?\r\n", ch );
    return;
  }

  if( ( victim = get_char_room( ch, arg ) ) == NULL )
  {
    send_to_char( "That player is not here.\r\n", ch );
    return;
  }

  if( IS_NPC( victim ) )
  {
    send_to_char( "Not on NPC's.\r\n", ch );
    return;
  }

  if( victim == ch )
  {
    send_to_char( "Kick yourself out of your own clan?\r\n", ch );
    return;
  }

  if( victim->pcdata->clan != ch->pcdata->clan )
  {
    send_to_char( "This player does not belong to your clan!\r\n", ch );
    return;
  }

  if( clan_char_is_leader( clan, victim ) )
  {
    send_to_char( "You are not able to outcast them.\r\n", ch );
    return;
  }

  --clan->members;
  victim->pcdata->clan = NULL;

  act( AT_MAGIC, "You outcast $N from $t", ch, clan->name, victim, TO_CHAR );
  act( AT_MAGIC, "$n outcasts $N from $t", ch, clan->name, victim, TO_ROOM );
  act( AT_MAGIC, "$n outcasts you from $t", ch, clan->name, victim, TO_VICT );
  sprintf( buf, "%s has been outcast from %s!", victim->name, clan->name );
  echo_to_all( AT_MAGIC, buf, ECHOTAR_ALL );

  clan_clear_permissions( clan, victim );

  save_char_obj( victim );  /* clan gets saved when pfile is saved */
  save_clan( clan );        /* Nope, it doesn't because pcdata->clan is NULL */
}

void do_setclan( CHAR_DATA * ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  CLAN_DATA *clan = NULL;

  if( IS_NPC( ch ) || !ch->pcdata )
    return;

  if( !ch->desc )
    return;

  switch ( ch->substate )
  {
    default:
      break;
    case SUB_CLANDESC:
      clan = ( CLAN_DATA * ) ch->dest_buf;
      if( !clan )
      {
	bug( "setclan: sub_clandesc: NULL ch->dest_buf", 0 );
	stop_editing( ch );
	ch->substate = ch->tempnum;
	send_to_char( "&RError: clan lost.\r\n", ch );
	return;
      }
      STRFREE( clan->description );
      clan->description = copy_buffer( ch );
      stop_editing( ch );
      ch->substate = ch->tempnum;
      save_clan( clan );
      return;
  }

  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );

  if( arg1[0] == '\0' )
  {
    send_to_char( "Usage: setclan <clan> <field> <player>\r\n", ch );
    send_to_char( "\r\nField being one of:\r\n", ch );
    send_to_char( "members funds addleader rmleader\r\n", ch );
    send_to_char( "name filename desc atwar\r\n", ch );
    return;
  }

  clan = get_clan( arg1 );
  if( !clan )
  {
    send_to_char( "No such clan.\r\n", ch );
    return;
  }

  if( !strcmp( arg2, "members" ) )
  {
    clan->members = atoi( argument );
    send_to_char( "Done.\r\n", ch );
    save_clan( clan );
    return;
  }

  if( !strcmp( arg2, "funds" ) )
  {
    clan->funds = atoi( argument );
    send_to_char( "Done.\r\n", ch );
    save_clan( clan );
    return;
  }


  if( !strcmp( arg2, "addleader" ) )
  {
    clan_add_leader( clan, argument );
    send_to_char( "Done.\r\n", ch );
    save_clan( clan );
    return;
  }

  if( !str_cmp( arg2, "rmleader" ) )
  {
    clan_remove_leader( clan, argument );
    send_to_char( "Done.\r\n", ch );
    save_clan( clan );
    return;
  }

  if( !strcmp( arg2, "atwar" ) )
  {
    STRFREE( clan->atwar );
    clan->atwar = STRALLOC( argument );
    send_to_char( "Done.\r\n", ch );
    save_clan( clan );
    return;
  }


  if( !strcmp( arg2, "name" ) )
  {
    if( !argument || argument[0] == '\0' )
    {
      send_to_char( "You can't name a clan nothing.\r\n", ch );
      return;
    }

    if( get_clan( argument ) )
    {
      send_to_char( "There is already another clan with that name.\r\n",
	  ch );
      return;
    }

    STRFREE( clan->name );
    clan->name = STRALLOC( argument );
    send_to_char( "Done.\r\n", ch );
    save_clan( clan );
    return;
  }

  if( !strcmp( arg2, "filename" ) )
  {
    char filename[256];

    if( !is_valid_filename( ch, CLAN_DIR, argument ) )
      return;

    sprintf( filename, "%s%s", CLAN_DIR, clan->filename );

    if( remove( filename ) )
      send_to_char( "Old clan file deleted.\r\n", ch );

    DISPOSE( clan->filename );
    clan->filename = str_dup( argument );
    send_to_char( "Done.\r\n", ch );
    save_clan( clan );
    write_clan_list();
    return;
  }

  if( !str_cmp( arg2, "desc" ) )
  {
    ch->substate = SUB_CLANDESC;
    ch->dest_buf = clan;
    start_editing( ch, clan->description );
    return;
  }

  do_setclan( ch, STRLIT_EMPTY );
}

void do_showclan( CHAR_DATA * ch, char *argument )
{
  CLAN_DATA *clan = NULL;
  PLANET_DATA *planet = NULL;
  int pCount = 0;
  int support = 0;
  long revenue = 0;

  if( IS_NPC( ch ) )
  {
    send_to_char( "Huh?\r\n", ch );
    return;
  }

  if( argument[0] == '\0' )
  {
    send_to_char( "Usage: showclan <clan>\r\n", ch );
    return;
  }

  clan = get_clan( argument );

  if( !clan )
  {
    send_to_char( "No such clan.\r\n", ch );
    return;
  }

  for( planet = first_planet; planet; planet = planet->next )
    if( clan == planet->governed_by )
    {
      support += ( int ) planet->pop_support;
      pCount++;
      revenue += get_taxes( planet );
    }

  if( pCount > 1 )
    support /= pCount;


  ch_printf( ch, "&W%s      %s\r\n",
      clan->name, IS_IMMORTAL( ch ) ? clan->filename : "" );
  ch_printf( ch,
      "&WDescription:&G\r\n%s\r\n&WLeaders: &G%s\r\n&WAt War With: &G%s\r\n",
      clan->description, clan->leaders, clan->atwar );
  ch_printf( ch, "&WMembers: &G%d\r\n", clan->members );
  ch_printf( ch, "&WSpacecraft: &G%d\r\n", clan->spacecraft );
  ch_printf( ch, "&WVehicles: &G%d\r\n", clan->vehicles );
  ch_printf( ch, "&WPlanets Controlled: &G%d\r\n", pCount );
  ch_printf( ch, "&WAverage Popular Support: &G%d\r\n", support );
  ch_printf( ch, "&WMonthly Revenue: &G%ld\r\n", revenue );
  ch_printf( ch, "&WHourly Wages: &G%d\r\n", clan->salary );
  ch_printf( ch, "&WTotal Funds: &G%ld\r\n", clan->funds );
}

void do_makeclan( CHAR_DATA * ch, char *argument )
{
  char filename[256];
  CLAN_DATA *clan = NULL;

  if( !argument || argument[0] == '\0' )
  {
    send_to_char( "Usage: makeclan <clan name>\r\n", ch );
    return;
  }

  if( get_clan( argument ) )
    {
      ch_printf( ch, "&RThere is already an organization with that name.&w\r\n" );
      return;
    }

  sprintf( filename, "%s", strlower( argument ) );
  replace_char( filename, ' ', '_' );

  if( !is_valid_filename( ch, CLAN_DIR, filename ) )
    return;

  CREATE( clan, CLAN_DATA, 1 );
  LINK( clan, first_clan, last_clan, next, prev );
  clan->name = STRALLOC( argument );
  clan->filename = STRALLOC( filename );
  clan->description = STRALLOC( "" );
  clan->leaders = STRALLOC( "" );
  clan->atwar = STRALLOC( "" );
  clan->funds = 0;
  clan->salary = 0;
  clan->members = 0;
  save_clan( clan );
  write_clan_list();
}

void do_clans( CHAR_DATA * ch, char *argument )
{
  CLAN_DATA *clan = NULL;
  PLANET_DATA *planet = NULL;
  int count = 0;

  ch_printf( ch,
      "&WOrganization                                       Planets   Score\r\n" );
  for( clan = first_clan; clan; clan = clan->next )
  {
    int pCount = 0;
    int revenue = 0;

    for( planet = first_planet; planet; planet = planet->next )
      if( clan == planet->governed_by )
      {
	pCount++;
	revenue += get_taxes( planet ) / 720;
      }

    ch_printf( ch, "&Y%-50s %-3d       %d\r\n",
	clan->name, pCount, revenue );
    count++;
  }

  if( !count )
  {
    set_char_color( AT_BLOOD, ch );
    send_to_char( "There are no organizations currently formed.\r\n", ch );
  }

  set_char_color( AT_WHITE, ch );
  send_to_char( "\r\nFor more information use: SHOWCLAN\r\n", ch );
  send_to_char( "See also: PLANETS\r\n", ch );
}

void do_resign( CHAR_DATA * ch, char *argument )
{

  CLAN_DATA *clan;
  char buf[MAX_STRING_LENGTH];

  if( IS_NPC( ch ) || !ch->pcdata )
  {
    send_to_char( "You can't do that.\r\n", ch );
    return;
  }

  clan = ch->pcdata->clan;

  if( clan == NULL )
  {
    send_to_char
      ( "You have to join an organization before you can quit it.\r\n",
	ch );
    return;
  }

  if( clan_char_is_leader( ch->pcdata->clan, ch ) )
  {
    ch_printf( ch,
	"You can't resign from %s ... you are one of the leaders!\r\n",
	clan->name );
    return;
  }

  --clan->members;
  ch->pcdata->clan = NULL;

  act( AT_MAGIC, "You resign your position in $t", ch, clan->name, NULL,
      TO_CHAR );
  sprintf( buf, "%s has quit %s!", ch->name, clan->name );
  echo_to_all( AT_MAGIC, buf, ECHOTAR_ALL );

  clan_clear_permissions( clan, ch );

  save_char_obj( ch );
  save_clan( clan );
}

void do_clan_withdraw( CHAR_DATA * ch, char *argument )
{
  CLAN_DATA *clan = NULL;
  long amount = 0;

  if( IS_NPC( ch ) || !ch->pcdata->clan )
  {
    send_to_char
      ( "You don't seem to belong to an organization to withdraw funds from...\r\n",
	ch );
    return;
  }

  if( !ch->in_room || !IS_SET( ch->in_room->room_flags, ROOM_BANK ) )
  {
    send_to_char( "You must be in a bank to do that!\r\n", ch );
    return;
  }

  clan = ch->pcdata->clan;

  if( !clan_char_has_permission( clan, ch, CLAN_PERM_WITHDRAW ) )
    {
      ch_printf( ch, "You don't have permission to %s.\r\n",
                 get_clan_permission_description( CLAN_PERM_WITHDRAW ) );
      return;
    }

  clan = ch->pcdata->clan;

  amount = atoi( argument );

  if( !amount )
  {
    send_to_char( "How much would you like to withdraw?\r\n", ch );
    return;
  }

  if( amount > clan->funds )
  {
    ch_printf( ch, "%s doesn't have that much!\r\n", clan->name );
    return;
  }

  if( amount < 0 )
  {
    ch_printf( ch, "Nice try...\r\n" );
    return;
  }

  ch_printf( ch, "You withdraw %ld credits from %s's funds.\r\n", amount,
      clan->name );

  clan->funds -= amount;
  ch->gold += amount;
  save_char_obj( ch );
  save_clan( clan );
}

void do_clan_donate( CHAR_DATA * ch, char *argument )
{
  CLAN_DATA *clan;
  long amount;

  if( IS_NPC( ch ) || !ch->pcdata->clan )
  {
    send_to_char
      ( "You don't seem to belong to an organization to donate to...\r\n",
	ch );
    return;
  }

  if( !ch->in_room || !IS_SET( ch->in_room->room_flags, ROOM_BANK ) )
  {
    send_to_char( "You must be in a bank to do that!\r\n", ch );
    return;
  }

  clan = ch->pcdata->clan;

  amount = atoi( argument );

  if( !amount )
  {
    send_to_char( "How much would you like to donate?\r\n", ch );
    return;
  }

  if( amount < 0 )
  {
    ch_printf( ch, "Nice try...\r\n" );
    return;
  }

  if( amount > ch->gold )
  {
    send_to_char( "You don't have that much!\r\n", ch );
    return;
  }

  ch_printf( ch, "You donate %ld credits to %s's funds.\r\n", amount,
      clan->name );

  clan->funds += amount;
  ch->gold -= amount;
  save_clan( clan );
  save_char_obj( ch );
}


void do_appoint( CHAR_DATA * ch, char *argument )
{
  char name[MAX_INPUT_LENGTH];
  char fname[MAX_STRING_LENGTH];
  struct stat fst;

  if( IS_NPC( ch ) || !ch->pcdata )
    return;

  if( !ch->pcdata->clan )
  {
    send_to_char( "Huh?\r\n", ch );
    return;
  }

  if( !clan_char_is_leader( ch->pcdata->clan, ch ) )
  {
    send_to_char( "Only your leaders can do that!\r\n", ch );
    return;
  }

  if( argument[0] == '\0' )
  {
    send_to_char( "Useage: appoint name\r\n", ch );
    return;
  }

  one_argument( argument, name );

  name[0] = UPPER( name[0] );

  sprintf( fname, "%s%c/%s", PLAYER_DIR, tolower( ( int ) name[0] ),
      capitalize( name ) );

  if( stat( fname, &fst ) == -1 )
  {
    send_to_char( "No such player...\r\n", ch );
    return;
  }

  clan_add_leader( ch->pcdata->clan, name );
  save_clan( ch->pcdata->clan );

}

void do_demote( CHAR_DATA * ch, char *argument )
{

}

void do_empower( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  CHAR_DATA *victim = NULL;
  CLAN_DATA *clan = NULL;
  int clan_perm_id = 0;

  if( IS_NPC( ch ) || !ch->pcdata->clan )
  {
    send_to_char( "Huh?\r\n", ch );
    return;
  }

  clan = ch->pcdata->clan;

  if( !clan_char_has_permission( clan, ch, CLAN_PERM_EMPOWER ) )
    {
      ch_printf( ch, "You don't have permission to %s.\r\n",
                 get_clan_permission_description( CLAN_PERM_EMPOWER ) );
      return;
    }

  argument = one_argument( argument, arg );
  argument = one_argument( argument, arg2 );

  if( arg[0] == '\0' )
  {
    send_to_char( "Empower whom to do what?\r\n", ch );
    return;
  }

  if( ( victim = get_char_room( ch, arg ) ) == NULL )
  {
    send_to_char( "That player is not here.\r\n", ch );
    return;
  }

  if( IS_NPC( victim ) )
  {
    send_to_char( "Not on NPC's.\r\n", ch );
    return;
  }

  if( victim == ch && !IS_IMMORTAL( ch ) )
    {
      send_to_char( "Nice try.\r\n", ch );
      return;
    }

  if( victim->pcdata->clan != ch->pcdata->clan )
  {
    send_to_char( "This player does not belong to your clan!\r\n", ch );
    return;
  }

  clan_perm_id = get_clan_permission_id( arg2 );

  if( clan_perm_id == CLAN_PERM_INVALID
      && str_cmp( arg2, "list" ) && str_cmp( arg2, "none" ) )
    {
      size_t n = 0;
      send_to_char
	( "Currently you may empower members with only the following:\r\n",
	  ch );

      for( n = 0; n < clan_permission_table_size(); ++n )
	{
	  ch_printf( ch, "%-12s - ability to %s\r\n",
		     get_clan_permission_name( n ),
		     get_clan_permission_description( n ) );
	}

      ch_printf( ch, "\r\nAdditional options:\r\n" );
      send_to_char( "none         - removes bestowed abilities\r\n", ch );
      send_to_char( "list         - shows bestowed abilities\r\n", ch );
      return;
    }

  if( !str_cmp( arg2, "list" ) )
  {
    ch_printf( ch, "Current bestowed commands on %s: %s.\r\n",
	       victim->name, get_clan_permission_string( victim ) );
    return;
  }

  if( !str_cmp( arg2, "none" ) && clan_char_is_leader( clan, ch ) )
    {
      clan_clear_permissions( clan, victim );
      ch_printf( ch, "Permissions removed from %s.\r\n", victim->name );
      ch_printf( victim, "%s has removed your bestowed clan abilities.\r\n",
		 ch->name );
      save_char_obj( victim );
      return;
    }

  if( !clan_char_has_permission( clan, ch, clan_perm_id ) )
    {
      send_to_char
	( "&RI don't think you're even allowed to do that.&W\r\n", ch );
      return;
    }

  clan_toggle_permission( clan, victim, clan_perm_id );

  if( clan_char_has_permission( clan, victim, clan_perm_id ) )
    {
      ch_printf( victim, "%s has given you permission to %s.\r\n",
		 ch->name, get_clan_permission_description( clan_perm_id ) );
      ch_printf( ch, "Ok, they now have the ability to %s.\r\n",
		 get_clan_permission_description( clan_perm_id ) );
    }
  else
    {
      ch_printf( victim, "Your permission to %s was just revoked.\r\n",
		 get_clan_permission_description( clan_perm_id ) );
      ch_printf( ch, "Ok, they no longer have the ability to %s.\r\n",
                 get_clan_permission_description( clan_perm_id ) );
    }

  save_char_obj( victim );
}

void do_overthrow( CHAR_DATA * ch, char *argument )
{
  if( IS_NPC( ch ) )
    return;

  if( !ch->pcdata || !ch->pcdata->clan )
  {
    send_to_char
      ( "You have to be part of an organization before you can claim leadership.\r\n",
	ch );
    return;
  }

  if( ch->pcdata->clan->leaders && ch->pcdata->clan->leaders[0] != '\0' )
  {
    send_to_char( "Your organization already has strong leadership...\r\n",
	ch );
    return;
  }

  ch_printf( ch, "OK. You are now a leader of %s.\r\n",
      ch->pcdata->clan->name );

  clan_add_leader( ch->pcdata->clan, ch->name );
  save_char_obj( ch );
}

void do_war( CHAR_DATA * ch, char *argument )
{
  CLAN_DATA *wclan;
  CLAN_DATA *clan;
  char buf[MAX_STRING_LENGTH];

  if( IS_NPC( ch ) || !ch->pcdata || !ch->pcdata->clan )
  {
    send_to_char( "Huh?\r\n", ch );
    return;
  }

  clan = ch->pcdata->clan;

  if( !clan_char_has_permission( clan, ch, CLAN_PERM_WAR ) )
    {
      ch_printf( ch, "You don't have permission to %s.\r\n",
                 get_clan_permission_description( CLAN_PERM_WAR ) );
      return;
    }

  if( argument[0] == '\0' )
  {
    send_to_char( "Declare war on who?\r\n", ch );
    return;
  }

  if( ( wclan = get_clan( argument ) ) == NULL )
  {
    send_to_char( "No such clan.\r\n", ch );
    return;
  }

  if( wclan == clan )
  {
    send_to_char( "Declare war on yourself?!\r\n", ch );
    return;
  }

  if( nifty_is_name( wclan->name, clan->atwar ) )
  {
    CLAN_DATA *tclan;
    strcpy( buf, "" );

    for( tclan = first_clan; tclan; tclan = tclan->next )
      if( nifty_is_name( tclan->name, clan->atwar ) && tclan != wclan )
      {
	strcat( buf, "\r\n " );
	strcat( buf, tclan->name );
	strcat( buf, " " );
      }

    STRFREE( clan->atwar );
    clan->atwar = STRALLOC( buf );

    sprintf( buf, "%s has declared a ceasefire with %s!", clan->name,
	wclan->name );
    echo_to_all( AT_WHITE, buf, ECHOTAR_ALL );

    save_char_obj( ch );	/* clan gets saved when pfile is saved */

    return;
  }

  strcpy( buf, clan->atwar );
  strcat( buf, "\r\n " );
  strcat( buf, wclan->name );
  strcat( buf, " " );

  STRFREE( clan->atwar );
  clan->atwar = STRALLOC( buf );

  sprintf( buf, "%s has declared war on %s!", clan->name, wclan->name );
  echo_to_all( AT_RED, buf, ECHOTAR_ALL );

  save_char_obj( ch );		/* clan gets saved when pfile is saved */

}

void do_setwages( CHAR_DATA * ch, char *argument )
{
  CLAN_DATA *clan;

  if( IS_NPC( ch ) || !ch->pcdata || !ch->pcdata->clan )
  {
    send_to_char( "Huh?\r\n", ch );
    return;
  }

  clan = ch->pcdata->clan;

  if( !clan_char_has_permission( clan, ch, CLAN_PERM_PAYROLL ) )
  {
    send_to_char( "You clan hasn't empowered you to set wages!\r\n", ch );
    return;
  }

  if( argument[0] == '\0' )
  {
    send_to_char( "Set clan wages to what?\r\n", ch );
    return;
  }

  clan->salary = atoi( argument );

  ch_printf( ch, "Clan wages set to %d credits per hour\r\n", clan->salary );

  save_char_obj( ch );		/* clan gets saved when pfile is saved */
}

void clan_decrease_vehicles_owned( CLAN_DATA * clan, const SHIP_DATA * ship )
{
  if( ship->type != MOB_SHIP )
  {
    if( ship->ship_class <= SPACE_STATION )
    {
      clan->spacecraft--;
    }
    else
    {
      clan->vehicles--;
    }

    save_clan( clan );
  }
}

bool clan_char_is_leader( const CLAN_DATA * clan, const CHAR_DATA * ch )
{
  return nifty_is_name( ch->name, clan->leaders );
}

void clan_add_leader( CLAN_DATA * clan, const char *name )
{
  if( !nifty_is_name( name, clan->leaders ) )
  {
    char buf[MAX_STRING_LENGTH];
    sprintf( buf, "%s %s", clan->leaders, name );
    STRFREE( clan->leaders );
    clan->leaders = STRALLOC( buf );
    save_clan( clan );
  }
}

void clan_remove_leader( CLAN_DATA * clan, const char *name )
{
  char tc[MAX_STRING_LENGTH];
  char on[MAX_STRING_LENGTH];
  char *leadership = clan->leaders;

  strcpy( tc, "" );

  while( leadership[0] != '\0' )
  {
    leadership = one_argument( leadership, on );

    if( str_cmp( name, on )
	&& ( strlen( on ) + strlen( tc ) ) < ( MAX_STRING_LENGTH + 1 ) )
    {
      if( strlen( tc ) != 0 )
	strcat( tc, " " );

      strcat( tc, on );
    }
  }

  STRFREE( clan->leaders );
  clan->leaders = STRALLOC( tc );
  save_clan( clan );
}

/*
 * Clan permission code.
 */

typedef struct clan_permission
{
  const char *name;
  const char *description;
} CLAN_PERMISSION;

static const CLAN_PERMISSION clan_permission_table[] =
  {
    { "pilot",       "fly clan ships"         }, /* CLAN_PERM_PILOT */
    { "withdraw",    "withdraw clan funds"    }, /* CLAN_PERM_WITHDRAW */
    { "clanbuyship", "buy clan ships"         }, /* CLAN_PERM_BUYSHIP */
    { "induct",      "induct new members"     }, /* CLAN_PERM_INDUCT */
    { "outcast",     "outcast members"        }, /* CLAN_PERM_OUCAST */
    { "empower",     "empower others"         }, /* CLAN_PERM_EMPOWER */
    { "build",       "build and modify areas" }, /* CLAN_PERM_BUILD */
    { "war",         "declare war"            }, /* CLAN_PERM_WAR */
    { "payroll",     "set salaries"           }  /* CLAN_PERM_PAYROLL */
  };

size_t clan_permission_table_size( void )
{
  return sizeof( clan_permission_table ) / sizeof( *clan_permission_table );
}

const CLAN_PERMISSION *get_clan_permission( int id )
{
  if( id >= 0 && id < (int) clan_permission_table_size() )
    return &clan_permission_table[id];
  else
    return NULL;
}

bool clan_char_has_permission( const CLAN_DATA *clan,
			       const CHAR_DATA *ch, int id )
{
  if( ch->pcdata && ch->pcdata->clan && ch->pcdata->clan == clan )
    {
      const CLAN_PERMISSION *perm = get_clan_permission( id );

      if( perm )
	{
	  if( is_name( perm->name, ch->pcdata->clan_permissions )
	      || clan_char_is_leader( clan, ch ) )
	    {
	      return TRUE;
	    }
	}
    }

  return FALSE;
}

const char *get_clan_permission_name( int id )
{
  const CLAN_PERMISSION *perm = get_clan_permission( id );

  if( !perm )
    return "*out of bounds*";

  return perm->name;
}

const char *get_clan_permission_description( int id )
{
  const CLAN_PERMISSION *perm = get_clan_permission( id );

  if( !perm )
    return "*out of bounds*";

  return perm->description;
}

int get_clan_permission_id( const char *name )
{
  size_t n = 0;

  for( n = 0; n < clan_permission_table_size(); ++n )
    {
      if( !str_cmp( name, get_clan_permission_name( n ) ) )
	{
	  return n;
	}
    }

  return -1;
}

void clan_toggle_permission( const CLAN_DATA *clan, CHAR_DATA *ch, int id )
{
  if( !clan_char_has_permission( clan, ch, id ) )
    {
      char buf[MAX_STRING_LENGTH];
      snprintf( buf, MAX_STRING_LENGTH, "%s %s", ch->pcdata->clan_permissions,
		get_clan_permission_name( id ) );
      DISPOSE( ch->pcdata->clan_permissions );
      ch->pcdata->clan_permissions = str_dup( buf );
    }
  else
    {
      char tc[MAX_STRING_LENGTH];
      char on[MAX_STRING_LENGTH];
      char name[MAX_STRING_LENGTH];
      char *perms = ch->pcdata->clan_permissions;

      strcpy( tc, "" );
      snprintf( name, MAX_STRING_LENGTH, "%s", get_clan_permission_name( id ));

      while( perms[0] != '\0' )
	{
	  perms = one_argument( name, on );

	  if( str_cmp( name, on )
	      && ( strlen( on ) + strlen( tc ) ) < ( MAX_STRING_LENGTH + 1 ) )
	    {
	      if( strlen( tc ) != 0 )
		strcat( tc, " " );

	      strcat( tc, on );
	    }
	}

      DISPOSE( ch->pcdata->clan_permissions );
      ch->pcdata->clan_permissions = str_dup( tc );
      save_char_obj( ch );
    }
}

void clan_clear_permissions( const CLAN_DATA *clan, CHAR_DATA *ch )
{
  if( ch->pcdata && ch->pcdata->clan == clan )
    {
      DISPOSE( ch->pcdata->clan_permissions );
      ch->pcdata->clan_permissions = str_dup( "" );
    }
}

const char *get_clan_permission_string( const CHAR_DATA *ch )
{
  return ch->pcdata ? ch->pcdata->clan_permissions : "";
}
