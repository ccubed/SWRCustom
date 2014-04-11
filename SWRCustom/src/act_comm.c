#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "mud.h"

/*
 * Local functions.
 */
void talk_channel( CHAR_DATA * ch, const char *argument,
    int channel, const char *verb );
const char *drunk_speech( const char *argument, CHAR_DATA * ch );

void sound_to_room( const ROOM_INDEX_DATA * room, const char *argument )
{
  CHAR_DATA *vic = NULL;

  if( room == NULL )
    return;

  for( vic = room->first_person; vic; vic = vic->next_in_room )
    if( !IS_NPC( vic ) && IS_SET( vic->act, PLR_SOUND ) )
      send_to_char( argument, vic );
}

void do_beep( CHAR_DATA * ch, char *argument )
{
  CHAR_DATA *victim = NULL;
  char arg[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];

  argument = one_argument( argument, arg );

  REMOVE_BIT( ch->deaf, CHANNEL_TELLS );

  if( !IS_NPC( ch )
      && ( IS_SET( ch->act, PLR_SILENCE )
	|| IS_SET( ch->act, PLR_NO_TELL ) ) )
  {
    send_to_char( "You can't do that.\r\n", ch );
    return;
  }

  if( arg[0] == '\0' )
  {
    send_to_char( "Beep who?\r\n", ch );
    return;
  }

  if( ( victim = get_char_world( ch, arg ) ) == NULL
      || ( IS_NPC( victim ) && victim->in_room != ch->in_room )
      || ( !NOT_AUTHED( ch ) && NOT_AUTHED( victim ) && !IS_IMMORTAL( ch ) ) )
  {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }

  if( !character_has_comlink( ch ) )
  {
    send_to_char( "You need a comlink to do that!\r\n", ch );
    return;
  }

  if( !character_has_comlink( victim ) && !IS_IMMORTAL( ch ) )
  {
    send_to_char( "They don't seem to have a comlink!\r\n", ch );
    return;
  }

  if( NOT_AUTHED( ch ) && !NOT_AUTHED( victim ) && !IS_IMMORTAL( victim ) )
  {
    send_to_char( "They can't hear you because you are not authorized.\r\n",
	ch );
    return;
  }

  if( !IS_NPC( victim ) && ( victim->switched ) && IS_IMMORTAL( ch ) )
  {
    send_to_char( "That player is switched.\r\n", ch );
    return;
  }
  else if( !IS_NPC( victim ) && ( !victim->desc ) )
  {
    send_to_char( "That player is link-dead.\r\n", ch );
    return;
  }

  if( IS_SET( victim->deaf, CHANNEL_TELLS )
      && ( !IS_IMMORTAL( ch ) || ( get_trust( ch ) < get_trust( victim ) ) ) )
  {
    act( AT_PLAIN, "$E has $S tells turned off.", ch, NULL, victim,
	TO_CHAR );
    return;
  }

  if( !IS_NPC( victim ) && ( IS_SET( victim->act, PLR_SILENCE ) ) )
  {
    send_to_char
      ( "That player is silenced.  They will receive your message but can not respond.\r\n",
	ch );
  }

  if( !IS_IMMORTAL( ch ) && !IS_AWAKE( victim ) )
  {
    act( AT_PLAIN, "$E can't hear you.", ch, 0, victim, TO_CHAR );
    return;
  }

  if( victim->desc		/* make sure desc exists first  -Thoric */
      && victim->desc->connected == CON_EDITING && IS_IMMORTAL( ch ) )
  {
    act( AT_PLAIN,
	"$E is currently in a writing buffer.  Please try again in a few minutes.",
	ch, 0, victim, TO_CHAR );
    return;
  }

  ch_printf( ch, "&WYou beep %s: %s\r\n\a", victim->name, argument );
  send_to_char( "\a", victim );

  snprintf( buf, MAX_STRING_LENGTH, "%s beeps: '$t'", ch->name );

  act( AT_WHITE, buf, ch, argument, victim, TO_VICT );
}

