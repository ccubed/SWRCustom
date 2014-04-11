#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "mud.h"
#include "vector3_aux.h"

extern int top_area;
extern int top_r_vnum;

PLANET_DATA *first_planet = NULL;
PLANET_DATA *last_planet = NULL;

GUARD_DATA *first_guard = NULL;
GUARD_DATA *last_guard = NULL;

void fread_planet( PLANET_DATA * planet, FILE * fp );
bool load_planet_file( const char *planetfile );
void write_planet_list( void );

static PLANET_DATA *planet_create( void );

PLANET_DATA *get_planet( const char *name )
{
  PLANET_DATA *planet;

  if( name[0] == '\0' )
    return NULL;

  for( planet = first_planet; planet; planet = planet->next )
    if( !str_cmp( name, planet->name ) )
      return planet;

  for( planet = first_planet; planet; planet = planet->next )
    if( nifty_is_name( name, planet->name ) )
      return planet;

  for( planet = first_planet; planet; planet = planet->next )
    if( !str_prefix( name, planet->name ) )
      return planet;

  for( planet = first_planet; planet; planet = planet->next )
    if( nifty_is_name_prefix( name, planet->name ) )
      return planet;

  return NULL;
}

void write_planet_list()
{
  PLANET_DATA *tplanet;
  FILE *fpout;
  char filename[256];

  snprintf( filename, 256, "%s%s", PLANET_DIR, PLANET_LIST );
  fpout = fopen( filename, "w" );
  if( !fpout )
  {
    bug( "FATAL: cannot open planet.lst for writing!\r\n", 0 );
    return;
  }
  for( tplanet = first_planet; tplanet; tplanet = tplanet->next )
    fprintf( fpout, "%s\n", tplanet->filename );
  fprintf( fpout, "$\n" );
  fclose( fpout );
}

void save_planet( const PLANET_DATA * planet )
{
  FILE *fp = NULL;
  char filename[256];

  if( !planet )
  {
    bug( "save_planet: null planet pointer!", 0 );
    return;
  }

  if( !planet->filename || planet->filename[0] == '\0' )
  {
    bug( "save_planet: %s has no filename", planet->name );
    return;
  }

  snprintf( filename, 256, "%s%s", PLANET_DIR, planet->filename );

  if( ( fp = fopen( filename, "w" ) ) == NULL )
  {
    bug( "save_planet: fopen", 0 );
    perror( filename );
  }
  else
  {
    AREA_DATA *pArea;

    fprintf( fp, "#PLANET\n" );
    fprintf( fp, "Name         %s~\n", planet->name );
    fprintf( fp, "Filename     %s~\n", planet->filename );
    fprintf( fp, "X            %.0f\n", planet->pos.x );
    fprintf( fp, "Y            %.0f\n", planet->pos.y );
    fprintf( fp, "Z            %.0f\n", planet->pos.z );
    fprintf( fp, "Sector       %d\n", planet->sector );
    fprintf( fp, "PopSupport   %.0f\n", planet->pop_support );

    if( planet->starsystem && planet->starsystem->name )
      fprintf( fp, "Starsystem   %s~\n", planet->starsystem->name );

    if( planet->governed_by && planet->governed_by->name )
      fprintf( fp, "GovernedBy   %s~\n", planet->governed_by->name );

    pArea = planet->area;

    if( pArea->filename )
      fprintf( fp, "Area         %s~\n", pArea->filename );

    fprintf( fp, "End\n\n" );
    fprintf( fp, "#END\n" );
  }
  fclose( fp );
}

