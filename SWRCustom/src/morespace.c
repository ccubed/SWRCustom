#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "mud.h"

SHIP_PROTOTYPE *first_ship_prototype = NULL;
SHIP_PROTOTYPE *last_ship_prototype = NULL;

static void create_fighter1_rooms( SHIP_DATA * ship,
    ROOM_INDEX_DATA * room[] );
static void create_fighter2_rooms( SHIP_DATA * ship,
    ROOM_INDEX_DATA * room[] );
static void create_shuttle1_rooms( SHIP_DATA * ship,
    ROOM_INDEX_DATA * room[] );
static void create_shuttle2_rooms( SHIP_DATA * ship,
    ROOM_INDEX_DATA * room[] );
static void create_transport1_rooms( SHIP_DATA * s,
    ROOM_INDEX_DATA * room[] );
static void create_transport2_rooms( SHIP_DATA * s,
    ROOM_INDEX_DATA * room[] );
static void create_corvette_rooms( SHIP_DATA * ship,
    ROOM_INDEX_DATA * room[] );
static void create_frigate_rooms( SHIP_DATA * ship,
    ROOM_INDEX_DATA * room[] );
static void create_destroyer_rooms( SHIP_DATA * ship,
    ROOM_INDEX_DATA * room[] );
static void create_cruiser_rooms( SHIP_DATA * ship,
    ROOM_INDEX_DATA * room[] );
static void create_battleship_rooms( SHIP_DATA * s,
    ROOM_INDEX_DATA * room[] );
static void create_flagship_rooms( SHIP_DATA * ship,
    ROOM_INDEX_DATA * room[] );
static void create_default_rooms( SHIP_DATA * ship,
    ROOM_INDEX_DATA * room[] );

/* 
   Its important that the roomnumbers on this list matches the designs in 
   create_ship_rooms. Very bad things will happen if they don't. 
   */

const struct model_type model[MAX_SHIP_MODEL + 1] = {
  /* name, hyperspeed, realspeed, missiles, lasers, tractorbeam, manuever, energy, shield, hull, rooms, builder function */
  {"Short Range Fighter", 0, 255, 10, 4, 0, 255, 2500, 50, 250, 1,
    create_fighter1_rooms},	/* FIGHTER1  */

  {"Shuttle", 150, 200, 0, 1, 0, 100, 2500, 50, 250, 2,
    create_shuttle1_rooms},	/* SHUTTLE1  */

  {"Transport", 100, 100, 0, 2, 0, 75, 3500, 100, 1000, 4,
    create_transport1_rooms},	/* TRANSPORT1  */

  {"Long Range Fighter", 100, 150, 10, 4, 0, 150, 3500, 100, 500, 1,
    create_fighter2_rooms},	/* FIGHTER2  */

  {"Assault Shuttle", 150, 100, 0, 2, 0, 100, 3500, 100, 500, 3,
    create_shuttle2_rooms},	/* SHUTTLE2  */

  {"Assault Transport", 150, 75, 10, 2, 0, 75, 5000, 200, 1500, 5,
    create_transport2_rooms},	/* TRANSPORT2  */

  {"Corvette", 150, 100, 20, 6, 0, 100, 7500, 200, 2500, 8,
    create_corvette_rooms},	/* CORVETTE  */

  {"Frigate", 150, 85, 30, 6, 0, 85, 10000, 250, 5000, 10,
    create_frigate_rooms},	/* FRIGATE  */

  {"Destroyer", 150, 70, 50, 8, 0, 70, 15000, 350, 10000, 14,
    create_destroyer_rooms},	/* DESTROYER  */

  {"Cruiser", 200, 50, 100, 8, 0, 50, 20000, 500, 20000, 15,
    create_cruiser_rooms},	/* CRUISER  */

  {"Battlecruiser", 200, 35, 150, 10, 0, 35, 30000, 750, 25000, 18,
    create_battleship_rooms},	/* BATTLESHIP  */

  {"Flagship", 250, 25, 200, 10, 0, 25, 35000, 1000, 30000, 24,
    create_flagship_rooms},	/* FLAGSHIP  */

  {"Custom Ship", 255, 255, 200, 10, 0, 255, 35000, 1000, 30000,
    MAX_SHIP_ROOMS, create_default_rooms}	/* CUSTOM_SHIP  */
};

/* local routines */
void fread_ship_prototype( SHIP_PROTOTYPE * ship, FILE * fp );
bool load_ship_prototype( const char *shipfile );
void write_prototype_list( void );
void write_ship_list( void );
void bridge_rooms( ROOM_INDEX_DATA * rfrom, ROOM_INDEX_DATA * rto, int edir );
void bridge_elevator( ROOM_INDEX_DATA * rfrom, ROOM_INDEX_DATA * rto,
    int edir, const char *exname );
void post_ship_guard( ROOM_INDEX_DATA * pRoomIndex );
void make_cockpit( ROOM_INDEX_DATA * room, SHIP_DATA * ship );
void make_turret( ROOM_INDEX_DATA * room, SHIP_DATA * ship );
void make_bridge( ROOM_INDEX_DATA * room, SHIP_DATA * ship );
void make_pilot( ROOM_INDEX_DATA * room, SHIP_DATA * ship );
void make_copilot( ROOM_INDEX_DATA * room, SHIP_DATA * ship );
void make_engine( ROOM_INDEX_DATA * room, SHIP_DATA * ship );
void make_medical( ROOM_INDEX_DATA * room, SHIP_DATA * ship );
void make_lounge( ROOM_INDEX_DATA * room, SHIP_DATA * ship );
void make_entrance( ROOM_INDEX_DATA * room, SHIP_DATA * ship );
void make_hanger( ROOM_INDEX_DATA * room, SHIP_DATA * ship );
void make_garage( ROOM_INDEX_DATA * room, SHIP_DATA * ship );
void make_elevator( ROOM_INDEX_DATA * room, SHIP_DATA * ship );
long get_design_value( int hull, int energy, int shield, int speed,
    int manuever, int lasers, int missiles,
    int chaff, int smodel );

long int get_prototype_value( const SHIP_PROTOTYPE * prototype )
{
  long int price = 0;

  if( prototype->ship_class == SPACECRAFT )
    price = SPACECRAFT_BASE_PRICE;
  else if( prototype->ship_class == AIRCRAFT )
    price = AIRCRAFT_BASE_PRICE;
  else if( prototype->ship_class == SUBMARINE )
    price = SUBMARINE_BASE_PRICE;
  else if( prototype->ship_class == SPACE_STATION )
    price = SPACESTATION_BASE_PRICE;
  else
    price = DEFAULT_CRAFT_BASE_PRICE;

  price += ( prototype->tractorbeam * 100 );
  price += ( prototype->realspeed * 10 );
  price += ( 5 * prototype->maxhull );
  price += ( 2 * prototype->maxenergy );
  price += ( 100 * prototype->maxchaff );

  if( prototype->maxenergy > 5000 )
    price += ( ( prototype->maxenergy - 5000 ) * 20 );

  if( prototype->maxenergy > 10000 )
    price += ( ( prototype->maxenergy - 10000 ) * 50 );

  if( prototype->maxhull > 1000 )
    price += ( ( prototype->maxhull - 1000 ) * 10 );

  if( prototype->maxhull > 10000 )
    price += ( ( prototype->maxhull - 10000 ) * 20 );

  if( prototype->maxshield > 200 )
    price += ( ( prototype->maxshield - 200 ) * 50 );

  if( prototype->maxshield > 1000 )
    price += ( ( prototype->maxshield - 1000 ) * 100 );

  if( prototype->realspeed > 100 )
    price += ( ( prototype->realspeed - 100 ) * 500 );

  if( prototype->lasers > 5 )
    price += ( ( prototype->lasers - 5 ) * 500 );

  if( prototype->maxshield )
    price += ( 1000 + 10 * prototype->maxshield );

  if( prototype->lasers )
    price += ( 500 + 500 * prototype->lasers );

  if( prototype->maxmissiles )
    price += ( 1000 + 250 * prototype->maxmissiles );

  if( prototype->hyperspeed )
    price += ( 1000 + prototype->hyperspeed * 10 );

  price *= ( int ) ( 1 + prototype->model / 3 );

  return price;
}

