#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "mud.h"

void show_char_to_char( CHAR_DATA * list, CHAR_DATA * ch );
int ris_save( CHAR_DATA * ch, int chance, int ris );
bool check_illegal_psteal( CHAR_DATA * ch, CHAR_DATA * victim );

/* from magic.c */
void failed_casting( SKILLTYPE *skill, CHAR_DATA * ch,
    CHAR_DATA * victim, OBJ_DATA * obj );

int character_skill_level( const CHAR_DATA * ch, short skill )
{
  return IS_NPC( ch ) ? 100 : ( int ) ch->pcdata->learned[skill];
}

void set_skill_level( CHAR_DATA *ch, int sn, int lvl )
{
  if( !IS_NPC( ch ) )
    {
      ch->pcdata->learned[sn] = lvl;
    }
}

void modify_skill_level( CHAR_DATA *ch, int sn, int value )
{
  if( !IS_NPC( ch ) )
    {
      set_skill_level( ch, sn, character_skill_level( ch, sn ) + value );
    }
}

/*
 * Dummy function
 */
void skill_notfound( CHAR_DATA * ch, char *argument )
{
  send_to_char( "Huh?\r\n", ch );
  return;
}

bool is_legal_kill( CHAR_DATA * ch, CHAR_DATA * vch )
{
  if( IS_NPC( ch ) || IS_NPC( vch ) )
    return TRUE;
  if( is_safe( ch, vch ) )
    return FALSE;
  return TRUE;
}


extern const char *target_name;	/* from magic.c */

/*
 * Perform a binary search on a section of the skill table
 * Each different section of the skill table is sorted alphabetically
 * Only match skills player knows				-Thoric
 */
bool check_skill( CHAR_DATA * ch, const char *command, char *argument )
{
  int sn = 0;
  int first = gsn_first_skill;
  int top = gsn_first_weapon - 1;
  struct timeval time_used;
  int mana = 0;
  SKILLTYPE *skill = NULL;

  /* bsearch for the skill */
  for( ;; )
  {
    sn = ( first + top ) >> 1;
    skill = skill_table[sn];

    if( LOWER( command[0] ) == LOWER( skill->name[0] )
	&& !str_prefix( command, skill->name )
	&& ( skill->skill_fun
	  || skill->spell_fun != spell_null )
	&& ( IS_NPC( ch ) || ( character_skill_level( ch, sn ) > 0 ) ) )
      break;

    if( first >= top )
      return FALSE;

    if( strcmp( command, skill->name ) < 1 )
      top = sn - 1;
    else
      first = sn + 1;
  }

  if( !check_pos( ch, skill->minimum_position ) )
    return TRUE;

  if( IS_NPC( ch )
      && ( IS_AFFECTED( ch, AFF_CHARM ) || IS_AFFECTED( ch, AFF_POSSESS ) ) )
  {
    send_to_char( "For some reason, you seem unable to perform that...\r\n",
	ch );
    act( AT_GREY, "$n looks around.", ch, NULL, NULL, TO_ROOM );
    return TRUE;
  }

  /* check if mana is required */
  if( skill->min_mana )
  {
    mana = IS_NPC( ch ) ? 0 : skill->min_mana;

    if( !IS_NPC( ch ) && ch->mana < mana )
    {
      send_to_char
	( "You need to rest before using the Force any more.\r\n", ch );
      return TRUE;
    }
  }
  else
  {
    mana = 0;
  }

  /*
   * Is this a real do-fun, or a really a spell?
   */
  if( !skill->skill_fun )
  {
    ch_ret retcode = rNONE;
    void *vo = NULL;
    CHAR_DATA *victim = NULL;
    OBJ_DATA *obj = NULL;

    target_name = "";

    switch ( skill->target )
    {
      default:
	bug( "Check_skill: bad target for sn %d.", sn );
	send_to_char( "Something went wrong...\r\n", ch );
	return TRUE;

      case TAR_IGNORE:
	vo = NULL;

	if( argument[0] == '\0' )
	{
	  if( ( victim = who_fighting( ch ) ) != NULL )
	    target_name = victim->name;
	}
	else
	{
	  target_name = argument;
	}

	break;

      case TAR_CHAR_OFFENSIVE:
	if( argument[0] == '\0' && ( victim = who_fighting( ch ) ) == NULL )
	{
	  ch_printf( ch, "%s who?\r\n",
	      capitalize( skill->name ) );
	  return TRUE;
	}
	else if( argument[0] != '\0'
	    && ( victim = get_char_room( ch, argument ) ) == NULL )
	{
	  send_to_char( "They aren't here.\r\n", ch );
	  return TRUE;
	}

	if( is_safe( ch, victim ) )
	  return TRUE;

	vo = ( void * ) victim;
	break;

      case TAR_CHAR_DEFENSIVE:
	if( argument[0] != '\0'
	    && ( victim = get_char_room( ch, argument ) ) == NULL )
	{
	  send_to_char( "They aren't here.\r\n", ch );
	  return TRUE;
	}

	if( !victim )
	  victim = ch;

	vo = ( void * ) victim;
	break;

      case TAR_CHAR_SELF:
	vo = ( void * ) ch;
	break;

      case TAR_OBJ_INV:
	if( ( obj = get_obj_carry( ch, argument ) ) == NULL )
	{
	  send_to_char( "You can't find that.\r\n", ch );
	  return TRUE;
	}

	vo = ( void * ) obj;
	break;
    }

    /* waitstate */
    WAIT_STATE( ch, skill->beats );

    /* check for failure */
    if( ( number_percent() + skill->difficulty * 5 )
	> ( IS_NPC( ch ) ? 75 : character_skill_level( ch, sn ) ) )
    {
      failed_casting( skill, ch, ( CHAR_DATA * ) vo, obj );
      learn_from_failure( ch, sn );

      if( mana )
      {
	ch->mana -= mana / 2;
      }

      return TRUE;
    }

    if( mana )
    {
      ch->mana -= mana;
    }

    start_timer( &time_used );


    retcode = ( *skill->spell_fun ) ( sn, ch->top_level, ch, vo );
    end_timer( &time_used );
    update_userec( &time_used, &skill->userec );

    if( retcode == rCHAR_DIED || retcode == rERROR )
      return TRUE;

    if( char_died( ch ) )
      return TRUE;

    if( retcode == rSPELL_FAILED )
    {
      learn_from_failure( ch, sn );
      retcode = rNONE;
    }
    else
    {
      learn_from_success( ch, sn );
    }

    if( skill->target == TAR_CHAR_OFFENSIVE
	&& victim != ch && !char_died( victim ) )
    {
      CHAR_DATA *vch = NULL;
      CHAR_DATA *vch_next = NULL;

      for( vch = ch->in_room->first_person; vch; vch = vch_next )
      {
	vch_next = vch->next_in_room;

	if( victim == vch && !victim->fighting && victim->master != ch )
	{
	  retcode = multi_hit( victim, ch, TYPE_UNDEFINED );
	  break;
	}
      }
    }

    return TRUE;
  }

  if( mana )
  {
    ch->mana -= mana;
  }

  ch->last_cmd = skill->skill_fun;
  start_timer( &time_used );
  ( *skill->skill_fun ) ( ch, argument );
  end_timer( &time_used );
  update_userec( &time_used, &skill->userec );

  return TRUE;
}

/*
 * Lookup a skills information
 * High god command
 */
