#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "mud.h"
#include "vector3.h"
#include "vector3_aux.h"

/* Increase this to make hyper travel faster. */
#define HYPERSPEED_MULTIPLIER 2

SHIP_DATA *first_ship = NULL;
SHIP_DATA *last_ship = NULL;

MISSILE_DATA *first_missile = NULL;
MISSILE_DATA *last_missile = NULL;

SPACE_DATA *first_starsystem = NULL;
SPACE_DATA *last_starsystem = NULL;

/* local routines */
static void handle_ship_collision( void );
static void fread_ship( SHIP_DATA * ship, FILE * fp );
static bool load_ship_file( const char *shipfile );
void write_ship_list( void );
static void fread_starsystem( SPACE_DATA * starsystem, FILE * fp );
static bool load_starsystem( const char *starsystemfile );
static void resetship( SHIP_DATA * ship );
static void landship( SHIP_DATA * ship, char *arg );
static void launchship( SHIP_DATA * ship );
static void echo_to_room_dnr( int ecolor, ROOM_INDEX_DATA * room,
    const char *argument );
static ch_ret drive_ship( CHAR_DATA * ch, SHIP_DATA * ship,
    EXIT_DATA * xit, int fall );
static bool autofly( SHIP_DATA * ship );
static void ship_untarget_all( SHIP_DATA *ship );
static void transship( SHIP_DATA *ship, int vnum );

/* from act_info.c */
bool is_online( const char *argument );

void echo_to_room_dnr( int ecolor, ROOM_INDEX_DATA * room,
    const char *argument )
{
  CHAR_DATA *vic = NULL;

  if( room == NULL )
    return;

  for( vic = room->first_person; vic; vic = vic->next_in_room )
  {
    set_char_color( ecolor, vic );
    send_to_char( argument, vic );
  }
}

bool laserfire_hits( SHIP_DATA * ship, SHIP_DATA * target, int chance )
{
  char buf[MAX_STRING_LENGTH];

  if( number_percent() > chance )
  {
    sprintf( buf, "Lasers fire from %s at you but miss.", ship->name );
    echo_to_cockpit( AT_ORANGE, target, buf );
    sprintf( buf, "The ship's lasers fire at %s but miss.", target->name );
    echo_to_cockpit( AT_ORANGE, ship, buf );
    sprintf( buf, "Laserfire from %s barely misses %s.",
	ship->name, target->name );
    echo_to_system( AT_ORANGE, ship, buf, target );
    return FALSE;
  }

  sprintf( buf, "Laserfire from %s hits %s.", ship->name, target->name );
  echo_to_system( AT_ORANGE, ship, buf, target );
  sprintf( buf, "You are hit by lasers from %s!", ship->name );
  echo_to_cockpit( AT_BLOOD, target, buf );
  sprintf( buf, "Your ship's lasers hit %s!.", target->name );
  echo_to_cockpit( AT_YELLOW, ship, buf );
  echo_to_ship( AT_RED, target,
      "A small explosion vibrates through the ship." );

  return TRUE;
}


bool missile_launcher_can_be_fired( const CHAR_DATA * ch,
    const SHIP_DATA * ship )
{
  if( ship->missilestate == MISSILE_DAMAGED )
  {
    send_to_char( "&RThe ships missile launchers are dammaged.\r\n", ch );
    return FALSE;
  }

  if( ship->missiles <= 0 )
  {
    send_to_char( "&RYou have no missiles to fire!\r\n", ch );
    return FALSE;
  }

  if( ship->missilestate != MISSILE_READY )
  {
    send_to_char( "&RThe missiles are still reloading.\r\n", ch );
    return FALSE;
  }

  if( ship->target == NULL )
  {
    send_to_char( "&RYou need to choose a target first.\r\n", ch );
    return FALSE;
  }

  return TRUE;
}

bool laser_can_be_fired( const CHAR_DATA * ch, const SHIP_DATA * ship )
{
  if( ship->laserstate == LASER_DAMAGED )
  {
    send_to_char( "&RThe ships main laser is damaged.\r\n", ch );
    return FALSE;
  }

  if( ship->laserstate >= ship->lasers )
  {
    send_to_char( "&RThe lasers are still recharging.\r\n", ch );
    return FALSE;
  }

  if( ship->target == NULL )
  {
    send_to_char( "&RYou need to choose a target first.\r\n", ch );
    return FALSE;
  }

  return TRUE;
}

bool turret_can_be_fired( const CHAR_DATA * ch, const TURRET_DATA * turret )
{
  if( turret->laserstate == LASER_DAMAGED )
  {
    send_to_char( "&RThe turret is damaged.\r\n", ch );
    return FALSE;
  }

  if( turret->laserstate > 1 )
  {
    send_to_char( "&RThe lasers are still recharging.\r\n", ch );
    return FALSE;
  }

  if( turret->target == NULL )
  {
    send_to_char( "&RYou need to choose a target first.\r\n", ch );
    return FALSE;
  }

  return TRUE;
}

void ship_acquire_target( SHIP_DATA * ship, SHIP_DATA * target )
{
  char buf[MAX_STRING_LENGTH];
  ship->target = target;
  sprintf( buf, "You are being targetted by %s.", ship->name );
  echo_to_cockpit( AT_BLOOD, target, buf );
}

int laser_hit_modifier( const SHIP_DATA * ship, const SHIP_DATA * target )
{
  int chance = 0;
  chance += target->model * 5;
  chance -= target->manuever / 10;
  chance -= target->currspeed / 20;
  chance -= ( abs( ( int ) ( target->pos.x - ship->pos.x ) ) / 70 );
  chance -= ( abs( ( int ) ( target->pos.y - ship->pos.y ) ) / 70 );
  chance -= ( abs( ( int ) ( target->pos.z - ship->pos.z ) ) / 70 );
  chance = URANGE( 10, chance, 90 );

  return chance;
}

int missile_hit_modifier( const SHIP_DATA * ship, const SHIP_DATA * target )
{
  int chance = 0;
  chance -= target->manuever / 5;
  chance -= target->currspeed / 20;
  chance += target->model * target->model * 2;
  chance -= ( abs( ( int ) ( target->pos.x - ship->pos.x ) ) / 100 );
  chance -= ( abs( ( int ) ( target->pos.y - ship->pos.y ) ) / 100 );
  chance -= ( abs( ( int ) ( target->pos.z - ship->pos.z ) ) / 100 );
  chance += ( 30 );
  chance = URANGE( 20, chance, 80 );

  return chance;
}

PLANET_DATA *find_planet_in_range( SHIP_DATA * ship, int distance )
{
  PLANET_DATA *planet = NULL;

  if( ship->starsystem )
  {
    for( planet = ship->starsystem->first_planet;
	planet; planet = planet->next_in_system )
    {
      if( ship_distance_to_planet( ship, planet ) <= distance )
      {
	break;
      }
    }
  }

  return planet;
}

SHIP_DATA *find_ship_in_range( SHIP_DATA * ship, int distance )
{
  SHIP_DATA *target = NULL;

  if( ship->starsystem )
  {
    for( target = ship->starsystem->first_ship; target;
	target = target->next_in_starsystem )
    {
      if( target != ship && ship_distance_to_ship( ship, target ) <= 500 )
      {
	break;
      }
    }
  }

  return target;
}

void ship_jump_to_lightspeed( SHIP_DATA * ship )
{
  char buf[MAX_STRING_LENGTH];
  sprintf( buf, "%s disappears from your scanner.", ship->name );
  echo_to_system( AT_YELLOW, ship, buf, NULL );

  ship_from_starsystem( ship, ship->starsystem );
  ship->shipstate = SHIP_HYPERSPACE;

  echo_to_ship( AT_YELLOW, ship,
      "The ship lurches slightly as it makes the jump to lightspeed." );
  echo_to_cockpit( AT_YELLOW, ship,
      "The stars become streaks of light as you enter hyperspace." );

  ship->energy -= ( 100 + ship->hyperdistance );
  vector_copy( &ship->pos, &ship->jump );
}

void missile_explode( MISSILE_DATA * missile )
{
  if( missile->target->chaff_released <= 0 )
  {
    SHIP_DATA *ship = missile->fired_from;
    SHIP_DATA *target = missile->target;
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *ch = NULL;
    bool ch_found = FALSE;

    echo_to_room( AT_YELLOW, ship->gunseat,
	"Your missile hits its target dead on!" );
    echo_to_cockpit( AT_BLOOD, target, "The ship is hit by a missile." );
    echo_to_ship( AT_RED, target,
	"A loud explosion shakes the ship violently!" );
    sprintf( buf, "You see a small explosion as %s is hit by a missile",
	target->name );
    echo_to_system( AT_ORANGE, target, buf, ship );

    for( ch = first_char; ch; ch = ch->next )
    {
      if( !IS_NPC( ch ) && nifty_is_name( missile->fired_by, ch->name ) )
      {
	ch_found = TRUE;
	break;
      }
    }

    damage_ship( target, 30, 50, ch_found ? ch : NULL );
    extract_missile( missile );
  }
  else
  {
    echo_to_room( AT_YELLOW, missile->fired_from->gunseat,
	"Your missile explodes harmlessly in a cloud of chaff!" );
    echo_to_cockpit( AT_YELLOW, missile->target,
	"A missile explodes in your chaff." );
    extract_missile( missile );
  }
}

void move_missiles( void )
{
  MISSILE_DATA *missile = NULL;
  MISSILE_DATA *m_next = NULL;

  for( missile = first_missile; missile; missile = m_next )
  {
    SHIP_DATA *target = missile->target;

    m_next = missile->next;

    if( target && target->starsystem
	&& target->starsystem == missile->starsystem )
    {
      missile_set_course_to_ship( missile, target );	/* home in */
      missile_move( missile );

      if( missile_distance_to_ship( missile, target ) <= 20 )
      {
	missile_explode( missile );
      }
      else
      {
	missile->age++;

	if( missile->age >= 50 )
	{
	  extract_missile( missile );
	}
      }
    }
    else
    {
      extract_missile( missile );
    }
  }
}

void ship_crash_with_star( SHIP_DATA * ship, const char *star_name )
{
  char buf[MAX_STRING_LENGTH];
  echo_to_cockpit( AT_BLOOD + AT_BLINK, ship,
      "You fly directly into the sun." );
  sprintf( buf, "%s flys directly into %s!", ship->name, star_name );
  echo_to_system( AT_ORANGE, ship, buf, NULL );
  destroy_ship( ship, NULL );
}

void ship_begin_orbit( SHIP_DATA * ship, const PLANET_DATA * planet )
{
  char buf[MAX_STRING_LENGTH];
  sprintf( buf, "You begin orbitting %s.", planet->name );
  echo_to_cockpit( AT_YELLOW, ship, buf );
  sprintf( buf, "%s begins orbiting %s.", ship->name, planet->name );
  echo_to_system( AT_ORANGE, ship, buf, NULL );
  ship->currspeed = 0;
}

void move_ships( void )
{
  SHIP_DATA *ship = NULL;
  SHIP_DATA *nextship = NULL;

  for( ship = first_ship; ship; ship = nextship )
  {
    nextship = ship->next;

    if( !ship->starsystem )
      continue;

    ship_move( ship );

    if( autofly( ship ) )
      continue;

    if( ship->starsystem->star1 && strcmp( ship->starsystem->star1, "" )
	&& vector_distance( &ship->pos,
	  &ship->starsystem->star1_pos ) < 10 )
    {
      ship_crash_with_star( ship, ship->starsystem->star1 );
      continue;
    }

    if( ship->starsystem->star2 && strcmp( ship->starsystem->star2, "" )
	&& vector_distance( &ship->pos,
	  &ship->starsystem->star2_pos ) < 10 )
    {
      ship_crash_with_star( ship, ship->starsystem->star2 );
      continue;
    }

    if( ship->currspeed > 0 )
    {
      PLANET_DATA *planet = find_planet_in_range( ship, 10 );

      if( planet )
      {
	ship_begin_orbit( ship, planet );
      }
    }
  }

  handle_ship_collision();
}

void handle_ship_collision( void )
{
  SHIP_DATA *ship = NULL;
  SHIP_DATA *nextship = NULL;

  for( ship = first_ship; ship; ship = nextship )
  {
    nextship = ship->next;

    if( ship->collision )
    {
      echo_to_cockpit( AT_WHITE + AT_BLINK, ship,
	  "You have collided with another ship!" );
      echo_to_ship( AT_RED, ship,
	  "A loud explosion shakes the ship violently!" );
      damage_ship( ship, ship->collision, ship->collision, NULL );
      ship->collision = 0;
    }
  }
}

void ship_handle_autofly_combat( SHIP_DATA * ship )
{
  int chance = 100;
  SHIP_DATA *target = ship->target;
  int shots = 0;

  for( shots = 0; shots <= ship->lasers; shots++ )
  {
    if( !ship->target )
      break;

    if( ship->shipstate != SHIP_HYPERSPACE && ship->energy > 25
	&& ship->target->starsystem == ship->starsystem
	&& ship_distance_to_ship( ship, target ) <= 1000
	&& ship->laserstate < ship->lasers )
    {
      if( ship->ship_class == SPACE_STATION
	  || ship_is_facing_ship( ship, target ) )
      {
	chance += laser_hit_modifier( ship, target );

	if( laserfire_hits( ship, target, chance ) )
	{
	  if( autofly( target ) )
	    target->target = ship;

	  damage_ship( target, 5, 10, NULL );
	}

	ship->laserstate++;
      }
    }
  }
}

void ship_reload_missiles( SHIP_DATA * ship )
{
  if( ship->missilestate == MISSILE_RELOAD_2 )
  {
    ship->missilestate = MISSILE_READY;
    if( ship->missiles > 0 )
      echo_to_room( AT_YELLOW, ship->gunseat,
	  "Missile launcher reloaded." );
  }

  if( ship->missilestate == MISSILE_RELOAD )
  {
    ship->missilestate = MISSILE_RELOAD_2;
  }

  if( ship->missilestate == MISSILE_FIRED )
    ship->missilestate = MISSILE_RELOAD;
}

void ship_update_laserstate( SHIP_DATA * ship )
{
  TURRET_DATA *turret = NULL;

  for( turret = ship->first_turret; turret; turret = turret->next )
  {
    if( turret->laserstate > 0 )
    {
      ship->energy -= turret->laserstate;
      turret->laserstate = 0;
    }
  }

  if( ship->laserstate > 0 )
  {
    ship->energy -= ship->laserstate;
    ship->laserstate = 0;
  }
}

void recharge_ships( void )
{
  SHIP_DATA *ship = NULL;
  SHIP_DATA *nextship = NULL;

  for( ship = first_ship; ship; ship = nextship )
  {
    nextship = ship->next;

    ship_update_laserstate( ship );
    ship_reload_missiles( ship );

    if( autofly( ship ) && ship->starsystem
	&& ship->target && ship->laserstate != LASER_DAMAGED )
    {
      ship_handle_autofly_combat( ship );
    }
  }
}

void ship_update_energy( SHIP_DATA * ship )
{
  if( ship->starsystem )
  {
    if( ship->energy > 0
	&& ship->shipstate == SHIP_DISABLED
	&& ship->ship_class != SPACE_STATION )
    {
      ship->energy -= 100;
    }
    else if( ship->energy > 0 )
    {
      ship->energy += 5 + ship->model * 2;
    }
    else
    {
      destroy_ship( ship, NULL );
    }
  }
}

void ship_update_shipstate( SHIP_DATA * ship )
{
  if( ship->shipstate == SHIP_BUSY_3 )
  {
    echo_to_room( AT_YELLOW, ship->pilotseat, "Manuever complete." );
    ship->shipstate = SHIP_READY;
  }

  if( ship->shipstate == SHIP_BUSY_2 )
    ship->shipstate = SHIP_BUSY_3;

  if( ship->shipstate == SHIP_BUSY )
    ship->shipstate = SHIP_BUSY_2;

  if( ship->shipstate == SHIP_LAND_2 )
    landship( ship, ship->dest );

  if( ship->shipstate == SHIP_LAND )
    ship->shipstate = SHIP_LAND_2;

  if( ship->shipstate == SHIP_LAUNCH_2 )
    launchship( ship );

  if( ship->shipstate == SHIP_LAUNCH )
    ship->shipstate = SHIP_LAUNCH_2;
}

void ship_drop_from_hyperspace( SHIP_DATA * ship )
{
  ship_to_starsystem( ship, ship->currjump );

  if( ship->starsystem == NULL )
  {
    echo_to_cockpit( AT_RED, ship,
	"Ship lost in Hyperspace. Make new calculations." );
  }
  else
  {
    char buf[MAX_STRING_LENGTH];
    echo_to_room( AT_YELLOW, ship->pilotseat, "Hyperjump complete." );
    echo_to_ship( AT_YELLOW, ship,
	"The ship lurches slightly as it comes out of hyperspace." );
    sprintf( buf, "%s enters the starsystem at %.0f %.0f %.0f",
	ship->name, ship->pos.x, ship->pos.y, ship->pos.z );
    echo_to_system( AT_YELLOW, ship, buf, NULL );
    ship->shipstate = SHIP_READY;
    STRFREE( ship->home );
    ship->home = STRALLOC( ship->starsystem->name );
  }
}

void ship_update_hyperspace( SHIP_DATA * ship )
{
  if( ship->shipstate == SHIP_HYPERSPACE )
  {
    ship->hyperdistance -= ship->hyperspeed * HYPERSPEED_MULTIPLIER;

    if( ship->hyperdistance <= 0 )
    {
      ship_drop_from_hyperspace( ship );
    }
    else
    {
      char buf[MAX_STRING_LENGTH];
      sprintf( buf, "%d", ship->hyperdistance );
      echo_to_room_dnr( AT_YELLOW, ship->pilotseat,
	  "Remaining jump distance: " );
      echo_to_room( AT_WHITE, ship->pilotseat, buf );
    }
  }
}

void ship_update_shields( SHIP_DATA * ship )
{
  ship->shield = UMAX( 0, ship->shield - 1 - ship->model / 5 );

  if( ship->autorecharge
      && ship->maxshield > ship->shield && ship->energy > 100 )
  {
    int recharge =
      UMIN( ship->maxshield - ship->shield, 10 + ship->model * 2 );
    recharge = UMIN( recharge, ship->energy - 100 );
    recharge = UMAX( 1, recharge );
    ship->shield += recharge;
    ship->energy -= recharge;
  }

  if( ship->shield > 0 )
  {
    if( ship->energy < 200 )
    {
      ship->shield = 0;
      echo_to_cockpit( AT_RED, ship,
	  "The ships shields fizzle and die." );
      ship->autorecharge = FALSE;
    }
  }
}

void ship_show_space_prompt( const SHIP_DATA * ship )
{
  if( ship->starsystem && ship->currspeed > 0 )
  {
    char buf[MAX_STRING_LENGTH];
    sprintf( buf, "%d", ship->currspeed );
    echo_to_room_dnr( AT_BLUE, ship->pilotseat, "Speed: " );
    echo_to_room_dnr( AT_LBLUE, ship->pilotseat, buf );
    sprintf( buf, "%.0f %.0f %.0f", ship->pos.x, ship->pos.y, ship->pos.z );
    echo_to_room_dnr( AT_BLUE, ship->pilotseat, "  Coords: " );
    echo_to_room( AT_LBLUE, ship->pilotseat, buf );
  }
}

void ship_echo_proximity_alert( const SHIP_DATA * ship,
    const char *target_name,
    const Vector3 * target_position )
{
  char buf[MAX_STRING_LENGTH];
  sprintf( buf, "Proximity alert: %s  %.0f %.0f %.0f", target_name,
      target_position->x, target_position->y, target_position->z );
  echo_to_room( AT_RED, ship->pilotseat, buf );
}

void ship_check_proximity( const SHIP_DATA * ship )
{
  if( ship->starsystem )
  {
    double too_close = ship->currspeed + 50;
    SHIP_DATA *target = NULL;

    for( target = ship->starsystem->first_ship; target;
	target = target->next_in_starsystem )
    {
      double target_too_close = too_close + target->currspeed;

      if( target != ship
	  && ship_distance_to_ship( ship, target ) < target_too_close )
      {
	ship_echo_proximity_alert( ship, target->name, &target->pos );
      }
    }

    too_close = ship->currspeed + 100;

    if( ship->starsystem->star1 && strcmp( ship->starsystem->star1, "" )
	&& vector_distance( &ship->pos, &ship->starsystem->star1_pos )
	< too_close )
    {
      ship_echo_proximity_alert( ship, ship->starsystem->star1,
	  &ship->starsystem->star1_pos );
    }

    if( ship->starsystem->star2 && strcmp( ship->starsystem->star2, "" )
	&& vector_distance( &ship->pos, &ship->starsystem->star2_pos )
	< too_close )
    {
      ship_echo_proximity_alert( ship, ship->starsystem->star2,
	  &ship->starsystem->star2_pos );
    }
  }
}

void ship_show_combat_prompt( SHIP_DATA * ship )
{
  if( ship->target )
  {
    char buf[MAX_STRING_LENGTH];
    sprintf( buf, "%s   %.0f %.0f %.0f", ship->target->name,
	ship->target->pos.x, ship->target->pos.y,
	ship->target->pos.z );
    echo_to_room_dnr( AT_BLUE, ship->pilotseat, "Target: " );
    echo_to_room( AT_LBLUE, ship->pilotseat, buf );

    if( ship->starsystem != ship->target->starsystem )
      ship->target = NULL;
  }
}

void ship_warn_on_low_fuel( const SHIP_DATA * ship )
{
  if( ship->energy < 100 && ship->starsystem )
  {
    echo_to_cockpit( AT_RED, ship, "Warning: Ship fuel low." );
  }
}