long int get_design_value( int hull, int energy, int shield, int speed,
    int manuever, int lasers, int missiles,
    int chaff, int smodel )
{
  long int price = SPACECRAFT_BASE_PRICE;
  price += ( speed * 10 );
  price += ( 5 * hull );
  price += ( 2 * energy );
  price += ( 100 * chaff );

  if( energy > 5000 )
    price += ( ( energy - 5000 ) * 20 );

  if( energy > 10000 )
    price += ( ( energy - 10000 ) * 50 );

  if( hull > 1000 )
    price += ( ( hull - 1000 ) * 10 );

  if( hull > 10000 )
    price += ( ( hull - 10000 ) * 20 );

  if( shield > 200 )
    price += ( ( shield - 200 ) * 50 );

  if( shield > 1000 )
    price += ( ( shield - 1000 ) * 100 );

  if( speed > 100 )
    price += ( ( speed - 100 ) * 500 );

  if( lasers > 5 )
    price += ( ( lasers - 5 ) * 500 );

  if( shield )
    price += ( 1000 + 10 * shield );

  if( lasers )
    price += ( 500 + 500 * lasers );

  if( missiles )
    price += ( 1000 + 250 * missiles );

  if( model[smodel].hyperspeed )
    price += ( 1000 + model[smodel].hyperspeed * 10 );

  price *= ( int ) ( 1 + smodel / 3 );

  return price;
}

void write_prototype_list()
{
  SHIP_PROTOTYPE *prototype = NULL;
  FILE *fpout = NULL;
  char filename[256];

  sprintf( filename, "%s%s", PROTOTYPE_DIR, PROTOTYPE_LIST );
  fpout = fopen( filename, "w" );

  if( !fpout )
  {
    bug( "FATAL: cannot open protoytpe.lst for writing!\r\n", 0 );
    return;
  }

  for( prototype = first_ship_prototype; prototype;
      prototype = prototype->next )
    fprintf( fpout, "%s\n", prototype->filename );

  fprintf( fpout, "$\n" );
  fclose( fpout );
}

SHIP_PROTOTYPE *get_ship_prototype( const char *name )
{
  SHIP_PROTOTYPE *prototype = NULL;

  for( prototype = first_ship_prototype; prototype;
      prototype = prototype->next )
    if( !str_cmp( name, prototype->name ) )
      return prototype;

  for( prototype = first_ship_prototype; prototype;
      prototype = prototype->next )
    if( nifty_is_name_prefix( name, prototype->name ) )
      return prototype;

  return NULL;
}

void save_ship_prototype( const SHIP_PROTOTYPE * prototype )
{
  FILE *fp;
  char filename[256];
  char buf[MAX_STRING_LENGTH];

  if( !prototype )
  {
    bug( "save_ship_prototype: null prototype pointer!", 0 );
    return;
  }

  if( !prototype->filename || prototype->filename[0] == '\0' )
  {
    sprintf( buf, "save_ship_prototype: %s has no filename",
	prototype->name );
    bug( buf, 0 );
    return;
  }

  sprintf( filename, "%s%s", PROTOTYPE_DIR, prototype->filename );

  if( ( fp = fopen( filename, "w" ) ) == NULL )
  {
    bug( "save_ship_prototype: fopen", 0 );
    perror( filename );
  }
  else
  {
    fprintf( fp, "#PROTOTYPE\n" );
    fprintf( fp, "Name         %s~\n", prototype->name );
    fprintf( fp, "Filename     %s~\n", prototype->filename );
    fprintf( fp, "Description  %s~\n", prototype->description );
    fprintf( fp, "Class        %d\n", prototype->ship_class );
    fprintf( fp, "Model        %d\n", prototype->model );
    fprintf( fp, "Tractorbeam  %d\n", prototype->tractorbeam );
    fprintf( fp, "Lasers       %d\n", prototype->lasers );
    fprintf( fp, "Maxmissiles  %d\n", prototype->maxmissiles );
    fprintf( fp, "Maxshield    %d\n", prototype->maxshield );
    fprintf( fp, "Maxhull      %d\n", prototype->maxhull );
    fprintf( fp, "Maxenergy    %d\n", prototype->maxenergy );
    fprintf( fp, "Hyperspeed   %d\n", prototype->hyperspeed );
    fprintf( fp, "Maxchaff     %d\n", prototype->maxchaff );
    fprintf( fp, "Realspeed    %d\n", prototype->realspeed );
    fprintf( fp, "Manuever     %d\n", prototype->manuever );
    fprintf( fp, "End\n\n" );
    fprintf( fp, "#END\n" );
  }

  fclose( fp );
}

/*
 * Read in actual prototype data.
 */

void fread_ship_prototype( SHIP_PROTOTYPE * prototype, FILE * fp )
{
  char buf[MAX_STRING_LENGTH];
  const char *word;
  bool fMatch;

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
	KEY( "Class", prototype->ship_class, fread_number( fp ) );
	break;

      case 'D':
	KEY( "Description", prototype->description, fread_string( fp ) );
	break;

      case 'E':
	if( !str_cmp( word, "End" ) )
	{
	  if( !prototype->name )
	    prototype->name = STRALLOC( "" );

	  if( !prototype->description )
	    prototype->description = STRALLOC( "" );

	  return;
	}
	break;

      case 'F':
	KEY( "Filename", prototype->filename, fread_string_nohash( fp ) );
	break;

      case 'H':
	KEY( "Hyperspeed", prototype->hyperspeed, fread_number( fp ) );
	break;

      case 'L':
	KEY( "Lasers", prototype->lasers, fread_number( fp ) );
	break;

      case 'M':
	KEY( "Model", prototype->model, fread_number( fp ) );
	KEY( "Manuever", prototype->manuever, fread_number( fp ) );
	KEY( "Maxmissiles", prototype->maxmissiles, fread_number( fp ) );
	KEY( "Maxshield", prototype->maxshield, fread_number( fp ) );
	KEY( "Maxenergy", prototype->maxenergy, fread_number( fp ) );
	KEY( "Maxhull", prototype->maxhull, fread_number( fp ) );
	KEY( "Maxchaff", prototype->maxchaff, fread_number( fp ) );
	break;

      case 'N':
	KEY( "Name", prototype->name, fread_string( fp ) );
	break;

      case 'R':
	KEY( "Realspeed", prototype->realspeed, fread_number( fp ) );
	break;

      case 'T':
	KEY( "Tractorbeam", prototype->tractorbeam, fread_number( fp ) );
	break;
    }

    if( !fMatch )
    {
      sprintf( buf, "Fread_ship_prototype: no match: %s", word );
      bug( buf, 0 );
    }
  }
}

SHIP_PROTOTYPE *ship_prototype_create( void )
{
  SHIP_PROTOTYPE *ship = NULL;
  CREATE( ship, SHIP_PROTOTYPE, 1 );
  ship->next = NULL;
  ship->prev = NULL;
  ship->filename = NULL;
  ship->name = NULL;
  ship->description = NULL;
  ship->ship_class = 0;
  ship->model = 0;
  ship->hyperspeed = 0;
  ship->realspeed = 0;
  ship->maxmissiles = 0;
  ship->lasers = 0;
  ship->tractorbeam = 0;
  ship->manuever = 0;
  ship->maxenergy = 0;
  ship->maxshield = 0;
  ship->maxhull = 0;
  ship->maxchaff = 0;

  return ship;
}

/*
 * Load a prototype file
 */

bool load_ship_prototype( const char *prototypefile )
{
  FILE *fp = NULL;
  char filename[256];
  SHIP_PROTOTYPE *prototype = ship_prototype_create();
  bool found = FALSE;
  sprintf( filename, "%s%s", PROTOTYPE_DIR, prototypefile );

  if( ( fp = fopen( filename, "r" ) ) != NULL )
  {
    found = TRUE;

    for( ;; )
    {
      const char *word;
      char letter = fread_letter( fp );

      if( letter == '*' )
      {
	fread_to_eol( fp );
	continue;
      }

      if( letter != '#' )
      {
	bug( "Load_ship_prototype: # not found.", 0 );
	break;
      }

      word = fread_word( fp );

      if( !str_cmp( word, "PROTOTYPE" ) )
      {
	fread_ship_prototype( prototype, fp );
	break;
      }
      else if( !str_cmp( word, "END" ) )
      {
	break;
      }
      else
      {
	char buf[MAX_STRING_LENGTH];

	sprintf( buf, "Load_ship_prototype: bad section: %s.", word );
	bug( buf, 0 );
	break;
      }
    }

    fclose( fp );
  }

  if( !( found ) )
  {
    DISPOSE( prototype );
  }
  else
  {
    LINK( prototype, first_ship_prototype, last_ship_prototype, next,
	prev );
  }

  return found;
}