void do_slookup( CHAR_DATA * ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  int sn = 0;
  SKILLTYPE *skill = NULL;

  one_argument( argument, arg );

  if( arg[0] == '\0' )
  {
    send_to_char( "Slookup what?\r\n", ch );
    return;
  }

  if( !str_cmp( arg, "all" ) )
  {
    for( sn = 0; sn < top_sn && skill_table[sn] && skill_table[sn]->name;
	sn++ )
      pager_printf( ch,
	  "Sn: %4d Slot: %4d Skill/spell: '%-20s' Damtype: %s\r\n",
	  sn, skill_table[sn]->slot, skill_table[sn]->name,
	  spell_damage[SPELL_DAMAGE( skill_table[sn] )] );
  }
  else
  {
    SMAUG_AFF *aff = NULL;
    int cnt = 0;

    if( is_number( arg ) )
    {
      sn = atoi( arg );

      if( ( skill = get_skilltype( sn ) ) == NULL )
      {
	send_to_char( "Invalid sn.\r\n", ch );
	return;
      }

      sn %= 1000;
    }
    else if( ( sn = skill_lookup( arg ) ) >= 0 )
    {
      skill = skill_table[sn];
    }
    else
    {
      send_to_char( "No such skill, spell or proficiency.\r\n", ch );
      return;
    }

    if( !skill )
    {
      send_to_char( "Not created yet.\r\n", ch );
      return;
    }

    ch_printf( ch, "Sn: %4d Slot: %4d %s: '%-20s'\r\n",
	sn, skill->slot, skill_tname[skill->type], skill->name );

    if( skill->flags )
    {
      int x = 0;

      ch_printf( ch,
	  "Damtype: %s  Acttype: %s   Classtype: %s   Powertype: %s\r\n",
	  spell_damage[SPELL_DAMAGE( skill )],
	  spell_action[SPELL_ACTION( skill )],
	  spell_class[SPELL_CLASS( skill )],
	  spell_power[SPELL_POWER( skill )] );
      strcpy( buf, "Flags:" );

      for( x = 11; x < 32; x++ )
      {
	if( SPELL_FLAG( skill, 1 << x ) )
	{
	  strcat( buf, " " );
	  strcat( buf, spell_flag[x - 11] );
	}
      }

      strcat( buf, "\r\n" );
      send_to_char( buf, ch );
    }

    ch_printf( ch, "Saves: %s\r\n", spell_saves[( int ) skill->saves] );

    if( skill->difficulty != '\0' )
      ch_printf( ch, "Difficulty: %d\r\n", ( int ) skill->difficulty );

    ch_printf( ch,
	"Type: %s  Target: %s  Minpos: %d  Mana: %d  Beats: %d\r\n",
	skill_tname[skill->type],
	target_type[URANGE
	( TAR_IGNORE, skill->target, TAR_OBJ_INV )],
	skill->minimum_position, skill->min_mana, skill->beats );
    ch_printf( ch, "Flags: %d  Code: %s\r\n", skill->flags,
	skill->skill_fun || skill->spell_fun ? skill->fun_name : "(none set)" );
    ch_printf( ch, "Dammsg: %s\r\nWearoff: %s\n", skill->noun_damage,
	skill->msg_off ? skill->msg_off : "(none set)" );
    if( skill->dice && skill->dice[0] != '\0' )
      ch_printf( ch, "Dice: %s\r\n", skill->dice );

    if( skill->components && skill->components[0] != '\0' )
      ch_printf( ch, "Components: %s\r\n", skill->components );

    if( skill->participants )
      ch_printf( ch, "Participants: %d\r\n", ( int ) skill->participants );

    if( skill->userec.num_uses )
      send_timer( &skill->userec, ch );

    for( aff = skill->affects; aff; aff = aff->next )
    {
      if( aff == skill->affects )
	send_to_char( "\r\n", ch );
      sprintf( buf, "Affect %d", ++cnt );

      if( aff->location )
      {
	strcat( buf, " modifies " );
	strcat( buf, a_types[aff->location % REVERSE_APPLY] );
	strcat( buf, " by '" );
	strcat( buf, aff->modifier );

	if( aff->bitvector )
	  strcat( buf, "' and" );
	else
	  strcat( buf, "'" );
      }

      if( aff->bitvector )
      {
	int x = 0;

	strcat( buf, " applies" );

	for( x = 0; x < 32; x++ )
	{
	  if( IS_SET( aff->bitvector, 1 << x ) )
	  {
	    strcat( buf, " " );
	    strcat( buf, a_flags[x] );
	  }
	}
      }

      if( aff->duration[0] != '\0' && aff->duration[0] != '0' )
      {
	strcat( buf, " for '" );
	strcat( buf, aff->duration );
	strcat( buf, "' rounds" );
      }

      if( aff->location >= REVERSE_APPLY )
	strcat( buf, " (affects caster only)" );

      strcat( buf, "\r\n" );
      send_to_char( buf, ch );

      if( !aff->next )
	send_to_char( "\r\n", ch );
    }

    if( skill->hit_char && skill->hit_char[0] != '\0' )
      ch_printf( ch, "Hitchar   : %s\r\n", skill->hit_char );

    if( skill->hit_vict && skill->hit_vict[0] != '\0' )
      ch_printf( ch, "Hitvict   : %s\r\n", skill->hit_vict );

    if( skill->hit_room && skill->hit_room[0] != '\0' )
      ch_printf( ch, "Hitroom   : %s\r\n", skill->hit_room );

    if( skill->miss_char && skill->miss_char[0] != '\0' )
      ch_printf( ch, "Misschar  : %s\r\n", skill->miss_char );

    if( skill->miss_vict && skill->miss_vict[0] != '\0' )
      ch_printf( ch, "Missvict  : %s\r\n", skill->miss_vict );

    if( skill->miss_room && skill->miss_room[0] != '\0' )
      ch_printf( ch, "Missroom  : %s\r\n", skill->miss_room );

    if( skill->die_char && skill->die_char[0] != '\0' )
      ch_printf( ch, "Diechar   : %s\r\n", skill->die_char );

    if( skill->die_vict && skill->die_vict[0] != '\0' )
      ch_printf( ch, "Dievict   : %s\r\n", skill->die_vict );

    if( skill->die_room && skill->die_room[0] != '\0' )
      ch_printf( ch, "Dieroom   : %s\r\n", skill->die_room );

    if( skill->imm_char && skill->imm_char[0] != '\0' )
      ch_printf( ch, "Immchar   : %s\r\n", skill->imm_char );

    if( skill->imm_vict && skill->imm_vict[0] != '\0' )
      ch_printf( ch, "Immvict   : %s\r\n", skill->imm_vict );

    if( skill->imm_room && skill->imm_room[0] != '\0' )
      ch_printf( ch, "Immroom   : %s\r\n", skill->imm_room );

    send_to_char( "\r\n", ch );
  }
}

/*
 * Set a skill's attributes or what skills a player has.
 * High god command, with support for creating skills/spells/etc
 */
