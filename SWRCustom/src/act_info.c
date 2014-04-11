#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "mud.h"

/*
 * Local functions.
 */
void show_char_to_char_0( CHAR_DATA * victim, CHAR_DATA * ch );
void show_char_to_char_1( CHAR_DATA * victim, CHAR_DATA * ch );
void show_char_to_char( CHAR_DATA * list, CHAR_DATA * ch );
void show_ships_to_char( SHIP_DATA * ship, CHAR_DATA * ch );
bool check_blind( CHAR_DATA * ch );
void show_condition( CHAR_DATA * ch, CHAR_DATA * victim );
bool is_online( const char *argument );

char *format_obj_to_char( const OBJ_DATA * obj, CHAR_DATA * ch, bool fShort )
{
  static char buf[MAX_STRING_LENGTH];

  buf[0] = '\0';

  if( IS_OBJ_STAT( obj, ITEM_INVIS ) )
    strcat( buf, "(Invis) " );

  if( ( IS_AFFECTED( ch, AFF_DETECT_MAGIC ) || IS_IMMORTAL( ch ) )
      && IS_OBJ_STAT( obj, ITEM_MAGIC ) )
    strcat( buf, "&B(Blue Aura)&w " );

  if( IS_OBJ_STAT( obj, ITEM_GLOW ) )
    strcat( buf, "(Glowing) " );

  if( IS_OBJ_STAT( obj, ITEM_HUM ) )
    strcat( buf, "(Humming) " );

  if( IS_OBJ_STAT( obj, ITEM_HIDDEN ) )
    strcat( buf, "(Hidden) " );

  if( IS_OBJ_STAT( obj, ITEM_BURRIED ) )
    strcat( buf, "(Burried) " );

  if( IS_IMMORTAL( ch ) && IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
    strcat( buf, "(PROTO) " );

  if( fShort )
  {
    if( obj->short_descr )
      strcat( buf, obj->short_descr );
  }
  else
  {
    if( obj->description )
      strcat( buf, obj->description );
  }

  return buf;
}

/*
 * Some increasingly freaky halucinated objects		-Thoric
 */
const char *halucinated_object( int ms, bool fShort )
{
  int sms = URANGE( 1, ( ms + 10 ) / 5, halucinated_object_short_size() );

  if( fShort )
  {
    return
      halucinated_object_short[number_range
      ( 6 - URANGE( 1, sms / 2, 5 ), sms )];
  }

  return
    halucinated_object_long[number_range( 6 - URANGE( 1, sms / 2, 5 ), sms )];
}

/*
 * Show a list to a character.
 * Can coalesce duplicated items.
 */
void show_list_to_char( const OBJ_DATA * list, CHAR_DATA * ch,
			bool fShort, bool fShowNothing )
{
  char **prgpstrShow = NULL;
  int *prgnShow = NULL;
  int *pitShow = NULL;
  char *pstrShow = NULL;
  const OBJ_DATA *obj;
  int nShow = 0;
  int iShow = 0;
  int count = 0, offcount = 0, tmp = 0, ms = 0, cnt = 0;
  bool fCombine = FALSE;

  if( !ch->desc )
    return;

  /*
   * if there's no list... then don't do all this crap!  -Thoric
   */
  if( !list )
  {
    if( fShowNothing )
    {
      if( IS_NPC( ch ) || IS_SET( ch->act, PLR_COMBINE ) )
	send_to_char( "     ", ch );
      send_to_char( "Nothing.\r\n", ch );
    }

    return;
  }

  /*
   * Alloc space for output lines.
   */
  count = 0;
  for( obj = list; obj; obj = obj->next_content )
    count++;

  ms = ( ch->mental_state ? ch->mental_state : 1 )
    *
    ( IS_NPC( ch ) ? 1
      : ( ch->pcdata->
	condition[COND_DRUNK] ? ( ch->pcdata->condition[COND_DRUNK] /
	  12 ) : 1 ) );

  /*
   * If not mentally stable...
   */
  if( abs( ms ) > 40 )
  {
    offcount = URANGE( -( count ), ( count * ms ) / 100, count * 2 );

    if( offcount < 0 )
      offcount += number_range( 0, abs( offcount ) );
    else if( offcount > 0 )
      offcount -= number_range( 0, offcount );
  }
  else
    offcount = 0;

  if( count + offcount <= 0 )
  {
    if( fShowNothing )
    {
      if( IS_NPC( ch ) || IS_SET( ch->act, PLR_COMBINE ) )
	send_to_char( "     ", ch );

      send_to_char( "Nothing.\r\n", ch );
    }

    return;
  }

  CREATE( prgpstrShow, char *, count + ( ( offcount > 0 ) ? offcount : 0 ) );
  CREATE( prgnShow, int, count + ( ( offcount > 0 ) ? offcount : 0 ) );
  CREATE( pitShow, int, count + ( ( offcount > 0 ) ? offcount : 0 ) );
  nShow = 0;
  tmp = ( offcount > 0 ) ? offcount : 0;
  cnt = 0;

  /*
   * Format the list of objects.
   */
  for( obj = list; obj; obj = obj->next_content )
  {
    if( offcount < 0 && ++cnt > ( count + offcount ) )
      break;

    if( tmp > 0 && number_bits( 1 ) == 0 )
    {
      prgpstrShow[nShow] = str_dup( halucinated_object( ms, fShort ) );
      prgnShow[nShow] = 1;
      pitShow[nShow] = number_range( 0, MAX_ITEM_TYPE );
      nShow++;
      --tmp;
    }

    if( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj ) )
    {
      pstrShow = format_obj_to_char( obj, ch, fShort );
      fCombine = FALSE;

      if( IS_NPC( ch ) || IS_SET( ch->act, PLR_COMBINE ) )
      {
	/*
	 * Look for duplicates, case sensitive.
	 * Matches tend to be near end so run loop backwords.
	 */
	for( iShow = nShow - 1; iShow >= 0; iShow-- )
	{
	  if( !strcmp( prgpstrShow[iShow], pstrShow ) )
	  {
	    prgnShow[iShow] += obj->count;
	    fCombine = TRUE;
	    break;
	  }
	}
      }

      pitShow[nShow] = obj->item_type;
      /*
       * Couldn't combine, or didn't want to.
       */
      if( !fCombine )
      {
	prgpstrShow[nShow] = str_dup( pstrShow );
	prgnShow[nShow] = obj->count;
	nShow++;
      }
    }
  }

  if( tmp > 0 )
  {
    int x = 0;

    for( x = 0; x < tmp; x++ )
    {
      prgpstrShow[nShow] = str_dup( halucinated_object( ms, fShort ) );
      prgnShow[nShow] = 1;
      pitShow[nShow] = number_range( 0, MAX_ITEM_TYPE );
      nShow++;
    }
  }

  /*
   * Output the formatted list.         -Color support by Thoric
   */
  for( iShow = 0; iShow < nShow; iShow++ )
  {
    switch ( pitShow[iShow] )
    {
      default:
	set_char_color( AT_OBJECT, ch );
	break;

      case ITEM_MONEY:
	set_char_color( AT_YELLOW, ch );
	break;

      case ITEM_FOOD:
	set_char_color( AT_HUNGRY, ch );
	break;

      case ITEM_DRINK_CON:
      case ITEM_FOUNTAIN:
	set_char_color( AT_THIRSTY, ch );
	break;
    }

    if( fShowNothing )
      send_to_char( "     ", ch );

    send_to_char( prgpstrShow[iShow], ch );
    /*        if ( IS_NPC(ch) || IS_SET(ch->act, PLR_COMBINE) ) */
    {
      if( prgnShow[iShow] != 1 )
	ch_printf( ch, " (%d)", prgnShow[iShow] );
    }

    send_to_char( "\r\n", ch );
    DISPOSE( prgpstrShow[iShow] );
  }

  if( fShowNothing && nShow == 0 )
  {
    if( IS_NPC( ch ) || IS_SET( ch->act, PLR_COMBINE ) )
      send_to_char( "     ", ch );

    send_to_char( "Nothing.\r\n", ch );
  }

  /*
   * Clean up.
   */
  DISPOSE( prgpstrShow );
  DISPOSE( prgnShow );
  DISPOSE( pitShow );
}

/*
 * Show fancy descriptions for certain spell affects		-Thoric
 */
void show_visible_affects_to_char( CHAR_DATA * victim, CHAR_DATA * ch )
{
  if( IS_AFFECTED( victim, AFF_SANCTUARY ) )
  {
    if( IS_GOOD( victim ) )
    {
      set_char_color( AT_WHITE, ch );
      ch_printf( ch, "%s glows with an aura of divine radiance.\r\n",
	  IS_NPC( victim ) ? capitalize( victim->
	    short_descr ) : ( victim->
	      name ) );
    }
    else if( IS_EVIL( victim ) )
    {
      set_char_color( AT_WHITE, ch );
      ch_printf( ch, "%s shimmers beneath an aura of dark energy.\r\n",
	  IS_NPC( victim ) ? capitalize( victim->
	    short_descr ) : ( victim->
	      name ) );
    }
    else
    {
      set_char_color( AT_WHITE, ch );
      ch_printf( ch, "%s is shrouded in flowing shadow and light.\r\n",
	  IS_NPC( victim ) ? capitalize( victim->
	    short_descr ) : ( victim->
	      name ) );
    }
  }

  if( IS_AFFECTED( victim, AFF_FIRESHIELD ) )
  {
    set_char_color( AT_FIRE, ch );
    ch_printf( ch, "%s is engulfed within a blaze of mystical flame.\r\n",
	IS_NPC( victim ) ? capitalize( victim->
	  short_descr ) : ( victim->
	    name ) );
  }

  if( IS_AFFECTED( victim, AFF_SHOCKSHIELD ) )
  {
    set_char_color( AT_BLUE, ch );
    ch_printf( ch, "%s is surrounded by cascading torrents of energy.\r\n",
	IS_NPC( victim ) ? capitalize( victim->
	  short_descr ) : ( victim->
	    name ) );
  }

  /*Scryn 8/13*/
  if( IS_AFFECTED( victim, AFF_ICESHIELD ) )
  {
    set_char_color( AT_LBLUE, ch );
    ch_printf( ch, "%s is ensphered by shards of glistening ice.\r\n",
	IS_NPC( victim ) ? capitalize( victim->
	  short_descr ) : ( victim->
	    name ) );
  }

  if( IS_AFFECTED( victim, AFF_CHARM ) )
  {
    set_char_color( AT_MAGIC, ch );
    ch_printf( ch, "%s looks ahead free of expression.\r\n",
	IS_NPC( victim ) ? capitalize( victim->
	  short_descr ) : ( victim->
	    name ) );
  }

  if( !IS_NPC( victim ) && !victim->desc
      && victim->switched && IS_AFFECTED( victim->switched, AFF_POSSESS ) )
  {
    set_char_color( AT_MAGIC, ch );
    ch_printf( ch, "%s appears to be in a deep trance...\r\n",
	       PERS( victim, ch ) );
  }
}

static void show_char_position_dead( BUFFER *buf )
{
  buffer_strcat( buf, " is DEAD!!" );
}

static void show_char_position_mortal( BUFFER *buf )
{
  buffer_strcat( buf, " is mortally wounded." );
}

static void show_char_position_incap( BUFFER *buf )
{
  buffer_strcat( buf, " is incapacitated." );
}

static void show_char_position_stunned( BUFFER *buf )
{
  buffer_strcat( buf, " is lying here stunned." );
}

static void show_char_position_sleeping( BUFFER *buf, const CHAR_DATA *ch )
{
  if( ch->position == POS_SITTING || ch->position == POS_RESTING )
    buffer_strcat( buf, " is sleeping nearby." );
  else
    buffer_strcat( buf, " is deep in slumber here." );
}

static void show_char_position_resting( BUFFER *buf, const CHAR_DATA * ch )
{
  if( ch->position == POS_RESTING )
    buffer_strcat( buf, " is sprawled out alongside you." );
  else if( ch->position == POS_MOUNTED )
    buffer_strcat( buf, " is sprawled out at the foot of your mount." );
  else
    buffer_strcat( buf, " is sprawled out here." );
}

static void show_char_position_sitting( BUFFER *buf, const CHAR_DATA * ch )
{
  if( ch->position == POS_SITTING )
    buffer_strcat( buf, " sits here with you." );
  else if( ch->position == POS_RESTING )
    buffer_strcat( buf, " sits nearby as you lie around." );
  else
    buffer_strcat( buf, " sits upright here." );
}

static void show_char_position_standing( BUFFER *buf, const CHAR_DATA * ch,
					 const CHAR_DATA * victim )
{
  if( IS_IMMORTAL( victim ) )
    buffer_strcat( buf, " is here before you." );
  else if( ( victim->in_room->sector_type == SECT_UNDERWATER )
	   && !IS_AFFECTED( victim, AFF_AQUA_BREATH ) && !IS_NPC( victim ) )
    buffer_strcat( buf, " is drowning here." );
  else if( victim->in_room->sector_type == SECT_UNDERWATER )
    buffer_strcat( buf, " is here in the water." );
  else if( ( victim->in_room->sector_type == SECT_OCEANFLOOR )
	   && !IS_AFFECTED( victim, AFF_AQUA_BREATH ) && !IS_NPC( victim ) )
    buffer_strcat( buf, " is drowning here." );
  else if( victim->in_room->sector_type == SECT_OCEANFLOOR )
    buffer_strcat( buf, " is standing here in the water." );
  else if( IS_AFFECTED( victim, AFF_FLOATING )
	   || IS_AFFECTED( victim, AFF_FLYING ) )
    buffer_strcat( buf, " is hovering here." );
  else
    buffer_strcat( buf, " is standing here." );
}

