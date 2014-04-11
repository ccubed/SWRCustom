#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "mud.h"

/*
 *  Locals
 */
void do_gold( CHAR_DATA * ch, char *argument )
{
  set_char_color( AT_GOLD, ch );
  ch_printf( ch, "You have %d credits.\r\n", ch->gold );
}

void do_score( CHAR_DATA * ch, char *argument )
{
  if( IS_NPC( ch ) || !ch->pcdata )
    do_oldscore( ch, STRLIT_EMPTY );

  if( IS_AFFECTED( ch, AFF_POSSESS ) )
  {
    send_to_char( "You can't do that in your current state of mind!\r\n",
	ch );
    return;
  }

  ch_printf( ch, "&G%s\r\n", ch->pcdata->title );
  ch_printf( ch,
      "&W------------------------------------------------------------------------\r\n" );

  ch_printf( ch, "&WName: &G%s   &WAge: &G%d\r\n", ch->name, get_age( ch ) );

  send_to_char( "&WStrength: &G", ch );
  {
    int score = get_curr_str( ch );

    if( score >= 20 )
      send_to_char( "immortal", ch );
    else if( score > 17 )
      send_to_char( "superior", ch );
    else if( score > 15 )
      send_to_char( "strong", ch );
    else if( score > 12 )
      send_to_char( "good", ch );
    else if( score > 9 )
      send_to_char( "average", ch );
    else if( score > 7 )
      send_to_char( "weak", ch );
    else if( score > 5 )
      send_to_char( "wimpy", ch );
    else
      send_to_char( "pathetic", ch );
  }
  send_to_char( "&W   Constitution: &G", ch );
  {
    int score = get_curr_con( ch );

    if( score >= 20 )
      send_to_char( "immortal", ch );
    else if( score > 17 )
      send_to_char( "superior", ch );
    else if( score > 15 )
      send_to_char( "strong", ch );
    else if( score > 12 )
      send_to_char( "good", ch );
    else if( score > 9 )
      send_to_char( "average", ch );
    else if( score > 7 )
      send_to_char( "weak", ch );
    else if( score > 5 )
      send_to_char( "wimpy", ch );
    else
      send_to_char( "pathetic", ch );
  }
  send_to_char( "&W   Dexterity: &G", ch );
  {
    int score = get_curr_dex( ch );

    if( score >= 20 )
      send_to_char( "immortal", ch );
    else if( score > 17 )
      send_to_char( "superior", ch );
    else if( score > 15 )
      send_to_char( "agile", ch );
    else if( score > 12 )
      send_to_char( "good", ch );
    else if( score > 9 )
      send_to_char( "average", ch );
    else if( score > 7 )
      send_to_char( "poor", ch );
    else if( score > 5 )
      send_to_char( "uncoordinated", ch );
    else
      send_to_char( "pathetic", ch );
  }

  send_to_char( "\r\n", ch );
  send_to_char( "&WIntelligence: &G", ch );
  {
    int score = get_curr_int( ch );

    if( score >= 20 )
      send_to_char( "immortal", ch );
    else if( score > 17 )
      send_to_char( "superior", ch );
    else if( score > 15 )
      send_to_char( "bright", ch );
    else if( score > 12 )
      send_to_char( "good", ch );
    else if( score > 9 )
      send_to_char( "average", ch );
    else if( score > 7 )
      send_to_char( "dim", ch );
    else if( score > 5 )
      send_to_char( "stupid", ch );
    else
      send_to_char( "brainless", ch );
  }
  send_to_char( "&W   Wisdom: &G", ch );
  {
    int score = get_curr_wis( ch );

    if( score >= 20 )
      send_to_char( "immortal", ch );
    else if( score > 17 )
      send_to_char( "superior", ch );
    else if( score > 15 )
      send_to_char( "wise", ch );
    else if( score > 12 )
      send_to_char( "good", ch );
    else if( score > 9 )
      send_to_char( "average", ch );
    else if( score > 7 )
      send_to_char( "poor", ch );
    else if( score > 5 )
      send_to_char( "unwise", ch );
    else
      send_to_char( "moron", ch );
  }
  send_to_char( "&W   Charisma: &G ", ch );
  {
    int score = get_curr_cha( ch );

    if( score >= 20 )
      send_to_char( "immortal", ch );
    else if( score > 17 )
      send_to_char( "superior", ch );
    else if( score > 15 )
      send_to_char( "charming", ch );
    else if( score > 12 )
      send_to_char( "good", ch );
    else if( score > 9 )
      send_to_char( "average", ch );
    else if( score > 7 )
      send_to_char( "unlikeable", ch );
    else if( score > 5 )
      send_to_char( "rude", ch );
    else
      send_to_char( "go hide", ch );
  }

  send_to_char( "\r\n", ch );

  ch_printf( ch,
      "&W------------------------------------------------------------------------\r\n" );

  send_to_char( "&WHealth &G ", ch );
  if( ch->hit >= 100 )
    send_to_char( "healthy", ch );
  else if( ch->hit > 90 )
    send_to_char( "slightly wounded", ch );
  else if( ch->hit > 75 )
    send_to_char( "hurt", ch );
  else if( ch->hit > 50 )
    send_to_char( "in pain", ch );
  else if( ch->hit > 35 )
    send_to_char( "severely wounded", ch );
  else if( ch->hit > 20 )
    send_to_char( "writhing in pain", ch );
  else if( ch->hit > 0 )
    send_to_char( "barely conscious", ch );
  else if( ch->hit > -300 )
    send_to_char( "uncounscious", ch );
  else
    send_to_char( "nearly DEAD", ch );

  send_to_char( "&W   Energy: &G ", ch );
  if( ch->move > 500 )
    send_to_char( "energetic", ch );
  else if( ch->move > 100 )
    send_to_char( "rested", ch );
  else if( ch->move > 50 )
    send_to_char( "worn out", ch );
  else if( ch->move > 0 )
    send_to_char( "exhausted", ch );
  else
    send_to_char( "immobile", ch );

  send_to_char( "&W   Mental: &G ", ch );

  if( !IS_NPC( ch ) && ch->pcdata->condition[COND_DRUNK] > 10 )
    send_to_char( "drunk", ch );
  else
    switch ( ch->mental_state / 10 )
    {
      default:
	send_to_char( "completely messed up", ch );
	break;
      case -10:
      case -9:
	send_to_char( "barely conscious", ch );
	break;
      case -8:
      case -7:
      case -6:
	send_to_char( "very drowsy", ch );
	break;
      case -5:
      case -4:
	send_to_char( "tired", ch );
	break;
      case -3:
      case -2:
	send_to_char( "under the weather", ch );
	break;
      case -1:
      case 0:
      case 1:
	send_to_char( "great", ch );
	break;
      case 2:
      case 3:
	send_to_char( "mind is racing", ch );
	break;
      case 4:
      case 5:
	send_to_char( "high as a kite", ch );
	break;
      case 6:
      case 7:
      case 8:
	send_to_char( "losing reality", ch );
	break;
      case 9:
      case 10:
	send_to_char( "psychotic", ch );
	break;
    }
  send_to_char( "\r\n", ch );

  send_to_char( "&WArmor: &G", ch );
  if( GET_AC( ch ) >= 80 )
    send_to_char( "naked", ch );
  else if( GET_AC( ch ) >= 60 )
    send_to_char( "clothed", ch );
  else if( GET_AC( ch ) >= 40 )
    send_to_char( "slightly protected", ch );
  else if( GET_AC( ch ) >= 20 )
    send_to_char( "somewhat protected", ch );
  else if( GET_AC( ch ) >= 0 )
    send_to_char( "armored", ch );
  else if( GET_AC( ch ) >= -20 )
    send_to_char( "well armored.", ch );
  else if( GET_AC( ch ) >= -40 )
    send_to_char( "strongly armored", ch );
  else if( GET_AC( ch ) >= -60 )
    send_to_char( "heavily armored", ch );
  else if( GET_AC( ch ) >= -80 )
    send_to_char( "superbly armored", ch );
  else if( GET_AC( ch ) >= -100 )
    send_to_char( "divinely armored.", ch );
  else
    send_to_char( "invincible", ch );

  send_to_char( "   &WAlignment: &G", ch );
  if( ch->alignment > 900 )
    send_to_char( "angelic", ch );
  else if( ch->alignment > 700 )
    send_to_char( "saintly", ch );
  else if( ch->alignment > 350 )
    send_to_char( "good", ch );
  else if( ch->alignment > 100 )
    send_to_char( "kind", ch );
  else if( ch->alignment > -100 )
    send_to_char( "neutral", ch );
  else if( ch->alignment > -350 )
    send_to_char( "mean", ch );
  else if( ch->alignment > -700 )
    send_to_char( "evil", ch );
  else if( ch->alignment > -900 )
    send_to_char( "demonic", ch );
  else
    send_to_char( "satanic", ch );

  ch_printf( ch, "   &WCredits: &G%ld &WBank: &G%ld\r\n",
	     ch->gold, !IS_NPC( ch ) ? ch->pcdata->bank : 0 );

  ch_printf( ch,
      "&W------------------------------------------------------------------------\r\n" );

  ch_printf( ch,
      "&WAutoexit: &G%s   &WAutoloot: &G%s   &WAutosac: &G%s   &WAutocred: &G%s\r\n",
      ( !IS_NPC( ch )
	&& IS_SET( ch->act, PLR_AUTOEXIT ) ) ? "yes" : "no",
      ( !IS_NPC( ch )
	&& IS_SET( ch->act, PLR_AUTOLOOT ) ) ? "yes" : "no",
      ( !IS_NPC( ch )
	&& IS_SET( ch->act, PLR_AUTOSAC ) ) ? "yes" : "no",
      ( !IS_NPC( ch )
	&& IS_SET( ch->act, PLR_AUTOGOLD ) ) ? "yes" : "no" );

  ch_printf( ch, "&WWimpy set to &G%d &Wpercent.\r\n", ch->wimpy );

  ch_printf( ch,
      "&W------------------------------------------------------------------------\r\n" );

  if( !IS_NPC( ch ) && ch->pcdata )
  {
    int sn;

    send_to_char( "&WSkilled at: &G", ch );
    for( sn = 0; sn < top_sn; sn++ )
      if( character_skill_level( ch, sn ) > 0
	  && character_skill_level( ch, sn ) < 100 )
	ch_printf( ch, "%s  ", skill_table[sn]->name );
    send_to_char( "\r\n", ch );

    send_to_char( "&WAdept at: &G", ch );
    for( sn = 0; sn < top_sn; sn++ )
      if( character_skill_level( ch, sn ) >= 100 )
	ch_printf( ch, "%s  ", skill_table[sn]->name );

    send_to_char( "\r\n", ch );
  }

  ch_printf( ch, "&W------------------------------------------------------------------------\r\n" );  

  if( !IS_NPC( ch ) && IS_IMMORTAL( ch ) )
  {
    ch_printf( ch, "&WWizInvis level: &G%d   &WWizInvis is &G%s\r\n",
	       ch->pcdata->wizinvis,
	       IS_SET( ch->act, PLR_WIZINVIS ) ? "ON" : "OFF" );
  }

  switch ( ch->position )
  {
    case POS_DEAD:
      send_to_char( "&GYou are DEAD!!   ", ch );
      break;
    case POS_MORTAL:
      send_to_char( "&GYou are mortally wounded.   ", ch );
      break;
    case POS_INCAP:
      send_to_char( "&GYou are incapacitated.   ", ch );
      break;
    case POS_STUNNED:
      send_to_char( "&GYou are stunned.   ", ch );
      break;
    case POS_SLEEPING:
      send_to_char( "&GYou are sleeping.   ", ch );
      break;
    case POS_RESTING:
      send_to_char( "&GYou are resting.   ", ch );
      break;
    case POS_STANDING:
      send_to_char( "&GYou are standing.   ", ch );
      break;
    case POS_FIGHTING:
      send_to_char( "&GYou are fighting.   ", ch );
      break;
    case POS_MOUNTED:
      send_to_char( "&GYou are mounted.   ", ch );
      break;
    case POS_SHOVE:
      send_to_char( "&GYou are being shoved.   ", ch );
      break;
    case POS_DRAG:
      send_to_char( "&GYou are being dragged.   ", ch );
      break;
    default:
      break;
  }

  if( !IS_NPC( ch ) && ch->pcdata->condition[COND_THIRST] == 0 )
    send_to_char( "&GYou are thirsty.   ", ch );

  if( !IS_NPC( ch ) && ch->pcdata->condition[COND_FULL] == 0 )
    send_to_char( "&GYou are hungry.   ", ch );

  send_to_char( "\r\n", ch );
}