/*
 * Load in all the prototype files.
 */
void load_prototypes()
{
  FILE *fpList = NULL;
  char prototypelist[256];

  first_ship_prototype = NULL;
  last_ship_prototype = NULL;

  sprintf( prototypelist, "%s%s", PROTOTYPE_DIR, PROTOTYPE_LIST );

  if( ( fpList = fopen( prototypelist, "r" ) ) == NULL )
  {
    perror( prototypelist );
    exit( 1 );
  }

  for( ;; )
  {
    const char *filename = feof( fpList ) ? "$" : fread_word( fpList );
    char buf[MAX_STRING_LENGTH];

    if( filename[0] == '$' )
      break;

    if( !load_ship_prototype( filename ) )
    {
      sprintf( buf, "Cannot load ship prototype file: %s", filename );
      bug( buf, 0 );
    }
  }

  fclose( fpList );
}

void do_setprototype( CHAR_DATA * ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  SHIP_PROTOTYPE *prototype = NULL;

  if( IS_NPC( ch ) || !ch->pcdata )
    return;

  if( !ch->desc )
    return;

  switch ( ch->substate )
  {
    default:
      break;

    case SUB_SHIPDESC:
      prototype = ( SHIP_PROTOTYPE * ) ch->dest_buf;
      if( !prototype )
      {
	bug( "setprototype: sub_shipdesc: NULL ch->dest_buf", 0 );
	stop_editing( ch );
	ch->substate = ch->tempnum;
	send_to_char( "&RError: prototype lost.\r\n", ch );
	return;
      }

      STRFREE( prototype->description );
      prototype->description = copy_buffer( ch );
      stop_editing( ch );
      ch->substate = ch->tempnum;
      save_ship_prototype( prototype );
      return;
  }

  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );

  if( arg1[0] == '\0' || arg2[0] == '\0' || arg1[0] == '\0' )
  {
    send_to_char( "Usage: setprototype <prototype> <field> <values>\r\n",
	ch );
    send_to_char( "\r\nField being one of:\r\n", ch );
    send_to_char( "filename name description class model\r\n", ch );
    send_to_char( "manuever speed hyperspeed tractorbeam\r\n", ch );
    send_to_char( "lasers missiles shield hull energy chaff\r\n", ch );
    return;
  }

  prototype = get_ship_prototype( arg1 );

  if( !prototype )
  {
    send_to_char( "No such ship prototype.\r\n", ch );
    return;
  }

  if( !str_cmp( arg2, "name" ) )
  {
    if( *argument == '\0' )
    {
      ch_printf( ch, "Blank name not permitted.\r\n" );
      return;
    }

    if( get_ship_prototype( argument ) )
    {
      ch_printf( ch, "A prototype with that name already exists.\r\n" );
      return;
    }

    STRFREE( prototype->name );
    prototype->name = STRALLOC( argument );
    send_to_char( "Done.\r\n", ch );
    save_ship_prototype( prototype );
    return;
  }

  if( !str_cmp( arg2, "filename" ) )
  {
    DISPOSE( prototype->filename );
    prototype->filename = str_dup( argument );
    send_to_char( "Done.\r\n", ch );
    save_ship_prototype( prototype );
    write_prototype_list();
    return;
  }

  if( !str_cmp( arg2, "description" ) )
  {
    ch->substate = SUB_SHIPDESC;
    ch->dest_buf = prototype;
    start_editing( ch, prototype->description );
    return;
  }

  if( !str_cmp( arg2, "manuever" ) )
  {
    prototype->manuever = URANGE( 0, atoi( argument ), 120 );
    send_to_char( "Done.\r\n", ch );
    save_ship_prototype( prototype );
    return;
  }

  if( !str_cmp( arg2, "lasers" ) )
  {
    prototype->lasers = URANGE( 0, atoi( argument ), 10 );
    send_to_char( "Done.\r\n", ch );
    save_ship_prototype( prototype );
    return;
  }

  if( !str_cmp( arg2, "class" ) )
  {
    prototype->ship_class = URANGE( 0, atoi( argument ), 9 );
    send_to_char( "Done.\r\n", ch );
    save_ship_prototype( prototype );
    return;
  }

  if( !str_cmp( arg2, "model" ) )
  {
    prototype->model = URANGE( 0, atoi( argument ), 9 );
    send_to_char( "Done.\r\n", ch );
    save_ship_prototype( prototype );
    return;
  }

  if( !str_cmp( arg2, "missiles" ) )
  {
    prototype->maxmissiles = URANGE( 0, atoi( argument ), 255 );
    send_to_char( "Done.\r\n", ch );
    save_ship_prototype( prototype );
    return;
  }

  if( !str_cmp( arg2, "speed" ) )
  {
    prototype->realspeed = URANGE( 0, atoi( argument ), 150 );
    send_to_char( "Done.\r\n", ch );
    save_ship_prototype( prototype );
    return;
  }

  if( !str_cmp( arg2, "tractorbeam" ) )
  {
    prototype->tractorbeam = URANGE( 0, atoi( argument ), 255 );
    send_to_char( "Done.\r\n", ch );
    save_ship_prototype( prototype );
    return;
  }

  if( !str_cmp( arg2, "hyperspeed" ) )
  {
    prototype->hyperspeed = URANGE( 0, atoi( argument ), 255 );
    send_to_char( "Done.\r\n", ch );
    save_ship_prototype( prototype );
    return;
  }

  if( !str_cmp( arg2, "shield" ) )
  {
    prototype->maxshield = URANGE( 0, atoi( argument ), 1000 );
    send_to_char( "Done.\r\n", ch );
    save_ship_prototype( prototype );
    return;
  }

  if( !str_cmp( arg2, "hull" ) )
  {
    prototype->maxhull = URANGE( 1, atoi( argument ), 20000 );
    send_to_char( "Done.\r\n", ch );
    save_ship_prototype( prototype );
    return;
  }

  if( !str_cmp( arg2, "energy" ) )
  {
    prototype->maxenergy = URANGE( 1, atoi( argument ), 30000 );
    send_to_char( "Done.\r\n", ch );
    save_ship_prototype( prototype );
    return;
  }

  if( !str_cmp( arg2, "chaff" ) )
  {
    prototype->maxchaff = URANGE( 0, atoi( argument ), 25 );
    send_to_char( "Done.\r\n", ch );
    save_ship_prototype( prototype );
    return;
  }

  do_setprototype( ch, STRLIT_EMPTY );
}

void do_showprototype( CHAR_DATA * ch, char *argument )
{
  SHIP_PROTOTYPE *prototype;

  if( IS_NPC( ch ) )
  {
    send_to_char( "Huh?\r\n", ch );
    return;
  }

  if( argument[0] == '\0' )
  {
    send_to_char( "Usage: showprototype <prototype>\r\n", ch );
    return;
  }

  prototype = get_ship_prototype( argument );

  if( !prototype )
  {
    send_to_char( "No such prototype.\r\n", ch );
    return;
  }

  set_char_color( AT_YELLOW, ch );
  ch_printf( ch, "%s : %s\r\nFilename: %s\r\n",
      prototype->ship_class ==
      SPACECRAFT ? model[prototype->model].name : ( prototype->
	ship_class ==
	SPACE_STATION ?
	"Space Station"
	: ( prototype->
	  ship_class ==
	  AIRCRAFT ?
	  "Aircraft"
	  : ( prototype->
	    ship_class
	    ==
	    BOAT ?
	    "Boat"
	    :
	    ( prototype->
	      ship_class
	      ==
	      SUBMARINE
	      ?
	      "Submatine"
	      :
	      ( prototype->
		ship_class
		==
		LAND_VEHICLE
		?
		"land vehicle"
		:
		"Unknown" ) ) ) ) ),
    prototype->name, prototype->filename );
  ch_printf( ch, "Description: %s\r\n", prototype->description );
  ch_printf( ch, "Tractor Beam: %d  ", prototype->tractorbeam );
  ch_printf( ch, "Lasers: %d     ", prototype->lasers );
  ch_printf( ch, "Missiles: %d\r\n", prototype->maxmissiles );
  ch_printf( ch, "Hull: %d     ", prototype->maxhull );
  ch_printf( ch, "Shields: %d   Energy(fuel): %d   Chaff: %d\r\n",
      prototype->maxshield,
      prototype->maxenergy, prototype->maxchaff );
  ch_printf( ch, "Speed: %d    Hyperspeed: %d   Manueverability: %d\r\n",
      prototype->realspeed, prototype->hyperspeed,
      prototype->manuever );
}