static void show_char_position_shove( BUFFER *buf )
{
  buffer_strcat( buf, " is being shoved around." );
}

static void show_char_position_drag( BUFFER *buf )
{
  buffer_strcat( buf, " is being dragged around." );
}

static void show_char_position_mounted( BUFFER *buf, const CHAR_DATA * ch,
					const CHAR_DATA * victim )
{
  buffer_strcat( buf, " is here, upon " );

  if( !victim->mount )
    buffer_strcat( buf, "thin air???" );
  else if( victim->mount == ch )
    buffer_strcat( buf, "your back." );
  else if( victim->in_room == victim->mount->in_room )
  {
    buffer_strcat( buf, PERS( victim->mount, ch ) );
    buffer_strcat( buf, "." );
  }
  else
    buffer_strcat( buf, "someone who left??" );
}

static void show_char_position_fighting( BUFFER *buf, const CHAR_DATA * ch,
					 const CHAR_DATA * victim )
{
  buffer_strcat( buf, " is here, fighting " );

  if( !victim->fighting )
    buffer_strcat( buf, "thin air???" );
  else if( who_fighting( victim ) == ch )
    buffer_strcat( buf, "YOU!" );
  else if( victim->in_room == victim->fighting->who->in_room )
  {
    buffer_strcat( buf, PERS( victim->fighting->who, ch ) );
    buffer_strcat( buf, "." );
  }
  else
    buffer_strcat( buf, "someone who left??" );
}

void show_char_to_char_0( CHAR_DATA * victim, CHAR_DATA * ch )
{
  BUFFER *buf = buffer_new( 512 );

  /*if( IS_NPC( victim ) )
    buffer_strcat( buf, " " );*/

  if( !IS_NPC( victim ) && !victim->desc )
    {
      if( !victim->switched )
	buffer_strcat( buf, "(Link Dead) " );
      else if( !IS_AFFECTED( victim->switched, AFF_POSSESS ) )
	buffer_strcat( buf, "(Switched) " );
    }

  if( !IS_NPC( victim ) && IS_SET( victim->act, PLR_AFK ) )
    buffer_strcat( buf, "[AFK] " );

  if( ( !IS_NPC( victim ) && IS_SET( victim->act, PLR_WIZINVIS ) )
      || ( IS_NPC( victim ) && IS_SET( victim->act, ACT_MOBINVIS ) ) )
    {
      char buf1[MAX_STRING_LENGTH];

      if( !IS_NPC( victim ) )
	snprintf( buf1, MAX_STRING_LENGTH, "(Invis %d) ",
		  victim->pcdata->wizinvis );
      else
	snprintf( buf1, MAX_STRING_LENGTH, "(Mobinvis %d) ", victim->mobinvis);

      buffer_strcat( buf, buf1 );
    }

  if( IS_AFFECTED( victim, AFF_INVISIBLE ) )
    buffer_strcat( buf, "(Invis) " );

  if( IS_AFFECTED( victim, AFF_HIDE ) )
    buffer_strcat( buf, "(Hide) " );

  if( IS_AFFECTED( victim, AFF_PASS_DOOR ) )
    buffer_strcat( buf, "(Translucent) " );

  if( IS_AFFECTED( victim, AFF_FAERIE_FIRE ) )
    buffer_strcat( buf, "&P(Pink Aura)&w " );

  if( IS_EVIL( victim ) && IS_AFFECTED( ch, AFF_DETECT_EVIL ) )
    buffer_strcat( buf, "&R(Red Aura)&w " );

  if( !IS_NPC( victim ) && IS_SET( victim->act, PLR_LITTERBUG ) )
    buffer_strcat( buf, "(LITTERBUG) " );

  if( IS_NPC( victim ) && IS_IMMORTAL( ch )
      && IS_SET( victim->act, ACT_PROTOTYPE ) )
    buffer_strcat( buf, "(PROTO) " );

  if( victim->desc && victim->desc->connected == CON_EDITING )
    buffer_strcat( buf, "(Writing) " );

  set_char_color( AT_PERSON, ch );

  if( victim->position == victim->defposition
      && victim->long_descr[0] != '\0' )
    {
      buffer_strcat( buf, victim->long_descr );
      send_to_char( buf->data, ch );
      show_visible_affects_to_char( victim, ch );
    }
  else
    {
      /*   strcat( buf, PERS( victim, ch ) );       old system of titles
       *    removed to prevent prepending of name to title     -Kuran  
       *
       *    But added back bellow so that you can see mobs too :P   -Durga 
       */
      if( !IS_NPC( victim ) && !IS_SET( ch->act, PLR_BRIEF ) )
	{
	  buffer_strcat( buf, victim->pcdata->title );
	}
      else
	{
	  char tmp[100];
	  strncpy( tmp, PERS( victim, ch ), 100 );
	  tmp[0] = UPPER( tmp[0] );
	  buffer_strcat( buf, tmp );
	}

      switch ( victim->position )
	{
	case POS_DEAD:
	  show_char_position_dead( buf );
	  break;

	case POS_MORTAL:
	  show_char_position_mortal( buf );
	  break;

	case POS_INCAP:
	  show_char_position_incap( buf );
	  break;

	case POS_STUNNED:
	  show_char_position_stunned( buf );
	  break;

	case POS_SLEEPING:
	  show_char_position_sleeping( buf, ch );
	  break;

	case POS_RESTING:
	  show_char_position_resting( buf, ch );
	  break;

	case POS_SITTING:
	  show_char_position_sitting( buf, ch );
	  break;

	case POS_STANDING:
	  show_char_position_standing( buf, ch, victim );
	  break;

	case POS_SHOVE:
	  show_char_position_shove( buf );
	  break;

	case POS_DRAG:
	  show_char_position_drag( buf );
	  break;

	case POS_MOUNTED:
	  show_char_position_mounted( buf, ch, victim );
	  break;

	case POS_FIGHTING:
	  show_char_position_fighting( buf, ch, victim );
	  break;
	}

      buffer_strcat( buf, "\r\n" );
      buf->data[0] = UPPER( buf->data[0] );
      send_to_char( buf->data, ch );
      show_visible_affects_to_char( victim, ch );
    }

  buffer_free( buf );
}

void show_char_to_char_1( CHAR_DATA * victim, CHAR_DATA * ch )
{
  OBJ_DATA *obj = NULL;
  int iWear = 0;
  bool found = FALSE;

  if( can_see( victim, ch ) )
  {
    act( AT_ACTION, "$n looks at you.", ch, NULL, victim, TO_VICT );
    act( AT_ACTION, "$n looks at $N.", ch, NULL, victim, TO_NOTVICT );
  }

  if( victim->description[0] != '\0' )
  {
    send_to_char( victim->description, ch );
  }
  else
  {
    act( AT_PLAIN, "You see nothing special about $M.", ch, NULL, victim,
	TO_CHAR );
  }

  show_condition( ch, victim );

  found = FALSE;
  for( iWear = 0; iWear < MAX_WEAR; iWear++ )
  {
    if( ( obj = get_eq_char( victim, iWear ) ) != NULL
	&& can_see_obj( ch, obj ) )
    {
      if( !found )
      {
	send_to_char( "\r\n", ch );
	act( AT_PLAIN, "$N is using:", ch, NULL, victim, TO_CHAR );
	found = TRUE;
      }
      send_to_char( where_name[iWear], ch );
      send_to_char( format_obj_to_char( obj, ch, TRUE ), ch );
      send_to_char( "\r\n", ch );
    }
  }

  /*
   * Crash fix here by Thoric
   */
  if( IS_NPC( ch ) || victim == ch
      || ( IS_IMMORTAL( victim ) && ( victim->top_level > ch->top_level ) ) )
    return;

  if( number_percent() < character_skill_level( ch, gsn_peek ) )
  {
    send_to_char( "\r\nYou peek at the inventory:\r\n", ch );
    show_list_to_char( victim->first_carrying, ch, TRUE, TRUE );
    learn_from_success( ch, gsn_peek );
  }
  else if( character_skill_level( ch, gsn_peek ) )
  {
    learn_from_failure( ch, gsn_peek );
  }
}

void show_char_to_char( CHAR_DATA * list, CHAR_DATA * ch )
{
  CHAR_DATA *rch = NULL;

  for( rch = list; rch; rch = rch->next_in_room )
  {
    if( rch == ch )
      continue;

    if( can_see( ch, rch ) )
    {
      show_char_to_char_0( rch, ch );
    }
    else if( room_is_dark( ch->in_room )
	&& IS_AFFECTED( rch, AFF_INFRARED ) )
    {
      set_char_color( AT_BLOOD, ch );
      send_to_char( "The red form of a living creature is here.\r\n",
	  ch );
    }
  }
}

void show_ships_to_char( SHIP_DATA * ship, CHAR_DATA * ch )
{
  SHIP_DATA *rship;

  send_to_char( "&C", ch );

  for( rship = ship; rship; rship = rship->next_in_room )
    {
      if( rship->owner && rship->owner[0] != '\0' )
	{
	  if( get_clan( rship->owner ) || is_online( rship->owner )
	      || is_online( rship->pilot ) || is_online( rship->copilot ) )
	    {
	      ch_printf( ch, "%s\r\n", rship->name );
	    }
	}
    }
}

bool check_blind( CHAR_DATA * ch )
{
  if( !IS_NPC( ch ) && IS_SET( ch->act, PLR_HOLYLIGHT ) )
    return TRUE;

  if( IS_AFFECTED( ch, AFF_TRUESIGHT ) )
    return TRUE;

  if( IS_AFFECTED( ch, AFF_BLIND ) )
  {
    send_to_char( "You can't see a thing!\r\n", ch );
    return FALSE;
  }

  return TRUE;
}

/*
 * Returns classical DIKU door direction based on text in arg	-Thoric
 */
int get_door( const char *arg )
{
  int door = DIR_SOMEWHERE;

  if( !str_cmp( arg, "n" ) || !str_cmp( arg, "north" ) )
    door = DIR_NORTH;
  else if( !str_cmp( arg, "e" ) || !str_cmp( arg, "east" ) )
    door = DIR_EAST;
  else if( !str_cmp( arg, "s" ) || !str_cmp( arg, "south" ) )
    door = DIR_SOUTH;
  else if( !str_cmp( arg, "w" ) || !str_cmp( arg, "west" ) )
    door = DIR_WEST;
  else if( !str_cmp( arg, "u" ) || !str_cmp( arg, "up" ) )
    door = DIR_UP;
  else if( !str_cmp( arg, "d" ) || !str_cmp( arg, "down" ) )
    door = DIR_DOWN;
  else if( !str_cmp( arg, "ne" ) || !str_cmp( arg, "northeast" ) )
    door = DIR_NORTHEAST;
  else if( !str_cmp( arg, "nw" ) || !str_cmp( arg, "northwest" ) )
    door = DIR_NORTHWEST;
  else if( !str_cmp( arg, "se" ) || !str_cmp( arg, "southeast" ) )
    door = DIR_SOUTHEAST;
  else if( !str_cmp( arg, "sw" ) || !str_cmp( arg, "southwest" ) )
    door = DIR_SOUTHWEST;
  else
    door = -1;

  return door;
}

