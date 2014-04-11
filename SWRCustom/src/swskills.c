#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "mud.h"

void add_reinforcements( CHAR_DATA * ch );
ch_ret one_hit( CHAR_DATA * ch, CHAR_DATA * victim, int dt );
int xp_compute( CHAR_DATA * ch, CHAR_DATA * victim );
int ris_save( CHAR_DATA * ch, int chance, int ris );
CHAR_DATA *get_char_room_mp( CHAR_DATA * ch, char *argument );
void clear_roomtype( ROOM_INDEX_DATA * room );

extern int top_affect;
extern int top_r_vnum;

void do_makeblade( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  int level = 0, chance = 0, charge = 0;
  OBJ_DATA *vibroblade = NULL, *toolkit = NULL, *durasteel = NULL, *battery = NULL, *oven = NULL;
  OBJ_INDEX_DATA *pObjIndex = NULL;
  AFFECT_DATA *paf = NULL;
  AFFECT_DATA *paf2 = NULL;

  strcpy( arg, argument );

  switch ( ch->substate )
  {
    default:

      if( arg[0] == '\0' )
      {
	send_to_char( "&RUsage: Makeblade <name>\r\n&w", ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_TOOLKIT ) )
      {
	send_to_char( "&RYou need toolkit to make a vibro-blade.\r\n", ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_METAL ) )
      {
	send_to_char( "&RYou need something to make it out of.\r\n", ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_BATTERY ) )
      {
	send_to_char( "&RYou need a power source for your blade.\r\n", ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_OVEN ) )
      {
	send_to_char( "&RYou need a small furnace to heat the metal.\r\n",
	    ch );
	return;
      }

      chance = character_skill_level( ch, gsn_makeblade );

      if( number_percent() < chance )
      {
	send_to_char
	  ( "&GYou begin the long process of crafting a vibroblade.\r\n",
	    ch );
	act( AT_PLAIN,
	    "$n takes $s tools and a small oven and begins to work on something.",
	    ch, NULL, argument, TO_ROOM );
	add_timer( ch, TIMER_DO_FUN, 10, do_makeblade, 1 );
	ch->dest_buf = str_dup( arg );
	return;
      }
      send_to_char
	( "&RYou can't figure out how to fit the parts together.\r\n", ch );
      learn_from_failure( ch, gsn_makeblade );
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
      send_to_char( "&RYou are interupted and fail to finish your work.\r\n",
	  ch );
      return;
  }

  ch->substate = SUB_NONE;

  level = character_skill_level( ch, gsn_makeblade );

  if( ( pObjIndex = get_obj_index( OBJ_VNUM_PROTO_BLADE ) ) == NULL )
  {
    send_to_char
      ( "&RThe item you are trying to create is missing from the database.\r\nPlease inform the administration of this error.\r\n",
	ch );
    return;
  }

  if( !( toolkit = get_obj_type_char( ch, ITEM_TOOLKIT ) )
      || !( durasteel = get_obj_type_char( ch, ITEM_METAL ) )
      || !( battery = get_obj_type_char( ch, ITEM_BATTERY ) )
      || !( oven = get_obj_type_char( ch, ITEM_OVEN ) ) )
    {
      ch_printf( ch, "&RSome of your materials seem to have gone missing, and you can't finish the vibroblade.\r\n" );
      return;
    }

  separate_obj( durasteel );
  obj_from_char( durasteel );
  extract_obj( durasteel );

  charge = UMAX( 5, battery->value[0] );
  separate_obj( battery );
  obj_from_char( battery );
  extract_obj( battery );

  vibroblade = create_object( pObjIndex );

  SET_BIT( vibroblade->wear_flags, ITEM_WIELD );
  SET_BIT( vibroblade->wear_flags, ITEM_TAKE );
  vibroblade->weight = 3;
  STRFREE( vibroblade->name );
  strcpy( buf, arg );
  strcat( buf, " vibro-blade blade" );
  vibroblade->name = STRALLOC( buf );
  strcpy( buf, arg );
  STRFREE( vibroblade->short_descr );
  vibroblade->short_descr = STRALLOC( buf );
  STRFREE( vibroblade->description );
  strcat( buf, " was left here." );
  vibroblade->description = STRALLOC( buf );
  CREATE( paf, AFFECT_DATA, 1 );
  paf->type = -1;
  paf->duration = -1;
  paf->location = get_atype( "backstab" );
  paf->modifier = level / 3;
  paf->bitvector = 0;
  paf->next = NULL;
  LINK( paf, vibroblade->first_affect, vibroblade->last_affect, next, prev );
  ++top_affect;
  CREATE( paf2, AFFECT_DATA, 1 );
  paf2->type = -1;
  paf2->duration = -1;
  paf2->location = get_atype( "hitroll" );
  paf2->modifier = -2;
  paf2->bitvector = 0;
  paf2->next = NULL;
  LINK( paf2, vibroblade->first_affect, vibroblade->last_affect, next, prev );
  ++top_affect;
  vibroblade->value[0] = INIT_WEAPON_CONDITION;
  vibroblade->value[1] = ( int ) ( level / 20 + 10 );	/* min dmg  */
  vibroblade->value[2] = ( int ) ( level / 10 + 20 );	/* max dmg */
  vibroblade->value[3] = WEAPON_VIBRO_BLADE;
  vibroblade->value[4] = charge;
  vibroblade->value[5] = charge;
  vibroblade->cost = vibroblade->value[2] * 10;

  vibroblade = obj_to_char( vibroblade, ch );

  send_to_char
    ( "&GYou finish your work and hold up your newly created blade.&w\r\n",
      ch );
  act( AT_PLAIN, "$n finishes crafting a vibro-blade.", ch, NULL, argument,
      TO_ROOM );

  learn_from_success( ch, gsn_makeblade );
  learn_from_success( ch, gsn_makeblade );
  learn_from_success( ch, gsn_makeblade );
}

void do_makeblaster( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  int level, chance;
  bool checktool, checkdura, checkbatt, checkoven, checkcond, checkcirc,
       checkammo;
  OBJ_DATA *obj;
  OBJ_INDEX_DATA *pObjIndex;
  int vnum, power, scope, ammo;
  AFFECT_DATA *paf;
  AFFECT_DATA *paf2;

  strcpy( arg, argument );

  switch ( ch->substate )
  {
    default:
      if( arg[0] == '\0' )
      {
	send_to_char( "&RUsage: Makeblaster <name>\r\n&w", ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_TOOLKIT ) )
      {
	send_to_char( "&RYou need toolkit to make a blaster.\r\n", ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_PLASTIC ) )
      {
	send_to_char( "&RYou need something to make it out of.\r\n", ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_BATTERY ) )
      {
	send_to_char( "&RYou need a power source for your blaster.\r\n",
	    ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_OVEN ) )
      {
	send_to_char
	  ( "&RYou need a small furnace to heat the plastics.\r\n", ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_CIRCUIT ) )
      {
	send_to_char
	  ( "&RYou need a small circuit board to control the firing mechanism.\r\n",
	    ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_SUPERCONDUCTOR ) )
      {
	send_to_char( "&RYou still need a small superconductor.\r\n", ch );
	return;
      }

      chance = character_skill_level( ch, gsn_makeblaster );
      if( number_percent() < chance )
      {
	send_to_char
	  ( "&GYou begin the long process of making a blaster.\r\n", ch );
	act( AT_PLAIN,
	    "$n takes $s tools and a small oven and begins to work on something.",
	    ch, NULL, argument, TO_ROOM );
	add_timer( ch, TIMER_DO_FUN, 10, do_makeblaster, 1 );
	ch->dest_buf = str_dup( arg );
	return;
      }
      send_to_char
	( "&RYou can't figure out how to fit the parts together.\r\n", ch );
      learn_from_failure( ch, gsn_makeblaster );
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
      send_to_char( "&RYou are interupted and fail to finish your work.\r\n",
	  ch );
      return;
  }

  ch->substate = SUB_NONE;

  level = character_skill_level( ch, gsn_makeblaster );
  vnum = OBJ_VNUM_PROTO_BLASTER;

  if( ( pObjIndex = get_obj_index( vnum ) ) == NULL )
  {
    send_to_char
      ( "&RThe item you are trying to create is missing from the database.\r\nPlease inform the administration of this error.\r\n",
	ch );
    return;
  }

  checkammo = FALSE;
  checktool = FALSE;
  checkdura = FALSE;
  checkbatt = FALSE;
  checkoven = FALSE;
  checkcond = FALSE;
  checkcirc = FALSE;
  power = 0;
  scope = 0;
  ammo = 0;

  for( obj = ch->last_carrying; obj; obj = obj->prev_content )
  {
    if( obj->item_type == ITEM_TOOLKIT )
      checktool = TRUE;
    if( obj->item_type == ITEM_OVEN )
      checkoven = TRUE;
    if( obj->item_type == ITEM_PLASTIC && checkdura == FALSE )
    {
      checkdura = TRUE;
      separate_obj( obj );
      obj_from_char( obj );
      extract_obj( obj );
    }
    if( obj->item_type == ITEM_AMMO && checkammo == FALSE )
    {
      ammo = obj->value[0];
      checkammo = TRUE;
      separate_obj( obj );
      obj_from_char( obj );
      extract_obj( obj );
    }
    if( obj->item_type == ITEM_BATTERY && checkbatt == FALSE )
    {
      separate_obj( obj );
      obj_from_char( obj );
      extract_obj( obj );
      checkbatt = TRUE;
    }
    if( obj->item_type == ITEM_LENS && scope == 0 )
    {
      separate_obj( obj );
      obj_from_char( obj );
      extract_obj( obj );
      scope++;
    }
    if( obj->item_type == ITEM_SUPERCONDUCTOR && power < 2 )
    {
      power++;
      separate_obj( obj );
      obj_from_char( obj );
      extract_obj( obj );
      checkcond = TRUE;
    }
    if( obj->item_type == ITEM_CIRCUIT && checkcirc == FALSE )
    {
      separate_obj( obj );
      obj_from_char( obj );
      extract_obj( obj );
      checkcirc = TRUE;
    }
  }

  if( ( !checktool ) || ( !checkdura ) || ( !checkbatt ) || ( !checkoven )
      || ( !checkcond ) || ( !checkcirc ) )
  {
    send_to_char
      ( "&RYou hold up your new blaster and aim at a leftover piece of plastic.\r\n",
	ch );
    send_to_char
      ( "&RYou slowly squeeze the trigger hoping for the best...\r\n", ch );
    send_to_char
      ( "&RYour blaster backfires destroying your weapon and burning your hand.\r\n",
	ch );
    learn_from_failure( ch, gsn_makeblaster );
    return;
  }

  obj = create_object( pObjIndex );

  obj->item_type = ITEM_WEAPON;
  SET_BIT( obj->wear_flags, ITEM_WIELD );
  SET_BIT( obj->wear_flags, ITEM_TAKE );
  obj->weight = 2 + level / 10;
  STRFREE( obj->name );
  strcpy( buf, arg );
  strcat( buf, " blaster" );
  obj->name = STRALLOC( buf );
  strcpy( buf, arg );
  STRFREE( obj->short_descr );
  obj->short_descr = STRALLOC( buf );
  STRFREE( obj->description );
  strcat( buf, " was carelessly misplaced here." );
  obj->description = STRALLOC( buf );
  CREATE( paf, AFFECT_DATA, 1 );
  paf->type = -1;
  paf->duration = -1;
  paf->location = get_atype( "hitroll" );
  paf->modifier = URANGE( 0, 1 + scope, level / 30 );
  paf->bitvector = 0;
  paf->next = NULL;
  LINK( paf, obj->first_affect, obj->last_affect, next, prev );
  ++top_affect;
  CREATE( paf2, AFFECT_DATA, 1 );
  paf2->type = -1;
  paf2->duration = -1;
  paf2->location = get_atype( "damroll" );
  paf2->modifier = URANGE( 0, power, level / 30 );
  paf2->bitvector = 0;
  paf2->next = NULL;
  LINK( paf2, obj->first_affect, obj->last_affect, next, prev );
  ++top_affect;
  obj->value[0] = INIT_WEAPON_CONDITION;	/* condition  */
  obj->value[1] = ( int ) ( level / 10 + 15 );	/* min dmg  */
  obj->value[2] = ( int ) ( level / 5 + 25 );	/* max dmg  */
  obj->value[3] = WEAPON_BLASTER;
  obj->value[4] = ammo;
  obj->value[5] = 2000;
  obj->cost = obj->value[2] * 50;

  obj = obj_to_char( obj, ch );

  send_to_char
    ( "&GYou finish your work and hold up your newly created blaster.&w\r\n",
      ch );
  act( AT_PLAIN, "$n finishes making $s new blaster.", ch, NULL, argument,
      TO_ROOM );

  learn_from_success( ch, gsn_makeblaster );
  learn_from_success( ch, gsn_makeblaster );
  learn_from_success( ch, gsn_makeblaster );
}

