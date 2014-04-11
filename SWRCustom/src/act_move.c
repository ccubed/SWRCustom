#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "mud.h"

const int sent_total[SECT_MAX] = {
  4,  /* SECT_INSIDE */
  24, /* SECT_CITY */
  4,  /* SECT_FIELD */
  4,  /* SECT_FOREST */
  1,  /* SECT_HILLS */
  1,  /* SECT_MOUNTAIN */
  1,  /* SECT_WATER_SWIM */
  1,  /* SECT_WATER_NOSWIM */
  1,  /* SECT_UNDERWATER */
  2,  /* SECT_AIR */
  2,  /* SECT_DESERT */
  1,  /* SECT_DUNNO */
  1,  /* SECT_OCEANFLOOR */
  1,  /* SECT_UNDERGROUND */
  1,  /* SECT_SCRUB */
  1,  /* SECT_ROCKY */
  1,  /* SECT_SAVANNA */
  1,  /* SECT_TUNDRA */
  1,  /* SECT_GLACIAL */
  1,  /* SECT_RAINFOREST */
  1,  /* SECT_JUNGLE */
  1,  /* SECT_SWAMP */
  1,  /* SECT_WETLANDS */
  1,  /* SECT_BRUSH */
  1,  /* SECT_STEPPE */
  1,  /* SECT_FARMLAND */
  1   /* SECT_VOLCANIC */
};

const char *const room_sents[SECT_MAX][25] = {
  /* SECT_INSIDE */
  {
    "The rough hewn walls are made of granite.",
    "You see an occasional spider crawling around.",
    "You notice signs of a recent battle from the bloodstains on the floor.",
    "This place hasa damp musty odour not unlike rotting vegetation."},
  /* SECT_CITY */
  {
    "You notice the occasional stray looking for food.",
    "Tall buildings loom on either side of you stretching to the sky.",
    "Some street people are putting on an interesting display of talent trying to earn some credits.",
    "Two people nearby shout heated words of argument at one another.",
    "You think you can make out several shady figures talking down a dark alleyway."
      "A slight breeze blows through the tall buildings.",
    "A small crowd of people have gathered at one side of the street.",
    "Clouds far above you obscure the tops of the highest skyscrapers.",
    "A speeder moves slowly through the street avoiding pedestrians.",
    "A cloudcar flys by overhead.",
    "The air is thick and hard to breath.",
    "The many smells of the city assault your senses.",
    "You hear a scream far of in the distance.",
    "The buildings around you seem endless in number.",
    "The city stretches seemingly endless in all directions.",
    "The street is wide and long.",
    "A swoop rider passes quickly by weaving in and out of pedestrians and other vehicles.",
    "The surface of the road is worn from many travellers.",
    "You feel it would be very easy to get lost in such an enormous city.",
    "You can see other streets above and bellow this one running in many directions.",
    "There are entrances to several buildings at this level.",
    "Along the edge of the street railings prevent pedestrians from falling to their death.",
    "In between the many towers you can see down into depths of the lower city.",
    "A grate in the street prevents rainwater from building up.",
    "You can see you reflection in several of the transparisteel windows as you pass by."
      "You hear a scream far of in the distance.",
  },
  /* SECT_FIELD */
  {
    "You notice sparce patches of brush and shrubs.",
    "There is a small cluster of trees far off in the distance.",
    "Around you are grassy fields as far as the eye can see.",
    "Throughout the plains a wide variety of weeds and wildflowers are scattered."},
  /* SECT_FOREST */
  {
    "Tall, dark evergreens prevent you from seeing very far.",
    "Many huge oak trees that look several hundred years old are here.",
    "You notice a solitary lonely weeping willow.",
    "To your left is a patch of bright white birch trees, slender and tall."},
  /* SECT_HILLS */
  {
    "The rolling hills are lightly speckled with violet wildflowers."},
  /* SECT_MOUNTAIN */
  {
    "The rocky mountain pass offers many hiding places."},
  /* SECT_WATER_SWIM */
  {
    "The water is smooth as glass."},
  /* SECT_WATER_NOSWIM */
  {
    "Rough waves splash about angrily."},
  /* SECT_UNDERWATER */
  {
    "A small school of fish swims by."},
  /* SECT_AIR */
  {
    "The land is far far below.",
    "A misty haze of clouds drifts by."},
  /* SECT_DESERT */
  {
    "Around you is sand as far as the eye can see.",
    "You think you see an oasis far in the distance."},
  /* SECT_DUNNO */
  {
    "You notice nothing unusual."},
  /* SECT_OCEANFLOOR */
  {
    "There are many rocks and coral which litter the ocean floor."},
  /* SECT_UNDERGROUND */
  {
    "You stand in a lengthy tunnel of rock."},
  /* SECT_SCRUB */
  {
    "Hmm..."},
  /* SECT_ROCKY */
  {
    "Hmm..."},
  /* SECT_SAVANNA */
  {
    "Hmm..."},
  /* SECT_TUNDRA */
  {
    "Hmm..."},
  /* SECT_GLACIAL */
  {
    "Hmm..."},
  /* SECT_RAINFOREST */
  {
    "Hmm..."},
  /* SECT_JUNGLE */
  {
    "Hmm..."},
  /* SECT_SWAMP */
  {
    "Hmm..."},
  /* SECT_WETLANDS */
  {
    "Hmm..."},
  /* SECT_BRUSH */
  {
    "Hmm..."},
  /* SECT_STEPPE */
  {
    "Hmm..."},
  /* SECT_FARMLAND */
  {
    "Hmm..."},
  /* SECT_VOLCANIC */
  {
    "Hmm..."}
};

int wherehome( const CHAR_DATA * ch )
{
  SHIP_DATA *ship = 0;
  PLANET_DATA *planet = 0;
  CLAN_DATA *clan = 0;
  ROOM_INDEX_DATA *room = 0;

  if( ch->plr_home )
    return ch->plr_home->vnum;

  if( IS_IMMORTAL( ch ) )
    return ROOM_IMMORTAL_START;

  if( ch->pcdata && ch->pcdata->clan )
    for( planet = first_planet; planet; planet = planet->next )
      if( planet->governed_by == clan && planet->area )
	for( room = planet->area->first_room; room;
	    room = room->next_in_area )
	  if( IS_SET( room->room_flags, ROOM_HOTEL )
	      && !IS_SET( room->room_flags, ROOM_PLR_HOME ) )
	    return room->vnum;

  for( ship = first_ship; ship; ship = ship->next )
  {
    if( !str_cmp( ship->owner, ch->name )
	&& ship->shipstate == SHIP_DOCKED )
      return ship->lastdoc;
  }

  for( planet = first_planet; planet; planet = planet->next )
    if( planet->area )
      for( room = planet->area->first_room; room; room = room->next_in_area )
	if( IS_SET( room->room_flags, ROOM_HOTEL )
	    && !IS_SET( room->room_flags, ROOM_PLR_HOME ) )
	  return room->vnum;

  return ROOM_VNUM_LIMBO;
}