/**
 * Command that displays to the character its skills and skill levels.
 *
 * @param ch       Character that initiated the command.
 * @param argument Argument passed to the command by the player.
 */
void do_skills ( CHAR_DATA * ch, char *argument )
{
	int skills = 0;
	int sn;
	int level;

	send_to_char( "&W-----------------------------------[ &GSkills&W ]-----------------------------------\r\n", ch );

    for( sn = 0; sn < top_sn; sn++ )
	{
		level = character_skill_level( ch, sn );

        if( level > 0)
		{
			/* 15 spaces for the skills */
	        ch_printf( ch, "&W%-15.15s ", skill_table[sn]->name );

			// TODO: base this off of the starting skill level
			if (level <= 25)
				ch_printf( ch, "&G%-10s", "Inept" );
			else if (level <= 50)
				ch_printf( ch, "&G%-10s", "Unskilled" );
			else if (level <= 60 )
				ch_printf( ch, "&G%-10s", "Average" );
			else if (level <=70 )
				ch_printf( ch, "&G%-10s", "Proficient" );
			else if (level < 60)
				ch_printf( ch, "&G%-10s", "Skilled" );
			else if (level < 90)
				ch_printf( ch, "&G%-10s", "Adept" );
			else
				ch_printf( ch, "&C%-10s", "Mastered" );
			
			/* Only 3 columns of skills */
			if ( ++skills % 3 == 0)
				send_to_char( "\r\n", ch );
			else 
				send_to_char( " ", ch );
		}		
	}

	if (skills == 0)
		send_to_char( "&GYou don't have any skills... how sad.\r\n", ch);		
	else if (skills % 3 != 0) /* Make sure there is a new line after the last skill */
		send_to_char( "\r\n", ch );

	send_to_char( "&W--------------------------------------------------------------------------------\r\n\r\n", ch );
}