void do_makelightsaber( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  int chance;
  bool checktool, checkdura, checkbatt,
       checkoven, checkcond, checkcirc, checklens, checkgems, checkmirr;
  OBJ_DATA *obj;
  OBJ_INDEX_DATA *pObjIndex;
  int level, gems, charge, gemtype;
  AFFECT_DATA *paf;
  AFFECT_DATA *paf2;

  strcpy( arg, argument );

  switch ( ch->substate )
  {
    default:
      if( arg[0] == '\0' )
      {
	send_to_char( "&RUsage: Makelightsaber <name>\r\n&w", ch );
	return;
      }

      if( !IS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
      {
	send_to_char
	  ( "&RYou need to be in a quiet peaceful place to craft a lightsaber.\r\n",
	    ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_TOOLKIT ) )
      {
	send_to_char( "&RYou need toolkit to make a lightsaber.\r\n", ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_PLASTIC )
	  && !get_obj_type_char( ch, ITEM_METAL ) )
      {
	send_to_char( "&RYou need something to make it out of.\r\n", ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_BATTERY ) )
      {
	send_to_char( "&RYou need a power source for your lightsaber.\r\n",
	    ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_OVEN ) )
      {
	send_to_char
	  ( "&RYou need a small furnace to heat and shape the components.\r\n",
	    ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_CIRCUIT ) )
      {
	send_to_char( "&RYou need a small circuit board.\r\n", ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_SUPERCONDUCTOR ) )
      {
	send_to_char
	  ( "&RYou still need a small superconductor for your lightsaber.\r\n",
	    ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_LENS ) )
      {
	send_to_char( "&RYou still need a lens to focus the beam.\r\n",
	    ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_CRYSTAL ) )
      {
	send_to_char
	  ( "&RLightsabers require 1 to 3 gems to work properly.\r\n", ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_MIRROR ) )
      {
	send_to_char
	  ( "&RYou need a high intesity reflective cup to create a lightsaber.\r\n",
	    ch );
	return;
      }

      chance = character_skill_level( ch, gsn_lightsaber_crafting );

      if( number_percent() < chance )
      {
	send_to_char
	  ( "&GYou begin the long process of crafting a lightsaber.\r\n",
	    ch );
	act( AT_PLAIN,
	    "$n takes $s tools and a small oven and begins to work on something.",
	    ch, NULL, argument, TO_ROOM );
	add_timer( ch, TIMER_DO_FUN, 10, do_makelightsaber, 1 );
	ch->dest_buf = str_dup( arg );
	return;
      }
      send_to_char
	( "&RYou can't figure out how to fit the parts together.\r\n", ch );
      learn_from_failure( ch, gsn_lightsaber_crafting );
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
      send_to_char( "&RYou are interupted and fail to finish your work.\r\n",
	  ch );
      return;
  }

  ch->substate = SUB_NONE;

  level = character_skill_level( ch, gsn_lightsaber_crafting );

  if( ( pObjIndex = get_obj_index( OBJ_VNUM_PROTO_LIGHTSABER ) ) == NULL )
  {
    send_to_char
      ( "&RThe item you are trying to create is missing from the database.\r\nPlease inform the administration of this error.\r\n",
	ch );
    return;
  }

  checktool = FALSE;
  checkdura = FALSE;
  checkbatt = FALSE;
  checkoven = FALSE;
  checkcond = FALSE;
  checkcirc = FALSE;
  checklens = FALSE;
  checkgems = FALSE;
  checkmirr = FALSE;
  gems = 0;
  charge = 0;
  gemtype = 0;

  for( obj = ch->last_carrying; obj; obj = obj->prev_content )
  {
    if( obj->item_type == ITEM_TOOLKIT )
      checktool = TRUE;
    if( obj->item_type == ITEM_OVEN )
      checkoven = TRUE;
    if( ( obj->item_type == ITEM_PLASTIC || obj->item_type == ITEM_METAL )
	&& checkdura == FALSE )
    {
      checkdura = TRUE;
      separate_obj( obj );
      obj_from_char( obj );
      extract_obj( obj );
    }
    if( obj->item_type == ITEM_METAL && checkdura == FALSE )
    {
      checkdura = TRUE;
      separate_obj( obj );
      obj_from_char( obj );
      extract_obj( obj );
    }
    if( obj->item_type == ITEM_BATTERY && checkbatt == FALSE )
    {
      charge = UMIN( obj->value[1], 10 );
      separate_obj( obj );
      obj_from_char( obj );
      extract_obj( obj );
      checkbatt = TRUE;
    }
    if( obj->item_type == ITEM_SUPERCONDUCTOR && checkcond == FALSE )
    {
      separate_obj( obj );
      obj_from_char( obj );
      extract_obj( obj );
      checkcond = TRUE;
    }
    if( obj->item_type == ITEM_CIRCUIT && checkcirc == FALSE )
    {
      separate_obj( obj );
      obj_from_char( obj );
      extract_obj( obj );
      checkcirc = TRUE;
    }
    if( obj->item_type == ITEM_LENS && checklens == FALSE )
    {
      separate_obj( obj );
      obj_from_char( obj );
      extract_obj( obj );
      checklens = TRUE;
    }
    if( obj->item_type == ITEM_MIRROR && checkmirr == FALSE )
    {
      separate_obj( obj );
      obj_from_char( obj );
      extract_obj( obj );
      checkmirr = TRUE;
    }
    if( obj->item_type == ITEM_CRYSTAL && gems < 3 )
    {
      gems++;
      if( gemtype < obj->value[0] )
	gemtype = obj->value[0];
      separate_obj( obj );
      obj_from_char( obj );
      extract_obj( obj );
      checkgems = TRUE;
    }
  }


  if( ( !checktool ) || ( !checkdura ) || ( !checkbatt ) || ( !checkoven )
      || ( !checkmirr ) || ( !checklens ) || ( !checkgems ) || ( !checkcond )
      || ( !checkcirc ) )

  {
    send_to_char
      ( "&RYou hold up your new lightsaber and press the switch hoping for the best.\r\n",
	ch );
    send_to_char
      ( "&RInstead of a blade of light, smoke starts pouring from the handle.\r\n",
	ch );
    send_to_char
      ( "&RYou drop the hot handle and watch as it melts on away on the floor.\r\n",
	ch );
    learn_from_failure( ch, gsn_lightsaber_crafting );
    return;
  }

  obj = create_object( pObjIndex );

  obj->item_type = ITEM_WEAPON;
  SET_BIT( obj->wear_flags, ITEM_WIELD );
  SET_BIT( obj->wear_flags, ITEM_TAKE );
  obj->weight = 5;
  STRFREE( obj->name );
  obj->name = STRALLOC( "lightsaber saber" );
  strcpy( buf, arg );
  STRFREE( obj->short_descr );
  obj->short_descr = STRALLOC( buf );
  STRFREE( obj->description );
  strcat( buf, " was carelessly misplaced here." );
  obj->description = STRALLOC( buf );
  STRFREE( obj->action_desc );
  strcpy( buf, arg );
  strcat( buf, " ignites with a hum and a soft glow." );
  obj->action_desc = STRALLOC( buf );
  CREATE( paf, AFFECT_DATA, 1 );
  paf->type = -1;
  paf->duration = -1;
  paf->location = get_atype( "hitroll" );
  paf->modifier = URANGE( 0, gems, level / 30 );
  paf->bitvector = 0;
  paf->next = NULL;
  LINK( paf, obj->first_affect, obj->last_affect, next, prev );
  ++top_affect;
  CREATE( paf2, AFFECT_DATA, 1 );
  paf2->type = -1;
  paf2->duration = -1;
  paf2->location = get_atype( "parry" );
  paf2->modifier = ( level / 3 );
  paf2->bitvector = 0;
  paf2->next = NULL;
  LINK( paf2, obj->first_affect, obj->last_affect, next, prev );
  ++top_affect;
  obj->value[0] = INIT_WEAPON_CONDITION;	/* condition  */
  obj->value[1] = ( int ) ( level / 10 + gemtype * 2 );	/* min dmg  */
  obj->value[2] = ( int ) ( level / 5 + gemtype * 6 );	/* max dmg */
  obj->value[3] = WEAPON_LIGHTSABER;
  obj->value[4] = charge;
  obj->value[5] = charge;
  obj->cost = obj->value[2] * 75;

  obj = obj_to_char( obj, ch );

  send_to_char
    ( "&GYou finish your work and hold up your newly created lightsaber.&w\r\n",
      ch );
  act( AT_PLAIN, "$n finishes making $s new lightsaber.", ch, NULL, argument,
      TO_ROOM );

  learn_from_success( ch, gsn_lightsaber_crafting );
  learn_from_success( ch, gsn_lightsaber_crafting );
  learn_from_success( ch, gsn_lightsaber_crafting );
}



void do_makejewelry( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  int level = 0, chance = 0;
  bool checktool = FALSE, checkoven = FALSE, checkmetal = FALSE;
  OBJ_DATA *obj = NULL;
  OBJ_DATA *metal = NULL;
  int value = 0, cost = 0;

  argument = one_argument( argument, arg );
  strcpy( arg2, argument );


  switch ( ch->substate )
  {
    default:

      if( str_cmp( arg, "finger" )
	  && str_cmp( arg, "wrist" )
	  && str_cmp( arg, "neck" ) && str_cmp( arg, "ears" ) )
      {
	send_to_char( "&RYou cannot make jewelry for that body part.\r\n&w",
	    ch );
	send_to_char( "&RTry .\r\n&w", ch );
	return;
      }

      if( arg2[0] == '\0' )
      {
	send_to_char( "&RUsage: Makejewelry <wearloc> <name>\r\n&w", ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_TOOLKIT ) )
      {
	send_to_char( "&RYou need a toolkit.\r\n", ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_OVEN ) )
      {
	send_to_char( "&RYou need an oven.\r\n", ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_RARE_METAL ) )
      {
	send_to_char( "&RYou need some precious metal.\r\n", ch );
	return;
      }

      chance = character_skill_level( ch, gsn_makejewelry );

      if( number_percent() < chance )
      {
	send_to_char
	  ( "&GYou begin the long process of creating some jewelry.\r\n",
	    ch );
	act( AT_PLAIN,
	    "$n takes $s toolkit and some metal and begins to work.", ch,
	    NULL, argument, TO_ROOM );
	add_timer( ch, TIMER_DO_FUN, 10, do_makejewelry, 1 );
	ch->dest_buf = str_dup( arg );
	ch->dest_buf_2 = str_dup( arg2 );
	return;
      }
      send_to_char( "&RYou can't figure out what to do.\r\n", ch );
      learn_from_failure( ch, gsn_makejewelry );
      return;

    case 1:
      if( !ch->dest_buf )
	return;
      if( !ch->dest_buf_2 )
	return;
      strcpy( arg, ( const char * ) ch->dest_buf );
      DISPOSE( ch->dest_buf );
      strcpy( arg2, ( const char * ) ch->dest_buf_2 );
      DISPOSE( ch->dest_buf_2 );
      break;

    case SUB_TIMER_DO_ABORT:
      DISPOSE( ch->dest_buf );
      DISPOSE( ch->dest_buf_2 );
      ch->substate = SUB_NONE;
      send_to_char( "&RYou are interupted and fail to finish your work.\r\n",
	  ch );
      return;
  }

  ch->substate = SUB_NONE;

  level = character_skill_level( ch, gsn_makejewelry );

  checkmetal = FALSE;
  checkoven = FALSE;
  checktool = FALSE;
  value = 0;
  cost = 0;

  for( obj = ch->last_carrying; obj; obj = obj->prev_content )
  {
    if( obj->item_type == ITEM_TOOLKIT )
      checktool = TRUE;
    if( obj->item_type == ITEM_OVEN )
      checkoven = TRUE;
    if( obj->item_type == ITEM_RARE_METAL && checkmetal == FALSE )
    {
      checkmetal = TRUE;
      separate_obj( obj );
      obj_from_char( obj );
      metal = obj;
    }
    if( obj->item_type == ITEM_CRYSTAL )
    {
      cost += obj->cost;
      separate_obj( obj );
      obj_from_char( obj );
      extract_obj( obj );
    }
  }

  if( ( !checkoven ) || ( !checktool ) || ( !checkmetal ) )
  {
    send_to_char( "&RYou hold up your newly created jewelry.\r\n", ch );
    send_to_char
      ( "&RIt suddenly dawns upon you that you have created the most useless\r\n",
	ch );
    send_to_char
      ( "&Rpiece of junk you've ever seen. You quickly hide your mistake...\r\n",
	ch );
    learn_from_failure( ch, gsn_makejewelry );
    return;
  }

  obj = metal;

  obj->item_type = ITEM_ARMOR;
  SET_BIT( obj->wear_flags, ITEM_TAKE );
  value = get_wflag( arg );
  if( value < 0 || value > 31 )
    SET_BIT( obj->wear_flags, ITEM_WEAR_NECK );
  else
    SET_BIT( obj->wear_flags, 1 << value );
  STRFREE( obj->name );
  strcpy( buf, arg2 );
  obj->name = STRALLOC( buf );
  strcpy( buf, arg2 );
  STRFREE( obj->short_descr );
  obj->short_descr = STRALLOC( buf );
  STRFREE( obj->description );
  strcat( buf, " was dropped here." );
  obj->description = STRALLOC( buf );
  obj->value[0] = obj->value[1];
  obj->cost *= 10;
  obj->cost += cost;

  obj = obj_to_char( obj, ch );

  send_to_char
    ( "&GYou finish your work and hold up your newly created jewelry.&w\r\n",
      ch );
  act( AT_PLAIN, "$n finishes sewing some new jewelry.", ch, NULL, argument,
      TO_ROOM );

  learn_from_success( ch, gsn_makejewelry );
  learn_from_success( ch, gsn_makejewelry );

}

void do_makearmor( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  int level = 0, chance = 0;
  bool checksew = FALSE, checkfab = FALSE;
  OBJ_DATA *obj = NULL;
  OBJ_DATA *material = NULL;
  int value = 0;

  argument = one_argument( argument, arg );
  strcpy( arg2, argument );


  switch ( ch->substate )
  {
    default:

      if( str_cmp( arg, "body" )
	  && str_cmp( arg, "head" )
	  && str_cmp( arg, "legs" )
	  && str_cmp( arg, "feet" )
	  && str_cmp( arg, "hands" )
	  && str_cmp( arg, "arms" )
	  && str_cmp( arg, "about" ) && str_cmp( arg, "waist" ) )
      {
	send_to_char
	  ( "&RYou cannot make clothing for that body part.\r\n&w", ch );
	send_to_char
	  ( "&RTry: body, head, legs, feet, hands, arms, about, or waist.\r\n&w",
	    ch );
	return;
      }

      if( arg2[0] == '\0' )
      {
	send_to_char( "&RUsage: Makearmor <wearloc> <name>\r\n&w", ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_FABRIC ) )
      {
	send_to_char( "&RYou need some sort of fabric or material.\r\n",
	    ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_THREAD ) )
      {
	send_to_char( "&RYou need a needle and some thread.\r\n", ch );
	return;
      }

      chance = character_skill_level( ch, gsn_makearmor );

      if( number_percent() < chance )
      {
	send_to_char
	  ( "&GYou begin the long process of creating some armor.\r\n",
	    ch );
	act( AT_PLAIN,
	    "$n takes $s sewing kit and some material and begins to work.",
	    ch, NULL, argument, TO_ROOM );
	add_timer( ch, TIMER_DO_FUN, 10, do_makearmor, 1 );
	ch->dest_buf = str_dup( arg );
	ch->dest_buf_2 = str_dup( arg2 );
	return;
      }
      send_to_char( "&RYou can't figure out what to do.\r\n", ch );
      learn_from_failure( ch, gsn_makearmor );
      return;

    case 1:
      if( !ch->dest_buf )
	return;
      if( !ch->dest_buf_2 )
	return;
      strcpy( arg, ( const char * ) ch->dest_buf );
      DISPOSE( ch->dest_buf );
      strcpy( arg2, ( const char * ) ch->dest_buf_2 );
      DISPOSE( ch->dest_buf_2 );
      break;

    case SUB_TIMER_DO_ABORT:
      DISPOSE( ch->dest_buf );
      DISPOSE( ch->dest_buf_2 );
      ch->substate = SUB_NONE;
      send_to_char( "&RYou are interupted and fail to finish your work.\r\n",
	  ch );
      return;
  }

  ch->substate = SUB_NONE;

  level = character_skill_level( ch, gsn_makearmor );

  checksew = FALSE;
  checkfab = FALSE;

  for( obj = ch->last_carrying; obj; obj = obj->prev_content )
  {
    if( obj->item_type == ITEM_THREAD )
      checksew = TRUE;
    if( obj->item_type == ITEM_FABRIC && checkfab == FALSE )
    {
      checkfab = TRUE;
      separate_obj( obj );
      obj_from_char( obj );
      material = obj;
    }
  }

  if( ( !checkfab ) || ( !checksew ) )
  {
    send_to_char( "&RYou hold up your newly created armor.\r\n", ch );
    send_to_char
      ( "&RIt suddenly dawns upon you that you have created the most useless\r\n",
	ch );
    send_to_char
      ( "&Rgarment you've ever seen. You quickly hide your mistake...\r\n",
	ch );
    learn_from_failure( ch, gsn_makearmor );
    return;
  }

  obj = material;

  obj->item_type = ITEM_ARMOR;
  SET_BIT( obj->wear_flags, ITEM_TAKE );
  value = get_wflag( arg );
  if( value < 0 || value > 31 )
    SET_BIT( obj->wear_flags, ITEM_WEAR_BODY );
  else
    SET_BIT( obj->wear_flags, 1 << value );
  STRFREE( obj->name );
  strcpy( buf, arg2 );
  obj->name = STRALLOC( buf );
  strcpy( buf, arg2 );
  STRFREE( obj->short_descr );
  obj->short_descr = STRALLOC( buf );
  STRFREE( obj->description );
  strcat( buf, " was dropped here." );
  obj->description = STRALLOC( buf );
  obj->value[0] = obj->value[1];
  obj->cost *= 10;

  obj = obj_to_char( obj, ch );

  send_to_char
    ( "&GYou finish your work and hold up your newly created garment.&w\r\n",
      ch );
  act( AT_PLAIN, "$n finishes sewing some new armor.", ch, NULL, argument,
      TO_ROOM );

  learn_from_success( ch, gsn_makearmor );
  learn_from_success( ch, gsn_makearmor );
  learn_from_success( ch, gsn_makearmor );
}


void do_makeshield( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  int chance = 0;
  bool checktool = FALSE, checkbatt = FALSE, checkcond = FALSE, checkcirc =
    FALSE, checkgems = FALSE;
  OBJ_DATA *obj = NULL;
  OBJ_INDEX_DATA *pObjIndex = NULL;
  int vnum = 0, level = 0, charge = 0, gemtype = 0;

  strcpy( arg, argument );

  switch ( ch->substate )
  {
    default:
      if( arg[0] == '\0' )
      {
	send_to_char( "&RUsage: Makeshield <name>\r\n&w", ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_TOOLKIT ) )
      {
	send_to_char( "&RYou need toolkit to make an energy shield.\r\n",
	    ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_BATTERY ) )
      {
	send_to_char
	  ( "&RYou need a power source for your energy shield.\r\n", ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_CIRCUIT ) )
      {
	send_to_char( "&RYou need a small circuit board.\r\n", ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_SUPERCONDUCTOR ) )
      {
	send_to_char
	  ( "&RYou still need a small superconductor for your energy shield.\r\n",
	    ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_CRYSTAL ) )
      {
	send_to_char( "&RYou need a small crystal.\r\n", ch );
	return;
      }

      chance = character_skill_level( ch, gsn_makeshield );
      if( number_percent() < chance )
      {
	send_to_char
	  ( "&GYou begin the long process of crafting an energy shield.\r\n",
	    ch );
	act( AT_PLAIN, "$n takes $s tools and begins to work on something.",
	    ch, NULL, argument, TO_ROOM );
	add_timer( ch, TIMER_DO_FUN, 10, do_makeshield, 1 );
	ch->dest_buf = str_dup( arg );
	return;
      }
      send_to_char
	( "&RYou can't figure out how to fit the parts together.\r\n", ch );
      learn_from_failure( ch, gsn_makeshield );
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
      send_to_char( "&RYou are interupted and fail to finish your work.\r\n",
	  ch );
      return;
  }

  ch->substate = SUB_NONE;

  level = character_skill_level( ch, gsn_makeshield );
  vnum = OBJ_VNUM_PROTO_SHIELD;

  if( ( pObjIndex = get_obj_index( vnum ) ) == NULL )
  {
    send_to_char
      ( "&RThe item you are trying to create is missing from the database.\r\nPlease inform the administration of this error.\r\n",
	ch );
    return;
  }

  checktool = FALSE;
  checkbatt = FALSE;
  checkcond = FALSE;
  checkcirc = FALSE;
  checkgems = FALSE;
  charge = 0;

  for( obj = ch->last_carrying; obj; obj = obj->prev_content )
  {
    if( obj->item_type == ITEM_TOOLKIT )
      checktool = TRUE;

    if( obj->item_type == ITEM_BATTERY && checkbatt == FALSE )
    {
      charge = UMIN( obj->value[1], 10 );
      separate_obj( obj );
      obj_from_char( obj );
      extract_obj( obj );
      checkbatt = TRUE;
    }
    if( obj->item_type == ITEM_SUPERCONDUCTOR && checkcond == FALSE )
    {
      separate_obj( obj );
      obj_from_char( obj );
      extract_obj( obj );
      checkcond = TRUE;
    }
    if( obj->item_type == ITEM_CIRCUIT && checkcirc == FALSE )
    {
      separate_obj( obj );
      obj_from_char( obj );
      extract_obj( obj );
      checkcirc = TRUE;
    }
    if( obj->item_type == ITEM_CRYSTAL && checkgems == FALSE )
    {
      gemtype = obj->value[0];
      separate_obj( obj );
      obj_from_char( obj );
      extract_obj( obj );
      checkgems = TRUE;
    }
  }

  if( ( !checktool ) || ( !checkbatt )
      || ( !checkgems ) || ( !checkcond ) || ( !checkcirc ) )

  {
    send_to_char
      ( "&RYou hold up your new energy shield and press the switch hoping for the best.\r\n",
	ch );
    send_to_char
      ( "&RInstead of a field of energy being created, smoke starts pouring from the device.\r\n",
	ch );
    send_to_char
      ( "&RYou drop the hot device and watch as it melts on away on the floor.\r\n",
	ch );
    learn_from_failure( ch, gsn_makeshield );
    return;
  }

  obj = create_object( pObjIndex );

  obj->item_type = ITEM_ARMOR;
  SET_BIT( obj->wear_flags, ITEM_WIELD );
  SET_BIT( obj->wear_flags, ITEM_WEAR_SHIELD );
  obj->weight = 2;
  STRFREE( obj->name );
  obj->name = STRALLOC( "energy shield" );
  strcpy( buf, arg );
  STRFREE( obj->short_descr );
  obj->short_descr = STRALLOC( buf );
  STRFREE( obj->description );
  strcat( buf, " was carelessly misplaced here." );
  obj->description = STRALLOC( buf );
  obj->value[0] = ( int ) ( level / 10 + gemtype * 2 );	/* condition */
  obj->value[1] = ( int ) ( level / 10 + gemtype * 2 );	/* armor */
  obj->value[4] = charge;
  obj->value[5] = charge;
  obj->cost = obj->value[2] * 100;

  obj = obj_to_char( obj, ch );

  send_to_char
    ( "&GYou finish your work and hold up your newly created energy shield.&w\r\n",
      ch );
  act( AT_PLAIN, "$n finishes making $s new energy shield.", ch, NULL,
      argument, TO_ROOM );

  learn_from_success( ch, gsn_makeshield );
  learn_from_success( ch, gsn_makeshield );
  learn_from_success( ch, gsn_makeshield );

}

void do_makecontainer( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  int level = 0, chance = 0;
  bool checksew = FALSE, checkfab = FALSE;
  OBJ_DATA *obj = NULL;
  OBJ_DATA *material = NULL;
  int value = 0;

  argument = one_argument( argument, arg );
  strcpy( arg2, argument );


  switch ( ch->substate )
  {
    default:

      if( str_cmp( arg, "body" )
	  && str_cmp( arg, "legs" )
	  && str_cmp( arg, "about" )
	  && str_cmp( arg, "waist" ) && str_cmp( arg, "hold" ) )
      {
	send_to_char
	  ( "&RYou cannot make a container for that body part.\r\n&w", ch );
	send_to_char( "&RTry body, legs, about, waist or hold.\r\n&w", ch );
	return;
      }

      if( arg2[0] == '\0' )
      {
	send_to_char( "&RUsage: Makecontainer <wearloc> <name>\r\n&w", ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_FABRIC ) )
      {
	send_to_char( "&RYou need some sort of fabric or material.\r\n",
	    ch );
	return;
      }

      if( !get_obj_type_char( ch, ITEM_THREAD ) )
      {
	send_to_char( "&RYou need a needle and some thread.\r\n", ch );
	return;
      }

      chance = character_skill_level( ch, gsn_makecontainer );

      if( number_percent() < chance )
      {
	send_to_char( "&GYou begin the long process of creating a bag.\r\n",
	    ch );
	act( AT_PLAIN,
	    "$n takes $s sewing kit and some material and begins to work.",
	    ch, NULL, argument, TO_ROOM );
	add_timer( ch, TIMER_DO_FUN, 10, do_makecontainer, 1 );
	ch->dest_buf = str_dup( arg );
	ch->dest_buf_2 = str_dup( arg2 );
	return;
      }
      send_to_char( "&RYou can't figure out what to do.\r\n", ch );
      learn_from_failure( ch, gsn_makecontainer );
      return;

    case 1:
      if( !ch->dest_buf )
	return;
      if( !ch->dest_buf_2 )
	return;
      strcpy( arg, ( const char * ) ch->dest_buf );
      DISPOSE( ch->dest_buf );
      strcpy( arg2, ( const char * ) ch->dest_buf_2 );
      DISPOSE( ch->dest_buf_2 );
      break;

    case SUB_TIMER_DO_ABORT:
      DISPOSE( ch->dest_buf );
      DISPOSE( ch->dest_buf_2 );
      ch->substate = SUB_NONE;
      send_to_char( "&RYou are interupted and fail to finish your work.\r\n",
	  ch );
      return;
  }

  ch->substate = SUB_NONE;

  level = character_skill_level( ch, gsn_makecontainer );

  checksew = FALSE;
  checkfab = FALSE;

  for( obj = ch->last_carrying; obj; obj = obj->prev_content )
  {
    if( obj->item_type == ITEM_THREAD )
      checksew = TRUE;
    if( obj->item_type == ITEM_FABRIC && checkfab == FALSE )
    {
      checkfab = TRUE;
      separate_obj( obj );
      obj_from_char( obj );
      material = obj;
    }
  }

  if( ( !checkfab ) || ( !checksew ) )
  {
    send_to_char( "&RYou hold up your newly created container.\r\n", ch );
    send_to_char
      ( "&RIt suddenly dawns upon you that you have created the most useless\r\n",
	ch );
    send_to_char
      ( "&Rcontainer you've ever seen. You quickly hide your mistake...\r\n",
	ch );
    learn_from_failure( ch, gsn_makecontainer );
    return;
  }

  obj = material;

  obj->item_type = ITEM_CONTAINER;
  SET_BIT( obj->wear_flags, ITEM_TAKE );
  value = get_wflag( arg );
  if( value < 0 || value > 31 )
    SET_BIT( obj->wear_flags, ITEM_HOLD );
  else
    SET_BIT( obj->wear_flags, 1 << value );
  STRFREE( obj->name );
  strcpy( buf, arg2 );
  obj->name = STRALLOC( buf );
  strcpy( buf, arg2 );
  STRFREE( obj->short_descr );
  obj->short_descr = STRALLOC( buf );
  STRFREE( obj->description );
  strcat( buf, " was dropped here." );
  obj->description = STRALLOC( buf );
  obj->value[0] = level;
  obj->value[1] = 0;
  obj->value[2] = 0;
  obj->value[3] = 10;
  obj->cost *= 2;

  obj = obj_to_char( obj, ch );

  send_to_char
    ( "&GYou finish your work and hold up your newly created container.&w\r\n",
      ch );
  act( AT_PLAIN, "$n finishes sewing a new container.", ch, NULL, argument,
      TO_ROOM );

  learn_from_success( ch, gsn_makecontainer );
  learn_from_success( ch, gsn_makecontainer );
  learn_from_success( ch, gsn_makecontainer );
}

void do_reinforcements( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  int chance, credits;

  if( IS_NPC( ch ) || !ch->pcdata )
    return;

  strcpy( arg, argument );

  switch ( ch->substate )
  {
    default:
      if( ch->backup_wait )
      {
	send_to_char( "&RYour reinforcements are already on the way.\r\n",
	    ch );
	return;
      }

      if( !ch->pcdata->clan )
      {
	send_to_char
	  ( "&RYou need to be a member of an organization before you can call for reinforcements.\r\n",
	    ch );
	return;
      }

      if( ch->gold < 5000 )
      {
	ch_printf( ch,
	    "&RYou dont have enough credits to send for reinforcements.\r\n" );
	return;
      }

      chance = character_skill_level( ch, gsn_reinforcements );

      if( number_percent() < chance )
      {
	send_to_char( "&GYou begin making the call for reinforcements.\r\n",
	    ch );
	act( AT_PLAIN, "$n begins issuing orders int $s comlink.", ch, NULL,
	    argument, TO_ROOM );
	add_timer( ch, TIMER_DO_FUN, 1, do_reinforcements, 1 );
	ch->dest_buf = str_dup( arg );
	return;
      }
      send_to_char( "&RYou call for reinforcements but nobody answers.\r\n",
	  ch );
      learn_from_failure( ch, gsn_reinforcements );
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
	( "&RYou are interupted before you can finish your call.\r\n", ch );
      return;
  }

  ch->substate = SUB_NONE;

  send_to_char( "&GYour reinforcements are on the way.\r\n", ch );
  credits = 5000;
  ch_printf( ch, "It cost you %d credits.\r\n", credits );
  ch->gold -= UMIN( credits, ch->gold );

  learn_from_success( ch, gsn_reinforcements );
  learn_from_success( ch, gsn_reinforcements );

  ch->backup_mob = MOB_VNUM_SOLDIER;

  ch->backup_wait = 1;
}

void do_postguard( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  int chance, credits;

  if( IS_NPC( ch ) || !ch->pcdata )
    return;

  strcpy( arg, argument );

  switch ( ch->substate )
  {
    default:
      if( ch->backup_wait )
      {
	send_to_char( "&RYou already have backup coming.\r\n", ch );
	return;
      }

      if( !ch->pcdata->clan )
      {
	send_to_char
	  ( "&RYou need to be a member of an organization before you can call for a guard.\r\n",
	    ch );
	return;
      }

      if( !ch->in_room || !ch->in_room->area ||
	  ( ch->in_room->area->planet
	    && ch->in_room->area->planet->governed_by != ch->pcdata->clan ) )
      {
	send_to_char
	  ( "&RYou cannot post guards on enemy planets. Try calling for reinforcements instead.\r\n",
	    ch );
	return;
      }

      if( ch->gold < 5000 )
      {
	ch_printf( ch, "&RYou dont have enough credits.\r\n", ch );
	return;
      }

      chance = character_skill_level( ch, gsn_postguard );

      if( number_percent() < chance )
      {
	send_to_char( "&GYou begin making the call for reinforcements.\r\n",
	    ch );
	act( AT_PLAIN, "$n begins issuing orders int $s comlink.", ch, NULL,
	    argument, TO_ROOM );
	add_timer( ch, TIMER_DO_FUN, 1, do_postguard, 1 );
	ch->dest_buf = str_dup( arg );
	return;
      }
      send_to_char( "&RYou call for a guard but nobody answers.\r\n", ch );
      learn_from_failure( ch, gsn_postguard );
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
	( "&RYou are interupted before you can finish your call.\r\n", ch );
      return;
  }

  ch->substate = SUB_NONE;

  send_to_char( "&GYour guard is on the way.\r\n", ch );

  credits = 5000;
  ch_printf( ch, "It cost you %d credits.\r\n", credits );
  ch->gold -= UMIN( credits, ch->gold );

  learn_from_success( ch, gsn_postguard );
  learn_from_success( ch, gsn_postguard );

  ch->backup_mob = MOB_VNUM_GUARD;

  ch->backup_wait = 1;

}

void add_reinforcements( CHAR_DATA * ch )
{
  MOB_INDEX_DATA *pMobIndex = NULL;
  OBJ_DATA *blaster = NULL;
  OBJ_INDEX_DATA *pObjIndex = NULL;

  if( !ch->in_room )
    return;

  if( ( pMobIndex = get_mob_index( ch->backup_mob ) ) == NULL )
    return;

  if( ch->backup_mob == MOB_VNUM_SOLDIER )
  {
    CHAR_DATA *mob[3];
    int mob_cnt = 0;

    send_to_char( "Your reinforcements have arrived.\r\n", ch );

    for( mob_cnt = 0; mob_cnt < 3; mob_cnt++ )
    {
      mob[mob_cnt] = create_mobile( pMobIndex );

      if( !mob[mob_cnt] )
	return;

      char_to_room( mob[mob_cnt], ch->in_room );
      act( AT_IMMORT, "$N has arrived.", ch, NULL, mob[mob_cnt],
	  TO_ROOM );
      mob[mob_cnt]->top_level = 50;
      mob[mob_cnt]->hit = 100;
      mob[mob_cnt]->max_hit = 100;
      mob[mob_cnt]->armor = 50;
      mob[mob_cnt]->damroll = 0;
      mob[mob_cnt]->hitroll = 10;

      if( ( pObjIndex = get_obj_index( OBJ_VNUM_BLASTER ) ) != NULL )
      {
	blaster = create_object( pObjIndex );
	obj_to_char( blaster, mob[mob_cnt] );
	equip_char( mob[mob_cnt], blaster, WEAR_WIELD );
      }

      if( mob[mob_cnt]->master )
	stop_follower( mob[mob_cnt] );

      add_follower( mob[mob_cnt], ch );
      SET_BIT( mob[mob_cnt]->affected_by, AFF_CHARM );
      do_setblaster( mob[mob_cnt], STRLIT_FULL );

      if( ch->pcdata && ch->pcdata->clan )
	mob[mob_cnt]->mob_clan = ch->pcdata->clan;
    }
  }
  else
  {
    CHAR_DATA *mob = create_mobile( pMobIndex );
    char_to_room( mob, ch->in_room );

    if( ch->pcdata && ch->pcdata->clan )
    {
      char tmpbuf[MAX_STRING_LENGTH];
      sprintf( tmpbuf, "A guard stands at attention. (%s)\r\n",
	  ch->pcdata->clan->name );
      STRFREE( mob->long_descr );
      mob->long_descr = STRALLOC( tmpbuf );
    }

    act( AT_IMMORT, "$N has arrived.", ch, NULL, mob, TO_ROOM );
    send_to_char( "Your guard has arrived.\r\n", ch );
    mob->top_level = 75;
    mob->hit = 200;
    mob->max_hit = 200;
    mob->armor = 0;
    mob->damroll = 5;
    mob->hitroll = 20;

    if( ( pObjIndex = get_obj_index( OBJ_VNUM_BLASTER ) ) != NULL )
    {
      blaster = create_object( pObjIndex );
      obj_to_char( blaster, mob );
      equip_char( mob, blaster, WEAR_WIELD );
    }

    do_setblaster( mob, STRLIT_FULL );

    if( ch->pcdata && ch->pcdata->clan )
      mob->mob_clan = ch->pcdata->clan;
  }
}

void do_torture( CHAR_DATA * ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim = NULL;
  int chance = 0, dam = 0;
  bool fail = FALSE;

  if( !IS_NPC( ch ) && character_skill_level( ch, gsn_torture ) <= 0 )
  {
    send_to_char
      ( "Your mind races as you realize you have no idea how to do that.\r\n",
	ch );
    return;
  }

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
    send_to_char( "Torture whom?\r\n", ch );
    return;
  }

  if( ( victim = get_char_room( ch, arg ) ) == NULL )
  {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }

  if( victim == ch )
  {
    send_to_char( "Are you masacistic or what...\r\n", ch );
    return;
  }

  if( !IS_AWAKE( victim ) )
  {
    send_to_char( "You need to wake them first.\r\n", ch );
    return;
  }

  if( is_safe( ch, victim ) )
    return;

  if( victim->fighting )
  {
    send_to_char( "You can't torture someone whos in combat.\r\n", ch );
    return;
  }

  ch->alignment -= 100;
  ch->alignment = URANGE( -1000, ch->alignment, 1000 );

  WAIT_STATE( ch, skill_table[gsn_torture]->beats );

  fail = FALSE;
  chance =
    ris_save( victim, character_skill_level( ch, gsn_torture ),
	RIS_PARALYSIS );
  if( chance == 1000 )
    fail = TRUE;
  else
    fail = saves_para_petri( chance, victim );

  chance = 5;
  if( !fail
      && ( IS_NPC( ch )
	|| ( number_percent() + chance ) < character_skill_level( ch,
	  gsn_torture ) ) )
  {
    learn_from_success( ch, gsn_torture );
    WAIT_STATE( ch, 2 * PULSE_VIOLENCE );
    WAIT_STATE( victim, PULSE_VIOLENCE );
    act( AT_SKILL, "$N slowly tortures you. The pain is excruciating.",
	victim, NULL, ch, TO_CHAR );
    act( AT_SKILL, "You torture $N, leaving $M screaming in pain.", ch,
	NULL, victim, TO_CHAR );
    act( AT_SKILL, "$n tortures $N, leaving $M screaming in agony!", ch,
	NULL, victim, TO_NOTVICT );

    dam = dice( character_skill_level( ch, gsn_torture ) / 10, 4 );
    dam = URANGE( 0, victim->max_hit - 10, dam );
    victim->hit -= dam;
    victim->max_hit -= dam;

    ch_printf( victim, "You lose %d permanent hit points.", dam );
    ch_printf( ch, "They lose %d permanent hit points.", dam );

  }
  else
  {
    act( AT_SKILL, "$N tries to cut off your finger!", victim, NULL, ch,
	TO_CHAR );
    act( AT_SKILL, "You mess up big time.", ch, NULL, victim, TO_CHAR );
    act( AT_SKILL, "$n tries to painfully torture $N.", ch, NULL, victim,
	TO_NOTVICT );
    WAIT_STATE( ch, 2 * PULSE_VIOLENCE );
    global_retcode = multi_hit( victim, ch, TYPE_UNDEFINED );
  }
  return;

}

