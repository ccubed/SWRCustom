#include <stdio.h>
#include <string.h>
#include "mud.h"

bool remove_obj( CHAR_DATA * ch, int iWear, bool fReplace );

/*
 * The following special functions are available for mobiles.
 */
DECLARE_SPEC_FUN( spec_fido );
DECLARE_SPEC_FUN( spec_guardian );
DECLARE_SPEC_FUN( spec_janitor );
DECLARE_SPEC_FUN( spec_poison );
DECLARE_SPEC_FUN( spec_thief );
DECLARE_SPEC_FUN( spec_auth );
DECLARE_SPEC_FUN( spec_clan_guard );
DECLARE_SPEC_FUN( spec_ship_guard );

typedef struct spec_fun_entry
{
  const char *fun_name;
  SPEC_FUN *fun_ptr;
} SPEC_FUN_ENTRY;

static const SPEC_FUN_ENTRY spec_fun_table[] = {
  { "spec_fido",       spec_fido },
  { "spec_guardian",   spec_guardian },
  { "spec_janitor",    spec_janitor },
  { "spec_poison",     spec_poison },
  { "spec_thief",      spec_thief },
  { "spec_auth",       spec_auth },
  { "spec_clan_guard", spec_clan_guard },
  { "spec_ship_guard", spec_ship_guard }
};

static size_t spec_fun_table_size( void )
{
  return sizeof( spec_fun_table ) / sizeof( *spec_fun_table );
}

/*
 * Given a name, return the appropriate spec fun.
 */
SPEC_FUN *spec_lookup( const char *name )
{
  SPEC_FUN *fun_ptr = NULL;
  size_t i = 0;

  for( i = 0; i < spec_fun_table_size(); ++i )
    {
      if( !str_cmp( name, spec_fun_table[i].fun_name ) )
	{
	  fun_ptr = spec_fun_table[i].fun_ptr;
	  break;
	}
    }

  return fun_ptr;
}

/*
 * Given a pointer, return the appropriate spec fun text.
 */
const char *lookup_spec( SPEC_FUN *special )
{
  size_t i = 0;

  for( i = 0; i < spec_fun_table_size(); ++i )
    {
      if( spec_fun_table[i].fun_ptr == special )
	{
	  return spec_fun_table[i].fun_name;
	}
    }

  return "";
}

bool spec_clan_guard( CHAR_DATA * ch )
{
  CHAR_DATA *victim = NULL;
  CHAR_DATA *v_next = NULL;

  if( !IS_AWAKE( ch ) || ch->fighting || !ch->mob_clan )
    return FALSE;

  for( victim = ch->in_room->first_person; victim; victim = v_next )
  {
    v_next = victim->next_in_room;
    if( !can_see( ch, victim ) )
      continue;
    if( get_timer( victim, TIMER_RECENTFIGHT ) > 0 )
      continue;
    if( !IS_NPC( victim ) && victim->pcdata && victim->pcdata->clan
	&& IS_AWAKE( victim ) && victim->pcdata->clan != ch->mob_clan
	&& nifty_is_name( victim->pcdata->clan->name,
	  ch->mob_clan->atwar ) )
    {
      char buf[MAX_STRING_LENGTH];
      snprintf( buf, MAX_STRING_LENGTH, "%s",
		"Hey you're not allowed in here!" );
      do_yell( ch, buf );
      multi_hit( ch, victim, TYPE_UNDEFINED );
      return TRUE;
    }
  }

  return FALSE;
}

bool spec_ship_guard( CHAR_DATA * ch )
{
  CHAR_DATA *victim = NULL;
  CHAR_DATA *v_next = NULL;

  if( !IS_AWAKE( ch ) || ch->fighting || !ch->mob_clan )
    return FALSE;

  for( victim = ch->in_room->first_person; victim; victim = v_next )
  {
    v_next = victim->next_in_room;
    if( !can_see( ch, victim ) )
      continue;
    if( get_timer( victim, TIMER_RECENTFIGHT ) > 0 )
      continue;
    if( !IS_NPC( victim ) && victim->pcdata && victim->pcdata->clan
	&& IS_AWAKE( victim ) && victim->pcdata->clan != ch->mob_clan
	&& nifty_is_name( victim->pcdata->clan->name,
	  ch->mob_clan->atwar ) )
    {
      char buf[MAX_STRING_LENGTH];
      snprintf( buf, MAX_STRING_LENGTH, "%s",
                "Hey you're not allowed in here!" );
      do_yell( ch, buf );
      multi_hit( ch, victim, TYPE_UNDEFINED );
      return TRUE;
    }
  }

  return FALSE;
}


bool spec_fido( CHAR_DATA * ch )
{
  OBJ_DATA *corpse = NULL;
  OBJ_DATA *c_next = NULL;
  OBJ_DATA *obj = NULL;
  OBJ_DATA *obj_next = NULL;

  if( !IS_AWAKE( ch ) )
    return FALSE;

  for( corpse = ch->in_room->first_content; corpse; corpse = c_next )
  {
    c_next = corpse->next_content;
    if( corpse->item_type != ITEM_CORPSE_NPC )
      continue;

    act( AT_ACTION, "$n savagely devours a corpse.", ch, NULL, NULL,
	TO_ROOM );
    for( obj = corpse->first_content; obj; obj = obj_next )
    {
      obj_next = obj->next_content;
      obj_from_obj( obj );
      obj_to_room( obj, ch->in_room );
    }
    extract_obj( corpse );
    return TRUE;
  }

  return FALSE;
}