void ship_update_autotrack( SHIP_DATA * ship )
{
  if( ship->autotrack && ship->target && ship->ship_class < SPACE_STATION )
  {
    SHIP_DATA *target = ship->target;
    double too_close = ship->currspeed + 10;
    double target_too_close = too_close + target->currspeed;

    if( target != ship && ship->shipstate == SHIP_READY &&
	ship_distance_to_ship( ship, target ) < target_too_close )
    {
      ship_turn_180( ship );
      ship->energy -= ship->currspeed / 10;
      echo_to_room( AT_RED, ship->pilotseat,
	  "Autotrack: Evading to avoid collision!\r\n" );

      if( ship->manuever > 100 )
	ship->shipstate = SHIP_BUSY_3;
      else if( ship->manuever > 50 )
	ship->shipstate = SHIP_BUSY_2;
      else
	ship->shipstate = SHIP_BUSY;
    }
    else if( !ship_is_facing_ship( ship, ship->target ) )
    {
      ship_set_course_to_ship( ship, target );
      ship->energy -= ship->currspeed / 10;
      echo_to_room( AT_BLUE, ship->pilotseat,
	  "Autotracking target... setting new course.\r\n" );

      if( ship->manuever > 100 )
	ship->shipstate = SHIP_BUSY_3;
      else if( ship->manuever > 50 )
	ship->shipstate = SHIP_BUSY_2;
      else
	ship->shipstate = SHIP_BUSY;
    }
  }
}

void spacestation_regenerate_missiles( SHIP_DATA * ship )
{
  if( ship->ship_class == SPACE_STATION && ship->target == NULL )
  {
    if( ship->missiles < ship->maxmissiles )
      ship->missiles++;
  }
}

void ship_respawn_mob_ship( SHIP_DATA * ship )
{
  if( ( ship->ship_class == SPACE_STATION
	|| ship->type == MOB_SHIP ) && ship->home )
  {
    /* 4% chance of respawning mob ship or space station. */
    if( number_range( 1, 25 ) == 25 )
    {
      ship_to_starsystem( ship, starsystem_from_name( ship->home ) );
      vector_init( &ship->pos );
      vector_randomize( &ship->pos, -5000, 5000 );
      vector_set( &ship->head, 1, 1, 1 );
      ship->autopilot = TRUE;
    }
  }
}

void ship_handle_autoflying_clanship( SHIP_DATA * ship, CLAN_DATA * clan )
{
  SHIP_DATA *target = NULL;

  for( target = ship->starsystem->first_ship; target;
      target = target->next_in_starsystem )
  {
    int targetok = 0;
    ROOM_INDEX_DATA *room = NULL;
    char buf[MAX_STRING_LENGTH];

    if( autofly( target ) && !str_cmp( ship->owner, target->owner ) )
    {
      targetok = 1;
    }

    for( room = target->first_room; room; room = room->next_in_ship )
    {
      CHAR_DATA *passenger = NULL;

      for( passenger = room->first_person; passenger;
	  passenger = passenger->next_in_room )
      {
	if( !IS_NPC( passenger ) && passenger->pcdata )
	{
	  if( passenger->pcdata->clan == clan )
	  {
	    targetok = 1;
	  }
	  else if( passenger->pcdata->clan
	      && passenger->pcdata->clan != clan
	      && targetok == 0 )
	  {
	    if( nifty_is_name( passenger->pcdata->clan->name,
		  clan->atwar ) )
	    {
	      targetok = -1;
	    }
	  }
	}
      }
    }

    if( targetok == 1 && target->target )
    {
      ship_acquire_target( ship, target->target );
      break;
    }

    if( targetok == 0 && target->target )
    {
      if( !str_cmp( target->target->owner, clan->name ) )
      {
	targetok = -1;
      }
      else if( nifty_is_name( target->owner, clan->atwar ) )
      {
	targetok = -1;
      }
      else
      {
	for( room = target->target->first_room; room;
	    room = room->next_in_ship )
	{
	  CHAR_DATA *passenger = NULL;

	  for( passenger = room->first_person;
	      passenger; passenger = passenger->next_in_room )
	  {
	    if( !IS_NPC( passenger )
		&& passenger->pcdata
		&& passenger->pcdata->clan == clan )
	    {
	      targetok = -1;
	    }
	  }
	}
      }
    }

    if( targetok >= 0 )
      continue;

    sprintf( buf, "You are being scanned by %s.", ship->name );
    echo_to_cockpit( AT_WHITE, target, buf );
    ship_acquire_target( ship, target );
    break;
  }
}

void ship_fire_missile( SHIP_DATA * ship, SHIP_DATA * target )
{
  char buf[MAX_STRING_LENGTH];
  new_missile( ship, target, NULL );
  ship->missiles--;
  sprintf( buf, "Incoming missile from %s.", ship->name );
  echo_to_cockpit( AT_BLOOD, target, buf );
  sprintf( buf, "%s fires a missile towards %s.", ship->name, target->name );
  echo_to_system( AT_ORANGE, target, buf, NULL );

  if( ship->ship_class == SPACE_STATION )
    ship->missilestate = MISSILE_RELOAD_2;
  else
    ship->missilestate = MISSILE_FIRED;
}

void ship_handle_autoflying_has_target( SHIP_DATA * ship )
{
  SHIP_DATA *target = ship->target;
  ship->autotrack = TRUE;

  if( ship->ship_class != SPACE_STATION )
    ship->currspeed = ship->realspeed;

  if( ship->energy > 200 )
    ship->autorecharge = TRUE;

  if( ship->shipstate != SHIP_HYPERSPACE
      && ship->energy > 200
      && ship->hyperspeed > 0
      && ship->target->starsystem == ship->starsystem
      && ship_distance_to_ship( ship, target ) > 5000
      && number_bits( 2 ) == 0 )
  {
    ship->currjump = ship->starsystem;
    ship->hyperdistance = 1;

    ship_jump_to_lightspeed( ship );
  }

  if( ship->shipstate != SHIP_HYPERSPACE && ship->energy > 25
      && ship->missilestate == MISSILE_READY
      && ship->target->starsystem == ship->starsystem
      && ship_distance_to_ship( ship, target ) <= 1200 && ship->missiles > 0 )
  {
    if( ship->ship_class == SPACE_STATION
	|| ship_is_facing_ship( ship, target ) )
    {
      int chance = 100 + missile_hit_modifier( ship, target );

      if( number_percent() > chance )
      {

      }
      else
      {
	ship_fire_missile( ship, target );
      }
    }
  }

  if( ship->missilestate == MISSILE_DAMAGED )
    ship->missilestate = MISSILE_READY;

  if( ship->laserstate == LASER_DAMAGED )
    ship->laserstate = LASER_READY;

  if( ship->shipstate == SHIP_DISABLED )
    ship->shipstate = SHIP_READY;
}

void ship_handle_autoflying( SHIP_DATA * ship )
{
  if( autofly( ship ) )
  {
    if( ship->starsystem )
    {
      if( ship->target )
      {
	ship_handle_autoflying_has_target( ship );
      }
      else
      {
	CLAN_DATA *clan = NULL;
	CLAN_DATA *shipclan = NULL;

	ship->currspeed = 0;

	for( clan = first_clan; clan; clan = clan->next )
	  if( !str_cmp( ship->owner, clan->name ) )
	    shipclan = clan;

	if( shipclan )
	{
	  ship_handle_autoflying_clanship( ship, shipclan );
	}
      }
    }
    else
    {
      ship_respawn_mob_ship( ship );
    }
  }
}

void update_space( void )
{
  SHIP_DATA *ship = 0;
  SHIP_DATA *nextship = 0;

  for( ship = first_ship; ship; ship = nextship )
  {
    nextship = ship->next;

    ship_update_energy( ship );

    if( ship->chaff_released > 0 )
      ship->chaff_released = FALSE;

    ship_update_hyperspace( ship );
    ship_update_shipstate( ship );
    ship_update_shields( ship );
    ship_show_space_prompt( ship );
    ship_check_proximity( ship );
    ship_show_combat_prompt( ship );
    ship_warn_on_low_fuel( ship );

    ship->energy = URANGE( 0, ship->energy, ship->maxenergy );
  }

  for( ship = first_ship; ship; ship = ship->next )
  {
    ship_update_autotrack( ship );
    ship_handle_autoflying( ship );
    spacestation_regenerate_missiles( ship );
  }
}

void write_starsystem_list( void )
{
  SPACE_DATA *tstarsystem = NULL;
  FILE *fpout = NULL;
  char filename[256];

  sprintf( filename, "%s%s", SPACE_DIR, SPACE_LIST );
  fpout = fopen( filename, "w" );

  if( !fpout )
  {
    bug( "FATAL: cannot open starsystem.lst for writing!\r\n", 0 );
    return;
  }

  for( tstarsystem = first_starsystem; tstarsystem;
      tstarsystem = tstarsystem->next )
    fprintf( fpout, "%s\n", tstarsystem->filename );

  fprintf( fpout, "$\n" );
  fclose( fpout );
}


/*
 * Get pointer to space structure from starsystem name.
 */
SPACE_DATA *starsystem_from_name( const char *name )
{
  SPACE_DATA *starsystem = NULL;

  for( starsystem = first_starsystem; starsystem;
      starsystem = starsystem->next )
    if( !str_cmp( name, starsystem->name ) )
      return starsystem;

  for( starsystem = first_starsystem; starsystem;
      starsystem = starsystem->next )
    if( !str_prefix( name, starsystem->name ) )
      return starsystem;

  return NULL;
}

/*
 * Get pointer to space structure from the dock vnun.
 */
SPACE_DATA *starsystem_from_room( const ROOM_INDEX_DATA * room )
{
  SHIP_DATA *ship = NULL;
  ROOM_INDEX_DATA *sRoom = NULL;

  if( room == NULL )
    return NULL;

  if( room->area != NULL && room->area->planet != NULL )
    return room->area->planet->starsystem;

  for( ship = first_ship; ship; ship = ship->next )
  {
    for( sRoom = ship->first_room; sRoom; sRoom = sRoom->next_in_ship )
    {
      if( room == sRoom )
      {
	return ship->starsystem;
      }
    }
  }

  return NULL;
}


/*
 * Save a starsystem's data to its data file
 */
void save_starsystem( const SPACE_DATA * starsystem )
{
  FILE *fp = NULL;
  char filename[256];

  if( !starsystem )
  {
    bug( "save_starsystem: null starsystem pointer!", 0 );
    return;
  }

  if( !starsystem->filename || starsystem->filename[0] == '\0' )
  {
    bug( "save_starsystem: %s has no filename", starsystem->name );
    return;
  }

  sprintf( filename, "%s%s", SPACE_DIR, starsystem->filename );

  if( ( fp = fopen( filename, "w" ) ) == NULL )
  {
    bug( "save_starsystem: fopen", 0 );
    perror( filename );
  }
  else
  {
    fprintf( fp, "#SPACE\n" );
    fprintf( fp, "Name         %s~\n", starsystem->name );
    fprintf( fp, "Filename     %s~\n", starsystem->filename );
    fprintf( fp, "Star1        %s~\n", starsystem->star1 );
    fprintf( fp, "Star2        %s~\n", starsystem->star2 );
    fprintf( fp, "S1x          %.0f\n", starsystem->star1_pos.x );
    fprintf( fp, "S1y          %.0f\n", starsystem->star1_pos.y );
    fprintf( fp, "S1z          %.0f\n", starsystem->star1_pos.z );
    fprintf( fp, "S2x          %.0f\n", starsystem->star2_pos.x );
    fprintf( fp, "S2y          %.0f\n", starsystem->star2_pos.y );
    fprintf( fp, "S2z          %.0f\n", starsystem->star2_pos.z );
    fprintf( fp, "End\n\n" );
    fprintf( fp, "#END\n" );
  }

  fclose( fp );
}

/*
 * Read in actual starsystem data.
 */
void fread_starsystem( SPACE_DATA * starsystem, FILE * fp )
{
  char buf[MAX_STRING_LENGTH];
  bool fMatch = FALSE;

  for( ;; )
  {
    const char *word = feof( fp ) ? "End" : fread_word( fp );
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
	  if( !starsystem->name )
	    starsystem->name = STRALLOC( "" );
	  if( !starsystem->star1 )
	    starsystem->star1 = STRALLOC( "" );
	  if( !starsystem->star2 )
	    starsystem->star2 = STRALLOC( "" );
	  return;
	}
	break;

      case 'F':
	KEY( "Filename", starsystem->filename, fread_string( fp ) );
	break;

      case 'N':
	KEY( "Name", starsystem->name, fread_string( fp ) );
	break;

      case 'S':
	KEY( "Star1", starsystem->star1, fread_string( fp ) );
	KEY( "Star2", starsystem->star2, fread_string( fp ) );
	KEY( "S1x", starsystem->star1_pos.x, fread_number( fp ) );
	KEY( "S1y", starsystem->star1_pos.y, fread_number( fp ) );
	KEY( "S1z", starsystem->star1_pos.z, fread_number( fp ) );
	KEY( "S2x", starsystem->star2_pos.x, fread_number( fp ) );
	KEY( "S2y", starsystem->star2_pos.y, fread_number( fp ) );
	KEY( "S2z", starsystem->star2_pos.z, fread_number( fp ) );
    }

    if( !fMatch )
    {
      sprintf( buf, "Fread_starsystem: no match: %s", word );
      bug( buf, 0 );
    }
  }
}

/*
 * Load a starsystem file
 */

SPACE_DATA *starsystem_create( void )
{
  SPACE_DATA *starsystem = NULL;

  CREATE( starsystem, SPACE_DATA, 1 );

  starsystem->next = NULL;
  starsystem->prev = NULL;
  starsystem->first_planet = NULL;
  starsystem->last_planet = NULL;
  starsystem->first_ship = NULL;
  starsystem->last_ship = NULL;
  starsystem->first_missile = NULL;
  starsystem->last_missile = NULL;
  starsystem->filename = NULL;
  starsystem->name = NULL;
  starsystem->star1 = NULL;
  starsystem->star2 = NULL;
  vector_init( &starsystem->star1_pos );
  vector_init( &starsystem->star2_pos );
  starsystem->crash = 0;

  return starsystem;
}

bool load_starsystem( const char *starsystemfile )
{
  char filename[256];
  FILE *fp = NULL;
  bool found = FALSE;

  sprintf( filename, "%s%s", SPACE_DIR, starsystemfile );

  if( ( fp = fopen( filename, "r" ) ) != NULL )
  {
    SPACE_DATA *starsystem = starsystem_create();
    LINK( starsystem, first_starsystem, last_starsystem, next, prev );
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
	bug( "Load_starsystem_file: # not found.", 0 );
	break;
      }

      word = fread_word( fp );

      if( !str_cmp( word, "SPACE" ) )
      {
	fread_starsystem( starsystem, fp );
	break;
      }
      else if( !str_cmp( word, "END" ) )
      {
	break;
      }
      else
      {
	char buf[MAX_STRING_LENGTH];

	sprintf( buf, "Load_starsystem_file: bad section: %s.", word );
	bug( buf, 0 );
	break;
      }
    }

    fclose( fp );
  }

  return found;
}

/*
 * Load in all the starsystem files.
 */
void load_space( void )
{
  FILE *fpList = NULL;
  char starsystemlist[256];

  sprintf( starsystemlist, "%s%s", SPACE_DIR, SPACE_LIST );

  if( ( fpList = fopen( starsystemlist, "r" ) ) == NULL )
  {
    perror( starsystemlist );
    exit( 1 );
  }

  for( ;; )
  {
    const char *filename = feof( fpList ) ? "$" : fread_word( fpList );

    if( filename[0] == '$' )
      break;

    if( !load_starsystem( filename ) )
    {
      bug( "Cannot load starsystem file: %s", filename );
    }
  }

  fclose( fpList );
}

void do_setstarsystem( CHAR_DATA * ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  SPACE_DATA *starsystem;

  if( IS_NPC( ch ) )
  {
    send_to_char( "Huh?\r\n", ch );
    return;
  }

  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );

  if( arg2[0] == '\0' || arg1[0] == '\0' )
  {
    send_to_char( "Usage: setstarsystem <starsystem> <field> <values>\r\n",
	ch );
    send_to_char( "\r\nField being one of:\r\n", ch );
    send_to_char( "name filename\r\n", ch );
    send_to_char( "star1 s1x s1y s1z\r\n", ch );
    send_to_char( "star2 s2x s2y s2z\r\n", ch );
    send_to_char( "", ch );
    return;
  }

  starsystem = starsystem_from_name( arg1 );
  if( !starsystem )
  {
    send_to_char( "No such starsystem.\r\n", ch );
    return;
  }

  if( !str_cmp( arg2, "s1x" ) )
  {
    starsystem->star1_pos.x = atoi( argument );
    send_to_char( "Done.\r\n", ch );
    save_starsystem( starsystem );
    return;
  }
  if( !str_cmp( arg2, "s1y" ) )
  {
    starsystem->star1_pos.y = atoi( argument );
    send_to_char( "Done.\r\n", ch );
    save_starsystem( starsystem );
    return;
  }
  if( !str_cmp( arg2, "s1z" ) )
  {
    starsystem->star1_pos.z = atoi( argument );
    send_to_char( "Done.\r\n", ch );
    save_starsystem( starsystem );
    return;
  }

  if( !str_cmp( arg2, "s2x" ) )
  {
    starsystem->star2_pos.x = atoi( argument );
    send_to_char( "Done.\r\n", ch );
    save_starsystem( starsystem );
    return;
  }
  if( !str_cmp( arg2, "s2y" ) )
  {
    starsystem->star2_pos.y = atoi( argument );
    send_to_char( "Done.\r\n", ch );
    save_starsystem( starsystem );
    return;
  }
  if( !str_cmp( arg2, "s2z" ) )
  {
    starsystem->star2_pos.z = atoi( argument );
    send_to_char( "Done.\r\n", ch );
    save_starsystem( starsystem );
    return;
  }


  if( !str_cmp( arg2, "name" ) )
  {
    if( *argument == '\0' )
    {
      ch_printf( ch, "You must choose a name.\r\n" );
      return;
    }

    if( starsystem_from_name( argument ) )
    {
      ch_printf( ch, "A starsystem with that name already exists.\r\n" );
      return;
    }

    STRFREE( starsystem->name );
    starsystem->name = STRALLOC( argument );
    send_to_char( "Done.\r\n", ch );
    save_starsystem( starsystem );
    return;
  }

  if( !str_cmp( arg2, "star1" ) )
  {
    STRFREE( starsystem->star1 );
    starsystem->star1 = STRALLOC( argument );
    send_to_char( "Done.\r\n", ch );
    save_starsystem( starsystem );
    return;
  }

  if( !str_cmp( arg2, "star2" ) )
  {
    STRFREE( starsystem->star2 );
    starsystem->star2 = STRALLOC( argument );
    send_to_char( "Done.\r\n", ch );
    save_starsystem( starsystem );
    return;
  }

  do_setstarsystem( ch, STRLIT_EMPTY );
}

void showstarsystem( const CHAR_DATA * ch, const SPACE_DATA * starsystem )
{
  PLANET_DATA *planet;

  ch_printf( ch, "Starsystem:%s     Filename: %s\r\n",
      starsystem->name, starsystem->filename );
  ch_printf( ch, "Star1: %s   Coordinates: %.0f %.0f %.0f\r\n",
      starsystem->star1,
      starsystem->star1_pos.x,
      starsystem->star1_pos.y, starsystem->star1_pos.z );
  ch_printf( ch, "Star2: %s   Coordinates: %.0f %.0f %.0f\r\n",
      starsystem->star2,
      starsystem->star2_pos.x,
      starsystem->star2_pos.y, starsystem->star2_pos.z );
  for( planet = starsystem->first_planet; planet;
      planet = planet->next_in_system )
    ch_printf( ch, "Planet: %s   Coordinates: %.0f %.0f %.0f\r\n",
	planet->name, planet->pos.x, planet->pos.y, planet->pos.z );
  return;
}

void do_showstarsystem( CHAR_DATA * ch, char *argument )
{
  SPACE_DATA *starsystem;

  starsystem = starsystem_from_name( argument );

  if( starsystem == NULL )
    send_to_char( "&RNo such starsystem.\r\n", ch );
  else
    showstarsystem( ch, starsystem );

}

void do_starsystems( CHAR_DATA * ch, char *argument )
{
  SPACE_DATA *starsystem;
  int count = 0;

  for( starsystem = first_starsystem; starsystem;
      starsystem = starsystem->next )
  {
    set_char_color( AT_NOTE, ch );
    ch_printf( ch, "%s\r\n", starsystem->name );
    count++;
  }

  if( !count )
  {
    send_to_char( "There are no starsystems currently formed.\r\n", ch );
    return;
  }
}

void echo_to_ship( int color, const SHIP_DATA * ship, const char *argument )
{
  ROOM_INDEX_DATA *room;

  for( room = ship->first_room; room; room = room->next_in_ship )
    echo_to_room( color, room, argument );

}

void echo_to_cockpit( int color, const SHIP_DATA * ship,
    const char *argument )
{
  TURRET_DATA *turret;

  echo_to_room( color, ship->pilotseat, argument );

  if( ship->pilotseat != ship->gunseat )
    echo_to_room( color, ship->gunseat, argument );

  if( ship->pilotseat != ship->viewscreen
      && ship->viewscreen != ship->gunseat )
    echo_to_room( color, ship->viewscreen, argument );

  for( turret = ship->first_turret; turret; turret = turret->next )
    if( turret->room )
      echo_to_room( color, turret->room, argument );
}

void echo_to_system( int color, const SHIP_DATA * ship,
    const char *argument, const SHIP_DATA * ignore )
{
  SHIP_DATA *target = NULL;

  if( !ship->starsystem )
    return;

  for( target = ship->starsystem->first_ship; target;
      target = target->next_in_starsystem )
  {
    if( target != ship && target != ignore )
      echo_to_cockpit( color, target, argument );
  }
}

void write_ship_list()
{
  SHIP_DATA *tship = NULL;
  FILE *fpout = NULL;
  char filename[256];

  sprintf( filename, "%s%s", SHIP_DIR, SHIP_LIST );
  fpout = fopen( filename, "w" );

  if( !fpout )
  {
    bug( "FATAL: cannot open ship.lst for writing!\r\n", 0 );
    return;
  }

  for( tship = first_ship; tship; tship = tship->next )
    if( tship->type != MOB_SHIP && tship->owner && tship->owner[0] != '\0' )
      fprintf( fpout, "%s\n", tship->filename );

  fprintf( fpout, "$\n" );
  fclose( fpout );
}

