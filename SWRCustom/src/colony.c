#include "mud.h"
#include <string.h>

void reset_room( ROOM_INDEX_DATA *pRoomIndex );

const int MAX_COLONY_SIZE = 2000;
const int CONSTRUCTION_COST = 500;

extern int top_r_vnum;

typedef void ROOM_TYPE_BUILDER( ROOM_INDEX_DATA* );

ROOM_TYPE_BUILDER *get_room_type_builder( int room_type );

typedef struct room_type_data
{
  const char *name;
  const char *description;
  ROOM_TYPE_BUILDER *build_room;
} ROOM_TYPE_DATA;

void room_set_sector( ROOM_INDEX_DATA *room, int sector_type )
{
  room->sector_type = sector_type;

  if( room->area->planet )
    {
      switch( sector_type )
        {
        case SECT_INSIDE:
          room->area->planet->citysize++;
          break;

        case SECT_CITY:
          room->area->planet->citysize++;
          break;

        case SECT_FARMLAND:
          room->area->planet->farmland++;
          break;

        case SECT_UNDERGROUND:
          room->area->planet->wilderness++;
          break;

        default:
          break;
        }
    }
}

static void build_room_type_dummy( ROOM_INDEX_DATA *foo )
{

}

static void build_room_type_city( ROOM_INDEX_DATA *location )
{
  room_set_sector( location, SECT_CITY );
}

static void build_room_type_wilderness( ROOM_INDEX_DATA *location )
{
  location->area->planet->wilderness++;
  location->sector_type = location->area->planet->sector;
}

static void build_room_type_inside( ROOM_INDEX_DATA *location )
{
  room_set_sector( location, SECT_INSIDE );
  SET_BIT( location->room_flags, ROOM_INDOORS );
}

static void build_room_type_farmland( ROOM_INDEX_DATA *location )
{
  room_set_sector( location, SECT_FARMLAND );
}

static void build_room_type_platform( ROOM_INDEX_DATA *location )
{
  room_set_sector( location, SECT_CITY );
  SET_BIT( location->room_flags, ROOM_CAN_LAND );
}

static void build_room_type_shipyard( ROOM_INDEX_DATA *location )
{
  room_set_sector( location, SECT_CITY );
  SET_BIT( location->room_flags, ROOM_SHIPYARD );
}

static void build_room_type_house( ROOM_INDEX_DATA *location )
{
  room_set_sector( location, SECT_INSIDE );
  SET_BIT( location->room_flags, ROOM_EMPTY_HOME );
  SET_BIT( location->room_flags, ROOM_NO_MOB );
}

static void build_room_type_cave( ROOM_INDEX_DATA *location )
{
  room_set_sector( location, SECT_UNDERGROUND );
  SET_BIT( location->room_flags, ROOM_DARK );
}

static void build_room_type_info( ROOM_INDEX_DATA *location )
{
  room_set_sector( location, SECT_INSIDE );
  SET_BIT( location->room_flags, ROOM_INFO );
  SET_BIT( location->room_flags, ROOM_NO_MOB );
}

static void build_room_type_mail( ROOM_INDEX_DATA *location )
{
  room_set_sector( location, SECT_INSIDE );
  SET_BIT( location->room_flags, ROOM_MAIL );
  SET_BIT( location->room_flags, ROOM_NO_MOB );
}

static void build_room_type_bank( ROOM_INDEX_DATA *location )
{
  room_set_sector( location, SECT_INSIDE );
  SET_BIT( location->room_flags, ROOM_BANK );
}

static void build_room_type_hotel( ROOM_INDEX_DATA *location )
{
  room_set_sector( location, SECT_INSIDE );
  SET_BIT( location->room_flags, ROOM_HOTEL );
  SET_BIT( location->room_flags, ROOM_SAFE );
  SET_BIT( location->room_flags, ROOM_NO_MOB );
}

static void build_room_type_trade( ROOM_INDEX_DATA *location )
{
  room_set_sector( location, SECT_INSIDE );
  SET_BIT( location->room_flags, ROOM_SAFE );
  SET_BIT( location->room_flags, ROOM_TRADE );
}

static void build_room_type_supply( ROOM_INDEX_DATA *location )
{
  room_set_sector( location, SECT_INSIDE );
  SET_BIT( location->room_flags, ROOM_SAFE );
  SET_BIT( location->room_flags, ROOM_SUPPLY );
}

static void build_room_type_pawn( ROOM_INDEX_DATA *location )
{
  room_set_sector( location, SECT_INSIDE );
  SET_BIT( location->room_flags, ROOM_SAFE );
  SET_BIT( location->room_flags, ROOM_PAWN );
}