void decorate_room( ROOM_INDEX_DATA * room )
{
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  int nRand;
  int iRand, len;
  int previous[8];
  int sector = room->sector_type;

  if( room->name )
    STRFREE( room->name );
  if( room->description )
    STRFREE( room->description );

  room->name = STRALLOC( get_sector_name( sector ) );
  buf[0] = '\0';
  nRand = number_range( 1, UMIN( 8, sent_total[sector] ) );

  for( iRand = 0; iRand < nRand; iRand++ )
    previous[iRand] = -1;

  for( iRand = 0; iRand < nRand; iRand++ )
  {
    while( previous[iRand] == -1 )
    {
      int x, z;

      x = number_range( 0, sent_total[sector] - 1 );

      for( z = 0; z < iRand; z++ )
	if( previous[z] == x )
	  break;

      if( z < iRand )
	continue;

      previous[iRand] = x;

      len = strlen( buf );
      sprintf( buf2, "%s", room_sents[sector][x] );
      if( len > 5 && buf[len - 1] == '.' )
      {
	strcat( buf, "  " );
	buf2[0] = UPPER( buf2[0] );
      }
      else if( len == 0 )
	buf2[0] = UPPER( buf2[0] );
      strcat( buf, buf2 );
    }
  }
  sprintf( buf2, "%s\r\n", wordwrap( buf, 78 ) );
  room->description = STRALLOC( buf2 );
}

void clear_vrooms()
{
}

/*
 * Function to get the equivelant exit of DIR 0-MAXDIR out of linked list.
 * Made to allow old-style diku-merc exit functions to work.	-Thoric
 */
EXIT_DATA *get_exit( const ROOM_INDEX_DATA * room, short dir )
{
  EXIT_DATA *xit;

  if( !room )
  {
    bug( "Get_exit: NULL room", 0 );
    return NULL;
  }

  for( xit = room->first_exit; xit; xit = xit->next )
    if( xit->vdir == dir )
      return xit;
  return NULL;
}

/*
 * Function to get an exit, leading the the specified room
 */
EXIT_DATA *get_exit_to( const ROOM_INDEX_DATA * room, short dir, long vnum )
{
  EXIT_DATA *xit;

  if( !room )
  {
    bug( "Get_exit: NULL room", 0 );
    return NULL;
  }

  for( xit = room->first_exit; xit; xit = xit->next )
    if( xit->vdir == dir && xit->vnum == vnum )
      return xit;
  return NULL;
}

/*
 * Function to get the nth exit of a room			-Thoric
 */
EXIT_DATA *get_exit_num( const ROOM_INDEX_DATA * room, short count )
{
  EXIT_DATA *xit;
  int cnt;

  if( !room )
  {
    bug( "Get_exit: NULL room", 0 );
    return NULL;
  }

  for( cnt = 0, xit = room->first_exit; xit; xit = xit->next )
    if( ++cnt == count )
      return xit;
  return NULL;
}


/*
 * Modify movement due to encumbrance				-Thoric
 */
short encumbrance( const CHAR_DATA * ch, short move )
{
  int max = can_carry_w( ch );
  int cur = ch->carry_weight;

  if( cur >= max )
    return move * 4;
  else if( cur >= max * 0.95 )
    return ( short ) ( move * 3.5 );
  else if( cur >= max * 0.90 )
    return move * 3;
  else if( cur >= max * 0.85 )
    return ( short ) ( move * 2.5 );
  else if( cur >= max * 0.80 )
    return move * 2;
  else if( cur >= max * 0.75 )
    return ( short ) ( move * 1.5 );
  else
    return move;
}


/*
 * Check to see if a character can fall down, checks for looping   -Thoric
 */
bool will_fall( CHAR_DATA * ch, int fall )
{
  if( IS_SET( ch->in_room->room_flags, ROOM_NOFLOOR )
      && CAN_GO( ch, DIR_DOWN )
      && ( !IS_AFFECTED( ch, AFF_FLYING )
	|| ( ch->mount && !IS_AFFECTED( ch->mount, AFF_FLYING ) ) ) )
  {
    if( fall > 80 )
    {
      bug( "Falling (in a loop?) more than 80 rooms: vnum %d",
	  ch->in_room->vnum );
      char_from_room( ch );
      char_to_room( ch, get_room_index( wherehome( ch ) ) );
      fall = 0;
      return TRUE;
    }

    set_char_color( AT_FALLING, ch );
    send_to_char( "You're falling down...\r\n", ch );
    move_char( ch, get_exit( ch->in_room, DIR_DOWN ), ++fall );
    return TRUE;
  }

  return FALSE;
}