void do_look( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  EXIT_DATA *pexit = NULL;
  CHAR_DATA *victim = NULL;
  OBJ_DATA *obj = NULL;
  ROOM_INDEX_DATA *original = NULL;
  const char *pdesc = NULL;
  bool doexaprog = FALSE;
  short door = 0;
  int number = 0, cnt = 0;

  if( !ch->desc )
    return;

  if( ch->position < POS_SLEEPING )
  {
    send_to_char( "You can't see anything but stars!\r\n", ch );
    return;
  }

  if( ch->position == POS_SLEEPING )
  {
    send_to_char( "You can't see anything, you're sleeping!\r\n", ch );
    return;
  }

  if( !check_blind( ch ) )
    return;

  if( !IS_NPC( ch )
      && !IS_SET( ch->act, PLR_HOLYLIGHT )
      && !IS_AFFECTED( ch, AFF_TRUESIGHT ) && room_is_dark( ch->in_room ) )
  {
    set_char_color( AT_DGREY, ch );
    send_to_char( "It is pitch black ... \r\n", ch );
    show_char_to_char( ch->in_room->first_person, ch );
    return;
  }

  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );
  argument = one_argument( argument, arg3 );

  doexaprog = str_cmp( "noprog", arg2 ) && str_cmp( "noprog", arg3 );

  if( arg1[0] == '\0' || !str_cmp( arg1, "auto" ) )
  {
    SHIP_DATA *ship = NULL;

    /* 'look' or 'look auto' */
    set_char_color( AT_RMNAME, ch );
    ch_printf( ch, "%s (%s) ", ch->in_room->name,
	       get_sector_name(ch->in_room->sector_type) );

    if( !ch->desc->original )
    {
      if( ( IS_IMMORTAL( ch ) )
	  && ( IS_SET( ch->pcdata->flags, PCFLAG_ROOM ) ) )
      {
	set_char_color( AT_BLUE, ch );	/* Added 10/17 by Kuran of */
	send_to_char( "{", ch );	/* SWReality */
	ch_printf( ch, "%d", ch->in_room->vnum );
	send_to_char( "}", ch );
	set_char_color( AT_CYAN, ch );
	send_to_char( "[", ch );
	send_to_char( flag_string( ch->in_room->room_flags, r_flags ),
	    ch );
	send_to_char( "]", ch );
      }
    }

    send_to_char( "\r\n", ch );
    set_char_color( AT_RMDESC, ch );

    if( arg1[0] == '\0'
	|| ( !IS_NPC( ch ) && !IS_SET( ch->act, PLR_BRIEF ) ) )
      send_to_char( ch->in_room->description, ch );

    if( !IS_NPC( ch ) && IS_SET( ch->act, PLR_AUTOEXIT ) )
      do_exits( ch, STRLIT_EMPTY );

    show_ships_to_char( ch->in_room->first_ship, ch );
    show_list_to_char( ch->in_room->first_content, ch, FALSE, FALSE );
    show_char_to_char( ch->in_room->first_person, ch );

    if( str_cmp( arg1, "auto" ) )
      if( ( ship = ship_from_cockpit( ch->in_room ) ) != NULL )
      {
	set_char_color( AT_WHITE, ch );
	ch_printf( ch,
	    "\r\nThrough the transparisteel windows you see:\r\n" );

	if( ship->starsystem )
	{
	  MISSILE_DATA *missile;
	  SHIP_DATA *target;
	  PLANET_DATA *planet;

	  set_char_color( AT_GREEN, ch );
	  if( ship->starsystem->star1
	      && str_cmp( ship->starsystem->star1, "" ) )
	    ch_printf( ch, "&YThe star, %s.\r\n",
		ship->starsystem->star1 );
	  if( ship->starsystem->star2
	      && str_cmp( ship->starsystem->star2, "" ) )
	    ch_printf( ch, "&YThe star, %s.\r\n",
		ship->starsystem->star2 );
	  for( planet = ship->starsystem->first_planet; planet;
	      planet = planet->next_in_system )
	    ch_printf( ch, "&GThe planet, %s.\r\n", planet->name );
	  for( target = ship->starsystem->first_ship; target;
	      target = target->next_in_starsystem )
	  {
	    if( target != ship )
	      ch_printf( ch, "&C%s\r\n", target->name );
	  }
	  for( missile = ship->starsystem->first_missile; missile;
	      missile = missile->next_in_starsystem )
	    ch_printf( ch, "&RA missile.\r\n" );
	}
	else if( ship->location == ship->lastdoc )
	{
	  ROOM_INDEX_DATA *to_room;

	  if( ( to_room = get_room_index( ship->location ) ) != NULL )
	  {
	    ch_printf( ch, "\r\n" );
	    original = ch->in_room;
	    char_from_room( ch );
	    char_to_room( ch, to_room );
	    do_glance( ch, STRLIT_EMPTY );
	    char_from_room( ch );
	    char_to_room( ch, original );
	  }
	}
      }

    return;
  }

  if( !str_cmp( arg1, "under" ) )
  {
    int count;

    /* 'look under' */
    if( arg2[0] == '\0' )
    {
      send_to_char( "Look beneath what?\r\n", ch );
      return;
    }

    if( ( obj = get_obj_here( ch, arg2 ) ) == NULL )
    {
      send_to_char( "You do not see that here.\r\n", ch );
      return;
    }
    if( ch->carry_weight + obj->weight > can_carry_w( ch ) )
    {
      send_to_char( "It's too heavy for you to look under.\r\n", ch );
      return;
    }
    count = obj->count;
    obj->count = 1;
    act( AT_PLAIN, "You lift $p and look beneath it:", ch, obj, NULL,
	TO_CHAR );
    act( AT_PLAIN, "$n lifts $p and looks beneath it:", ch, obj, NULL,
	TO_ROOM );
    obj->count = count;
    if( IS_OBJ_STAT( obj, ITEM_COVERING ) )
      show_list_to_char( obj->first_content, ch, TRUE, TRUE );
    else
      send_to_char( "Nothing.\r\n", ch );
    if( doexaprog )
      oprog_examine_trigger( ch, obj );
    return;
  }

  if( !str_cmp( arg1, "i" ) || !str_cmp( arg1, "in" ) )
  {
    int count;

    /* 'look in' */
    if( arg2[0] == '\0' )
    {
      send_to_char( "Look in what?\r\n", ch );
      return;
    }

    if( ( obj = get_obj_here( ch, arg2 ) ) == NULL )
    {
      send_to_char( "You do not see that here.\r\n", ch );
      return;
    }

    switch ( obj->item_type )
    {
      default:
	send_to_char( "That is not a container.\r\n", ch );
	break;

      case ITEM_DRINK_CON:
	if( obj->value[1] <= 0 )
	{
	  send_to_char( "It is empty.\r\n", ch );
	  if( doexaprog )
	    oprog_examine_trigger( ch, obj );
	  break;
	}

	ch_printf( ch, "It's %s full of a %s liquid.\r\n",
	    obj->value[1] < obj->value[0] / 4
	    ? "less than" :
	    obj->value[1] < 3 * obj->value[0] / 4
	    ? "about" : "more than",
	    liq_table[obj->value[2]].liq_color );

	if( doexaprog )
	  oprog_examine_trigger( ch, obj );
	break;

      case ITEM_CONTAINER:
      case ITEM_CORPSE_NPC:
      case ITEM_CORPSE_PC:
      case ITEM_DROID_CORPSE:
	if( IS_SET( obj->value[1], CONT_CLOSED ) )
	{
	  send_to_char( "It is closed.\r\n", ch );
	  break;
	}

	count = obj->count;
	obj->count = 1;
	act( AT_PLAIN, "$p contains:", ch, obj, NULL, TO_CHAR );
	obj->count = count;
	show_list_to_char( obj->first_content, ch, TRUE, TRUE );
	if( doexaprog )
	  oprog_examine_trigger( ch, obj );
	break;
    }
    return;
  }

  if( ( pdesc =
	get_extra_descr( arg1, ch->in_room->first_extradesc ) ) != NULL )
  {
    send_to_char( pdesc, ch );
    return;
  }

  door = get_door( arg1 );
  if( ( pexit = find_door( ch, arg1, TRUE ) ) != NULL )
  {
    if( pexit->keyword )
    {
      if( IS_SET( pexit->exit_info, EX_CLOSED )
	  && !IS_SET( pexit->exit_info, EX_WINDOW ) )
      {
	if( IS_SET( pexit->exit_info, EX_SECRET ) && door != -1 )
	  send_to_char( "Nothing special there.\r\n", ch );
	else
	  act( AT_PLAIN, "The $d is closed.", ch, NULL, pexit->keyword,
	      TO_CHAR );
	return;
      }
      if( IS_SET( pexit->exit_info, EX_BASHED ) )
	act( AT_RED, "The $d has been bashed from its hinges!", ch, NULL,
	    pexit->keyword, TO_CHAR );
    }

    if( pexit->description && pexit->description[0] != '\0' )
      send_to_char( pexit->description, ch );
    else
      send_to_char( "Nothing special there.\r\n", ch );

    /*
     * Ability to look into the next room                     -Thoric
     */
    if( pexit->to_room )
    {
      if( room_is_private( ch, pexit->to_room ) && !IS_IMMORTAL( ch ) )
      {
	set_char_color( AT_WHITE, ch );
	send_to_char( "That room is private buster!\r\n", ch );
	return;
      }
      original = ch->in_room;
      char_from_room( ch );
      char_to_room( ch, pexit->to_room );
      do_look( ch, STRLIT_AUTO );
      do_scan( ch, arg1 );
      char_from_room( ch );
      char_to_room( ch, original );
    }
    return;
  }
  else if( door != -1 )
  {
    send_to_char( "Nothing special there.\r\n", ch );
    return;
  }

  if( ( victim = get_char_room( ch, arg1 ) ) != NULL )
  {
    show_char_to_char_1( victim, ch );
    return;
  }

  /* finally fixed the annoying look 2.obj desc bug   -Thoric */
  number = number_argument( arg1, arg );
  for( cnt = 0, obj = ch->last_carrying; obj; obj = obj->prev_content )
  {
    if( can_see_obj( ch, obj ) )
    {
      if( ( pdesc =
	    get_extra_descr( arg, obj->first_extradesc ) ) != NULL )
      {
	if( ( cnt += obj->count ) < number )
	  continue;
	send_to_char( pdesc, ch );
	if( doexaprog )
	  oprog_examine_trigger( ch, obj );
	return;
      }

      if( ( pdesc = get_extra_descr( arg, obj->pIndexData->first_extradesc ) ) != NULL )
      {
	if( ( cnt += obj->count ) < number )
	  continue;
	send_to_char( pdesc, ch );
	if( doexaprog )
	  oprog_examine_trigger( ch, obj );
	return;
      }

      if( nifty_is_name_prefix( arg, obj->name ) )
      {
	if( ( cnt += obj->count ) < number )
	  continue;
	pdesc =
	  get_extra_descr( obj->name,
	      obj->pIndexData->first_extradesc );
	if( !pdesc )
	  pdesc = get_extra_descr( obj->name, obj->first_extradesc );
	if( !pdesc )
	  send_to_char( "You see nothing special.\r\n", ch );
	else
	  send_to_char( pdesc, ch );
	if( doexaprog )
	  oprog_examine_trigger( ch, obj );
	return;
      }
    }
  }

  for( obj = ch->in_room->last_content; obj; obj = obj->prev_content )
  {
    if( can_see_obj( ch, obj ) )
    {
      if( ( pdesc = get_extra_descr( arg, obj->first_extradesc ) ) != NULL )
      {
	if( ( cnt += obj->count ) < number )
	  continue;
	send_to_char( pdesc, ch );
	if( doexaprog )
	  oprog_examine_trigger( ch, obj );
	return;
      }

      if( ( pdesc = get_extra_descr( arg, obj->pIndexData->first_extradesc )))
      {
	if( ( cnt += obj->count ) < number )
	  continue;
	send_to_char( pdesc, ch );
	if( doexaprog )
	  oprog_examine_trigger( ch, obj );
	return;
      }

      if( nifty_is_name_prefix( arg, obj->name ) )
      {
	if( ( cnt += obj->count ) < number )
	  continue;
	pdesc =
	  get_extra_descr( obj->name,
	      obj->pIndexData->first_extradesc );
	if( !pdesc )
	  pdesc = get_extra_descr( obj->name, obj->first_extradesc );
	if( !pdesc )
	  send_to_char( "You see nothing special.\r\n", ch );
	else
	  send_to_char( pdesc, ch );
	if( doexaprog )
	  oprog_examine_trigger( ch, obj );
	return;
      }
    }
  }

  send_to_char( "You do not see that here.\r\n", ch );
}

void show_condition( CHAR_DATA * ch, CHAR_DATA * victim )
{
  char buf[MAX_STRING_LENGTH];
  int percent = 0;

  if( victim->max_hit > 0 )
    percent =
      ( int ) ( ( 100.0 * ( double ) ( victim->hit ) ) /
		( double ) ( victim->max_hit ) );
  else
    percent = -1;

  strcpy( buf, PERS( victim, ch ) );

  if( IS_NPC( victim ) && IS_SET( victim->act, ACT_DROID ) )
  {
    if( percent >= 100 )
      strcat( buf, " is in perfect condition.\r\n" );
    else if( percent >= 90 )
      strcat( buf, " is slightly scratched.\r\n" );
    else if( percent >= 80 )
      strcat( buf, " has a few scrapes.\r\n" );
    else if( percent >= 70 )
      strcat( buf, " has some dents.\r\n" );
    else if( percent >= 60 )
      strcat( buf, " has a couple holes in its plating.\r\n" );
    else if( percent >= 50 )
      strcat( buf, " has a many broken pieces.\r\n" );
    else if( percent >= 40 )
      strcat( buf, " has many exposed circuits.\r\n" );
    else if( percent >= 30 )
      strcat( buf, " is leaking oil.\r\n" );
    else if( percent >= 20 )
      strcat( buf, " has smoke coming out of it.\r\n" );
    else if( percent >= 10 )
      strcat( buf, " is almost completely broken.\r\n" );
    else
      strcat( buf, " is about to EXPLODE.\r\n" );
  }
  else
  {
    if( percent >= 100 )
      strcat( buf, " is in perfect health.\r\n" );
    else if( percent >= 90 )
      strcat( buf, " is slightly scratched.\r\n" );
    else if( percent >= 80 )
      strcat( buf, " has a few bruises.\r\n" );
    else if( percent >= 70 )
      strcat( buf, " has some cuts.\r\n" );
    else if( percent >= 60 )
      strcat( buf, " has several wounds.\r\n" );
    else if( percent >= 50 )
      strcat( buf, " has many nasty wounds.\r\n" );
    else if( percent >= 40 )
      strcat( buf, " is bleeding freely.\r\n" );
    else if( percent >= 30 )
      strcat( buf, " is covered in blood.\r\n" );
    else if( percent >= 20 )
      strcat( buf, " is leaking guts.\r\n" );
    else if( percent >= 10 )
      strcat( buf, " is almost dead.\r\n" );
    else
      strcat( buf, " is DYING.\r\n" );
  }

  buf[0] = UPPER( buf[0] );
  send_to_char( buf, ch );
}

/* A much simpler version of look, this function will show you only
   the condition of a mob or pc, or if used without an argument, the
   same you would see if you enter the room and have config +brief.
   -- Narn, winter '96
   */