void do_sset( CHAR_DATA * ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  CHAR_DATA *victim = NULL;
  int value = 0;
  int sn = 0;
  bool fAll = FALSE;

  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );

  if( arg1[0] == '\0' || arg2[0] == '\0' || argument[0] == '\0' )
  {
    send_to_char( "Syntax: sset <victim> <skill> <value>\r\n", ch );
    send_to_char( "or:     sset <victim> all     <value>\r\n", ch );
    if( IS_IMMORTAL( ch ) )
    {
      send_to_char( "or:     sset save skill table\r\n", ch );
      send_to_char( "or:     sset create skill 'new skill'\r\n", ch );
      send_to_char( "or:     sset <sn>     <field> <value>\r\n", ch );
      send_to_char( "\r\nField being one of:\r\n", ch );
      send_to_char
	( "  name code target minpos slot mana beats dammsg wearoff minlevel\r\n",
	  ch );
      send_to_char
	( "  type damtype acttype classtype powertype flag dice value difficulty affect\r\n",
	  ch );
      send_to_char
	( "  rmaffect level hit miss die imm (char/vict/room)\r\n",
	  ch );
      send_to_char( "  components\r\n", ch );
      send_to_char
	( "Affect having the fields: <location> <modfifier> [duration] [bitvector]\r\n",
	  ch );
      send_to_char
	( "(See AFFECTTYPES for location, and AFFECTED_BY for bitvector)\r\n",
	  ch );
    }
    send_to_char( "Skill being any skill or spell.\r\n", ch );
    return;
  }

  if( !str_cmp( arg1, "save" ) && !str_cmp( argument, "table" ) )
  {
    if( !str_cmp( arg2, "skill" ) )
    {
      send_to_char( "Saving skill table...\r\n", ch );
      save_skill_table();
      return;
    }
  }
  if( !str_cmp( arg1, "create" ) && !str_cmp( arg2, "skill" ) )
  {
    SKILLTYPE *skill = NULL;
    short type = SKILL_UNKNOWN;

    if( top_sn >= MAX_SKILL )
    {
      ch_printf( ch, "The current top sn is %d, which is the maximum.  "
	  "To add more skills,\r\nMAX_SKILL will have to be "
	  "raised in mud.h, and the mud recompiled.\r\n", top_sn );
      return;
    }

    skill = create_skill();
    skill_table[top_sn++] = skill;
    skill->name = str_dup( argument );
    skill->fun_name = str_dup( "" );
    skill->noun_damage = str_dup( "" );
    skill->msg_off = str_dup( "" );
    skill->spell_fun = spell_smaug;
    skill->type = type;
    send_to_char( "Done.\r\n", ch );
    return;
  }

  if( arg1[0] == 'h' )
    sn = atoi( arg1 + 1 );
  else
    sn = atoi( arg1 );
  if( ( ( arg1[0] == 'h' && is_number( arg1 + 1 )
	  && ( sn = atoi( arg1 + 1 ) ) >= 0 ) || ( is_number( arg1 )
	  && ( sn =
	    atoi( arg1 ) ) >=
	  0 ) ) )
  {
    SKILLTYPE *skill = NULL;

    if( ( skill = get_skilltype( sn ) ) == NULL )
    {
      send_to_char( "Skill number out of range.\r\n", ch );
      return;
    }
    sn %= 1000;

    if( !str_cmp( arg2, "difficulty" ) )
    {
      skill->difficulty = atoi( argument );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "participants" ) )
    {
      skill->participants = atoi( argument );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "alignment" ) )
    {
      skill->alignment = atoi( argument );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "damtype" ) )
    {
      int x = get_sdamage( argument );

      if( x == -1 )
	send_to_char( "Not a spell damage type.\r\n", ch );
      else
      {
	SET_SDAM( skill, x );
	send_to_char( "Ok.\r\n", ch );
      }
      return;
    }
    if( !str_cmp( arg2, "acttype" ) )
    {
      int x = get_saction( argument );

      if( x == -1 )
	send_to_char( "Not a spell action type.\r\n", ch );
      else
      {
	SET_SACT( skill, x );
	send_to_char( "Ok.\r\n", ch );
      }
      return;
    }
    if( !str_cmp( arg2, "classtype" ) )
    {
      int x = get_sclass( argument );

      if( x == -1 )
	send_to_char( "Not a spell class type.\r\n", ch );
      else
      {
	SET_SCLA( skill, x );
	send_to_char( "Ok.\r\n", ch );
      }
      return;
    }
    if( !str_cmp( arg2, "powertype" ) )
    {
      int x = get_spower( argument );

      if( x == -1 )
	send_to_char( "Not a spell power type.\r\n", ch );
      else
      {
	SET_SPOW( skill, x );
	send_to_char( "Ok.\r\n", ch );
      }
      return;
    }
    if( !str_cmp( arg2, "flag" ) )
    {
      int x = get_sflag( argument );

      if( x == -1 )
	send_to_char( "Not a spell flag.\r\n", ch );
      else
      {
	TOGGLE_BIT( skill->flags, 1 << ( x + 11 ) );
	send_to_char( "Ok.\r\n", ch );
      }
      return;
    }
    if( !str_cmp( arg2, "saves" ) )
    {
      int x = get_ssave( argument );

      if( x == -1 )
	send_to_char( "Not a saving type.\r\n", ch );
      else
      {
	skill->saves = x;
	send_to_char( "Ok.\r\n", ch );
      }
      return;
    }

    if( !str_cmp( arg2, "code" ) )
    {
      SPELL_FUN *spellfun = NULL;
      DO_FUN *dofun = NULL;

      if( !str_prefix( "spell_", argument )
	  && ( spellfun = spell_function( argument ) ) != spell_notfound )
      {
	skill->spell_fun = spellfun;
	skill->skill_fun = NULL;
	DISPOSE( skill->fun_name );
	skill->fun_name = str_dup( argument );
      }
      else if( !str_prefix( "do_", argument )
	       && ( dofun = skill_function( argument ) ) != skill_notfound )
      {
	skill->skill_fun = dofun;
	skill->spell_fun = NULL;
	DISPOSE( skill->fun_name );
	skill->fun_name = str_dup( argument );
      }
      else
      {
	send_to_char( "Not a spell or skill.\r\n", ch );
	return;
      }
      send_to_char( "Ok.\r\n", ch );
      return;
    }

    if( !str_cmp( arg2, "target" ) )
    {
      int x = get_starget( argument );

      if( x == -1 )
	send_to_char( "Not a valid target type.\r\n", ch );
      else
      {
	skill->target = x;
	send_to_char( "Ok.\r\n", ch );
      }
      return;
    }
    if( !str_cmp( arg2, "minpos" ) )
    {
      skill->minimum_position =
	URANGE( POS_DEAD, atoi( argument ), POS_DRAG );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "slot" ) )
    {
      skill->slot = URANGE( 0, atoi( argument ), 30000 );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "mana" ) )
    {
      skill->min_mana = URANGE( 0, atoi( argument ), 2000 );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "beats" ) )
    {
      skill->beats = URANGE( 0, atoi( argument ), 120 );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "value" ) )
    {
      skill->value = atoi( argument );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "type" ) )
    {
      skill->type = get_skill( argument );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "rmaffect" ) )
    {
      SMAUG_AFF *aff = skill->affects;
      SMAUG_AFF *aff_next = NULL;
      int num = atoi( argument );
      int cnt = 1;

      if( !aff )
      {
	send_to_char
	  ( "This spell has no special affects to remove.\r\n", ch );
	return;
      }
      if( num == 1 )
      {
	skill->affects = aff->next;
	DISPOSE( aff->duration );
	DISPOSE( aff->modifier );
	DISPOSE( aff );
	send_to_char( "Removed.\r\n", ch );
	return;
      }
      for( ; aff; aff = aff->next )
      {
	if( ++cnt == num && ( aff_next = aff->next ) != NULL )
	{
	  aff->next = aff_next->next;
	  DISPOSE( aff_next->duration );
	  DISPOSE( aff_next->modifier );
	  DISPOSE( aff_next );
	  send_to_char( "Removed.\r\n", ch );
	  return;
	}
      }
      send_to_char( "Not found.\r\n", ch );
      return;
    }
    /*
     * affect <location> <modifier> <duration> <bitvector>
     */
    if( !str_cmp( arg2, "affect" ) )
    {
      char location[MAX_INPUT_LENGTH];
      char modifier[MAX_INPUT_LENGTH];
      char duration[MAX_INPUT_LENGTH];
      char bitvector[MAX_INPUT_LENGTH];
      int loc = 0, bit = 0, tmpbit = 0;
      SMAUG_AFF *aff = NULL;

      argument = one_argument( argument, location );
      argument = one_argument( argument, modifier );
      argument = one_argument( argument, duration );

      if( location[0] == '!' )
	loc = get_atype( location + 1 ) + REVERSE_APPLY;
      else
	loc = get_atype( location );
      if( ( loc % REVERSE_APPLY ) < 0
	  || ( loc % REVERSE_APPLY ) >= MAX_APPLY_TYPE )
      {
	send_to_char( "Unknown affect location.  See AFFECTTYPES.\r\n",
	    ch );
	return;
      }

      while( argument[0] != 0 )
      {
	argument = one_argument( argument, bitvector );
	if( ( tmpbit = get_aflag( bitvector ) ) == -1 )
	  ch_printf( ch, "Unknown bitvector: %s.  See AFFECTED_BY\r\n",
	      bitvector );
	else
	  bit |= ( 1 << tmpbit );
      }

      CREATE( aff, SMAUG_AFF, 1 );
      if( !str_cmp( duration, "0" ) )
	duration[0] = '\0';
      if( !str_cmp( modifier, "0" ) )
	modifier[0] = '\0';
      aff->duration = str_dup( duration );
      aff->location = loc;
      if( loc == APPLY_AFFECT || loc == APPLY_RESISTANT
	  || loc == APPLY_IMMUNE || loc == APPLY_SUSCEPTIBLE )
      {
	int modval = get_aflag( modifier );

	/* Sanitize the flag input for the modifier if needed -- Samson */
	if( modval < 0 )
	  modval = 0;
	sprintf( modifier, "%d", modval );
      }
      aff->modifier = str_dup( modifier );
      aff->bitvector = bit;
      aff->next = skill->affects;
      skill->affects = aff;
      send_to_char( "Ok.\n\r", ch );
      return;
    }

    if( !str_cmp( arg2, "name" ) )
    {
      if( *argument == '\0' )
      {
	ch_printf( ch, "Blank name not accepted.\r\n" );
	return;
      }

      if( get_skill( argument ) )
      {
	ch_printf( ch, "A skill with that name already exists.\r\n" );
	return;
      }

      DISPOSE( skill->name );
      skill->name = str_dup( argument );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "dammsg" ) )
    {
      DISPOSE( skill->noun_damage );
      if( !str_cmp( argument, "clear" ) )
	skill->noun_damage = str_dup( "" );
      else
	skill->noun_damage = str_dup( argument );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "wearoff" ) )
    {
      DISPOSE( skill->msg_off );
      if( str_cmp( argument, "clear" ) )
	skill->msg_off = str_dup( argument );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "hitchar" ) )
    {
      if( skill->hit_char )
	DISPOSE( skill->hit_char );
      if( str_cmp( argument, "clear" ) )
	skill->hit_char = str_dup( argument );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "hitvict" ) )
    {
      if( skill->hit_vict )
	DISPOSE( skill->hit_vict );
      if( str_cmp( argument, "clear" ) )
	skill->hit_vict = str_dup( argument );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "hitroom" ) )
    {
      if( skill->hit_room )
	DISPOSE( skill->hit_room );
      if( str_cmp( argument, "clear" ) )
	skill->hit_room = str_dup( argument );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "misschar" ) )
    {
      if( skill->miss_char )
	DISPOSE( skill->miss_char );
      if( str_cmp( argument, "clear" ) )
	skill->miss_char = str_dup( argument );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "missvict" ) )
    {
      if( skill->miss_vict )
	DISPOSE( skill->miss_vict );
      if( str_cmp( argument, "clear" ) )
	skill->miss_vict = str_dup( argument );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "missroom" ) )
    {
      if( skill->miss_room )
	DISPOSE( skill->miss_room );
      if( str_cmp( argument, "clear" ) )
	skill->miss_room = str_dup( argument );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "diechar" ) )
    {
      if( skill->die_char )
	DISPOSE( skill->die_char );
      if( str_cmp( argument, "clear" ) )
	skill->die_char = str_dup( argument );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "dievict" ) )
    {
      if( skill->die_vict )
	DISPOSE( skill->die_vict );
      if( str_cmp( argument, "clear" ) )
	skill->die_vict = str_dup( argument );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "dieroom" ) )
    {
      if( skill->die_room )
	DISPOSE( skill->die_room );
      if( str_cmp( argument, "clear" ) )
	skill->die_room = str_dup( argument );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "immchar" ) )
    {
      if( skill->imm_char )
	DISPOSE( skill->imm_char );
      if( str_cmp( argument, "clear" ) )
	skill->imm_char = str_dup( argument );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "immvict" ) )
    {
      if( skill->imm_vict )
	DISPOSE( skill->imm_vict );
      if( str_cmp( argument, "clear" ) )
	skill->imm_vict = str_dup( argument );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "immroom" ) )
    {
      if( skill->imm_room )
	DISPOSE( skill->imm_room );
      if( str_cmp( argument, "clear" ) )
	skill->imm_room = str_dup( argument );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "dice" ) )
    {
      if( skill->dice )
	DISPOSE( skill->dice );
      if( str_cmp( argument, "clear" ) )
	skill->dice = str_dup( argument );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    if( !str_cmp( arg2, "components" ) )
    {
      if( skill->components )
	DISPOSE( skill->components );
      if( str_cmp( argument, "clear" ) )
	skill->components = str_dup( argument );
      send_to_char( "Ok.\r\n", ch );
      return;
    }
    do_sset( ch, STRLIT_EMPTY );
    return;
  }

  if( ( victim = get_char_world( ch, arg1 ) ) == NULL )
  {
    if( ( sn = skill_lookup( arg1 ) ) >= 0 )
    {
      sprintf( arg1, "%d %s %s", sn, arg2, argument );
      do_sset( ch, arg1 );
    }
    else
      send_to_char( "They aren't here.\r\n", ch );
    return;
  }

  if( IS_NPC( victim ) )
  {
    send_to_char( "Not on NPC's.\r\n", ch );
    return;
  }

  fAll = !str_cmp( arg2, "all" );
  sn = 0;
  if( !fAll && ( sn = skill_lookup( arg2 ) ) < 0 )
  {
    send_to_char( "No such skill or spell.\r\n", ch );
    return;
  }

  /*
   * Snarf the value.
   */
  if( !is_number( argument ) )
  {
    send_to_char( "Value must be numeric.\r\n", ch );
    return;
  }

  value = atoi( argument );
  if( value < 0 || value > 100 )
  {
    send_to_char( "Value range is 0 to 100.\r\n", ch );
    return;
  }

  if( fAll )
    {
      for( sn = 0; sn < top_sn; sn++ )
	{
	  set_skill_level( victim, sn, value );
	}
    }
  else
    {
      set_skill_level( victim, sn, value );
    }
}

void learn_from_success( CHAR_DATA * ch, int sn )
{
  int adept = 100;

  if( IS_NPC( ch ) || character_skill_level( ch, sn ) == 0 )
    return;

  if( character_skill_level( ch, sn ) >= adept )
    return;

  if( character_skill_level( ch, sn ) < 100 )
  {
    int chance = character_skill_level( ch, sn ) / 4
      + 50 + ( 5 * skill_table[sn]->difficulty );
    int percent = number_percent();
    int learn = 0;

    if( percent > 95 )
      learn = 3;
    else if( percent > 90 )
      learn = 2;
    else if( percent > chance )
      learn = 1;
    set_skill_level( ch, sn, UMIN( adept, character_skill_level( ch, sn )
				   + learn ) );

    if( character_skill_level( ch, sn ) >= adept )	/* fully learned! */
    {
      set_char_color( AT_WHITE, ch );

      if( number_bits( 1 ) == 0 && sn != gsn_landscape )
      {
	send_to_char( "You feel somewhat brighter.\r\n", ch );
	ch->perm_int++;
	ch->perm_int = UMIN( ch->perm_int, 25 );
	set_skill_level( ch, sn, adept - 1 );
      }
      else
      {
	ch_printf( ch, "You are now an adept of %s!\r\n",
	    skill_table[sn]->name );
	ch->pcdata->adept_skills++;
      }
    }
  }
}