void do_oldscore( CHAR_DATA * ch, char *argument )
{
  if( IS_AFFECTED( ch, AFF_POSSESS ) )
  {
    send_to_char( "You can't do that in your current state of mind!\r\n",
	ch );
    return;
  }

  set_char_color( AT_SCORE, ch );
  ch_printf( ch,
      "You are %s, the %d year old.\r\n",
      IS_NPC( ch ) ? ch->name : ch->pcdata->title, get_age( ch ) );

  send_to_char( "You are ", ch );
  if( ch->hit >= 100 )
    send_to_char( "healthy.", ch );
  else if( ch->hit > 90 )
    send_to_char( "slightly wounded.", ch );
  else if( ch->hit > 75 )
    send_to_char( "hurt.", ch );
  else if( ch->hit > 50 )
    send_to_char( "in pain.", ch );
  else if( ch->hit > 35 )
    send_to_char( "severely wounded.", ch );
  else if( ch->hit > 20 )
    send_to_char( "writhing in pain.", ch );
  else if( ch->hit > 0 )
    send_to_char( "barely conscious.", ch );
  else if( ch->hit > -300 )
    send_to_char( "uncounscious.", ch );
  else
    send_to_char( "nearly DEAD.", ch );


  send_to_char( "   You are ", ch );
  if( ch->move > 500 )
    send_to_char( "energetic.\r\n", ch );
  else if( ch->move > 100 )
    send_to_char( "rested.\r\n", ch );
  else if( ch->move > 50 )
    send_to_char( "worn out.\r\n", ch );
  else if( ch->move > 0 )
    send_to_char( "exhausted.\r\n", ch );
  else
    send_to_char( "to tired to move.\r\n", ch );


  if( !IS_NPC( ch ) && ch->pcdata->condition[COND_DRUNK] > 10 )
    send_to_char( "You are drunk.\r\n", ch );
  if( !IS_NPC( ch ) && ch->pcdata->condition[COND_THIRST] == 0 )
    send_to_char( "You are thirsty.\r\n", ch );
  if( !IS_NPC( ch ) && ch->pcdata->condition[COND_FULL] == 0 )
    send_to_char( "You are hungry.\r\n", ch );

  switch ( ch->mental_state / 10 )
  {
    default:
      send_to_char( "You're completely messed up!", ch );
      break;
    case -10:
      send_to_char( "You're barely conscious.", ch );
      break;
    case -9:
      send_to_char( "You can barely keep your eyes open.", ch );
      break;
    case -8:
      send_to_char( "You're extremely drowsy.", ch );
      break;
    case -7:
      send_to_char( "You feel very unmotivated.", ch );
      break;
    case -6:
      send_to_char( "You feel sedated.", ch );
      break;
    case -5:
      send_to_char( "You feel sleepy.", ch );
      break;
    case -4:
      send_to_char( "You feel tired.", ch );
      break;
    case -3:
      send_to_char( "You could use a rest.", ch );
      break;
    case -2:
      send_to_char( "You feel a little under the weather.", ch );
      break;
    case -1:
      send_to_char( "You feel fine.", ch );
      break;
    case 0:
      send_to_char( "You feel great.", ch );
      break;
    case 1:
      send_to_char( "You feel energetic.", ch );
      break;
    case 2:
      send_to_char( "Your mind is racing.", ch );
      break;
    case 3:
      send_to_char( "You can't think straight.", ch );
      break;
    case 4:
      send_to_char( "Your mind is going 100 miles an hour.", ch );
      break;
    case 5:
      send_to_char( "You're high as a kite.", ch );
      break;
    case 6:
      send_to_char( "Your mind and body are slipping appart.", ch );
      break;
    case 7:
      send_to_char( "Reality is slipping away.", ch );
      break;
    case 8:
      send_to_char( "You have no idea what is real, and what is not.", ch );
      break;
    case 9:
      send_to_char( "You feel immortal.", ch );
      break;
    case 10:
      send_to_char( "You are a Supreme Entity.", ch );
      break;
  }

  switch ( ch->position )
  {
    case POS_DEAD:
      send_to_char( "   You are DEAD!!\r\n", ch );
      break;
    case POS_MORTAL:
      send_to_char( "   You are mortally wounded.\r\n", ch );
      break;
    case POS_INCAP:
      send_to_char( "   You are incapacitated.\r\n", ch );
      break;
    case POS_STUNNED:
      send_to_char( "   You are stunned.\r\n", ch );
      break;
    case POS_SLEEPING:
      send_to_char( "   You are sleeping.\r\n", ch );
      break;
    case POS_RESTING:
      send_to_char( "   You are resting.\r\n", ch );
      break;
    case POS_STANDING:
      send_to_char( "   You are standing.\r\n", ch );
      break;
    case POS_FIGHTING:
      send_to_char( "   You are fighting.\r\n", ch );
      break;
    case POS_MOUNTED:
      send_to_char( "   Mounted.\r\n", ch );
      break;
    case POS_SHOVE:
      send_to_char( "   Being shoved.\r\n", ch );
      break;
    case POS_DRAG:
      send_to_char( "   Being dragged.\r\n", ch );
      break;
    default:
      send_to_char( "\r\n", ch );
      break;
  }

  send_to_char( "You are ", ch );

  if( GET_AC( ch ) >= 101 )
    send_to_char( "WORSE than naked!", ch );
  else if( GET_AC( ch ) >= 80 )
    send_to_char( "naked.", ch );
  else if( GET_AC( ch ) >= 60 )
    send_to_char( "wearing clothes.", ch );
  else if( GET_AC( ch ) >= 40 )
    send_to_char( "slightly armored.", ch );
  else if( GET_AC( ch ) >= 20 )
    send_to_char( "somewhat armored.", ch );
  else if( GET_AC( ch ) >= 0 )
    send_to_char( "armored.", ch );
  else if( GET_AC( ch ) >= -20 )
    send_to_char( "well armored.", ch );
  else if( GET_AC( ch ) >= -40 )
    send_to_char( "strongly armored.", ch );
  else if( GET_AC( ch ) >= -60 )
    send_to_char( "heavily armored.", ch );
  else if( GET_AC( ch ) >= -80 )
    send_to_char( "superbly armored.", ch );
  else if( GET_AC( ch ) >= -100 )
    send_to_char( "divinely armored.", ch );
  else
    send_to_char( "invincible!", ch );

  send_to_char( "   You are ", ch );
  if( ch->alignment > 900 )
    send_to_char( "angelic.\r\n", ch );
  else if( ch->alignment > 700 )
    send_to_char( "saintly.\r\n", ch );
  else if( ch->alignment > 350 )
    send_to_char( "good.\r\n", ch );
  else if( ch->alignment > 100 )
    send_to_char( "kind.\r\n", ch );
  else if( ch->alignment > -100 )
    send_to_char( "neutral.\r\n", ch );
  else if( ch->alignment > -350 )
    send_to_char( "mean.\r\n", ch );
  else if( ch->alignment > -700 )
    send_to_char( "evil.\r\n", ch );
  else if( ch->alignment > -900 )
    send_to_char( "demonic.\r\n", ch );
  else
    send_to_char( "satanic.\r\n", ch );

  ch_printf( ch, "You have have %d credits.\r\n", ch->gold );

  ch_printf( ch,
      "Autoexit: %s   Autoloot: %s   Autosac: %s   Autocred: %s\r\n",
      ( !IS_NPC( ch )
	&& IS_SET( ch->act, PLR_AUTOEXIT ) ) ? "yes" : "no",
      ( !IS_NPC( ch )
	&& IS_SET( ch->act, PLR_AUTOLOOT ) ) ? "yes" : "no",
      ( !IS_NPC( ch )
	&& IS_SET( ch->act, PLR_AUTOSAC ) ) ? "yes" : "no",
      ( !IS_NPC( ch )
	&& IS_SET( ch->act, PLR_AUTOGOLD ) ) ? "yes" : "no" );

  ch_printf( ch, "Wimpy set to %d percent.\r\n", ch->wimpy );

  if( !IS_NPC( ch ) && ch->pcdata )
  {
    int sn;

    send_to_char( "You are skilled at: ", ch );
    for( sn = 0; sn < top_sn; sn++ )
      if( character_skill_level( ch, sn ) > 0
	  && character_skill_level( ch, sn ) < 100 )
	ch_printf( ch, "%s  ", skill_table[sn]->name );
    send_to_char( "\r\n", ch );

    send_to_char( "You are an adept of: ", ch );
    for( sn = 0; sn < top_sn; sn++ )
      if( character_skill_level( ch, sn ) >= 100 )
	ch_printf( ch, "%s  ", skill_table[sn]->name );
    send_to_char( "\r\n", ch );

  }

  if( !IS_NPC( ch ) && IS_IMMORTAL( ch ) )
  {
    ch_printf( ch, "WizInvis level: %d   WizInvis is %s\r\n",
	ch->pcdata->wizinvis,
	IS_SET( ch->act, PLR_WIZINVIS ) ? "ON" : "OFF" );
  }
}