static void build_room_type_restaurant( ROOM_INDEX_DATA *location )
{
  room_set_sector( location, SECT_INSIDE );
  SET_BIT( location->room_flags, ROOM_RESTAURANT );
}

static void build_room_type_bar( ROOM_INDEX_DATA *location )
{
  room_set_sector( location, SECT_INSIDE );
  SET_BIT( location->room_flags, ROOM_BAR );
}

static void build_room_type_control( ROOM_INDEX_DATA *location )
{
  room_set_sector( location, SECT_INSIDE );
  location->area->planet->controls++;
  SET_BIT( location->room_flags, ROOM_CONTROL );
}

static void build_room_type_barracks( ROOM_INDEX_DATA *location )
{
  room_set_sector( location, SECT_INSIDE );
  location->area->planet->barracks++;
  SET_BIT( location->room_flags, ROOM_BARRACKS );
}

static void build_room_type_garage( ROOM_INDEX_DATA *location )
{
  room_set_sector( location, SECT_INSIDE );
  SET_BIT( location->room_flags, ROOM_GARAGE );
}

static void build_room_type_employment( ROOM_INDEX_DATA *location )
{
  room_set_sector( location, SECT_INSIDE );
  SET_BIT( location->room_flags, ROOM_EMPLOYMENT );
}

static void build_room_type_medical( ROOM_INDEX_DATA *location )
{
  room_set_sector( location, SECT_INSIDE );
  SET_BIT( location->room_flags, ROOM_MEDICAL );
  SET_BIT( location->room_flags, ROOM_SAFE );
}

void make_default_colony_supply_shop( PLANET_DATA *planet,
				      ROOM_INDEX_DATA *location )
{
  static const char *room_descr =
    "This visible part of this shop consists of a long desk with a couple\r\n"
    "of computer terminals located along its length. A large set of sliding\r\n"
    "doors conceals the supply room behind. In front of the main desk a\r\n"
    "smaller circular desk houses several mail terminals as the shop also\r\n"
    "doubles as a post office. This shop stocks an assortment of basic\r\n"
    "supplies useful to both travellers and settlers. The shopkeeper will\r\n"
    "also purchase some items that might have some later resale or trade\r\n"
    "value.\r\n";

  location->name = STRALLOC( "Supply Shop" );
  location->description = STRALLOC( room_descr );
  room_set_sector( location, SECT_INSIDE );
  SET_BIT( location->room_flags, ROOM_INDOORS );
  SET_BIT( location->room_flags, ROOM_NO_MOB );
  SET_BIT( location->room_flags, ROOM_SAFE );
  SET_BIT( location->room_flags, ROOM_NOPEDIT );
  SET_BIT( location->room_flags, ROOM_MAIL );
  SET_BIT( location->room_flags, ROOM_TRADE );
  SET_BIT( location->room_flags, ROOM_BANK );
}

void make_default_colony_center( PLANET_DATA *planet,
				 ROOM_INDEX_DATA *location )
{
  char buf[MAX_STRING_LENGTH];
  static const char *room_descr = 
    "You stand in the main foyer of the colonization center. This is one of\r\n"
    "many similar buildings scattered on planets throughout the galaxy. It and\r\n"
    "the others like it serve two main purposes. The first is as an initial\r\n"
    "living and working space for early settlers to the planet. It provides\r\n"
    "a center of operations during the early stages of a colony while the\r\n"
    "surrounding area is being developed. Its second purpose after the\r\n"
    "initial community has been settled is to provide an information center\r\n"
    "for new citizens and for tourists. Food, transportation, shelter, \r\n"
    "supplies, and information are all contained in one area making it easy\r\n"
    "for those unfamiliar with the planet to adjust. This also makes it a\r\n"
    "very crowded place at times.\r\n";

  snprintf( buf, MAX_STRING_LENGTH, "%s: Colonization Center", planet->name );
  location->name = STRALLOC( buf );
  location->description = STRALLOC( room_descr );
  room_set_sector( location, SECT_INSIDE );
  SET_BIT( location->room_flags, ROOM_INDOORS );
  SET_BIT( location->room_flags, ROOM_NO_MOB );
  SET_BIT( location->room_flags, ROOM_SAFE );
  SET_BIT( location->room_flags, ROOM_NOPEDIT );
  SET_BIT( location->room_flags, ROOM_INFO );
}