void do_glance( CHAR_DATA * ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  int save_act;

  if( !ch->desc )
    return;

  if( ch->position < POS_SLEEPING )
  {
    send_to_char( "You can't see anything but stars!\r\n", ch );
    return;
  }

  if( ch->position == POS_SLEEPING )
  {
    send_to_char( "You can't see anything, you're sleeping!\r\n", ch );
    return;
  }

  if( !check_blind( ch ) )
    return;

  argument = one_argument( argument, arg1 );

  if( arg1[0] == '\0' )
  {
    save_act = ch->act;
    SET_BIT( ch->act, PLR_BRIEF );
    do_look( ch, STRLIT_AUTO );
    ch->act = save_act;
    return;
  }

  if( ( victim = get_char_room( ch, arg1 ) ) == NULL )
  {
    send_to_char( "They're not here.", ch );
    return;
  }
  else
  {
    if( can_see( victim, ch ) )
    {
      act( AT_ACTION, "$n glances at you.", ch, NULL, victim, TO_VICT );
      act( AT_ACTION, "$n glances at $N.", ch, NULL, victim, TO_NOTVICT );
    }

    show_condition( ch, victim );
    return;
  }
}

static void examine_obj_armor( CHAR_DATA * ch, OBJ_DATA * obj )
{
  int dam = 0;
  char buf[MAX_STRING_LENGTH];
  *buf = '\0';

  if( obj->value[1] == 0 )
    obj->value[1] = obj->value[0];

  if( obj->value[1] == 0 )
    obj->value[1] = 1;

  dam = ( obj->value[0] * 10 ) / obj->value[1];

  strcpy( buf, "As you look more closely, you notice that it is " );

  if( dam >= 10 )
    strcat( buf, "in superb condition." );
  else if( dam == 9 )
    strcat( buf, "in very good condition." );
  else if( dam == 8 )
    strcat( buf, "in good shape." );
  else if( dam == 7 )
    strcat( buf, "showing a bit of wear." );
  else if( dam == 6 )
    strcat( buf, "a little run down." );
  else if( dam == 5 )
    strcat( buf, "in need of repair." );
  else if( dam == 4 )
    strcat( buf, "in great need of repair." );
  else if( dam == 3 )
    strcat( buf, "in dire need of repair." );
  else if( dam == 2 )
    strcat( buf, "very badly worn." );
  else if( dam == 1 )
    strcat( buf, "practically worthless." );
  else if( dam <= 0 )
    strcat( buf, "broken." );

  strcat( buf, "\r\n" );
  send_to_char( buf, ch );
}

static void examine_obj_weapon( CHAR_DATA * ch, OBJ_DATA * obj )
{
  int dam = INIT_WEAPON_CONDITION - obj->value[0];
  char buf[MAX_STRING_LENGTH];
  *buf = '\0';

  strcpy( buf, "As you look more closely, you notice that it is " );

  if( dam == 0 )
    strcat( buf, "in superb condition." );
  else if( dam == 1 )
    strcat( buf, "in excellent condition." );
  else if( dam == 2 )
    strcat( buf, "in very good condition." );
  else if( dam == 3 )
    strcat( buf, "in good shape." );
  else if( dam == 4 )
    strcat( buf, "showing a bit of wear." );
  else if( dam == 5 )
    strcat( buf, "a little run down." );
  else if( dam == 6 )
    strcat( buf, "in need of repair." );
  else if( dam == 7 )
    strcat( buf, "in great need of repair." );
  else if( dam == 8 )
    strcat( buf, "in dire need of repair." );
  else if( dam == 9 )
    strcat( buf, "very badly worn." );
  else if( dam == 10 )
    strcat( buf, "practically worthless." );
  else if( dam == 11 )
    strcat( buf, "almost broken." );
  else if( dam == 12 )
    strcat( buf, "broken." );

  strcat( buf, "\r\n" );

  send_to_char( buf, ch );

  if( obj->value[3] == WEAPON_BLASTER )
  {
    if( obj->blaster_setting == BLASTER_FULL )
      ch_printf( ch, "It is set on FULL power.\r\n" );
    else if( obj->blaster_setting == BLASTER_HIGH )
      ch_printf( ch, "It is set on HIGH power.\r\n" );
    else if( obj->blaster_setting == BLASTER_NORMAL )
      ch_printf( ch, "It is set on NORMAL power.\r\n" );
    else if( obj->blaster_setting == BLASTER_HALF )
      ch_printf( ch, "It is set on HALF power.\r\n" );
    else if( obj->blaster_setting == BLASTER_LOW )
      ch_printf( ch, "It is set on LOW power.\r\n" );
    else if( obj->blaster_setting == BLASTER_STUN )
      ch_printf( ch, "It is set on STUN.\r\n" );
    ch_printf( ch, "It has from %d to %d shots remaining.\r\n",
	obj->value[4] / 5, obj->value[4] );
  }
  else if( ( obj->value[3] == WEAPON_LIGHTSABER ||
	obj->value[3] == WEAPON_VIBRO_BLADE ) )
  {
    ch_printf( ch, "It has %d/%d units of charge remaining.\r\n",
	obj->value[4], obj->value[5] );
  }
}

static void examine_obj_food( CHAR_DATA * ch, OBJ_DATA * obj )
{
  int dam = 0;
  char buf[MAX_STRING_LENGTH];
  *buf = '\0';

  if( obj->timer > 0 && obj->value[1] > 0 )
    dam = ( obj->timer * 10 ) / obj->value[1];
  else
    dam = 10;

  strcpy( buf, "As you examine it carefully you notice that it " );

  if( dam >= 10 )
    strcat( buf, "is fresh." );
  else if( dam == 9 )
    strcat( buf, "is nearly fresh." );
  else if( dam == 8 )
    strcat( buf, "is perfectly fine." );
  else if( dam == 7 )
    strcat( buf, "looks good." );
  else if( dam == 6 )
    strcat( buf, "looks ok." );
  else if( dam == 5 )
    strcat( buf, "is a little stale." );
  else if( dam == 4 )
    strcat( buf, "is a bit stale." );
  else if( dam == 3 )
    strcat( buf, "smells slightly off." );
  else if( dam == 2 )
    strcat( buf, "smells quite rank." );
  else if( dam == 1 )
    strcat( buf, "smells revolting." );
  else if( dam <= 0 )
    strcat( buf, "is crawling with maggots." );

  strcat( buf, "\r\n" );
  send_to_char( buf, ch );
}

static void examine_obj_corpse( CHAR_DATA * ch, OBJ_DATA * obj )
{
  short timerfrac = obj->timer;
  char buf[MAX_STRING_LENGTH];

  if( obj->item_type == ITEM_CORPSE_PC )
    timerfrac = ( int ) obj->timer / 8 + 1;

  switch ( timerfrac )
  {
    default:
      send_to_char( "This corpse has recently been slain.\r\n", ch );
      break;
    case 4:
      send_to_char( "This corpse was slain a little while ago.\r\n", ch );
      break;
    case 3:
      send_to_char
	( "A foul smell rises from the corpse, and it is covered in flies.\r\n",
	  ch );
      break;
    case 2:
      send_to_char
	( "A writhing mass of maggots and decay, you can barely go near this corpse.\r\n",
	  ch );
      break;
    case 1:
    case 0:
      send_to_char
	( "Little more than bones, there isn't much left of this corpse.\r\n",
	  ch );
      break;
  }

  if( IS_OBJ_STAT( obj, ITEM_COVERING ) )
    return;

  send_to_char( "When you look inside, you see:\r\n", ch );
  snprintf( buf, MAX_STRING_LENGTH, "in '%s' noprog", obj->name );
  do_look( ch, buf );
}

static void examine_obj_droid_corpse( CHAR_DATA * ch, OBJ_DATA * obj )
{
  short timerfrac = obj->timer;
  char buf[MAX_STRING_LENGTH];

  switch ( timerfrac )
  {
    default:
      send_to_char( "These remains are still smoking.\r\n", ch );
      break;

    case 4:
      send_to_char
	( "The parts of this droid have cooled down completely.\r\n", ch );
      break;

    case 3:
      send_to_char( "The broken droid components are beginning to rust.\r\n",
	  ch );
      break;

    case 2:
      send_to_char( "The pieces are completely covered in rust.\r\n", ch );
      break;

    case 1:
    case 0:
      send_to_char( "All that remains of it is a pile of crumbling rust.\r\n",
	  ch );
      break;
  }

  if( IS_OBJ_STAT( obj, ITEM_COVERING ) )
    return;

  send_to_char( "When you look inside, you see:\r\n", ch );
  snprintf( buf, MAX_STRING_LENGTH, "in '%s' noprog", obj->name );
  do_look( ch, buf );
}

static void examine_obj_drink_container( CHAR_DATA * ch, OBJ_DATA * obj )
{
  char buf[MAX_STRING_LENGTH];
  send_to_char( "When you look inside, you see:\r\n", ch );
  snprintf( buf, MAX_STRING_LENGTH, "in '%s' noprog", obj->name );
  do_look( ch, buf );
}

void do_examine( CHAR_DATA * ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  OBJ_DATA *obj = NULL;
  BOARD_DATA *board = NULL;

  if( !argument )
  {
    bug( "do_examine: null argument.", 0 );
    return;
  }

  if( !ch )
  {
    bug( "do_examine: null ch.", 0 );
    return;
  }

  one_argument( argument, arg );

  if( arg[0] == '\0' )
  {
    send_to_char( "Examine what?\r\n", ch );
    return;
  }

  snprintf( buf, MAX_STRING_LENGTH, "%s noprog", arg );
  do_look( ch, buf );

  /*
   * Support for looking at boards, checking equipment conditions,
   * and support for trigger positions by Thoric
   */
  if( ( obj = get_obj_here( ch, arg ) ) != NULL )
  {
    if( ( board = get_board( obj ) ) != NULL )
    {
      if( board->num_posts )
	ch_printf( ch,
	    "There are about %d notes posted here.  Type 'note list' to list them.\r\n",
	    board->num_posts );
      else
	send_to_char( "There aren't any notes posted here.\r\n", ch );
    }

    switch ( obj->item_type )
    {
      default:
	break;

      case ITEM_ARMOR:
	examine_obj_armor( ch, obj );
	break;

      case ITEM_WEAPON:
	examine_obj_weapon( ch, obj );
	break;

      case ITEM_FOOD:
	examine_obj_food( ch, obj );
	break;

      case ITEM_CORPSE_PC:
      case ITEM_CORPSE_NPC:
	examine_obj_corpse( ch, obj );
	break;

      case ITEM_DROID_CORPSE:
	examine_obj_droid_corpse( ch, obj );
	break;

      case ITEM_CONTAINER:
	if( IS_OBJ_STAT( obj, ITEM_COVERING ) )
	  break;

      case ITEM_DRINK_CON:
	examine_obj_drink_container( ch, obj );
	break;
    }

    if( IS_OBJ_STAT( obj, ITEM_COVERING ) )
    {
      snprintf( buf, MAX_STRING_LENGTH, "under %s noprog", arg );
      do_look( ch, buf );
    }

    oprog_examine_trigger( ch, obj );
    if( char_died( ch ) || obj_extracted( obj ) )
      return;
  }
}