void do_affected( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];

  if( IS_NPC( ch ) )
    return;

  argument = one_argument( argument, arg );

  if( !str_cmp( arg, "by" ) )
  {
    set_char_color( AT_BLUE, ch );
    send_to_char( "\r\nImbued with:\r\n", ch );
    set_char_color( AT_SCORE, ch );
    ch_printf( ch, "%s\r\n", affect_bit_name( ch->affected_by ) );
    return;
  }

  if( !ch->first_affect )
  {
    set_char_color( AT_SCORE, ch );
    send_to_char( "\r\nNo skill affects you.\r\n", ch );
  }
  else
  {
    AFFECT_DATA *paf = NULL;
    SKILLTYPE *skill = NULL;
    send_to_char( "\r\n", ch );

    for( paf = ch->first_affect; paf; paf = paf->next )
      if( ( skill = get_skilltype( paf->type ) ) != NULL )
      {
	set_char_color( AT_BLUE, ch );
	send_to_char( "Affected:  ", ch );
	set_char_color( AT_SCORE, ch );
	ch_printf( ch, "%-18s\r\n", skill->name );
      }
  }
}

void do_inventory( CHAR_DATA * ch, char *argument )
{
  set_char_color( AT_RED, ch );
  send_to_char( "You are carrying:\r\n", ch );
  show_list_to_char( ch->first_carrying, ch, TRUE, TRUE );
}