SHIP_DATA *ship_in_room( const ROOM_INDEX_DATA * room, const char *name )
{
  SHIP_DATA *ship = NULL;

  if( !room )
    return NULL;

  for( ship = room->first_ship; ship; ship = ship->next_in_room )
    if( !str_cmp( name, ship->name ) )
      if( ship->owner && ship->owner[0] != '\0' )
	if( get_clan( ship->owner ) || is_online( ship->owner )
	    || is_online( ship->pilot ) || is_online( ship->copilot ) )
	  return ship;

  for( ship = room->first_ship; ship; ship = ship->next_in_room )
    if( nifty_is_name( name, ship->name ) )
      if( ship->owner && ship->owner[0] != '\0' )
	if( get_clan( ship->owner ) || is_online( ship->owner )
	    || is_online( ship->pilot ) || is_online( ship->copilot ) )
	  return ship;

  for( ship = room->first_ship; ship; ship = ship->next_in_room )
    if( !str_prefix( name, ship->name ) )
      if( ship->owner && ship->owner[0] != '\0' )
	if( get_clan( ship->owner ) || is_online( ship->owner )
	    || is_online( ship->pilot ) || is_online( ship->copilot ) )
	  return ship;

  for( ship = room->first_ship; ship; ship = ship->next_in_room )
    if( nifty_is_name_prefix( name, ship->name ) )
      if( ship->owner && ship->owner[0] != '\0' )
	if( get_clan( ship->owner ) || is_online( ship->owner )
	    || is_online( ship->pilot ) || is_online( ship->copilot ) )
	  return ship;

  return NULL;
}

/*
 * Get pointer to ship structure from ship name.
 */
SHIP_DATA *get_ship( const char *name )
{
  SHIP_DATA *ship = NULL;

  for( ship = first_ship; ship; ship = ship->next )
    if( !str_cmp( name, ship->name ) )
      return ship;

  for( ship = first_ship; ship; ship = ship->next )
    if( nifty_is_name( name, ship->name ) )
      return ship;

  for( ship = first_ship; ship; ship = ship->next )
    if( !str_prefix( name, ship->name ) )
      return ship;

  for( ship = first_ship; ship; ship = ship->next )
    if( nifty_is_name_prefix( name, ship->name ) )
      return ship;

  return NULL;
}

/*
 * Checks if ships in a starsystem and returns poiner if it is.
 */
SHIP_DATA *get_ship_here( const char *name, const SPACE_DATA * starsystem )
{
  SHIP_DATA *ship = NULL;

  if( starsystem == NULL )
    return NULL;

  for( ship = starsystem->first_ship; ship; ship = ship->next_in_starsystem )
    if( !str_cmp( name, ship->name ) )
      return ship;

  for( ship = starsystem->first_ship; ship; ship = ship->next_in_starsystem )
    if( nifty_is_name( name, ship->name ) )
      return ship;

  for( ship = starsystem->first_ship; ship; ship = ship->next_in_starsystem )
    if( !str_prefix( name, ship->name ) )
      return ship;

  for( ship = starsystem->first_ship; ship; ship = ship->next_in_starsystem )
    if( nifty_is_name_prefix( name, ship->name ) )
      return ship;

  return NULL;
}


SHIP_DATA *ship_from_pilot( const char *name )
{
  SHIP_DATA *ship = NULL;

  for( ship = first_ship; ship; ship = ship->next )
    if( !str_cmp( name, ship->pilot ) )
      return ship;
  if( !str_cmp( name, ship->copilot ) )
    return ship;
  if( !str_cmp( name, ship->owner ) )
    return ship;

  return NULL;
}


/*
 * Get pointer to ship structure from cockpit, or entrance ramp vnum.
 */

SHIP_DATA *ship_from_room( const ROOM_INDEX_DATA * room )
{
  SHIP_DATA *ship = NULL;
  ROOM_INDEX_DATA *sRoom = NULL;

  if( room == NULL )
    return NULL;

  for( ship = first_ship; ship; ship = ship->next )
    for( sRoom = ship->first_room; sRoom; sRoom = sRoom->next_in_ship )
      if( room == sRoom )
	return ship;

  return NULL;
}

SHIP_DATA *ship_from_cockpit( const ROOM_INDEX_DATA * room )
{
  SHIP_DATA *ship = NULL;
  TURRET_DATA *turret = NULL;

  if( room == NULL )
    return NULL;

  for( ship = first_ship; ship; ship = ship->next )
  {
    if( room == ship->pilotseat )
      return ship;
    if( room == ship->gunseat )
      return ship;
    if( room == ship->viewscreen )
      return ship;
    for( turret = ship->first_turret; turret; turret = turret->next )
      if( room == turret->room )
	return ship;
  }

  return NULL;
}

SHIP_DATA *ship_from_pilotseat( const ROOM_INDEX_DATA * room )
{
  SHIP_DATA *ship = NULL;

  if( room == NULL )
    return NULL;

  for( ship = first_ship; ship; ship = ship->next )
    if( room == ship->pilotseat )
      return ship;

  return NULL;
}

SHIP_DATA *ship_from_gunseat( const ROOM_INDEX_DATA * room )
{
  SHIP_DATA *ship;

  if( room == NULL )
    return NULL;

  for( ship = first_ship; ship; ship = ship->next )
    if( room == ship->gunseat )
      return ship;

  return NULL;
}

SHIP_DATA *ship_from_entrance( const ROOM_INDEX_DATA * room )
{
  SHIP_DATA *ship;

  if( room == NULL )
    return NULL;

  for( ship = first_ship; ship; ship = ship->next )
    if( room == ship->entrance )
      return ship;

  return NULL;
}

SHIP_DATA *ship_from_engine( const ROOM_INDEX_DATA * room )
{
  SHIP_DATA *ship;

  if( room == NULL )
    return NULL;

  for( ship = first_ship; ship; ship = ship->next )
    if( room == ship->engine )
      return ship;

  return NULL;
}

SHIP_DATA *ship_from_turret( const ROOM_INDEX_DATA * room )
{
  SHIP_DATA *ship;
  TURRET_DATA *turret;

  if( room == NULL )
    return NULL;

  for( ship = first_ship; ship; ship = ship->next )
  {
    if( room == ship->gunseat )
      return ship;
    for( turret = ship->first_turret; turret; turret = turret->next )
      if( room == turret->room )
	return ship;
  }

  return NULL;
}

void save_ship( const SHIP_DATA * ship )
{
  FILE *fp;
  char filename[256];
  char buf[MAX_STRING_LENGTH];
  ROOM_INDEX_DATA *room;

  if( !ship )
  {
    bug( "save_ship: null ship pointer!", 0 );
    return;
  }

  if( ship->type == MOB_SHIP )
    return;

  if( !ship->filename || ship->filename[0] == '\0' )
  {
    sprintf( buf, "save_ship: %s has no filename", ship->name );
    bug( buf, 0 );
    return;
  }

  sprintf( filename, "%s%s", SHIP_DIR, ship->filename );

  if( ( fp = fopen( filename, "w" ) ) == NULL )
  {
    bug( "save_ship: fopen", 0 );
    perror( filename );
  }
  else
  {
    fprintf( fp, "#SHIP\n" );
    fprintf( fp, "Name         %s~\n", ship->name );
    fprintf( fp, "Filename     %s~\n", ship->filename );
    for( room = ship->first_room; room; room = room->next_in_ship )
      fprintf( fp, "Description  %s~\n", room->description );
    fprintf( fp, "Owner        %s~\n", ship->owner );
    fprintf( fp, "Pilot        %s~\n", ship->pilot );
    fprintf( fp, "Copilot      %s~\n", ship->copilot );
    fprintf( fp, "Model        %d\n", ship->model );
    fprintf( fp, "Class        %d\n", ship->ship_class );
    fprintf( fp, "Tractorbeam  %d\n", ship->tractorbeam );
    fprintf( fp, "Shipyard     %d\n", ship->shipyard );
    fprintf( fp, "Laserstate   %d\n", ship->laserstate );
    fprintf( fp, "Lasers       %d\n", ship->lasers );
    fprintf( fp, "Missiles     %d\n", ship->missiles );
    fprintf( fp, "Maxmissiles  %d\n", ship->maxmissiles );
    fprintf( fp, "Lastdoc      %d\n", ship->lastdoc );
    fprintf( fp, "Shield       %d\n", ship->shield );
    fprintf( fp, "Maxshield    %d\n", ship->maxshield );
    fprintf( fp, "Hull         %d\n", ship->hull );
    fprintf( fp, "Maxhull      %d\n", ship->maxhull );
    fprintf( fp, "Maxenergy    %d\n", ship->maxenergy );
    fprintf( fp, "Hyperspeed   %d\n", ship->hyperspeed );
    fprintf( fp, "Chaff        %d\n", ship->chaff );
    fprintf( fp, "Maxchaff     %d\n", ship->maxchaff );
    fprintf( fp, "Realspeed    %d\n", ship->realspeed );
    fprintf( fp, "Type         %d\n", ship->type );
    fprintf( fp, "Shipstate    %d\n", ship->shipstate );
    fprintf( fp, "Missilestate %d\n", ship->missilestate );
    fprintf( fp, "Energy       %d\n", ship->energy );
    fprintf( fp, "Manuever     %d\n", ship->manuever );
    fprintf( fp, "Home         %s~\n", ship->home );
    fprintf( fp, "End\n\n" );
    fprintf( fp, "#END\n" );
  }
  fclose( fp );
}


/*
 * Read in actual ship data.
 */
void fread_ship( SHIP_DATA * ship, FILE * fp )
{
  bool fMatch = FALSE;
  int dIndex = 0;

  for( ;; )
  {
    const char *word = feof( fp ) ? "End" : fread_word( fp );
    fMatch = FALSE;

    switch ( UPPER( word[0] ) )
    {
      case '*':
	fMatch = TRUE;
	fread_to_eol( fp );
	break;

      case 'C':
	KEY( "Class", ship->ship_class, fread_number( fp ) );
	KEY( "Copilot", ship->copilot, fread_string( fp ) );
	KEY( "Chaff", ship->chaff, fread_number( fp ) );
	break;

      case 'D':
	if( dIndex < MAX_SHIP_ROOMS )
	  KEY( "Description", ship->description[dIndex++],
	      fread_string( fp ) );
	break;

      case 'E':
	KEY( "Energy", ship->energy, fread_number( fp ) );
	if( !str_cmp( word, "End" ) )
	{
	  if( !ship->home )
	    ship->home = STRALLOC( "" );
	  if( !ship->name )
	    ship->name = STRALLOC( "" );
	  if( !ship->owner )
	    ship->owner = STRALLOC( "" );
	  if( !ship->copilot )
	    ship->copilot = STRALLOC( "" );
	  if( !ship->pilot )
	    ship->pilot = STRALLOC( "" );
	  if( ship->shipstate != SHIP_DISABLED )
	    ship->shipstate = SHIP_DOCKED;
	  if( ship->laserstate != LASER_DAMAGED )
	    ship->laserstate = LASER_READY;
	  if( ship->missilestate != MISSILE_DAMAGED )
	    ship->missilestate = MISSILE_READY;
	  if( ship->shipyard <= 0 )
	    ship->shipyard = ROOM_LIMBO_SHIPYARD;
	  if( ship->lastdoc <= 0 )
	    ship->lastdoc = ship->shipyard;

	  ship->autopilot = FALSE;
	  ship->hatchopen = FALSE;
	  ship->starsystem = NULL;
	  ship->energy = ship->maxenergy;
	  ship->hull = ship->maxhull;
	  ship->in_room = NULL;
	  ship->next_in_room = NULL;
	  ship->prev_in_room = NULL;
	  ship->first_turret = NULL;
	  ship->last_turret = NULL;
	  ship->first_hangar = NULL;
	  ship->last_hangar = NULL;
	  create_ship_rooms( ship );
	  return;
	}
	break;

      case 'F':
	KEY( "Filename", ship->filename, fread_string_nohash( fp ) );
	break;

      case 'H':
	KEY( "Home", ship->home, fread_string( fp ) );
	KEY( "Hyperspeed", ship->hyperspeed, fread_number( fp ) );
	KEY( "Hull", ship->hull, fread_number( fp ) );
	break;

      case 'L':
	KEY( "Lasers", ship->lasers, fread_number( fp ) );
	KEY( "Laserstate", ship->lasers, fread_number( fp ) );
	KEY( "Lastdoc", ship->lastdoc, fread_number( fp ) );
	break;

      case 'M':
	KEY( "Model", ship->model, fread_number( fp ) );
	KEY( "Manuever", ship->manuever, fread_number( fp ) );
	KEY( "Maxmissiles", ship->maxmissiles, fread_number( fp ) );
	KEY( "Missiles", ship->missiles, fread_number( fp ) );
	KEY( "Maxshield", ship->maxshield, fread_number( fp ) );
	KEY( "Maxenergy", ship->maxenergy, fread_number( fp ) );
	KEY( "Missilestate", ship->missilestate, fread_number( fp ) );
	KEY( "Maxhull", ship->maxhull, fread_number( fp ) );
	KEY( "Maxchaff", ship->maxchaff, fread_number( fp ) );
	break;

      case 'N':
	KEY( "Name", ship->name, fread_string( fp ) );
	break;

      case 'O':
	KEY( "Owner", ship->owner, fread_string( fp ) );
	break;

      case 'P':
	KEY( "Pilot", ship->pilot, fread_string( fp ) );
	break;

      case 'R':
	KEY( "Realspeed", ship->realspeed, fread_number( fp ) );
	break;

      case 'S':
	KEY( "Shipyard", ship->shipyard, fread_number( fp ) );
	KEY( "Shield", ship->shield, fread_number( fp ) );
	KEY( "Shipstate", ship->shipstate, fread_number( fp ) );
	break;

      case 'T':
	KEY( "Type", ship->type, fread_number( fp ) );
	KEY( "Tractorbeam", ship->tractorbeam, fread_number( fp ) );
	break;
    }

    if( !fMatch )
    {
      bug( "Fread_ship: no match: %s", word );
    }
  }
}

/*
 * Load a ship file
 */

bool load_ship_file( const char *shipfile )
{
  char filename[256];
  SHIP_DATA *ship = ship_create();
  FILE *fp = NULL;
  bool found = FALSE;
  CLAN_DATA *clan = NULL;

  sprintf( filename, "%s%s", SHIP_DIR, shipfile );

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
	bug( "Load_ship_file: # not found.", 0 );
	break;
      }

      word = fread_word( fp );

      if( !str_cmp( word, "SHIP" ) )
      {
	fread_ship( ship, fp );
	break;
      }
      else if( !str_cmp( word, "END" ) )
	break;
      else
      {
	char buf[MAX_STRING_LENGTH];

	sprintf( buf, "Load_ship_file: bad section: %s.", word );
	bug( buf, 0 );
	break;
      }
    }

    fclose( fp );
  }

  if( !( found ) )
  {
    DISPOSE( ship );
  }
  else
  {
    LINK( ship, first_ship, last_ship, next, prev );

    if( ship->ship_class == SPACE_STATION || ship->type == MOB_SHIP )
    {
      ship->energy = ship->maxenergy;
      ship->chaff = ship->maxchaff;
      ship->hull = ship->maxhull;

      ship->laserstate = LASER_READY;
      ship->missilestate = LASER_READY;

      ship->missiles = ship->maxmissiles;

      ship_to_starsystem( ship, starsystem_from_name( ship->home ) );
      vector_init( &ship->pos );
      vector_randomize( &ship->pos, -5000, 5000 );
      vector_set( &ship->head, 1, 1, 1 );
      ship->autopilot = TRUE;
      ship->autorecharge = TRUE;
      ship->shield = ship->maxshield;
    }
    else
    {
      ship_to_room( ship, ship->lastdoc );
      ship->location = ship->lastdoc;
    }

    if( ship->type != MOB_SHIP
	&& ( clan = get_clan( ship->owner ) ) != NULL )
    {
      if( ship->ship_class <= SPACE_STATION )
	clan->spacecraft++;
      else
	clan->vehicles++;
    }
  }

  return found;
}

/*
 * Load in all the ship files.
 */
void load_ships()
{
  FILE *fpList = NULL;
  char shiplist[256];

  sprintf( shiplist, "%s%s", SHIP_DIR, SHIP_LIST );

  if( ( fpList = fopen( shiplist, "r" ) ) == NULL )
  {
    perror( shiplist );
    exit( 1 );
  }

  for( ;; )
  {
    const char *filename = feof( fpList ) ? "$" : fread_word( fpList );

    if( filename[0] == '$' )
      break;

    if( !load_ship_file( filename ) )
    {
      bug( "Cannot load ship file: %s", filename );
    }
  }

  fclose( fpList );
}

void resetship( SHIP_DATA * ship )
{
  ship->shipstate = SHIP_READY;

  if( ship->ship_class != SPACE_STATION && ship->type != MOB_SHIP )
  {
    extract_ship( ship );
    ship_to_room( ship, ship->lastdoc );
    ship->location = ship->lastdoc;
    ship->shipstate = SHIP_DOCKED;
    STRFREE( ship->home );
    ship->home = STRALLOC( "" );
  }

  if( ship->starsystem )
    ship_from_starsystem( ship, ship->starsystem );

  ship->currspeed = 0;
  ship->energy = ship->maxenergy;
  ship->chaff = ship->maxchaff;
  ship->hull = ship->maxhull;
  ship->shield = 0;

  ship->laserstate = LASER_READY;
  ship->missilestate = LASER_READY;

  ship->currjump = NULL;
  ship->target = NULL;

  ship->hatchopen = FALSE;

  ship->missiles = ship->maxmissiles;
  ship->autorecharge = FALSE;
  ship->autotrack = FALSE;
  ship->autospeed = FALSE;

  save_ship( ship );
}

void do_resetship( CHAR_DATA * ch, char *argument )
{
  SHIP_DATA *ship = get_ship( argument );

  if( ship == NULL )
  {
    send_to_char( "&RNo such ship!", ch );
    return;
  }

  resetship( ship );

  if( ( ship->ship_class == SPACE_STATION || ship->type == MOB_SHIP )
      && ship->home )
  {
    ship_to_starsystem( ship, starsystem_from_name( ship->home ) );
    vector_init( &ship->pos );
    vector_randomize( &ship->pos, -5000, 5000 );
    ship->shipstate = SHIP_READY;
    ship->autopilot = TRUE;
    ship->autorecharge = TRUE;
    ship->shield = ship->maxshield;
  }
}

void do_setship( CHAR_DATA * ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  SHIP_DATA *ship = NULL;
  int tempnum = 0;
  ROOM_INDEX_DATA *roomindex = NULL;

  if( IS_NPC( ch ) || !ch->pcdata )
    return;

  if( !ch->desc )
    return;

  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );

  if( arg1[0] == '\0' || arg2[0] == '\0' || arg1[0] == '\0' )
  {
    send_to_char( "Usage: setship <ship> <field> <values>\r\n", ch );
    send_to_char( "\r\nField being one of:\r\n", ch );
    send_to_char( "name owner copilot pilot home\r\n", ch );
    send_to_char( "manuever speed hyperspeed tractorbeam\r\n", ch );
    send_to_char( "lasers missiles shield hull energy chaff\r\n", ch );
    send_to_char( "lastdoc class\r\n", ch );
    return;
  }

  ship = get_ship( arg1 );

  if( !ship )
  {
    send_to_char( "No such ship.\r\n", ch );
    return;
  }

  if( !str_cmp( arg2, "owner" ) )
  {
    CLAN_DATA *clan;
    if( ship->type != MOB_SHIP
	&& ( clan = get_clan( ship->owner ) ) != NULL )
    {
      if( ship->ship_class <= SPACE_STATION )
	clan->spacecraft--;
      else
	clan->vehicles--;
    }
    STRFREE( ship->owner );
    ship->owner = STRALLOC( argument );
    send_to_char( "Done.\r\n", ch );
    save_ship( ship );
    if( ship->type != MOB_SHIP
	&& ( clan = get_clan( ship->owner ) ) != NULL )
    {
      if( ship->ship_class <= SPACE_STATION )
	clan->spacecraft++;
      else
	clan->vehicles++;
    }
    return;
  }

  if( !str_cmp( arg2, "home" ) )
  {
    STRFREE( ship->home );
    ship->home = STRALLOC( argument );
    send_to_char( "Done.\r\n", ch );
    save_ship( ship );
    return;
  }

  if( !str_cmp( arg2, "pilot" ) )
  {
    STRFREE( ship->pilot );
    ship->pilot = STRALLOC( argument );
    send_to_char( "Done.\r\n", ch );
    save_ship( ship );
    return;
  }

  if( !str_cmp( arg2, "copilot" ) )
  {
    STRFREE( ship->copilot );
    ship->copilot = STRALLOC( argument );
    send_to_char( "Done.\r\n", ch );
    save_ship( ship );
    return;
  }


  if( !str_cmp( arg2, "shipyard" ) )
  {
    tempnum = atoi( argument );
    roomindex = get_room_index( tempnum );
    if( roomindex == NULL )
    {
      send_to_char( "That room doesn't exist.", ch );
      return;
    }
    ship->shipyard = tempnum;
    send_to_char( "Done.\r\n", ch );
    save_ship( ship );
    return;
  }

  if( !str_cmp( arg2, "name" ) )
  {
    if( *argument == '\0' )
    {
      ch_printf( ch, "Blank name not allowed.\r\n" );
      return;
    }

    if( get_ship( argument ) )
    {
      ch_printf( ch, "A ship with that name already exists.\r\n" );
      return;
    }

    STRFREE( ship->name );
    ship->name = STRALLOC( argument );
    send_to_char( "Done.\r\n", ch );
    save_ship( ship );
    return;
  }

  if( !str_cmp( arg2, "manuever" ) )
  {
    ship->manuever = URANGE( 0, atoi( argument ), 120 );
    send_to_char( "Done.\r\n", ch );
    save_ship( ship );
    return;
  }

  if( !str_cmp( arg2, "lasers" ) )
  {
    ship->lasers = URANGE( 0, atoi( argument ), 10 );
    send_to_char( "Done.\r\n", ch );
    save_ship( ship );
    return;
  }

  if( !str_cmp( arg2, "class" ) )
  {
    ship->ship_class = URANGE( 0, atoi( argument ), 9 );
    send_to_char( "Done.\r\n", ch );
    save_ship( ship );
    return;
  }

  if( !str_cmp( arg2, "lastdoc" ) )
  {
    ship->lastdoc = URANGE( 0, atoi( argument ), 9 );
    send_to_char( "Done.\r\n", ch );
    save_ship( ship );
    return;
  }


  if( !str_cmp( arg2, "missiles" ) )
  {
    ship->maxmissiles = URANGE( 0, atoi( argument ), 255 );
    ship->missiles = URANGE( 0, atoi( argument ), 255 );
    send_to_char( "Done.\r\n", ch );
    save_ship( ship );
    return;
  }

  if( !str_cmp( arg2, "speed" ) )
  {
    ship->realspeed = URANGE( 0, atoi( argument ), 150 );
    send_to_char( "Done.\r\n", ch );
    save_ship( ship );
    return;
  }

  if( !str_cmp( arg2, "tractorbeam" ) )
  {
    ship->tractorbeam = URANGE( 0, atoi( argument ), 255 );
    send_to_char( "Done.\r\n", ch );
    save_ship( ship );
    return;
  }

  if( !str_cmp( arg2, "hyperspeed" ) )
  {
    ship->hyperspeed = URANGE( 0, atoi( argument ), 255 );
    send_to_char( "Done.\r\n", ch );
    save_ship( ship );
    return;
  }

  if( !str_cmp( arg2, "shield" ) )
  {
    ship->maxshield = URANGE( 0, atoi( argument ), 1000 );
    send_to_char( "Done.\r\n", ch );
    save_ship( ship );
    return;
  }

  if( !str_cmp( arg2, "hull" ) )
  {
    ship->hull = URANGE( 1, atoi( argument ), 20000 );
    ship->maxhull = URANGE( 1, atoi( argument ), 20000 );
    send_to_char( "Done.\r\n", ch );
    save_ship( ship );
    return;
  }

  if( !str_cmp( arg2, "energy" ) )
  {
    ship->energy = URANGE( 1, atoi( argument ), 30000 );
    ship->maxenergy = URANGE( 1, atoi( argument ), 30000 );
    send_to_char( "Done.\r\n", ch );
    save_ship( ship );
    return;
  }

  if( !str_cmp( arg2, "chaff" ) )
  {
    ship->chaff = URANGE( 0, atoi( argument ), 25 );
    ship->maxchaff = URANGE( 0, atoi( argument ), 25 );
    send_to_char( "Done.\r\n", ch );
    save_ship( ship );
    return;
  }

  do_setship( ch, STRLIT_EMPTY );
}