void learn_from_failure( CHAR_DATA * ch, int sn )
{
}

void do_gouge( CHAR_DATA * ch, char *argument )
{
  CHAR_DATA *victim;
  AFFECT_DATA af;
  short dam;
  int percent;

  if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
  {
    send_to_char( "You can't concentrate enough for that.\r\n", ch );
    return;
  }

  if( !IS_NPC( ch ) && !character_skill_level( ch, gsn_gouge ) )
  {
    send_to_char( "You do not yet know of this skill.\r\n", ch );
    return;
  }

  if( ch->mount )
  {
    send_to_char( "You can't get close enough while mounted.\r\n", ch );
    return;
  }

  if( ( victim = who_fighting( ch ) ) == NULL )
  {
    send_to_char( "You aren't fighting anyone.\r\n", ch );
    return;
  }

  percent = number_percent() - ( get_curr_lck( ch ) - 13 );

  if( IS_NPC( ch ) || percent < character_skill_level( ch, gsn_gouge ) )
  {
    dam = number_range( 1, character_skill_level( ch, gsn_gouge ) );
    global_retcode = damage( ch, victim, dam, gsn_gouge );
    if( global_retcode == rNONE )
    {
      if( !IS_AFFECTED( victim, AFF_BLIND ) )
      {
	af.type = gsn_blindness;
	af.location = APPLY_HITROLL;
	af.modifier = -6;
	af.duration =
	  3 + IS_NPC( ch ) ? ch->top_level : character_skill_level( ch,
	      gsn_gouge )
	  / 20;
	af.bitvector = AFF_BLIND;
	affect_to_char( victim, &af );
	act( AT_SKILL, "You can't see a thing!", victim, NULL, NULL,
	    TO_CHAR );
      }
      WAIT_STATE( ch, PULSE_VIOLENCE );
      WAIT_STATE( victim, PULSE_VIOLENCE );
      /* Taken out by request - put back in by Thoric
       * This is how it was designed.  You'd be a tad stunned
       * if someone gouged you in the eye.
       */
    }
    else if( global_retcode == rVICT_DIED )
    {
      act( AT_BLOOD,
	  "Your fingers plunge into your victim's brain, causing immediate death!",
	  ch, NULL, NULL, TO_CHAR );
    }
    if( global_retcode != rCHAR_DIED && global_retcode != rBOTH_DIED )
      learn_from_success( ch, gsn_gouge );
  }
  else
  {
    WAIT_STATE( ch, skill_table[gsn_gouge]->beats );
    global_retcode = damage( ch, victim, 0, gsn_gouge );
    learn_from_failure( ch, gsn_gouge );
  }

  return;
}