void fread_planet( PLANET_DATA * planet, FILE * fp )
{
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
	if( !str_cmp( word, "Area" ) )
	{
	  char *aName = fread_string( fp );
	  AREA_DATA *pArea = NULL;

	  for( pArea = first_area; pArea; pArea = pArea->next )
	  {
	    if( pArea->filename && !str_cmp( pArea->filename, aName ) )
	    {
	      ROOM_INDEX_DATA *room;

	      pArea->planet = planet;
	      planet->area = pArea;
	      for( room = pArea->first_room; room;
		  room = room->next_in_area )
	      {
		planet->size++;
		if( room->sector_type <= SECT_CITY )
		  planet->citysize++;
		else if( room->sector_type == SECT_FARMLAND )
		  planet->farmland++;
		else if( room->sector_type != SECT_DUNNO )
		  planet->wilderness++;

		if( IS_SET( room->room_flags, ROOM_CONTROL ) )
		  planet->controls++;
		if( IS_SET( room->room_flags, ROOM_BARRACKS ) )
		  planet->barracks++;
	      }
	    }
	  }

	  STRFREE( aName );
	  fMatch = TRUE;
	}
	break;

      case 'E':
	if( !str_cmp( word, "End" ) )
	{
	  if( !planet->name )
	    planet->name = STRALLOC( "" );
	  return;
	}
	break;

      case 'F':
	KEY( "Filename", planet->filename, fread_string_nohash( fp ) );
	break;

      case 'G':
	if( !str_cmp( word, "GovernedBy" ) )
	{
	  char *clan_name = fread_string( fp );
	  planet->governed_by = get_clan( clan_name );
	  fMatch = TRUE;
	  STRFREE( clan_name );
	}
	break;

      case 'N':
	KEY( "Name", planet->name, fread_string( fp ) );
	break;

      case 'P':
	KEY( "PopSupport", planet->pop_support, fread_float( fp ) );
	break;

      case 'S':
	KEY( "Sector", planet->sector, fread_number( fp ) );
	if( !str_cmp( word, "Starsystem" ) )
	{
	  char *systemname = fread_string( fp );
	  planet->starsystem = starsystem_from_name( systemname );
	  STRFREE( systemname );

	  if( planet->starsystem )
	  {
	    SPACE_DATA *starsystem = planet->starsystem;

	    LINK( planet, starsystem->first_planet,
		starsystem->last_planet, next_in_system,
		prev_in_system );
	  }
	  fMatch = TRUE;
	}
	break;

      case 'X':
	KEY( "X", planet->pos.x, fread_number( fp ) );
	break;

      case 'Y':
	KEY( "Y", planet->pos.y, fread_number( fp ) );
	break;

      case 'Z':
	KEY( "Z", planet->pos.z, fread_number( fp ) );
	break;
    }

    if( !fMatch )
    {
      bug( "Fread_planet: no match: %s", word );
    }
  }
}

bool load_planet_file( const char *planetfile )
{
  char filename[256];
  FILE *fp;
  PLANET_DATA *planet = planet_create();
  bool found = FALSE;
  snprintf( filename, 256, "%s%s", PLANET_DIR, planetfile );

  if( ( fp = fopen( filename, "r" ) ) != NULL )
  {
    found = TRUE;
    for( ;; )
    {
      char letter;
      const char *word;

      letter = fread_letter( fp );
      if( letter == '*' )
      {
	fread_to_eol( fp );
	continue;
      }

      if( letter != '#' )
      {
	bug( "Load_planet_file: # not found.", 0 );
	break;
      }

      word = fread_word( fp );

      if( !str_cmp( word, "PLANET" ) )
	{
	  fread_planet( planet, fp );
	  break;
	}
      else if( !str_cmp( word, "END" ) )
	{
	  break;
	}
      else
	{
	  bug( "Load_planet_file: bad section: %s.", word );
	  break;
	}
    }
    fclose( fp );
  }

  if( !found )
    DISPOSE( planet );
  else
    LINK( planet, first_planet, last_planet, next, prev );

  return found;
}

void load_planets()
{
  FILE *fpList = NULL;
  char planetlist[256];

  first_planet = NULL;
  last_planet = NULL;

  snprintf( planetlist, 256, "%s%s", PLANET_DIR, PLANET_LIST );

  if( ( fpList = fopen( planetlist, "r" ) ) == NULL )
  {
    perror( planetlist );
    exit( 1 );
  }

  for( ;; )
  {
    const char *filename = feof( fpList ) ? "$" : fread_word( fpList );

    if( filename[0] == '$' )
      break;

    if( !load_planet_file( filename ) )
    {
      bug( "Cannot load planet file: %s", filename );
    }
  }
  fclose( fpList );
}