void do_showship( CHAR_DATA * ch, char *argument )
{
  SHIP_DATA *ship = NULL;

  if( IS_NPC( ch ) )
  {
    send_to_char( "Huh?\r\n", ch );
    return;
  }

  if( argument[0] == '\0' )
  {
    send_to_char( "Usage: showship <ship>\r\n", ch );
    return;
  }

  ship = get_ship( argument );
  if( !ship )
  {
    send_to_char( "No such ship.\r\n", ch );
    return;
  }
  set_char_color( AT_YELLOW, ch );
  ch_printf( ch, "%s %s : %s\r\nFilename: %s\r\n",
      ship->type == PLAYER_SHIP ? "Player" : "Mob",
      ship->ship_class == SPACECRAFT ? model[ship->model].name :
      ( ship->ship_class == SPACE_STATION ? "Space Station" :
	( ship->ship_class == AIRCRAFT ? "Aircraft" :
	  ( ship->ship_class == BOAT ? "Boat" :
	    ( ship->ship_class == SUBMARINE ? "Submatine" :
	      ( ship->ship_class ==
		LAND_VEHICLE ? "land vehicle" : "Unknown" ) ) ) ) ),
      ship->name, ship->filename );
  ch_printf( ch, "Home: %s\r\nOwner: %s   Pilot: %s   Copilot: %s\r\n",
      ship->home, ship->owner, ship->pilot, ship->copilot );
  ch_printf( ch, "Location: %d   Lastdoc: %d   Shipyard: %d\r\n",
      ship->location, ship->lastdoc, ship->shipyard );
  ch_printf( ch, "Tractor Beam: %d  ", ship->tractorbeam );
  ch_printf( ch, "Lasers: %d  Laser Condition: %s\r\n",
      ship->lasers,
      ship->laserstate == LASER_DAMAGED ? "Damaged" : "Good" );
  ch_printf( ch, "Missiles: %d/%d  Condition: %s\r\n",
      ship->missiles,
      ship->maxmissiles,
      ship->missilestate == MISSILE_DAMAGED ? "Damaged" : "Good" );
  ch_printf( ch, "Hull: %d/%d  Ship Condition: %s\r\n",
      ship->hull,
      ship->maxhull,
      ship->shipstate == SHIP_DISABLED ? "Disabled" : "Running" );

  ch_printf( ch, "Shields: %d/%d   Energy(fuel): %d/%d   Chaff: %d/%d\r\n",
      ship->shield,
      ship->maxshield,
      ship->energy, ship->maxenergy, ship->chaff, ship->maxchaff );
  ch_printf( ch, "Current Coordinates: %.0f %.0f %.0f\r\n",
      ship->pos.x, ship->pos.y, ship->pos.z );
  ch_printf( ch, "Current Heading: %.2f %.2f %.2f\r\n",
      ship->head.x, ship->head.y, ship->head.z );
  ch_printf( ch, "Speed: %d/%d   Hyperspeed: %d\r\n  Manueverability: %d",
      ship->currspeed, ship->realspeed, ship->hyperspeed,
      ship->manuever );
  return;
}

void do_ships( CHAR_DATA * ch, char *argument )
{
  SHIP_DATA *ship = NULL;
  int count = 0;

  if( IS_NPC( ch ) )
    return;

  send_to_char
    ( "&YThe following ships are owned by you or by your organization:\r\n",
      ch );
  send_to_char( "\r\n&WShip                               Location\r\n", ch );

  for( ship = first_ship; ship; ship = ship->next )
  {
    if( str_cmp( ship->owner, ch->name ) )
    {
      if( !ch->pcdata || !ch->pcdata->clan
	  || str_cmp( ship->owner, ch->pcdata->clan->name )
	  || ship->ship_class > SPACE_STATION )
	continue;
    }

    if( ship->type == MOB_SHIP )
      continue;
    set_char_color( AT_BLUE, ch );

    if( ship->in_room )
    {
      if( ship->in_room->area && ship->in_room->area->planet )
	ch_printf( ch, "%s  -%s (%s)\r\n", ship->name,
	    ship->in_room->name,
	    ship->in_room->area->planet->name );
      else
	ch_printf( ch, "%s  -%s\r\n", ship->name, ship->in_room->name );
    }
    else
      ch_printf( ch, "%s\r\n", ship->name );

    count++;
  }

  if( !count )
  {
    send_to_char( "There are no ships owned by you.\r\n", ch );
  }
}

void do_speeders( CHAR_DATA * ch, char *argument )
{
  SHIP_DATA *ship = NULL;
  int count = 0;

  if( IS_NPC( ch ) )
    return;

  send_to_char
    ( "&YThe following are owned by you or by your organization:\r\n", ch );
  send_to_char( "\r\n&WVehicle                            Location\r\n", ch );
  for( ship = first_ship; ship; ship = ship->next )
  {
    if( str_cmp( ship->owner, ch->name ) )
    {
      if( !ch->pcdata || !ch->pcdata->clan
	  || str_cmp( ship->owner, ch->pcdata->clan->name )
	  || ship->ship_class <= SPACE_STATION )
	continue;
    }
    if( ship->location != ch->in_room->vnum
	|| ship->ship_class <= SPACE_STATION )
      continue;

    if( ship->type == MOB_SHIP )
      continue;
    set_char_color( AT_BLUE, ch );

    if( ship->in_room )
    {
      if( ship->in_room->area && ship->in_room->area
	  && ship->in_room->area->planet )
	ch_printf( ch, "%s  -%s (%s)\r\n", ship->name,
	    ship->in_room->name,
	    ship->in_room->area->planet->name );
      else
	ch_printf( ch, "%s  -%s\r\n", ship->name, ship->in_room->name );
    }
    else
      ch_printf( ch, "%s\r\n", ship->name );

    count++;
  }

  if( !count )
  {
    send_to_char( "There are no land or air vehicles owned by you.\r\n",
	ch );
  }
}

void do_allspeeders( CHAR_DATA * ch, char *argument )
{
  SHIP_DATA *ship = NULL;
  int count = 0;

  send_to_char
    ( "&Y\r\nThe following sea/land/air vehicles are currently formed:\r\n",
      ch );
  send_to_char( "\r\n&WVehicle                            Owner\r\n", ch );

  for( ship = first_ship; ship; ship = ship->next )
  {
    if( ship->ship_class <= SPACE_STATION )
      continue;

    if( ship->type == MOB_SHIP )
      continue;

    set_char_color( AT_BLUE, ch );
    ch_printf( ch, "%-35s %-15s\r\n", ship->name, ship->owner );

    count++;
  }

  if( !count )
  {
    send_to_char( "There are none currently formed.\r\n", ch );
    return;
  }
}

void do_allships( CHAR_DATA * ch, char *argument )
{
  SHIP_DATA *ship;
  int count = 0;
  send_to_char( "&Y\r\nThe following ships are currently formed:\r\n", ch );

  send_to_char( "\r\n&WShip                               Owner\r\n", ch );

  for( ship = first_ship; ship; ship = ship->next )
  {
    if( ship->ship_class > SPACE_STATION )
      continue;

    if( ship->type == MOB_SHIP )
      continue;
    set_char_color( AT_BLUE, ch );

    ch_printf( ch, "%-35s %-15s\r\n", ship->name, ship->owner );

    count++;
  }

  if( !count )
  {
    send_to_char( "There are no ships currently formed.\r\n", ch );
    return;
  }
}

void ship_to_starsystem( SHIP_DATA * ship, SPACE_DATA * starsystem )
{
  if( starsystem == NULL || ship == NULL )
    return;

  LINK( ship, starsystem->first_ship, starsystem->last_ship,
      next_in_starsystem, prev_in_starsystem );

  ship->starsystem = starsystem;
}

void new_missile( SHIP_DATA * ship, SHIP_DATA * target, CHAR_DATA * ch )
{
  SPACE_DATA *starsystem = NULL;
  MISSILE_DATA *missile = NULL;

  if( ship == NULL )
    return;

  if( target == NULL )
    return;

  if( ( starsystem = ship->starsystem ) == NULL )
    return;

  CREATE( missile, MISSILE_DATA, 1 );
  LINK( missile, first_missile, last_missile, next, prev );

  vector_init( &missile->pos );
  vector_init( &missile->head );

  missile->target = target;
  missile->fired_from = ship;

  if( ch )
    missile->fired_by = STRALLOC( ch->name );
  else
    missile->fired_by = STRALLOC( "" );

  missile->age = 0;
  missile->speed = 200;

  vector_copy( &missile->pos, &ship->pos );
  missile_set_course_to_ship( missile, target );

  if( starsystem->first_missile == NULL )
    starsystem->first_missile = missile;

  if( starsystem->last_missile )
  {
    starsystem->last_missile->next_in_starsystem = missile;
    missile->prev_in_starsystem = starsystem->last_missile;
  }

  starsystem->last_missile = missile;
  missile->starsystem = starsystem;
}

void ship_from_starsystem( SHIP_DATA * ship, SPACE_DATA * starsystem )
{
  if( starsystem == NULL || ship == NULL )
    return;

  UNLINK( ship, starsystem->first_ship, starsystem->last_ship,
      next_in_starsystem, prev_in_starsystem );

  ship->starsystem = NULL;
}

void extract_missile( MISSILE_DATA * missile )
{
  SPACE_DATA *starsystem = NULL;

  if( missile == NULL )
    return;

  if( ( starsystem = missile->starsystem ) != NULL )
  {
    if( starsystem->last_missile == missile )
      starsystem->last_missile = missile->prev_in_starsystem;

    if( starsystem->first_missile == missile )
      starsystem->first_missile = missile->next_in_starsystem;

    if( missile->prev_in_starsystem )
      missile->prev_in_starsystem->next_in_starsystem =
	missile->next_in_starsystem;

    if( missile->next_in_starsystem )
      missile->next_in_starsystem->prev_in_starsystem =
	missile->prev_in_starsystem;

    missile->starsystem = NULL;
    missile->next_in_starsystem = NULL;
    missile->prev_in_starsystem = NULL;
  }

  UNLINK( missile, first_missile, last_missile, next, prev );

  missile->target = NULL;
  missile->fired_from = NULL;

  if( missile->fired_by )
    STRFREE( missile->fired_by );

  DISPOSE( missile );
}

bool check_pilot( const CHAR_DATA * ch, const SHIP_DATA * ship )
{
  if( !str_cmp( ch->name, ship->owner ) || !str_cmp( ch->name, ship->pilot )
      || !str_cmp( ch->name, ship->copilot ) )
    return TRUE;

  if( !IS_NPC( ch ) && ch->pcdata && ch->pcdata->clan )
  {
    if( !str_cmp( ch->pcdata->clan->name, ship->owner ) )
    {
      if( clan_char_has_permission( ch->pcdata->clan, ch, CLAN_PERM_PILOT ) )
	return TRUE;
    }
  }

  return FALSE;
}

bool extract_ship( SHIP_DATA * ship )
{
  ROOM_INDEX_DATA *room;

  if( ( room = ship->in_room ) != NULL )
  {
    UNLINK( ship, room->first_ship, room->last_ship, next_in_room,
	prev_in_room );
    ship->in_room = NULL;
  }
  return TRUE;
}

void damage_ship( SHIP_DATA * ship, int min, int max, CHAR_DATA * ch )
{
  int damg = number_range( min, max );

  if( ship->shield > 0 )
  {
    int shield_dmg = UMIN( ship->shield, damg );
    damg -= shield_dmg;
    ship->shield -= shield_dmg;
    if( ship->shield == 0 )
      echo_to_cockpit( AT_BLOOD, ship, "Shields down..." );
  }

  if( damg > 0 )
  {
    if( number_range( 1, 100 ) <= 5 && ship->shipstate != SHIP_DISABLED )
    {
      echo_to_cockpit( AT_BLOOD + AT_BLINK, ship,
	  "Ship's drive DAMAGED!" );
      ship->shipstate = SHIP_DISABLED;
    }

    if( number_range( 1, 100 ) <= 5 && ship->missilestate != MISSILE_DAMAGED
	&& ship->maxmissiles > 0 )
    {
      echo_to_room( AT_BLOOD + AT_BLINK, ship->gunseat,
	  "Ship's missile launcher DAMAGED!" );
      ship->missilestate = MISSILE_DAMAGED;
    }

    if( number_range( 1, 100 ) <= 2 && ship->laserstate != LASER_DAMAGED )
    {
      echo_to_room( AT_BLOOD + AT_BLINK, ship->gunseat,
	  "Lasers DAMAGED!" );
      ship->laserstate = LASER_DAMAGED;
    }
  }

  ship->hull -= damg * 5;

  if( ship->hull <= 0 )
  {
    destroy_ship( ship, ch );
    return;
  }

  if( ship->hull <= ship->maxhull / 20 )
    echo_to_cockpit( AT_BLOOD + AT_BLINK, ship,
	"WARNING! Ship hull severely damaged!" );
}

void ship_clear_turret_targets( SHIP_DATA * ship )
{
  TURRET_DATA *turret = NULL;

  for( turret = ship->first_turret; turret; turret = turret->next )
  {
    turret->target = NULL;
  }
}

void ship_free_room_descriptions( SHIP_DATA * ship )
{
  size_t dIndex = 0;

  for( dIndex = 0; dIndex < MAX_SHIP_ROOMS; dIndex++ )
  {
    if( ship->description[dIndex] )
    {
      STRFREE( ship->description[dIndex] );
    }
  }
}

void ship_untarget_by_attackers( const SHIP_DATA * ship )
{
  SHIP_DATA *att = NULL;

  for( att = first_ship; att; att = att->next )
  {
    TURRET_DATA *turret = NULL;

    if( att->target == ship )
    {
      att->target = NULL;
    }

    for( turret = att->first_turret; turret; turret = turret->next )
    {
      if( turret->target == ship )
      {
	turret->target = NULL;
      }
    }
  }
}

void ship_untarget_by_missiles( const SHIP_DATA * ship )
{
  MISSILE_DATA *missile = NULL;
  MISSILE_DATA *m_next = NULL;

  for( missile = first_missile; missile; missile = m_next )
  {
    m_next = missile->next;

    if( missile->target && missile->target == ship )
      extract_missile( missile );

    if( missile->fired_from && missile->fired_from == ship )
      extract_missile( missile );
  }
}

void destroy_ship_kill_characters( const SHIP_DATA * ship,
    CHAR_DATA * killer )
{
  ROOM_INDEX_DATA *room = NULL;

  for( room = ship->first_room; room; room = room->next_in_ship )
  {
    CHAR_DATA *rch = room->first_person;

    while( rch )
    {
      bool survived = FALSE;

      if( !IS_NPC( rch ) && ship->starsystem
	  && ship->starsystem->last_planet
	  && ship->starsystem->last_planet->area )
      {
	ROOM_INDEX_DATA *pRoom = NULL;
	OBJ_DATA *scraps = NULL;
	int rnum = 0;
	int tnum =
	  number_range( 0,
	      ship->starsystem->last_planet->wilderness +
	      ship->starsystem->last_planet->farmland );

	for( pRoom = ship->starsystem->last_planet->area->first_room;
	    pRoom; pRoom = pRoom->next_in_area )
	  if( pRoom->sector_type != SECT_CITY
	      && pRoom->sector_type != SECT_DUNNO
	      && pRoom->sector_type != SECT_INSIDE
	      && pRoom->sector_type != SECT_UNDERGROUND )
	  {
	    if( rnum++ < tnum )
	      continue;

	    char_from_room( rch );
	    char_to_room( rch, pRoom );

	    if( !IS_IMMORTAL( rch ) )
	      rch->hit = -1;

	    update_pos( rch );
	    echo_to_room( AT_WHITE, rch->in_room,
		"There is loud explosion as an escape pod hits the earth." );

	    scraps =
	      create_object( get_obj_index( OBJ_VNUM_SCRAPS ) );
	    scraps->timer = 15;
	    STRFREE( scraps->short_descr );
	    scraps->short_descr = STRALLOC( "a battered escape pod" );
	    STRFREE( scraps->description );
	    scraps->description =
	      STRALLOC
	      ( "The smoking shell of an escape pod litters the earth.\r\n" );
	    obj_to_room( scraps, pRoom );

	    survived = TRUE;
	    break;
	  }
      }

      if( !survived && IS_IMMORTAL( rch ) )
      {
	char_from_room( rch );
	char_to_room( rch, get_room_index( wherehome( rch ) ) );
	survived = TRUE;
      }

      if( !survived )
      {
	if( killer )
	{
	  raw_kill( killer, rch );
	}
	else
	{
	  raw_kill( rch, rch );
	}
      }

      rch = room->first_person;
    }

    room_extract_contents( room );
  }
}

void destroy_ship( SHIP_DATA * ship, CHAR_DATA * killer )
{
  char buf[MAX_STRING_LENGTH];
  CLAN_DATA *clan = NULL;

  sprintf( buf, "%s explodes in a blinding flash of light!", ship->name );
  echo_to_system( AT_WHITE + AT_BLINK, ship, buf, NULL );

  sprintf( buf, "%s destroyed by %s", ship->name,
      killer ? killer->name : "(none)" );
  log_string( buf );
  echo_to_ship( AT_WHITE, ship,
      "The ship is shaken by a FATAL explosion. You realize it's escape or perish." );
  echo_to_ship( AT_WHITE, ship,
      "The last thing you remember is reaching for the escape pod release lever." );
  echo_to_ship( AT_WHITE + AT_BLINK, ship, "A blinding flash of light." );
  echo_to_ship( AT_WHITE, ship, "And then darkness...." );

  destroy_ship_kill_characters( ship, killer );

  if( ship->starsystem )
    ship_from_starsystem( ship, ship->starsystem );

  extract_ship( ship );

  clan = get_clan( ship->owner );

  if( clan )
  {
    clan_decrease_vehicles_owned( clan, ship );
  }

  STRFREE( ship->owner );
  STRFREE( ship->pilot );
  STRFREE( ship->copilot );
  STRFREE( ship->home );
  ship->owner = STRALLOC( "" );
  ship->pilot = STRALLOC( "" );
  ship->copilot = STRALLOC( "" );
  ship->home = STRALLOC( "" );
  ship->target = NULL;

  ship_clear_turret_targets( ship );
  ship_free_room_descriptions( ship );

  UNLINK( ship, first_ship, last_ship, next, prev );

  ship_untarget_by_attackers( ship );
  ship_untarget_by_missiles( ship );

  write_ship_list();
}

bool ship_to_room( SHIP_DATA * ship, long vnum )
{
  ROOM_INDEX_DATA *shipto = NULL;

  if( ( shipto = get_room_index( vnum ) ) == NULL )
    return FALSE;

  LINK( ship, shipto->first_ship, shipto->last_ship, next_in_room,
      prev_in_room );
  ship->in_room = shipto;
  return TRUE;
}