void make_default_colony_shuttle_platform( PLANET_DATA *planet,
					   ROOM_INDEX_DATA *location )
{
  static const char *room_descr =
    "This platform is large enough for several spacecraft to land and take off\r\n"
    "from. Its surface is a hard glossy substance that is mostly smooth except\r\n"
    "a few ripples and impressions that suggest its liquid origin. Power boxes\r\n"
    "are scattered about the platform strung together by long strands of thick\r\n"
    "power cables and fuel hoses. Glowing strips divide the platform into\r\n"
    "multiple landing areas. Hard rubber pathways mark pathways for pilots and\r\n"
    "passengers, leading from the various landing areas to the Colonization\r\n"
    "Center.\r\n";
  location->description = STRALLOC( room_descr );
  location->name = STRALLOC( "Community Shuttle Platform" );
  room_set_sector( location, SECT_CITY );
  SET_BIT( location->room_flags, ROOM_SHIPYARD );
  SET_BIT( location->room_flags, ROOM_CAN_LAND );
  SET_BIT( location->room_flags, ROOM_NO_MOB );
  SET_BIT( location->room_flags, ROOM_NOPEDIT );
}

void make_default_colony_hotel( PLANET_DATA *planet,
				ROOM_INDEX_DATA *location )
{
  char buf[MAX_STRING_LENGTH];
  static const char *room_descr =
    "This part of the center serves as a temporary home for new settlers\r\n"
    "until a more permanent residence is found. It is also used as a hotel\r\n"
    "for tourists and visitors. the shape of the hotel is circular with rooms\r\n"
    "located around the perimeter extending several floors above ground level.\r\n"
    "\r\nThis is a good place to rest. You may safely leave and reenter the\r\n"
    "game from here.\r\n";

  location->description = STRALLOC( room_descr );
  snprintf( buf, MAX_STRING_LENGTH, "%s: Center Hotel", planet->name );
  location->name = STRALLOC( buf );
  room_set_sector( location, SECT_INSIDE );
  SET_BIT( location->room_flags, ROOM_INDOORS );
  SET_BIT( location->room_flags, ROOM_HOTEL );
  SET_BIT( location->room_flags, ROOM_SAFE );
  SET_BIT( location->room_flags, ROOM_NO_MOB );
  SET_BIT( location->room_flags, ROOM_NOPEDIT );
}

void make_default_colony_wilderness( PLANET_DATA *planet,
				     ROOM_INDEX_DATA *location,
				     const char *description )
{
  location->description = STRALLOC( description );
  location->name = STRALLOC( planet->name );
  room_set_sector( location, planet->sector );
}

void make_colony_room_exits( ROOM_INDEX_DATA *location, int room_type )
{
  if( room_type > 5
      && room_type != 17
      && room_type != COLONY_ROOM_SUPPLY_SHOP
      && room_type != COLONY_ROOM_SHUTTLE_PLATFORM
      && room_type != 19
      && room_type != 23 )
    {
      ROOM_INDEX_DATA *troom = get_room_index( top_r_vnum - 5 );
      make_bexit( location, troom, DIR_NORTH );
    }

  if( room_type != COLONY_ROOM_FIRST
      && room_type != 6
      && room_type != 11
      && room_type != COLONY_ROOM_SUPPLY_SHOP
      && room_type != 15
      && room_type != 16
      && room_type != COLONY_ROOM_HOTEL
      && room_type != 19
      && room_type != 21 )
    {
      ROOM_INDEX_DATA *troom = get_room_index( top_r_vnum - 1 );
      make_bexit( location, troom, DIR_WEST );
    }
}