void do_makeship( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  SHIP_PROTOTYPE *prototype = NULL;

  argument = one_argument( argument, arg );

  if( !argument || argument[0] == '\0' )
  {
    send_to_char( "Usage: makeship <filename> <prototype name>\r\n", ch );
    return;
  }

  prototype = ship_prototype_create();

  LINK( prototype, first_ship_prototype, last_ship_prototype, next, prev );

  prototype->name = STRALLOC( argument );
  prototype->description = STRALLOC( "" );

  prototype->filename = str_dup( arg );
  save_ship_prototype( prototype );
  write_prototype_list();
}

SHIP_DATA *make_ship( const SHIP_PROTOTYPE * prototype )
{
  SHIP_DATA *ship = NULL;
  int shipreg = 0;
  char filename[10];
  char shipname[MAX_STRING_LENGTH];

  if( prototype == NULL )
    return NULL;

  for( ship = first_ship; ship; ship = ship->next )
    if( shipreg < atoi( ship->filename ) )
      shipreg = atoi( ship->filename );

  shipreg++;
  sprintf( filename, "%d", shipreg );
  sprintf( shipname, "%s %d", prototype->name, shipreg );

  ship = ship_create();

  LINK( ship, first_ship, last_ship, next, prev );

  ship->filename = str_dup( filename );
  ship->name = STRALLOC( shipname );
  ship->home = STRALLOC( "" );

  if( prototype->description )
    ship->description[0] = STRALLOC( prototype->description );

  ship->owner = STRALLOC( "" );
  ship->pilot = STRALLOC( "" );
  ship->copilot = STRALLOC( "" );
  ship->type = PLAYER_SHIP;
  ship->ship_class = prototype->ship_class;
  ship->model = prototype->model;
  ship->hyperspeed = prototype->hyperspeed;
  ship->realspeed = prototype->realspeed;
  ship->shipstate = SHIP_DOCKED;
  ship->missiles = prototype->maxmissiles;
  ship->maxmissiles = prototype->maxmissiles;
  ship->lasers = prototype->lasers;
  ship->tractorbeam = prototype->tractorbeam;
  ship->manuever = prototype->manuever;
  ship->maxenergy = prototype->maxenergy;
  ship->energy = prototype->maxenergy;
  ship->maxshield = prototype->maxshield;
  ship->hull = prototype->maxhull;
  ship->maxhull = prototype->maxhull;
  ship->chaff = prototype->maxchaff;
  ship->maxchaff = prototype->maxchaff;
  create_ship_rooms( ship );

  save_ship( ship );
  write_ship_list();

  return ship;
}

void do_prototypes( CHAR_DATA * ch, char *argument )
{
  if( str_cmp( argument, "vehicles" ) )
  {
    int count = 0;
    SHIP_PROTOTYPE *prototype = NULL;
    send_to_pager
      ( "&Y\r\nThe following ships are currently prototyped:&W\r\n", ch );
    for( prototype = first_ship_prototype; prototype;
	prototype = prototype->next )
    {
      if( prototype->ship_class > SPACE_STATION )
	continue;

      pager_printf( ch, "%-35s    ", prototype->name );
      pager_printf( ch, "%ld to buy.\r\n",
	  get_prototype_value( prototype ) );
      count++;
    }

    if( !count )
    {
      send_to_pager( "There are no ship prototypes currently formed.\r\n",
	  ch );
      return;
    }
  }

  if( str_cmp( argument, "ships" ) )
  {
    int count = 0;
    SHIP_PROTOTYPE *prototype = NULL;
    send_to_pager
      ( "&Y\r\nThe following vehicles are currently prototyped:&W\r\n",
	ch );

    for( prototype = first_ship_prototype; prototype;
	prototype = prototype->next )
    {
      if( prototype->ship_class <= SPACE_STATION )
	continue;

      pager_printf( ch, "%-35s    ", prototype->name );
      pager_printf( ch, "%ld to buy.\r\n",
	  get_prototype_value( prototype ) );
      count++;
    }

    if( !count )
    {
      send_to_pager
	( "There are no vehicle prototypes currently formed.\r\n", ch );
      return;
    }
  }
}

static void create_default_rooms( SHIP_DATA * ship, ROOM_INDEX_DATA * room[] )
{
  ship->pilotseat = room[0];
  ship->gunseat = room[0];
  ship->viewscreen = room[0];
  ship->entrance = room[0];
  ship->engine = room[0];
}

static void create_fighter1_rooms( SHIP_DATA * ship,
    ROOM_INDEX_DATA * room[] )
{
  ship->pilotseat = room[0];
  ship->gunseat = room[0];
  ship->viewscreen = room[0];
  room[0]->tunnel = 1;
  ship->entrance = room[0];
  ship->engine = room[0];
}

static void create_fighter2_rooms( SHIP_DATA * ship,
    ROOM_INDEX_DATA * room[] )
{
  ship->pilotseat = room[0];
  ship->gunseat = room[0];
  ship->viewscreen = room[0];
  room[0]->tunnel = 2;
  ship->entrance = room[0];
  ship->engine = room[0];
}

static void create_shuttle1_rooms( SHIP_DATA * ship,
    ROOM_INDEX_DATA * room[] )
{
  bridge_rooms( room[1], room[0], DIR_NORTH );
  make_cockpit( room[0], ship );
  ship->entrance = room[0];
  ship->engine = room[0];
}

static void create_shuttle2_rooms( SHIP_DATA * ship,
    ROOM_INDEX_DATA * room[] )
{
  bridge_rooms( room[1], room[0], DIR_NORTH );
  bridge_rooms( room[1], room[2], DIR_SOUTH );
  make_cockpit( room[0], ship );
  make_turret( room[2], ship );
  ship->entrance = room[0];
  ship->engine = room[0];
}

static void create_transport1_rooms( SHIP_DATA * ship,
    ROOM_INDEX_DATA * room[] )
{
  bridge_rooms( room[1], room[0], DIR_NORTH );
  bridge_rooms( room[1], room[2], DIR_SOUTH );
  bridge_rooms( room[1], room[3], DIR_WEST );
  make_cockpit( room[0], ship );
  make_engine( room[2], ship );
  make_entrance( room[1], ship );
}

static void create_transport2_rooms( SHIP_DATA * ship,
    ROOM_INDEX_DATA * room[] )
{
  bridge_rooms( room[1], room[0], DIR_NORTH );
  bridge_rooms( room[1], room[2], DIR_SOUTH );
  bridge_rooms( room[1], room[3], DIR_WEST );
  bridge_rooms( room[3], room[4], DIR_DOWN );
  make_cockpit( room[0], ship );
  make_engine( room[2], ship );
  make_entrance( room[1], ship );
  make_turret( room[4], ship );
}

static void create_corvette_rooms( SHIP_DATA * ship,
    ROOM_INDEX_DATA * room[] )
{
  make_pilot( room[0], ship );
  make_bridge( room[1], ship );
  make_copilot( room[2], ship );
  make_engine( room[5], ship );
  make_entrance( room[3], ship );
  make_turret( room[6], ship );
  make_turret( room[7], ship );
  bridge_rooms( room[0], room[1], DIR_EAST );
  bridge_rooms( room[1], room[2], DIR_EAST );
  bridge_rooms( room[1], room[3], DIR_SOUTH );
  bridge_rooms( room[3], room[4], DIR_SOUTH );
  bridge_rooms( room[4], room[5], DIR_SOUTH );
  bridge_rooms( room[4], room[6], DIR_WEST );
  bridge_rooms( room[4], room[7], DIR_EAST );
  post_ship_guard( room[1] );
}