void do_exits( CHAR_DATA * ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  EXIT_DATA *pexit = NULL;
  bool found = FALSE;
  bool fAuto = FALSE;

  set_char_color( AT_EXITS, ch );
  buf[0] = '\0';
  fAuto = !str_cmp( argument, "auto" );

  if( !check_blind( ch ) )
    return;

  strcpy( buf, fAuto ? "Exits:" : "Obvious exits:\r\n" );

  for( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
  {
    if( pexit->to_room && !IS_SET( pexit->exit_info, EX_HIDDEN ) )
    {
      found = TRUE;
      if( !fAuto )
      {
	if( IS_SET( pexit->exit_info, EX_CLOSED ) )
	{
	  snprintf( buf + strlen( buf ), MAX_STRING_LENGTH - strlen( buf ),
	      "%-5s - (closed)\r\n",
	      capitalize( dir_name[pexit->vdir] ) );
	}
	else if( IS_SET( pexit->exit_info, EX_WINDOW ) )
	{
	  snprintf( buf + strlen( buf ), MAX_STRING_LENGTH - strlen( buf ),
	      "%-5s - (window)\r\n",
	      capitalize( dir_name[pexit->vdir] ) );
	}
	else if( IS_SET( pexit->exit_info, EX_xAUTO ) )
	{
	  snprintf( buf + strlen( buf ), MAX_STRING_LENGTH - strlen( buf ),
	      "%-5s - %s\r\n",
	      capitalize( pexit->keyword ),
	      room_is_dark( pexit->to_room )
	      ? "Too dark to tell" : pexit->to_room->name );
	}
	else
	  snprintf( buf + strlen( buf ), MAX_STRING_LENGTH - strlen( buf ),
	      "%-5s - %s\r\n",
	      capitalize( dir_name[pexit->vdir] ),
	      room_is_dark( pexit->to_room )
	      ? "Too dark to tell" : pexit->to_room->name );
      }
      else
      {
	snprintf( buf + strlen( buf ), MAX_STRING_LENGTH - strlen( buf ), " %s",
	    capitalize( dir_name[pexit->vdir] ) );
      }
    }
  }

  if( !found )
    strcat( buf, fAuto ? " none.\r\n" : "None.\r\n" );
  else if( fAuto )
    strcat( buf, ".\r\n" );
  send_to_char( buf, ch );
}

void do_time( CHAR_DATA * ch, char *argument )
{
  extern char str_boot_time[];
  extern char reboot_time[];
  const char *suf;
  int day = time_info.day + 1;

  if( day > 4 && day < 20 )
    suf = "th";
  else if( day % 10 == 1 )
    suf = "st";
  else if( day % 10 == 2 )
    suf = "nd";
  else if( day % 10 == 3 )
    suf = "rd";
  else
    suf = "th";

  set_char_color( AT_YELLOW, ch );
  ch_printf( ch,
      "It is %d o'clock %s, Day of %s, %d%s the Month of %s.\r\n"
      "The mud started up at:    %s\r"
      "The system time (E.S.T.): %s\r"
      "Next Reboot is set for:   %s\r",
      ( time_info.hour % 12 == 0 ) ? 12 : time_info.hour % 12,
      time_info.hour >= 12 ? "pm" : "am",
      day_name[day % 7],
      day, suf,
      month_name[time_info.month],
      str_boot_time, ( char * ) ctime( &current_time ), reboot_time );
}

void do_weather( CHAR_DATA * ch, char *argument )
{
  static const char *const sky_look[4] = {
    "cloudless",
    "cloudy",
    "rainy",
    "lit by flashes of lightning"
  };

  if( !IS_OUTSIDE( ch ) )
  {
    send_to_char( "You can't see the sky from here.\r\n", ch );
    return;
  }

  set_char_color( AT_BLUE, ch );
  ch_printf( ch, "The sky is %s and %s.\r\n",
      sky_look[weather_info.sky],
      weather_info.change >= 0
      ? "a warm southerly breeze blows"
      : "a cold northern gust blows" );
}


/*
 * Moved into a separate function so it can be used for other things
 * ie: online help editing				-Thoric
 */
HELP_DATA *get_help( CHAR_DATA *ch, const char *orig_argument )
{
  char argall[MAX_INPUT_LENGTH];
  char argone[MAX_INPUT_LENGTH];
  char argnew[MAX_INPUT_LENGTH];
  char argument_buf[MAX_INPUT_LENGTH];
  char *argument = argument_buf;
  HELP_DATA *pHelp = NULL;
  int lev = 0;

  snprintf( argument_buf, MAX_INPUT_LENGTH, "%s", orig_argument );

  if( argument[0] == '\0' )
    snprintf( argument_buf, MAX_INPUT_LENGTH, "%s", "summary" );

  if( isdigit( ( int ) argument[0] ) )
  {
    lev = number_argument( argument, argnew );
    argument = argnew;
  }
  else
    lev = -2;
  /*
   * Tricky argument handling so 'help a b' doesn't match a.
   */
  argall[0] = '\0';
  while( argument[0] != '\0' )
  {
    argument = one_argument( argument, argone );
    if( argall[0] != '\0' )
      strcat( argall, " " );
    strcat( argall, argone );
  }

  for( pHelp = first_help; pHelp; pHelp = pHelp->next )
  {
    if( pHelp->level > get_trust( ch ) )
      continue;
    if( lev != -2 && pHelp->level != lev )
      continue;

    if( is_name( argall, pHelp->keyword ) )
      return pHelp;
  }

  return NULL;
}


/*
 * Now this is cleaner
 */
void do_help( CHAR_DATA * ch, char *argument )
{
  HELP_DATA *pHelp = NULL;

  if( ( pHelp = get_help( ch, argument ) ) == NULL )
  {
    send_to_char( "No help on that word.\r\n", ch );
    return;
  }

  if( pHelp->level >= 0 && str_cmp( argument, "imotd" ) )
  {
    send_to_pager( pHelp->keyword, ch );
    send_to_pager( "\r\n", ch );
  }

  if( !IS_NPC( ch ) && IS_SET( ch->act, PLR_SOUND ) )
    send_to_pager( "!!SOUND(help)", ch );

  /*
   * Strip leading '.' to allow initial blanks.
   */
  if( pHelp->text[0] == '.' )
    send_to_pager_color( pHelp->text + 1, ch );
  else
    send_to_pager_color( pHelp->text, ch );
}

/*
 * Help editor							-Thoric
 */
void do_hedit( CHAR_DATA * ch, char *argument )
{
  HELP_DATA *pHelp = NULL;

  if( !ch->desc )
  {
    send_to_char( "You have no descriptor.\r\n", ch );
    return;
  }

  switch ( ch->substate )
  {
    default:
      break;

    case SUB_HELP_EDIT:
      if( ( pHelp = ( HELP_DATA * ) ch->dest_buf ) == NULL )
      {
	bug( "hedit: sub_help_edit: NULL ch->dest_buf", 0 );
	stop_editing( ch );
	return;
      }

      STRFREE( pHelp->text );
      pHelp->text = copy_buffer( ch );
      stop_editing( ch );
      return;
  }

  if( ( pHelp = get_help( ch, argument ) ) == NULL )	/* new help */
  {
    HELP_DATA *tHelp = NULL;
    char argnew[MAX_INPUT_LENGTH];
    int lev = 0;
    bool new_help = TRUE;

    for( tHelp = first_help; tHelp; tHelp = tHelp->next )
      if( !str_cmp( argument, tHelp->keyword ) )
      {
	pHelp = tHelp;
	new_help = FALSE;
	break;
      }

    if( new_help )
    {
      if( isdigit( ( int ) argument[0] ) )
      {
	lev = number_argument( argument, argnew );
	argument = argnew;
      }
      else
	lev = get_trust( ch );

      CREATE( pHelp, HELP_DATA, 1 );
      pHelp->keyword = STRALLOC( strupper( argument ) );
      pHelp->text = STRALLOC( "" );
      pHelp->level = lev;
      add_help( pHelp );
    }
  }

  ch->substate = SUB_HELP_EDIT;
  ch->dest_buf = pHelp;
  start_editing( ch, pHelp->text );
}

/*
 * Stupid leading space muncher fix				-Thoric
 */
const char *help_fix( const char *text )
{
  char *fixed = NULL;

  if( !text )
    return "";

  fixed = strip_cr( text );

  if( fixed[0] == ' ' )
    fixed[0] = '.';

  return fixed;
}

void do_hset( CHAR_DATA * ch, char *argument )
{
  HELP_DATA *pHelp;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];

  smash_tilde( argument );
  argument = one_argument( argument, arg1 );
  if( arg1[0] == '\0' )
  {
    send_to_char( "Syntax: hset <field> [value] [help page]\r\n", ch );
    send_to_char( "\r\n", ch );
    send_to_char( "Field being one of:\r\n", ch );
    send_to_char( "  level keyword remove save\r\n", ch );
    return;
  }

  if( !str_cmp( arg1, "save" ) )
  {
    FILE *fpout = NULL;
    char bak_filename[MAX_STRING_LENGTH];
    snprintf( bak_filename, MAX_STRING_LENGTH, "%s.bak", HELP_FILE );

    log_string_plus( "Saving help.dat...", LOG_NORMAL );

    rename( HELP_FILE, bak_filename );

    if( ( fpout = fopen( HELP_FILE, "w" ) ) == NULL )
    {
      bug( "hset save: fopen", 0 );
      perror( "help.dat" );
      return;
    }

    for( pHelp = first_help; pHelp; pHelp = pHelp->next )
      fprintf( fpout, "%d %s~\n%s~\n\n",
	  pHelp->level, pHelp->keyword, help_fix( pHelp->text ) );

    fprintf( fpout, "0 $~\n\n\n#$\n" );
    fclose( fpout );
    send_to_char( "Saved.\r\n", ch );
    return;
  }
  if( str_cmp( arg1, "remove" ) )
    argument = one_argument( argument, arg2 );

  if( ( pHelp = get_help( ch, argument ) ) == NULL )
  {
    send_to_char( "Cannot find help on that subject.\r\n", ch );
    return;
  }
  if( !str_cmp( arg1, "remove" ) )
  {
    UNLINK( pHelp, first_help, last_help, next, prev );
    free_help( pHelp );
    send_to_char( "Removed.\r\n", ch );
    return;
  }
  if( !str_cmp( arg1, "level" ) )
  {
    int lev;

    if( !is_number( arg2 ) )
    {
      send_to_char( "Level field must be numeric.\r\n", ch );
      return;
    }

    lev = atoi( arg2 );

    if( lev < -1 || lev > get_trust( ch ) )
    {
      send_to_char( "You can't set the level to that.\r\n", ch );
      return;
    }

    pHelp->level = lev;
    send_to_char( "Done.\r\n", ch );
    return;
  }

  if( !str_cmp( arg1, "keyword" ) )
  {
    STRFREE( pHelp->keyword );
    pHelp->keyword = STRALLOC( strupper( arg2 ) );
    send_to_char( "Done.\r\n", ch );
    return;
  }

  do_hset( ch, STRLIT_EMPTY );
}

/*
 * Show help topics in a level range				-Thoric
 * Idea suggested by Gorog
 */
void do_hlist( CHAR_DATA * ch, char *argument )
{
  int min = 0, max = 0, cnt = 0;
  char arg[MAX_INPUT_LENGTH];
  HELP_DATA *help = NULL;
  int maxlimit = get_trust( ch );
  int minlimit = IS_IMMORTAL( ch ) ? -1 : 0;
  argument = one_argument( argument, arg );

  if( arg[0] != '\0' )
  {
    min = URANGE( minlimit, atoi( arg ), maxlimit );

    if( argument[0] != '\0' )
      max = URANGE( min, atoi( argument ), maxlimit );
    else
      max = maxlimit;
  }
  else
  {
    min = minlimit;
    max = maxlimit;
  }

  set_pager_color( AT_GREEN, ch );
  pager_printf( ch, "Help Topics in level range %d to %d:\r\n\r\n", min,
      max );

  for( cnt = 0, help = first_help; help; help = help->next )
    if( help->level >= min && help->level <= max )
    {
      pager_printf( ch, "  %3d %s\r\n", help->level, help->keyword );
      ++cnt;
    }

  if( cnt )
    pager_printf( ch, "\r\n%d pages found.\r\n", cnt );
  else
    send_to_char( "None found.\r\n", ch );
}

/* 
 * New do_who	
 *
 * with WHO REQUEST, clan and homepage support.  -Thoric
 *
 * Latest version of do_who eliminates redundant code by using linked lists.
 * Shows imms separately, indicates guest and retired immortals.
 * Narn, Oct/96
 */