void do_equipment( CHAR_DATA * ch, char *argument )
{
  OBJ_DATA *obj = NULL;
  int iWear = 0, dam = 0;
  bool found = FALSE;
  char buf[MAX_STRING_LENGTH];
  *buf = '\0';

  set_char_color( AT_RED, ch );
  send_to_char( "You are using:\r\n", ch );
  set_char_color( AT_OBJECT, ch );

  for( iWear = 0; iWear < MAX_WEAR; iWear++ )
  {
    for( obj = ch->first_carrying; obj; obj = obj->next_content )
      if( obj->wear_loc == iWear )
      {
	send_to_char( where_name[iWear], ch );

	if( can_see_obj( ch, obj ) )
	{
	  send_to_char( format_obj_to_char( obj, ch, TRUE ), ch );
	  strcpy( buf, "" );

	  switch ( obj->item_type )
	  {
	    default:
	      break;

	    case ITEM_ARMOR:
	      if( obj->value[1] == 0 )
		obj->value[1] = obj->value[0];
	      if( obj->value[1] == 0 )
		obj->value[1] = 1;

	      dam = ( short ) ( ( obj->value[0] * 10 ) / obj->value[1] );

	      if( dam >= 10 )
		strcat( buf, " (superb) " );
	      else if( dam >= 7 )
		strcat( buf, " (good) " );
	      else if( dam >= 5 )
		strcat( buf, " (worn) " );
	      else if( dam >= 3 )
		strcat( buf, " (poor) " );
	      else if( dam >= 1 )
		strcat( buf, " (awful) " );
	      else if( dam == 0 )
		strcat( buf, " (broken) " );

	      send_to_char( buf, ch );
	      break;

	    case ITEM_WEAPON:
	      dam = INIT_WEAPON_CONDITION - obj->value[0];

	      if( dam < 2 )
		strcat( buf, " (superb) " );
	      else if( dam < 4 )
		strcat( buf, " (good) " );
	      else if( dam < 7 )
		strcat( buf, " (worn) " );
	      else if( dam < 10 )
		strcat( buf, " (poor) " );
	      else if( dam < 12 )
		strcat( buf, " (awful) " );
	      else if( dam == 12 )
		strcat( buf, " (broken) " );

	      send_to_char( buf, ch );

	      if( obj->value[3] == WEAPON_BLASTER )
	      {
		if( obj->blaster_setting == BLASTER_FULL )
		  ch_printf( ch, "FULL" );
		else if( obj->blaster_setting == BLASTER_HIGH )
		  ch_printf( ch, "HIGH" );
		else if( obj->blaster_setting == BLASTER_NORMAL )
		  ch_printf( ch, "NORMAL" );
		else if( obj->blaster_setting == BLASTER_HALF )
		  ch_printf( ch, "HALF" );
		else if( obj->blaster_setting == BLASTER_LOW )
		  ch_printf( ch, "LOW" );
		else if( obj->blaster_setting == BLASTER_STUN )
		  ch_printf( ch, "STUN" );
		ch_printf( ch, " %d", obj->value[4] );
	      }
	      else if( ( obj->value[3] == WEAPON_LIGHTSABER ||
		    obj->value[3] == WEAPON_VIBRO_BLADE ) )
	      {
		ch_printf( ch, "%d", obj->value[4] );
	      }
	      break;
	  }
	  send_to_char( "\r\n", ch );
	}
	else
	  send_to_char( "something.\r\n", ch );

	found = TRUE;
      }
  }

  if( !found )
    send_to_char( "Nothing.\r\n", ch );
}