static void create_frigate_rooms( SHIP_DATA * ship, ROOM_INDEX_DATA * room[] )
{
  make_pilot( room[0], ship );
  make_bridge( room[1], ship );
  make_copilot( room[2], ship );
  make_engine( room[9], ship );
  make_entrance( room[6], ship );
  make_turret( room[3], ship );
  make_turret( room[5], ship );
  make_lounge( room[8], ship );
  bridge_rooms( room[0], room[1], DIR_EAST );
  bridge_rooms( room[1], room[2], DIR_EAST );
  bridge_rooms( room[3], room[4], DIR_EAST );
  bridge_rooms( room[4], room[5], DIR_EAST );
  bridge_rooms( room[7], room[8], DIR_EAST );
  bridge_rooms( room[6], room[7], DIR_EAST );
  bridge_rooms( room[1], room[4], DIR_SOUTH );
  bridge_rooms( room[4], room[7], DIR_SOUTH );
  bridge_rooms( room[7], room[9], DIR_SOUTH );
  post_ship_guard( room[1] );
}

static void create_destroyer_rooms( SHIP_DATA * ship,
    ROOM_INDEX_DATA * room[] )
{
  make_pilot( room[0], ship );
  make_bridge( room[1], ship );
  make_copilot( room[2], ship );
  make_engine( room[9], ship );
  make_entrance( room[10], ship );
  make_turret( room[3], ship );
  make_turret( room[5], ship );
  make_garage( room[8], ship );
  make_hanger( room[6], ship );
  make_lounge( room[11], ship );
  bridge_rooms( room[0], room[1], DIR_EAST );
  bridge_rooms( room[1], room[2], DIR_EAST );
  bridge_rooms( room[3], room[4], DIR_EAST );
  bridge_rooms( room[4], room[5], DIR_EAST );
  bridge_rooms( room[7], room[8], DIR_EAST );
  bridge_rooms( room[6], room[7], DIR_EAST );
  bridge_rooms( room[1], room[4], DIR_SOUTH );
  bridge_rooms( room[4], room[7], DIR_SOUTH );
  bridge_rooms( room[7], room[9], DIR_SOUTH );
  bridge_rooms( room[7], room[10], DIR_DOWN );
  bridge_rooms( room[7], room[11], DIR_UP );
  post_ship_guard( room[1] );
}

static void create_cruiser_rooms( SHIP_DATA * ship, ROOM_INDEX_DATA * room[] )
{
  make_pilot( room[0], ship );
  make_bridge( room[1], ship );
  make_copilot( room[2], ship );
  make_engine( room[7], ship );
  make_entrance( room[5], ship );
  make_turret( room[11], ship );
  make_turret( room[12], ship );
  make_turret( room[13], ship );
  make_turret( room[14], ship );
  make_garage( room[10], ship );
  make_hanger( room[9], ship );
  make_lounge( room[4], ship );
  bridge_rooms( room[0], room[1], DIR_EAST );
  bridge_rooms( room[1], room[2], DIR_EAST );
  bridge_rooms( room[1], room[3], DIR_DOWN );
  bridge_rooms( room[3], room[4], DIR_DOWN );
  bridge_rooms( room[3], room[5], DIR_NORTH );
  bridge_rooms( room[5], room[6], DIR_SOUTHWEST );
  bridge_rooms( room[6], room[7], DIR_SOUTHEAST );
  bridge_rooms( room[7], room[8], DIR_NORTHEAST );
  bridge_rooms( room[8], room[5], DIR_NORTHWEST );
  bridge_rooms( room[6], room[9], DIR_WEST );
  bridge_rooms( room[8], room[10], DIR_EAST );
  bridge_rooms( room[6], room[12], DIR_SOUTHWEST );
  bridge_rooms( room[8], room[14], DIR_SOUTHEAST );
  bridge_rooms( room[8], room[13], DIR_NORTHEAST );
  bridge_rooms( room[6], room[11], DIR_NORTHWEST );
  post_ship_guard( room[1] );
  post_ship_guard( room[1] );
}

static void create_battleship_rooms( SHIP_DATA * ship,
    ROOM_INDEX_DATA * room[] )
{
  make_pilot( room[0], ship );
  make_bridge( room[1], ship );
  make_copilot( room[2], ship );
  make_engine( room[11], ship );
  make_elevator( room[16], ship );
  make_turret( room[3], ship );
  make_turret( room[4], ship );
  make_turret( room[5], ship );
  make_turret( room[6], ship );
  make_turret( room[7], ship );
  make_turret( room[8], ship );
  make_garage( room[12], ship );
  make_hanger( room[13], ship );
  make_hanger( room[15], ship );
  make_lounge( room[9], ship );
  make_medical( room[10], ship );
  bridge_rooms( room[0], room[1], DIR_EAST );
  bridge_rooms( room[1], room[2], DIR_EAST );
  bridge_rooms( room[17], room[3], DIR_NORTHWEST );
  bridge_rooms( room[17], room[4], DIR_NORTHEAST );
  bridge_rooms( room[17], room[5], DIR_EAST );
  bridge_rooms( room[17], room[6], DIR_SOUTHEAST );
  bridge_rooms( room[17], room[7], DIR_SOUTHWEST );
  bridge_rooms( room[17], room[8], DIR_WEST );
  bridge_rooms( room[13], room[14], DIR_EAST );
  bridge_rooms( room[14], room[15], DIR_EAST );
  bridge_rooms( room[12], room[14], DIR_SOUTH );
  bridge_elevator( room[1], room[16], DIR_SOUTH, "Navigation" );
  bridge_elevator( room[17], room[16], DIR_SOUTH, "Turrets" );
  bridge_elevator( room[9], room[16], DIR_SOUTH, "Lounge" );
  bridge_elevator( room[10], room[16], DIR_SOUTH, "Medical" );
  bridge_elevator( room[11], room[16], DIR_SOUTH, "Engineering" );
  bridge_elevator( room[14], room[16], DIR_SOUTH, "Hangers" );
  post_ship_guard( room[1] );
  post_ship_guard( room[1] );
}

static void create_flagship_rooms( SHIP_DATA * ship,
    ROOM_INDEX_DATA * room[] )
{
  make_pilot( room[0], ship );
  make_bridge( room[1], ship );
  make_copilot( room[2], ship );
  make_engine( room[20], ship );
  make_entrance( room[7], ship );
  make_turret( room[22], ship );
  make_turret( room[23], ship );
  make_turret( room[15], ship );
  make_turret( room[16], ship );
  make_turret( room[17], ship );
  make_turret( room[18], ship );
  make_garage( room[19], ship );
  make_garage( room[21], ship );
  make_hanger( room[8], ship );
  make_hanger( room[14], ship );
  make_lounge( room[5], ship );
  make_medical( room[3], ship );
  bridge_rooms( room[0], room[1], DIR_EAST );
  bridge_rooms( room[1], room[2], DIR_EAST );
  bridge_rooms( room[3], room[4], DIR_EAST );
  bridge_rooms( room[4], room[5], DIR_EAST );
  bridge_rooms( room[8], room[9], DIR_EAST );
  bridge_rooms( room[9], room[10], DIR_EAST );
  bridge_rooms( room[10], room[11], DIR_EAST );
  bridge_rooms( room[11], room[12], DIR_EAST );
  bridge_rooms( room[12], room[13], DIR_EAST );
  bridge_rooms( room[13], room[14], DIR_EAST );
  bridge_rooms( room[1], room[4], DIR_SOUTH );
  bridge_rooms( room[4], room[6], DIR_SOUTH );
  bridge_rooms( room[6], room[7], DIR_SOUTH );
  bridge_rooms( room[7], room[11], DIR_SOUTH );
  bridge_rooms( room[11], room[20], DIR_SOUTH );
  bridge_rooms( room[15], room[9], DIR_SOUTH );
  bridge_rooms( room[9], room[16], DIR_SOUTH );
  bridge_rooms( room[10], room[19], DIR_SOUTH );
  bridge_rooms( room[17], room[13], DIR_SOUTH );
  bridge_rooms( room[13], room[18], DIR_SOUTH );
  bridge_rooms( room[12], room[21], DIR_SOUTH );
  post_ship_guard( room[1] );
  post_ship_guard( room[1] );
}