void do_setplanet( CHAR_DATA * ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  PLANET_DATA *planet;

  if( IS_NPC( ch ) )
  {
    send_to_char( "Huh?\r\n", ch );
    return;
  }

  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );

  if( arg1[0] == '\0' )
  {
    send_to_char( "Usage: setplanet <planet> <field> [value]\r\n", ch );
    send_to_char( "\r\nField being one of:\r\n", ch );
    send_to_char( " name filename starsystem governed_by\r\n", ch );
    return;
  }

  planet = get_planet( arg1 );
  if( !planet )
  {
    send_to_char( "No such planet.\r\n", ch );
    return;
  }


  if( !strcmp( arg2, "name" ) )
  {
    if( !argument || argument[0] == '\0' )
    {
      send_to_char( "You must choose a name.\r\n", ch );
      return;
    }

    if( get_planet( argument ) )
    {
      send_to_char( "A planet with that name already Exists!\r\n", ch );
      return;
    }

    STRFREE( planet->name );
    planet->name = STRALLOC( argument );
    send_to_char( "Done.\r\n", ch );
    save_planet( planet );
    return;
  }

  if( !strcmp( arg2, "sector" ) )
  {
    planet->sector = atoi( argument );
    send_to_char( "Done.\r\n", ch );
    save_planet( planet );
    return;
  }

  if( !strcmp( arg2, "governed_by" ) )
  {
    CLAN_DATA *clan;
    clan = get_clan( argument );
    if( clan )
    {
      planet->governed_by = clan;
      send_to_char( "Done.\r\n", ch );
      save_planet( planet );
    }
    else
      send_to_char( "No such clan.\r\n", ch );
    return;
  }

  if( !strcmp( arg2, "starsystem" ) )
  {
    SPACE_DATA *starsystem;

    if( ( starsystem = planet->starsystem ) != NULL )
      UNLINK( planet, starsystem->first_planet, starsystem->last_planet,
	  next_in_system, prev_in_system );
    if( ( planet->starsystem = starsystem_from_name( argument ) ) )
    {
      starsystem = planet->starsystem;
      LINK( planet, starsystem->first_planet, starsystem->last_planet,
	  next_in_system, prev_in_system );
      send_to_char( "Done.\r\n", ch );
    }
    else
      send_to_char( "No such starsystem.\r\n", ch );
    save_planet( planet );
    return;
  }

  if( !strcmp( arg2, "filename" ) )
  {
    char filename[256];

    if( !is_valid_filename( ch, PLANET_DIR, argument ) )
      return;

    snprintf( filename, 256, "%s%s", PLANET_DIR, planet->filename );
    remove( filename );
    DISPOSE( planet->filename );
    planet->filename = str_dup( argument );
    send_to_char( "Done.\r\n", ch );
    save_planet( planet );
    write_planet_list();
    return;
  }


  do_setplanet( ch, STRLIT_EMPTY );
}