void do_steal( CHAR_DATA * ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  CHAR_DATA *victim, *mst;
  OBJ_DATA *obj, *obj_next;
  int percent;

  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );

  if( ch->mount )
  {
    send_to_char( "You can't do that while mounted.\r\n", ch );
    return;
  }

  if( arg1[0] == '\0' || arg2[0] == '\0' )
  {
    send_to_char( "Steal what from whom?\r\n", ch );
    return;
  }

  if( ms_find_obj( ch ) )
    return;

  if( ( victim = get_char_room( ch, arg2 ) ) == NULL )
  {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }

  if( victim == ch )
  {
    send_to_char( "That's pointless.\r\n", ch );
    return;
  }

  if( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
  {
    set_char_color( AT_MAGIC, ch );
    send_to_char( "This isn't a good place to do that.\r\n", ch );
    return;
  }

  if( check_illegal_psteal( ch, victim ) )
  {
    send_to_char( "You can't steal from that player.\r\n", ch );
    return;
  }

  WAIT_STATE( ch, skill_table[gsn_steal]->beats );
  percent = number_percent() + ( IS_AWAKE( victim ) ? 10 : -50 )
    - ( get_curr_lck( ch ) - 15 ) + ( get_curr_lck( victim ) - 13 )
    + times_killed( ch, victim ) * 7;

  if( victim->position == POS_FIGHTING
      || percent >
      ( IS_NPC( ch ) ? 90 : character_skill_level( ch, gsn_steal ) ) )
  {
    /*
     * Failure.
     */
    send_to_char( "Oops...\r\n", ch );
    act( AT_ACTION, "$n tried to steal from you!\r\n", ch, NULL, victim,
	TO_VICT );
    act( AT_ACTION, "$n tried to steal from $N.\r\n", ch, NULL, victim,
	TO_NOTVICT );

    sprintf( buf, "%s is a bloody thief!", ch->name );
    do_yell( victim, buf );

    learn_from_failure( ch, gsn_steal );
    if( !IS_NPC( ch ) )
    {
      if( legal_loot( ch, victim ) )
      {
	global_retcode = multi_hit( victim, ch, TYPE_UNDEFINED );
      }
      else
      {
	/* log_string( buf ); */
	if( IS_NPC( ch ) )
	{
	  if( ( mst = ch->master ) == NULL )
	    return;
	}
	else
	  mst = ch;
	if( IS_NPC( mst ) )
	  return;

      }
    }

    return;
  }

  if( IS_NPC( victim ) )
    add_kill( ch, victim );	/* makes it harder to steal from same char */

  if( !str_cmp( arg1, "credits" )
      || !str_cmp( arg1, "credit" ) || !str_cmp( arg1, "money" ) )
  {
    int amount;

    amount = ( int ) ( victim->gold * number_range( 1, 10 ) / 100 );
    if( amount <= 0 )
    {
      send_to_char( "You couldn't get any credits.\r\n", ch );
      learn_from_failure( ch, gsn_steal );
      return;
    }

    ch->gold += amount;
    victim->gold -= amount;
    ch_printf( ch, "Aha!  You got %d credits.\r\n", amount );
    learn_from_success( ch, gsn_steal );
    return;
  }

  if( ( obj = get_obj_carry( victim, arg1 ) ) == NULL )
  {
    if( victim->position <= POS_SLEEPING )
    {
      if( ( obj = get_obj_wear( victim, arg1 ) ) != NULL )
      {
	if( ( obj_next = get_eq_char( victim, obj->wear_loc ) ) != obj )
	{
	  ch_printf( ch, "They are wearing %s on top of %s.\r\n",
	      obj_next->short_descr, obj->short_descr );
	  send_to_char( "You'll have to steal that first.\r\n", ch );
	  learn_from_failure( ch, gsn_steal );
	  return;
	}
	else
	  unequip_char( victim, obj );
      }
    }

    send_to_char( "You can't seem to find it.\r\n", ch );
    learn_from_failure( ch, gsn_steal );
    return;
  }

  if( !can_drop_obj( ch, obj )
      || IS_OBJ_STAT( obj, ITEM_INVENTORY )
      || IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
  {
    send_to_char( "You can't manage to pry it away.\r\n", ch );
    learn_from_failure( ch, gsn_steal );
    return;
  }

  if( ch->carry_number + ( get_obj_number( obj ) / obj->count ) >
      can_carry_n( ch ) )
  {
    send_to_char( "You have your hands full.\r\n", ch );
    learn_from_failure( ch, gsn_steal );
    return;
  }

  if( ch->carry_weight + ( get_obj_weight( obj ) / obj->count ) >
      can_carry_w( ch ) )
  {
    send_to_char( "You can't carry that much weight.\r\n", ch );
    learn_from_failure( ch, gsn_steal );
    return;
  }

  send_to_char( "Ok.\r\n", ch );
  learn_from_success( ch, gsn_steal );
  separate_obj( obj );
  obj_from_char( obj );
  obj_to_char( obj, ch );
}

void do_backstab( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  OBJ_DATA *obj;
  int percent;

  if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
  {
    send_to_char( "You can't do that right now.\r\n", ch );
    return;
  }

  one_argument( argument, arg );

  if( ch->mount )
  {
    send_to_char( "You can't get close enough while mounted.\r\n", ch );
    return;
  }

  if( arg[0] == '\0' )
  {
    send_to_char( "Backstab whom?\r\n", ch );
    return;
  }

  if( ( victim = get_char_room( ch, arg ) ) == NULL )
  {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }

  if( victim == ch )
  {
    send_to_char( "How can you sneak up on yourself?\r\n", ch );
    return;
  }

  if( is_safe( ch, victim ) )
    return;

  /* Added stabbing weapon. -Narn */
  if( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL
      || ( obj->value[3] != WEAPON_VIBRO_BLADE ) )
  {
    send_to_char( "You need to wield a stabbing weapon.\r\n", ch );
    return;
  }

  if( victim->fighting )
  {
    send_to_char( "You can't backstab someone who is in combat.\r\n", ch );
    return;
  }

  /* Can backstab a char even if it's hurt as long as it's sleeping. -Narn */
  if( victim->hit < victim->max_hit && IS_AWAKE( victim ) )
  {
    act( AT_PLAIN, "$N is hurt and suspicious ... you can't sneak up.",
	ch, NULL, victim, TO_CHAR );
    return;
  }

  percent = number_percent() - ( get_curr_lck( ch ) - 14 )
    + ( get_curr_lck( victim ) - 13 );

  WAIT_STATE( ch, skill_table[gsn_backstab]->beats );
  if( !IS_AWAKE( victim )
      || IS_NPC( ch ) || percent < character_skill_level( ch, gsn_backstab ) )
  {
    learn_from_success( ch, gsn_backstab );
    global_retcode = multi_hit( ch, victim, gsn_backstab );

  }
  else
  {
    learn_from_failure( ch, gsn_backstab );
    global_retcode = damage( ch, victim, 0, gsn_backstab );
  }
  return;
}

void do_rescue( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  CHAR_DATA *fch;
  int percent;

  if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
  {
    send_to_char( "You can't concentrate enough for that.\r\n", ch );
    return;
  }

  one_argument( argument, arg );
  if( arg[0] == '\0' )
  {
    send_to_char( "Rescue whom?\r\n", ch );
    return;
  }

  if( ( victim = get_char_room( ch, arg ) ) == NULL )
  {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }

  if( victim == ch )
  {
    send_to_char( "You try and rescue yourself, but fail miserably.\r\n",
	ch );
    return;
  }

  if( ch->mount )
  {
    send_to_char( "You can't do that while mounted.\r\n", ch );
    return;
  }

  if( !IS_NPC( ch ) && IS_NPC( victim ) )
  {
    send_to_char( "Doesn't need your help!\r\n", ch );
    return;
  }

  if( ( fch = who_fighting( victim ) ) == NULL )
  {
    send_to_char( "They are not fighting right now.\r\n", ch );
    return;
  }

  if( who_fighting( victim ) == ch )
  {
    send_to_char( "One would imagine THEY don't need your help.\r\n", ch );
    return;
  }

  ch->alignment = ch->alignment + 5;
  ch->alignment = URANGE( -1000, ch->alignment, 1000 );

  percent = number_percent() - ( get_curr_lck( ch ) - 14 )
    - ( get_curr_lck( victim ) - 16 );

  WAIT_STATE( ch, skill_table[gsn_rescue]->beats );
  if( !IS_NPC( ch ) && percent > character_skill_level( ch, gsn_rescue ) )
  {
    send_to_char( "You fail the rescue.\r\n", ch );
    act( AT_SKILL, "$n tries to rescue you!", ch, NULL, victim, TO_VICT );
    act( AT_SKILL, "$n tries to rescue $N!", ch, NULL, victim, TO_NOTVICT );
    learn_from_failure( ch, gsn_rescue );
    return;
  }

  act( AT_SKILL, "You rescue $N!", ch, NULL, victim, TO_CHAR );
  act( AT_SKILL, "$n rescues you!", ch, NULL, victim, TO_VICT );
  act( AT_SKILL, "$n moves in front of $N!", ch, NULL, victim, TO_NOTVICT );

  ch->alignment = ch->alignment + 50;
  ch->alignment = URANGE( -1000, ch->alignment, 1000 );

  learn_from_success( ch, gsn_rescue );
  stop_fighting( fch, FALSE );
  stop_fighting( victim, FALSE );
  if( ch->fighting )
    stop_fighting( ch, FALSE );

  /* check_killer( ch, fch ); */
  set_fighting( ch, fch );
  set_fighting( fch, ch );
  return;
}

void do_kick( CHAR_DATA * ch, char *argument )
{
  CHAR_DATA *victim;

  if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
  {
    send_to_char( "You can't concentrate enough for that.\r\n", ch );
    return;
  }

  if( ( victim = who_fighting( ch ) ) == NULL )
  {
    send_to_char( "You aren't fighting anyone.\r\n", ch );
    return;
  }

  WAIT_STATE( ch, skill_table[gsn_kick]->beats );
  if( IS_NPC( ch )
      || number_percent() < character_skill_level( ch, gsn_kick ) )
  {
    learn_from_success( ch, gsn_kick );
    global_retcode =
      damage( ch, victim,
	  number_range( 1,
	    IS_NPC( ch ) ? ch->top_level /
	    5 : character_skill_level( ch, gsn_kick ) / 5 ),
	  gsn_kick );
  }
  else
  {
    learn_from_failure( ch, gsn_kick );
    global_retcode = damage( ch, victim, 0, gsn_kick );
  }
  return;
}

/*
 * Disarm a creature.
 * Caller must check for successful attack.
 * Check for loyalty flag (weapon disarms to inventory) for pkillers -Blodkai
 */
void disarm( CHAR_DATA * ch, CHAR_DATA * victim )
{
  OBJ_DATA *obj = NULL, *tmpobj = NULL;

  if( ( obj = get_eq_char( victim, WEAR_WIELD ) ) == NULL )
    return;

  if( ( tmpobj = get_eq_char( victim, WEAR_DUAL_WIELD ) ) != NULL
      && number_bits( 1 ) == 0 )
    obj = tmpobj;

  if( get_eq_char( ch, WEAR_WIELD ) == NULL && number_bits( 1 ) == 0 )
  {
    learn_from_failure( ch, gsn_disarm );
    return;
  }

  if( IS_NPC( ch ) && !can_see_obj( ch, obj ) && number_bits( 1 ) == 0 )
  {
    learn_from_failure( ch, gsn_disarm );
    return;
  }

  if( check_grip( ch, victim ) )
  {
    learn_from_failure( ch, gsn_disarm );
    return;
  }

  act( AT_SKILL, "$n DISARMS you!", ch, NULL, victim, TO_VICT );
  act( AT_SKILL, "You disarm $N!", ch, NULL, victim, TO_CHAR );
  act( AT_SKILL, "$n disarms $N!", ch, NULL, victim, TO_NOTVICT );
  learn_from_success( ch, gsn_disarm );

  if( obj == get_eq_char( victim, WEAR_WIELD )
      && ( tmpobj = get_eq_char( victim, WEAR_DUAL_WIELD ) ) != NULL )
    tmpobj->wear_loc = WEAR_WIELD;

  obj_from_char( obj );
  obj_to_room( obj, victim->in_room );
}


void do_disarm( CHAR_DATA * ch, char *argument )
{
  CHAR_DATA *victim;
  OBJ_DATA *obj;
  int percent;

  if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
  {
    send_to_char( "You can't concentrate enough for that.\r\n", ch );
    return;
  }

  if( !IS_NPC( ch ) && character_skill_level( ch, gsn_disarm ) <= 0 )
  {
    send_to_char( "You don't know how to disarm opponents.\r\n", ch );
    return;
  }

  if( get_eq_char( ch, WEAR_WIELD ) == NULL )
  {
    send_to_char( "You must wield a weapon to disarm.\r\n", ch );
    return;
  }

  if( ( victim = who_fighting( ch ) ) == NULL )
  {
    send_to_char( "You aren't fighting anyone.\r\n", ch );
    return;
  }

  if( ( obj = get_eq_char( victim, WEAR_WIELD ) ) == NULL )
  {
    send_to_char( "Your opponent is not wielding a weapon.\r\n", ch );
    return;
  }

  WAIT_STATE( ch, skill_table[gsn_disarm]->beats );
  percent = number_percent()
    - ( get_curr_lck( ch ) - 15 ) + ( get_curr_lck( victim ) - 15 );
  if( !can_see_obj( ch, obj ) )
    percent += 10;
  if( IS_NPC( ch )
      || percent < character_skill_level( ch, gsn_disarm ) * 2 / 3 )
    disarm( ch, victim );
  else
  {
    send_to_char( "You failed.\r\n", ch );
    learn_from_failure( ch, gsn_disarm );
  }
  return;
}


/*
 * Trip a creature.
 * Caller must check for successful attack.
 */
void trip( CHAR_DATA * ch, CHAR_DATA * victim )
{
  if( IS_AFFECTED( victim, AFF_FLYING )
      || IS_AFFECTED( victim, AFF_FLOATING ) )
    return;
  if( victim->mount )
  {
    if( IS_AFFECTED( victim->mount, AFF_FLYING )
	|| IS_AFFECTED( victim->mount, AFF_FLOATING ) )
      return;
    act( AT_SKILL, "$n trips your mount and you fall off!", ch, NULL,
	victim, TO_VICT );
    act( AT_SKILL, "You trip $N's mount and $N falls off!", ch, NULL,
	victim, TO_CHAR );
    act( AT_SKILL, "$n trips $N's mount and $N falls off!", ch, NULL,
	victim, TO_NOTVICT );
    REMOVE_BIT( victim->mount->act, ACT_MOUNTED );
    victim->mount = NULL;
    WAIT_STATE( ch, 2 * PULSE_VIOLENCE );
    WAIT_STATE( victim, 2 * PULSE_VIOLENCE );
    victim->position = POS_RESTING;
    return;
  }
  if( victim->wait == 0 )
  {
    act( AT_SKILL, "$n trips you and you go down!", ch, NULL, victim,
	TO_VICT );
    act( AT_SKILL, "You trip $N and $N goes down!", ch, NULL, victim,
	TO_CHAR );
    act( AT_SKILL, "$n trips $N and $N goes down!", ch, NULL, victim,
	TO_NOTVICT );

    WAIT_STATE( ch, 2 * PULSE_VIOLENCE );
    WAIT_STATE( victim, 2 * PULSE_VIOLENCE );
    victim->position = POS_RESTING;
  }

  return;
}


void do_pick( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *gch;
  OBJ_DATA *obj;
  EXIT_DATA *pexit;
  SHIP_DATA *ship;

  if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
  {
    send_to_char( "You can't concentrate enough for that.\r\n", ch );
    return;
  }

  one_argument( argument, arg );

  if( arg[0] == '\0' )
  {
    send_to_char( "Pick what?\r\n", ch );
    return;
  }

  if( ms_find_obj( ch ) )
    return;

  if( ch->mount )
  {
    send_to_char( "You can't do that while mounted.\r\n", ch );
    return;
  }

  WAIT_STATE( ch, skill_table[gsn_pick_lock]->beats );

  /* look for guards */
  for( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
  {
    if( IS_NPC( gch ) && IS_AWAKE( gch ) )
    {
      act( AT_PLAIN, "$N is standing too close to the lock.",
	  ch, NULL, gch, TO_CHAR );
      return;
    }
  }


  if( ( pexit = find_door( ch, arg, TRUE ) ) != NULL )
  {
    /* 'pick door' */
    /*	ROOM_INDEX_DATA *to_room; *//* Unused */
    EXIT_DATA *pexit_rev;

    if( !IS_SET( pexit->exit_info, EX_CLOSED ) )
    {
      send_to_char( "It's not closed.\r\n", ch );
      return;
    }
    if( pexit->key < 0 )
    {
      send_to_char( "It can't be picked.\r\n", ch );
      return;
    }
    if( !IS_SET( pexit->exit_info, EX_LOCKED ) )
    {
      send_to_char( "It's already unlocked.\r\n", ch );
      return;
    }
    if( IS_SET( pexit->exit_info, EX_PICKPROOF ) )
    {
      send_to_char( "You failed.\r\n", ch );
      learn_from_failure( ch, gsn_pick_lock );
      return;
    }

    if( !IS_NPC( ch )
	&& number_percent() > character_skill_level( ch, gsn_pick_lock ) )
    {
      send_to_char( "You failed.\r\n", ch );
      learn_from_failure( ch, gsn_pick_lock );
      return;
    }

    REMOVE_BIT( pexit->exit_info, EX_LOCKED );
    send_to_char( "*Click*\r\n", ch );
    act( AT_ACTION, "$n picks the $d.", ch, NULL, pexit->keyword, TO_ROOM );
    learn_from_success( ch, gsn_pick_lock );
    /* pick the other side */
    if( ( pexit_rev = pexit->rexit ) != NULL
	&& pexit_rev->to_room == ch->in_room )
    {
      REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
    }
    return;
  }

  if( ( obj = get_obj_here( ch, arg ) ) != NULL )
  {
    if( obj->item_type != ITEM_CONTAINER )
    {
      send_to_char( "You can't pick that.\r\n", ch );
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
    if( !IS_SET( obj->value[1], CONT_LOCKED ) )
    {
      send_to_char( "It's already unlocked.\r\n", ch );
      return;
    }
    if( IS_SET( obj->value[1], CONT_PICKPROOF ) )
    {
      send_to_char( "You failed.\r\n", ch );
      learn_from_failure( ch, gsn_pick_lock );
      return;
    }

    if( !IS_NPC( ch )
	&& number_percent() > character_skill_level( ch, gsn_pick_lock ) )
    {
      send_to_char( "You failed.\r\n", ch );
      learn_from_failure( ch, gsn_pick_lock );
      return;
    }

    separate_obj( obj );
    REMOVE_BIT( obj->value[1], CONT_LOCKED );
    send_to_char( "*Click*\r\n", ch );
    act( AT_ACTION, "$n picks $p.", ch, obj, NULL, TO_ROOM );
    learn_from_success( ch, gsn_pick_lock );
    return;
  }

  if( ( ship = ship_in_room( ch->in_room, arg ) ) != NULL )
  {
    int chance;

    char buf[MAX_STRING_LENGTH];

    if( check_pilot( ch, ship ) )
    {
      send_to_char( "&RWhat would be the point of that!\r\n", ch );
      return;
    }

    if( ship->shipstate != SHIP_DOCKED && ship->shipstate != SHIP_DISABLED )
    {
      send_to_char( "&RThat ship has already started to launch", ch );
      return;
    }

    WAIT_STATE( ch, skill_table[gsn_pickshiplock]->beats );

    chance = URANGE( 0, character_skill_level( ch, gsn_pickshiplock ), 95 );

    if( IS_NPC( ch ) || !ch->pcdata || number_percent() > chance )
    {
      CHAR_DATA *wch;

      send_to_char( "You failed.\r\n", ch );
      sprintf( buf, "&R[ALARM] Someone is attempting to pick %s.",
	  ship->name );
      echo_to_area( ch->in_room->area, AT_RED, buf, 0 );
      for( wch = first_char; wch; wch = wch->next )
	if( !str_cmp( ship->owner, wch->name ) )
	  send_to_char( buf, ch );
      return;
    }

    if( !ship->hatchopen )
    {
      ship->hatchopen = TRUE;
      act( AT_PLAIN, "You pick the lock and open the hatch on $T.", ch,
	  NULL, ship->name, TO_CHAR );
      act( AT_PLAIN, "$n picks open the hatch on $T.", ch, NULL,
	  ship->name, TO_ROOM );
      echo_to_room( AT_YELLOW, ship->entrance,
	  "The hatch opens from the outside." );
      learn_from_success( ch, gsn_pickshiplock );
    }
    return;
  }

  ch_printf( ch, "You see no %s here.\r\n", arg );
  return;
}



void do_sneak( CHAR_DATA * ch, char *argument )
{
  AFFECT_DATA af;

  if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
  {
    send_to_char( "You can't concentrate enough for that.\r\n", ch );
    return;
  }

  if( ch->mount )
  {
    send_to_char( "You can't do that while mounted.\r\n", ch );
    return;
  }

  send_to_char( "You attempt to move silently.\r\n", ch );
  affect_strip( ch, gsn_sneak );

  if( IS_NPC( ch )
      || number_percent() < character_skill_level( ch, gsn_sneak ) )
  {
    af.type = gsn_sneak;
    af.duration =
      IS_NPC( ch ) ? ch->
      top_level : ( short ) ( character_skill_level( ch, gsn_sneak ) *
	  DUR_CONV );
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_SNEAK;
    affect_to_char( ch, &af );
    learn_from_success( ch, gsn_sneak );
  }
  else
    learn_from_failure( ch, gsn_sneak );

  return;
}



void do_hide( CHAR_DATA * ch, char *argument )
{
  if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
  {
    send_to_char( "You can't concentrate enough for that.\r\n", ch );
    return;
  }

  if( ch->mount )
  {
    send_to_char( "You can't do that while mounted.\r\n", ch );
    return;
  }

  send_to_char( "You attempt to hide.\r\n", ch );

  if( IS_AFFECTED( ch, AFF_HIDE ) )
    REMOVE_BIT( ch->affected_by, AFF_HIDE );

  if( IS_NPC( ch )
      || number_percent() < character_skill_level( ch, gsn_hide ) )
  {
    SET_BIT( ch->affected_by, AFF_HIDE );
    learn_from_success( ch, gsn_hide );
  }
  else
    learn_from_failure( ch, gsn_hide );
  return;
}



/*
 * Contributed by Alander.
 */
void do_visible( CHAR_DATA * ch, char *argument )
{
  affect_strip( ch, gsn_invis );
  affect_strip( ch, gsn_mass_invis );
  affect_strip( ch, gsn_sneak );
  REMOVE_BIT( ch->affected_by, AFF_HIDE );
  REMOVE_BIT( ch->affected_by, AFF_INVISIBLE );
  REMOVE_BIT( ch->affected_by, AFF_SNEAK );
  send_to_char( "Ok.\r\n", ch );
}

void do_recall( CHAR_DATA * ch, char *argument )
{
  ROOM_INDEX_DATA *location = get_room_index( wherehome( ch ) );
  CHAR_DATA *opponent;

  if( !IS_IMMORTAL( ch ) )
  {
    return;
  }

  if( !location )
  {
    send_to_char( "You are completely lost.\r\n", ch );
    return;
  }

  if( ch->in_room == location )
    return;

  if( IS_SET( ch->affected_by, AFF_CURSE ) )
  {
    send_to_char( "You are cursed and cannot recall!\r\n", ch );
    return;
  }

  if( ( opponent = who_fighting( ch ) ) != NULL )
  {
    if( number_bits( 1 ) == 0
	|| ( !IS_NPC( opponent ) && number_bits( 3 ) > 1 ) )
    {
      WAIT_STATE( ch, 4 );
      ch_printf( ch, "You failed!\r\n" );
      return;
    }

    ch_printf( ch, "You recall from combat!\r\n" );
    stop_fighting( ch, TRUE );
  }

  act( AT_ACTION, "$n disappears in a swirl of the Force.", ch, NULL, NULL,
      TO_ROOM );
  char_from_room( ch );
  char_to_room( ch, location );

  if( ch->mount )
  {
    char_from_room( ch->mount );
    char_to_room( ch->mount, location );
  }

  act( AT_ACTION, "$n appears in a swirl of the Force.",
      ch, NULL, NULL, TO_ROOM );
  do_look( ch, STRLIT_AUTO );
}


void do_aid( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  int percent;

  if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
  {
    send_to_char( "You can't concentrate enough for that.\r\n", ch );
    return;
  }

  one_argument( argument, arg );
  if( arg[0] == '\0' )
  {
    send_to_char( "Aid whom?\r\n", ch );
    return;
  }

  if( ( victim = get_char_room( ch, arg ) ) == NULL )
  {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }

  if( ch->mount )
  {
    send_to_char( "You can't do that while mounted.\r\n", ch );
    return;
  }

  if( victim == ch )
  {
    send_to_char( "Aid yourself?\r\n", ch );
    return;
  }

  if( victim->position > POS_STUNNED )
  {
    act( AT_PLAIN, "$N doesn't need your help.", ch, NULL, victim,
	TO_CHAR );
    return;
  }

  if( victim->hit <= -400 )
  {
    act( AT_PLAIN, "$N's condition is beyond your aiding ability.", ch,
	NULL, victim, TO_CHAR );
    return;
  }

  ch->alignment = ch->alignment + 20;
  ch->alignment = URANGE( -1000, ch->alignment, 1000 );

  percent = number_percent() - ( get_curr_lck( ch ) - 13 );
  WAIT_STATE( ch, skill_table[gsn_aid]->beats );
  if( !IS_NPC( ch ) && percent > character_skill_level( ch, gsn_aid ) )
  {
    send_to_char( "You fail.\r\n", ch );
    learn_from_failure( ch, gsn_aid );
    return;
  }

  ch->alignment = ch->alignment + 20;
  ch->alignment = URANGE( -1000, ch->alignment, 1000 );

  act( AT_SKILL, "You aid $N!", ch, NULL, victim, TO_CHAR );
  act( AT_SKILL, "$n aids $N!", ch, NULL, victim, TO_NOTVICT );
  learn_from_success( ch, gsn_aid );
  if( victim->hit < 1 )
    victim->hit = 1;

  update_pos( victim );
  act( AT_SKILL, "$n aids you!", ch, NULL, victim, TO_VICT );
  return;
}


void do_mount( CHAR_DATA * ch, char *argument )
{
  CHAR_DATA *victim;

  if( !IS_NPC( ch ) && character_skill_level( ch, gsn_mount ) <= 0 )
  {
    send_to_char( "I don't think that would be a good idea...\r\n", ch );
    return;
  }

  if( ch->mount )
  {
    send_to_char( "You're already mounted!\r\n", ch );
    return;
  }

  if( ( victim = get_char_room( ch, argument ) ) == NULL )
  {
    send_to_char( "You can't find that here.\r\n", ch );
    return;
  }

  if( !IS_NPC( victim ) || !IS_SET( victim->act, ACT_MOUNTABLE ) )
  {
    send_to_char( "You can't mount that!\r\n", ch );
    return;
  }

  if( IS_SET( victim->act, ACT_MOUNTED ) )
  {
    send_to_char( "That mount already has a rider.\r\n", ch );
    return;
  }

  if( victim->position < POS_STANDING )
  {
    send_to_char( "Your mount must be standing.\r\n", ch );
    return;
  }

  if( victim->position == POS_FIGHTING || victim->fighting )
  {
    send_to_char( "Your mount is moving around too much.\r\n", ch );
    return;
  }

  WAIT_STATE( ch, skill_table[gsn_mount]->beats );
  if( IS_NPC( ch )
      || number_percent() < character_skill_level( ch, gsn_mount ) )
  {
    SET_BIT( victim->act, ACT_MOUNTED );
    ch->mount = victim;
    act( AT_SKILL, "You mount $N.", ch, NULL, victim, TO_CHAR );
    act( AT_SKILL, "$n skillfully mounts $N.", ch, NULL, victim,
	TO_NOTVICT );
    act( AT_SKILL, "$n mounts you.", ch, NULL, victim, TO_VICT );
    learn_from_success( ch, gsn_mount );
    ch->position = POS_MOUNTED;
  }
  else
  {
    act( AT_SKILL, "You unsuccessfully try to mount $N.", ch, NULL, victim,
	TO_CHAR );
    act( AT_SKILL, "$n unsuccessfully attempts to mount $N.", ch, NULL,
	victim, TO_NOTVICT );
    act( AT_SKILL, "$n tries to mount you.", ch, NULL, victim, TO_VICT );
    learn_from_failure( ch, gsn_mount );
  }
  return;
}


void do_dismount( CHAR_DATA * ch, char *argument )
{
  CHAR_DATA *victim;

  if( ( victim = ch->mount ) == NULL )
  {
    send_to_char( "You're not mounted.\r\n", ch );
    return;
  }

  WAIT_STATE( ch, skill_table[gsn_mount]->beats );
  if( IS_NPC( ch )
      || number_percent() < character_skill_level( ch, gsn_mount ) )
  {
    act( AT_SKILL, "You dismount $N.", ch, NULL, victim, TO_CHAR );
    act( AT_SKILL, "$n skillfully dismounts $N.", ch, NULL, victim,
	TO_NOTVICT );
    act( AT_SKILL, "$n dismounts you.  Whew!", ch, NULL, victim, TO_VICT );
    REMOVE_BIT( victim->act, ACT_MOUNTED );
    ch->mount = NULL;
    ch->position = POS_STANDING;
    learn_from_success( ch, gsn_mount );
  }
  else
  {
    act( AT_SKILL, "You fall off while dismounting $N.  Ouch!", ch, NULL,
	victim, TO_CHAR );
    act( AT_SKILL, "$n falls off of $N while dismounting.", ch, NULL,
	victim, TO_NOTVICT );
    act( AT_SKILL, "$n falls off your back.", ch, NULL, victim, TO_VICT );
    learn_from_failure( ch, gsn_mount );
    REMOVE_BIT( victim->act, ACT_MOUNTED );
    ch->mount = NULL;
    ch->position = POS_SITTING;
    global_retcode = damage( ch, ch, 1, TYPE_UNDEFINED );
  }
  return;
}


/**************************************************************************/


/*
 * Check for parry.
 */
bool check_parry( CHAR_DATA * ch, CHAR_DATA * victim )
{
  int chances;
  OBJ_DATA *wield;

  if( !IS_AWAKE( victim ) )
    return FALSE;

  if( IS_NPC( victim ) && !IS_SET( victim->defenses, DFND_PARRY ) )
    return FALSE;

  if( IS_NPC( victim ) )
  {
    /* Tuan was here.  :)   *** so was Durga :p *** */
    chances = UMIN( 60, victim->top_level );
  }
  else
  {
    if( ( wield = get_eq_char( victim, WEAR_WIELD ) ) == NULL ||
	( wield->value[3] != WEAPON_LIGHTSABER ) )
    {
      if( ( wield = get_eq_char( victim, WEAR_DUAL_WIELD ) ) == NULL ||
	  ( wield->value[3] != WEAPON_LIGHTSABER ) )
	return FALSE;
    }
    chances = character_skill_level( victim, gsn_parry );
  }

  chances = URANGE( 10, chances, 90 );

  if( number_range( 1, 100 ) > chances )
  {
    learn_from_failure( victim, gsn_parry );
    return FALSE;
  }
  if( !IS_NPC( victim ) && !IS_SET( victim->pcdata->flags, PCFLAG_GAG ) )
    /*SB*/
    act( AT_SKILL, "You parry $n's attack.", ch, NULL, victim, TO_VICT );

  if( !IS_NPC( ch ) && !IS_SET( ch->pcdata->flags, PCFLAG_GAG ) )	/* SB */
    act( AT_SKILL, "$N parries your attack.", ch, NULL, victim, TO_CHAR );

  learn_from_success( victim, gsn_parry );
  return TRUE;
}



/*
 * Check for dodge.
 */
bool check_dodge( CHAR_DATA * ch, CHAR_DATA * victim )
{
  int chances;

  if( !IS_AWAKE( victim ) )
    return FALSE;

  if( IS_NPC( victim ) && !IS_SET( victim->defenses, DFND_DODGE ) )
    return FALSE;

  if( IS_NPC( victim ) )
    chances = UMIN( 60, victim->top_level );
  else
    chances = character_skill_level( victim, gsn_dodge ) / 2;

  if( number_range( 1, 100 ) > chances )
  {
    learn_from_failure( victim, gsn_dodge );
    return FALSE;
  }

  if( !IS_NPC( victim ) && !IS_SET( victim->pcdata->flags, PCFLAG_GAG ) )
    act( AT_SKILL, "You dodge $n's attack.", ch, NULL, victim, TO_VICT );

  if( !IS_NPC( ch ) && !IS_SET( ch->pcdata->flags, PCFLAG_GAG ) )
    act( AT_SKILL, "$N dodges your attack.", ch, NULL, victim, TO_CHAR );

  learn_from_success( victim, gsn_dodge );
  return TRUE;
}

void do_poison_weapon( CHAR_DATA * ch, char *argument )
{
  OBJ_DATA *obj;
  OBJ_DATA *pobj;
  OBJ_DATA *wobj;
  char arg[MAX_INPUT_LENGTH];
  int percent;

  if( !IS_NPC( ch ) && character_skill_level( ch, gsn_poison_weapon ) <= 0 )
  {
    send_to_char( "What do you think you are, a thief?\r\n", ch );
    return;
  }

  one_argument( argument, arg );

  if( arg[0] == '\0' )
  {
    send_to_char( "What are you trying to poison?\r\n", ch );
    return;
  }
  if( ch->fighting )
  {
    send_to_char( "While you're fighting?  Nice try.\r\n", ch );
    return;
  }
  if( ms_find_obj( ch ) )
    return;

  if( !( obj = get_obj_carry( ch, arg ) ) )
  {
    send_to_char( "You do not have that weapon.\r\n", ch );
    return;
  }
  if( obj->item_type != ITEM_WEAPON )
  {
    send_to_char( "That item is not a weapon.\r\n", ch );
    return;
  }
  if( IS_OBJ_STAT( obj, ITEM_POISONED ) )
  {
    send_to_char( "That weapon is already poisoned.\r\n", ch );
    return;
  }
  /* Now we have a valid weapon...check to see if we have the powder. */
  for( pobj = ch->first_carrying; pobj; pobj = pobj->next_content )
  {
    if( pobj->pIndexData->vnum == OBJ_VNUM_BLACK_POWDER )
      break;
  }
  if( !pobj )
  {
    send_to_char( "You do not have the black poison powder.\r\n", ch );
    return;
  }
  /* Okay, we have the powder...do we have water? */
  for( wobj = ch->first_carrying; wobj; wobj = wobj->next_content )
  {
    if( wobj->item_type == ITEM_DRINK_CON
	&& wobj->value[1] > 0 && wobj->value[2] == 0 )
      break;
  }
  if( !wobj )
  {
    send_to_char( "You have no water to mix with the powder.\r\n", ch );
    return;
  }
  /* And does the thief have steady enough hands? */
  if( !IS_NPC( ch ) && ( ch->pcdata->condition[COND_DRUNK] > 0 ) )
  {
    send_to_char
      ( "Your hands aren't steady enough to properly mix the poison.\r\n",
	ch );
    return;
  }
  WAIT_STATE( ch, skill_table[gsn_poison_weapon]->beats );

  percent = ( number_percent() - get_curr_lck( ch ) - 14 );

  /* Check the skill percentage */
  separate_obj( pobj );
  separate_obj( wobj );
  if( !IS_NPC( ch )
      && percent > character_skill_level( ch, gsn_poison_weapon ) )
  {
    set_char_color( AT_RED, ch );
    send_to_char( "You failed and spill some on yourself.  Ouch!\r\n", ch );
    set_char_color( AT_GREY, ch );
    damage( ch, ch, character_skill_level( ch, gsn_poison_weapon ),
	gsn_poison_weapon );
    act( AT_RED, "$n spills the poison all over!", ch, NULL, NULL,
	TO_ROOM );
    extract_obj( pobj );
    extract_obj( wobj );
    learn_from_failure( ch, gsn_poison_weapon );
    return;
  }
  separate_obj( obj );
  act( AT_RED, "You mix $p in $P, creating a deadly poison!", ch, pobj, wobj,
      TO_CHAR );
  act( AT_RED, "$n mixes $p in $P, creating a deadly poison!", ch, pobj, wobj,
      TO_ROOM );
  act( AT_GREEN, "You pour the poison over $p, which glistens wickedly!", ch,
      obj, NULL, TO_CHAR );
  act( AT_GREEN, "$n pours the poison over $p, which glistens wickedly!", ch,
      obj, NULL, TO_ROOM );
  SET_BIT( obj->extra_flags, ITEM_POISONED );
  obj->cost *= character_skill_level( ch, gsn_poison_weapon ) / 2;
  /* Set an object timer.  Don't want proliferation of poisoned weapons */
  obj->timer = 10 + character_skill_level( ch, gsn_poison_weapon );

  if( IS_OBJ_STAT( obj, ITEM_BLESS ) )
    obj->timer *= 2;

  if( IS_OBJ_STAT( obj, ITEM_MAGIC ) )
    obj->timer *= 2;

  /* WHAT?  All of that, just for that one bit?  How lame. ;) */
  act( AT_BLUE, "The remainder of the poison eats through $p.", ch, wobj,
      NULL, TO_CHAR );
  act( AT_BLUE, "The remainder of the poison eats through $p.", ch, wobj,
      NULL, TO_ROOM );
  extract_obj( pobj );
  extract_obj( wobj );
  learn_from_success( ch, gsn_poison_weapon );
  return;
}


bool check_grip( CHAR_DATA * ch, CHAR_DATA * victim )
{
  int chance;

  if( !IS_AWAKE( victim ) )
    return FALSE;

  if( IS_NPC( victim ) && !IS_SET( victim->defenses, DFND_GRIP ) )
    return FALSE;

  if( IS_NPC( victim ) )
    chance = UMIN( 60, 2 * victim->top_level );
  else
    chance = character_skill_level( victim, gsn_grip ) / 2;

  /* Consider luck as a factor */
  chance += ( 2 * ( get_curr_lck( victim ) - 13 ) );

  if( number_percent() >= chance + victim->top_level - ch->top_level )
  {
    learn_from_failure( victim, gsn_grip );
    return FALSE;
  }

  act( AT_SKILL, "You evade $n's attempt to disarm you.", ch, NULL,
      victim, TO_VICT );
  act( AT_SKILL, "$N holds $S weapon strongly, and is not disarmed.",
      ch, NULL, victim, TO_CHAR );
  learn_from_success( victim, gsn_grip );
  return TRUE;
}

void do_circle( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  OBJ_DATA *obj;
  int percent;

  if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
  {
    send_to_char( "You can't concentrate enough for that.\r\n", ch );
    return;
  }

  one_argument( argument, arg );

  if( ch->mount )
  {
    send_to_char( "You can't circle while mounted.\r\n", ch );
    return;
  }

  if( arg[0] == '\0' )
  {
    send_to_char( "Circle around whom?\r\n", ch );
    return;
  }

  if( ( victim = get_char_room( ch, arg ) ) == NULL )
  {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }

  if( victim == ch )
  {
    send_to_char( "How can you sneak up on yourself?\r\n", ch );
    return;
  }

  if( is_safe( ch, victim ) )
    return;

  if( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL
      || ( obj->value[3] != 11 && obj->value[3] != 2 ) )
  {
    send_to_char( "You need to wield a piercing or stabbing weapon.\r\n",
	ch );
    return;
  }

  if( !ch->fighting )
  {
    send_to_char( "You can't circle when you aren't fighting.\r\n", ch );
    return;
  }

  if( !victim->fighting )
  {
    send_to_char
      ( "You can't circle around a person who is not fighting.\r\n", ch );
    return;
  }

  if( victim->num_fighting < 2 )
  {
    act( AT_PLAIN, "You can't circle around them without a distraction.",
	ch, NULL, victim, TO_CHAR );
    return;
  }

  percent = number_percent() - ( get_curr_lck( ch ) - 16 )
    + ( get_curr_lck( victim ) - 13 );

  WAIT_STATE( ch, skill_table[gsn_circle]->beats );
  if( percent < character_skill_level( ch, gsn_circle ) )
  {
    learn_from_success( ch, gsn_circle );
    global_retcode = multi_hit( ch, victim, gsn_circle );

  }
  else
  {
    learn_from_failure( ch, gsn_circle );
    global_retcode = damage( ch, victim, 0, gsn_circle );
  }
  return;
}

/* Berserk and HitAll. -- Altrag */
void do_berserk( CHAR_DATA * ch, char *argument )
{
  short percent;
  AFFECT_DATA af;

  if( !ch->fighting )
  {
    send_to_char( "But you aren't fighting!\r\n", ch );
    return;
  }

  if( IS_AFFECTED( ch, AFF_BERSERK ) )
  {
    send_to_char( "Your rage is already at its peak!\r\n", ch );
    return;
  }

  percent = IS_NPC( ch ) ? 80 : character_skill_level( ch, gsn_berserk );
  WAIT_STATE( ch, skill_table[gsn_berserk]->beats );
  if( !luck_check( ch, percent ) )
  {
    send_to_char( "You couldn't build up enough rage.\r\n", ch );
    learn_from_failure( ch, gsn_berserk );
    return;
  }
  af.type = gsn_berserk;
  /* Hmmm.. 10-20 combat rounds at level 50.. good enough for most mobs,
     and if not they can always go berserk again.. shrug.. maybe even
     too high. -- Altrag */
  af.duration = number_range( ch->top_level / 5, ch->top_level * 2 / 5 );
  /* Hmm.. you get stronger when yer really enraged.. mind over matter
     type thing.. */
  af.location = APPLY_STR;
  af.modifier = 1;
  af.bitvector = AFF_BERSERK;
  affect_to_char( ch, &af );
  send_to_char( "You start to lose control..\r\n", ch );
  learn_from_success( ch, gsn_berserk );
  return;
}

/* External from fight.c */
ch_ret one_hit( CHAR_DATA * ch, CHAR_DATA * victim, int dt );
void do_hitall( CHAR_DATA * ch, char *argument )
{
  CHAR_DATA *vch;
  CHAR_DATA *vch_next;
  short nvict = 0;
  short nhit = 0;
  short percent;

  if( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
  {
    send_to_char( "You cannot do that here.\r\n", ch );
    return;
  }

  if( !ch->in_room->first_person )
  {
    send_to_char( "There's no one here!\r\n", ch );
    return;
  }
  percent = IS_NPC( ch ) ? 80 : character_skill_level( ch, gsn_hitall );
  for( vch = ch->in_room->first_person; vch; vch = vch_next )
  {
    vch_next = vch->next_in_room;
    if( is_same_group( ch, vch ) || !is_legal_kill( ch, vch ) ||
	!can_see( ch, vch ) || is_safe( ch, vch ) )
      continue;
    if( ++nvict > character_skill_level( ch, gsn_hitall ) / 5 )
      break;
    if( luck_check( ch, percent ) )
    {
      nhit++;
      global_retcode = one_hit( ch, vch, TYPE_UNDEFINED );
    }
    else
      global_retcode = damage( ch, vch, 0, TYPE_UNDEFINED );
    /* Fireshield, etc. could kill ch too.. :>.. -- Altrag */
    if( global_retcode == rCHAR_DIED || global_retcode == rBOTH_DIED
	|| char_died( ch ) )
      return;
  }
  if( !nvict )
  {
    send_to_char( "There's no one here!\r\n", ch );
    return;
  }
  ch->move = UMAX( 0, ch->move - nvict * 3 + nhit );
  if( nhit )
    learn_from_success( ch, gsn_hitall );
  else
    learn_from_failure( ch, gsn_hitall );
  return;
}



bool check_illegal_psteal( CHAR_DATA * ch, CHAR_DATA * victim )
{
  return FALSE;
}

void do_scan( CHAR_DATA * ch, char *argument )
{
  ROOM_INDEX_DATA *was_in_room;
  ROOM_INDEX_DATA *to_room;
  EXIT_DATA *pexit;
  short dir = -1;
  short dist;
  short max_dist = 5;

  if( IS_AFFECTED( ch, AFF_BLIND )
      && ( !IS_AFFECTED( ch, AFF_TRUESIGHT ) ||
	( !IS_NPC( ch ) && !IS_SET( ch->act, PLR_HOLYLIGHT ) ) ) )
  {
    send_to_char( "Everything looks the same when you're blind...\r\n",
	ch );
    return;
  }

  if( argument[0] == '\0' )
    return;

  if( ( dir = get_door( argument ) ) == -1 )
    return;

  was_in_room = ch->in_room;

  if( ( pexit = get_exit( ch->in_room, dir ) ) == NULL )
  {
    act( AT_GREY, "You can't see any further $t.", ch, dir_name[dir], NULL,
	TO_CHAR );
    return;
  }

  if( ch->top_level < 50 )
    max_dist--;
  if( ch->top_level < 20 )
    max_dist--;

  for( dist = 1; dist <= max_dist; )
  {
    if( IS_SET( pexit->exit_info, EX_CLOSED ) )
    {
      if( IS_SET( pexit->exit_info, EX_SECRET ) )
	act( AT_GREY, "Your view $t is blocked by a wall.", ch,
	    dir_name[dir], NULL, TO_CHAR );
      else
	act( AT_GREY, "Your view $t is blocked by a door.", ch,
	    dir_name[dir], NULL, TO_CHAR );
      break;
    }

    to_room = pexit->to_room;

    if( room_is_private( ch, to_room ) && !IS_IMMORTAL( ch ) )
    {
      act( AT_GREY, "Your view $t is blocked by a private room.", ch,
	  dir_name[dir], NULL, TO_CHAR );
      break;
    }
    char_from_room( ch );
    char_to_room( ch, to_room );
    set_char_color( AT_RMNAME, ch );
    send_to_char( ch->in_room->name, ch );
    send_to_char( "\r\n", ch );
    show_list_to_char( ch->in_room->first_content, ch, FALSE, FALSE );
    show_char_to_char( ch->in_room->first_person, ch );

    switch ( ch->in_room->sector_type )
    {
      default:
	dist++;
	break;
      case SECT_AIR:
	if( number_percent() < 80 )
	  dist++;
	break;
      case SECT_INSIDE:
      case SECT_FIELD:
      case SECT_UNDERGROUND:
	dist++;
	break;
      case SECT_FOREST:
      case SECT_CITY:
      case SECT_DESERT:
      case SECT_HILLS:
	dist += 2;
	break;
      case SECT_WATER_SWIM:
      case SECT_WATER_NOSWIM:
	dist += 3;
	break;
      case SECT_MOUNTAIN:
      case SECT_UNDERWATER:
      case SECT_OCEANFLOOR:
	dist += 4;
	break;
    }

    if( dist >= max_dist )
    {
      act( AT_GREY, "Your vision blurs with distance and you see no "
	  "farther $t.", ch, dir_name[dir], NULL, TO_CHAR );
      break;
    }
    if( ( pexit = get_exit( ch->in_room, dir ) ) == NULL )
    {
      act( AT_GREY, "Your view $t is blocked by a wall.", ch,
	  dir_name[dir], NULL, TO_CHAR );
      break;
    }
  }

  char_from_room( ch );
  char_to_room( ch, was_in_room );

  return;
}

void do_slice( CHAR_DATA * ch, char *argument )
{
}

SKILLTYPE *create_skill( void )
{
  SKILLTYPE *skill = NULL;
  CREATE( skill, SKILLTYPE, 1 );
  return skill;
}