void do_disguise( CHAR_DATA * ch, char *argument )
{
  int chance;

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

  chance = character_skill_level( ch, gsn_disguise );

  if( number_percent() > chance )
  {
    send_to_char( "You try to disguise yourself but fail.\r\n", ch );
    return;
  }

  if( strlen( argument ) > 50 )
    argument[50] = '\0';

  smash_tilde( argument );
  set_title( ch, argument );
  learn_from_success( ch, gsn_disguise );
  send_to_char( "Ok.\r\n", ch );
}

void do_first_aid( CHAR_DATA * ch, char *argument )
{
  OBJ_DATA *medpac;
  CHAR_DATA *victim;
  int heal;
  char buf[MAX_STRING_LENGTH];

  if( ch->position == POS_FIGHTING )
  {
    send_to_char( "You can't do that while fighting!\r\n", ch );
    return;
  }

  medpac = get_eq_char( ch, WEAR_HOLD );
  if( !medpac || medpac->item_type != ITEM_MEDPAC )
  {
    send_to_char( "You need to be holding a medpac.\r\n", ch );
    return;
  }

  if( medpac->value[0] <= 0 )
  {
    send_to_char( "Your medpac seems to be empty.\r\n", ch );
    return;
  }

  if( argument[0] == '\0' )
    victim = ch;
  else
    victim = get_char_room( ch, argument );

  if( !victim )
  {
    ch_printf( ch, "I don't see any %s here...\r\n", argument );
    return;
  }

  heal = number_range( 1, 150 );

  if( heal > character_skill_level( ch, gsn_first_aid ) * 2 )
  {
    ch_printf( ch, "You fail in your attempt at first aid.\r\n" );
    learn_from_failure( ch, gsn_first_aid );
    return;
  }

  if( victim == ch )
  {
    ch_printf( ch, "You tend to your wounds.\r\n" );
    sprintf( buf, "$n uses %s to help heal $s wounds.",
	medpac->short_descr );
    act( AT_ACTION, buf, ch, NULL, victim, TO_ROOM );
  }
  else
  {
    sprintf( buf, "You tend to $N's wounds." );
    act( AT_ACTION, buf, ch, NULL, victim, TO_CHAR );
    sprintf( buf, "$n uses %s to help heal $N's wounds.",
	medpac->short_descr );
    act( AT_ACTION, buf, ch, NULL, victim, TO_NOTVICT );
    sprintf( buf, "$n uses %s to help heal your wounds.",
	medpac->short_descr );
    act( AT_ACTION, buf, ch, NULL, victim, TO_VICT );
  }

  --medpac->value[0];
  victim->hit += URANGE( 0, heal, victim->max_hit - victim->hit );

  learn_from_success( ch, gsn_first_aid );
}