void do_showplanet( CHAR_DATA * ch, char *argument )
{
  GUARD_DATA *guard = NULL;
  PLANET_DATA *planet = NULL;
  int num_guards = 0;
  int pf = 0;
  int pc = 0;
  int pw = 0;

  if( IS_NPC( ch ) )
  {
    send_to_char( "Huh?\r\n", ch );
    return;
  }

  if( argument[0] == '\0' )
  {
    send_to_char( "Usage: showplanet <planet>\r\n", ch );
    return;
  }

  planet = get_planet( argument );
  if( !planet )
  {
    send_to_char( "No such planet.\r\n", ch );
    return;
  }

  for( guard = planet->first_guard; guard; guard = guard->next_on_planet )
    num_guards++;

  if( planet->size > 0 )
  {
    float tempf = ( float ) planet->citysize;
    pc = ( int ) ( tempf / planet->size * 100 );

    tempf = ( float ) planet->wilderness;
    pw = ( int ) ( tempf / planet->size * 100 );

    tempf = ( float ) planet->farmland;
    pf = ( int ) ( tempf / planet->size * 100 );
  }

  ch_printf( ch, "&W%s\r\n", planet->name );
  if( IS_IMMORTAL( ch ) )
    ch_printf( ch, "&WFilename: &G%s\r\n", planet->filename );

  ch_printf( ch, "&WTerrain: &G%s\r\n", get_sector_name(planet->sector) );
  ch_printf( ch, "&WGoverned by: &G%s\r\n",
      planet->governed_by ? planet->governed_by->name : "" );
  ch_printf( ch, "&WPlanet Size: &G%d\r\n", planet->size );
  ch_printf( ch, "&WPercent Civilized: &G%d\r\n", pc );
  ch_printf( ch, "&WPercent Wilderness: &G%d\r\n", pw );
  ch_printf( ch, "&WPercent Farmland: &G%d\r\n", pf );
  ch_printf( ch, "&WBarracks: &G%d\r\n", planet->barracks );
  ch_printf( ch, "&WControl Towers: &G%d\r\n", planet->controls );
  ch_printf( ch, "&WPatrols: &G%d&W/%d\r\n", num_guards,
      planet->barracks * 5 );
  ch_printf( ch, "&WPopulation: &G%d&W/%d\r\n", planet->population,
      max_population( planet ) );
  ch_printf( ch, "&WPopular Support: &G%.2f\r\n", planet->pop_support );
  ch_printf( ch, "&WCurrent Monthly Revenue: &G%ld\r\n",
      get_taxes( planet ) );
  if( IS_IMMORTAL( ch ) && !planet->area )
  {
    ch_printf( ch,
	"&RWarning - this planet is not attached to an area!&G" );
    ch_printf( ch, "\r\n" );
  }

  return;
}