static const ROOM_TYPE_DATA room_type_table[] = {
  /*  0: doesn't exist */
  { "_zero", "zero", build_room_type_dummy },

  /*  1: COLONY_ROOM_WILDERNESS */
  { "wilderness", "the planet's default terrain", build_room_type_wilderness },

  /*  2: COLONY_ROOM_FARMLAND */
  { "farmland", "cleared farmland", build_room_type_farmland },

  /*  3: COLONY_ROOM_CITY_STREET */
  { "city", "a city street", build_room_type_city },

  /*  4: COLONY_ROOM_SHIPYARD */
  { "shipyard", "ships are built here", build_room_type_shipyard },

  /*  5: COLONY_ROOM_INSIDE */
  { "inside", "inside a building", build_room_type_inside },

  /*  6: COLONY_ROOM_HOUSE */
  { "house", "may be used as a player's home", build_room_type_house },

  /*  7: COLONY_ROOM_CAVE */
  { "cave", "a mine or dug out tunnel", build_room_type_cave },

  /*  8: COLONY_ROOM_INFO */
  { "info", "message and information room", build_room_type_info },

  /*  9: COLONY_ROOM_MAIL */
  { "mail", "post office", build_room_type_mail },

  /* 10: COLONY_ROOM_TRADE */
  { "trade", "players can sell resources here", build_room_type_trade },

  /* 11: COLONY_ROOM_PAWN */
  { "pawn", "will trade useful items", build_room_type_pawn },

  /* 12: COLONY_ROOM_SUPPLY_SHOP */
  { "supply", "a supply store", build_room_type_supply },

  /* 13: COLONY_ROOM_COLONIZATION_CENTER */
  { "center", "the colonization center", build_room_type_dummy },

  /* 14: COLONY_ROOM_SHUTTLE_PLATFORM */
  { "platform", "ships land here", build_room_type_platform },

  /* 15: COLONY_ROOM_RESTAURANT */
  { "restaurant", "food is bought here", build_room_type_restaurant },

  /* 16: COLONY_ROOM_BAR */
  { "bar", "liquor is sold here", build_room_type_bar },

  /* 17: COLONY_ROOM_CONTROL */
  { "control", "control tower for patrol ships", build_room_type_control },

  /* 18: COLONY_ROOM_HOTEL */
  { "hotel", "players can enter/leave game here", build_room_type_hotel },

  /* 19: COLONY_ROOM_BARRACKS */
  { "barracks", "houses military patrols", build_room_type_barracks },

  /* 20: COLONY_ROOM_GARAGE */
  { "garage", "vehicles are built here", build_room_type_garage },

  /* 21: COLONY_ROOM_BANK */
  { "bank", "room is a bank", build_room_type_bank },

  /* 22: COLONY_ROOM_EMPLOYMENT */
  { "employment", "room is an employment office", build_room_type_employment },

  /* 23: COLONY_ROOM_UNUSED23*/
  { "clinic", "players can clone here", build_room_type_medical },

  /* 24: COLONY_ROOM_UNUSED24 */
  { "_unused24", "_unused24", build_room_type_dummy },

  /* 25: COLONY_ROOM_LAST */
  { "_last", "last", build_room_type_dummy }
};

size_t room_type_table_size( void )
{
  return sizeof( room_type_table ) / sizeof( *room_type_table );
}

int get_room_type_id( const char *name )
{
  size_t x = 0;

  for( x = 0; x < room_type_table_size(); ++x )
    if( !str_cmp( name, room_type_table[x].name ) )
      return x;

  return -1;
}

const ROOM_TYPE_DATA *get_room_type( int n )
{
  if( n < COLONY_ROOM_FIRST || n > COLONY_ROOM_LAST )
    return NULL;

  return &room_type_table[n];
}

const char *get_room_type_name( int room_type )
{
  if( room_type < COLONY_ROOM_FIRST || room_type > COLONY_ROOM_LAST )
    return "*out of bounds*";

  return room_type_table[room_type].name;
}

const char *get_room_type_description( int room_type )
{
  if( room_type < COLONY_ROOM_FIRST || room_type > COLONY_ROOM_LAST )
    return "_out of bounds_";

  return room_type_table[room_type].description;
}

ROOM_TYPE_BUILDER *get_room_type_builder( int room_type )
{
  if( room_type < COLONY_ROOM_FIRST || room_type > COLONY_ROOM_LAST )
    return build_room_type_dummy;

  return room_type_table[room_type].build_room;
}

bool room_type_in_use( size_t n )
{
  const char *name = get_room_type_name( n );
  return name[0] != '_' && n > 0 && n != COLONY_ROOM_COLONIZATION_CENTER;
}

void clear_roomtype( ROOM_INDEX_DATA * location )
{
  if( location->area->planet )
    {
      if( location->sector_type <= SECT_CITY )
	location->area->planet->citysize--;
      else if( location->sector_type == SECT_FARMLAND )
	location->area->planet->farmland--;
      else if( location->sector_type != SECT_DUNNO )
	location->area->planet->wilderness--;

      if( IS_SET( location->room_flags, ROOM_BARRACKS ) )
	location->area->planet->barracks--;
      if( IS_SET( location->room_flags, ROOM_CONTROL ) )
	location->area->planet->controls--;
    }

  REMOVE_BIT( location->room_flags, ROOM_NO_MOB );
  REMOVE_BIT( location->room_flags, ROOM_HOTEL );
  REMOVE_BIT( location->room_flags, ROOM_SAFE );
  REMOVE_BIT( location->room_flags, ROOM_CAN_LAND );
  REMOVE_BIT( location->room_flags, ROOM_SHIPYARD );
  REMOVE_BIT( location->room_flags, ROOM_EMPTY_HOME );
  REMOVE_BIT( location->room_flags, ROOM_DARK );
  REMOVE_BIT( location->room_flags, ROOM_INFO );
  REMOVE_BIT( location->room_flags, ROOM_MAIL );
  REMOVE_BIT( location->room_flags, ROOM_TRADE );
  REMOVE_BIT( location->room_flags, ROOM_SUPPLY );
  REMOVE_BIT( location->room_flags, ROOM_PAWN );
  REMOVE_BIT( location->room_flags, ROOM_RESTAURANT );
  REMOVE_BIT( location->room_flags, ROOM_BAR );
  REMOVE_BIT( location->room_flags, ROOM_CONTROL );
  REMOVE_BIT( location->room_flags, ROOM_BARRACKS );
  REMOVE_BIT( location->room_flags, ROOM_GARAGE );
  REMOVE_BIT( location->room_flags, ROOM_BANK );
  REMOVE_BIT( location->room_flags, ROOM_EMPLOYMENT );
  REMOVE_BIT( location->room_flags, ROOM_MEDICAL );
}