void create_ship_rooms( SHIP_DATA * ship )
{
  int roomnum = 0;
  int numrooms = 0;
  ROOM_INDEX_DATA *room[24];

  if( ship->ship_class != SPACECRAFT )
    ship->model = 0;

  numrooms = UMIN( model[ship->model].rooms, MAX_SHIP_ROOMS - 1 );

  for( roomnum = 0; roomnum < numrooms; roomnum++ )
    room[roomnum] = make_ship_room( ship );

  model[ship->model].room_builder( ship, room );

  for( roomnum = 0; roomnum < numrooms; roomnum++ )
  {
    if( ship->description[roomnum]
	&& ship->description[roomnum][0] != '\0' )
    {
      STRFREE( room[roomnum]->description );
      room[roomnum]->description = STRALLOC( ship->description[roomnum] );
    }
  }
}

void bridge_rooms( ROOM_INDEX_DATA * rfrom, ROOM_INDEX_DATA * rto, int edir )
{
  EXIT_DATA *xit = make_exit( rfrom, rto, edir );
  xit->keyword = STRALLOC( "" );
  xit->description = STRALLOC( "" );
  xit->key = -1;
  xit->exit_info = 0;
  xit = make_exit( rto, rfrom, rev_dir[edir] );
  xit->keyword = STRALLOC( "" );
  xit->description = STRALLOC( "" );
  xit->key = -1;
  xit->exit_info = 0;
}

void bridge_elevator( ROOM_INDEX_DATA * rfrom, ROOM_INDEX_DATA * rto,
    int edir, const char *exname )
{
  EXIT_DATA *xit = make_exit( rfrom, rto, edir );
  xit->keyword = STRALLOC( "" );
  xit->description = STRALLOC( "" );
  xit->key = -1;
  xit->exit_info = 0;
  xit = make_exit( rto, rfrom, DIR_SOMEWHERE );
  xit->keyword = STRALLOC( exname );
  xit->description = STRALLOC( "" );
  xit->key = -1;
  xit->exit_info = 526336;
}

void make_cockpit( ROOM_INDEX_DATA * room, SHIP_DATA * ship )
{
  char buf[MAX_STRING_LENGTH];

  room->tunnel = 4;

  if( !ship->pilotseat )
    ship->pilotseat = room;

  if( !ship->gunseat )
    ship->gunseat = room;

  if( !ship->viewscreen )
    ship->viewscreen = room;

  STRFREE( room->name );
  room->name = STRALLOC( "The Cockpit" );

  strcpy( buf,
      "This small cockpit houses the pilot controls as well as the ships main\r\n" );
  strcat( buf, "offensive and defensive systems.\r\n" );
  STRFREE( room->description );
  room->description = STRALLOC( buf );
}

void make_turret( ROOM_INDEX_DATA * room, SHIP_DATA * ship )
{
  char buf[MAX_STRING_LENGTH];
  TURRET_DATA *turret;

  room->tunnel = 1;

  STRFREE( room->name );
  room->name = STRALLOC( "A Laser Turret" );

  strcpy( buf,
      "This turbo laser turret extends from the outter hull of the ship.\r\n" );
  strcat( buf,
      "It is more powerful than the ships main laser batteries and must\r\n" );
  strcat( buf, "be operated manually by a trained gunner.\r\n" );
  STRFREE( room->description );
  room->description = STRALLOC( buf );

  CREATE( turret, TURRET_DATA, 1 );
  turret->next = NULL;
  turret->prev = NULL;
  turret->room = room;
  turret->target = NULL;
  turret->laserstate = 0;

  LINK( turret, ship->first_turret, ship->last_turret, next, prev );
}

void make_bridge( ROOM_INDEX_DATA * room, SHIP_DATA * ship )
{
  char buf[MAX_STRING_LENGTH];

  ship->viewscreen = room;

  STRFREE( room->name );
  room->name = STRALLOC( "The Bridge" );

  strcpy( buf,
      "A large panoramic viewscreen gives you a good view of everything\r\n" );
  strcat( buf,
      "outside of the ship. Several computer terminals provide information\r\n" );
  strcat( buf,
      "and status readouts. There are also consoles dedicated to scanning\r\n" );
  strcat( buf, "communications and navigation.\r\n" );
  STRFREE( room->description );
  room->description = STRALLOC( buf );
}

void make_pilot( ROOM_INDEX_DATA * room, SHIP_DATA * ship )
{
  char buf[MAX_STRING_LENGTH];

  ship->pilotseat = room;
  room->tunnel = 1;

  STRFREE( room->name );
  room->name = STRALLOC( "The Pilots Chair" );

  strcpy( buf,
      "The console in front of you contains the spacecrafts primary flight\r\n" );
  strcat( buf,
      "controls. All of the ships propulsion and directional controls are located\r\n" );
  strcat( buf, "here including the hyperdrive controls.\r\n" );
  STRFREE( room->description );
  room->description = STRALLOC( buf );
}

void make_copilot( ROOM_INDEX_DATA * room, SHIP_DATA * ship )
{
  char buf[MAX_STRING_LENGTH];

  ship->gunseat = room;
  room->tunnel = 1;

  STRFREE( room->name );
  room->name = STRALLOC( "The Gunners Chair" );

  strcpy( buf,
      "This is where the action is. All of the systems main offensive and defensive\r\n" );
  strcat( buf,
      "controls are located here. There are targetting computers for the main laser\r\n" );
  strcat( buf,
      "batteries and missile launchers as well as shield controls.\r\n" );
  STRFREE( room->description );
  room->description = STRALLOC( buf );
}

void make_engine( ROOM_INDEX_DATA * room, SHIP_DATA * ship )
{
  char buf[MAX_STRING_LENGTH];

  ship->engine = room;

  STRFREE( room->name );
  room->name = STRALLOC( "The Engine Room" );

  strcpy( buf,
      "Many large mechanical parts litter this room. Throughout the room dozens\r\n" );
  strcat( buf,
      "of small glowing readouts and computer terminals monitor and provide information\r\n" );
  strcat( buf,
      "about all of the ships systems. Engineering is the heart of the ship. If\r\n" );
  strcat( buf,
      "something is damaged fixing it will almost always start here.\r\n" );
  STRFREE( room->description );
  room->description = STRALLOC( buf );
}

void make_elevator( ROOM_INDEX_DATA * room, SHIP_DATA * ship )
{
  char buf[MAX_STRING_LENGTH];

  STRFREE( room->name );
  room->name = STRALLOC( "Entrance / Lift" );

  strcpy( buf,
      "This spacious lift provides access to all of the ships decks as well\r\n" );
  strcpy( buf, "as doubling as the main exit.\r\n\r\n" );
  strcpy( buf, "A sign on the wall lists several lift destinations:\r\n" );
  strcat( buf, "\r\n" );
  strcat( buf, "Navigation\r\n" );
  strcat( buf, "Turrets\r\n" );
  strcat( buf, "Lounge\r\n" );
  strcat( buf, "Medical\r\n" );
  strcat( buf, "Engineering\r\n" );
  strcat( buf, "Hangers\r\n" );
  STRFREE( room->description );
  room->description = STRALLOC( buf );
}

void make_medical( ROOM_INDEX_DATA * room, SHIP_DATA * ship )
{
  char buf[MAX_STRING_LENGTH];

  STRFREE( room->name );
  room->name = STRALLOC( "Medical Bay " );

  strcpy( buf, "\r\n" );
  strcat( buf, "This medical bay is out of order.\r\n" );
  strcat( buf, "\r\n" );
  STRFREE( room->description );
  room->description = STRALLOC( buf );
}

void make_lounge( ROOM_INDEX_DATA * room, SHIP_DATA * ship )
{
  char buf[MAX_STRING_LENGTH];

  STRFREE( room->name );
  room->name = STRALLOC( "Crew Lounge" );

  strcpy( buf, "\r\n" );
  strcat( buf, "The crew lounge needs to be furnished still.\r\n" );
  strcat( buf, "\r\n" );
  STRFREE( room->description );
  room->description = STRALLOC( buf );
}

void make_entrance( ROOM_INDEX_DATA * room, SHIP_DATA * ship )
{
  char buf[MAX_STRING_LENGTH];

  ship->entrance = room;

  STRFREE( room->name );
  room->name = STRALLOC( "The Entrance Ramp" );

  strcpy( buf, "\r\n" );
  strcat( buf, "Don't you wish durga would finish his descriptions.\r\n" );
  strcat( buf, "\r\n" );
  STRFREE( room->description );
  room->description = STRALLOC( buf );
}