void do_makeplanet( CHAR_DATA * ch, char *argument )
{
  AREA_DATA *pArea = NULL;
  PLANET_DATA *planet = NULL;
  char arg1[MAX_STRING_LENGTH];
  char arg3[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char filename[256];
  char pname[MAX_STRING_LENGTH];
  char *description = NULL;
  bool destok = TRUE;
  int rnum = 0, sector = 0;
  ROOM_INDEX_DATA *location = NULL;
  SPACE_DATA *starsystem = NULL;

  switch ( ch->substate )
  {
    default:
      break;
    case SUB_ROOM_DESC:
      pArea = ( AREA_DATA * ) ch->dest_buf;
      if( !pArea )
      {
	bug( "makep: sub_room_desc: NULL ch->dest_buf", 0 );
	destok = FALSE;
      }
      planet = ( PLANET_DATA * ) ch->dest_buf_2;
      if( !planet )
      {
	bug( "make: sub_room_desc: NULL ch->dest_buf2", 0 );
	destok = FALSE;
      }
      description = copy_buffer( ch );
      stop_editing( ch );
      ch->substate = ch->tempnum;
      if( !destok )
	return;

      for( rnum = COLONY_ROOM_FIRST; rnum <= COLONY_ROOM_LAST; rnum++ )
      {
	location = make_room( ++top_r_vnum );
	planet->size++;
	location->area = pArea;
	STRFREE( location->description );
	STRFREE( location->name );

	switch( rnum )
	  {
	  case COLONY_ROOM_SUPPLY_SHOP:
	    make_default_colony_supply_shop( planet, location );
	    break;

	  case COLONY_ROOM_COLONIZATION_CENTER:
	    make_default_colony_center( planet, location );
	    break;

	  case COLONY_ROOM_SHUTTLE_PLATFORM:
	    make_default_colony_shuttle_platform( planet, location );
	    break;

	  case COLONY_ROOM_HOTEL:
	    make_default_colony_hotel( planet, location );
	    break;

	  default:
	    make_default_colony_wilderness( planet, location, description );
	    break;
	  }

	LINK( location, pArea->first_room, pArea->last_room, next_in_area,
	    prev_in_area );

	make_colony_room_exits( location, rnum );
      }

      save_planet( planet );
      snprintf( filename, 256, "%s%s", AREA_DIR, pArea->filename );
      fold_area( pArea, filename, FALSE );
      write_area_list();
      write_planet_list();
      snprintf( buf, 256, "%d", top_r_vnum - 17 );
      ch_printf( ch, "You may now travel to your new colony.\r\n" );
      reset_all();

      return;
  }

  if( IS_NPC( ch ) || !ch->pcdata )
    return;

  if( !ch->in_room || !IS_SET( ch->in_room->room_flags, ROOM_SHIPYARD ) )
  {
    send_to_char
      ( "Exploration probes can only be launched from a shipyard.\r\n",
	ch );
    return;
  }

  if( ch->gold < COLONY_COST )
  {
    ch_printf( ch,
	"It costs %d credits to launch an exploration probe.\r\n",
	COLONY_COST );
    return;
  }

  if( !argument || argument[0] == '\0' )
  {
    send_to_char( "Would you like to explore an existing star system or a new one?\r\n\r\n", ch );
    send_to_char( "Usage: explore <starsystem> <planet name>\r\n", ch );
    return;
  }

  argument = one_argument( argument, arg1 );

  starsystem = starsystem_from_name( arg1 );

  if( starsystem )
  {
    PLANET_DATA *tp = NULL;

    if( starsystem == starsystem_from_name( NEWBIE_STARSYSTEM ) )
    {
      ch_printf( ch, "You cannot explore in that system.\r\n" );
      return;
    }

    for( tp = starsystem->first_planet; tp; tp = tp->next_in_system )
      if( tp->governed_by &&
	  ( !ch->pcdata->clan || ch->pcdata->clan != tp->governed_by ) )
      {
	ch_printf( ch,
	    "You cannot explore in that system without permission from %s.\r\n",
	    tp->governed_by->name );
	return;
      }
  }

  if( !argument || argument[0] == '\0' )
  {
    send_to_char
      ( "What would you call the new planet if you found it?\r\n\r\n", ch );
    send_to_char( "Usage: explore <starsystem> <planet name>\r\n", ch );
    return;
  }

  sector = 0;
  while( sector == 0
	 || sector == SECT_WATER_SWIM
	 || sector == SECT_WATER_NOSWIM
	 || sector == SECT_UNDERWATER
	 || sector == SECT_FARMLAND
	 || sector == SECT_AIR
	 || sector == SECT_OCEANFLOOR
	 || sector == SECT_DUNNO )
    sector = number_range( SECT_FIELD, SECT_MAX - 1 );

  strncpy( pname, argument, MAX_STRING_LENGTH );
  argument = one_argument( argument, arg3 );

  snprintf( buf, MAX_STRING_LENGTH, "%s.planet", strlower( arg3 ) );
  replace_char( buf, ' ', '_' );

  if( !is_valid_filename( ch, PLANET_DIR, buf ) )
    {
      return;
    }

  for( planet = first_planet; planet; planet = planet->next )
  {
    if( !str_cmp( planet->filename, buf ) )
    {
      send_to_char( "A planet with that filename already exists.\r\n", ch );
      return;
    }
  }

  ch->gold -= COLONY_COST;

  ch_printf( ch, "You spend %d credits to launch an explorer probe.\r\n",
      COLONY_COST );

  echo_to_room( AT_WHITE, ch->in_room,
      "A small probe lifts off into space." );

  if( number_percent() < 20 )
    return;

  sector = 0;
  while( sector == 0 || sector == SECT_WATER_SWIM
      || sector == SECT_WATER_NOSWIM || sector == SECT_UNDERWATER
      || sector == SECT_FARMLAND )
    sector = number_range( SECT_FIELD, SECT_MAX - 1 );

  pArea = NULL;
  planet = NULL;

  planet = planet_create();
  LINK( planet, first_planet, last_planet, next, prev );
  planet->name = STRALLOC( pname );
  planet->sector = sector;
  vector_randomize( &planet->pos, -10000, 10000 );

  if( starsystem )
  {
    planet->starsystem = starsystem;
    starsystem = planet->starsystem;
    LINK( planet, starsystem->first_planet, starsystem->last_planet,
	next_in_system, prev_in_system );
  }
  else
  {
    starsystem = starsystem_create();
    LINK( starsystem, first_starsystem, last_starsystem, next, prev );
    starsystem->name = STRALLOC( arg1 );
    starsystem->star1 = STRALLOC( arg1 );
    starsystem->star2 = STRALLOC( "" );
    starsystem->first_planet = planet;
    starsystem->last_planet = planet;
    snprintf( filename, 256, "%s.system", strlower( arg1 ) );
    replace_char( filename, ' ', '_' );
    starsystem->filename = str_dup( filename );
    save_starsystem( starsystem );
    write_starsystem_list();
    planet->starsystem = starsystem;
  }

  CREATE( pArea, AREA_DATA, 1 );
  pArea->first_room = NULL;
  pArea->last_room = NULL;
  pArea->planet = NULL;
  pArea->planet = planet;
  pArea->name = STRALLOC( pname );

  planet->area = pArea;

  LINK( pArea, first_area, last_area, next, prev );
  top_area++;

  pArea->filename = str_dup( arg3 );
  snprintf( filename, 256, "%s.planet", strlower( arg3 ) );
  replace_char( filename, ' ', '-' );
  planet->filename = str_dup( filename );

  send_to_char( "\r\n&YYour probe has discovered a new planet.\r\n", ch );
  send_to_char( "The terrain appears to be mostly &W", ch );
  send_to_char( strlower( get_sector_name( sector ) ), ch );
  send_to_char( "&Y.\r\n", ch );
  send_to_char( "\r\nPlease enter a description of your planet.\r\n", ch );
  send_to_char( "It should be a short paragraph of 5 or more lines.\r\n",
      ch );
  send_to_char
    ( "It will be used as the planet's default room descriptions.\r\n\r\n",
      ch );

  description = STRALLOC( "" );

  /* save them now just in case... */

  save_planet( planet );
  snprintf( filename, 256, "%s%s", AREA_DIR, pArea->filename );
  fold_area( pArea, filename, FALSE );
  write_area_list();
  write_planet_list();

  ch->tempnum = SUB_NONE;
  ch->substate = SUB_ROOM_DESC;
  ch->dest_buf = ( void * ) pArea;
  ch->dest_buf_2 = ( void * ) planet;
  start_editing( ch, description );
}

void do_planets( CHAR_DATA * ch, char *argument )
{
  PLANET_DATA *planet = NULL;
  int count = 0;
  SPACE_DATA *starsystem = NULL;

  set_char_color( AT_WHITE, ch );
  send_to_char( "Planet          Starsystem    Governed By                  Popular Support\r\n", ch );

  for( starsystem = first_starsystem; starsystem;
      starsystem = starsystem->next )
    for( planet = starsystem->first_planet; planet;
	planet = planet->next_in_system )
    {
      ch_printf( ch, "&G%-15s %-12s  %-25s    ",
	  planet->name, starsystem->name,
	  planet->governed_by ? planet->governed_by->name : "" );
      ch_printf( ch, "%.1f\r\n", planet->pop_support );

      if( IS_IMMORTAL( ch ) && !planet->area )
      {
	ch_printf( ch,
	    "&RWarning - this planet is not attached to an area!&G" );
	ch_printf( ch, "\r\n" );
      }

      count++;
    }

  for( planet = first_planet; planet; planet = planet->next )
  {
    if( planet->starsystem )
      continue;

    ch_printf( ch, "&G%-15s %-12s  %-25s    ",
	planet->name, "",
	planet->governed_by ? planet->governed_by->name : "" );
    ch_printf( ch, "%.1f\r\n", planet->pop_support );

    if( IS_IMMORTAL( ch ) && !planet->area )
    {
      ch_printf( ch,
	  "&RWarning - this planet is not attached to an area!&G" );
      ch_printf( ch, "\r\n" );
    }

    count++;
  }

  if( !count )
  {
    set_char_color( AT_BLOOD, ch );
    send_to_char( "There are no planets currently formed.\r\n", ch );
  }

  send_to_char( "&WUse SHOWPLANET for more information.\r\n", ch );
}

void do_capture( CHAR_DATA * ch, char *argument )
{
  CLAN_DATA *clan = NULL;
  PLANET_DATA *planet = NULL;
  PLANET_DATA *cPlanet = NULL;
  float support = 0.0;
  int pCount = 0;
  char buf[MAX_STRING_LENGTH];

  if( !ch->in_room || !ch->in_room->area )
    return;

  if( IS_NPC( ch ) || !ch->pcdata )
  {
    send_to_char( "huh?\r\n", ch );
    return;
  }

  if( !ch->pcdata->clan )
  {
    send_to_char
      ( "You need to be a member of an organization to do that!\r\n", ch );
    return;
  }

  clan = ch->pcdata->clan;

  if( ( planet = ch->in_room->area->planet ) == NULL )
  {
    send_to_char( "You must be on a planet to capture it.\r\n", ch );
    return;
  }

  if( clan == planet->governed_by )
  {
    send_to_char( "Your organization already controls this planet.\r\n", ch );
    return;
  }

  if( planet->starsystem )
  {
    SHIP_DATA *ship = NULL;
    CLAN_DATA *sClan = NULL;

    for( ship = planet->starsystem->first_ship; ship;
	ship = ship->next_in_starsystem )
    {
      sClan = get_clan( ship->owner );
      if( !sClan )
	continue;
      if( sClan == planet->governed_by )
      {
	send_to_char
	  ( "A planet cannot be captured while protected by orbiting spacecraft.\r\n",
	    ch );
	return;
      }
    }
  }

  if( planet->first_guard )
  {
    send_to_char( "This planet is protected by soldiers.\r\n", ch );
    send_to_char
      ( "You will have to eliminate all enemy forces before you can capture it.\r\n",
	ch );
    return;
  }

  if( planet->pop_support > 0 )
  {
    send_to_char
      ( "The population is not in favour of changing leaders right now.\r\n",
	ch );
    return;
  }

  for( cPlanet = first_planet; cPlanet; cPlanet = cPlanet->next )
    if( clan == cPlanet->governed_by )
    {
      pCount++;
      support += cPlanet->pop_support;
    }

  if( support < 0 )
  {
    send_to_char
      ( "There is not enough popular support for your organization!\r\nTry improving loyalty on the planets that you already control.\r\n",
	ch );
    return;
  }

  planet->governed_by = clan;
  planet->pop_support = 50;

  snprintf( buf, MAX_STRING_LENGTH,
	    "%s has been captured by %s!", planet->name, clan->name );
  echo_to_all( AT_RED, buf, 0 );
  save_planet( planet );
}

const long TAX_PER_POPULATION = 500;
const long TAX_PER_RESOURCE = 10000;
const long TAX_PER_SUPPORT = 100;
const long TAX_PER_BARRACKS = 5000;
const long TAX_PER_CONTROL = 10000;
const long TAX_PER_BIGSHIP = 50000;

long get_taxes( const PLANET_DATA * planet )
{
  long bigships = 0;
  int resource = UMIN( planet->wilderness, planet->population );
  long gain = TAX_PER_POPULATION * planet->population;
  gain += TAX_PER_RESOURCE * resource;
  gain += ( long ) ( planet->pop_support * TAX_PER_SUPPORT );

  gain -= TAX_PER_BARRACKS * planet->barracks;
  gain -= TAX_PER_CONTROL * planet->controls;

  bigships = planet->controls / 5;	/* 100k for destroyers, 1 mil for battleships */
  gain -= TAX_PER_BIGSHIP * bigships;

  return gain;
}

int max_population( const PLANET_DATA * planet )
{
  int support = ( int ) ( ( planet->pop_support + 200 ) / 3 );
  int pmax = planet->citysize;
  int rmax = planet->wilderness / 5 + 5 * planet->farmland;

  pmax = pmax * support / 100;
  return UMIN( rmax, pmax );
}

static PLANET_DATA *planet_create( void )
{
  PLANET_DATA *planet = 0;

  CREATE( planet, PLANET_DATA, 1 );

  planet->next = NULL;
  planet->prev = NULL;
  planet->next_in_system = NULL;
  planet->prev_in_system = NULL;
  planet->first_guard = NULL;
  planet->last_guard = NULL;
  planet->starsystem = NULL;
  planet->area = NULL;
  planet->name = NULL;
  planet->filename = NULL;
  planet->governed_by = NULL;
  planet->population = 0;
  planet->pop_support = 0.0;
  planet->sector = 0;
  vector_init( &planet->pos );
  planet->size = 0;
  planet->citysize = 0;
  planet->wilderness = 0;
  planet->wildlife = 0;
  planet->farmland = 0;
  planet->barracks = 0;
  planet->controls = 0;

  return planet;
}