void set_title( CHAR_DATA * ch, const char *title )
{
  char buf[MAX_STRING_LENGTH];
  *buf = '\0';

  if( IS_NPC( ch ) )
  {
    bug( "Set_title: NPC.", 0 );
    return;
  }

  if( isalpha( ( int ) title[0] ) || isdigit( ( int ) title[0] ) )
  {
    buf[0] = ' ';
    strcpy( buf + 1, title );
  }
  else
  {
    strcpy( buf, title );
  }

  STRFREE( ch->pcdata->title );
  ch->pcdata->title = STRALLOC( buf );
}

void do_title( CHAR_DATA * ch, char *argument )
{
  if( IS_NPC( ch ) )
    return;

  if( IS_SET( ch->pcdata->flags, PCFLAG_NOTITLE ) )
  {
    send_to_char( "You try but the Force resists you.\r\n", ch );
    return;
  }

  if( argument[0] == '\0' )
  {
    send_to_char( "Change your title to what?\r\n", ch );
    return;
  }

  if( !IS_IMMORTAL( ch ) && ( !nifty_is_name( ch->name, argument ) ) )
  {
    send_to_char( "You must include your name somewhere in your title!",
	ch );
    return;
  }

  smash_tilde( argument );
  set_title( ch, argument );
  send_to_char( "Ok.\r\n", ch );
}