void make_hanger( ROOM_INDEX_DATA * room, SHIP_DATA * ship )
{
  char buf[MAX_STRING_LENGTH];

  STRFREE( room->name );
  room->name = STRALLOC( "A Hanger" );

  strcpy( buf, "\r\n" );
  strcat( buf, "This hanger is out of order.\r\n" );
  strcat( buf, "\r\n" );
  STRFREE( room->description );
  room->description = STRALLOC( buf );
}

void make_garage( ROOM_INDEX_DATA * room, SHIP_DATA * ship )
{
  char buf[MAX_STRING_LENGTH];

  STRFREE( room->name );
  room->name = STRALLOC( "Vehicle Garage" );

  strcpy( buf, "\r\n" );
  strcat( buf, "This garage is out of order.\r\n" );
  strcat( buf, "\r\n" );
  STRFREE( room->description );
  room->description = STRALLOC( buf );
}

void do_designship( CHAR_DATA * ch, char *argument )
{
  int hull = 0, energy = 0, shield = 0, speed = 0, manuever = 0, lasers =
    0, missiles = 0, chaff = 0, smodel = 0;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  char arg4[MAX_INPUT_LENGTH];
  char arg5[MAX_INPUT_LENGTH];
  char arg6[MAX_INPUT_LENGTH];
  char arg7[MAX_INPUT_LENGTH];
  char arg8[MAX_INPUT_LENGTH];
  char arg9[MAX_INPUT_LENGTH];
  char arg0[MAX_INPUT_LENGTH];
  long price = 0;
  SHIP_DATA *ship = NULL;
  ROOM_INDEX_DATA *location = NULL;

  if( IS_NPC( ch ) || !ch->pcdata )
    return;

  if( !ch->desc )
    return;

  switch ( ch->substate )
  {
    default:
      break;
    case SUB_ROOM_DESC:
      location = ( ROOM_INDEX_DATA * ) ch->dest_buf;
      if( !location )
      {
	bug( "designship decorate: sub_room_desc: NULL ch->dest_buf", 0 );
	stop_editing( ch );
	ch->substate = ch->tempnum;
	return;
      }

      STRFREE( location->description );
      location->description = copy_buffer( ch );

      stop_editing( ch );
      ch->substate = ch->tempnum;

      ship = ( ship_from_room( location ) );

      if( ship )
	save_ship( ship );

      return;
  }

  if( character_skill_level( ch, gsn_shipdesign ) <= 0 )
  {
    send_to_char( "You have no idea how to do that..\r\n", ch );
    return;
  }

  if( argument[0] == '\0' )
  {
    send_to_char
      ( "This skill may be used in one of the following ways:\r\n", ch );
    send_to_char( "\r\n", ch );
    send_to_char( "DESIGNSHIP DECORATE\r\n", ch );
    send_to_char
      ( "DESIGNSHIP TEST      <shipstats> 'model' <prototype_name>\r\n",
	ch );
    send_to_char
      ( "DESIGNSHIP PROTOTYPE <shipstats> 'model' <prototype_name>\r\n",
	ch );
    send_to_char
      ( "DESIGNSHIP CLANSHIP  <shipstats> 'model' <ship_name>\r\n", ch );
    send_to_char
      ( "DESIGNSHIP PERSONAL  <shipstats> 'model' <ship_name>\r\n", ch );
    send_to_char( "DESIGNSHIP CUSTOM    <custom design info>\r\n", ch );
    send_to_char( "\r\n", ch );
    send_to_char
      ( "Shipstats are a list of numbers separated by spaces representing:\r\n",
	ch );
    send_to_char
      ( "hull energy shield speed manuever lasers missiles chaff\r\n", ch );
    send_to_char( "\r\n", ch );
    send_to_char( "Model may be one of the following:\r\n", ch );
    send_to_char
      ( "'short range fighter'   'shuttle'           'transport'\r\n", ch );
    send_to_char
      ( "'long range fighter'    'assault shuttle'   'assault transport'\r\n",
	ch );
    send_to_char
      ( "'corvette'              'frigate'           'destroyer'\r\n", ch );
    send_to_char
      ( "'cruiser'               'battlecruiser'     'flagship'\r\n", ch );
    send_to_char( "\r\n", ch );
    send_to_char( "For example:\r\n", ch );
    send_to_char
      ( "designship personal 100 1000 50 100 100 2 0 0 'shuttle' Durgas Shuttle\r\n",
	ch );
    send_to_char( "\r\n", ch );
    send_to_char
      ( "type 'designship custom' for details on custom design.\r\n", ch );
    return;
  }

  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );
  argument = one_argument( argument, arg3 );
  argument = one_argument( argument, arg4 );
  argument = one_argument( argument, arg5 );
  argument = one_argument( argument, arg6 );
  argument = one_argument( argument, arg7 );
  argument = one_argument( argument, arg8 );
  argument = one_argument( argument, arg9 );
  argument = one_argument( argument, arg0 );

  if( !str_cmp( arg1, "decorate" ) )
  {
    if( !ch->in_room )
      return;

    ship = ( ship_from_room( ch->in_room ) );

    if( !ship )
    {
      send_to_char( "&RYou must be inside a ship to do that.\r\n", ch );
      return;
    }

    if( !check_pilot( ch, ship ) )
    {
      send_to_char( "&RThis isn't your ship to decorate.\r\n", ch );
      return;
    }

    ch->substate = SUB_ROOM_DESC;
    ch->dest_buf = ch->in_room;
    start_editing( ch, ch->in_room->description );
    return;
  }

  if( !ch->in_room || !IS_SET( ch->in_room->room_flags, ROOM_SHIPYARD ) )
  {
    send_to_char( "&RYou must be in a shipyard to do that.\r\n", ch );
    return;
  }

  if( arg0[0] == '\0' )
  {
    if( !str_cmp( arg1, "custom" ) )
    {
      send_to_char( "Rules for custom design:\r\n", ch );
      send_to_char( "\r\n", ch );
      send_to_char
	( "DESIGNSHIP CUSTOM <shipstats> <layout> <ship_name>\r\n", ch );
      send_to_char( "\r\n", ch );
      send_to_char
	( "Shipstats are a list of numbers separated by spaces representing:\r\n",
	  ch );
      send_to_char
	( "hull energy shield speed manuever lasers missiles chaff\r\n",
	  ch );
      send_to_char( "\r\n", ch );
      send_to_char
	( "Layout is a string of letters that a robot uses as commands to build your\r\n",
	  ch );
      send_to_char
	( "ship. The robot starts at the entrance and uses the following commands:\r\n",
	  ch );
      send_to_char( "\r\n", ch );
      send_to_char
	( "n, e, w, s, u, d - move robot that direction and create a new room if needed\r\n",
	  ch );
      send_to_char
	( "c - make current room cockpit,          b - make current room bridge\r\n",
	  ch );
      send_to_char
	( "p - make current room pilot chair,      2 - make current room copilot chair\r\n",
	  ch );
      send_to_char
	( "m - make current room engine room,      + - make current room medical bay\r\n",
	  ch );
      send_to_char
	( "l - make current room crew lounge,      t - make current room turret\r\n",
	  ch );
      send_to_char
	( "h - make current room hanger,           v - make current room vehicle garage\r\n",
	  ch );
      send_to_char( "\r\n", ch );
      send_to_char( "For example:\r\n", ch );
      send_to_char
	( "designship custom 100 1000 50 100 100 2 0 0 endtunwncswsdtusw Durgas Shuttle\r\n",
	  ch );
      send_to_char( "\r\n", ch );
      send_to_char
	( "(would make a square shuttle, with a rear entrance, a cockpit at the front\r\nand belly turrets on the sides)\r\n",
	  ch );
      return;
    }
    else
    {
      do_designship( ch, STRLIT_EMPTY );
      return;
    }
  }

  if( !str_cmp( arg1, "custom" ) )
  {
    smodel = CUSTOM_SHIP;
  }
  else
  {
    int tm = 0;
    smodel = -1;

    for( tm = 0; tm < CUSTOM_SHIP; tm++ )
      if( !str_cmp( arg0, model[tm].name ) )
	smodel = tm;

    if( smodel < 0 )
    {
      do_designship( ch, STRLIT_EMPTY );
      return;
    }
  }

  hull = atoi( arg2 );
  energy = atoi( arg3 );
  shield = atoi( arg4 );
  speed = atoi( arg5 );
  manuever = atoi( arg6 );
  lasers = atoi( arg7 );
  missiles = atoi( arg8 );
  chaff = atoi( arg9 );

  if( hull < 50 || hull > model[smodel].hull )
  {
    ch_printf( ch, "&R%s hull values must be between 50 and %d.\r\n",
	model[smodel].name, model[smodel].hull );

    return;
  }

  if( energy < 500 || energy > model[smodel].energy )
  {
    ch_printf( ch, "&R%s energy values must be between 500 and %d.\r\n",
	model[smodel].name, model[smodel].energy );

    return;
  }

  if( shield < 0 || shield > model[smodel].shield )
  {
    ch_printf( ch, "&R%s shield values must be between 0 and %d.\r\n",
	model[smodel].name, model[smodel].shield );
    return;
  }

  if( speed < 10 || speed > model[smodel].realspeed )
  {
    ch_printf( ch, "&R%s speed values must be between 10 and %d.\r\n",
	model[smodel].name, model[smodel].realspeed );
    return;
  }

  if( manuever < 10 || manuever > model[smodel].manuever )
  {
    ch_printf( ch, "&R%s manuever values must be between 10 and %d.\r\n",
	model[smodel].name, model[smodel].manuever );
    return;
  }

  if( lasers < 0 || lasers > model[smodel].lasers )
  {
    ch_printf( ch, "&R%s lasers must be between 0 and %d.\r\n",
	model[smodel].name, model[smodel].lasers );
    return;
  }

  if( missiles < 0 || missiles > model[smodel].missiles )
  {
    ch_printf( ch, "&R%s missiles must be between 0 and %d.\r\n",
	model[smodel].name, model[smodel].missiles );
    return;
  }

  if( chaff < 0 || chaff > 20 )
  {
    ch_printf( ch, "&R%s chaff must be between 0 and 20.\r\n",
	model[smodel].name );
    return;
  }

  price =
    get_design_value( hull, energy, shield, speed, manuever, lasers, missiles,
	chaff, smodel );

  if( !str_cmp( arg1, "test" ) )
  {
    ch_printf( ch, "&YName:     &W%s\r\n", argument );
    ch_printf( ch, "&YModel:    &W%s\r\n", model[smodel].name );
    ch_printf( ch, "\r\n" );
    ch_printf( ch, "&YHull:     &W%d\r\n", hull );
    ch_printf( ch, "&YEnergy:   &W%d\r\n", energy );
    ch_printf( ch, "&YShield:   &W%d\r\n", shield );
    ch_printf( ch, "&YSpeed:    &W%d\r\n", speed );
    ch_printf( ch, "&YManuever: &W%d\r\n", manuever );
    ch_printf( ch, "&YLasers:   &W%d\r\n", lasers );
    ch_printf( ch, "&YMissiles: &W%d\r\n", missiles );
    ch_printf( ch, "&YChaff:    &W%d\r\n", chaff );
    ch_printf( ch, "\r\n" );
    ch_printf( ch, "&YThis ship would cost &W%d&Y credits.\r\n", price );
    return;
  }

  if( !str_cmp( arg1, "clanship" ) || !str_cmp( arg1, "personal" ) )
  {
    int shipreg = 0;
    char filename[10];
    char shipname[MAX_STRING_LENGTH];
    CLAN_DATA *clan = NULL;

    if( !str_cmp( arg1, "clanship" ) )
    {
      if( !ch->pcdata || !ch->pcdata->clan )
      {
	ch_printf( ch, "&RYou don't belong to a clan....\r\n" );
	return;
      }

      clan = ch->pcdata->clan;

      if( price > clan->funds )
      {
	ch_printf( ch,
	    "&RThat ship would cost %d. Your organization doesn't have enough.\r\n",
	    price );
	return;
      }
    }
    else
    {
      if( price > ch->gold )
      {
	ch_printf( ch,
	    "&RThat ship would cost %d. You don't have enough.\r\n",
	    price );
	return;
      }
    }

    for( ship = first_ship; ship; ship = ship->next )
      if( shipreg < atoi( ship->filename ) )
	shipreg = atoi( ship->filename );

    shipreg++;
    sprintf( filename, "%d", shipreg );
    sprintf( shipname, "%s %d", argument, shipreg );
    ship = ship_create();

    LINK( ship, first_ship, last_ship, next, prev );
    ship->filename = str_dup( filename );
    ship->name = STRALLOC( shipname );
    ship->home = STRALLOC( "" );

    if( !str_cmp( arg1, "clanship" ) )
      ship->owner = STRALLOC( clan->name );
    else
      ship->owner = STRALLOC( ch->name );

    ship->pilot = STRALLOC( "" );
    ship->copilot = STRALLOC( "" );
    ship->type = PLAYER_SHIP;
    ship->ship_class = SPACECRAFT;
    ship->model = smodel;
    ship->hyperspeed = model[smodel].hyperspeed;
    ship->realspeed = speed;
    ship->shipstate = SHIP_DOCKED;
    ship->missiles = missiles;
    ship->maxmissiles = missiles;
    ship->lasers = lasers;
    ship->manuever = manuever;
    ship->maxenergy = energy;
    ship->energy = energy;
    ship->maxshield = shield;
    ship->hull = hull;
    ship->maxhull = hull;
    ship->chaff = chaff;
    ship->maxchaff = chaff;
    create_ship_rooms( ship );

    if( !str_cmp( arg1, "clanship" ) )
    {
      clan->funds -= price;
      ch_printf( ch, "&G%s pays %ld credits for the ship.\r\n",
	  clan->name, price );
    }
    else
    {
      ch->gold -= price;
      ch_printf( ch, "&GYou pay %ld credits to design the ship.\r\n",
	  price );
    }

    act( AT_PLAIN,
	"$n walks over to a terminal and makes a credit transaction.", ch,
	NULL, argument, TO_ROOM );

    ship_to_room( ship, ch->in_room->vnum );
    ship->location = ch->in_room->vnum;
    ship->lastdoc = ch->in_room->vnum;
    save_ship( ship );
    write_ship_list();

    learn_from_success( ch, gsn_shipdesign );
    learn_from_success( ch, gsn_shipdesign );
    learn_from_success( ch, gsn_shipdesign );
    learn_from_success( ch, gsn_shipdesign );
    learn_from_success( ch, gsn_shipdesign );
    return;
  }

  send_to_char( "Unfortunately this skill isn't finished yet....\r\n", ch );
}