/*
 * In game commands
 */
void do_landscape( CHAR_DATA * ch, char *argument )
{
  CLAN_DATA *clan = NULL;
  ROOM_INDEX_DATA *location = NULL;
  int chance = 0;
  char arg[MAX_INPUT_LENGTH];
  char filename[256];
  int room_type_id = 0;

  if( IS_NPC( ch ) || !ch->pcdata || !ch->desc )
    return;

  switch( ch->substate )
    {
    default:
      break;

    case SUB_ROOM_DESC:
      location = ( ROOM_INDEX_DATA * ) ch->dest_buf;

      if( !location )
	{
	  bug( "landscape: sub_room_desc: NULL ch->dest_buf", 0 );
	  location = ch->in_room;
	}

      STRFREE( location->description );
      location->description = copy_buffer( ch );
      stop_editing( ch );
      ch->substate = ch->tempnum;

      if( strlen( location->description ) > 150 || IS_IMMORTAL( ch ) )
	{
	  learn_from_success( ch, gsn_landscape );
	}
      else
	{
	  ch_printf( ch, "That room's description is too short.\r\n" );
	  ch_printf( ch, "You skill level diminishes with your lazyness.\r\n");

	  if( character_skill_level( ch, gsn_landscape ) > 50 )
	    modify_skill_level( ch, gsn_landscape, -5 );
	}

      SET_BIT( location->area->flags, AFLAG_MODIFIED );
      room_extract_contents( ch->in_room );
      echo_to_room( AT_WHITE, location,
		    "The construction crew finishes its work." );
      sprintf( filename, "%s%s", AREA_DIR, location->area->filename );
      fold_area( location->area, filename, FALSE );
      return;
    }

  location = ch->in_room;
  clan = ch->pcdata->clan;

  if( !clan )
    {
    send_to_char
      ( "You need to be part of an organization before you can do that!\r\n",
        ch );
    return;
    }

  if( !clan_char_has_permission( clan, ch, CLAN_PERM_BUILD ) )
    {
      send_to_char( "Your organization hasn't given you permission to edit their lands!\r\n", ch );
      return;
    }

  if( !location->area->planet || clan != location->area->planet->governed_by )
    {
      send_to_char( "You may only landscape areas on planets that your organization controls!\r\n", ch );
      return;
    }

  if( IS_SET( location->room_flags, ROOM_NOPEDIT ) )
    {
      send_to_char( "Sorry, But you may not edit this room.\r\n", ch );
      return;
    }

  argument = one_argument( argument, arg );

  if( argument[0] == '\0' )
    {
      size_t n = 0;
      send_to_char( "Usage: LANDSCAPE  <Room Type>  <New Room Name>\r\n",
		    ch );
      send_to_char( "<Room Type> may be one of the following:\r\n\r\n", ch );

      for( n = 0; n < COLONY_ROOM_LAST; ++n )
	{
	  if( room_type_in_use( n ) )
	    {
	      ch_printf( ch, "%-12s - %s\r\n",
			 get_room_type_name( n ),
			 get_room_type_description( n ) );
	    }
	}

      return;
    }

  chance = character_skill_level( ch, gsn_landscape );

  if( number_percent() > chance )
    {
      send_to_char( "You can't quite get the desired affect.\r\n", ch );
      return;
    }

  room_type_id = get_room_type_id( arg );

  if( room_type_id < 0 || !room_type_in_use( room_type_id ) )
    {
      ch_printf( ch, "That's not a valid room type.\r\n" );
      return;
    }

  clear_roomtype( location );

  if( IS_SET( location->room_flags, ROOM_PLR_HOME ) )
    {
      location->area->planet->citysize++;
      SET_BIT( location->room_flags, ROOM_NO_MOB );
      SET_BIT( location->room_flags, ROOM_HOTEL );
      SET_BIT( location->room_flags, ROOM_SAFE );
    }
  else
    {
      ROOM_TYPE_BUILDER *build_room = get_room_type_builder( room_type_id );
      build_room( location );
    }

  reset_room( location );
  echo_to_room( AT_WHITE, location,
		"A construction crew enters the room and begins to work." );

  STRFREE( location->name );
  location->name = STRALLOC( argument );

  ch->substate = SUB_ROOM_DESC;
  ch->dest_buf = location;
  STRFREE( location->description );
  location->description = STRALLOC( "" );
  start_editing( ch, location->description );
}