const char *drunk_speech( const char *argument, CHAR_DATA * ch )
{
  const char *arg = argument;
  static char buf[MAX_INPUT_LENGTH * 2];
  char buf1[MAX_INPUT_LENGTH * 2];
  short drunk;
  char *txt;
  char *txt1;

  if( IS_NPC( ch ) || !ch->pcdata )
    return argument;

  drunk = ch->pcdata->condition[COND_DRUNK];

  if( drunk <= 0 )
    return ( char * ) argument;

  buf[0] = '\0';
  buf1[0] = '\0';

  if( !argument )
  {
    bug( "Drunk_speech: NULL argument", 0 );
    return "";
  }

  /*
     if ( *arg == '\0' )
     return (char *) argument;
     */

  txt = buf;
  txt1 = buf1;

  while( *arg != '\0' )
  {
    if( toupper( ( int ) *arg ) == 'S' )
    {
      if( number_percent() < ( drunk * 2 ) )	/* add 'h' after an 's' */
      {
	*txt++ = *arg;
	*txt++ = 'h';
      }
      else
      {
	*txt++ = *arg;
      }
    }
    else if( toupper( ( int ) *arg ) == 'X' )
    {
      if( number_percent() < ( drunk * 2 / 2 ) )
      {
	*txt++ = 'c', *txt++ = 's', *txt++ = 'h';
      }
      else
      {
	*txt++ = *arg;
      }
    }
    else if( number_percent() < ( drunk * 2 / 5 ) )	/* slurred letters */
    {
      int slurn = number_range( 1, 2 );
      short currslur = 0;

      while( currslur < slurn )
	*txt++ = *arg, currslur++;
    }
    else
    {
      *txt++ = *arg;
    }

    arg++;
  }

  *txt = '\0';

  txt = buf;

  while( *txt != '\0' )		/* Let's mess with the string's caps */
  {
    if( number_percent() < ( 2 * drunk / 2.5 ) )
    {
      if( isupper( ( int ) *txt ) )
	*txt1 = ( char ) tolower( ( int ) *txt );
      else if( islower( ( int ) *txt ) )
	*txt1 = ( char ) toupper( ( int ) *txt );
      else
	*txt1 = *txt;
    }
    else
    {
      *txt1 = *txt;
    }

    txt1++, txt++;
  }

  *txt1 = '\0';
  txt1 = buf1;
  txt = buf;

  while( *txt1 != '\0' )	/* Let's make them stutter */
  {
    if( *txt1 == ' ' )	/* If there's a space, then there's gotta be a */
    {			/* along there somewhere soon */

      while( *txt1 == ' ' )	/* Don't stutter on spaces */
	*txt++ = *txt1++;

      if( ( number_percent() < ( 2 * drunk / 4 ) ) && *txt1 != '\0' )
      {
	int offset = number_range( 0, 2 );
	short pos = 0;

	while( *txt1 != '\0' && pos < offset )
	  *txt++ = *txt1++, pos++;

	if( *txt1 == ' ' )	/* Make sure not to stutter a space after */
	{		/* the initial offset into the word */
	  *txt++ = *txt1++;
	  continue;
	}

	pos = 0;
	offset = number_range( 2, 4 );

	while( *txt1 != '\0' && pos < offset )
	{
	  *txt++ = *txt1;
	  pos++;

	  if( *txt1 == ' ' || pos == offset )	/* Make sure we don't stick */
	  {		/* A hyphen right before a space */
	    txt1--;
	    break;
	  }
	  *txt++ = '-';
	}

	if( *txt1 != '\0' )
	  txt1++;
      }
    }
    else
    {
      *txt++ = *txt1++;
    }
  }

  *txt = '\0';

  return buf;
}

static void broadcast_channel_default( CHAR_DATA * ch, const char *verb,
    const char *argument, char *buf, size_t len )
{
  set_char_color( AT_GOSSIP, ch );
  ch_printf( ch, "(%s) %s: %s\r\n", verb, ch->name, argument );
  snprintf( buf, len, "(%s) %s: $t",
      verb, IS_IMMORTAL( ch ) ? "$n" : ch->name );
}

static void broadcast_channel_clantalk( CHAR_DATA * ch, const char *verb,
    const char *argument, char *buf, size_t len )
{
  set_char_color( AT_CLAN, ch );
  ch_printf( ch, "Over the organizations private network you say, '%s'\r\n",
      argument );
  snprintf( buf, len, "%s speaks over the organizations network, '$t'",
      IS_IMMORTAL( ch ) ? "$n" : ch->name );
}

static void broadcast_channel_ship( CHAR_DATA * ch, const char *verb,
    const char *argument, char *buf, size_t len )
{
  set_char_color( AT_SHIP, ch );
  ch_printf( ch, "You tell the ship, '%s'\r\n", argument );
  snprintf( buf, len, "%s says over the ships com system, '$t'",
      IS_IMMORTAL( ch ) ? "$n" : ch->name );
}

static void broadcast_channel_yell( CHAR_DATA * ch, const char *verb,
    const char *argument, char *buf, size_t len )
{
  set_char_color( AT_GOSSIP, ch );
  ch_printf( ch, "You %s, '%s'\r\n", verb, argument );
  snprintf( buf, len, "%s %ss, '$t'",
      IS_IMMORTAL( ch ) ? "$n" : ch->name, verb );
}

static void broadcast_channel_newbie( CHAR_DATA * ch, const char *verb,
    const char *argument, char *buf, size_t len )
{
  set_char_color( AT_OOC, ch );
  ch_printf( ch, "(NEWBIE) %s: %s\r\n", ch->name, argument );
  snprintf( buf, len, "(NEWBIE) %s: $t", IS_IMMORTAL( ch ) ? "$n" : ch->name );
}

static void broadcast_channel_ooc( CHAR_DATA * ch, const char *verb,
    const char *argument, char *buf, size_t len )
{
  short position = ch->position;
  set_char_color( AT_OOC, ch );
  snprintf( buf, len, "(OOC) %s: $t", IS_IMMORTAL( ch ) ? "$n" : ch->name );
  ch->position = POS_STANDING;
  act( AT_OOC, buf, ch, argument, NULL, TO_CHAR );
  ch->position = position;
}

static void broadcast_channel_immtalk( CHAR_DATA * ch, const char *verb,
    const char *argument, char *buf, size_t len )
{
  short position = ch->position;
  snprintf( buf, len, "%s> $t", IS_IMMORTAL( ch ) ? "$n" : ch->name );
  ch->position = POS_STANDING;
  act( AT_IMMORT, buf, ch, argument, NULL, TO_CHAR );
  ch->position = position;
}

bool character_has_comlink( const CHAR_DATA * ch )
{
  bool ch_comlink = FALSE;

  if( IS_IMMORTAL( ch ) )
  {
    ch_comlink = TRUE;
  }
  else
  {
    if( get_obj_type_char( ch, ITEM_COMLINK ) )
    {
      ch_comlink = TRUE;
    }
  }

  return ch_comlink;
}

bool channel_needs_comlink( int channel )
{
  bool needs_comlink = FALSE;

  if( channel != CHANNEL_YELL
      && channel != CHANNEL_IMMTALK
      && channel != CHANNEL_OOC
      && channel != CHANNEL_NEWBIE
      && channel != CHANNEL_SHIP && channel != CHANNEL_SYSTEM )
  {
    needs_comlink = TRUE;
  }

  return needs_comlink;
}

/*
 * Generic channel function.
 */