void do_board( CHAR_DATA * ch, char *argument )
{
  ROOM_INDEX_DATA *fromroom;
  ROOM_INDEX_DATA *toroom;
  SHIP_DATA *ship;

  if( !argument || argument[0] == '\0' )
  {
    send_to_char( "Board what?\r\n", ch );
    return;
  }

  if( ( ship = ship_in_room( ch->in_room, argument ) ) == NULL )
  {
    act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
    return;
  }

  if( IS_SET( ch->act, ACT_MOUNTED ) )
  {
    act( AT_PLAIN, "You can't go in there riding THAT.", ch, NULL, argument,
	TO_CHAR );
    return;
  }

  fromroom = ch->in_room;

  if( ( toroom = ship->entrance ) != NULL )
  {
    if( !ship->hatchopen )
    {
      send_to_char( "&RThe hatch is closed!\r\n", ch );
      return;
    }

    if( toroom->tunnel > 0 )
    {
      CHAR_DATA *ctmp;
      int count = 0;

      for( ctmp = toroom->first_person; ctmp; ctmp = ctmp->next_in_room )
	if( ++count >= toroom->tunnel )
	{
	  send_to_char( "There is no room for you in there.\r\n", ch );
	  return;
	}
    }

    if( ship->shipstate == SHIP_LAUNCH || ship->shipstate == SHIP_LAUNCH_2 )
    {
      send_to_char( "&rThat ship has already started launching!\r\n",
	  ch );
      return;
    }

    act( AT_PLAIN, "$n enters $T.", ch, NULL, ship->name, TO_ROOM );
    act( AT_PLAIN, "You enter $T.", ch, NULL, ship->name, TO_CHAR );
    char_from_room( ch );
    char_to_room( ch, toroom );
    act( AT_PLAIN, "$n enters the ship.", ch, NULL, argument, TO_ROOM );
    do_look( ch, STRLIT_AUTO );

  }
  else
    send_to_char( "That ship has no entrance!\r\n", ch );
}

void do_leaveship( CHAR_DATA * ch, char *argument )
{
  ROOM_INDEX_DATA *fromroom;
  ROOM_INDEX_DATA *toroom;
  SHIP_DATA *ship;

  fromroom = ch->in_room;

  if( ( ship = ship_from_entrance( fromroom ) ) == NULL )
  {
    send_to_char( "I see no exit here.\r\n", ch );
    return;
  }

  if( ship->ship_class == SPACE_STATION )
  {
    send_to_char( "You can't do that here.\r\n", ch );
    return;
  }

  if( ship->lastdoc != ship->location )
  {
    send_to_char( "&rMaybe you should wait until the ship lands.\r\n", ch );
    return;
  }

  if( ship->shipstate != SHIP_DOCKED && ship->shipstate != SHIP_DISABLED )
  {
    send_to_char( "&rPlease wait till the ship is properly docked.\r\n",
	ch );
    return;
  }

  if( !ship->hatchopen )
  {
    send_to_char( "&RYou need to open the hatch first", ch );
    return;
  }

  if( ( toroom = get_room_index( ship->location ) ) != NULL )
  {
    act( AT_PLAIN, "$n exits the ship.", ch, NULL, argument, TO_ROOM );
    act( AT_PLAIN, "You exit the ship.", ch, NULL, argument, TO_CHAR );
    char_from_room( ch );
    char_to_room( ch, toroom );
    act( AT_PLAIN, "$n steps out of a ship.", ch, NULL, argument, TO_ROOM );
    do_look( ch, STRLIT_AUTO );
  }
  else
    send_to_char( "The exit doesn't seem to be working properly.\r\n", ch );
}

void do_launch( CHAR_DATA * ch, char *argument )
{
  int chance = 0;
  long price = 0;
  SHIP_DATA *ship = NULL;
  char buf[MAX_STRING_LENGTH];

  if( ( ship = ship_from_pilotseat( ch->in_room ) ) == NULL )
  {
    send_to_char
      ( "&RYou must be in the pilots seat of a ship to do that!\r\n", ch );
    return;
  }

  if( ship->ship_class > SPACE_STATION )
  {
    send_to_char( "&RThis isn't a spacecraft!\r\n", ch );
    return;
  }

  if( autofly( ship ) )
  {
    send_to_char
      ( "&RThe ship is set on autopilot, you'll have to turn it off first.\r\n",
	ch );
    return;
  }

  if( ship->ship_class == SPACE_STATION )
  {
    send_to_char( "You can't do that here.\r\n", ch );
    return;
  }

  if( !check_pilot( ch, ship ) )
  {
    send_to_char
      ( "&RHey, thats not your ship! Try renting a public one.\r\n", ch );
    return;
  }

  if( ship->lastdoc != ship->location )
  {
    send_to_char( "&rYou don't seem to be docked right now.\r\n", ch );
    return;
  }

  if( ship->shipstate != SHIP_DOCKED && ship->shipstate != SHIP_DISABLED )
  {
    send_to_char( "The ship is not docked right now.\r\n", ch );
    return;
  }

  chance = character_skill_level( ch, gsn_spacecraft );

  if( number_percent() < chance )
  {
    price = 20;
    price += ( ship->maxhull - ship->hull );

    if( ship->missiles )
      price += ( 50 * ( ship->maxmissiles - ship->missiles ) );

    if( ship->shipstate == SHIP_DISABLED )
      price += 200;

    if( ship->missilestate == MISSILE_DAMAGED )
      price += 100;

    if( ship->laserstate == LASER_DAMAGED )
      price += 50;

    if( ch->pcdata && ch->pcdata->clan
	&& !str_cmp( ch->pcdata->clan->name, ship->owner ) )
    {
      if( ch->pcdata->clan->funds < price )
      {
	ch_printf( ch,
	    "&R%s doesn't have enough funds to prepare this ship for launch.\r\n",
	    ch->pcdata->clan->name );
	return;
      }

      ch->pcdata->clan->funds -= price;
      ch_printf( ch,
	  "&GIt costs %s %ld credits to ready this ship for launch.\r\n",
	  ch->pcdata->clan->name, price );
    }
    else
    {
      if( ch->gold < price )
      {
	ch_printf( ch,
	    "&RYou don't have enough funds to prepare this ship for launch.\r\n" );
	return;
      }

      ch->gold -= price;
      ch_printf( ch,
	  "&GYou pay %ld credits to ready the ship for launch.\r\n",
	  price );
    }

    ship->energy = ship->maxenergy;
    ship->chaff = ship->maxchaff;
    ship->missiles = ship->maxmissiles;
    ship->shield = 0;
    ship->autorecharge = FALSE;
    ship->autotrack = FALSE;
    ship->autospeed = FALSE;
    ship->hull = ship->maxhull;
    ship->missilestate = MISSILE_READY;
    ship->laserstate = LASER_READY;
    ship->shipstate = SHIP_DOCKED;

    if( ship->hatchopen )
    {
      ship->hatchopen = FALSE;
      sprintf( buf, "The hatch on %s closes.", ship->name );
      echo_to_room( AT_YELLOW, get_room_index( ship->location ), buf );
      echo_to_room( AT_YELLOW, ship->entrance, "The hatch slides shut." );
      sound_to_room( ship->entrance, "!!SOUND(door)" );
      sound_to_room( get_room_index( ship->location ), "!!SOUND(door)" );
    }

    set_char_color( AT_GREEN, ch );
    send_to_char( "Launch sequence initiated.\r\n", ch );
    act( AT_PLAIN, "$n starts up the ship and begins the launch sequence.",
	ch, NULL, argument, TO_ROOM );
    echo_to_ship( AT_YELLOW, ship,
	"The ship hums as it lifts off the ground." );
    sprintf( buf, "%s begins to launch.", ship->name );
    echo_to_room( AT_YELLOW, get_room_index( ship->location ), buf );
    ship->shipstate = SHIP_LAUNCH;
    ship->currspeed = ship->realspeed;
    learn_from_success( ch, gsn_spacecraft );
    return;
  }

  set_char_color( AT_RED, ch );
  send_to_char( "You fail to work the controls properly!\r\n", ch );
}

void launchship( SHIP_DATA * ship )
{
  char buf[MAX_STRING_LENGTH];
  SHIP_DATA *source = NULL;
  ROOM_INDEX_DATA *room = NULL;

  ship_to_starsystem( ship, starsystem_from_room( ship->in_room ) );

  if( ship->starsystem == NULL )
  {
    echo_to_room( AT_YELLOW, ship->pilotseat,
	"Launch path blocked .. Launch aborted." );
    echo_to_ship( AT_YELLOW, ship,
	"The ship slowly sets back back down on the landing pad." );
    sprintf( buf, "%s slowly sets back down.", ship->name );
    echo_to_room( AT_YELLOW, get_room_index( ship->location ), buf );
    ship->shipstate = SHIP_DOCKED;
    return;
  }

  source = ship_from_room( ship->in_room );
  extract_ship( ship );

  ship->location = 0;

  if( ship->shipstate != SHIP_DISABLED )
    ship->shipstate = SHIP_READY;

  vector_randomize( &ship->head, -1, 2 );
  vector_normalize( &ship->head );

  if( ( room = get_room_index( ship->lastdoc ) ) != NULL &&
      room->area && room->area->planet && room->area->planet->starsystem
      && room->area->planet->starsystem == ship->starsystem )
  {
    vector_copy( &ship->pos, &room->area->planet->pos );
  }
  else if( source )
  {
    vector_copy( &ship->pos, &source->pos );
  }

  ship->energy -= ( 100 + 10 * ship->model );

  ship_move( ship );		/* move it away */

  echo_to_room( AT_GREEN, ship->pilotseat, "Launch complete.\r\n" );
  echo_to_ship( AT_YELLOW, ship,
      "The ship leaves the platform far behind as it flies into space." );
  sprintf( buf, "%s enters the starsystem at %.0f %.0f %.0f", ship->name,
      ship->pos.x, ship->pos.y, ship->pos.z );
  echo_to_system( AT_YELLOW, ship, buf, NULL );
  sprintf( buf, "%s lifts off into space.", ship->name );
  echo_to_room( AT_YELLOW, get_room_index( ship->lastdoc ), buf );
}


void do_land( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  char arg1[MAX_INPUT_LENGTH];
  int chance = 0;
  SHIP_DATA *ship = NULL;
  SHIP_DATA *target = NULL;
  PLANET_DATA *planet = NULL;
  bool pfound = FALSE;
  ROOM_INDEX_DATA *room = NULL;
  bool rfound = FALSE;

  strcpy( arg, argument );
  argument = one_argument( argument, arg1 );

  if( ( ship = ship_from_pilotseat( ch->in_room ) ) == NULL )
  {
    send_to_char
      ( "&RYou must be in the pilots seat of a ship to do that!\r\n", ch );
    return;
  }

  if( ship->ship_class > SPACE_STATION )
  {
    send_to_char( "&RThis isn't a spacecraft!\r\n", ch );
    return;
  }

  if( autofly( ship ) )
  {
    send_to_char
      ( "&RYou'll have to turn off the ships autopilot first.\r\n", ch );
    return;
  }

  if( ship->ship_class == SPACE_STATION )
  {
    send_to_char( "&RYou can't land space stations.\r\n", ch );
    return;
  }

  if( ship->shipstate == SHIP_DISABLED )
  {
    send_to_char( "&RThe ships drive is disabled. Unable to land.\r\n",
	ch );
    return;
  }
  if( ship->shipstate == SHIP_DOCKED )
  {
    send_to_char( "&RThe ship is already docked!\r\n", ch );
    return;
  }

  if( ship->shipstate == SHIP_HYPERSPACE )
  {
    send_to_char( "&RYou can only do that in realspace!\r\n", ch );
    return;
  }

  if( ship->shipstate != SHIP_READY )
  {
    send_to_char
      ( "&RPlease wait until the ship has finished its current maneuver.\r\n",
	ch );
    return;
  }

  if( ship->starsystem == NULL )
  {
    send_to_char( "&RThere's nowhere to land around here!", ch );
    return;
  }

  if( ship->energy < ( 25 + 5 * ship->model ) )
  {
    send_to_char( "&RTheres not enough fuel!\r\n", ch );
    return;
  }

  if( arg[0] == '\0' )
  {
    set_char_color( AT_CYAN, ch );
    ch_printf( ch, "%s", "Land where?\r\n\r\nChoices: \r\n" );

    for( target = ship->starsystem->first_ship; target;
	target = target->next_in_starsystem )
    {
      if( target->ship_class == SPACE_STATION && target != ship )
	ch_printf( ch, "%s    %.0f %.0f %.0f\r\n         ",
	    target->name,
	    target->pos.x, target->pos.y, target->pos.z );
    }

    for( planet = ship->starsystem->first_planet; planet;
	planet = planet->next_in_system )
      ch_printf( ch, "%s   %.0f %.0f %.0f\r\n", planet->name, planet->pos.x,
	  planet->pos.y, planet->pos.z );

    ch_printf( ch, "\r\nYour Coordinates: %.0f %.0f %.0f\r\n",
	ship->pos.x, ship->pos.y, ship->pos.z );
    return;
  }

  for( planet = ship->starsystem->first_planet; planet;
      planet = planet->next_in_system )
  {
    if( !str_prefix( arg1, planet->name ) )
    {
      pfound = TRUE;

      if( !planet->area )
      {
	send_to_char
	  ( "&RThat planet doesn't have any landing areas.\r\n", ch );
	return;
      }

      if( ship_distance_to_planet( ship, planet ) > 200 )
      {
	send_to_char
	  ( "&RThat planet is too far away! You'll have to fly a little closer.\r\n",
	    ch );
	return;
      }

      if( argument[0] != '\0' )
	for( room = planet->area->first_room; room;
	    room = room->next_in_area )
	{
	  if( IS_SET( room->room_flags, ROOM_CAN_LAND )
	      && !str_prefix( argument, room->name ) )
	  {
	    rfound = TRUE;
	    break;
	  }
	}

      if( !rfound )
      {
	send_to_char
	  ( "&CPlease type the location after the planet name.\r\n",
	    ch );
	ch_printf( ch, "Possible choices for %s:\r\n\r\n",
	    planet->name );
	for( room = planet->area->first_room; room;
	    room = room->next_in_area )
	  if( IS_SET( room->room_flags, ROOM_CAN_LAND ) )
	    ch_printf( ch, "%s\r\n", room->name );
	return;
      }

      break;
    }
  }

  if( !pfound )
  {
    target = get_ship_here( arg, ship->starsystem );

    if( target != NULL )
    {
      if( target == ship )
      {
	send_to_char( "&RYou can't land your ship inside itself!\r\n",
	    ch );
	return;
      }
      if( target->ship_class != SPACE_STATION )
      {
	send_to_char
	  ( "&RThat ship has no hangar for you to land in!\r\n", ch );
	return;
      }

      if( ship_distance_to_ship( ship, target ) > 200 )
      {
	send_to_char
	  ( "&R That ship is too far away! You'll have to fly a little closer.\r\n",
	    ch );
	return;
      }
    }
    else
    {
      send_to_char( "&RI don't see that here.\r\n&W", ch );
      do_land( ch, STRLIT_EMPTY );
      return;
    }
  }

  chance = character_skill_level( ch, gsn_spacecraft );

  if( number_percent() < chance )
  {
    set_char_color( AT_GREEN, ch );
    send_to_char( "Landing sequence initiated.\r\n", ch );
    act( AT_PLAIN, "$n begins the landing sequence.", ch,
	NULL, "", TO_ROOM );
    echo_to_ship( AT_YELLOW, ship,
	"The ship slowly begins its landing approach." );
    ship->dest = STRALLOC( arg );
    ship->shipstate = SHIP_LAND;
    ship->currspeed = 0;
    if( number_percent() == 23 )
    {
      send_to_char
	( "Your experience makes you feel more coordinated than before.\r\n",
	  ch );
      ch->perm_dex++;
      ch->perm_dex = UMIN( ch->perm_dex, 25 );
    }

    learn_from_success( ch, gsn_spacecraft );
    return;
  }

  send_to_char( "You fail to work the controls properly.\r\n", ch );
}

void landship( SHIP_DATA * ship, char *argument )
{
  SHIP_DATA *target = 0;
  char buf[MAX_STRING_LENGTH];
  int destination = 0;
  char arg[MAX_INPUT_LENGTH];
  char arg1[MAX_INPUT_LENGTH];
  PLANET_DATA *planet = NULL;
  ROOM_INDEX_DATA *room = NULL;

  strcpy( arg, argument );
  argument = one_argument( argument, arg1 );

  for( planet = ship->starsystem->first_planet; planet;
      planet = planet->next_in_system )
  {
    if( !str_prefix( arg1, planet->name ) )
    {
      if( !planet->area )
	continue;
      for( room = planet->area->first_room; room;
	  room = room->next_in_area )
      {
	if( IS_SET( room->room_flags, ROOM_CAN_LAND )
	    && !str_prefix( argument, room->name ) )
	{
	  destination = room->vnum;
	  break;
	}
      }
      break;
    }
  }

  if( !destination )
  {
    target = get_ship_here( arg, ship->starsystem );
    if( target != ship && target != NULL )
      destination = 0;
    destination = 0;		/* landing on ships is disabled */
  }

  if( !ship_to_room( ship, destination ) )
  {
    echo_to_room( AT_YELLOW, ship->pilotseat,
	"Could not complete approach. Landing aborted." );
    echo_to_ship( AT_YELLOW, ship,
	"The ship pulls back up out of its landing sequence." );
    if( ship->shipstate != SHIP_DISABLED )
      ship->shipstate = SHIP_READY;
    return;
  }

  echo_to_room( AT_YELLOW, ship->pilotseat, "Landing sequence complete." );
  echo_to_ship( AT_YELLOW, ship,
      "You feel a slight thud as the ship sets down on the ground." );
  sprintf( buf, "%s disappears from your scanner.", ship->name );
  echo_to_system( AT_YELLOW, ship, buf, NULL );

  ship->location = destination;
  ship->lastdoc = ship->location;
  if( ship->shipstate != SHIP_DISABLED )
    ship->shipstate = SHIP_DOCKED;
  ship_from_starsystem( ship, ship->starsystem );

  sprintf( buf, "%s lands on the platform.", ship->name );
  echo_to_room( AT_YELLOW, get_room_index( ship->location ), buf );

  ship->energy = ship->energy - 25 - 5 * ship->model;

  save_ship( ship );
}

void do_accelerate( CHAR_DATA * ch, char *argument )
{
  int chance = 0;
  int change = 0;
  SHIP_DATA *ship = NULL;
  char buf[MAX_STRING_LENGTH];

  if( ( ship = ship_from_pilotseat( ch->in_room ) ) == NULL )
  {
    send_to_char( "&RYou must be at the controls of a ship to do that!\r\n",
	ch );
    return;
  }

  if( ship->ship_class > SPACE_STATION )
  {
    send_to_char( "&RThis isn't a spacecraft!\r\n", ch );
    return;
  }

  if( autofly( ship ) )
  {
    send_to_char
      ( "&RYou'll have to turn off the ships autopilot first.\r\n", ch );
    return;
  }

  if( ship->ship_class == SPACE_STATION )
  {
    send_to_char( "&RPlatforms can't move!\r\n", ch );
    return;
  }

  if( ship->shipstate == SHIP_HYPERSPACE )
  {
    send_to_char( "&RYou can only do that in realspace!\r\n", ch );
    return;
  }

  if( ship->shipstate == SHIP_DISABLED )
  {
    send_to_char
      ( "&RThe ships drive is disabled. Unable to accelerate.\r\n", ch );
    return;
  }

  if( ship->shipstate == SHIP_DOCKED )
  {
    send_to_char( "&RYou can't do that until after you've launched!\r\n",
	ch );
    return;
  }

  if( ship->energy <
      abs( ( atoi( argument ) - abs( ship->currspeed ) ) / 10 ) )
  {
    send_to_char( "&RTheres not enough fuel!\r\n", ch );
    return;
  }

  chance = character_skill_level( ch, gsn_spacecraft );

  if( number_percent() >= chance )
  {
    send_to_char( "&RYou fail to work the controls properly.\r\n", ch );
    return;
  }

  change = atoi( argument );
  act( AT_PLAIN, "$n manipulates the ships controls.", ch,
      NULL, argument, TO_ROOM );

  if( change > ship->currspeed )
  {
    send_to_char( "&GAccelerating\r\n", ch );
    echo_to_cockpit( AT_YELLOW, ship, "The ship begins to accelerate." );
    sprintf( buf, "%s begins to speed up.", ship->name );
    echo_to_system( AT_ORANGE, ship, buf, NULL );
  }

  if( change < ship->currspeed )
  {
    send_to_char( "&GDecelerating\r\n", ch );
    echo_to_cockpit( AT_YELLOW, ship, "The ship begins to slow down." );
    sprintf( buf, "%s begins to slow down.", ship->name );
    echo_to_system( AT_ORANGE, ship, buf, NULL );
  }

  ship->energy -= abs( ( change - abs( ship->currspeed ) ) / 10 );
  ship->currspeed = URANGE( 0, change, ship->realspeed );
  learn_from_success( ch, gsn_spacecraft );
}