void do_construction( CHAR_DATA * ch, char *argument )
{
  CLAN_DATA *clan = NULL;
  int chance = 0, ll = 0;
  int edir = DIR_SOMEWHERE;
  ROOM_INDEX_DATA *nRoom = NULL;
  char buf[MAX_STRING_LENGTH];

  if( IS_NPC( ch ) || !ch->pcdata || !ch->in_room )
    return;

  clan = ch->pcdata->clan;

  if( !clan )
    {
      ch_printf( ch, "You need to be part of an organization before you can do that!\r\n" );
      return;
    }

  if( !clan_char_has_permission( clan, ch, CLAN_PERM_BUILD ) )
    {
      ch_printf( ch, "Your organization hasn't given you permission to build on their lands!\r\n" );
      return;
    }

  if( !ch->in_room->area || !ch->in_room->area->planet ||
      clan != ch->in_room->area->planet->governed_by )
    {
      ch_printf( ch, "You may only build on planets that your organization controls!\r\n" );
      return;
    }

  if( ch->in_room->area->planet->size >= MAX_COLONY_SIZE )
    {
      ch_printf( ch, "This planet is big enough. Go build somewhere else...\r\n" );
      return;
    }

  if( IS_SET( ch->in_room->room_flags, ROOM_NOPEDIT ) )
    {
      send_to_char( "Sorry, But you may not edit this room.\r\n", ch );
      return;
    }

  if( argument[0] == '\0' )
    {
      send_to_char( "Begin construction in what direction?\r\n", ch );
      return;
    }

  if( ch->gold < CONSTRUCTION_COST )
    {
      ch_printf( ch, "You do not have enough money. It will cost you %d credits to do that.\r\n", CONSTRUCTION_COST );
      return;
    }

  edir = get_dir( argument );

  if( get_exit( ch->in_room, edir ) )
    {
      send_to_char( "There is already a room in that direction.\r\n", ch );
      return;
    }

  chance = character_skill_level( ch, gsn_construction );

  if( number_percent() > chance )
    {
      send_to_char( "You can't quite get the desired affect.\r\n", ch );
      ch->gold -= CONSTRUCTION_COST / 100;
      return;
    }

  ch->gold -= CONSTRUCTION_COST;

  nRoom = make_room( ++top_r_vnum );
  nRoom->area = ch->in_room->area;
  LINK( nRoom, ch->in_room->area->first_room, ch->in_room->area->last_room,
	next_in_area, prev_in_area );
  STRFREE( nRoom->name );
  STRFREE( nRoom->description );
  nRoom->name = STRALLOC( "Construction Site" );
  nRoom->description = STRALLOC( "\r\nThis area is under construction.\r\nIt still needs some landscaping.\r\n\r\n" );
  nRoom->sector_type = SECT_DUNNO;
  SET_BIT( nRoom->room_flags, ROOM_NO_MOB );

  make_bexit( ch->in_room, nRoom, edir );

  ch->in_room->area->planet->size++;

  for( ll = 1; ll <= 20; ll++ )
    learn_from_success( ch, gsn_construction );

  SET_BIT( ch->in_room->area->flags, AFLAG_MODIFIED );

  sprintf( buf, "A construction crew begins working on a new area to the %s.",
	   dir_name[edir] );
  echo_to_room( AT_WHITE, ch->in_room, buf );
}