ch_ret move_char( CHAR_DATA * ch, EXIT_DATA * pexit, int fall )
{
  ROOM_INDEX_DATA *in_room = NULL;
  ROOM_INDEX_DATA *to_room = NULL;
  ROOM_INDEX_DATA *from_room = NULL;
  char buf[MAX_STRING_LENGTH];
  const char *txt = NULL;
  const char *dtxt = NULL;
  ch_ret retcode = rNONE;
  short door = 0, distance = 0;
  bool drunk = FALSE;
  bool brief = FALSE;

  if( !IS_NPC( ch ) )
    if( IS_DRUNK( ch, 2 ) && ( ch->position != POS_SHOVE )
	&& ( ch->position != POS_DRAG ) )
      drunk = TRUE;

  if( drunk && !fall )
  {
    door = number_door();
    pexit = get_exit( ch->in_room, door );
  }

#ifdef DEBUG
  if( pexit )
  {
    sprintf( buf, "move_char: %s to door %d", ch->name, pexit->vdir );
    log_string( buf );
  }
#endif

  if( IS_NPC( ch ) && IS_SET( ch->act, ACT_MOUNTED ) )
    return retcode;

  in_room = ch->in_room;
  from_room = in_room;
  if( !pexit || ( to_room = pexit->to_room ) == NULL )
  {
    if( drunk )
      send_to_char( "You hit a wall in your drunken state.\r\n", ch );
    else
      send_to_char( "Alas, you cannot go that way.\r\n", ch );
    return rNONE;
  }

  door = pexit->vdir;
  distance = pexit->distance;

  /*
   * Exit is only a "window", there is no way to travel in that direction
   * unless it's a door with a window in it           -Thoric
   */
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

  if( ( IS_SET( pexit->exit_info, EX_NOMOB )
	|| IS_SET( to_room->room_flags, ROOM_NO_MOB ) ) && IS_NPC( ch ) )
  {
    act( AT_PLAIN, "Mobs can't enter there.", ch, NULL, NULL, TO_CHAR );
    return rNONE;
  }

  if( IS_SET( pexit->exit_info, EX_CLOSED )
      && ( !IS_AFFECTED( ch, AFF_PASS_DOOR )
	|| IS_SET( pexit->exit_info, EX_NOPASSDOOR ) ) )
  {
    if( !IS_SET( pexit->exit_info, EX_SECRET )
	&& !IS_SET( pexit->exit_info, EX_DIG ) )
    {
      if( drunk )
      {
	act( AT_PLAIN, "$n runs into the $d in $s drunken state.", ch,
	    NULL, pexit->keyword, TO_ROOM );
	act( AT_PLAIN, "You run into the $d in your drunken state.", ch,
	    NULL, pexit->keyword, TO_CHAR );
      }
      else
	act( AT_PLAIN, "The $d is closed.", ch, NULL, pexit->keyword,
	    TO_CHAR );
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

  if( !fall
      && IS_AFFECTED( ch, AFF_CHARM )
      && ch->master && in_room == ch->master->in_room )
  {
    send_to_char( "What?  And leave your beloved master?\r\n", ch );
    return rNONE;
  }

  if( room_is_private( ch, to_room ) )
  {
    send_to_char( "That room is private right now.\r\n", ch );
    return rNONE;
  }

  if( !fall && !IS_NPC( ch ) )
  {
    int move = 0;

    if( in_room->sector_type == SECT_AIR
	|| to_room->sector_type == SECT_AIR
	|| IS_SET( pexit->exit_info, EX_FLY ) )
    {
      if( ch->mount && !IS_AFFECTED( ch->mount, AFF_FLYING ) )
      {
	send_to_char( "Your mount can't fly.\r\n", ch );
	return rNONE;
      }
      if( !ch->mount && !IS_AFFECTED( ch, AFF_FLYING ) )
      {
	send_to_char( "You'd need to fly to go there.\r\n", ch );
	return rNONE;
      }
    }

    if( in_room->sector_type == SECT_WATER_NOSWIM
	|| to_room->sector_type == SECT_WATER_NOSWIM )
    {
      bool found =FALSE;

      if( ch->mount )
      {
	if( IS_AFFECTED( ch->mount, AFF_FLYING )
	    || IS_AFFECTED( ch->mount, AFF_FLOATING ) )
	  found = TRUE;
      }
      else
	if( IS_AFFECTED( ch, AFF_FLYING )
	    || IS_AFFECTED( ch, AFF_FLOATING ) )
	  found = TRUE;

      if( !found )
      {
	send_to_char( "You'd need a boat to go there.\r\n", ch );
	return rNONE;
      }
    }

    if( IS_SET( pexit->exit_info, EX_CLIMB ) )
    {
      bool found = FALSE;

      if( ch->mount && IS_AFFECTED( ch->mount, AFF_FLYING ) )
	found = TRUE;
      else if( IS_AFFECTED( ch, AFF_FLYING ) )
	found = TRUE;

      if( !found && !ch->mount )
      {
	if( ( !IS_NPC( ch )
	      && number_percent() > character_skill_level( ch,
		gsn_climb ) )
	    || drunk || ch->mental_state < -90 )
	{
	  send_to_char
	    ( "You start to climb... but lose your grip and fall!\r\n",
	      ch );
	  learn_from_failure( ch, gsn_climb );
	  if( pexit->vdir == DIR_DOWN )
	  {
	    retcode = move_char( ch, pexit, 1 );
	    return retcode;
	  }
	  set_char_color( AT_HURT, ch );
	  send_to_char( "OUCH! You hit the ground!\r\n", ch );
	  WAIT_STATE( ch, 20 );
	  retcode =
	    damage( ch, ch, ( pexit->vdir == DIR_UP ? 10 : 5 ),
		TYPE_UNDEFINED );
	  return retcode;
	}
	found = TRUE;
	learn_from_success( ch, gsn_climb );
	WAIT_STATE( ch, skill_table[gsn_climb]->beats );
	txt = "climbs";
      }

      if( !found )
      {
	send_to_char( "You can't climb.\r\n", ch );
	return rNONE;
      }
    }

    if( ch->mount )
    {
      switch ( ch->mount->position )
      {
	case POS_DEAD:
	  send_to_char( "Your mount is dead!\r\n", ch );
	  return rNONE;
	  break;

	case POS_MORTAL:
	case POS_INCAP:
	  send_to_char( "Your mount is hurt far too badly to move.\r\n",
	      ch );
	  return rNONE;
	  break;

	case POS_STUNNED:
	  send_to_char( "Your mount is too stunned to do that.\r\n", ch );
	  return rNONE;
	  break;

	case POS_SLEEPING:
	  send_to_char( "Your mount is sleeping.\r\n", ch );
	  return rNONE;
	  break;

	case POS_RESTING:
	  send_to_char( "Your mount is resting.\r\n", ch );
	  return rNONE;
	  break;

	case POS_SITTING:
	  send_to_char( "Your mount is sitting down.\r\n", ch );
	  return rNONE;
	  break;

	default:
	  break;
      }

      if( !IS_AFFECTED( ch->mount, AFF_FLYING )
	  && !IS_AFFECTED( ch->mount, AFF_FLOATING ) )
	move = movement_loss[UMIN( SECT_MAX - 1, in_room->sector_type )];
      else
	move = 1;
      if( ch->mount->move < move )
      {
	send_to_char( "Your mount is too exhausted.\r\n", ch );
	return rNONE;
      }
    }
    else
    {
      if( !IS_AFFECTED( ch, AFF_FLYING )
	  && !IS_AFFECTED( ch, AFF_FLOATING ) )
	move =
	  encumbrance( ch,
	      movement_loss[UMIN
	      ( SECT_MAX - 1,
		in_room->sector_type )] );
      else
	move = 1;
      if( ch->move < move )
      {
	send_to_char( "You are too exhausted.\r\n", ch );
	return rNONE;
      }
    }

    WAIT_STATE( ch, move );
    if( ch->mount )
      ch->mount->move -= move;
    else
      ch->move -= move;
  }

  /*
   * Check if player can fit in the room
   */
  if( to_room->tunnel > 0 )
  {
    CHAR_DATA *ctmp = NULL;
    int count = ch->mount ? 1 : 0;

    for( ctmp = to_room->first_person; ctmp; ctmp = ctmp->next_in_room )
      if( ++count >= to_room->tunnel )
      {
	if( ch->mount && count == to_room->tunnel )
	  send_to_char
	    ( "There is no room for both you and your mount in there.\r\n",
	      ch );
	else
	  send_to_char( "There is no room for you in there.\r\n", ch );
	return rNONE;
      }
  }


  if( !IS_AFFECTED( ch, AFF_SNEAK )
      && ( IS_NPC( ch ) || !IS_SET( ch->act, PLR_WIZINVIS ) ) )
  {
    if( fall )
      txt = "falls";
    else if( !txt )
    {
      if( ch->mount )
      {
	if( IS_AFFECTED( ch->mount, AFF_FLOATING ) )
	  txt = "floats";
	else if( IS_AFFECTED( ch->mount, AFF_FLYING ) )
	  txt = "flys";
	else
	  txt = "rides";
      }
      else
      {
	if( IS_AFFECTED( ch, AFF_FLOATING ) )
	{
	  if( drunk )
	    txt = "floats unsteadily";
	  else
	    txt = "floats";
	}
	else if( IS_AFFECTED( ch, AFF_FLYING ) )
	{
	  if( drunk )
	    txt = "flys shakily";
	  else
	    txt = "flys";
	}
	else if( ch->position == POS_SHOVE )
	  txt = "is shoved";
	else if( ch->position == POS_DRAG )
	  txt = "is dragged";
	else
	{
	  if( drunk )
	    txt = "stumbles drunkenly";
	  else
	    txt = "leaves";
	}
      }
    }
    if( ch->mount )
    {
      sprintf( buf, "$n %s %s upon $N.", txt, dir_name[door] );
      act( AT_ACTION, buf, ch, NULL, ch->mount, TO_NOTVICT );
    }
    else
    {
      sprintf( buf, "$n %s $T.", txt );
      act( AT_ACTION, buf, ch, NULL, dir_name[door], TO_ROOM );
    }
  }

  rprog_leave_trigger( ch );
  if( char_died( ch ) )
    return global_retcode;

  char_from_room( ch );
  if( ch->mount )
  {
    rprog_leave_trigger( ch->mount );
    if( char_died( ch ) )
      return global_retcode;
    if( ch->mount )
    {
      char_from_room( ch->mount );
      char_to_room( ch->mount, to_room );
    }
  }


  char_to_room( ch, to_room );
  if( !IS_AFFECTED( ch, AFF_SNEAK )
      && ( IS_NPC( ch ) || !IS_SET( ch->act, PLR_WIZINVIS ) ) )
  {
    if( fall )
      txt = "falls";
    else if( ch->mount )
    {
      if( IS_AFFECTED( ch->mount, AFF_FLOATING ) )
	txt = "floats in";
      else if( IS_AFFECTED( ch->mount, AFF_FLYING ) )
	txt = "flys in";
      else
	txt = "rides in";
    }
    else
    {
      if( IS_AFFECTED( ch, AFF_FLOATING ) )
      {
	if( drunk )
	  txt = "floats in unsteadily";
	else
	  txt = "floats in";
      }
      else if( IS_AFFECTED( ch, AFF_FLYING ) )
      {
	if( drunk )
	  txt = "flys in shakily";
	else
	  txt = "flys in";
      }
      else if( ch->position == POS_SHOVE )
	txt = "is shoved in";
      else if( ch->position == POS_DRAG )
	txt = "is dragged in";
      else
      {
	if( drunk )
	  txt = "stumbles drunkenly in";
	else
	  txt = "arrives";
      }
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
    if( ch->mount )
    {
      sprintf( buf, "$n %s from %s upon $N.", txt, dtxt );
      act( AT_ACTION, buf, ch, NULL, ch->mount, TO_ROOM );
    }
    else
    {
      sprintf( buf, "$n %s from %s.", txt, dtxt );
      act( AT_ACTION, buf, ch, NULL, NULL, TO_ROOM );
    }
  }

  do_look( ch, STRLIT_AUTO );

  if( brief )
    SET_BIT( ch->act, PLR_BRIEF );

  /* BIG ugly looping problem here when the character is mptransed back
     to the starting room.  To avoid this, check how many chars are in 
     the room at the start and stop processing followers after doing
     the right number of them.  -- Narn
     */
  if( !fall )
  {
    CHAR_DATA *fch = NULL;
    CHAR_DATA *nextinroom = NULL;
    int chars = 0, count = 0;

    for( fch = from_room->first_person; fch; fch = fch->next_in_room )
      chars++;

    for( fch = from_room->first_person; fch && ( count < chars );
	fch = nextinroom )
    {
      nextinroom = fch->next_in_room;
      count++;

      if( fch != ch		/* loop room bug fix here by Thoric */
	  && fch->master == ch && fch->position == POS_STANDING )
      {
	if( !get_exit( from_room, door ) )
	{
	  act( AT_ACTION,
	      "The entrance closes behind $N, preventing you from following!",
	      fch, NULL, ch, TO_CHAR );
	  continue;
	}


	act( AT_ACTION, "You follow $N.", fch, NULL, ch, TO_CHAR );
	move_char( fch, pexit, 0 );
      }
    }
  }


  if( char_died( ch ) )
    return retcode;

  mprog_entry_trigger( ch );
  if( char_died( ch ) )
    return retcode;

  rprog_enter_trigger( ch );
  if( char_died( ch ) )
    return retcode;

  mprog_greet_trigger( ch );
  if( char_died( ch ) )
    return retcode;

  oprog_greet_trigger( ch );
  if( char_died( ch ) )
    return retcode;

  if( !will_fall( ch, fall ) && fall > 0 )
  {
    if( !IS_AFFECTED( ch, AFF_FLOATING )
	|| ( ch->mount && !IS_AFFECTED( ch->mount, AFF_FLOATING ) ) )
    {
      set_char_color( AT_HURT, ch );
      send_to_char( "OUCH! You hit the ground!\r\n", ch );
      WAIT_STATE( ch, 20 );
      retcode = damage( ch, ch, 50 * fall, TYPE_UNDEFINED );
    }
    else
    {
      set_char_color( AT_MAGIC, ch );
      send_to_char( "You lightly float down to the ground.\r\n", ch );
    }
  }
  return retcode;
}

void do_north( CHAR_DATA * ch, char *argument )
{
  move_char( ch, get_exit( ch->in_room, DIR_NORTH ), 0 );
}

void do_east( CHAR_DATA * ch, char *argument )
{
  move_char( ch, get_exit( ch->in_room, DIR_EAST ), 0 );
}

void do_south( CHAR_DATA * ch, char *argument )
{
  move_char( ch, get_exit( ch->in_room, DIR_SOUTH ), 0 );
}

void do_west( CHAR_DATA * ch, char *argument )
{
  move_char( ch, get_exit( ch->in_room, DIR_WEST ), 0 );
}

void do_up( CHAR_DATA * ch, char *argument )
{
  move_char( ch, get_exit( ch->in_room, DIR_UP ), 0 );
}

void do_down( CHAR_DATA * ch, char *argument )
{
  move_char( ch, get_exit( ch->in_room, DIR_DOWN ), 0 );
}

void do_northeast( CHAR_DATA * ch, char *argument )
{
  move_char( ch, get_exit( ch->in_room, DIR_NORTHEAST ), 0 );
}

void do_northwest( CHAR_DATA * ch, char *argument )
{
  move_char( ch, get_exit( ch->in_room, DIR_NORTHWEST ), 0 );
}

void do_southeast( CHAR_DATA * ch, char *argument )
{
  move_char( ch, get_exit( ch->in_room, DIR_SOUTHEAST ), 0 );
}

void do_southwest( CHAR_DATA * ch, char *argument )
{
  move_char( ch, get_exit( ch->in_room, DIR_SOUTHWEST ), 0 );
}

EXIT_DATA *find_door( CHAR_DATA * ch, const char *arg, bool quiet )
{
  EXIT_DATA *pexit = NULL;
  int door = 0;

  if( arg == NULL || !str_cmp( arg, "" ) )
    return NULL;

  if( !str_cmp( arg, "n" ) || !str_cmp( arg, "north" ) )
    door = 0;
  else if( !str_cmp( arg, "e" ) || !str_cmp( arg, "east" ) )
    door = 1;
  else if( !str_cmp( arg, "s" ) || !str_cmp( arg, "south" ) )
    door = 2;
  else if( !str_cmp( arg, "w" ) || !str_cmp( arg, "west" ) )
    door = 3;
  else if( !str_cmp( arg, "u" ) || !str_cmp( arg, "up" ) )
    door = 4;
  else if( !str_cmp( arg, "d" ) || !str_cmp( arg, "down" ) )
    door = 5;
  else if( !str_cmp( arg, "ne" ) || !str_cmp( arg, "northeast" ) )
    door = 6;
  else if( !str_cmp( arg, "nw" ) || !str_cmp( arg, "northwest" ) )
    door = 7;
  else if( !str_cmp( arg, "se" ) || !str_cmp( arg, "southeast" ) )
    door = 8;
  else if( !str_cmp( arg, "sw" ) || !str_cmp( arg, "southwest" ) )
    door = 9;
  else
  {
    for( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
    {
      if( ( quiet || IS_SET( pexit->exit_info, EX_ISDOOR ) )
	  && pexit->keyword && nifty_is_name( arg, pexit->keyword ) )
	return pexit;
    }
    if( !quiet )
      act( AT_PLAIN, "You see no $T here.", ch, NULL, arg, TO_CHAR );
    return NULL;
  }

  if( ( pexit = get_exit( ch->in_room, door ) ) == NULL )
  {
    if( !quiet )
      act( AT_PLAIN, "You see no $T here.", ch, NULL, arg, TO_CHAR );
    return NULL;
  }

  if( quiet )
    return pexit;

  if( IS_SET( pexit->exit_info, EX_SECRET ) )
  {
    act( AT_PLAIN, "You see no $T here.", ch, NULL, arg, TO_CHAR );
    return NULL;
  }

  if( !IS_SET( pexit->exit_info, EX_ISDOOR ) )
  {
    send_to_char( "You can't do that.\r\n", ch );
    return NULL;
  }

  return pexit;
}

void toggle_bexit_flag( EXIT_DATA * pexit, int flag )
{
  EXIT_DATA *pexit_rev = NULL;

  TOGGLE_BIT( pexit->exit_info, flag );
  if( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev != pexit )
    TOGGLE_BIT( pexit_rev->exit_info, flag );
}

void set_bexit_flag( EXIT_DATA * pexit, int flag )
{
  EXIT_DATA *pexit_rev = NULL;

  SET_BIT( pexit->exit_info, flag );
  if( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev != pexit )
    SET_BIT( pexit_rev->exit_info, flag );
}

void remove_bexit_flag( EXIT_DATA * pexit, int flag )
{
  EXIT_DATA *pexit_rev = NULL;

  REMOVE_BIT( pexit->exit_info, flag );
  if( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev != pexit )
    REMOVE_BIT( pexit_rev->exit_info, flag );
}

void do_open( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  OBJ_DATA *obj = NULL;
  EXIT_DATA *pexit = NULL;

  one_argument( argument, arg );

  if( arg[0] == '\0' )
  {
    do_openhatch( ch, STRLIT_EMPTY );
    return;
  }

  if( ( pexit = find_door( ch, arg, TRUE ) ) != NULL )
  {
    /* 'open door' */
    EXIT_DATA *pexit_rev = NULL;

    if( !IS_SET( pexit->exit_info, EX_ISDOOR ) )
    {
      send_to_char( "You can't do that.\r\n", ch );
      return;
    }
    if( !IS_SET( pexit->exit_info, EX_CLOSED ) )
    {
      send_to_char( "It's already open.\r\n", ch );
      return;
    }
    if( IS_SET( pexit->exit_info, EX_LOCKED ) )
    {
      send_to_char( "It's locked.\r\n", ch );
      return;
    }

    if( !IS_SET( pexit->exit_info, EX_SECRET )
	|| ( pexit->keyword && nifty_is_name( arg, pexit->keyword ) ) )
    {
      act( AT_ACTION, "$n opens the $d.", ch, NULL, pexit->keyword,
	  TO_ROOM );
      act( AT_ACTION, "You open the $d.", ch, NULL, pexit->keyword,
	  TO_CHAR );
      if( ( pexit_rev = pexit->rexit ) != NULL
	  && pexit_rev->to_room == ch->in_room )
      {
	CHAR_DATA *rch = NULL;

	for( rch = pexit->to_room->first_person; rch;
	    rch = rch->next_in_room )
	  act( AT_ACTION, "The $d opens.", rch, NULL,
	      pexit_rev->keyword, TO_CHAR );
	sound_to_room( pexit->to_room, "!!SOUND(door)" );
      }
      remove_bexit_flag( pexit, EX_CLOSED );
      return;
    }
  }

  if( ( obj = get_obj_here( ch, arg ) ) != NULL )
  {
    /* 'open object' */
    if( obj->item_type != ITEM_CONTAINER )
    {
      ch_printf( ch, "%s isn't a container.\r\n",
	  capitalize( obj->short_descr ) );
      return;
    }
    if( !IS_SET( obj->value[1], CONT_CLOSED ) )
    {
      ch_printf( ch, "%s is already open.\r\n",
	  capitalize( obj->short_descr ) );
      return;
    }
    if( !IS_SET( obj->value[1], CONT_CLOSEABLE ) )
    {
      ch_printf( ch, "%s cannot be opened or closed.\r\n",
	  capitalize( obj->short_descr ) );
      return;
    }
    if( IS_SET( obj->value[1], CONT_LOCKED ) )
    {
      ch_printf( ch, "%s is locked.\r\n",
	  capitalize( obj->short_descr ) );
      return;
    }

    REMOVE_BIT( obj->value[1], CONT_CLOSED );
    act( AT_ACTION, "You open $p.", ch, obj, NULL, TO_CHAR );
    act( AT_ACTION, "$n opens $p.", ch, obj, NULL, TO_ROOM );
    return;
  }

  if( !str_cmp( arg, "hatch" ) )
  {
    do_openhatch( ch, argument );
    return;
  }

  do_openhatch( ch, arg );
}

void do_close( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  OBJ_DATA *obj = NULL;
  EXIT_DATA *pexit = NULL;

  one_argument( argument, arg );

  if( arg[0] == '\0' )
  {
    do_closehatch( ch, STRLIT_EMPTY );
    return;
  }

  if( ( pexit = find_door( ch, arg, TRUE ) ) != NULL )
  {
    /* 'close door' */
    EXIT_DATA *pexit_rev = NULL;

    if( !IS_SET( pexit->exit_info, EX_ISDOOR ) )
    {
      send_to_char( "You can't do that.\r\n", ch );
      return;
    }
    if( IS_SET( pexit->exit_info, EX_CLOSED ) )
    {
      send_to_char( "It's already closed.\r\n", ch );
      return;
    }

    act( AT_ACTION, "$n closes the $d.", ch, NULL, pexit->keyword,
	TO_ROOM );
    act( AT_ACTION, "You close the $d.", ch, NULL, pexit->keyword,
	TO_CHAR );

    /* close the other side */
    if( ( pexit_rev = pexit->rexit ) != NULL
	&& pexit_rev->to_room == ch->in_room )
    {
      CHAR_DATA *rch = NULL;

      SET_BIT( pexit_rev->exit_info, EX_CLOSED );
      for( rch = pexit->to_room->first_person; rch;
	  rch = rch->next_in_room )
	act( AT_ACTION, "The $d closes.", rch, NULL, pexit_rev->keyword,
	    TO_CHAR );
    }
    set_bexit_flag( pexit, EX_CLOSED );
    return;
  }

  if( ( obj = get_obj_here( ch, arg ) ) != NULL )
  {
    /* 'close object' */
    if( obj->item_type != ITEM_CONTAINER )
    {
      ch_printf( ch, "%s isn't a container.\r\n",
	  capitalize( obj->short_descr ) );
      return;
    }
    if( IS_SET( obj->value[1], CONT_CLOSED ) )
    {
      ch_printf( ch, "%s is already closed.\r\n",
	  capitalize( obj->short_descr ) );
      return;
    }
    if( !IS_SET( obj->value[1], CONT_CLOSEABLE ) )
    {
      ch_printf( ch, "%s cannot be opened or closed.\r\n",
	  capitalize( obj->short_descr ) );
      return;
    }

    SET_BIT( obj->value[1], CONT_CLOSED );
    act( AT_ACTION, "You close $p.", ch, obj, NULL, TO_CHAR );
    act( AT_ACTION, "$n closes $p.", ch, obj, NULL, TO_ROOM );
    return;
  }

  if( !str_cmp( arg, "hatch" ) )
  {
    do_closehatch( ch, argument );
    return;
  }

  do_closehatch( ch, arg );
}

void do_lock( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  OBJ_DATA *obj = NULL;
  EXIT_DATA *pexit = NULL;

  argument = one_argument( argument, arg );

  if( arg[0] == '\0' )
  {
    send_to_char( "Lock what?\r\n", ch );
    return;
  }

  if( ( pexit = find_door( ch, arg, TRUE ) ) != NULL )
  {
    /* 'lock door' */

    if( !IS_SET( pexit->exit_info, EX_ISDOOR ) )
    {
      send_to_char( "You can't do that.\r\n", ch );
      return;
    }
    if( !IS_SET( pexit->exit_info, EX_CLOSED ) )
    {
      send_to_char( "It's not closed.\r\n", ch );
      return;
    }
    if( pexit->key < 0 )
    {
      send_to_char( "It can't be locked.\r\n", ch );
      return;
    }
    if( atoi( argument ) != pexit->key )
    {
      send_to_char( "Wrong combination.\r\n", ch );
      return;
    }
    if( IS_SET( pexit->exit_info, EX_LOCKED ) )
    {
      send_to_char( "It's already locked.\r\n", ch );
      return;
    }

    if( !IS_SET( pexit->exit_info, EX_SECRET )
	|| ( pexit->keyword && nifty_is_name( arg, pexit->keyword ) ) )
    {
      send_to_char( "*Click*\r\n", ch );
      act( AT_ACTION, "$n locks the $d.", ch, NULL, pexit->keyword,
	  TO_ROOM );
      set_bexit_flag( pexit, EX_LOCKED );
      return;
    }
  }

  if( ( obj = get_obj_here( ch, arg ) ) != NULL )
  {
    /* 'lock object' */
    if( obj->item_type != ITEM_CONTAINER )
    {
      send_to_char( "That's not a container.\r\n", ch );
      return;
    }
    if( !IS_SET( obj->value[1], CONT_CLOSED ) )
    {
      send_to_char( "It's not closed.\r\n", ch );
      return;
    }
    if( obj->value[2] < 0 )
    {
      send_to_char( "It can't be locked.\r\n", ch );
      return;
    }
    if( atoi( argument ) != obj->value[2] )
    {
      send_to_char( "Wrong combination.\r\n", ch );
      return;
    }
    if( IS_SET( obj->value[1], CONT_LOCKED ) )
    {
      send_to_char( "It's already locked.\r\n", ch );
      return;
    }

    SET_BIT( obj->value[1], CONT_LOCKED );
    send_to_char( "*Click*\r\n", ch );
    act( AT_ACTION, "$n locks $p.", ch, obj, NULL, TO_ROOM );
    return;
  }

  ch_printf( ch, "You see no %s here.\r\n", arg );
}

void do_unlock( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  OBJ_DATA *obj = NULL;
  EXIT_DATA *pexit = NULL;

  argument = one_argument( argument, arg );

  if( arg[0] == '\0' )
  {
    send_to_char( "Unlock what?\r\n", ch );
    return;
  }

  if( ( pexit = find_door( ch, arg, TRUE ) ) != NULL )
  {
    /* 'unlock door' */

    if( !IS_SET( pexit->exit_info, EX_ISDOOR ) )
    {
      send_to_char( "You can't do that.\r\n", ch );
      return;
    }
    if( !IS_SET( pexit->exit_info, EX_CLOSED ) )
    {
      send_to_char( "It's not closed.\r\n", ch );
      return;
    }
    if( pexit->key < 0 )
    {
      send_to_char( "It can't be unlocked.\r\n", ch );
      return;
    }
    if( atoi( argument ) != pexit->key )
    {
      send_to_char( "Wrong combination.\r\n", ch );
      return;
    }
    if( !IS_SET( pexit->exit_info, EX_LOCKED ) )
    {
      send_to_char( "It's already unlocked.\r\n", ch );
      return;
    }

    if( !IS_SET( pexit->exit_info, EX_SECRET )
	|| ( pexit->keyword && nifty_is_name( arg, pexit->keyword ) ) )
    {
      send_to_char( "*Click*\r\n", ch );
      act( AT_ACTION, "$n unlocks the $d.", ch, NULL, pexit->keyword,
	  TO_ROOM );
      remove_bexit_flag( pexit, EX_LOCKED );
      return;
    }
  }

  if( ( obj = get_obj_here( ch, arg ) ) != NULL )
  {
    /* 'unlock object' */
    if( obj->item_type != ITEM_CONTAINER )
    {
      send_to_char( "That's not a container.\r\n", ch );
      return;
    }
    if( !IS_SET( obj->value[1], CONT_CLOSED ) )
    {
      send_to_char( "It's not closed.\r\n", ch );
      return;
    }
    if( obj->value[2] < 0 )
    {
      send_to_char( "It can't be unlocked.\r\n", ch );
      return;
    }
    if( atoi( argument ) != obj->value[2] )
    {
      send_to_char( "Wrong combination.\r\n", ch );
      return;
    }
    if( !IS_SET( obj->value[1], CONT_LOCKED ) )
    {
      send_to_char( "It's already unlocked.\r\n", ch );
      return;
    }

    REMOVE_BIT( obj->value[1], CONT_LOCKED );
    send_to_char( "*Click*\r\n", ch );
    act( AT_ACTION, "$n unlocks $p.", ch, obj, NULL, TO_ROOM );
    return;
  }

  ch_printf( ch, "You see no %s here.\r\n", arg );
  return;
}

void do_bashdoor( CHAR_DATA * ch, char *argument )
{
  EXIT_DATA *pexit = NULL;
  char arg[MAX_INPUT_LENGTH];

  if( !IS_NPC( ch ) && character_skill_level( ch, gsn_bashdoor ) <= 0 )
  {
    send_to_char( "You're not enough of a warrior to bash doors!\r\n", ch );
    return;
  }

  one_argument( argument, arg );

  if( arg[0] == '\0' )
  {
    send_to_char( "Bash what?\r\n", ch );
    return;
  }

  if( ch->fighting )
  {
    send_to_char( "You can't break off your fight.\r\n", ch );
    return;
  }

  if( ( pexit = find_door( ch, arg, FALSE ) ) != NULL )
  {
    ROOM_INDEX_DATA *to_room = NULL;
    EXIT_DATA *pexit_rev = NULL;
    int chance = 0;
    const char *keyword = NULL;

    if( !IS_SET( pexit->exit_info, EX_CLOSED ) )
    {
      send_to_char( "Calm down.  It is already open.\r\n", ch );
      return;
    }

    WAIT_STATE( ch, skill_table[gsn_bashdoor]->beats );

    if( IS_SET( pexit->exit_info, EX_SECRET ) )
      keyword = "wall";
    else
      keyword = pexit->keyword;

    chance = IS_NPC( ch ) ? 90 : character_skill_level( ch, gsn_bashdoor );

    if( !IS_SET( pexit->exit_info, EX_BASHPROOF )
	&& ch->move >= 15
	&& number_percent() <
	( chance + 4 * ( get_curr_str( ch ) - 19 ) ) )
    {
      REMOVE_BIT( pexit->exit_info, EX_CLOSED );
      if( IS_SET( pexit->exit_info, EX_LOCKED ) )
	REMOVE_BIT( pexit->exit_info, EX_LOCKED );
      SET_BIT( pexit->exit_info, EX_BASHED );

      act( AT_SKILL, "Crash!  You bashed open the $d!", ch, NULL, keyword,
	  TO_CHAR );
      act( AT_SKILL, "$n bashes open the $d!", ch, NULL, keyword,
	  TO_ROOM );
      learn_from_success( ch, gsn_bashdoor );

      if( ( to_room = pexit->to_room ) != NULL
	  && ( pexit_rev = pexit->rexit ) != NULL
	  && pexit_rev->to_room == ch->in_room )
      {
	CHAR_DATA *rch;

	REMOVE_BIT( pexit_rev->exit_info, EX_CLOSED );
	if( IS_SET( pexit_rev->exit_info, EX_LOCKED ) )
	  REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
	SET_BIT( pexit_rev->exit_info, EX_BASHED );

	for( rch = to_room->first_person; rch; rch = rch->next_in_room )
	{
	  act( AT_SKILL, "The $d crashes open!",
	      rch, NULL, pexit_rev->keyword, TO_CHAR );
	}
      }

      damage( ch, ch, ( ch->max_hit / 20 ), gsn_bashdoor );
    }
    else
    {
      act( AT_SKILL,
	  "WHAAAAM!!!  You bash against the $d, but it doesn't budge.",
	  ch, NULL, keyword, TO_CHAR );
      act( AT_SKILL,
	  "WHAAAAM!!!  $n bashes against the $d, but it holds strong.",
	  ch, NULL, keyword, TO_ROOM );
      damage( ch, ch, ( ch->max_hit / 20 ) + 10, gsn_bashdoor );
      learn_from_failure( ch, gsn_bashdoor );
    }
  }
  else
  {
    act( AT_SKILL,
	"WHAAAAM!!!  You bash against the wall, but it doesn't budge.", ch,
	NULL, NULL, TO_CHAR );
    act( AT_SKILL,
	"WHAAAAM!!!  $n bashes against the wall, but it holds strong.", ch,
	NULL, NULL, TO_ROOM );
    damage( ch, ch, ( ch->max_hit / 20 ) + 10, gsn_bashdoor );
    learn_from_failure( ch, gsn_bashdoor );
  }
}


void do_stand( CHAR_DATA * ch, char *argument )
{
  switch ( ch->position )
  {
    case POS_SLEEPING:
      if( IS_AFFECTED( ch, AFF_SLEEP ) )
      {
	send_to_char( "You can't seem to wake up!\r\n", ch );
	return;
      }

      send_to_char( "You wake and climb quickly to your feet.\r\n", ch );
      act( AT_ACTION, "$n arises from $s slumber.", ch, NULL, NULL, TO_ROOM );
      ch->position = POS_STANDING;
      break;

    case POS_RESTING:
      send_to_char( "You gather yourself and stand up.\r\n", ch );
      act( AT_ACTION, "$n rises from $s rest.", ch, NULL, NULL, TO_ROOM );
      ch->position = POS_STANDING;
      break;

    case POS_SITTING:
      send_to_char( "You move quickly to your feet.\r\n", ch );
      act( AT_ACTION, "$n rises up.", ch, NULL, NULL, TO_ROOM );
      ch->position = POS_STANDING;
      break;

    case POS_STANDING:
      send_to_char( "You are already standing.\r\n", ch );
      break;

    case POS_FIGHTING:
      send_to_char( "You are already fighting!\r\n", ch );
      break;
  }
}

void do_sit( CHAR_DATA * ch, char *argument )
{
  switch ( ch->position )
  {
    case POS_SLEEPING:
      if( IS_AFFECTED( ch, AFF_SLEEP ) )
      {
	send_to_char( "You can't seem to wake up!\r\n", ch );
	return;
      }

      send_to_char( "You wake and sit up.\r\n", ch );
      act( AT_ACTION, "$n wakes and sits up.", ch, NULL, NULL, TO_ROOM );
      ch->position = POS_SITTING;
      break;

    case POS_RESTING:
      send_to_char( "You stop resting and sit up.\r\n", ch );
      act( AT_ACTION, "$n stops resting and sits up.", ch, NULL, NULL,
	  TO_ROOM );
      ch->position = POS_SITTING;
      break;

    case POS_STANDING:
      send_to_char( "You sit down.\r\n", ch );
      act( AT_ACTION, "$n sits down.", ch, NULL, NULL, TO_ROOM );
      ch->position = POS_SITTING;
      break;
    case POS_SITTING:
      send_to_char( "You are already sitting.\r\n", ch );
      return;

    case POS_FIGHTING:
      send_to_char( "You are busy fighting!\r\n", ch );
      return;
    case POS_MOUNTED:
      send_to_char( "You are already sitting - on your mount.\r\n", ch );
      return;
  }
}

void do_rest( CHAR_DATA * ch, char *argument )
{
  switch ( ch->position )
  {
    case POS_SLEEPING:
      if( IS_AFFECTED( ch, AFF_SLEEP ) )
      {
	send_to_char( "You can't seem to wake up!\r\n", ch );
	return;
      }

      send_to_char( "You rouse from your slumber.\r\n", ch );
      act( AT_ACTION, "$n rouses from $s slumber.", ch, NULL, NULL, TO_ROOM );
      ch->position = POS_RESTING;
      break;

    case POS_RESTING:
      send_to_char( "You are already resting.\r\n", ch );
      return;

    case POS_STANDING:
      send_to_char( "You sprawl out haphazardly.\r\n", ch );
      act( AT_ACTION, "$n sprawls out haphazardly.", ch, NULL, NULL,
	  TO_ROOM );
      ch->position = POS_RESTING;
      break;

    case POS_SITTING:
      send_to_char( "You lie back and sprawl out to rest.\r\n", ch );
      act( AT_ACTION, "$n lies back and sprawls out to rest.", ch, NULL, NULL,
	  TO_ROOM );
      ch->position = POS_RESTING;
      break;

    case POS_FIGHTING:
      send_to_char( "You are busy fighting!\r\n", ch );
      return;
    case POS_MOUNTED:
      send_to_char( "You'd better dismount first.\r\n", ch );
      return;
  }

  rprog_rest_trigger( ch );
}

void do_sleep( CHAR_DATA * ch, char *argument )
{
  switch ( ch->position )
  {
    case POS_SLEEPING:
      send_to_char( "You are already sleeping.\r\n", ch );
      return;

    case POS_RESTING:
      if( ch->mental_state > 30
	  && ( number_percent() + 10 ) < ch->mental_state )
      {
	send_to_char
	  ( "You just can't seem to calm yourself down enough to sleep.\r\n",
	    ch );
	act( AT_ACTION,
	    "$n closes $s eyes for a few moments, but just can't seem to go to sleep.",
	    ch, NULL, NULL, TO_ROOM );
	return;
      }
      send_to_char( "You close your eyes and drift into slumber.\r\n", ch );
      act( AT_ACTION, "$n closes $s eyes and drifts into a deep slumber.", ch,
	  NULL, NULL, TO_ROOM );
      ch->position = POS_SLEEPING;
      break;

    case POS_SITTING:
      if( ch->mental_state > 30
	  && ( number_percent() + 5 ) < ch->mental_state )
      {
	send_to_char
	  ( "You just can't seem to calm yourself down enough to sleep.\r\n",
	    ch );
	act( AT_ACTION,
	    "$n closes $s eyes for a few moments, but just can't seem to go to sleep.",
	    ch, NULL, NULL, TO_ROOM );
	return;
      }
      send_to_char( "You slump over and fall dead asleep.\r\n", ch );
      act( AT_ACTION, "$n nods off and slowly slumps over, dead asleep.", ch,
	  NULL, NULL, TO_ROOM );
      ch->position = POS_SLEEPING;
      break;

    case POS_STANDING:
      if( ch->mental_state > 30 && number_percent() < ch->mental_state )
      {
	send_to_char
	  ( "You just can't seem to calm yourself down enough to sleep.\r\n",
	    ch );
	act( AT_ACTION,
	    "$n closes $s eyes for a few moments, but just can't seem to go to sleep.",
	    ch, NULL, NULL, TO_ROOM );
	return;
      }
      send_to_char( "You collapse into a deep sleep.\r\n", ch );
      act( AT_ACTION, "$n collapses into a deep sleep.", ch, NULL, NULL,
	  TO_ROOM );
      ch->position = POS_SLEEPING;
      break;

    case POS_FIGHTING:
      send_to_char( "You are busy fighting!\r\n", ch );
      return;
    case POS_MOUNTED:
      send_to_char( "You really should dismount first.\r\n", ch );
      return;
  }

  rprog_sleep_trigger( ch );
}

void do_wake( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim = NULL;

  one_argument( argument, arg );
  if( arg[0] == '\0' )
  {
    do_stand( ch, argument );
    return;
  }

  if( !IS_AWAKE( ch ) )
  {
    send_to_char( "You are asleep yourself!\r\n", ch );
    return;
  }

  if( ( victim = get_char_room( ch, arg ) ) == NULL )
  {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }

  if( IS_AWAKE( victim ) )
  {
    act( AT_PLAIN, "$N is already awake.", ch, NULL, victim, TO_CHAR );
    return;
  }

  if( IS_AFFECTED( victim, AFF_SLEEP ) || victim->position < POS_SLEEPING )
  {
    act( AT_PLAIN, "You can't seem to wake $M!", ch, NULL, victim,
	TO_CHAR );
    return;
  }

  act( AT_ACTION, "You wake $M.", ch, NULL, victim, TO_CHAR );
  victim->position = POS_STANDING;
  act( AT_ACTION, "$n wakes you.", ch, NULL, victim, TO_VICT );
}

/*
 * teleport a character to another room
 */
void teleportch( CHAR_DATA * ch, ROOM_INDEX_DATA * room, bool show )
{
  if( room_is_private( ch, room ) )
    return;
  act( AT_ACTION, "$n disappears suddenly!", ch, NULL, NULL, TO_ROOM );
  char_from_room( ch );
  char_to_room( ch, room );
  act( AT_ACTION, "$n arrives suddenly!", ch, NULL, NULL, TO_ROOM );

  if( show )
    do_look( ch, STRLIT_AUTO );
}

void teleport( CHAR_DATA * ch, short room, int flags )
{
  CHAR_DATA *nch = NULL, *nch_next = NULL;
  ROOM_INDEX_DATA *pRoomIndex = get_room_index( room );
  bool show = FALSE;

  if( !pRoomIndex )
  {
    bug( "teleport: bad room vnum %d", room );
    return;
  }

  if( IS_SET( flags, TELE_SHOWDESC ) )
    show = TRUE;
  else
    show = FALSE;

  if( !IS_SET( flags, TELE_TRANSALL ) )
  {
    teleportch( ch, pRoomIndex, show );
    return;
  }
  for( nch = ch->in_room->first_person; nch; nch = nch_next )
  {
    nch_next = nch->next_in_room;
    teleportch( nch, pRoomIndex, show );
  }
}

/*
 * "Climb" in a certain direction.				-Thoric
 */
void do_climb( CHAR_DATA * ch, char *argument )
{
  EXIT_DATA *pexit = NULL;

  if( argument[0] == '\0' )
  {
    for( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
      if( IS_SET( pexit->exit_info, EX_xCLIMB ) )
      {
	move_char( ch, pexit, 0 );
	return;
      }
    send_to_char( "You cannot climb here.\r\n", ch );
    return;
  }

  if( ( pexit = find_door( ch, argument, TRUE ) ) != NULL
      && IS_SET( pexit->exit_info, EX_xCLIMB ) )
  {
    move_char( ch, pexit, 0 );
    return;
  }
  send_to_char( "You cannot climb there.\r\n", ch );
}

/*
 * "enter" something (moves through an exit)			-Thoric
 */
void do_enter( CHAR_DATA * ch, char *argument )
{
  EXIT_DATA *pexit = NULL;

  if( argument[0] == '\0' )
  {
    for( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
      if( IS_SET( pexit->exit_info, EX_xENTER ) )
      {
	move_char( ch, pexit, 0 );
	return;
      }
    send_to_char( "You cannot find an entrance here.\r\n", ch );
    return;
  }

  if( ( pexit = find_door( ch, argument, TRUE ) ) != NULL
      && IS_SET( pexit->exit_info, EX_xENTER ) )
  {
    move_char( ch, pexit, 0 );
    return;
  }
  do_board( ch, argument );
}

/*
 * Leave through an exit.					-Thoric
 */
void do_leave( CHAR_DATA * ch, char *argument )
{
  EXIT_DATA *pexit = NULL;

  if( argument[0] == '\0' )
  {
    for( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
      if( IS_SET( pexit->exit_info, EX_xLEAVE ) )
      {
	move_char( ch, pexit, 0 );
	return;
      }
    do_leaveship( ch, STRLIT_EMPTY );
    return;
  }

  if( ( pexit = find_door( ch, argument, TRUE ) ) != NULL
      && IS_SET( pexit->exit_info, EX_xLEAVE ) )
  {
    move_char( ch, pexit, 0 );
    return;
  }
  do_leaveship( ch, STRLIT_EMPTY );
}