void post_ship_guard( ROOM_INDEX_DATA * pRoomIndex )
{
  MOB_INDEX_DATA *pMobIndex = NULL;
  CHAR_DATA *mob = NULL;
  char tmpbuf[MAX_STRING_LENGTH];
  OBJ_DATA *blaster = NULL;
  OBJ_INDEX_DATA *pObjIndex = NULL;

  if( !( pMobIndex = get_mob_index( MOB_VNUM_SHIP_GUARD ) ) )
  {
    bug( "Reset_all: Missing default guard (%d)", MOB_VNUM_SHIP_GUARD );
    return;
  }

  mob = create_mobile( pMobIndex );
  char_to_room( mob, pRoomIndex );
  mob->top_level = 100;
  mob->hit = 250;
  mob->max_hit = 250;
  mob->armor = 0;
  mob->damroll = 0;
  mob->hitroll = 20;

  if( ( pObjIndex = get_obj_index( OBJ_VNUM_BLASTER ) ) != NULL )
  {
    blaster = create_object( pObjIndex );
    obj_to_char( blaster, mob );
    equip_char( mob, blaster, WEAR_WIELD );
  }

  do_setblaster( mob, STRLIT_FULL );

  if( room_is_dark( pRoomIndex ) )
    SET_BIT( mob->affected_by, AFF_INFRARED );

  sprintf( tmpbuf,
      "A Security Guard stands alert and ready for trouble.\r\n" );
  STRFREE( mob->long_descr );
  mob->long_descr = STRALLOC( tmpbuf );
  mob->mob_clan = NULL;
}