void do_bridge( CHAR_DATA * ch, char *argument )
{
  CLAN_DATA *clan = NULL;
  int chance = 0, ll = 0;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  EXIT_DATA *xit = NULL, *texit = NULL;
  int evnum = 0, edir = 0, ekey = 0;
  ROOM_INDEX_DATA *toroom = NULL;
  char buf[MAX_STRING_LENGTH];

  if( IS_NPC( ch ) || !ch->pcdata || !ch->in_room )
    return;

  clan = ch->pcdata->clan;

  if( !clan )
    {
      send_to_char( "You need to be part of an organization before you can do that!\r\n", ch );
      return;
    }

  if( !clan_char_has_permission( clan, ch, CLAN_PERM_BUILD ) )
    {
      send_to_char( "Your organization hasn't given you permission to build on their lands!\r\n", ch );
      return;
    }

  if( !ch->in_room->area || !ch->in_room->area->planet ||
      clan != ch->in_room->area->planet->governed_by )
    {
      send_to_char( "You may only build on planets that your organization controls!\r\n", ch );
      return;
    }

  if( IS_SET( ch->in_room->room_flags, ROOM_NOPEDIT ) )
    {
      send_to_char( "Sorry, But you may not edit this room.\r\n", ch );
      return;
    }

  if( ch->gold < CONSTRUCTION_COST )
    {
      ch_printf( ch, "You do not have enough money. It will cost you %d credits to do that.\r\n", CONSTRUCTION_COST );
      return;
    }

  argument = one_argument( argument, arg1 );

  if( argument[0] == '\0' )
    {
      send_to_char( "USAGE: bridge <direction> <action> <argument>\r\n", ch );
      send_to_char( "\r\nAction being one of the following:\r\n", ch );
      send_to_char( "connect, door, keycode\r\n", ch );
      return;
    }

  argument = one_argument( argument, arg2 );

  chance = character_skill_level( ch, gsn_bridge );

  if( number_percent() > chance )
    {
      send_to_char( "You can't quite get the desired affect.\r\n", ch );
      ch->gold -= 10;
      return;
    }

  edir = get_dir( arg1 );
  xit = get_exit( ch->in_room, edir );

  if( !str_cmp( arg2, "connect" ) )
    {
      if( xit )
	{
	  send_to_char( "There's already an exit in that direction.\r\n",
			ch );
	  return;
	}

      evnum = atoi( argument );

      if( ( toroom = get_room_index( evnum ) ) == NULL )
	{
	  ch_printf( ch, "Non-existant room: %d\r\n", evnum );
	  return;
	}

      if( ch->in_room->area != toroom->area )
	{
	  ch_printf( ch,
		     "Nice try. Room %d isn't even on the same planet!\r\n",
		     evnum );
	  return;
	}

      if( IS_SET( toroom->room_flags, ROOM_NOPEDIT ) )
	{
	  ch_printf( ch, "Room %d isn't editable by players!\r\n", evnum );
	  return;
	}

      if( get_exit( toroom, rev_dir[edir] ) )
	{
	  ch_printf( ch, "Room %d already has an entrance from that direction!\r\n", evnum );
	  return;
	}

      make_bexit( ch->in_room, toroom, edir );

      sprintf( buf, "A construction crew opens up a passage to the %s.",
	       dir_name[edir] );
      echo_to_room( AT_WHITE, ch->in_room, buf );
      sprintf( buf, "A construction crew opens up a passage from the %s.",
	       dir_name[rev_dir[edir]] );
      echo_to_room( AT_WHITE, toroom, buf );
    }
  else if( !str_cmp( arg2, "keycode" ) )
    {
      if( !xit )
	{
	  send_to_char( "There's no exit in that direction.\r\n", ch );
	  return;
	}

      if( !IS_SET( xit->exit_info, EX_ISDOOR ) )
	{
	  send_to_char( "There's no door in that direction.\r\n", ch );
	  return;
	}

      ekey = atoi( argument );
      ch_printf( ch, "Ok the lock code is now: %d", ekey );
      xit->key = ekey;

    }
  else if( !str_cmp( arg2, "door" ) )
    {
      if( !xit )
	{
	  send_to_char( "There's no exit in that direction.\r\n", ch );
	  return;
	}

      if( !IS_SET( xit->exit_info, EX_ISDOOR ) )
	{
	  sprintf( buf, "A construction crew builds a door to the %s.",
		   dir_name[edir] );
	  echo_to_room( AT_WHITE, ch->in_room, buf );
	  SET_BIT( xit->exit_info, EX_ISDOOR );

	  if( ( texit = get_exit_to( xit->to_room, rev_dir[edir], ch->in_room->vnum ) ) )
	    {
	      sprintf( buf, "A construction crew builds a door to the %s.",
		       dir_name[rev_dir[edir]] );
	      echo_to_room( AT_WHITE, xit->to_room, buf );
	      SET_BIT( texit->exit_info, EX_ISDOOR );
	    }
	}
      else
	{
	  sprintf( buf, "A construction crew removes the door to the %s.",
		   dir_name[edir] );
	  echo_to_room( AT_WHITE, ch->in_room, buf );
	  REMOVE_BIT( xit->exit_info, EX_ISDOOR );

	  if( ( texit = get_exit_to( xit->to_room, rev_dir[edir], ch->in_room->vnum ) ) )
	    {
	      sprintf( buf, "A construction crew removes the door to the %s.",
		       dir_name[rev_dir[edir]] );
	      echo_to_room( AT_WHITE, xit->to_room, buf );
	      REMOVE_BIT( texit->exit_info, EX_ISDOOR );
	    }
	}
    }
  else
    {
      do_bridge( ch, STRLIT_EMPTY );
      return;
    }

  ch->gold -= CONSTRUCTION_COST;

  for( ll = 1; ll <= 20; ll++ )
    learn_from_success( ch, gsn_bridge );

  SET_BIT( ch->in_room->area->flags, AFLAG_MODIFIED );
}