void do_trajectory( CHAR_DATA * ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  int chance = 0;
  SHIP_DATA *ship = NULL;
  Vector3 vec;
  vector_init( &vec );

  if( ( ship = ship_from_pilotseat( ch->in_room ) ) == NULL )
  {
    send_to_char( "&RYou must be at the healm of a ship to do that!\r\n",
	ch );
    return;
  }

  if( ship->ship_class > SPACE_STATION )
  {
    send_to_char( "&RThis isn't a spacecraft!\r\n", ch );
    return;
  }

  if( autofly( ship ) )
  {
    send_to_char
      ( "&RYou'll have to turn off the ships autopilot first.\r\n", ch );
    return;
  }

  if( ship->shipstate == SHIP_DISABLED )
  {
    send_to_char( "&RThe ships drive is disabled. Unable to manuever.\r\n",
	ch );
    return;
  }

  if( ship->ship_class == SPACE_STATION )
  {
    send_to_char( "&RPlatforms can't turn!\r\n", ch );
    return;
  }

  if( ship->shipstate == SHIP_HYPERSPACE )
  {
    send_to_char( "&RYou can only do that in realspace!\r\n", ch );
    return;
  }

  if( ship->shipstate == SHIP_DOCKED )
  {
    send_to_char( "&RYou can't do that until after you've launched!\r\n",
	ch );
    return;
  }

  if( ship->shipstate != SHIP_READY )
  {
    send_to_char
      ( "&RPlease wait until the ship has finished its current maneuver.\r\n",
	ch );
    return;
  }

  if( ship->energy < ( ship->currspeed / 10 ) )
  {
    send_to_char( "&RTheres not enough fuel!\r\n", ch );
    return;
  }

  chance = character_skill_level( ch, gsn_spacecraft );

  if( number_percent() > chance )
  {
    send_to_char( "&RYou fail to work the controls properly.\r\n", ch );
    return;
  }

  argument = one_argument( argument, arg2 );
  argument = one_argument( argument, arg3 );

  vector_set( &vec, atof( arg2 ), atof( arg3 ), atof( argument ) );

  if( vector_distance( &ship->pos, &vec ) < 1 )
  {
    ch_printf( ch, "The ship is already at %.0f %.0f %.0f !",
	vec.x, vec.y, vec.z );
  }

  ship_set_course( ship, &vec );
  ship->energy -= ( ship->currspeed / 10 );

  ch_printf( ch, "&GNew course set, approaching %.0f %.0f %.0f.\r\n",
      vec.x, vec.y, vec.z );
  act( AT_PLAIN, "$n manipulates the ships controls.", ch, NULL, argument,
      TO_ROOM );

  echo_to_cockpit( AT_YELLOW, ship, "The ship begins to turn.\r\n" );
  sprintf( buf, "%s turns altering its present course.", ship->name );
  echo_to_system( AT_ORANGE, ship, buf, NULL );

  if( ship->manuever > 100 )
    ship->shipstate = SHIP_BUSY_3;
  else if( ship->manuever > 50 )
    ship->shipstate = SHIP_BUSY_2;
  else
    ship->shipstate = SHIP_BUSY;

  learn_from_success( ch, gsn_spacecraft );
}

void do_buyship( CHAR_DATA * ch, char *argument )
{
  long price = 0;
  SHIP_DATA *ship = NULL;
  SHIP_PROTOTYPE *prototype = NULL;
  char shipname[MAX_STRING_LENGTH];
  char ships_buf[MAX_STRING_LENGTH];
  char vehicles_buf[MAX_STRING_LENGTH];

  snprintf( ships_buf, MAX_STRING_LENGTH, "%s", "ships" );
  snprintf( vehicles_buf, MAX_STRING_LENGTH, "%s", "vehicles" );

  if( IS_NPC( ch ) || !ch->pcdata )
  {
    send_to_char( "&ROnly players can do that!\r\n", ch );
    return;
  }

  if( !ch->in_room ||
      ( !IS_SET( ch->in_room->room_flags, ROOM_SHIPYARD )
	&& !IS_SET( ch->in_room->room_flags, ROOM_GARAGE ) ) )
  {
    send_to_char
      ( "&RYou must be in a shipyard or garage to purchase transportation.\r\n",
	ch );
    return;
  }

  prototype = get_ship_prototype( argument );

  if( prototype == NULL )
  {
    send_to_char( "&RThat model does not exist.\r\n", ch );

    if( IS_SET( ch->in_room->room_flags, ROOM_SHIPYARD ) )
      do_prototypes( ch, ships_buf );
    else
      do_prototypes( ch, vehicles_buf );

    return;
  }

  if( IS_SET( ch->in_room->room_flags, ROOM_SHIPYARD )
      && prototype->ship_class > SPACE_STATION )
  {
    send_to_char( "&RThats not a ship prototype.\r\n", ch );
    do_prototypes( ch, ships_buf );
    return;
  }

  if( IS_SET( ch->in_room->room_flags, ROOM_GARAGE )
      && prototype->ship_class <= SPACE_STATION )
  {
    send_to_char( "&RThats not a vehicle prototype.\r\n", ch );
    do_prototypes( ch, vehicles_buf );
    return;
  }

  price = get_prototype_value( prototype );

  if( ch->gold < price )
  {
    ch_printf( ch,
	"&RThat type of ship costs %ld. You don't have enough credits!\r\n",
	price );
    return;
  }

  ch->gold -= price;
  ch_printf( ch, "&GYou pay %ld credits to purchace the ship.\r\n", price );

  act( AT_PLAIN,
      "$n walks over to a terminal and makes a credit transaction.", ch,
      NULL, argument, TO_ROOM );
  ship = make_ship( prototype );
  ship_to_room( ship, ch->in_room->vnum );
  ship->location = ch->in_room->vnum;
  ship->lastdoc = ch->in_room->vnum;
  sprintf( shipname, "%ss %s %s", ch->name, prototype->name, ship->filename );

  STRFREE( ship->owner );
  ship->owner = STRALLOC( ch->name );
  STRFREE( ship->name );
  ship->name = STRALLOC( shipname );
  save_ship( ship );
}

void do_clanbuyship( CHAR_DATA * ch, char *argument )
{
  long price;
  SHIP_DATA *ship;
  CLAN_DATA *clan;
  SHIP_PROTOTYPE *prototype;
  char shipname[MAX_STRING_LENGTH];
  char ships_buf[MAX_STRING_LENGTH];
  char vehicles_buf[MAX_STRING_LENGTH];

  snprintf( ships_buf, MAX_STRING_LENGTH, "%s", "ships" );
  snprintf( vehicles_buf, MAX_STRING_LENGTH, "%s", "vehicles" );

  if( IS_NPC( ch ) || !ch->pcdata )
  {
    send_to_char( "&ROnly players can do that!\r\n", ch );
    return;
  }
  if( !ch->pcdata->clan )
  {
    send_to_char( "&RYou aren't a member of any organizations!\r\n", ch );
    return;
  }

  clan = ch->pcdata->clan;

  if( !clan_char_has_permission( clan, ch, CLAN_PERM_BUYSHIP ) )
  {
    send_to_char
      ( "&RYour organization hasn't seen fit to bestow you with that ability.\r\n",
	ch );
    return;
  }

  if( !ch->in_room ||
      ( !IS_SET( ch->in_room->room_flags, ROOM_SHIPYARD )
	&& !IS_SET( ch->in_room->room_flags, ROOM_GARAGE ) ) )
  {
    send_to_char
      ( "&RYou must be in a shipyard or garage to purchase transportation.\r\n",
	ch );
    return;
  }


  prototype = get_ship_prototype( argument );
  if( prototype == NULL )
  {
    send_to_char( "&RThat model does not exist.\r\n", ch );
    if( IS_SET( ch->in_room->room_flags, ROOM_SHIPYARD ) )
      do_prototypes( ch, ships_buf );
    else
      do_prototypes( ch, vehicles_buf );
    return;
  }

  if( IS_SET( ch->in_room->room_flags, ROOM_SHIPYARD )
      && prototype->ship_class > SPACE_STATION )
  {
    send_to_char( "&RThats not a ship prototype.\r\n", ch );
    do_prototypes( ch, ships_buf );
    return;
  }
  if( IS_SET( ch->in_room->room_flags, ROOM_GARAGE )
      && prototype->ship_class <= SPACE_STATION )
  {
    send_to_char( "&RThats not a vehicle prototype.\r\n", ch );
    do_prototypes( ch, vehicles_buf );
    return;
  }

  price = get_prototype_value( prototype );

  if( clan->funds < price )
  {
    ch_printf( ch,
	"&RThat type of ship costs %ld. Your organization can't afford it!\r\n",
	price );
    return;
  }

  clan->funds -= price;
  ch_printf( ch,
      "&GYou pay %ld credits from clan funds to purchace the ship.\r\n",
      price );

  act( AT_PLAIN,
      "$n walks over to a terminal and makes a credit transaction.", ch,
      NULL, argument, TO_ROOM );

  ship = make_ship( prototype );
  ship_to_room( ship, ch->in_room->vnum );
  ship->location = ch->in_room->vnum;
  ship->lastdoc = ch->in_room->vnum;

  sprintf( shipname, "%s %s %s", clan->name, prototype->name,
      ship->filename );

  STRFREE( ship->owner );
  ship->owner = STRALLOC( clan->name );
  STRFREE( ship->name );
  ship->name = STRALLOC( shipname );
  save_ship( ship );

  if( ship->ship_class <= SPACE_STATION )
    clan->spacecraft++;
  else
    clan->vehicles++;
}

void do_sellship( CHAR_DATA * ch, char *argument )
{
  long price;
  SHIP_DATA *ship;

  return;

  ship = ship_in_room( ch->in_room, argument );
  if( !ship )
  {
    act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
    return;
  }

  if( str_cmp( ship->owner, ch->name ) )
  {
    send_to_char( "&RThat isn't your ship!", ch );
    return;
  }

  price = 100;

  ch->gold += ( price - price / 10 );
  ch_printf( ch, "&GYou receive %ld credits from selling your ship.\r\n",
      price - price / 10 );

  act( AT_PLAIN,
      "$n walks over to a terminal and makes a credit transaction.", ch,
      NULL, argument, TO_ROOM );

  STRFREE( ship->owner );
  ship->owner = STRALLOC( "" );
  save_ship( ship );

}

void do_info( CHAR_DATA * ch, char *argument )
{
  SHIP_DATA *ship;
  SHIP_DATA *target;

  if( ( ship = ship_from_cockpit( ch->in_room ) ) == NULL )
  {
    if( argument[0] == '\0' )
    {
      act( AT_PLAIN, "Which ship do you want info on?.", ch, NULL, NULL,
	  TO_CHAR );
      return;
    }

    ship = ship_in_room( ch->in_room, argument );
    if( !ship )
    {
      act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
      return;
    }

    target = ship;
  }
  else if( argument[0] == '\0' )
    target = ship;
  else
    target = get_ship_here( argument, ship->starsystem );

  if( target == NULL )
  {
    send_to_char
      ( "&RI don't see that here.\r\nTry the radar, or type info by itself for info on this ship.\r\n",
	ch );
    return;
  }

  ch_printf( ch, "%s%s : %s\r\n",
      target->type == PLAYER_SHIP ? "" : "MOB ",
      target->ship_class == SPACECRAFT ? model[ship->model].name :
      ( target->ship_class == SPACE_STATION ? "Space Station" :
	( target->ship_class == AIRCRAFT ? "Aircraft" :
	  ( target->ship_class == BOAT ? "Boat" :
	    ( target->ship_class == SUBMARINE ? "Submatine" :
	      ( target->ship_class ==
		LAND_VEHICLE ? "land vehicle" : "Unknown" ) ) ) ) ),
      target->name );
  ch_printf( ch, "Owner: %s   Pilot: %s   Copilot: %s\r\n", target->owner,
      target->pilot, target->copilot );
  ch_printf( ch, "Laser cannons: %d  ", target->lasers );
  ch_printf( ch, "Maximum Missiles: %d  ", target->maxmissiles );
  ch_printf( ch, "Max Chaff: %d\r\n", target->maxchaff );
  ch_printf( ch, "Max Hull: %d  ", target->maxhull );
  ch_printf( ch, "Max Shields: %d   Max Energy(fuel): %d\r\n",
      target->maxshield, target->maxenergy );
  ch_printf( ch, "Maximum Speed: %d   Hyperspeed: %d   Manuever: %d\r\n",
      target->realspeed, target->hyperspeed, target->manuever );

  act( AT_PLAIN, "$n checks various gages and displays on the control panel.",
      ch, NULL, argument, TO_ROOM );

}

void do_autorecharge( CHAR_DATA * ch, char *argument )
{
  int chance;
  SHIP_DATA *ship;
  int recharge;


  if( ( ship = ship_from_gunseat( ch->in_room ) ) == NULL )
  {
    send_to_char
      ( "&RYou must be at the shield controls of a ship to do that!\r\n",
	ch );
    return;
  }

  if( autofly( ship ) )
  {
    send_to_char
      ( "&RYou'll have to turn off the ships autopilot first.\r\n", ch );
    return;
  }

  chance = character_skill_level( ch, gsn_spacecraft );

  if( number_percent() > chance )
  {
    send_to_char( "&RYou fail to work the controls properly.\r\n", ch );
    return;
  }

  act( AT_PLAIN, "$n flips a switch on the control panel.", ch,
      NULL, argument, TO_ROOM );

  if( !str_cmp( argument, "on" ) )
  {
    ship->autorecharge = TRUE;
    send_to_char( "&GYou power up the shields.\r\n", ch );
    echo_to_cockpit( AT_YELLOW, ship, "Shields ON. Autorecharge ON." );
  }
  else if( !str_cmp( argument, "off" ) )
  {
    ship->autorecharge = FALSE;
    send_to_char( "&GYou shutdown the shields.\r\n", ch );
    echo_to_cockpit( AT_YELLOW, ship,
	"Shields OFF. Shield strength set to 0. Autorecharge OFF." );
    ship->shield = 0;
  }
  else if( !str_cmp( argument, "idle" ) )
  {
    ship->autorecharge = FALSE;
    send_to_char( "&GYou let the shields idle.\r\n", ch );
    echo_to_cockpit( AT_YELLOW, ship,
	"Autorecharge OFF. Shields IDLEING." );
  }
  else
  {
    if( ship->autorecharge == TRUE )
    {
      ship->autorecharge = FALSE;
      send_to_char( "&GYou toggle the shields.\r\n", ch );
      echo_to_cockpit( AT_YELLOW, ship,
	  "Autorecharge OFF. Shields IDLEING." );
    }
    else
    {
      ship->autorecharge = TRUE;
      send_to_char( "&GYou toggle the shields.\r\n", ch );
      echo_to_cockpit( AT_YELLOW, ship, "Shields ON. Autorecharge ON" );
    }
  }

  if( ship->autorecharge )
  {
    recharge =
      URANGE( 0, ship->maxshield - ship->shield, 20 + ship->model * 10 );
    recharge = UMIN( recharge, ship->energy * 5 + 100 );
    ship->shield += recharge;
    ship->energy -= ( recharge * 5 );
  }

  learn_from_success( ch, gsn_spacecraft );

}

void do_autopilot( CHAR_DATA * ch, char *argument )
{
  SHIP_DATA *ship;

  if( ( ship = ship_from_pilotseat( ch->in_room ) ) == NULL )
  {
    send_to_char
      ( "&RYou must be at the pilot controls of a ship to do that!\r\n",
	ch );
    return;
  }

  if( !check_pilot( ch, ship ) )
  {
    send_to_char( "&RHey! Thats not your ship!\r\n", ch );
    return;
  }

  if( ship->target )
  {
    send_to_char( "&RNot while the ship is enganged with an enemy!\r\n",
	ch );
    return;
  }


  act( AT_PLAIN, "$n flips a switch on the control panel.", ch,
      NULL, argument, TO_ROOM );

  if( ship->autopilot == TRUE )
  {
    ship->autopilot = FALSE;
    send_to_char( "&GYou toggle the autopilot.\r\n", ch );
    echo_to_cockpit( AT_YELLOW, ship, "Autopilot OFF." );
  }
  else
  {
    ship->autopilot = TRUE;
    ship->autorecharge = TRUE;
    send_to_char( "&GYou toggle the autopilot.\r\n", ch );
    echo_to_cockpit( AT_YELLOW, ship, "Autopilot ON." );
  }

}

void do_openhatch( CHAR_DATA * ch, char *argument )
{
  SHIP_DATA *ship;
  char buf[MAX_STRING_LENGTH];

  if( !argument || argument[0] == '\0' || !str_cmp( argument, "hatch" ) )
  {
    ship = ship_from_entrance( ch->in_room );
    if( ship == NULL )
    {
      send_to_char( "&ROpen what?\r\n", ch );
      return;
    }
    else
    {
      if( !ship->hatchopen )
      {

	if( ship->ship_class == SPACE_STATION )
	{
	  send_to_char( "&RTry one of the docking bays!\r\n", ch );
	  return;
	}
	if( ship->location != ship->lastdoc ||
	    ( ship->shipstate != SHIP_DOCKED
	      && ship->shipstate != SHIP_DISABLED ) )
	{
	  send_to_char( "&RPlease wait till the ship lands!\r\n",
	      ch );
	  return;
	}
	ship->hatchopen = TRUE;
	send_to_char( "&GYou open the hatch.\r\n", ch );
	act( AT_PLAIN, "$n opens the hatch.", ch, NULL, argument,
	    TO_ROOM );
	sprintf( buf, "The hatch on %s opens.", ship->name );
	echo_to_room( AT_YELLOW, get_room_index( ship->location ),
	    buf );
	return;
      }
      else
      {
	send_to_char( "&RIt's already open.\r\n", ch );
	return;
      }
    }
  }

  ship = ship_in_room( ch->in_room, argument );
  if( !ship )
  {
    act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
    return;
  }

  if( ship->shipstate != SHIP_DOCKED && ship->shipstate != SHIP_DISABLED )
  {
    send_to_char( "&RThat ship has already started to launch", ch );
    return;
  }

  if( !check_pilot( ch, ship ) )
  {
    send_to_char( "&RHey! Thats not your ship!\r\n", ch );
    return;
  }

  if( !ship->hatchopen )
  {
    ship->hatchopen = TRUE;
    act( AT_PLAIN, "You open the hatch on $T.", ch, NULL, ship->name,
	TO_CHAR );
    act( AT_PLAIN, "$n opens the hatch on $T.", ch, NULL, ship->name,
	TO_ROOM );
    echo_to_room( AT_YELLOW, ship->entrance,
	"The hatch opens from the outside." );
    return;
  }

  send_to_char( "&GIts already open!\r\n", ch );

}


void do_closehatch( CHAR_DATA * ch, char *argument )
{
  SHIP_DATA *ship;
  char buf[MAX_STRING_LENGTH];

  if( !argument || argument[0] == '\0' || !str_cmp( argument, "hatch" ) )
  {
    ship = ship_from_entrance( ch->in_room );
    if( ship == NULL )
    {
      send_to_char( "&RClose what?\r\n", ch );
      return;
    }
    else
    {

      if( ship->ship_class == SPACE_STATION )
      {
	send_to_char( "&RTry one of the docking bays!\r\n", ch );
	return;
      }
      if( ship->hatchopen )
      {
	ship->hatchopen = FALSE;
	send_to_char( "&GYou close the hatch.\r\n", ch );
	act( AT_PLAIN, "$n closes the hatch.", ch, NULL, argument,
	    TO_ROOM );
	sprintf( buf, "The hatch on %s closes.", ship->name );
	echo_to_room( AT_YELLOW, get_room_index( ship->location ),
	    buf );
	return;
      }
      else
      {
	send_to_char( "&RIt's already closed.\r\n", ch );
	return;
      }
    }
  }

  ship = ship_in_room( ch->in_room, argument );
  if( !ship )
  {
    act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
    return;
  }

  if( ship->shipstate != SHIP_DOCKED && ship->shipstate != SHIP_DISABLED )
  {
    send_to_char( "&RThat ship has already started to launch", ch );
    return;
  }
  else
  {
    if( ship->hatchopen )
    {
      ship->hatchopen = FALSE;
      act( AT_PLAIN, "You close the hatch on $T.", ch, NULL, ship->name,
	  TO_CHAR );
      act( AT_PLAIN, "$n closes the hatch on $T.", ch, NULL, ship->name,
	  TO_ROOM );
      echo_to_room( AT_YELLOW, ship->entrance,
	  "The hatch is closed from outside." );

      return;
    }
    else
    {
      send_to_char( "&RIts already closed.\r\n", ch );
      return;
    }
  }


}

void do_status( CHAR_DATA * ch, char *argument )
{
  int chance;
  SHIP_DATA *ship;
  SHIP_DATA *target;

  if( ( ship = ship_from_cockpit( ch->in_room ) ) == NULL )
  {
    send_to_char
      ( "&RYou must be in the cockpit, turret or engineroom of a ship to do that!\r\n",
	ch );
    return;
  }

  if( argument[0] == '\0' )
    target = ship;
  else
    target = get_ship_here( argument, ship->starsystem );

  if( target == NULL )
  {
    send_to_char
      ( "&RI don't see that here.\r\nTry the radar, or type status by itself for your ships status.\r\n",
	ch );
    return;
  }

  chance = character_skill_level( ch, gsn_spacecraft );

  if( number_percent() > chance )
  {
    send_to_char( "&RYou cant figure out what the readout means.\r\n", ch );
    return;
  }

  act( AT_PLAIN, "$n checks various gages and displays on the control panel.",
      ch, NULL, argument, TO_ROOM );

  ch_printf( ch, "&W%s:\r\n", target->name );
  ch_printf( ch, "&OCurrent Coordinates:&Y %.0f %.0f %.0f\r\n",
      target->pos.x, target->pos.y, target->pos.z );
  ch_printf( ch, "&OCurrent Heading:&Y %.2f %.2f %.2f\r\n",
      target->head.x, target->head.y, target->head.z );
  ch_printf( ch, "&OCurrent Speed:&Y %d&O/%d\r\n",
      target->currspeed, target->realspeed );
  ch_printf( ch, "&OHull:&Y %d&O/%d  Ship Condition:&Y %s\r\n",
      target->hull,
      target->maxhull,
      target->shipstate == SHIP_DISABLED ? "Disabled" : "Running" );
  ch_printf( ch, "&OShields:&Y %d&O/%d   Energy(fuel):&Y %d&O/%d\r\n",
      target->shield,
      target->maxshield, target->energy, target->maxenergy );
  ch_printf( ch, "&OLaser Condition:&Y %s  &OCurrent Target:&Y %s\r\n",
      target->laserstate == LASER_DAMAGED ? "Damaged" : "Good",
      target->target ? target->target->name : "none" );
  ch_printf( ch, "\r\n&OMissiles:&Y %d&O/%d  Condition:&Y %s&w\r\n",
      ship->missiles, ship->maxmissiles,
      ship->missilestate == MISSILE_DAMAGED ? "Damaged" : "Good" );

  learn_from_success( ch, gsn_spacecraft );


}