void do_who( CHAR_DATA * ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char clan_name[MAX_INPUT_LENGTH];
  char invis_str[MAX_INPUT_LENGTH];
  char char_name[MAX_INPUT_LENGTH];
  char extra_title[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *d = NULL;
  int iLevelLower = 0;
  int iLevelUpper = 200;
  int nNumber = 0;
  int nMatch = 0;
  bool fImmortalOnly = FALSE;
  bool fClanMatch = FALSE;	/* SB who clan */
  CLAN_DATA *pClan = 0;

  WHO_DATA *cur_who = NULL;
  WHO_DATA *next_who = NULL;
  WHO_DATA *first_mortal = NULL;
  WHO_DATA *first_imm = NULL;

  /*
   * Parse arguments.
   */
  for( ;; )
  {
    char arg[MAX_STRING_LENGTH];

    argument = one_argument( argument, arg );

    if( arg[0] == '\0' )
      break;

    if( is_number( arg ) )
    {
      switch ( ++nNumber )
      {
	case 1:
	  iLevelLower = atoi( arg );
	  break;
	case 2:
	  iLevelUpper = atoi( arg );
	  break;
	default:
	  send_to_char( "Only two level numbers allowed.\r\n", ch );
	  return;
      }
    }
    else
    {
      if( strlen( arg ) < 3 )
      {
	send_to_char( "Be a little more specific please.\r\n", ch );
	return;
      }

      /*
       * Look for classes to turn on.
       */

      if( !str_cmp( arg, "imm" ) || !str_cmp( arg, "gods" ) )
      {
	fImmortalOnly = TRUE;
      }
      else if( ( pClan = get_clan( arg ) ) )
      {
	fClanMatch = TRUE;
      }
      else
      {
	send_to_char( "That's not an organization.\r\n", ch );
	return;
      }
    }
  }

  /*
   * Now find matching chars.
   */
  nMatch = 0;
  buf[0] = '\0';

  set_pager_color( AT_GREEN, ch );

  /* start from last to first to get it in the proper order */
  for( d = last_descriptor; d; d = d->prev )
  {
    CHAR_DATA *wch = NULL;

    if( ( d->connected != CON_PLAYING && d->connected != CON_EDITING )
	|| ( !can_see( ch, d->character ) && IS_IMMORTAL( d->character ) )
	|| d->original )
      continue;

    wch = d->original ? d->original : d->character;

    if( wch->top_level < iLevelLower
	|| wch->top_level > iLevelUpper
	|| ( fImmortalOnly && !IS_IMMORTAL( wch ) )
	|| ( fClanMatch && ( pClan != wch->pcdata->clan ) ) /* SB */  )
      continue;

    nMatch++;

    strcpy( char_name, "" );

    if( !nifty_is_name( wch->name, wch->pcdata->title )
	&& ch->top_level > wch->top_level )
      snprintf( extra_title, MAX_STRING_LENGTH, " [%s]", wch->name );
    else
      strcpy( extra_title, "" );

    if( wch->pcdata->clan )
    {
      CLAN_DATA *pclan = wch->pcdata->clan;
      strcpy( clan_name, " (" );

      if( clan_char_is_leader( pclan, wch ) )
	strcat( clan_name, "Leader, " );

      strcat( clan_name, pclan->name );
      strcat( clan_name, ")" );
    }
    else
      clan_name[0] = '\0';

    if( IS_SET( wch->act, PLR_WIZINVIS ) )
      snprintf( invis_str, MAX_STRING_LENGTH, "(%d) ", wch->pcdata->wizinvis );
    else
      invis_str[0] = '\0';

    if( wch->desc->connected == CON_EDITING )
      strcat( invis_str, "[Writing] " );

    snprintf( buf, MAX_STRING_LENGTH, "%s%s%s%s %s%s%s\r\n",
	invis_str,
	IS_SET( wch->act, PLR_AFK ) ? "[AFK] " : "",
	char_name,
	wch->pcdata->title,
	extra_title,
	clan_name,
	IS_IMMORTAL( wch ) ? "&R [&WCoder&R/&WImmortal&R]&W" : "&W" );

    /*  
     * This is where the old code would display the found player to the ch.
     * What we do instead is put the found data into a linked list
     */

    /* First make the structure. */
    CREATE( cur_who, WHO_DATA, 1 );
    cur_who->text = str_dup( buf );

    if( IS_OFFICIAL( wch ) )
      cur_who->type = WT_IMM;
    else
      cur_who->type = WT_MORTAL;

    /* Then put it into the appropriate list. */
    switch ( cur_who->type )
    {
      case WT_MORTAL:
	cur_who->next = first_mortal;
	first_mortal = cur_who;
	break;
      case WT_IMM:
	cur_who->next = first_imm;
	first_imm = cur_who;
	break;
    }
  }

  /* Ok, now we have three separate linked lists and what remains is to 
   * display the information and clean up.
   */

  /* Deadly list removed for swr ... now only 2 lists */
  if( first_mortal )
  {
    send_to_pager( "\r\n&R--------------------------------[ &YGalactic Citizens&R ]-------------------------&W\r\n\r\n", ch );
  }

  for( cur_who = first_mortal; cur_who; cur_who = next_who )
  {
    send_to_pager( cur_who->text, ch );

    next_who = cur_who->next;
    DISPOSE( cur_who->text );
    DISPOSE( cur_who );
  }

  if( first_imm )
  {
    send_to_pager( "\r\n&R--------------------------------[ &YElected Officials&R ]-------------------------&W\r\n\r\n", ch );
  }

  for( cur_who = first_imm; cur_who; cur_who = next_who )
  {
    send_to_pager( cur_who->text, ch );

    next_who = cur_who->next;
    DISPOSE( cur_who->text );
    DISPOSE( cur_who );
  }

  set_char_color( AT_YELLOW, ch );
  ch_printf( ch, "\r\n%d player%s.\r\n", nMatch, nMatch == 1 ? "" : "s" );
}

void do_compare( CHAR_DATA * ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  OBJ_DATA *obj1 = NULL;
  OBJ_DATA *obj2 = NULL;
  int value1 = 0;
  int value2 = 0;
  const char *msg = NULL;

  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );

  if( arg1[0] == '\0' )
  {
    send_to_char( "Compare what to what?\r\n", ch );
    return;
  }

  if( ( obj1 = get_obj_carry( ch, arg1 ) ) == NULL )
  {
    send_to_char( "You do not have that item.\r\n", ch );
    return;
  }

  if( arg2[0] == '\0' )
  {
    for( obj2 = ch->first_carrying; obj2; obj2 = obj2->next_content )
    {
      if( obj2->wear_loc != WEAR_NONE
	  && can_see_obj( ch, obj2 )
	  && obj1->item_type == obj2->item_type
	  && ( obj1->wear_flags & obj2->wear_flags & ~ITEM_TAKE ) != 0 )
	break;
    }

    if( !obj2 )
    {
      send_to_char( "You aren't wearing anything comparable.\r\n", ch );
      return;
    }
  }
  else
  {
    if( ( obj2 = get_obj_carry( ch, arg2 ) ) == NULL )
    {
      send_to_char( "You do not have that item.\r\n", ch );
      return;
    }
  }

  if( obj1 == obj2 )
  {
    msg = "You compare $p to itself.  It looks about the same.";
  }
  else if( obj1->item_type != obj2->item_type )
  {
    msg = "You can't compare $p and $P.";
  }
  else
  {
    switch ( obj1->item_type )
    {
      default:
	msg = "You can't compare $p and $P.";
	break;

      case ITEM_ARMOR:
	value1 = obj1->value[0];
	value2 = obj2->value[0];
	break;

      case ITEM_WEAPON:
	value1 = obj1->value[1] + obj1->value[2];
	value2 = obj2->value[1] + obj2->value[2];
	break;
    }
  }

  if( !msg )
  {
    if( value1 == value2 )
      msg = "$p and $P look about the same.";
    else if( value1 > value2 )
      msg = "$p looks better than $P.";
    else
      msg = "$p looks worse than $P.";
  }

  act( AT_PLAIN, msg, ch, obj1, obj2, TO_CHAR );
}

void do_where( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim = NULL;
  DESCRIPTOR_DATA *d = NULL;
  bool found = FALSE;

  if( !IS_IMMORTAL( ch ) )
  {
    send_to_char( "If only life were really that simple...\r\n", ch );
    return;
  }

  one_argument( argument, arg );

  set_pager_color( AT_PERSON, ch );
  if( arg[0] == '\0' )
  {
    send_to_pager( "Players logged in:\r\n", ch );
    found = FALSE;
    for( d = first_descriptor; d; d = d->next )
      if( ( d->connected == CON_PLAYING || d->connected == CON_EDITING )
	  && ( victim = d->character ) != NULL
	  && !IS_NPC( victim ) && victim->in_room && can_see( ch, victim ) )
      {
	found = TRUE;
	pager_printf( ch, "%-28s %s\r\n",
	    victim->name, victim->in_room->name );
      }
    if( !found )
      send_to_char( "None\r\n", ch );
  }
  else
  {
    found = FALSE;
    for( victim = first_char; victim; victim = victim->next )
      if( victim->in_room
	  && !IS_AFFECTED( victim, AFF_HIDE )
	  && !IS_AFFECTED( victim, AFF_SNEAK )
	  && can_see( ch, victim ) && is_name( arg, victim->name ) )
      {
	found = TRUE;
	pager_printf( ch, "%-28s %s\r\n",
	    PERS( victim, ch ), victim->in_room->name );
	break;
      }
    if( !found )
      act( AT_PLAIN, "You didn't find any $T.", ch, NULL, arg, TO_CHAR );
  }
}

void do_consider( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim = NULL;
  const char *msg = "";
  int diff = 0;

  one_argument( argument, arg );

  if( arg[0] == '\0' )
  {
    send_to_char( "Consider killing whom?\r\n", ch );
    return;
  }

  if( ( victim = get_char_room( ch, arg ) ) == NULL )
  {
    send_to_char( "They're not here.\r\n", ch );
    return;
  }

  diff = ( victim->hitroll - ch->hitroll ) * 5;
  diff += ( int ) ( victim->max_hit - ch->max_hit );

  if( diff <= -200 )
    msg = "$N looks like a feather!";
  else if( diff <= -150 )
    msg = "Hey! Where'd $N go?";
  else if( diff <= -100 )
    msg = "Easy as picking off womp rats at beggers canyon!";
  else if( diff <= -50 )
    msg = "$N is a wimp.";
  else if( diff <= 0 )
    msg = "$N looks weaker than you.";
  else if( diff <= 50 )
    msg = "$N looks about as strong as you.";
  else if( diff <= 100 )
    msg = "It would take a bit of luck...";
  else if( diff <= 150 )
    msg = "It would take a lot of luck, and a really big blaster!";
  else if( diff <= 200 )
    msg = "Why don't you just attack a star destoyer with a vibroblade?";
  else
    msg = "$N is built like an AT-AT!";

  act( AT_CONSIDER, msg, ch, NULL, victim, TO_CHAR );
}

void do_forget( CHAR_DATA * ch, char *argument )
{
  int sn = 0;

  if( IS_NPC( ch ) || !ch->pcdata )
    return;

  if( argument[0] == '\0' )
  {
    send_to_char( "Forget what?\r\n", ch );
    return;
  }

  sn = skill_lookup( argument );

  if( sn == -1 )
  {
    send_to_char( "No such skill...\r\n", ch );
    return;
  }

  if( character_skill_level( ch, sn ) <= 0 )
  {
    send_to_char( "You don't know that skill...\r\n", ch );
    return;
  }

  if( character_skill_level( ch, sn ) > 50 )
  {
    send_to_char( "You cannot forget something you know so well...\r\n",
	ch );
    return;
  }

  send_to_char( "Forget what?... *chuckle*\r\n", ch );
  set_skill_level( ch, sn, 0 );
  ch->pcdata->num_skills--;
}

void do_teach( CHAR_DATA * ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  int sn = 0;
  char arg[MAX_INPUT_LENGTH];

  if( IS_NPC( ch ) )
    return;

  argument = one_argument( argument, arg );

  if( argument[0] == '\0' )
  {
    send_to_char( "Teach who, what?\r\n", ch );
    return;
  }
  else
  {
    CHAR_DATA *victim = NULL;
    int adept = 20;

    if( !IS_AWAKE( ch ) )
    {
      send_to_char( "In your dreams, or what?\r\n", ch );
      return;
    }

    if( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
      send_to_char( "They don't seem to be here...\r\n", ch );
      return;
    }

    if( IS_NPC( victim ) )
    {
      send_to_char( "You can't teach that to them!\r\n", ch );
      return;
    }

    sn = skill_lookup( argument );

    if( sn == -1 )
    {
      act( AT_TELL, "You have no idea what that is.",
	  victim, NULL, ch, TO_VICT );
      return;
    }

    if( character_skill_level( victim, sn ) >= adept )
    {
      act( AT_TELL, "$n must practice that on their own.", victim, NULL,
	  ch, TO_VICT );
      return;
    }

    if( victim->pcdata->num_skills >= get_curr_int( victim ) )
    {
      act( AT_TELL,
	  "$n is not intelligent enough to learn any more skills.",
	  victim, NULL, ch, TO_VICT );
      return;
    }

    if( ( victim->pcdata->num_skills - victim->pcdata->adept_skills ) >= 5 )
    {
      act( AT_TELL,
	  "$N needs to perfect some of the skills $E has before learning new ones.",
	  ch, NULL, victim, TO_CHAR );
      return;
    }

    if( skill_table[sn]->type == SKILL_SPELL
	&& ( get_curr_frc( victim ) <= 0 || victim->max_mana <= 0 ) )
    {
      act( AT_TELL, "$n is not force capable.", victim, NULL, ch,
	  TO_VICT );
      return;
    }

    if( ( sn == gsn_lightsaber_crafting || sn == gsn_parry
	  || sn == gsn_lightsabers ) && ( get_curr_frc( victim ) <= 0
	    || victim->max_mana <= 0 ) )
    {
      act( AT_TELL, "$n is not a jedi.", victim, NULL, ch, TO_VICT );
      return;
    }

    if( get_curr_frc( victim ) > 0 && victim->max_mana > 0
	&& skill_table[sn]->type != SKILL_SPELL )
      if( sn != gsn_lightsaber_crafting && sn != gsn_parry
	  && sn != gsn_lightsabers && sn != gsn_aid && sn != gsn_dodge
	  && sn != gsn_rescue && sn != gsn_grip && sn != gsn_sneak
	  && sn != gsn_hide && sn != gsn_spacecraft && sn != gsn_disarm
	  && sn != gsn_spacecombat && sn != gsn_weaponsystems )
      {
	act( AT_TELL,
	    "$n needs to spend time learning and refining jedi abilities instead.",
	    victim, NULL, ch, TO_VICT );
	return;
      }

    if( character_skill_level( ch, sn ) < 100 )
    {
      act( AT_TELL,
	  "You must perfect that yourself before teaching others.",
	  victim, NULL, ch, TO_VICT );
      return;
    }
    else
    {
      if( character_skill_level( victim, sn ) <= 0 )
	victim->pcdata->num_skills++;
      modify_skill_level( victim, sn, int_app[get_curr_int( ch )].learn );
      snprintf( buf, MAX_STRING_LENGTH, "You teach %s $T.", victim->name );
      act( AT_ACTION, buf, ch, NULL, skill_table[sn]->name, TO_CHAR );
      snprintf( buf, MAX_STRING_LENGTH, "%s teaches you $T.", ch->name );
      act( AT_ACTION, buf, victim, NULL, skill_table[sn]->name, TO_CHAR );
    }
  }
}

void do_wimpy( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  int wimpy = 0;

  one_argument( argument, arg );

  if( arg[0] == '\0' )
    wimpy = ( int ) ch->max_hit / 5;
  else
    wimpy = atoi( arg );

  if( wimpy < 0 )
  {
    send_to_char( "Your courage exceeds your wisdom.\r\n", ch );
    return;
  }

  if( wimpy > ch->max_hit )
  {
    send_to_char( "Such cowardice ill becomes you.\r\n", ch );
    return;
  }

  ch->wimpy = wimpy;
  ch_printf( ch, "Wimpy set to %d percent.\r\n", wimpy );
}