bool spec_guardian( CHAR_DATA * ch )
{
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim = NULL;
  CHAR_DATA *v_next = NULL;
  CHAR_DATA *ech = NULL;
  const char *crime = "";
  int max_evil = 300;

  if( !IS_AWAKE( ch ) || ch->fighting )
    return FALSE;

  for( victim = ch->in_room->first_person; victim; victim = v_next )
  {
    v_next = victim->next_in_room;
    if( victim->fighting
	&& who_fighting( victim ) != ch && victim->alignment < max_evil )
    {
      max_evil = victim->alignment;
      ech = victim;
    }
  }

  if( victim && IS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
  {
    snprintf( buf, MAX_STRING_LENGTH, "%s is a %s! As well as a COWARD!",
	      victim->name, crime );
    do_yell( ch, buf );
    return TRUE;
  }

  if( victim )
  {
    snprintf( buf, MAX_STRING_LENGTH, "%s is a %s! PROTECT THE INNOCENT!!",
	      victim->name, crime );
    do_yell( ch, buf );
    multi_hit( ch, victim, TYPE_UNDEFINED );
    return TRUE;
  }

  if( ech )
  {
    act( AT_YELL, "$n screams 'PROTECT THE INNOCENT!'",
	ch, NULL, NULL, TO_ROOM );
    multi_hit( ch, ech, TYPE_UNDEFINED );
    return TRUE;
  }

  return FALSE;
}



bool spec_janitor( CHAR_DATA * ch )
{
  OBJ_DATA *trash = NULL;
  OBJ_DATA *trash_next = NULL;

  if( !IS_AWAKE( ch ) )
    return FALSE;

  for( trash = ch->in_room->first_content; trash; trash = trash_next )
  {
    trash_next = trash->next_content;

    if( !IS_SET( trash->wear_flags, ITEM_TAKE )
	|| IS_OBJ_STAT( trash, ITEM_BURRIED ) )
    {
      continue;
    }

    if( IS_OBJ_STAT( trash, ITEM_PROTOTYPE )
	&& !IS_SET( ch->act, ACT_PROTOTYPE ) )
    {
      continue;
    }

    if( trash->item_type == ITEM_DRINK_CON
	|| trash->item_type == ITEM_TRASH
	|| trash->cost < 10
	|| ( trash->pIndexData->vnum == OBJ_VNUM_SHOPPING_BAG
	  && !trash->first_content ) )
    {
      act( AT_ACTION, "$n picks up some trash.", ch, NULL, NULL,
	  TO_ROOM );
      obj_from_room( trash );
      obj_to_char( trash, ch );
      return TRUE;
    }
  }

  return FALSE;
}



bool spec_poison( CHAR_DATA * ch )
{
  CHAR_DATA *victim = NULL;

  if( ch->position != POS_FIGHTING
      || ( victim = who_fighting( ch ) ) == NULL
      || number_percent() > 2 * ch->top_level )
    return FALSE;

  act( AT_HIT, "You bite $N!", ch, NULL, victim, TO_CHAR );
  act( AT_ACTION, "$n bites $N!", ch, NULL, victim, TO_NOTVICT );
  act( AT_POISON, "$n bites you!", ch, NULL, victim, TO_VICT );
  spell_poison( gsn_poison, ch->top_level, ch, victim );
  return TRUE;
}



bool spec_thief( CHAR_DATA * ch )
{
  CHAR_DATA *victim = NULL;
  CHAR_DATA *v_next = NULL;
  int gold = 0, maxgold = 0;

  if( ch->position != POS_STANDING )
    return FALSE;

  for( victim = ch->in_room->first_person; victim; victim = v_next )
  {
    v_next = victim->next_in_room;

    if( IS_NPC( victim ) || IS_IMMORTAL( victim )
	|| number_bits( 2 ) != 0 || !can_see( ch, victim ) ) /* Thx Glop */
      continue;

    if( IS_AWAKE( victim ) && number_range( 0, ch->top_level ) == 0 )
    {
      act( AT_ACTION, "You discover $n's hands in your wallet!",
	  ch, NULL, victim, TO_VICT );
      act( AT_ACTION, "$N discovers $n's hands in $S wallet!",
	  ch, NULL, victim, TO_NOTVICT );
      return TRUE;
    }
    else
    {
      maxgold = ch->top_level * ch->top_level * 1000;
      gold = victim->gold
	* number_range( 1, URANGE( 2, ch->top_level / 4, 10 ) ) / 100;
      ch->gold += 9 * gold / 10;
      victim->gold -= gold;
      if( ch->gold > maxgold )
      {
	ch->gold = maxgold / 2;
      }
      return TRUE;
    }
  }

  return FALSE;
}

bool spec_auth( CHAR_DATA * ch )
{
  CHAR_DATA *victim = NULL;
  CHAR_DATA *v_next = NULL;
  char buf[MAX_STRING_LENGTH];

  for( victim = ch->in_room->first_person; victim; victim = v_next )
  {
    v_next = victim->next_in_room;

    if( IS_NPC( victim )
	|| !IS_SET( victim->pcdata->flags, PCFLAG_UNAUTHED ) )
      continue;

    victim->pcdata->auth_state = 3;
    REMOVE_BIT( victim->pcdata->flags, PCFLAG_UNAUTHED );
    if( victim->pcdata->authed_by )
      STRFREE( victim->pcdata->authed_by );
    victim->pcdata->authed_by = QUICKLINK( ch->name );
    snprintf( buf, MAX_STRING_LENGTH,
	      "%s has graduated the academy.", victim->name );
    to_channel( buf, CHANNEL_MONITOR, "Monitor", ch->top_level );
    if( victim->pcdata->clan )
      victim->pcdata->clan->members++;
  }

  return FALSE;
}