void do_hyperspace( CHAR_DATA * ch, char *argument )
{
  int chance = 0;
  SHIP_DATA *ship = 0;
  SHIP_DATA *eShip = 0;

  if( ( ship = ship_from_pilotseat( ch->in_room ) ) == NULL )
  {
    send_to_char( "&RYou must be in control of a ship to do that!\r\n",
	ch );
    return;
  }

  if( ship->ship_class > SPACE_STATION )
  {
    send_to_char( "&RThis isn't a spacecraft!\r\n", ch );
    return;
  }

  if( autofly( ship ) )
  {
    send_to_char
      ( "&RYou'll have to turn off the ships autopilot first.\r\n", ch );
    return;
  }

  if( ship->ship_class == SPACE_STATION )
  {
    send_to_char( "&RPlatforms can't move!\r\n", ch );
    return;
  }

  if( ship->hyperspeed == 0 )
  {
    send_to_char( "&RThis ship is not equipped with a hyperdrive!\r\n",
	ch );
    return;
  }

  if( ship->shipstate == SHIP_HYPERSPACE )
  {
    send_to_char( "&RYou are already travelling lightspeed!\r\n", ch );
    return;
  }

  if( ship->shipstate == SHIP_DISABLED )
  {
    send_to_char( "&RThe ships drive is disabled. Unable to manuever.\r\n",
	ch );
    return;
  }

  if( ship->shipstate == SHIP_DOCKED )
  {
    send_to_char( "&RYou can't do that until after you've launched!\r\n",
	ch );
    return;
  }

  if( ship->shipstate != SHIP_READY )
  {
    send_to_char
      ( "&RPlease wait until the ship has finished its current maneuver.\r\n",
	ch );
    return;
  }

  if( !ship->currjump )
  {
    send_to_char( "&RYou need to calculate your jump first!\r\n", ch );
    return;
  }

  if( ship->energy < ( 200 + ship->hyperdistance ) )
  {
    send_to_char( "&RTheres not enough fuel!\r\n", ch );
    return;
  }

  if( ship->currspeed <= 0 )
  {
    send_to_char( "&RYou need to speed up a little first!\r\n", ch );
    return;
  }

  if( ( eShip = find_ship_in_range( ship, 500 ) ) )
  {
    ch_printf( ch,
	"&RYou are too close to %s to make the jump to lightspeed.\r\n",
	eShip->name );
    return;
  }

  chance = character_skill_level( ch, gsn_spacecraft );

  if( number_percent() > chance )
  {
    send_to_char( "&RYou can't figure out which lever to use.\r\n", ch );
    return;
  }

  send_to_char( "&GYou push forward the hyperspeed lever.\r\n", ch );
  act( AT_PLAIN, "$n pushes a lever forward on the control panel.", ch,
      NULL, argument, TO_ROOM );

  ship_jump_to_lightspeed( ship );

  learn_from_success( ch, gsn_spacecraft );
}


void do_target( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  int chance;
  SHIP_DATA *ship;
  SHIP_DATA *target;
  char buf[MAX_STRING_LENGTH];
  TURRET_DATA *turret = NULL;

  strcpy( arg, argument );

  switch ( ch->substate )
  {
    default:
      if( ( ship = ship_from_turret( ch->in_room ) ) == NULL )
      {
	send_to_char
	  ( "&RYou must be in the gunners seat or turret of a ship to do that!\r\n",
	    ch );
	return;
      }

      if( ship->ship_class > SPACE_STATION )
      {
	send_to_char( "&RThis isn't a spacecraft!\r\n", ch );
	return;
      }

      if( ship->shipstate == SHIP_HYPERSPACE )
      {
	send_to_char( "&RYou can only do that in realspace!\r\n", ch );
	return;
      }

      if( !ship->starsystem )
      {
	send_to_char
	  ( "&RYou can't do that until you've finished launching!\r\n",
	    ch );
	return;
      }

      if( autofly( ship ) )
      {
	send_to_char
	  ( "&RYou'll have to turn off the ships autopilot first....\r\n",
	    ch );
	return;
      }

      if( arg[0] == '\0' )
      {
	send_to_char( "&RYou need to specify a target!\r\n", ch );
	return;
      }

      if( !str_cmp( arg, "none" ) )
      {
	send_to_char( "&GTarget set to none.\r\n", ch );
	ship->target = NULL;
	return;
      }

      target = get_ship_here( arg, ship->starsystem );

      if( target == NULL )
      {
	send_to_char( "&RThat ship isn't here!\r\n", ch );
	return;
      }

      if( target == ship )
      {
	send_to_char( "&RYou can't target your own ship!\r\n", ch );
	return;
      }

      if( !str_cmp( target->owner, ship->owner )
	  && str_cmp( target->owner, "" ) )
      {
	send_to_char
	  ( "&RThat ship has the same owner... try targetting an enemy ship instead!\r\n",
	    ch );
	return;
      }

      if( ship_distance_to_ship( ship, target ) > 5000 )
      {
	send_to_char( "&RThat ship is too far away to target.\r\n", ch );
	return;
      }

      chance = character_skill_level( ch, gsn_weaponsystems );

      if( number_percent() < chance )
      {
	send_to_char( "&GTracking target.\r\n", ch );
	act( AT_PLAIN,
	    "$n makes some adjustments on the targeting computer.", ch,
	    NULL, argument, TO_ROOM );
	add_timer( ch, TIMER_DO_FUN, 1, do_target, 1 );
	ch->dest_buf = str_dup( arg );
	return;
      }

      send_to_char( "&RYou fail to work the controls properly.\r\n", ch );
      learn_from_failure( ch, gsn_weaponsystems );
      return;

    case 1:
      if( !ch->dest_buf )
	return;
      strcpy( arg, ( const char * ) ch->dest_buf );
      DISPOSE( ch->dest_buf );
      break;

    case SUB_TIMER_DO_ABORT:
      DISPOSE( ch->dest_buf );
      ch->substate = SUB_NONE;
      send_to_char
	( "&RYour concentration is broken. You fail to lock onto your target.\r\n",
	  ch );
      return;
  }

  ch->substate = SUB_NONE;

  if( ( ship = ship_from_turret( ch->in_room ) ) == NULL )
  {
    return;
  }

  target = get_ship_here( arg, ship->starsystem );

  if( target == NULL || target == ship )
  {
    send_to_char
      ( "&RThe ship has left the starsytem. Targeting aborted.\r\n", ch );
    return;
  }

  if( ch->in_room == ship->gunseat )
  {
    ship->target = target;
  }
  else
  {
    for( turret = ship->first_turret; turret; turret = turret->next )
      if( turret->room == ch->in_room )
	break;

    if( !turret )
      return;

    turret->target = target;
  }

  send_to_char( "&GTarget Locked.\r\n", ch );
  sprintf( buf, "You are being targetted by %s.", ship->name );
  echo_to_cockpit( AT_BLOOD, target, buf );

  learn_from_success( ch, gsn_weaponsystems );

  if( autofly( target ) && !target->target )
  {
    ship_acquire_target( target, ship );
  }
}

void do_fire( CHAR_DATA * ch, char *argument )
{
  int chance = 0;
  SHIP_DATA *ship = NULL;
  SHIP_DATA *target = NULL;
  TURRET_DATA *turret = NULL;
  int skill_level = 0;

  if( ( ship = ship_from_turret( ch->in_room ) ) == NULL )
  {
    send_to_char
      ( "&RYou must be in the gunners chair or turret of a ship to do that!\r\n",
	ch );
    return;
  }

  if( ship->ship_class > SPACE_STATION )
  {
    send_to_char( "&RThis isn't a spacecraft!\r\n", ch );
    return;
  }

  if( ship->shipstate == SHIP_HYPERSPACE )
  {
    send_to_char( "&RYou can only do that in realspace!\r\n", ch );
    return;
  }

  if( ship->starsystem == NULL )
  {
    send_to_char
      ( "&RYou can't do that until after you've finished launching!\r\n",
	ch );
    return;
  }

  if( ship->energy < 5 )
  {
    send_to_char( "&RTheres not enough energy left to fire!\r\n", ch );
    return;
  }

  if( autofly( ship ) )
  {
    send_to_char
      ( "&RYou'll have to turn off the ships autopilot first.\r\n", ch );
    return;
  }

  skill_level = character_skill_level( ch, gsn_weaponsystems )
    + ch->perm_dex * 2 + character_skill_level( ch, gsn_spacecombat ) / 2;

  chance = IS_NPC( ch ) ? 100 : skill_level;

  if( !str_prefix( argument, "lasers" ) )
  {
    if( ch->in_room == ship->gunseat )
    {
      if( !laser_can_be_fired( ch, ship ) )
      {
	return;
      }

      target = ship->target;
    }
    else
    {
      for( turret = ship->first_turret; turret; turret = turret->next )
	if( turret->room == ch->in_room )
	  break;

      if( !turret )
      {
	send_to_char( "&RThis turret is out of order.\r\n", ch );
	return;
      }

      if( !turret_can_be_fired( ch, turret ) )
      {
	return;
      }

      target = turret->target;
    }

    if( target->starsystem != ship->starsystem )
    {
      send_to_char( "&RYour target seems to have left.\r\n", ch );
      ship->target = NULL;
      return;
    }

    if( ship_distance_to_ship( target, ship ) > 1000 )
    {
      send_to_char( "&RThat ship is out of laser range.\r\n", ch );
      return;
    }

    if( ship->ship_class < SPACE_STATION
	&& !ship_is_facing_ship( ship, target ) && !turret )
    {
      send_to_char
	( "&RThe main laser can only fire forward. You'll need to turn your ship!\r\n",
	  ch );
      return;
    }

    if( ch->in_room == ship->gunseat )
      ship->laserstate++;
    else
      turret->laserstate++;

    chance += laser_hit_modifier( ship, target );

    act( AT_PLAIN, "$n presses the fire button.", ch,
	NULL, argument, TO_ROOM );

    if( !laserfire_hits( ship, target, chance ) )
    {
      return;
    }

    learn_from_success( ch, gsn_spacecombat );

    if( turret )
      damage_ship( target, 15, 25, ch );
    else
      damage_ship( target, 5, 10, ch );

    if( target->starsystem && autofly( target ) && target->target != ship )
    {
      ship_acquire_target( target, ship );
    }

    return;
  }

  if( !str_prefix( argument, "missile" ) )
  {
    if( !missile_launcher_can_be_fired( ch, ship ) )
    {
      return;
    }

    target = ship->target;

    if( ship->target->starsystem != ship->starsystem )
    {
      send_to_char( "&RYour target seems to have left.\r\n", ch );
      ship->target = NULL;
      return;
    }

    if( ship_distance_to_ship( ship, target ) > 1000 )
    {
      send_to_char( "&RThat ship is out of missile range.\r\n", ch );
      return;
    }

    if( ship->ship_class < SPACE_STATION
	&& !ship_is_facing_ship( ship, target ) )
    {
      send_to_char
	( "&RMissiles can only fire in a forward. You'll need to turn your ship!\r\n",
	  ch );
      return;
    }

    chance += missile_hit_modifier( ship, target );

    act( AT_PLAIN, "$n presses the fire button.", ch,
	NULL, argument, TO_ROOM );

    if( number_percent() > chance )
    {
      send_to_char( "&RYou fail to lock onto your target!", ch );
      ship->missilestate = MISSILE_RELOAD_2;
      return;
    }

    echo_to_cockpit( AT_YELLOW, ship, "Missiles launched." );
    ship_fire_missile( ship, target );
    learn_from_success( ch, gsn_weaponsystems );

    if( ship->ship_class == SPACE_STATION )
      ship->missilestate = MISSILE_RELOAD;
    else
      ship->missilestate = MISSILE_FIRED;

    if( autofly( target ) && target->target != ship )
    {
      ship_acquire_target( target, ship );
    }

    return;
  }

  send_to_char( "&RYou can't fire that!\r\n", ch );
}


void do_calculate( CHAR_DATA * ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  int chance = 0, count = 0;
  SHIP_DATA *ship = NULL;
  PLANET_DATA *planet = NULL;

  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );
  argument = one_argument( argument, arg3 );

  if( ( ship = ship_from_pilotseat( ch->in_room ) ) == NULL )
  {
    send_to_char( "&RYou must be in control of a ship to do that!\r\n",
	ch );
    return;
  }

  if( ship->ship_class > SPACE_STATION )
  {
    send_to_char( "&RThis isn't a spacecraft!\r\n", ch );
    return;
  }

  if( autofly( ship ) )
  {
    send_to_char
      ( "&RYou'll have to turn off the ships autopilot first....\r\n", ch );
    return;
  }

  if( ship->ship_class == SPACE_STATION )
  {
    send_to_char( "&RAnd what exactly are you going to calculate...?\r\n",
	ch );
    return;
  }

  if( ship->hyperspeed == 0 )
  {
    send_to_char( "&RThis ship is not equipped with a hyperdrive!\r\n",
	ch );
    return;
  }

  if( ship->shipstate == SHIP_DOCKED )
  {
    send_to_char( "&RYou can't do that until after you've launched!\r\n",
	ch );
    return;
  }

  if( ship->starsystem == NULL )
  {
    send_to_char( "&RYou can only do that in realspace.\r\n", ch );
    return;
  }

  if( argument[0] == '\0' )
  {
    SPACE_DATA *starsystem = NULL;
    send_to_char
      ( "&WFormat: Calculate <starsystem> <entry x> <entry y> <entry z>\r\n&wPossible destinations:\r\n",
	ch );
    for( starsystem = first_starsystem; starsystem;
	starsystem = starsystem->next )
    {
      set_char_color( AT_NOTE, ch );
      ch_printf( ch, "%s ----- ", starsystem->name );
      count++;
    }
    if( !count )
    {
      send_to_char( "No Starsystems found.\r\n", ch );
    }
    return;
  }
  chance = character_skill_level( ch, gsn_spacecraft );

  if( number_percent() > chance )
  {
    send_to_char( "&RYou cant seem to figure the charts out today.\r\n",
	ch );
    return;
  }

  ship->currjump = starsystem_from_name( arg1 );
  vector_set( &ship->jump, atoi( arg2 ), atoi( arg3 ), atoi( argument ) );

  if( ship->currjump == NULL )
  {
    send_to_char
      ( "&RYou can't seem to find that starsytem on your charts.\r\n", ch );
    return;
  }
  else
  {
    SPACE_DATA *starsystem = ship->currjump;

    if( starsystem->star1 && strcmp( starsystem->star1, "" ) &&
	vector_distance( &ship->jump, &starsystem->star1_pos ) < 300 )
    {
      echo_to_cockpit( AT_RED, ship,
	  "WARNING.. Jump coordinates too close to stellar object." );
      echo_to_cockpit( AT_RED, ship, "WARNING.. Hyperjump NOT set." );
      ship->currjump = NULL;
      return;
    }
    else if( starsystem->star2 && strcmp( starsystem->star2, "" ) &&
	vector_distance( &ship->jump, &starsystem->star2_pos ) < 300 )
    {
      echo_to_cockpit( AT_RED, ship,
	  "WARNING.. Jump coordinates too close to stellar object." );
      echo_to_cockpit( AT_RED, ship, "WARNING.. Hyperjump NOT set." );
      ship->currjump = NULL;
      return;
    }

    for( planet = starsystem->first_planet; planet;
	planet = planet->next_in_system )
      if( vector_distance( &ship->jump, &planet->pos ) < 300 )
      {
	echo_to_cockpit( AT_RED, ship,
	    "WARNING.. Jump coordinates too close to stellar object." );
	echo_to_cockpit( AT_RED, ship, "WARNING.. Hyperjump NOT set." );
	ship->currjump = NULL;
	return;
      }

    vector_randomize( &ship->jump, -250, 250 );
  }

  if( ship->starsystem == ship->currjump )
    ship->hyperdistance = number_range( 0, 200 );
  else
    ship->hyperdistance = number_range( 500, 1000 );

  send_to_char
    ( "&GHyperspace course set. Ready for the jump to lightspeed.\r\n", ch );
  act( AT_PLAIN, "$n does some calculations using the ships computer.", ch,
      NULL, argument, TO_ROOM );

  learn_from_success( ch, gsn_spacecraft );
  WAIT_STATE( ch, 2 * PULSE_VIOLENCE );
}

void do_recharge( CHAR_DATA * ch, char *argument )
{
  int recharge;
  int chance;
  SHIP_DATA *ship;


  if( ( ship = ship_from_gunseat( ch->in_room ) ) == NULL )
  {
    send_to_char
      ( "&RYou must be at the shield controls of a ship to do that!\r\n",
	ch );
    return;
  }

  if( autofly( ship ) )
  {
    send_to_char( "&R...\r\n", ch );
    return;
  }

  if( ship->shipstate == SHIP_DISABLED )
  {
    send_to_char( "&RThe ships drive is disabled. Unable to manuever.\r\n",
	ch );
    return;
  }

  if( ship->energy < 100 )
  {
    send_to_char( "&RTheres not enough energy!\r\n", ch );
    return;
  }

  chance = character_skill_level( ch, gsn_spacecraft );

  if( number_percent() > chance )
  {
    send_to_char( "&RYou fail to work the controls properly.\r\n", ch );
    return;
  }

  send_to_char( "&GRecharging shields..\r\n", ch );
  act( AT_PLAIN, "$n pulls back a lever on the control panel.", ch,
      NULL, argument, TO_ROOM );

  learn_from_success( ch, gsn_spacecraft );

  recharge = UMIN( ship->maxshield - ship->shield, ship->energy * 5 + 100 );
  recharge = URANGE( 0, recharge, 20 + ship->model * 10 );
  ship->shield += recharge;
  ship->energy -= ( recharge * 5 );
}


void do_repairship( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  int chance, change;
  SHIP_DATA *ship;

  strcpy( arg, argument );

  switch ( ch->substate )
  {
    default:
      if( ( ship = ship_from_engine( ch->in_room ) ) == NULL )
      {
	send_to_char
	  ( "&RYou must be in the engine room of a ship to do that!\r\n",
	    ch );
	return;
      }

      if( str_cmp( argument, "hull" ) && str_cmp( argument, "drive" ) &&
	  str_cmp( argument, "launcher" ) && str_cmp( argument, "laser" ) )
      {
	send_to_char( "&RYou need to spceify something to repair:\r\n",
	    ch );
	send_to_char( "&rTry: hull, drive, launcher, laser\r\n", ch );
	return;
      }

      chance = character_skill_level( ch, gsn_shipmaintenance );

      if( number_percent() < chance )
      {
	send_to_char( "&GYou begin your repairs\r\n", ch );
	act( AT_PLAIN, "$n begins repairing the ships $T.", ch,
	    NULL, argument, TO_ROOM );
	if( !str_cmp( arg, "hull" ) )
	  add_timer( ch, TIMER_DO_FUN, 15, do_repairship, 1 );
	else
	  add_timer( ch, TIMER_DO_FUN, 5, do_repairship, 1 );
	ch->dest_buf = str_dup( arg );
	return;
      }
      send_to_char( "&RYou fail to locate the source of the problem.\r\n",
	  ch );
      return;

    case 1:
      if( !ch->dest_buf )
	return;
      strcpy( arg, ( const char * ) ch->dest_buf );
      DISPOSE( ch->dest_buf );
      break;

    case SUB_TIMER_DO_ABORT:
      DISPOSE( ch->dest_buf );
      ch->substate = SUB_NONE;
      send_to_char
	( "&RYou are distracted and fail to finish your repairs.\r\n", ch );
      return;
  }

  ch->substate = SUB_NONE;

  if( ( ship = ship_from_engine( ch->in_room ) ) == NULL )
  {
    return;
  }

  if( !str_cmp( arg, "hull" ) )
  {
    change = URANGE( 0,
	number_range( character_skill_level
	  ( ch, gsn_shipmaintenance ) / 2,
	  character_skill_level( ch,
	    gsn_shipmaintenance ) ),
	( ship->maxhull - ship->hull ) );
    ship->hull += change;
    ch_printf( ch,
	"&GRepair complete. Hull strength increased by %d points.\r\n",
	change );
  }

  if( !str_cmp( arg, "drive" ) )
  {
    if( ship->location == ship->lastdoc )
      ship->shipstate = SHIP_DOCKED;
    else
      ship->shipstate = SHIP_READY;
    send_to_char( "&GShips drive repaired.\r\n", ch );
  }

  if( !str_cmp( arg, "launcher" ) )
  {
    ship->missilestate = MISSILE_READY;
    send_to_char( "&GMissile launcher repaired.\r\n", ch );
  }

  if( !str_cmp( arg, "laser" ) )
  {
    ship->laserstate = LASER_READY;
    send_to_char( "&GMain laser repaired.\r\n", ch );
  }

  act( AT_PLAIN, "$n finishes the repairs.", ch, NULL, argument, TO_ROOM );

  learn_from_success( ch, gsn_shipmaintenance );

}


void do_addpilot( CHAR_DATA * ch, char *argument )
{
  SHIP_DATA *ship;

  if( ( ship = ship_from_cockpit( ch->in_room ) ) == NULL )
  {
    send_to_char( "&RYou must be in the cockpit of a ship to do that!\r\n",
	ch );
    return;
  }

  if( ship->ship_class == SPACE_STATION )
  {
    send_to_char( "&RYou can't do that here.\r\n", ch );
    return;
  }

  if( str_cmp( ship->owner, ch->name ) )
  {

    if( !IS_NPC( ch ) && ch->pcdata && ch->pcdata->clan
	&& !str_cmp( ch->pcdata->clan->name, ship->owner ) )
      if( clan_char_is_leader( ch->pcdata->clan, ch ) )
	;
      else
      {
	send_to_char( "&RThat isn't your ship!", ch );
	return;
      }
    else
    {
      send_to_char( "&RThat isn't your ship!", ch );
      return;
    }

  }

  if( argument[0] == '\0' )
  {
    send_to_char( "&RAdd which pilot?\r\n", ch );
    return;
  }

  if( str_cmp( ship->pilot, "" ) )
  {
    if( str_cmp( ship->copilot, "" ) )
    {
      send_to_char( "&RYou are ready have a pilot and copilot..\r\n",
	  ch );
      send_to_char( "&RTry rempilot first.\r\n", ch );
      return;
    }

    STRFREE( ship->copilot );
    ship->copilot = STRALLOC( argument );
    send_to_char( "Copilot Added.\r\n", ch );
    save_ship( ship );
    return;

    return;
  }

  STRFREE( ship->pilot );
  ship->pilot = STRALLOC( argument );
  send_to_char( "Pilot Added.\r\n", ch );
  save_ship( ship );

}