void do_password( CHAR_DATA * ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char *pArg = arg1;
  char *pwdnew = NULL;
  char *p = NULL;
  char cEnd = ' ';

  if( IS_NPC( ch ) )
    return;

  /*
   * Can't use one_argument here because it smashes case.
   * So we just steal all its code.  Bleagh.
   */

  while( isspace( ( int ) *argument ) )
    argument++;

  if( *argument == '\'' || *argument == '"' )
    cEnd = *argument++;

  while( *argument != '\0' )
  {
    if( *argument == cEnd )
    {
      argument++;
      break;
    }
    *pArg++ = *argument++;
  }
  *pArg = '\0';

  pArg = arg2;
  while( isspace( ( int ) *argument ) )
    argument++;

  cEnd = ' ';
  if( *argument == '\'' || *argument == '"' )
    cEnd = *argument++;

  while( *argument != '\0' )
  {
    if( *argument == cEnd )
    {
      argument++;
      break;
    }
    *pArg++ = *argument++;
  }
  *pArg = '\0';

  if( arg1[0] == '\0' || arg2[0] == '\0' )
  {
    send_to_char( "Syntax: password <old> <new>.\r\n", ch );
    return;
  }

  if( strcmp( encode_string( arg1 ), ch->pcdata->pwd ) )
  {
    WAIT_STATE( ch, 40 );
    send_to_char( "Wrong password. Wait 10 seconds.\r\n", ch );
    return;
  }

  if( strlen( arg2 ) < 5 )
  {
    send_to_char( "New password must be at least five characters long.\r\n",
	ch );
    return;
  }

  /*
   * No tilde allowed because of player file format.
   */
  pwdnew = encode_string( arg2 );

  for( p = pwdnew; *p != '\0'; p++ )
  {
    if( *p == '~' )
    {
      send_to_char( "New password not acceptable, try again.\r\n", ch );
      return;
    }
  }

  DISPOSE( ch->pcdata->pwd );
  ch->pcdata->pwd = str_dup( pwdnew );

  if( IS_SET( sysdata.save_flags, SV_PASSCHG ) )
    save_char_obj( ch );

  send_to_char( "Ok.\r\n", ch );
}

void do_socials( CHAR_DATA * ch, char *argument )
{
  int iHash = 0;
  int col = 0;
  SOCIALTYPE *social = NULL;

  set_pager_color( AT_PLAIN, ch );
  for( iHash = 0; iHash < 27; iHash++ )
    for( social = social_index[iHash]; social; social = social->next )
    {
      pager_printf( ch, "%-12s", social->name );
      if( ++col % 6 == 0 )
	send_to_pager( "\r\n", ch );
    }

  if( col % 6 != 0 )
    send_to_pager( "\r\n", ch );
}

void do_commands( CHAR_DATA * ch, char *argument )
{
  int col = 0;
  bool found = FALSE;
  int hash = 0;
  CMDTYPE *command = NULL;

  set_pager_color( AT_PLAIN, ch );

  if( argument[0] == '\0' )
  {
    for( hash = 0; hash < 126; hash++ )
      for( command = command_hash[hash]; command; command = command->next )
	if( command->level <= get_trust( ch )
	    && ( command->name[0] != 'm' && command->name[1] != 'p' ) )
	{
	  pager_printf( ch, "%-12s", command->name );
	  if( ++col % 6 == 0 )
	    send_to_pager( "\r\n", ch );
	}
    if( col % 6 != 0 )
      send_to_pager( "\r\n", ch );
  }
  else
  {
    found = FALSE;
    for( hash = 0; hash < 126; hash++ )
      for( command = command_hash[hash]; command; command = command->next )
	if( command->level <= get_trust( ch )
	    && !str_prefix( argument, command->name )
	    && ( command->name[0] != 'm' && command->name[1] != 'p' ) )
	{
	  pager_printf( ch, "%-12s", command->name );
	  found = TRUE;
	  if( ++col % 6 == 0 )
	    send_to_pager( "\r\n", ch );
	}

    if( col % 6 != 0 )
      send_to_pager( "\r\n", ch );
    if( !found )
      ch_printf( ch, "No command found under %s.\r\n", argument );
  }
}

void do_channels( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];

  one_argument( argument, arg );

  if( arg[0] == '\0' )
  {
    if( !IS_NPC( ch ) && IS_SET( ch->act, PLR_SILENCE ) )
    {
      send_to_char( "You are silenced.\r\n", ch );
      return;
    }

    send_to_char( "Channels:", ch );

    if( get_trust( ch ) > 2 && !NOT_AUTHED( ch ) )
    {
      send_to_char( !IS_SET( ch->deaf, CHANNEL_AUCTION )
	  ? " +AUCTION" : " -auction", ch );
    }

    send_to_char( !IS_SET( ch->deaf, CHANNEL_OOC )
	? " +OOC" : " -ooc", ch );

    send_to_char( !IS_SET( ch->deaf, CHANNEL_PNET )
	? " +pnet" : " -pnet", ch );

    send_to_char( !IS_SET( ch->deaf, CHANNEL_GNET )
	? " +gnet" : " -gnet", ch );

    if( !IS_NPC( ch ) && ch->pcdata->clan )
    {
      send_to_char( !IS_SET( ch->deaf, CHANNEL_CLAN )
	  ? " +CLAN" : " -clan", ch );
    }

    send_to_char( !IS_SET( ch->deaf, CHANNEL_TELLS )
	? " +TELLS" : " -tells", ch );

    if( IS_IMMORTAL( ch ) )
    {
      send_to_char( !IS_SET( ch->deaf, CHANNEL_IMMTALK )
	  ? " +IMMTALK" : " -immtalk", ch );
    }

    send_to_char( !IS_SET( ch->deaf, CHANNEL_YELL )
	? " +YELL" : " -yell", ch );

    if( IS_IMMORTAL( ch ) )
    {
      send_to_char( !IS_SET( ch->deaf, CHANNEL_MONITOR )
	  ? " +MONITOR" : " -monitor", ch );
    }

    send_to_char( !IS_SET( ch->deaf, CHANNEL_NEWBIE )
	? " +NEWBIE" : " -newbie", ch );

    if( IS_IMMORTAL( ch ) )
    {
      send_to_char( !IS_SET( ch->deaf, CHANNEL_LOG )
	  ? " +LOG" : " -log", ch );

      send_to_char( !IS_SET( ch->deaf, CHANNEL_BUILD )
	  ? " +BUILD" : " -build", ch );

      send_to_char( !IS_SET( ch->deaf, CHANNEL_COMM )
	  ? " +COMM" : " -comm", ch );
    }
    send_to_char( ".\r\n", ch );
  }
  else
  {
    bool fClear = FALSE;
    bool ClearAll = FALSE;
    int bit = 0;

    if( arg[0] == '+' )
      fClear = TRUE;
    else if( arg[0] == '-' )
      fClear = FALSE;
    else
    {
      send_to_char( "Channels -channel or +channel?\r\n", ch );
      return;
    }

    if( !str_cmp( arg + 1, "auction" ) )
      bit = CHANNEL_AUCTION;
    else if( !str_cmp( arg + 1, "ooc" ) )
      bit = CHANNEL_OOC;
    else if( !str_cmp( arg + 1, "clan" ) )
      bit = CHANNEL_CLAN;
    else if( !str_cmp( arg + 1, "tells" ) )
      bit = CHANNEL_TELLS;
    else if( !str_cmp( arg + 1, "immtalk" ) )
      bit = CHANNEL_IMMTALK;
    else if( !str_cmp( arg + 1, "log" ) )
      bit = CHANNEL_LOG;
    else if( !str_cmp( arg + 1, "build" ) )
      bit = CHANNEL_BUILD;
    else if( !str_cmp( arg + 1, "monitor" ) )
      bit = CHANNEL_MONITOR;
    else if( !str_cmp( arg + 1, "newbie" ) )
      bit = CHANNEL_NEWBIE;
    else if( !str_cmp( arg + 1, "yell" ) )
      bit = CHANNEL_YELL;
    else if( !str_cmp( arg + 1, "comm" ) )
      bit = CHANNEL_COMM;
    else if( !str_cmp( arg + 1, "pnet" ) )
      bit = CHANNEL_PNET;
    else if( !str_cmp( arg + 1, "gnet" ) )
      bit = CHANNEL_GNET;
    else if( !str_cmp( arg + 1, "all" ) )
      ClearAll = TRUE;
    else
    {
      send_to_char( "Set or clear which channel?\r\n", ch );
      return;
    }

    if( ( fClear ) && ( ClearAll ) )
    {
      REMOVE_BIT( ch->deaf, CHANNEL_AUCTION );
      REMOVE_BIT( ch->deaf, CHANNEL_YELL );

      if( IS_IMMORTAL( ch ) )
	REMOVE_BIT( ch->deaf, CHANNEL_COMM );

    }
    else if( ( !fClear ) && ( ClearAll ) )
    {
      SET_BIT( ch->deaf, CHANNEL_AUCTION );
      SET_BIT( ch->deaf, CHANNEL_YELL );

      if( IS_IMMORTAL( ch ) )
	SET_BIT( ch->deaf, CHANNEL_COMM );

    }
    else if( fClear )
    {
      REMOVE_BIT( ch->deaf, bit );
    }
    else
    {
      SET_BIT( ch->deaf, bit );
    }

    send_to_char( "Ok.\r\n", ch );
  }
}

void do_wizlist( CHAR_DATA * ch, char *argument )
{
  ch_printf( ch, "\r\n" );
  ch_printf( ch, "&WPlanets: The Quest For Galactic Domination\r\n" );
  ch_printf( ch, "\r\n" );
  ch_printf( ch, "  SWR 2.0-alpha2 by\r\n" );
  ch_printf( ch, "  Sean Cooper (aka Durga)\r\n" );
  ch_printf( ch, "  specs@golden.net\r\n" );
  ch_printf( ch, "\r\n" );
  ch_printf( ch, "Elected Officials:\r\n" );
  ch_printf( ch, "\r\n" );
  ch_printf( ch, "  %s\r\n", sysdata.officials );
  ch_printf( ch, "\r\n" );
}

/*
 * Contributed by Grodyn.
 */