void do_homepage( CHAR_DATA * ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];

  if( IS_NPC( ch ) )
    return;

  if( argument[0] == '\0' )
  {
    if( !ch->pcdata->homepage )
      ch->pcdata->homepage = str_dup( "" );

    ch_printf( ch, "Your homepage is: %s\r\n",
	show_tilde( ch->pcdata->homepage ) );

    return;
  }

  if( !str_cmp( argument, "clear" ) )
  {
    if( ch->pcdata->homepage )
      DISPOSE( ch->pcdata->homepage );

    ch->pcdata->homepage = str_dup( "" );
    send_to_char( "Homepage cleared.\r\n", ch );
    return;
  }

  if( strstr( argument, "://" ) )
    strcpy( buf, argument );
  else
    sprintf( buf, "http://%s", argument );

  if( strlen( buf ) > 70 )
    buf[70] = '\0';

  hide_tilde( buf );

  if( ch->pcdata->homepage )
    DISPOSE( ch->pcdata->homepage );

  ch->pcdata->homepage = str_dup( buf );
  send_to_char( "Homepage set.\r\n", ch );
}

/*
 * Set your personal description				-Thoric
 */
void do_description( CHAR_DATA * ch, char *argument )
{
  if( IS_NPC( ch ) )
  {
    send_to_char( "Monsters are too dumb to do that!\r\n", ch );
    return;
  }

  if( !ch->desc )
  {
    bug( "do_description: no descriptor", 0 );
    return;
  }

  switch ( ch->substate )
  {
    default:
      bug( "do_description: illegal substate", 0 );
      return;

    case SUB_RESTRICTED:
      send_to_char
	( "You cannot use this command from within another command.\r\n",
	  ch );
      return;

    case SUB_NONE:
      ch->substate = SUB_PERSONAL_DESC;
      ch->dest_buf = ch;
      start_editing( ch, ch->description );
      return;

    case SUB_PERSONAL_DESC:
      STRFREE( ch->description );
      ch->description = copy_buffer( ch );
      stop_editing( ch );
      return;
  }
}