void do_rempilot( CHAR_DATA * ch, char *argument )
{
  SHIP_DATA *ship;

  if( ( ship = ship_from_cockpit( ch->in_room ) ) == NULL )
  {
    send_to_char( "&RYou must be in the cockpit of a ship to do that!\r\n",
	ch );
    return;
  }

  if( ship->ship_class == SPACE_STATION )
  {
    send_to_char( "&RYou can't do that here.\r\n", ch );
    return;
  }

  if( str_cmp( ship->owner, ch->name ) )
  {

    if( !IS_NPC( ch ) && ch->pcdata && ch->pcdata->clan
	&& !str_cmp( ch->pcdata->clan->name, ship->owner ) )
      if( clan_char_is_leader( ch->pcdata->clan, ch ) )
	;
      else
      {
	send_to_char( "&RThat isn't your ship!", ch );
	return;
      }
    else
    {
      send_to_char( "&RThat isn't your ship!", ch );
      return;
    }

  }

  if( argument[0] == '\0' )
  {
    send_to_char( "&RRemove which pilot?\r\n", ch );
    return;
  }

  if( !str_cmp( ship->pilot, argument ) )
  {
    STRFREE( ship->pilot );
    ship->pilot = STRALLOC( "" );
    send_to_char( "Pilot Removed.\r\n", ch );
    save_ship( ship );
    return;
  }

  if( !str_cmp( ship->copilot, argument ) )
  {
    STRFREE( ship->copilot );
    ship->copilot = STRALLOC( "" );
    send_to_char( "Copilot Removed.\r\n", ch );
    save_ship( ship );
    return;
  }

  send_to_char( "&RThat person isn't listed as one of the ships pilots.\r\n",
      ch );

}

void do_radar( CHAR_DATA * ch, char *argument )
{
  SHIP_DATA *target = NULL;
  int chance = 0;
  SHIP_DATA *ship = NULL;
  MISSILE_DATA *missile = NULL;
  PLANET_DATA *planet = NULL;

  if( ( ship = ship_from_cockpit( ch->in_room ) ) == NULL )
  {
    send_to_char
      ( "&RYou must be in the cockpit or turret of a ship to do that!\r\n",
	ch );
    return;
  }

  if( ship->ship_class > SPACE_STATION )
  {
    send_to_char( "&RThis isn't a spacecraft!\r\n", ch );
    return;
  }

  if( ship->shipstate == SHIP_DOCKED )
  {
    send_to_char( "&RWait until after you launch!\r\n", ch );
    return;
  }

  if( ship->shipstate == SHIP_HYPERSPACE )
  {
    send_to_char( "&RYou can only do that in realspace!\r\n", ch );
    return;
  }

  if( ship->starsystem == NULL )
  {
    send_to_char
      ( "&RYou can't do that unless the ship is flying in realspace!\r\n",
	ch );
    return;
  }

  chance = character_skill_level( ch, gsn_spacecraft );

  if( number_percent() > chance )
  {
    send_to_char( "&RYou fail to work the controls properly.\r\n", ch );
    return;
  }

  act( AT_PLAIN, "$n checks the radar.", ch, NULL, argument, TO_ROOM );

  set_char_color( AT_WHITE, ch );
  ch_printf( ch, "%s\r\n\r\n", ship->starsystem->name );

  if( ship->starsystem->star1 && str_cmp( ship->starsystem->star1, "" ) )
    ch_printf( ch, "&Y%s   %.0f %.0f %.0f [Distance %.0f]\r\n",
	ship->starsystem->star1,
	ship->starsystem->star1_pos.x,
	ship->starsystem->star1_pos.y,
	ship->starsystem->star1_pos.z,
	vector_distance( &ship->pos, &ship->starsystem->star1_pos ) );

  if( ship->starsystem->star2 && str_cmp( ship->starsystem->star2, "" ) )
    ch_printf( ch, "&Y%s   %.0f %.0f %.0f [Distance %.0f]\r\n",
	ship->starsystem->star2,
	ship->starsystem->star2_pos.x,
	ship->starsystem->star2_pos.y,
	ship->starsystem->star2_pos.z,
	vector_distance( &ship->pos, &ship->starsystem->star2_pos ) );

  for( planet = ship->starsystem->first_planet; planet;
      planet = planet->next_in_system )
    ch_printf( ch, "&G%s   %.0f %.0f %.0f [Distance %.0f]\r\n",
	planet->name,
	planet->pos.x,
	planet->pos.y,
	planet->pos.z, ship_distance_to_planet( ship, planet ) );
  ch_printf( ch, "\r\n" );

  for( target = ship->starsystem->first_ship;
      target; target = target->next_in_starsystem )
  {
    if( target != ship )
      ch_printf( ch, "&C%s    %.0f %.0f %.0f [Distance %.0f]\r\n",
	  target->name,
	  target->pos.x,
	  target->pos.y,
	  target->pos.z, ship_distance_to_ship( ship, target ) );
  }

  ch_printf( ch, "\r\n" );

  for( missile = ship->starsystem->first_missile; missile;
      missile = missile->next_in_starsystem )
  {
    ch_printf( ch, "&RA Missile    %.0f %.0f %.0f [Distance %.0f]\r\n",
	missile->pos.x,
	missile->pos.y,
	missile->pos.z, missile_distance_to_ship( missile, ship ) );
  }

  ch_printf( ch, "\r\n&WYour Coordinates: %.0f %.0f %.0f\r\n",
      ship->pos.x, ship->pos.y, ship->pos.z );

  learn_from_success( ch, gsn_spacecraft );
}

void do_autotrack( CHAR_DATA * ch, char *argument )
{
  SHIP_DATA *ship;
  int chance;

  if( ( ship = ship_from_pilotseat( ch->in_room ) ) == NULL )
  {
    send_to_char( "&RYou must be at the controls of a ship to do that!\r\n",
	ch );
    return;
  }

  if( ship->ship_class > SPACE_STATION )
  {
    send_to_char( "&RThis isn't a spacecraft!\r\n", ch );
    return;
  }


  if( ship->ship_class == SPACE_STATION )
  {
    send_to_char( "&RSpace Stations don't have autotracking systems!\r\n",
	ch );
    return;
  }

  if( autofly( ship ) )
  {
    send_to_char
      ( "&RYou'll have to turn off the ships autopilot first....\r\n", ch );
    return;
  }

  chance = character_skill_level( ch, gsn_spacecraft );
  if( number_percent() > chance )
  {
    send_to_char( "&RYour notsure which switch to flip.\r\n", ch );
    return;
  }

  act( AT_PLAIN, "$n flips a switch on the control panel.", ch,
      NULL, argument, TO_ROOM );
  if( ship->autotrack )
  {
    ship->autotrack = FALSE;
    echo_to_cockpit( AT_YELLOW, ship, "Autotracking off." );
  }
  else
  {
    ship->autotrack = TRUE;
    echo_to_cockpit( AT_YELLOW, ship, "Autotracking on." );
  }

  learn_from_success( ch, gsn_spacecraft );

}

void do_closebay( CHAR_DATA * ch, char *argument )
{
}
void do_openbay( CHAR_DATA * ch, char *argument )
{
}

void do_tractorbeam( CHAR_DATA * ch, char *argument )
{
}

void do_fly( CHAR_DATA * ch, char *argument )
{
}

void do_drive( CHAR_DATA * ch, char *argument )
{
  int dir;
  SHIP_DATA *ship;

  if( ( ship = ship_from_pilotseat( ch->in_room ) ) == NULL )
  {
    send_to_char
      ( "&RYou must be in the drivers seat of a land vehicle to do that!\r\n",
	ch );
    return;
  }

  if( ship->ship_class != LAND_VEHICLE )
  {
    send_to_char( "&RThis isn't a land vehicle!\r\n", ch );
    return;
  }


  if( ship->shipstate == SHIP_DISABLED )
  {
    send_to_char( "&RThe drive is disabled.\r\n", ch );
    return;
  }

  if( ship->energy < 1 )
  {
    send_to_char( "&RTheres not enough fuel!\r\n", ch );
    return;
  }

  if( ( dir = get_door( argument ) ) == -1 )
  {
    send_to_char( "Usage: drive <direction>\r\n", ch );
    return;
  }

  drive_ship( ch, ship, get_exit( get_room_index( ship->location ), dir ),
      0 );

}

ch_ret drive_ship( CHAR_DATA * ch, SHIP_DATA * ship, EXIT_DATA * pexit,
    int fall )
{
  ROOM_INDEX_DATA *in_room = NULL;
  ROOM_INDEX_DATA *to_room = NULL;
  ROOM_INDEX_DATA *from_room = NULL;
  ROOM_INDEX_DATA *original = NULL;
  char buf[MAX_STRING_LENGTH];
  const char *txt = NULL;
  const char *dtxt = NULL;
  ch_ret retcode = rNONE;
  short door = 0, distance = 0;
  bool drunk = FALSE;
  CHAR_DATA *rch = NULL;
  CHAR_DATA *next_rch = NULL;

  if( !IS_NPC( ch ) )
    if( IS_DRUNK( ch, 2 ) && ( ch->position != POS_SHOVE )
	&& ( ch->position != POS_DRAG ) )
      drunk = TRUE;

  if( drunk && !fall )
  {
    door = number_door();
    pexit = get_exit( get_room_index( ship->location ), door );
  }

#ifdef DEBUG
  if( pexit )
  {
    sprintf( buf, "drive_ship: %s to door %d", ch->name, pexit->vdir );
    log_string( buf );
  }
#endif

  in_room = get_room_index( ship->location );
  from_room = in_room;

  if( !pexit || ( to_room = pexit->to_room ) == NULL )
  {
    if( drunk )
      send_to_char( "You drive into a wall in your drunken state.\r\n",
	  ch );
    else
      send_to_char( "Alas, you cannot go that way.\r\n", ch );
    return rNONE;
  }

  door = pexit->vdir;
  distance = pexit->distance;

  if( IS_SET( pexit->exit_info, EX_WINDOW )
      && !IS_SET( pexit->exit_info, EX_ISDOOR ) )
  {
    send_to_char( "Alas, you cannot go that way.\r\n", ch );
    return rNONE;
  }

  if( IS_SET( pexit->exit_info, EX_PORTAL ) && IS_NPC( ch ) )
  {
    act( AT_PLAIN, "Mobs can't use portals.", ch, NULL, NULL, TO_CHAR );
    return rNONE;
  }

  if( IS_SET( pexit->exit_info, EX_NOMOB ) && IS_NPC( ch ) )
  {
    act( AT_PLAIN, "Mobs can't enter there.", ch, NULL, NULL, TO_CHAR );
    return rNONE;
  }

  if( IS_SET( pexit->exit_info, EX_CLOSED )
      && ( IS_SET( pexit->exit_info, EX_NOPASSDOOR ) ) )
  {
    if( !IS_SET( pexit->exit_info, EX_SECRET )
	&& !IS_SET( pexit->exit_info, EX_DIG ) )
    {
      if( drunk )
      {
	act( AT_PLAIN, "$n drives into the $d in $s drunken state.",
	    ch, NULL, pexit->keyword, TO_ROOM );
	act( AT_PLAIN, "You drive into the $d in your drunken state.",
	    ch, NULL, pexit->keyword, TO_CHAR );
      }
      else
	act( AT_PLAIN, "The $d is closed.", ch, NULL,
	    pexit->keyword, TO_CHAR );
    }
    else
    {
      if( drunk )
	send_to_char( "You hit a wall in your drunken state.\r\n", ch );
      else
	send_to_char( "Alas, you cannot go that way.\r\n", ch );
    }

    return rNONE;
  }

  if( room_is_private( ch, to_room ) )
  {
    send_to_char( "That room is private right now.\r\n", ch );
    return rNONE;
  }

  if( !fall )
  {
    if( IS_SET( to_room->room_flags, ROOM_INDOORS )
	|| IS_SET( to_room->room_flags, ROOM_SPACECRAFT )
	|| to_room->sector_type == SECT_INSIDE )
    {
      send_to_char( "You can't drive indoors!\r\n", ch );
      return rNONE;
    }

    if( in_room->sector_type == SECT_AIR
	|| to_room->sector_type == SECT_AIR
	|| IS_SET( pexit->exit_info, EX_FLY ) )
    {
      if( ship->ship_class > AIRCRAFT )
      {
	send_to_char( "You'd need to fly to go there.\r\n", ch );
	return rNONE;
      }
    }

    if( in_room->sector_type == SECT_WATER_NOSWIM
	|| to_room->sector_type == SECT_WATER_NOSWIM
	|| to_room->sector_type == SECT_WATER_SWIM
	|| to_room->sector_type == SECT_UNDERWATER
	|| to_room->sector_type == SECT_OCEANFLOOR )
    {
      if( ship->ship_class != BOAT && ship->ship_class != SUBMARINE )
      {
	send_to_char( "You'd need a boat to go there.\r\n", ch );
	return rNONE;
      }
    }

    if( to_room->sector_type == SECT_UNDERWATER
	|| to_room->sector_type == SECT_OCEANFLOOR )
    {
      if( ship->ship_class != SUBMARINE )
      {
	send_to_char( "You'd need a submarine to go there.\r\n", ch );
	return rNONE;
      }
    }

    if( IS_SET( pexit->exit_info, EX_CLIMB ) )
    {
      if( ship->ship_class > AIRCRAFT )
      {
	send_to_char( "You need to fly or climb to get up there.\r\n",
	    ch );
	return rNONE;
      }
    }
  }

  if( to_room->tunnel > 0 )
  {
    CHAR_DATA *ctmp;
    int count = 0;

    for( ctmp = to_room->first_person; ctmp; ctmp = ctmp->next_in_room )
      if( ++count >= to_room->tunnel )
      {
	send_to_char( "There is no room for you in there.\r\n", ch );
	return rNONE;
      }
  }

  if( fall )
    txt = "falls";
  else if( !txt )
  {
    if( ship->ship_class < BOAT )
      txt = "fly";
    else if( ship->ship_class <= SUBMARINE )
    {
      txt = "float";
    }
    else if( ship->ship_class > SUBMARINE )
    {
      txt = "drive";
    }
  }

  sprintf( buf, "$n %ss the vehicle $T.", txt );
  act( AT_ACTION, buf, ch, NULL, dir_name[door], TO_ROOM );
  sprintf( buf, "You %s the vehicle $T.", txt );
  act( AT_ACTION, buf, ch, NULL, dir_name[door], TO_CHAR );
  sprintf( buf, "%s %ss %s.", ship->name, txt, dir_name[door] );
  echo_to_room( AT_ACTION, get_room_index( ship->location ), buf );

  extract_ship( ship );
  ship_to_room( ship, to_room->vnum );
  ship->location = to_room->vnum;
  ship->lastdoc = ship->location;

  if( fall )
    txt = "falls";
  else if( ship->ship_class < BOAT )
    txt = "flys in";
  else if( ship->ship_class < LAND_VEHICLE )
  {
    txt = "floats in";
  }
  else if( ship->ship_class == LAND_VEHICLE )
  {
    txt = "drives in";
  }

  switch ( door )
  {
    default:
      dtxt = "somewhere";
      break;

    case 0:
      dtxt = "the south";
      break;

    case 1:
      dtxt = "the west";
      break;

    case 2:
      dtxt = "the north";
      break;

    case 3:
      dtxt = "the east";
      break;

    case 4:
      dtxt = "below";
      break;

    case 5:
      dtxt = "above";
      break;

    case 6:
      dtxt = "the south-west";
      break;

    case 7:
      dtxt = "the south-east";
      break;

    case 8:
      dtxt = "the north-west";
      break;

    case 9:
      dtxt = "the north-east";
      break;
  }

  sprintf( buf, "%s %s from %s.", ship->name, txt, dtxt );
  echo_to_room( AT_ACTION, get_room_index( ship->location ), buf );

  for( rch = ch->in_room->last_person; rch; rch = next_rch )
  {
    next_rch = rch->prev_in_room;
    original = rch->in_room;
    char_from_room( rch );
    char_to_room( rch, to_room );
    do_look( rch, STRLIT_AUTO );
    char_from_room( rch );
    char_to_room( rch, original );
  }

  return retcode;
}

void do_bomb( CHAR_DATA * ch, char *argument )
{
}

void do_chaff( CHAR_DATA * ch, char *argument )
{
  int chance;
  SHIP_DATA *ship;


  if( ( ship = ship_from_gunseat( ch->in_room ) ) == NULL )
  {
    send_to_char
      ( "&RYou must be at the weapon controls of a ship to do that!\r\n",
	ch );
    return;
  }

  if( ship->ship_class > SPACE_STATION )
  {
    send_to_char( "&RThis isn't a spacecraft!\r\n", ch );
    return;
  }


  if( autofly( ship ) )
  {
    send_to_char( "&RYou'll have to turn the autopilot off first...\r\n",
	ch );
    return;
  }

  if( ship->shipstate == SHIP_HYPERSPACE )
  {
    send_to_char( "&RYou can only do that in realspace!\r\n", ch );
    return;
  }
  if( ship->shipstate == SHIP_DOCKED )
  {
    send_to_char( "&RYou can't do that until after you've launched!\r\n",
	ch );
    return;
  }
  if( ship->chaff <= 0 )
  {
    send_to_char( "&RYou don't have any chaff to release!\r\n", ch );
    return;
  }

  chance = character_skill_level( ch, gsn_weaponsystems );

  if( number_percent() > chance )
  {
    send_to_char( "&RYou can't figure out which switch it is.\r\n", ch );
    return;
  }

  ship->chaff--;

  ship->chaff_released++;

  send_to_char( "You flip the chaff release switch.\r\n", ch );
  act( AT_PLAIN, "$n flips a switch on the control pannel", ch,
      NULL, argument, TO_ROOM );
  echo_to_cockpit( AT_YELLOW, ship,
      "A burst of chaff is released from the ship." );

  learn_from_success( ch, gsn_weaponsystems );

}

bool autofly( SHIP_DATA * ship )
{
  return ship->type == MOB_SHIP || ship->autopilot;
}

void do_rentship( CHAR_DATA * ch, char *argument )
{
}

SHIP_DATA *ship_create( void )
{
  size_t n = 0;
  SHIP_DATA *s = NULL;
  CREATE( s, SHIP_DATA, 1 );

  s->next = NULL;
  s->prev = NULL;
  s->next_in_starsystem = NULL;
  s->prev_in_starsystem = NULL;
  s->next_in_room = NULL;
  s->prev_in_room = NULL;
  s->in_room = NULL;
  s->first_room = NULL;
  s->last_room = NULL;
  s->pilotseat = NULL;
  s->gunseat = NULL;
  s->viewscreen = NULL;
  s->engine = NULL;
  s->entrance = NULL;
  s->starsystem = NULL;
  s->first_turret = NULL;
  s->last_turret = NULL;
  s->first_hangar = NULL;
  s->last_hangar = NULL;
  s->filename = NULL;
  s->name = NULL;
  s->home = NULL;
  s->owner = NULL;
  s->pilot = NULL;
  s->copilot = NULL;
  s->dest = NULL;
  s->type = 0;
  s->ship_class = 0;
  s->model = 0;
  s->hyperspeed = 0;
  s->hyperdistance = 0;
  s->realspeed = 0;
  s->currspeed = 0;
  s->shipstate = SHIP_READY;
  s->laserstate = LASER_READY;
  s->missilestate = MISSILE_READY;
  s->missiles = 0;
  s->maxmissiles = 0;
  s->lasers = 0;
  s->tractorbeam = 0;
  s->manuever = 0;
  s->hatchopen = FALSE;
  s->autorecharge = FALSE;
  s->autotrack = FALSE;
  s->autospeed = FALSE;
  s->maxenergy = 0;
  s->energy = 0;
  s->shield = 0;
  s->maxshield = 0;
  s->hull = 0;
  s->maxhull = 0;
  s->location = 0;
  s->lastdoc = 0;
  s->shipyard = 0;
  s->collision = 0;
  s->target = NULL;
  s->currjump = NULL;
  s->chaff = 0;
  s->maxchaff = 0;
  s->chaff_released = FALSE;
  s->autopilot = FALSE;

  vector_init( &s->pos );
  vector_init( &s->head );
  vector_init( &s->jump );

  for( n = 0; n < MAX_SHIP_ROOMS; ++n )
    s->description[n] = NULL;

  return s;
}

void ship_untarget_all( SHIP_DATA *ship )
{
  TURRET_DATA *turret = NULL;

  ship->target = NULL;

  for( turret = ship->first_turret; turret; turret = turret->next )
    {
      turret->target = NULL;
    }
}

void transship( SHIP_DATA *ship, int vnum )
{
  if( get_room_index( vnum ) )
    {
      ship_untarget_by_attackers( ship );
      ship_untarget_by_missiles( ship );

      if( ship->starsystem )
	ship_from_starsystem( ship, ship->starsystem );

      extract_ship( ship );
      ship_to_room( ship, vnum );
      ship->shipstate = SHIP_DOCKED;
      ship->location = vnum;
      ship->lastdoc = vnum;
      ship_untarget_all( ship );
      save_ship( ship );
    }
}

void do_transship( CHAR_DATA *ch, char *argument )
{
  char shipname[MAX_INPUT_LENGTH];
  SHIP_DATA *ship = NULL;
  int vnum = 0;

  argument = one_argument( argument, shipname );

  if( *argument == '\0' || *shipname == '\0' )
    {
      ch_printf( ch, "Usage: transship <ship> <destination vnum>\r\n" );
      return;
    }

  if( !( ship = get_ship( shipname ) ) )
    {
      ch_printf( ch, "Can't find a ship with that name.\r\n" );
      return;
    }

  vnum = strtol( argument, 0, 10 );

  if( !get_room_index( vnum ) )
    {
      ch_printf( ch, "No room with that vnum.\r\n" );
      return;
    }

  transship( ship, vnum );
  ch_printf( ch, "Ship successfully transferred to new location.\r\n" );
}