void do_snipe( CHAR_DATA * ch, char *argument )
{
  OBJ_DATA *wield = NULL;
  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  short dir = 0, dist = 0;
  short max_dist = 3;
  EXIT_DATA *pexit = NULL;
  ROOM_INDEX_DATA *was_in_room = NULL;
  ROOM_INDEX_DATA *to_room = NULL;
  CHAR_DATA *victim = NULL;
  char buf[MAX_STRING_LENGTH];
  bool pfound = FALSE;

  if( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
  {
    set_char_color( AT_MAGIC, ch );
    send_to_char( "You'll have to do that elsewhere.\r\n", ch );
    return;
  }

  if( get_eq_char( ch, WEAR_DUAL_WIELD ) != NULL )
  {
    send_to_char( "You can't do that while wielding two weapons.", ch );
    return;
  }

  wield = get_eq_char( ch, WEAR_WIELD );
  if( !wield || wield->item_type != ITEM_WEAPON
      || wield->value[3] != WEAPON_BLASTER )
  {
    send_to_char( "You don't seem to be holding a blaster", ch );
    return;
  }

  argument = one_argument( argument, arg );
  argument = one_argument( argument, arg2 );

  if( ( dir = get_door( arg ) ) == -1 || arg2[0] == '\0' )
  {
    send_to_char( "Usage: snipe <dir> <target>\r\n", ch );
    return;
  }

  if( ( pexit = get_exit( ch->in_room, dir ) ) == NULL )
  {
    send_to_char( "Are you expecting to fire through a wall!?\r\n", ch );
    return;
  }

  if( IS_SET( pexit->exit_info, EX_CLOSED ) )
  {
    send_to_char( "Are you expecting to fire through a door!?\r\n", ch );
    return;
  }

  was_in_room = ch->in_room;

  for( dist = 0; dist <= max_dist; dist++ )
  {
    if( IS_SET( pexit->exit_info, EX_CLOSED ) )
      break;

    if( !pexit->to_room )
      break;

    to_room = pexit->to_room;

    char_from_room( ch );
    char_to_room( ch, to_room );


    if( IS_NPC( ch ) && ( victim = get_char_room_mp( ch, arg2 ) ) != NULL )
    {
      pfound = TRUE;
      break;
    }
    else if( !IS_NPC( ch )
	&& ( victim = get_char_room( ch, arg2 ) ) != NULL )
    {
      pfound = TRUE;
      break;
    }


    if( ( pexit = get_exit( ch->in_room, dir ) ) == NULL )
      break;

  }

  char_from_room( ch );
  char_to_room( ch, was_in_room );

  if( !pfound )
  {
    ch_printf( ch, "You don't see that person to the %s!\r\n",
	dir_name[dir] );
    char_from_room( ch );
    char_to_room( ch, was_in_room );
    return;
  }

  if( victim == ch )
  {
    send_to_char( "Shoot yourself ... really?\r\n", ch );
    return;
  }

  if( IS_SET( victim->in_room->room_flags, ROOM_SAFE ) )
  {
    set_char_color( AT_MAGIC, ch );
    send_to_char( "You can't shoot them there.\r\n", ch );
    return;
  }

  if( is_safe( ch, victim ) )
    return;

  if( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim )
  {
    act( AT_PLAIN, "$N is your beloved master.", ch, NULL, victim,
	TO_CHAR );
    return;
  }

  if( ch->position == POS_FIGHTING )
  {
    send_to_char( "You do the best you can!\r\n", ch );
    return;
  }

  if( !IS_NPC( victim ) && IS_SET( ch->act, PLR_NICE ) )
  {
    send_to_char( "You feel too nice to do that!\r\n", ch );
    return;
  }

  switch ( dir )
  {
    case DIR_NORTH:
    case DIR_EAST:
      dir += 2;
      break;
    case DIR_SOUTH:
    case DIR_WEST:
      dir -= 2;
      break;
    case DIR_UP:
    case DIR_NORTHWEST:
      dir += 1;
      break;
    case DIR_DOWN:
    case DIR_SOUTHEAST:
      dir -= 1;
      break;
    case DIR_NORTHEAST:
      dir += 3;
      break;
    case DIR_SOUTHWEST:
      dir -= 3;
      break;
  }

  char_from_room( ch );
  char_to_room( ch, victim->in_room );

  sprintf( buf, "A blaster shot fires at you from the %s.", dir_name[dir] );
  act( AT_ACTION, buf, victim, NULL, ch, TO_CHAR );
  act( AT_ACTION, "You fire at $N.", ch, NULL, victim, TO_CHAR );
  sprintf( buf, "A blaster shot fires at $N from the %s.", dir_name[dir] );
  act( AT_ACTION, buf, ch, NULL, victim, TO_NOTVICT );

  one_hit( ch, victim, TYPE_UNDEFINED );

  if( char_died( ch ) )
    return;

  stop_fighting( ch, TRUE );


  char_from_room( ch );
  char_to_room( ch, was_in_room );

  WAIT_STATE( ch, 1 * PULSE_VIOLENCE );

  if( IS_NPC( victim ) && !char_died( victim ) )
  {
    if( IS_SET( victim->act, ACT_SENTINEL ) )
    {
      victim->was_sentinel = victim->in_room;
      REMOVE_BIT( victim->act, ACT_SENTINEL );
    }

    start_hating( victim, ch );
    start_hunting( victim, ch );

  }
}

/* syntax throw <obj> [direction] [target] */

void do_throw( CHAR_DATA * ch, char *argument )
{
  OBJ_DATA *obj;
  OBJ_DATA *tmpobj;
  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  short dir;
  EXIT_DATA *pexit;
  ROOM_INDEX_DATA *was_in_room;
  ROOM_INDEX_DATA *to_room;
  CHAR_DATA *victim;
  char buf[MAX_STRING_LENGTH];


  argument = one_argument( argument, arg );
  argument = one_argument( argument, arg2 );
  argument = one_argument( argument, arg3 );

  was_in_room = ch->in_room;

  if( arg[0] == '\0' )
  {
    send_to_char( "Usage: throw <object> [direction] [target]\r\n", ch );
    return;
  }


  obj = get_eq_char( ch, WEAR_MISSILE_WIELD );
  if( !obj || !nifty_is_name( arg, obj->name ) )
    obj = get_eq_char( ch, WEAR_HOLD );
  if( !obj || !nifty_is_name( arg, obj->name ) )
    obj = get_eq_char( ch, WEAR_WIELD );
  if( !obj || !nifty_is_name( arg, obj->name ) )
    obj = get_eq_char( ch, WEAR_DUAL_WIELD );
  if( !obj || !nifty_is_name( arg, obj->name ) )
    if( !obj || !nifty_is_name_prefix( arg, obj->name ) )
      obj = get_eq_char( ch, WEAR_HOLD );
  if( !obj || !nifty_is_name_prefix( arg, obj->name ) )
    obj = get_eq_char( ch, WEAR_WIELD );
  if( !obj || !nifty_is_name_prefix( arg, obj->name ) )
    obj = get_eq_char( ch, WEAR_DUAL_WIELD );
  if( !obj || !nifty_is_name_prefix( arg, obj->name ) )
  {
    ch_printf( ch, "You don't seem to be holding or wielding %s.\r\n",
	arg );
    return;
  }

  if( IS_OBJ_STAT( obj, ITEM_NOREMOVE ) )
  {
    act( AT_PLAIN, "You can't throw $p.", ch, obj, NULL, TO_CHAR );
    return;
  }

  if( ch->position == POS_FIGHTING )
  {
    victim = who_fighting( ch );
    if( char_died( victim ) )
      return;
    act( AT_ACTION, "You throw $p at $N.", ch, obj, victim, TO_CHAR );
    act( AT_ACTION, "$n throws $p at $N.", ch, obj, victim, TO_NOTVICT );
    act( AT_ACTION, "$n throw $p at you.", ch, obj, victim, TO_VICT );
  }
  else if( arg2[0] == '\0' )
  {
    sprintf( buf, "$n throws %s at the floor.", obj->short_descr );
    act( AT_ACTION, buf, ch, NULL, NULL, TO_ROOM );
    ch_printf( ch, "You throw %s at the floor.\r\n", obj->short_descr );

    victim = NULL;
  }
  else if( ( dir = get_door( arg2 ) ) != -1 )
  {
    if( ( pexit = get_exit( ch->in_room, dir ) ) == NULL )
    {
      send_to_char( "Are you expecting to throw it through a wall!?\r\n",
	  ch );
      return;
    }


    if( IS_SET( pexit->exit_info, EX_CLOSED ) )
    {
      send_to_char( "Are you expecting to throw it  through a door!?\r\n",
	  ch );
      return;
    }


    switch ( dir )
    {
      case DIR_NORTH:
      case DIR_EAST:
	dir += 2;
	break;
      case DIR_SOUTH:
      case DIR_WEST:
	dir -= 2;
	break;
      case DIR_UP:
      case DIR_NORTHWEST:
	dir += 1;
	break;
      case DIR_DOWN:
      case DIR_SOUTHEAST:
	dir -= 1;
	break;
      case DIR_NORTHEAST:
	dir += 3;
	break;
      case DIR_SOUTHWEST:
	dir -= 3;
	break;
    }

    to_room = pexit->to_room;


    char_from_room( ch );
    char_to_room( ch, to_room );

    victim = get_char_room( ch, arg3 );

    if( victim )
    {
      if( is_safe( ch, victim ) )
	return;

      if( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim )
      {
	act( AT_PLAIN, "$N is your beloved master.", ch, NULL, victim,
	    TO_CHAR );
	return;
      }

      if( !IS_NPC( victim ) && IS_SET( ch->act, PLR_NICE ) )
      {
	send_to_char( "You feel too nice to do that!\r\n", ch );
	return;
      }

      char_from_room( ch );
      char_to_room( ch, was_in_room );


      if( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
      {
	set_char_color( AT_MAGIC, ch );
	send_to_char( "You'll have to do that elsewhere.\r\n", ch );
	return;
      }

      to_room = pexit->to_room;


      char_from_room( ch );
      char_to_room( ch, to_room );

      sprintf( buf, "Someone throws %s at you from the %s.",
	  obj->short_descr, dir_name[dir] );
      act( AT_ACTION, buf, victim, NULL, ch, TO_CHAR );
      act( AT_ACTION, "You throw %p at $N.", ch, obj, victim, TO_CHAR );
      sprintf( buf, "%s is thrown at $N from the %s.", obj->short_descr,
	  dir_name[dir] );
      act( AT_ACTION, buf, ch, NULL, victim, TO_NOTVICT );


    }
    else
    {
      ch_printf( ch, "You throw %s %s.\r\n", obj->short_descr,
	  dir_name[get_dir( arg2 )] );
      sprintf( buf, "%s is thrown from the %s.", obj->short_descr,
	  dir_name[dir] );
      act( AT_ACTION, buf, ch, NULL, NULL, TO_ROOM );

    }
  }
  else if( ( victim = get_char_room( ch, arg2 ) ) != NULL )
  {
    if( is_safe( ch, victim ) )
      return;

    if( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim )
    {
      act( AT_PLAIN, "$N is your beloved master.", ch, NULL, victim,
	  TO_CHAR );
      return;
    }

    if( !IS_NPC( victim ) && IS_SET( ch->act, PLR_NICE ) )
    {
      send_to_char( "You feel too nice to do that!\r\n", ch );
      return;
    }

  }
  else
  {
    ch_printf( ch, "They don't seem to be here!\r\n" );
    return;
  }


  if( obj == get_eq_char( ch, WEAR_WIELD )
      && ( tmpobj = get_eq_char( ch, WEAR_DUAL_WIELD ) ) != NULL )
    tmpobj->wear_loc = WEAR_WIELD;

  unequip_char( ch, obj );
  separate_obj( obj );
  obj_from_char( obj );
  obj = obj_to_room( obj, ch->in_room );

  damage_obj( obj );

  /* NOT NEEDED UNLESS REFERING TO OBJECT AGAIN 

     if( obj_extracted(obj) )
     return;
     */
  if( ch->in_room != was_in_room )
  {
    char_from_room( ch );
    char_to_room( ch, was_in_room );
  }

  if( !victim || char_died( victim ) )
    learn_from_failure( ch, gsn_throw );
  else
  {

    WAIT_STATE( ch, skill_table[gsn_throw]->beats );
    if( IS_NPC( ch )
	|| number_percent() < character_skill_level( ch, gsn_throw ) )
    {
      learn_from_success( ch, gsn_throw );
      global_retcode =
	damage( ch, victim,
	    number_range( obj->weight * 2,
	      ( obj->weight * 2 + ch->perm_str ) ),
	    TYPE_HIT );
    }
    else
    {
      learn_from_failure( ch, gsn_throw );
      global_retcode = damage( ch, victim, 0, TYPE_HIT );
    }

    if( IS_NPC( victim ) && !char_died( victim ) )
    {
      if( IS_SET( victim->act, ACT_SENTINEL ) )
      {
	victim->was_sentinel = victim->in_room;
	REMOVE_BIT( victim->act, ACT_SENTINEL );
      }

      start_hating( victim, ch );
      start_hunting( victim, ch );

    }

  }
}

void do_pickshiplock( CHAR_DATA * ch, char *argument )
{
  do_pick( ch, argument );
}

void do_hijack( CHAR_DATA * ch, char *argument )
{
  int chance;
  SHIP_DATA *ship;
  char buf[MAX_STRING_LENGTH];
  bool uhoh = FALSE;
  CHAR_DATA *guard;
  ROOM_INDEX_DATA *room;

  if( ( ship = ship_from_cockpit( ch->in_room ) ) == NULL )
  {
    send_to_char( "&RYou must be in the cockpit of a ship to do that!\r\n",
	ch );
    return;
  }

  if( ship->ship_class > SPACE_STATION )
  {
    send_to_char( "&RThis isn't a spacecraft!\r\n", ch );
    return;
  }

  if( check_pilot( ch, ship ) )
  {
    send_to_char( "&RWhat would be the point of that!\r\n", ch );
    return;
  }

  if( ship->type == MOB_SHIP && get_trust( ch ) < 102 )
  {
    send_to_char
      ( "&RThis ship isn't pilotable by mortals at this point in time...\r\n",
	ch );
    return;
  }

  if( ship->ship_class == SPACE_STATION )
  {
    send_to_char( "You can't do that here.\r\n", ch );
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

  if( ship->shipstate == SHIP_DISABLED )
  {
    send_to_char( "The ships drive is disabled .\r\n", ch );
    return;
  }

  for( room = ship->first_room; room; room = room->next_in_ship )
  {
    for( guard = room->first_person; guard; guard = guard->next_in_room )
      if( IS_NPC( guard ) && guard->pIndexData
	  && guard->pIndexData->vnum == MOB_VNUM_SHIP_GUARD
	  && guard->position > POS_SLEEPING && !guard->fighting )
      {
	start_hating( guard, ch );
	start_hunting( guard, ch );
	uhoh = TRUE;
      }
  }

  if( uhoh )
  {
    send_to_char( "Uh oh....\r\n", ch );
    return;
  }

  chance = IS_NPC( ch ) ? 0 : character_skill_level( ch, gsn_hijack );

  if( number_percent() > chance )
  {
    send_to_char( "You fail to figure out the correct launch code.\r\n",
	ch );
    return;
  }

  chance = IS_NPC( ch ) ? 0 : character_skill_level( ch, gsn_spacecraft );
  if( number_percent() < chance )
  {
    if( ship->hatchopen )
    {
      ship->hatchopen = FALSE;
      sprintf( buf, "The hatch on %s closes.", ship->name );
      echo_to_room( AT_YELLOW, get_room_index( ship->location ), buf );
      echo_to_room( AT_YELLOW, ship->entrance, "The hatch slides shut." );
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
    learn_from_success( ch, gsn_hijack );
    sprintf( buf, "%s has been hijacked!", ship->name );
    echo_to_all( AT_RED, buf, 0 );

    return;
  }

  set_char_color( AT_RED, ch );
  send_to_char( "You fail to work the controls properly!\r\n", ch );
}


void do_propeganda( CHAR_DATA * ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char arg1[MAX_INPUT_LENGTH];
  CHAR_DATA *victim = NULL;
  PLANET_DATA *planet = NULL;
  CLAN_DATA *clan = NULL;
  int percent = 0;

  if( IS_NPC( ch ) || !ch->pcdata || !ch->pcdata->clan || !ch->in_room->area
      || !ch->in_room->area->planet )
  {
    send_to_char( "What would be the point of that.\r\n", ch );
    return;
  }

  argument = one_argument( argument, arg1 );

  if( ch->mount )
  {
    send_to_char( "You can't do that while mounted.\r\n", ch );
    return;
  }

  if( arg1[0] == '\0' )
  {
    send_to_char( "Spread propeganda to who?\r\n", ch );
    return;
  }

  if( ( victim = get_char_room( ch, arg1 ) ) == NULL )
  {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }

  if( victim == ch )
  {
    send_to_char( "That's pointless.\r\n", ch );
    return;
  }

  if( !IS_SET( victim->act, ACT_CITIZEN ) )
  {
    send_to_char( "I don't think diplomacy will work on them...\r\n", ch );
    return;
  }

  if( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
  {
    set_char_color( AT_MAGIC, ch );
    send_to_char( "This isn't a good place to do that.\r\n", ch );
    return;
  }

  if( ch->position == POS_FIGHTING )
  {
    send_to_char( "Interesting combat technique.\r\n", ch );
    return;
  }

  if( victim->position == POS_FIGHTING )
  {
    send_to_char( "They're a little busy right now.\r\n", ch );
    return;
  }

  if( ch->position <= POS_SLEEPING )
  {
    send_to_char( "In your dreams or what?\r\n", ch );
    return;
  }

  if( victim->position <= POS_SLEEPING )
  {
    send_to_char( "You might want to wake them first...\r\n", ch );
    return;
  }

  clan = ch->pcdata->clan;

  planet = ch->in_room->area->planet;

  sprintf( buf, ", and the evils of %s",
      planet->governed_by ? planet->governed_by->
      name : "their current leaders" );
  ch_printf( ch, "You speak to them about the benefits of the %s%s.\r\n",
      ch->pcdata->clan->name, planet->governed_by == clan ? "" : buf );
  act( AT_ACTION, "$n speaks about his organization.\r\n", ch, NULL, victim,
      TO_VICT );
  act( AT_ACTION, "$n tells $N about their organization.\r\n", ch, NULL,
      victim, TO_NOTVICT );

  WAIT_STATE( ch, skill_table[gsn_propeganda]->beats );

  if( percent - get_curr_cha( ch ) + victim->top_level >
      character_skill_level( ch, gsn_propeganda ) )
  {
    if( planet->governed_by != clan )
    {
      sprintf( buf, "%s is a traitor!", ch->name );
      do_yell( victim, buf );
      global_retcode = multi_hit( victim, ch, TYPE_UNDEFINED );
    }

    return;
  }

  if( planet->governed_by == clan )
  {
    planet->pop_support += 1;
    send_to_char( "Popular support for your organization increases.\r\n",
	ch );
  }
  else
  {
    planet->pop_support -= ( float ) .1;
    send_to_char
      ( "Popular support for the current government decreases.\r\n", ch );
  }

  if( number_percent() == 23 )
  {
    send_to_char( "You feel a bit more charming than you used to...\r\n",
	ch );
    ch->perm_cha++;
    ch->perm_cha = UMIN( ch->perm_cha, 25 );
  }

  learn_from_success( ch, gsn_propeganda );

  if( planet->pop_support > 100 )
    planet->pop_support = 100;
  if( planet->pop_support < -100 )
    planet->pop_support = -100;

  save_planet( planet );
}

void do_quicktalk( CHAR_DATA * ch, char *argument )
{
  CHAR_DATA *rch;
  int chance;

  if( ch->position != POS_FIGHTING )
  {
    send_to_char
      ( "You can't talk your way out of a fight that hasn't started yet...\r\n",
	ch );
    return;
  }

  act( AT_ACTION, "$n attempts to stop the fight.", ch, NULL, NULL, TO_ROOM );

  chance = character_skill_level( ch, gsn_quicktalk );
  chance = UMIN( chance, 95 );

  if( number_percent() > chance )
  {
    send_to_char( "You fail to calm your attackers.\r\n", ch );
    return;
  }

  for( rch = ch->in_room->first_person; rch; rch = rch->next_in_room )
  {
    if( rch->fighting )
      stop_fighting( rch, TRUE );

    stop_hating( rch );
    stop_hunting( rch );
    stop_fearing( rch );
  }

  learn_from_success( ch, gsn_quicktalk );

  send_to_char
    ( "You successfully talk your way out of a sticky situation.\r\n", ch );
}