void do_config( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];

  if( IS_NPC( ch ) )
    return;

  one_argument( argument, arg );

  set_char_color( AT_WHITE, ch );
  if( arg[0] == '\0' )
  {
    send_to_char( "[ Keyword  ] Option\r\n", ch );

    send_to_char( IS_SET( ch->act, PLR_FLEE )
	? "[+FLEE     ] You flee if you get attacked.\r\n"
	: "[-flee     ] You fight back if you get attacked.\r\n",
	ch );

    send_to_char( IS_SET( ch->pcdata->flags, PCFLAG_NORECALL )
	?
	"[+NORECALL ] You fight to the death, link-dead or not.\r\n"
	:
	"[-norecall ] You try to recall if fighting link-dead.\r\n",
	ch );

    send_to_char( IS_SET( ch->act, PLR_AUTOEXIT )
	? "[+AUTOEXIT ] You automatically see exits.\r\n"
	: "[-autoexit ] You don't automatically see exits.\r\n",
	ch );

    send_to_char( IS_SET( ch->act, PLR_AUTOLOOT )
	? "[+AUTOLOOT ] You automatically loot corpses.\r\n"
	:
	"[-autoloot ] You don't automatically loot corpses.\r\n",
	ch );

    send_to_char( IS_SET( ch->act, PLR_AUTOSAC )
	? "[+AUTOSAC  ] You automatically sacrifice corpses.\r\n"
	:
	"[-autosac  ] You don't automatically sacrifice corpses.\r\n",
	ch );

    send_to_char( IS_SET( ch->act, PLR_AUTOGOLD )
	?
	"[+AUTOCRED ] You automatically split credits from kills in groups.\r\n"
	:
	"[-autocred ] You don't automatically split credits from kills in groups.\r\n",
	ch );

    send_to_char( IS_SET( ch->pcdata->flags, PCFLAG_GAG )
	? "[+GAG      ] You see only necessary battle text.\r\n"
	: "[-gag      ] You see full battle text.\r\n", ch );

    send_to_char( IS_SET( ch->pcdata->flags, PCFLAG_PAGERON )
	? "[+PAGER    ] Long output is page-paused.\r\n"
	: "[-pager    ] Long output scrolls to the end.\r\n",
	ch );

    send_to_char( IS_SET( ch->act, PLR_BLANK )
	?
	"[+BLANK    ] You have a blank line before your prompt.\r\n"
	:
	"[-blank    ] You have no blank line before your prompt.\r\n",
	ch );

    send_to_char( IS_SET( ch->act, PLR_BRIEF )
	? "[+BRIEF    ] You see brief descriptions.\r\n"
	: "[-brief    ] You see long descriptions.\r\n", ch );

    send_to_char( IS_SET( ch->act, PLR_COMBINE )
	?
	"[+COMBINE  ] You see object lists in combined format.\r\n"
	:
	"[-combine  ] You see object lists in single format.\r\n",
	ch );

    send_to_char( IS_SET( ch->pcdata->flags, PCFLAG_NOINTRO )
	?
	"[+NOINTRO  ] You don't see the ascii intro screen on login.\r\n"
	:
	"[-nointro  ] You see the ascii intro screen on login.\r\n",
	ch );

    send_to_char( IS_SET( ch->act, PLR_PROMPT )
	? "[+PROMPT   ] You have a prompt.\r\n"
	: "[-prompt   ] You don't have a prompt.\r\n", ch );

    send_to_char( IS_SET( ch->act, PLR_TELNET_GA )
	? "[+TELNETGA ] You receive a telnet GA sequence.\r\n"
	:
	"[-telnetga ] You don't receive a telnet GA sequence.\r\n",
	ch );

    send_to_char( IS_SET( ch->act, PLR_ANSI )
	? "[+ANSI     ] You receive ANSI color sequences.\r\n"
	:
	"[-ansi     ] You don't receive receive ANSI colors.\r\n",
	ch );

    send_to_char( IS_SET( ch->act, PLR_SOUND )
	? "[+SOUND     ] You have MSP support.\r\n"
	: "[-sound     ] You don't have MSP support.\r\n", ch );


    send_to_char( IS_SET( ch->act, PLR_SHOVEDRAG )
	?
	"[+SHOVEDRAG] You allow yourself to be shoved and dragged around.\r\n"
	:
	"[-shovedrag] You'd rather not be shoved or dragged around.\r\n",
	ch );

    send_to_char( IS_SET( ch->pcdata->flags, PCFLAG_NOSUMMON )
	?
	"[+NOSUMMON ] You do not allow other players to summon you.\r\n"
	:
	"[-nosummon ] You allow other players to summon you.\r\n",
	ch );

    if( IS_IMMORTAL( ch ) )
      send_to_char( IS_SET( ch->act, PLR_ROOMVNUM )
	  ? "[+VNUM     ] You can see the VNUM of a room.\r\n"
	  : "[-vnum     ] You do not see the VNUM of a room.\r\n",
	  ch );

    if( IS_IMMORTAL( ch ) )
      send_to_char( IS_SET( ch->act, PLR_AUTOMAP )	/* maps */
	  ? "[+MAP      ] You can see the MAP of a room.\r\n"
	  : "[-map      ] You do not see the MAP of a room.\r\n",
	  ch );

    if( IS_IMMORTAL( ch ) )	/* Added 10/16 by Kuran of SWR */
      send_to_char( IS_SET( ch->pcdata->flags, PCFLAG_ROOM )
	  ? "[+ROOMFLAGS] You will see room flags.\r\n"
	  : "[-roomflags] You will not see room flags.\r\n", ch );

    send_to_char( IS_SET( ch->act, PLR_SILENCE )
	? "[+SILENCE  ] You are silenced.\r\n" : "", ch );

    send_to_char( !IS_SET( ch->act, PLR_NO_EMOTE )
	? "" : "[-emote    ] You can't emote.\r\n", ch );

    send_to_char( !IS_SET( ch->act, PLR_NO_TELL )
	? "" : "[-tell     ] You can't use 'tell'.\r\n", ch );

    send_to_char( !IS_SET( ch->act, PLR_LITTERBUG )
	? ""
	:
	"[-litter  ] A convicted litterbug. You cannot drop anything.\r\n",
	ch );
  }
  else
  {
    bool fSet = FALSE;
    int bit = 0;

    if( arg[0] == '+' )
      fSet = TRUE;
    else if( arg[0] == '-' )
      fSet = FALSE;
    else
    {
      send_to_char( "Config -option or +option?\r\n", ch );
      return;
    }

    if( !str_prefix( arg + 1, "autoexit" ) )
      bit = PLR_AUTOEXIT;
    else if( !str_prefix( arg + 1, "autoloot" ) )
      bit = PLR_AUTOLOOT;
    else if( !str_prefix( arg + 1, "autosac" ) )
      bit = PLR_AUTOSAC;
    else if( !str_prefix( arg + 1, "autocred" ) )
      bit = PLR_AUTOGOLD;
    else if( !str_prefix( arg + 1, "blank" ) )
      bit = PLR_BLANK;
    else if( !str_prefix( arg + 1, "brief" ) )
      bit = PLR_BRIEF;
    else if( !str_prefix( arg + 1, "combine" ) )
      bit = PLR_COMBINE;
    else if( !str_prefix( arg + 1, "prompt" ) )
      bit = PLR_PROMPT;
    else if( !str_prefix( arg + 1, "telnetga" ) )
      bit = PLR_TELNET_GA;
    else if( !str_prefix( arg + 1, "ansi" ) )
      bit = PLR_ANSI;
    else if( !str_prefix( arg + 1, "sound" ) )
      bit = PLR_SOUND;
    else if( !str_prefix( arg + 1, "flee" ) )
      bit = PLR_FLEE;
    else if( !str_prefix( arg + 1, "nice" ) )
      bit = PLR_NICE;
    else if( !str_prefix( arg + 1, "shovedrag" ) )
      bit = PLR_SHOVEDRAG;
    else if( IS_IMMORTAL( ch ) && !str_prefix( arg + 1, "vnum" ) )
      bit = PLR_ROOMVNUM;
    else if( IS_IMMORTAL( ch ) && !str_prefix( arg + 1, "map" ) )
      bit = PLR_AUTOMAP;	/* maps */

    if( bit )
    {
      if( fSet )
	SET_BIT( ch->act, bit );
      else
	REMOVE_BIT( ch->act, bit );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    else
    {
      if( !str_prefix( arg + 1, "norecall" ) )
	bit = PCFLAG_NORECALL;
      else if( !str_prefix( arg + 1, "nointro" ) )
	bit = PCFLAG_NOINTRO;
      else if( !str_prefix( arg + 1, "nosummon" ) )
	bit = PCFLAG_NOSUMMON;
      else if( !str_prefix( arg + 1, "gag" ) )
	bit = PCFLAG_GAG;
      else if( !str_prefix( arg + 1, "pager" ) )
	bit = PCFLAG_PAGERON;
      else if( !str_prefix( arg + 1, "roomflags" )
	  && ( IS_IMMORTAL( ch ) ) )
	bit = PCFLAG_ROOM;
      else
      {
	send_to_char( "Config which option?\r\n", ch );
	return;
      }

      if( fSet )
	SET_BIT( ch->pcdata->flags, bit );
      else
	REMOVE_BIT( ch->pcdata->flags, bit );

      send_to_char( "Ok.\r\n", ch );
      return;
    }
  }
}

void do_credits( CHAR_DATA * ch, char *argument )
{
  char credits[MAX_INPUT_LENGTH];
  snprintf( credits, MAX_INPUT_LENGTH, "%s", "credits" );
  do_help( ch, credits );
}

extern int top_area;

/* 
 * New do_areas with soft/hard level ranges 
 */

void do_areas( CHAR_DATA * ch, char *argument )
{
  set_pager_color( AT_PLAIN, ch );
  pager_printf( ch, "AREAS\r\n\r\n" );
  pager_printf( ch,
      "All areas are collectively built and modified by the\r\norganizations that control them.\r\n" );
  pager_printf( ch, "\r\nSee PLANETS and ORGANIZATIONS.\r\n" );
}

void do_afk( CHAR_DATA * ch, char *argument )
{
  if( IS_NPC( ch ) )
    return;

  if( IS_SET( ch->act, PLR_AFK ) )
  {
    REMOVE_BIT( ch->act, PLR_AFK );
    send_to_char( "You are no longer afk.\r\n", ch );
    act( AT_GREY, "$n is no longer afk.", ch, NULL, NULL, TO_ROOM );
  }
  else
  {
    SET_BIT( ch->act, PLR_AFK );
    send_to_char( "You are now afk.\r\n", ch );
    act( AT_GREY, "$n is now afk.", ch, NULL, NULL, TO_ROOM );
    return;
  }

}

void do_slist( CHAR_DATA * ch, char *argument )
{
}

void do_whois( CHAR_DATA * ch, char *argument )
{
  CHAR_DATA *victim;
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];

  buf[0] = '\0';

  if( IS_NPC( ch ) )
    return;

  if( argument[0] == '\0' )
  {
    send_to_char( "You must input the name of a player online.\r\n", ch );
    return;
  }

  strcat( buf, "0." );
  strcat( buf, argument );
  if( ( ( victim = get_char_world( ch, buf ) ) == NULL ) )
  {
    send_to_char( "No such player online.\r\n", ch );
    return;
  }

  if( IS_NPC( victim ) )
  {
    send_to_char( "That's not a player!\r\n", ch );
    return;
  }

  ch_printf( ch, "%s is a %s character",
      victim->name,
      victim->sex == SEX_MALE ? "male" :
      victim->sex == SEX_FEMALE ? "female" : "neutral" );
  if( IS_IMMORTAL( ch ) )
    ch_printf( ch, " in room %d.\r\n", victim->in_room->vnum );
  else
    ch_printf( ch, ".\r\n" );

  if( victim->pcdata->clan )
  {
    send_to_char( ", and belongs to organization ", ch );
    send_to_char( victim->pcdata->clan->name, ch );
  }
  send_to_char( ".\r\n", ch );

  if( victim->pcdata->homepage && victim->pcdata->homepage[0] != '\0' )
    ch_printf( ch, "%s's homepage can be found at %s.\r\n",
	victim->name, victim->pcdata->homepage );

  if( victim->pcdata->bio && victim->pcdata->bio[0] != '\0' )
    ch_printf( ch, "%s's personal bio:\r\n%s",
	victim->name, victim->pcdata->bio );

  if( IS_IMMORTAL( ch ) )
  {
    send_to_char
      ( "----------------------------------------------------\r\n", ch );

    send_to_char( "Info for immortals:\r\n", ch );

    if( victim->pcdata->authed_by && victim->pcdata->authed_by[0] != '\0' )
      ch_printf( ch, "%s was authorized by %s.\r\n",
	  victim->name, victim->pcdata->authed_by );

    if( victim->pcdata->pkills || victim->pcdata->pdeaths )
      ch_printf( ch,
	  "%s has killed %d players, and been killed by a player %d times.\r\n",
	  victim->name, victim->pcdata->pkills,
	  victim->pcdata->pdeaths );

    ch_printf( ch, "%s is %shelled at the moment.\r\n",
	victim->name,
	( victim->pcdata->release_date == 0 ) ? "not " : "" );

    if( victim->pcdata->release_date != 0 )
      ch_printf( ch,
	  "%s was helled by %s, and will be released on %24.24s.\r\n",
	  victim->sex == SEX_MALE ? "He" : victim->sex ==
	  SEX_FEMALE ? "She" : "It", victim->pcdata->helled_by,
	  ctime( &victim->pcdata->release_date ) );

    if( get_trust( victim ) < get_trust( ch ) )
    {
      snprintf( buf2, MAX_STRING_LENGTH, "list %s", buf );
    }

    if( IS_SET( victim->act, PLR_SILENCE )
	|| IS_SET( victim->act, PLR_NO_EMOTE )
	|| IS_SET( victim->act, PLR_NO_TELL ) )
    {
      snprintf( buf2, MAX_STRING_LENGTH,
	  "This player has the following flags set:" );
      if( IS_SET( victim->act, PLR_SILENCE ) )
	strcat( buf2, " silence" );
      if( IS_SET( victim->act, PLR_NO_EMOTE ) )
	strcat( buf2, " noemote" );
      if( IS_SET( victim->act, PLR_NO_TELL ) )
	strcat( buf2, " notell" );
      strcat( buf2, ".\r\n" );
      send_to_char( buf2, ch );
    }

    if( victim->desc && victim->desc->host[0] != '\0' )	/* added by Gorog */
    {
      snprintf( buf2, MAX_STRING_LENGTH, "%s's IP info: %s ", victim->name,
	  victim->desc->host );

      if( IS_IMMORTAL( ch ) )
      {
	strcat( buf2, victim->desc->host );
      }

      strcat( buf2, "\r\n" );
      send_to_char( buf2, ch );

      snprintf( buf2, MAX_STRING_LENGTH, "Terminal type: %s  MCCP: %s\r\n",
		victim->desc->terminal_type,
		victim->desc->mccp ? "Yes" : "No" );
      send_to_char( buf2, ch );
    }

    if( IS_IMMORTAL( ch ) && get_trust( ch ) >= get_trust( victim )
	&& victim->pcdata )
    {
      snprintf( buf2, MAX_STRING_LENGTH, "Email: %s\r\n",
	  victim->pcdata->email );
      send_to_char( buf2, ch );
    }
  }
}

void do_pager( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];

  if( IS_NPC( ch ) )
    return;

  argument = one_argument( argument, arg );

  if( !*arg )
  {
    char pager_arg[MAX_INPUT_LENGTH];

    if( IS_SET( ch->pcdata->flags, PCFLAG_PAGERON ) )
      {
	snprintf( pager_arg, MAX_INPUT_LENGTH, "%s", "-pager" );
      }
    else
      {
	snprintf( pager_arg, MAX_INPUT_LENGTH, "%s", "+pager" );
      }

    do_config( ch, pager_arg );
    return;
  }
  if( !is_number( arg ) )
  {
    send_to_char( "Set page pausing to how many lines?\r\n", ch );
    return;
  }
  ch->pcdata->pagerlen = atoi( arg );
  if( ch->pcdata->pagerlen < 5 )
    ch->pcdata->pagerlen = 5;
  ch_printf( ch, "Page pausing set to %d lines.\r\n", ch->pcdata->pagerlen );
}

bool is_online( const char *argument )
{
  DESCRIPTOR_DATA *d = NULL;

  if( argument[0] == '\0' )
    return FALSE;

  for( d = last_descriptor; d; d = d->prev )
  {
    CHAR_DATA *wch = NULL;

    if( ( d->connected != CON_PLAYING && d->connected != CON_EDITING )
	|| d->original )
      continue;
    wch = d->original ? d->original : d->character;

    if( !str_cmp( argument, wch->name ) )
      return TRUE;
  }

  return FALSE;
}