/* Ripped off do_description for whois bio's -- Scryn*/
void do_bio( CHAR_DATA * ch, char *argument )
{
  if( IS_NPC( ch ) )
  {
    send_to_char( "Mobs can't set bio's!\r\n", ch );
    return;
  }

  if( !ch->desc )
  {
    bug( "do_bio: no descriptor", 0 );
    return;
  }

  switch ( ch->substate )
  {
    default:
      bug( "do_bio: illegal substate", 0 );
      return;

    case SUB_RESTRICTED:
      send_to_char
	( "You cannot use this command from within another command.\r\n",
	  ch );
      return;

    case SUB_NONE:
      ch->substate = SUB_PERSONAL_BIO;
      ch->dest_buf = ch;
      start_editing( ch, ch->pcdata->bio );
      return;

    case SUB_PERSONAL_BIO:
      STRFREE( ch->pcdata->bio );
      ch->pcdata->bio = copy_buffer( ch );
      stop_editing( ch );
      return;
  }
}

void do_prompt( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];

  if( IS_NPC( ch ) )
  {
    send_to_char( "NPC's can't change their prompt..\r\n", ch );
    return;
  }

  smash_tilde( argument );
  one_argument( argument, arg );

  if( !*arg )
  {
    send_to_char( "Set prompt to what? (try help prompt)\r\n", ch );
    return;
  }

  if( ch->pcdata->prompt )
    STRFREE( ch->pcdata->prompt );

  if( strlen( argument ) > 128 )
    argument[128] = '\0';

  /* Can add a list of pre-set prompts here if wanted.. perhaps
     'prompt 1' brings up a different, pre-set prompt */
  if( !str_cmp( arg, "default" ) )
    ch->pcdata->prompt = STRALLOC( "" );
  else
    ch->pcdata->prompt = STRALLOC( argument );

  send_to_char( "Ok.\r\n", ch );
}
