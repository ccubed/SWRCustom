#include <string.h>
#include "mud.h"

static const int STARTING_SKILLEVEL = 50;

static void package_builder_architect( CHAR_DATA *ch )
{
  set_skill_level( ch, gsn_survey,       STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_landscape,    STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_construction, STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_bridge,       STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_spacecraft,   STARTING_SKILLEVEL );
  ch->pcdata->num_skills = 5;
}

static void package_builder_soldier( CHAR_DATA *ch )
{
  set_skill_level( ch, gsn_spacecraft,      STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_blasters,        STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_enhanced_damage, STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_kick,            STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_second_attack,   STARTING_SKILLEVEL );
  ch->pcdata->num_skills = 5;
}

static void package_builder_medic( CHAR_DATA *ch )
{
  set_skill_level( ch, gsn_spacecraft, STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_aid,        STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_first_aid,  STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_rescue,     STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_dodge,      STARTING_SKILLEVEL );
  ch->pcdata->num_skills = 5;
}

static void package_builder_assassin( CHAR_DATA *ch )
{
  set_skill_level( ch, gsn_spacecraft,    STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_backstab,      STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_vibro_blades,  STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_poison_weapon, STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_track,         STARTING_SKILLEVEL );
  ch->pcdata->num_skills = 5;
}

static void package_builder_pilot( CHAR_DATA *ch )
{
  set_skill_level( ch, gsn_spacecraft,      STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_shipmaintenance, STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_shipdesign,      STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_weaponsystems,   STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_spacecombat,     STARTING_SKILLEVEL );
  ch->pcdata->num_skills = 5;
}

static void package_builder_senator( CHAR_DATA *ch )
{
  set_skill_level( ch, gsn_spacecraft,     STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_survey,         STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_postguard,      STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_reinforcements, STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_propeganda,     STARTING_SKILLEVEL );
  ch->pcdata->num_skills = 5;
}

static void package_builder_spy( CHAR_DATA *ch )
{
  set_skill_level( ch, gsn_spacecraft, STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_sneak,      STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_peek,       STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_pick_lock,  STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_hide,       STARTING_SKILLEVEL );
  ch->pcdata->num_skills = 5;
}

static void package_builder_thief( CHAR_DATA *ch )
{
  set_skill_level( ch, gsn_spacecraft, STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_steal,      STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_peek,       STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_pick_lock,  STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_hide,       STARTING_SKILLEVEL );
  ch->pcdata->num_skills = 5;
}

static void package_builder_pirate( CHAR_DATA *ch )
{
  set_skill_level( ch, gsn_spacecraft,    STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_hijack,        STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_pickshiplock,  STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_weaponsystems, STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_pick_lock,     STARTING_SKILLEVEL );
  ch->pcdata->num_skills = 5;
}

static void package_builder_tailor( CHAR_DATA *ch )
{
  set_skill_level( ch, gsn_spacecraft,    STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_makearmor,     STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_makecontainer, STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_makejewelry,   STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_quicktalk,     STARTING_SKILLEVEL );
  ch->pcdata->num_skills = 5;
}

static void package_builder_weaponsmith( CHAR_DATA *ch )
{
  set_skill_level( ch, gsn_spacecraft,  STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_makeblaster, STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_makeblade,   STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_makeshield,  STARTING_SKILLEVEL );
  set_skill_level( ch, gsn_quicktalk,   STARTING_SKILLEVEL );
  ch->pcdata->num_skills = 5;
}

static SKILL_PACKAGE package_table[] = {
  { "Architect",   package_builder_architect },
  { "Assassin",    package_builder_assassin },
  { "Medic",       package_builder_medic },
  { "Pilot",       package_builder_pilot },
  { "Pirate",      package_builder_pirate },
  { "Senator",     package_builder_senator },
  { "Soldier",     package_builder_soldier },
  { "Spy",         package_builder_spy },
  { "Tailor",      package_builder_tailor },
  { "Thief",       package_builder_thief },
  { "Weaponsmith", package_builder_weaponsmith }
};

size_t skill_package_table_size( void )
{
  return sizeof( package_table ) / sizeof( *package_table );
}

SKILL_PACKAGE *get_skill_package( const char *argument )
{
  size_t n = 0;

  for( n = 0; n < skill_package_table_size(); ++n )
    {
      SKILL_PACKAGE *package = &(package_table[n]);

      if( !str_prefix( argument, package->name ) )
        {
          return package;
        }
    }

  return NULL;
}

static size_t longest_package_name( void )
{
  size_t longest = 0, n = 0;

  for( n = 0; n < skill_package_table_size(); ++n )
    {
      size_t current = strlen( package_table[n].name );

      if( current > longest )
        {
          longest = current;
        }
    }

  return longest;
}

char *generate_skillpackage_table( void )
{
  const int min_pad = 10;
  const int columns = 3;
  size_t n = 0;
  static char buf[MAX_STRING_LENGTH];
  *buf = '\0';

  for( n = 0; n < skill_package_table_size(); ++n )
    {
      size_t pad = longest_package_name() - strlen( package_table[n].name ) + min_pad;
      size_t pad_iter = 0;

      strcat( buf, package_table[n].name );

      for( pad_iter = 0; pad_iter < pad; ++pad_iter )
        {
          strcat( buf, " " );
        }

      if( ( n + 1 ) % columns == 0 )
        {
          strcat( buf, "\r\n" );
        }
    }

  strcat( buf, "\r\n" );

  return buf;
}