void do_survey( CHAR_DATA * ch, char *argument )
{
  ROOM_INDEX_DATA *room = ch->in_room;
  int chance = character_skill_level( ch, gsn_survey );

  if( IS_NPC( ch ) || !ch->pcdata )
    return;

  if( number_percent() > chance || !room->area->planet )
    {
      send_to_char( "You have a hard time surveying this region.\r\n", ch );
      return;
    }

  ch_printf( ch, "&Y%s\r\n\r\n", room->name );
  ch_printf( ch, "&WIndex:&Y %d\r\n", room->vnum );

  if( room->area && room->area->planet )
    ch_printf( ch, "&WPlanet:&Y %s\r\n", room->area->planet->name );

  ch_printf( ch, "&WSize:&Y %d\r\n", room->tunnel );
  ch_printf( ch, "&WSector:&Y %s\r\n", get_sector_name(room->sector_type) );
  ch_printf( ch, "&WInfo:\r\n" );

  if( IS_SET( room->room_flags, ROOM_DARK ) )
    ch_printf( ch, "&Y   Room is always dark.\r\n" );

  if( IS_SET( room->room_flags, ROOM_INDOORS ) )
    ch_printf( ch, "&Y   Room is indoors.\r\n" );

  if( IS_SET( room->room_flags, ROOM_SHIPYARD ) )
    ch_printf( ch, "   &YSpacecraft can be built or purchased here.\r\n" );

  if( IS_SET( room->room_flags, ROOM_CAN_LAND ) )
    ch_printf( ch, "&Y   Spacecraft can land here.\r\n" );

  if( IS_SET( room->room_flags, ROOM_EMPLOYMENT ) )
    ch_printf( ch, "&Y   You can find temporary employment here.\r\n" );

  if( IS_SET( room->room_flags, ROOM_BANK ) )
    ch_printf( ch, "&Y   This room may be used as a bank.\r\n" );

  if( IS_SET( room->room_flags, ROOM_SAFE ) )
    ch_printf( ch, "&Y   Combat cannot take place in this room.\r\n" );

  if( IS_SET( room->room_flags, ROOM_HOTEL ) )
    ch_printf( ch, "&Y   Players may quit and enter the game from here.\r\n" );

  if( IS_SET( room->room_flags, ROOM_EMPTY_HOME ) )
    ch_printf( ch, "&Y   This room may be purchased for use as a players home / storage locker.\r\n" );

  if( IS_SET( room->room_flags, ROOM_PLR_HOME ) )
    ch_printf( ch, "&Y   This room is a players private home.\r\n" );

  if( IS_SET( room->room_flags, ROOM_MAIL ) )
    ch_printf( ch, "&Y   This is a post office.\r\n" );

  if( IS_SET( room->room_flags, ROOM_INFO ) )
    ch_printf( ch, "&Y   A message and information terminal may be installed here.\r\n" );

  if( IS_SET( room->room_flags, ROOM_TRADE ) )
    ch_printf( ch, "&Y   This room is used for resource trade.\r\n" );

  if( IS_SET( room->room_flags, ROOM_SUPPLY ) )
    ch_printf( ch, "&Y   This is a supply store.\r\n" );

  if( IS_SET( room->room_flags, ROOM_PAWN ) )
    ch_printf( ch, "&Y   You can buy and sell useful items here.\r\n" );

  if( IS_SET( room->room_flags, ROOM_RESTAURANT ) )
    ch_printf( ch, "&Y   This room is a restaurant.\r\n" );

  if( IS_SET( room->room_flags, ROOM_BARRACKS ) )
    ch_printf( ch, "&Y   This is a military barracks.\r\n" );

  if( IS_SET( room->room_flags, ROOM_GARAGE ) )
    ch_printf( ch, "&Y   Vehicles are built and sold here.\r\n" );

  if( IS_SET( room->room_flags, ROOM_CONTROL ) )
    ch_printf( ch, "&Y   This is a control tower for patrol spacecraft.\r\n" );

  if( IS_SET( room->room_flags, ROOM_BAR ) )
    ch_printf( ch, "&Y   This is bar.\r\n" );

  if ( IS_SET( room->room_flags, ROOM_MEDICAL ))
    send_to_char( "&Y   This is a medical clinic.\r\n", ch );

  if( IS_SET( room->room_flags, ROOM_NOPEDIT ) )
    ch_printf( ch, "&WThis room is NOT player editable.\r\n" );
  else
    ch_printf( ch, "&WThis room IS editable by players.\r\n" );

  learn_from_success( ch, gsn_survey );
}