void talk_channel( CHAR_DATA * ch, const char *argument, int channel,
    const char *verb )
{
  char buf[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *d = NULL;
  short position = 0;
  CLAN_DATA *clan = NULL;
  PLANET_DATA *planet = NULL;

  if( channel_needs_comlink( channel ) && !character_has_comlink( ch ) )
  {
    send_to_char( "You need a comlink to do that!\r\n", ch );
    return;
  }

  if( IS_NPC( ch ) && channel == CHANNEL_CLAN )
  {
    send_to_char( "Mobs can't be in clans.\r\n", ch );
    return;
  }

  if( channel == CHANNEL_PNET &&
      ( !ch->in_room->area
	|| ( planet = ch->in_room->area->planet ) == NULL ) )
  {
    send_to_char( "Planet Net only works on planets...\r\n", ch );
    return;
  }

  if( channel == CHANNEL_CLAN )
    clan = ch->pcdata->clan;

  if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
  {
    if( ch->master )
      send_to_char( "I don't think so...\r\n", ch->master );
    return;
  }

  if( argument[0] == '\0' )
  {
    snprintf( buf, MAX_STRING_LENGTH, "%s what?\r\n", verb );
    buf[0] = UPPER( buf[0] );
    send_to_char( buf, ch );	/* where'd this line go? */
    return;
  }

  if( !IS_NPC( ch ) && IS_SET( ch->act, PLR_SILENCE ) )
  {
    ch_printf( ch, "You can't %s.\r\n", verb );
    return;
  }

  REMOVE_BIT( ch->deaf, channel );

  switch ( channel )
  {
    default:
      broadcast_channel_default( ch, verb, argument, buf, MAX_STRING_LENGTH );
      break;

    case CHANNEL_CLANTALK:
      broadcast_channel_clantalk( ch, verb, argument, buf, MAX_STRING_LENGTH );
      break;

    case CHANNEL_SHIP:
      broadcast_channel_ship( ch, verb, argument, buf, MAX_STRING_LENGTH );
      break;

    case CHANNEL_YELL:
      broadcast_channel_yell( ch, verb, argument, buf, MAX_STRING_LENGTH );
      break;

    case CHANNEL_NEWBIE:
      broadcast_channel_newbie( ch, verb, argument, buf, MAX_STRING_LENGTH );
      break;

    case CHANNEL_OOC:
      broadcast_channel_ooc( ch, verb, argument, buf, MAX_STRING_LENGTH );
      break;

    case CHANNEL_IMMTALK:
      broadcast_channel_immtalk( ch, verb, argument, buf, MAX_STRING_LENGTH );
      break;
  }

  for( d = first_descriptor; d; d = d->next )
  {
    CHAR_DATA *och = d->original ? d->original : d->character;
    CHAR_DATA *vch = d->character;

    if( d->connected == CON_PLAYING
	&& vch != ch && !IS_SET( och->deaf, channel ) )
    {
      const char *sbuf = argument;

      if( channel_needs_comlink( channel )
	  && !character_has_comlink( ch ) )
      {
	continue;
      }

      if( channel == CHANNEL_IMMTALK && !IS_IMMORTAL( och ) )
	continue;

      if( channel == CHANNEL_YELL && ch->in_room != vch->in_room )
	continue;

      if( channel == CHANNEL_PNET )
      {
	if( !vch->in_room
	    || !vch->in_room->area
	    || ( vch->in_room->area->planet != planet ) )
	  continue;
      }

      if( channel == CHANNEL_CLAN
	  && ( IS_NPC( vch )
	    || !vch->pcdata->clan || vch->pcdata->clan != clan ) )
	continue;

      if( channel == CHANNEL_SYSTEM )
      {
	SHIP_DATA *ship = ship_from_cockpit( ch->in_room );
	SHIP_DATA *target = NULL;

	if( !ship )
	  continue;

	if( !vch->in_room )
	  continue;

	target = ship_from_cockpit( vch->in_room );

	if( !target )
	  continue;

	if( channel == CHANNEL_SYSTEM )
	  if( target->starsystem != ship->starsystem )
	    continue;
      }

      if( channel == CHANNEL_SHIP )
      {
	SHIP_DATA *ship = ship_from_room( ch->in_room );
	SHIP_DATA *target = NULL;

	if( !ship )
	  continue;

	if( !vch->in_room )
	  continue;

	target = ship_from_room( vch->in_room );

	if( !target )
	  continue;

	if( target != ship )
	  continue;
      }

      position = vch->position;

      if( channel != CHANNEL_YELL )
	vch->position = POS_STANDING;

      MOBtrigger = FALSE;

      if( channel == CHANNEL_IMMTALK )
	act( AT_IMMORT, buf, ch, sbuf, vch, TO_VICT );
      else if( channel == CHANNEL_OOC || channel == CHANNEL_NEWBIE )
	act( AT_OOC, buf, ch, sbuf, vch, TO_VICT );
      else if( channel == CHANNEL_SHIP )
	act( AT_SHIP, buf, ch, sbuf, vch, TO_VICT );
      else if( channel == CHANNEL_CLAN )
	act( AT_CLAN, buf, ch, sbuf, vch, TO_VICT );
      else
	act( AT_GOSSIP, buf, ch, sbuf, vch, TO_VICT );

      vch->position = position;
    }
  }
}

void to_channel( const char *argument, int channel,
    const char *verb, short level )
{
  char buf[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *d;

  if( !first_descriptor || argument[0] == '\0' )
    return;

  snprintf( buf, MAX_STRING_LENGTH, "%s: %s\r\n", verb, argument );

  for( d = first_descriptor; d; d = d->next )
  {
    CHAR_DATA *och = d->original ? d->original : d->character;
    CHAR_DATA *vch = d->character;

    if( !och || !vch )
      continue;

    if( !IS_IMMORTAL( vch )
	&& ( channel == CHANNEL_LOG || channel == CHANNEL_COMM ) )
      continue;

    if( d->connected == CON_PLAYING
	&& !IS_SET( och->deaf, channel ) && get_trust( vch ) >= level )
    {
      set_char_color( AT_LOG, vch );
      send_to_char( buf, vch );
      set_char_color( AT_PLAIN, vch );
    }
  }
}

void do_gnet( CHAR_DATA * ch, char *argument )
{
  talk_channel( ch, argument, CHANNEL_GNET, "GNET" );
}

void do_pnet( CHAR_DATA * ch, char *argument )
{
  talk_channel( ch, argument, CHANNEL_PNET, "PNET" );
}

void do_shiptalk( CHAR_DATA * ch, char *argument )
{
  SHIP_DATA *ship = ship_from_cockpit( ch->in_room );

  if( !ship )
  {
    send_to_char( "&RYou must be in the cockpit of a ship to do that!\r\n",
	ch );
    return;
  }

  talk_channel( ch, argument, CHANNEL_SHIP, "shiptalk" );
}

void do_systemtalk( CHAR_DATA * ch, char *argument )
{
  SHIP_DATA *ship = ship_from_cockpit( ch->in_room );

  if( !ship )
  {
    send_to_char( "&RYou must be in the cockpit of a ship to do that!\r\n",
	ch );
    return;
  }

  talk_channel( ch, argument, CHANNEL_SYSTEM, "Systemtalk" );
}

void do_ooc( CHAR_DATA * ch, char *argument )
{
  talk_channel( ch, argument, CHANNEL_OOC, "ooc" );
}

void do_clantalk( CHAR_DATA * ch, char *argument )
{
  if( NOT_AUTHED( ch ) )
  {
    send_to_char( "Huh?\r\n", ch );
    return;
  }

  if( IS_NPC( ch ) || !ch->pcdata->clan )
  {
    send_to_char( "Huh?\r\n", ch );
    return;
  }

  talk_channel( ch, argument, CHANNEL_CLAN, "clantalk" );
}

void do_newbiechat( CHAR_DATA * ch, char *argument )
{
  talk_channel( ch, argument, CHANNEL_NEWBIE, "newbiechat" );
}

void do_yell( CHAR_DATA * ch, char *argument )
{
  if( NOT_AUTHED( ch ) )
  {
    send_to_char( "Huh?\r\n", ch );
    return;
  }

  talk_channel( ch, drunk_speech( argument, ch ), CHANNEL_YELL, "yell" );
}



void do_immtalk( CHAR_DATA * ch, char *argument )
{
  if( NOT_AUTHED( ch ) )
  {
    send_to_char( "Huh?\r\n", ch );
    return;
  }

  talk_channel( ch, argument, CHANNEL_IMMTALK, "immtalk" );
}


void do_say( CHAR_DATA * ch, char *argument )
{
  CHAR_DATA *vch = NULL;
  int actflags = 0;

  if( argument[0] == '\0' )
  {
    send_to_char( "Say what?\r\n", ch );
    return;
  }

  actflags = ch->act;

  if( IS_NPC( ch ) )
    REMOVE_BIT( ch->act, ACT_SECRETIVE );

  for( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
  {
    const char *sbuf = argument;

    if( vch == ch )
      continue;

    sbuf = drunk_speech( sbuf, ch );

    MOBtrigger = FALSE;
    act( AT_SAY, "$n says '$t'", ch, sbuf, vch, TO_VICT );
  }

  /*    MOBtrigger = FALSE;
	act( AT_SAY, "$n says '$T'", ch, NULL, argument, TO_ROOM ); */
  ch->act = actflags;
  MOBtrigger = FALSE;
  act( AT_SAY, "You say '$T'", ch, NULL, drunk_speech( argument, ch ),
      TO_CHAR );
  mprog_speech_trigger( argument, ch );

  if( char_died( ch ) )
    return;

  oprog_speech_trigger( argument, ch );

  if( char_died( ch ) )
    return;

  rprog_speech_trigger( argument, ch );
}



void do_tell( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];
  CHAR_DATA *victim = NULL;
  short position = 0;
  CHAR_DATA *switched_victim = NULL;

  if( IS_SET( ch->deaf, CHANNEL_TELLS ) && !IS_IMMORTAL( ch ) )
  {
    act( AT_PLAIN, "You have tells turned off... try chan +tells first", ch,
	NULL, NULL, TO_CHAR );
    return;
  }

  if( !IS_NPC( ch )
      && ( IS_SET( ch->act, PLR_SILENCE )
	|| IS_SET( ch->act, PLR_NO_TELL ) ) )
  {
    send_to_char( "You can't do that.\r\n", ch );
    return;
  }

  argument = one_argument( argument, arg );

  if( arg[0] == '\0' || argument[0] == '\0' )
  {
    send_to_char( "Tell whom what?\r\n", ch );
    return;
  }

  if( ( victim = get_char_world( ch, arg ) ) == NULL
      || ( IS_NPC( victim ) && victim->in_room != ch->in_room )
      || ( !NOT_AUTHED( ch ) && NOT_AUTHED( victim ) && !IS_IMMORTAL( ch ) ) )
  {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }

  if( ch == victim )
  {
    send_to_char( "You have a nice little chat with yourself.\r\n", ch );
    return;
  }

  if( victim->in_room != ch->in_room && !IS_IMMORTAL( ch ) )
  {
    if( !character_has_comlink( ch ) )
    {
      send_to_char( "You need a comlink to do that!\r\n", ch );
      return;
    }

    if( !character_has_comlink( victim ) )
    {
      send_to_char( "They don't seem to have a comlink!\r\n", ch );
      return;
    }
  }

  if( NOT_AUTHED( ch ) && !NOT_AUTHED( victim ) && !IS_IMMORTAL( victim ) )
  {
    send_to_char
      ( "They can't hear you because you have not yet finished the academy.\r\n",
	ch );
    return;
  }

  if( !IS_NPC( victim ) && ( victim->switched )
      && ( IS_IMMORTAL( ch ) )
      && !IS_SET( victim->switched->act, ACT_POLYMORPHED )
      && !IS_AFFECTED( victim->switched, AFF_POSSESS ) )
  {
    send_to_char( "That player is switched.\r\n", ch );
    return;
  }
  else if( !IS_NPC( victim ) && ( victim->switched )
      && ( IS_SET( victim->switched->act, ACT_POLYMORPHED )
	|| IS_AFFECTED( victim->switched, AFF_POSSESS ) ) )
  {
    switched_victim = victim->switched;
  }
  else if( !IS_NPC( victim ) && ( !victim->desc ) )
  {
    send_to_char( "That player is link-dead.\r\n", ch );
    return;
  }

  if( !IS_NPC( victim ) && ( IS_SET( victim->act, PLR_AFK ) ) )
  {
    send_to_char( "That player is afk.\r\n", ch );
    return;
  }

  if( IS_SET( victim->deaf, CHANNEL_TELLS )
      && ( !IS_IMMORTAL( ch ) || ( get_trust( ch ) < get_trust( victim ) ) ) )
  {
    act( AT_PLAIN, "$E has $S tells turned off.", ch, NULL, victim,
	TO_CHAR );
    return;
  }

  if( !IS_NPC( victim ) && ( IS_SET( victim->act, PLR_SILENCE ) ) )
  {
    send_to_char
      ( "That player is silenced.  They will receive your message but can not respond.\r\n",
	ch );
  }

  if( !IS_IMMORTAL( ch ) && !IS_AWAKE( victim ) )
  {
    act( AT_PLAIN, "$E can't hear you.", ch, 0, victim, TO_CHAR );
    return;
  }

  if( victim->desc		/* make sure desc exists first  -Thoric */
      && victim->desc->connected == CON_EDITING && IS_IMMORTAL( ch ) )
  {
    act( AT_PLAIN,
	"$E is currently in a writing buffer.  Please try again in a few minutes.",
	ch, 0, victim, TO_CHAR );
    return;
  }

  if( switched_victim )
    victim = switched_victim;

  snprintf( buf, MAX_STRING_LENGTH, "You tell %s '$t'", victim->name );
  act( AT_TELL, buf, ch, argument, victim, TO_CHAR );
  position = victim->position;
  victim->position = POS_STANDING;
  snprintf( buf, MAX_STRING_LENGTH, "%s tells you '$t'", ch->name );
  act( AT_TELL, buf, ch, argument, victim, TO_VICT );
  victim->position = position;
  victim->reply = ch;
  mprog_speech_trigger( argument, ch );
}

void do_reply( CHAR_DATA * ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim = NULL;
  short position = 0;
  REMOVE_BIT( ch->deaf, CHANNEL_TELLS );

  if( !IS_NPC( ch ) && IS_SET( ch->act, PLR_SILENCE ) )
  {
    send_to_char( "Your message didn't get through.\r\n", ch );
    return;
  }

  if( ( victim = ch->reply ) == NULL )
  {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }

  if( !IS_NPC( victim ) && ( victim->switched )
      && can_see( ch, victim ) && IS_IMMORTAL( ch ) )
  {
    send_to_char( "That player is switched.\r\n", ch );
    return;
  }
  else if( !IS_NPC( victim ) && ( !victim->desc ) )
  {
    send_to_char( "That player is link-dead.\r\n", ch );
    return;
  }

  if( !IS_NPC( victim ) && ( IS_SET( victim->act, PLR_AFK ) ) )
  {
    send_to_char( "That player is afk.\r\n", ch );
    return;
  }

  if( IS_SET( victim->deaf, CHANNEL_TELLS )
      && ( !IS_IMMORTAL( ch ) || ( get_trust( ch ) < get_trust( victim ) ) ) )
  {
    act( AT_PLAIN, "$E has $S tells turned off.", ch, NULL, victim,
	TO_CHAR );
    return;
  }

  if( !IS_IMMORTAL( ch ) && !IS_AWAKE( victim ) )
  {
    act( AT_PLAIN, "$E can't hear you.", ch, 0, victim, TO_CHAR );
    return;
  }

  if( victim->in_room != ch->in_room && !IS_IMMORTAL( ch ) )
  {
    if( !character_has_comlink( ch ) )
    {
      send_to_char( "You need a comlink to do that!\r\n", ch );
      return;
    }

    if( !character_has_comlink( victim ) )
    {
      send_to_char( "They don't seem to have a comlink!\r\n", ch );
      return;
    }
  }

  snprintf( buf, MAX_STRING_LENGTH, "You tell %s '$t'", victim->name );
  act( AT_TELL, buf, ch, argument, victim, TO_CHAR );
  position = victim->position;
  victim->position = POS_STANDING;
  snprintf( buf, MAX_STRING_LENGTH, "%s tells you '$t'", ch->name );
  act( AT_TELL, buf, ch, argument, victim, TO_VICT );
  victim->position = position;
  victim->reply = ch;
}



void do_emote( CHAR_DATA * ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  const char *plast = NULL;
  int actflags = ch->act;

  if( !IS_NPC( ch ) && IS_SET( ch->act, PLR_NO_EMOTE ) )
  {
    send_to_char( "You can't show your emotions.\r\n", ch );
    return;
  }

  if( argument[0] == '\0' )
  {
    send_to_char( "Emote what?\r\n", ch );
    return;
  }

  if( IS_NPC( ch ) )
    REMOVE_BIT( ch->act, ACT_SECRETIVE );

  for( plast = argument; *plast != '\0'; plast++ )
    ;

  strcpy( buf, argument );

  if( isalpha( ( int ) plast[-1] ) )
    strcat( buf, "." );

  MOBtrigger = FALSE;
  act( AT_ACTION, "$n $T", ch, NULL, buf, TO_ROOM );
  MOBtrigger = FALSE;
  act( AT_ACTION, "$n $T", ch, NULL, buf, TO_CHAR );
  ch->act = actflags;
}

void do_bug( CHAR_DATA * ch, char *argument )
{
  if( !argument || argument[0] == '\0' )
  {
    send_to_char( "What do you want to submit as a bug?\n\r", ch );
    return;
  }

  append_file( ch, BUG_FILE, argument );
  send_to_char( "Ok.  Thanks.\n\r", ch );
}

void do_typo( CHAR_DATA * ch, char *argument )
{
  if( !argument || argument[0] == '\0' )
  {
    send_to_char( "What do you want to submit as an idea?\n\r", ch );
    return;
  }

  append_file( ch, TYPO_FILE, argument );
  send_to_char( "Ok.  Thanks.\r\n", ch );
}

void do_quit( CHAR_DATA * ch, char *argument )
{
  size_t x = 0, y = 0;

  if( IS_NPC( ch ) && IS_SET( ch->act, ACT_POLYMORPHED ) )
  {
    send_to_char( "You can't quit while polymorphed.\r\n", ch );
    return;
  }

  if( IS_NPC( ch ) )
    return;

  if( ch->position == POS_FIGHTING )
  {
    set_char_color( AT_RED, ch );
    send_to_char( "No way! You are fighting.\r\n", ch );
    return;
  }

  if( ch->position < POS_STUNNED )
  {
    set_char_color( AT_BLOOD, ch );
    send_to_char( "You're not DEAD yet.\r\n", ch );
    return;
  }

  if( auction->item != NULL
      && ( ( ch == auction->buyer ) || ( ch == auction->seller ) ) )
  {
    send_to_char
      ( "Wait until you have bought/sold the item on auction.\r\n", ch );
    return;
  }

  if( !IS_IMMORTAL( ch ) && ch->in_room
      && !IS_SET( ch->in_room->room_flags, ROOM_HOTEL ) && !NOT_AUTHED( ch ) )
  {
    send_to_char( "You may not quit here.\r\n", ch );
    send_to_char
      ( "You will have to find a safer resting place such as a hotel...\r\n",
	ch );
    send_to_char( "Maybe you could HAIL a speeder.\r\n", ch );
    return;
  }

  set_char_color( AT_WHITE, ch );
  send_to_char
    ( "Your surroundings begin to fade as a mystical swirling vortex of colors\r\nenvelops your body... When you come to, things are not as they were.\r\n\r\n",
      ch );
  act( AT_SAY, "A strange voice says, 'We await your return, $n...'", ch,
      NULL, NULL, TO_CHAR );
  act( AT_BYE, "$n has left the game.", ch, NULL, NULL, TO_ROOM );
  set_char_color( AT_GREY, ch );

  snprintf( log_buf, MAX_STRING_LENGTH, "%s has quit.", ch->name );
  quitting_char = ch;
  save_char_obj( ch );
  save_home( ch );
  saving_char = NULL;

  /*
   * After extract_char the ch is no longer valid!
   */
  extract_char( ch, TRUE );

  for( x = 0; x < MAX_WEAR; x++ )
    for( y = 0; y < MAX_LAYERS; y++ )
      save_equipment[x][y] = NULL;

  /* don't show who's logging off to leaving player */
  log_string_plus( log_buf, LOG_COMM );
}

void do_ansi( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];

  one_argument( argument, arg );

  if( arg[0] == '\0' )
  {
    send_to_char( "ANSI ON or OFF?\r\n", ch );
    return;
  }

  if( ( strcmp( arg, "on" ) == 0 ) || ( strcmp( arg, "ON" ) == 0 ) )
  {
    SET_BIT( ch->act, PLR_ANSI );
    set_char_color( AT_WHITE + AT_BLINK, ch );
    send_to_char( "ANSI ON!!!\r\n", ch );
    return;
  }

  if( ( strcmp( arg, "off" ) == 0 ) || ( strcmp( arg, "OFF" ) == 0 ) )
  {
    REMOVE_BIT( ch->act, PLR_ANSI );
    send_to_char( "Okay... ANSI support is now off\r\n", ch );
    return;
  }
}

void do_save( CHAR_DATA * ch, char *argument )
{
  if( IS_NPC( ch ) && IS_SET( ch->act, ACT_POLYMORPHED ) )
  {
    send_to_char( "You can't save while polymorphed.\r\n", ch );
    return;
  }

  if( IS_NPC( ch ) )
    return;

  if( NOT_AUTHED( ch ) )
  {
    send_to_char
      ( "You can't save untill after you've graduated from the acadamey.\r\n",
	ch );
    return;
  }

  save_char_obj( ch );
  save_home( ch );
  saving_char = NULL;
  send_to_char( "Ok.\r\n", ch );
}

/*
 * Something from original DikuMUD that Merc yanked out.
 * Used to prevent following loops, which can cause problems if people
 * follow in a loop through an exit leading back into the same room
 * (Which exists in many maze areas)			-Thoric
 */
bool circle_follow( const CHAR_DATA * ch, const CHAR_DATA * victim )
{
  const CHAR_DATA *tmp = NULL;

  for( tmp = victim; tmp; tmp = tmp->master )
    if( tmp == ch )
      return TRUE;

  return FALSE;
}

void do_follow( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;

  one_argument( argument, arg );

  if( arg[0] == '\0' )
  {
    send_to_char( "Follow whom?\r\n", ch );
    return;
  }

  if( ( victim = get_char_room( ch, arg ) ) == NULL )
  {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }

  if( IS_AFFECTED( ch, AFF_CHARM ) && ch->master )
  {
    act( AT_PLAIN, "But you'd rather follow $N!", ch, NULL, ch->master,
	TO_CHAR );
    return;
  }

  if( victim == ch )
  {
    if( !ch->master )
    {
      send_to_char( "You already follow yourself.\r\n", ch );
      return;
    }

    stop_follower( ch );
    return;
  }

  if( circle_follow( ch, victim ) )
  {
    send_to_char( "Following in loops is not allowed... sorry.\r\n", ch );
    return;
  }

  if( ch->master )
    stop_follower( ch );

  add_follower( ch, victim );
}

void add_follower( CHAR_DATA * ch, CHAR_DATA * master )
{
  if( ch->master )
  {
    bug( "Add_follower: non-null master.", 0 );
    return;
  }

  ch->master = master;
  ch->leader = NULL;

  if( can_see( master, ch ) )
    act( AT_ACTION, "$n now follows you.", ch, NULL, master, TO_VICT );

  act( AT_ACTION, "You now follow $N.", ch, NULL, master, TO_CHAR );
}

void stop_follower( CHAR_DATA * ch )
{
  if( !ch->master )
  {
    bug( "Stop_follower: null master.", 0 );
    return;
  }

  if( IS_AFFECTED( ch, AFF_CHARM ) )
  {
    REMOVE_BIT( ch->affected_by, AFF_CHARM );
    affect_strip( ch, gsn_charm_person );
  }

  if( can_see( ch->master, ch ) )
    act( AT_ACTION, "$n stops following you.", ch, NULL, ch->master,
	TO_VICT );

  act( AT_ACTION, "You stop following $N.", ch, NULL, ch->master, TO_CHAR );

  ch->master = NULL;
  ch->leader = NULL;
}

void die_follower( CHAR_DATA * ch )
{
  CHAR_DATA *fch = NULL;

  if( ch->master )
    stop_follower( ch );

  ch->leader = NULL;

  for( fch = first_char; fch; fch = fch->next )
  {
    if( fch->master == ch )
      stop_follower( fch );
    if( fch->leader == ch )
      fch->leader = fch;
  }
}

void do_order( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  char argbuf[MAX_INPUT_LENGTH];
  CHAR_DATA *victim = NULL;
  CHAR_DATA *och = NULL;
  CHAR_DATA *och_next = NULL;
  bool found = FALSE;
  bool fAll = FALSE;
  int toomany = 0;

  strcpy( argbuf, argument );
  argument = one_argument( argument, arg );

  if( arg[0] == '\0' || argument[0] == '\0' )
  {
    send_to_char( "Order whom to do what?\r\n", ch );
    return;
  }

  if( IS_AFFECTED( ch, AFF_CHARM ) )
  {
    send_to_char( "You feel like taking, not giving, orders.\r\n", ch );
    return;
  }

  if( !str_cmp( arg, "all" ) )
  {
    fAll = TRUE;
    victim = NULL;
  }
  else
  {
    fAll = FALSE;

    if( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
      send_to_char( "They aren't here.\r\n", ch );
      return;
    }

    if( victim == ch )
    {
      send_to_char( "Aye aye, right away!\r\n", ch );
      return;
    }

    if( !IS_AFFECTED( victim, AFF_CHARM ) || victim->master != ch )
    {
      send_to_char( "Do it yourself!\r\n", ch );
      return;
    }
  }

  found = FALSE;

  for( och = ch->in_room->first_person; och; och = och_next )
  {
    och_next = och->next_in_room;

    if( IS_AFFECTED( och, AFF_CHARM )
	&& och->master == ch
	&& ( fAll || och == victim ) && !IS_IMMORTAL( och ) )
    {
      found = TRUE;
      act( AT_ACTION, "$n orders you to '$t'.", ch, argument, och,
	  TO_VICT );
      interpret( och, argument );
    }

    if( toomany++ > 100 )
      break;
  }

  if( found )
  {
    snprintf( log_buf, MAX_STRING_LENGTH, "%s: order %s.", ch->name, argbuf );
    log_string_plus( log_buf, LOG_NORMAL );
    send_to_char( "Ok.\r\n", ch );
    WAIT_STATE( ch, 12 );
  }
  else
  {
    send_to_char( "You have no followers here.\r\n", ch );
  }
}

void do_group( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim = 0;

  one_argument( argument, arg );

  if( arg[0] == '\0' )
  {
    CHAR_DATA *gch = 0;
    CHAR_DATA *leader = ch->leader ? ch->leader : ch;
    set_char_color( AT_GREEN, ch );
    ch_printf( ch, "%s's group:\r\n", PERS( leader, ch ) );

    for( gch = first_char; gch; gch = gch->next )
    {
      if( is_same_group( gch, ch ) )
      {
	set_char_color( AT_DGREEN, ch );
	ch_printf( ch, "%s\r\n", capitalize( PERS( gch, ch ) ) );
      }
    }

    return;
  }

  if( !strcmp( arg, "disband" ) )
  {
    CHAR_DATA *gch;
    int count = 0;

    if( ch->leader || ch->master )
    {
      send_to_char
	( "You cannot disband a group if you're following someone.\r\n",
	  ch );
      return;
    }

    for( gch = first_char; gch; gch = gch->next )
    {
      if( is_same_group( ch, gch ) && ( ch != gch ) )
      {
	gch->leader = NULL;
	gch->master = NULL;
	count++;
	send_to_char( "Your group is disbanded.\r\n", gch );
      }
    }

    if( count == 0 )
      send_to_char( "You have no group members to disband.\r\n", ch );
    else
      send_to_char( "You disband your group.\r\n", ch );

    return;
  }

  if( !strcmp( arg, "all" ) )
  {
    CHAR_DATA *rch;
    int count = 0;

    for( rch = ch->in_room->first_person; rch; rch = rch->next_in_room )
    {
      if( ch != rch
	  && !IS_NPC( rch )
	  && rch->master == ch
	  && !ch->master && !ch->leader && !is_same_group( rch, ch ) )
      {
	rch->leader = ch;
	count++;
      }
    }

    if( count == 0 )
    {
      send_to_char( "You have no eligible group members.\r\n", ch );
    }
    else
    {
      act( AT_ACTION, "$n groups $s followers.", ch, NULL, victim,
	  TO_ROOM );
      send_to_char( "You group your followers.\r\n", ch );
    }

    return;
  }

  if( ( victim = get_char_room( ch, arg ) ) == NULL )
  {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }

  if( ch->master || ( ch->leader && ch->leader != ch ) )
  {
    send_to_char( "But you are following someone else!\r\n", ch );
    return;
  }

  if( victim->master != ch && ch != victim )
  {
    act( AT_PLAIN, "$N isn't following you.", ch, NULL, victim, TO_CHAR );
    return;
  }

  if( is_same_group( victim, ch ) && ch != victim )
  {
    victim->leader = NULL;
    act( AT_ACTION, "$n removes $N from $s group.", ch, NULL, victim,
	TO_NOTVICT );
    act( AT_ACTION, "$n removes you from $s group.", ch, NULL, victim,
	TO_VICT );
    act( AT_ACTION, "You remove $N from your group.", ch, NULL, victim,
	TO_CHAR );
    return;
  }

  victim->leader = ch;
  act( AT_ACTION, "$N joins $n's group.", ch, NULL, victim, TO_NOTVICT );
  act( AT_ACTION, "You join $n's group.", ch, NULL, victim, TO_VICT );
  act( AT_ACTION, "$N joins your group.", ch, NULL, victim, TO_CHAR );
}

/*
 * 'Split' originally by Gnort, God of Chaos.
 */
void do_split( CHAR_DATA * ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *gch = NULL;
  int members = 0;
  int amount = 0;
  int share = 0;
  int extra = 0;

  one_argument( argument, arg );

  if( arg[0] == '\0' )
  {
    send_to_char( "Split how much?\r\n", ch );
    return;
  }

  amount = atoi( arg );

  if( amount < 0 )
  {
    send_to_char( "Your group wouldn't like that.\r\n", ch );
    return;
  }

  if( amount == 0 )
  {
    send_to_char( "You hand out zero credits, but no one notices.\r\n",
	ch );
    return;
  }

  if( ch->gold < amount )
  {
    send_to_char( "You don't have that many credits.\r\n", ch );
    return;
  }

  members = 0;

  for( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
  {
    if( is_same_group( gch, ch ) )
      members++;
  }


  if( ( IS_SET( ch->act, PLR_AUTOGOLD ) ) && ( members < 2 ) )
    return;

  if( members < 2 )
  {
    send_to_char( "Just keep it all.\r\n", ch );
    return;
  }

  share = amount / members;
  extra = amount % members;

  if( share == 0 )
  {
    send_to_char( "Don't even bother, cheapskate.\r\n", ch );
    return;
  }

  ch->gold -= amount;
  ch->gold += share + extra;

  set_char_color( AT_GOLD, ch );
  ch_printf( ch,
      "You split %d credits.  Your share is %d credits.\r\n",
      amount, share + extra );

  snprintf( buf, MAX_STRING_LENGTH, "$n splits %d credits. Your share is %d credits.",
      amount, share );

  for( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
  {
    if( gch != ch && is_same_group( gch, ch ) )
    {
      act( AT_GOLD, buf, ch, NULL, gch, TO_VICT );
      gch->gold += share;
    }
  }
}

void do_gtell( CHAR_DATA * ch, char *argument )
{
  CHAR_DATA *gch = NULL;

  if( argument[0] == '\0' )
  {
    send_to_char( "Tell your group what?\r\n", ch );
    return;
  }

  if( IS_SET( ch->act, PLR_NO_TELL ) )
  {
    send_to_char( "Your message didn't get through!\r\n", ch );
    return;
  }

  /*
   * Note use of send_to_char, so gtell works on sleepers.
   */
  for( gch = first_char; gch; gch = gch->next )
  {
    if( is_same_group( gch, ch ) )
    {
      set_char_color( AT_GTELL, gch );
      ch_printf( gch, "%s tells the group '%s'.\r\n", ch->name,
	  argument );
    }
  }
}

/*
 * It is very important that this be an equivalence relation:
 * (1) A ~ A
 * (2) if A ~ B then B ~ A
 * (3) if A ~ B  and B ~ C, then A ~ C
 */
bool is_same_group( const CHAR_DATA * ach, const CHAR_DATA * bch )
{
  if( ach->leader )
    ach = ach->leader;

  if( bch->leader )
    bch = bch->leader;

  return ach == bch;
}

/*
 * this function sends raw argument over the AUCTION: channel
 * I am not too sure if this method is right..
 */

void talk_auction( const char *argument )
{
  DESCRIPTOR_DATA *d = NULL;
  char buf[MAX_STRING_LENGTH];

  snprintf( buf, MAX_STRING_LENGTH, "Auction: %s", argument );	/* last %s to reset color */

  for( d = first_descriptor; d; d = d->next )
  {
    CHAR_DATA *original = d->original ? d->original : d->character;	/* if switched */
    if( ( d->connected == CON_PLAYING )
	&& !IS_SET( original->deaf, CHANNEL_AUCTION )
	&& !NOT_AUTHED( original ) )
      act( AT_GOSSIP, buf, original, NULL, NULL, TO_CHAR );
  }
}
