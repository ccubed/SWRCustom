#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "mud.h"
#include "os.h"
#include "telnet.h"

#ifndef _WIN32
const char go_ahead_str[] = { IAC, GA, '\0' };
#endif

/*  from act_info?  */
void show_condition( CHAR_DATA * ch, CHAR_DATA * victim );
void write_ship_list( void );

/* db.c */
void allocate_string_literals( void );

/* telopt.c */
void announce_support( DESCRIPTOR_DATA *d );
void end_compress( DESCRIPTOR_DATA *d );
int translate_telopts( DESCRIPTOR_DATA *d, char *src, int srclen, char *out );
void write_compressed( DESCRIPTOR_DATA *d );

/*
 * Global variables.
 */
FILE *out_stream = NULL;
DESCRIPTOR_DATA *first_descriptor = NULL;	/* First descriptor        */
DESCRIPTOR_DATA *last_descriptor = NULL;	/* Last descriptor      */
DESCRIPTOR_DATA *d_next = NULL;	/* Next descriptor in loop */
int num_descriptors = 0;
bool mud_down = FALSE;		/* Shutdown             */
bool wizlock = FALSE;		/* Game is wizlocked    */
time_t boot_time = 0;
HOUR_MIN_SEC set_boot_time_struct;
HOUR_MIN_SEC *set_boot_time = NULL;
struct tm *new_boot_time = NULL;
struct tm new_boot_struct;
char str_boot_time[MAX_INPUT_LENGTH];
char lastplayercmd[MAX_INPUT_LENGTH * 2];
time_t current_time = 0;	/* Time of this pulse           */
SOCKET control = 0;		/* Controlling descriptor       */
int port = 0;
SOCKET newdesc = 0;		/* New descriptor               */
fd_set in_set;			/* Set of desc's for reading    */
fd_set out_set;			/* Set of desc's for writing    */
fd_set exc_set;			/* Set of desc's with errors    */
int maxdesc = 0;

/*
 * OS-dependent local functions.
 */
void game_loop( void );
SOCKET init_socket( int listen_port );
void new_descriptor( SOCKET new_desc );
bool read_from_descriptor( DESCRIPTOR_DATA * d );
bool write_to_descriptor( DESCRIPTOR_DATA *d, const char *txt, int length );

/*
 * Other local functions (OS-independent).
 */
bool check_reconnect( DESCRIPTOR_DATA * d, const char *name, bool fConn );
bool check_parse_name( const char *name );
bool check_playing( DESCRIPTOR_DATA * d, const char *name, bool kick );
bool check_multi( DESCRIPTOR_DATA * d, const char *name );
void nanny( DESCRIPTOR_DATA * d, char *argument );
bool flush_buffer( DESCRIPTOR_DATA * d, bool fPrompt );
void read_from_buffer( DESCRIPTOR_DATA * d );
void stop_idling( CHAR_DATA * ch );
void free_desc( DESCRIPTOR_DATA * d );
void display_prompt( DESCRIPTOR_DATA * d );
int make_color_sequence( const char *col, char *buf, DESCRIPTOR_DATA * d );
void set_pager_input( DESCRIPTOR_DATA * d, char *argument );
bool pager_output( DESCRIPTOR_DATA * d );
void free_memory( void );

static void execute_on_exit( void )
{
  free_memory();
  DISPOSE( sysdata.mccp_buf );

#ifdef SWR2_USE_DLSYM
  dlclose( sysdata.dl_handle );
#endif

  os_cleanup();
}

int main( int argc, char **argv )
{
  struct timeval now_time;
  bool fCopyOver = FALSE;
  const char *filename = NULL;
#ifdef SWR2_USE_IMC
  SOCKET imcsocket = INVALID_SOCKET;
#endif

  allocate_string_literals();
  os_setup();
  CREATE( sysdata.mccp_buf, unsigned char, COMPRESS_BUF_SIZE );

  atexit( execute_on_exit );
#ifdef SWR2_USE_DLSYM
  sysdata.dl_handle = dlopen( NULL, RTLD_LAZY );

  if( !sysdata.dl_handle )
  {
    fprintf( out_stream, "Failed opening dl handle to self: %s\n", dlerror() );
    exit( 1 );
  }
#endif

  num_descriptors = 0;
  first_descriptor = NULL;
  last_descriptor = NULL;
  sysdata.NO_NAME_RESOLVING = TRUE;
  sysdata.WAIT_FOR_AUTH = TRUE;
  sysdata.exe_filename = NULL;

  /*
   * Init time.
   */
  gettimeofday( &now_time, NULL );
  current_time = ( time_t ) now_time.tv_sec;
  /*  gettimeofday( &boot_time, NULL);   okay, so it's kludgy, sue me :) */
  boot_time = time( 0 );	/*  <-- I think this is what you wanted */
  strcpy( str_boot_time, ctime( &current_time ) );

  /*
   * Init boot time.
   */
  set_boot_time = &set_boot_time_struct;
  /*  set_boot_time->hour   = 6;
      set_boot_time->min    = 0;
      set_boot_time->sec    = 0; */
  set_boot_time->manual = 0;

  new_boot_time = update_time( localtime( &current_time ) );
  /* Copies *new_boot_time to new_boot_struct, and then points
     new_boot_time to new_boot_struct again. -- Alty */
  new_boot_struct = *new_boot_time;
  new_boot_time = &new_boot_struct;
  new_boot_time->tm_mday += 1;

  if( new_boot_time->tm_hour > 12 )
    new_boot_time->tm_mday += 1;

  new_boot_time->tm_sec = 0;
  new_boot_time->tm_min = 0;
  new_boot_time->tm_hour = 6;

  /* Update new_boot_time (due to day increment) */
  new_boot_time = update_time( new_boot_time );
  new_boot_struct = *new_boot_time;
  new_boot_time = &new_boot_struct;

  /* Set reboot time string for do_time */
  get_reboot_string();

  /*
   * Get the port number.
   */
  port = 4000;

  filename = argv[0];

  while( *filename == '.' || *filename == '/' )
    ++filename;

  sysdata.exe_filename = STRALLOC( filename );

  if( argc > 1 )
  {
    if( !is_number( argv[1] ) )
    {
      fprintf( out_stream, "Usage: %s [port #]\n", argv[0] );
      exit( 1 );
    }
    else if( ( port = atoi( argv[1] ) ) <= 1024 )
    {
      fprintf( out_stream, "Port number must be above 1024.\n" );
      exit( 1 );
    }

    if( argv[2] && argv[2][0] )
    {
      fCopyOver = TRUE;
#if defined(AMIGA) || defined(__MORPHOS__)
      control = ObtainSocket( atoi( argv[3] ), PF_INET, SOCK_STREAM, IPPROTO_TCP );
#ifdef SWR2_USE_IMC
      imcsocket = ObtainSocket( atoi( argv[4] ), PF_INET, SOCK_STREAM, IPPROTO_TCP );
#endif /* imc */
#else
      control = atoi( argv[3] );
#ifdef SWR2_USE_IMC
      imcsocket = atoi( argv[4] );
#endif /* imc */
#endif
    }
    else
    {
      fCopyOver = FALSE;
    }
  }

  /*
   * Run the game.
   */
#ifdef SWR2_USE_IMC
  log_string( "Starting IMC2" );
  imc_startup( FALSE, imcsocket, fCopyOver );
#endif

  log_string( "Booting Database" );
  boot_db( fCopyOver );
  log_string( "Initializing socket" );

  if( !fCopyOver )
  {
    control = init_socket( port );
  }

  sprintf( log_buf, "SWR 2.0 ready on port %d.", port );
  log_string( log_buf );
  game_loop();
  closesocket( control );

#ifdef SWR2_USE_IMC
  imc_shutdown( FALSE );
#endif

  /*
   * That's all, folks.
   */
  log_string( "Normal termination of game." );
  exit( 0 );			/* network_teardown automatically called */
}


SOCKET init_socket( int listen_port )
{
  struct sockaddr_in sa;
#ifdef _WIN32
  const char optval = 1;
#else
  int optval = 1;
#endif
  socklen_t optlen = sizeof( optval );
  SOCKET fd = 0;

  if( ( fd = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP ) ) == INVALID_SOCKET )
  {
    perror( "Init_socket: socket" );
    exit( 1 );
  }

  if( ( setsockopt( fd, SOL_SOCKET, SO_REUSEADDR,
	  &optval, optlen ) ) == SOCKET_ERROR )
  {
    perror( "Init_socket: SO_REUSEADDR" );
    closesocket( fd );
    exit( 1 );
  }

#if defined(SO_DONTLINGER) && !defined(SYSV)
  {
    struct linger ld;

    ld.l_onoff = 1;
    ld.l_linger = 1000;

    if( setsockopt( fd, SOL_SOCKET, SO_DONTLINGER,
	  ( void * ) &ld, sizeof( ld ) ) == SOCKET_ERROR )
    {
      perror( "Init_socket: SO_DONTLINGER" );
      closesocket( fd );
      exit( 1 );
    }
  }
#endif

  memset( &sa, '\0', sizeof( sa ) );
  sa.sin_family = AF_INET;	/* hp->h_addrtype; */
  sa.sin_port = htons( listen_port );

  if( bind( fd, ( struct sockaddr * ) &sa, sizeof( sa ) ) == SOCKET_ERROR )
  {
    perror( "Init_socket: bind" );
    closesocket( fd );
    exit( 1 );
  }

  if( listen( fd, 50 ) == SOCKET_ERROR )
  {
    perror( "Init_socket: listen" );
    closesocket( fd );
    exit( 1 );
  }

  return fd;
}

/*
 * LAG alarm!							-Thoric
 */
#if !defined(AMIGA) && !defined(__MORPHOS__) && !defined(_WIN32)
static void caught_alarm( int foo )
{
  char buf[MAX_STRING_LENGTH];
  bug( "ALARM CLOCK!" );
  strcpy( buf,
      "Alas, the hideous malevolent entity known only as 'Lag' rises once more!\r\n" );
  echo_to_all( AT_IMMORT, buf, ECHOTAR_ALL );
  if( newdesc )
  {
    FD_CLR( newdesc, &in_set );
    FD_CLR( newdesc, &out_set );
    log_string( "clearing newdesc" );
  }
  game_loop();
  closesocket( control );

  log_string( "Normal termination of game." );
  exit( 0 );
}
#endif

bool check_bad_desc( SOCKET desc )
{
  if( FD_ISSET( desc, &exc_set ) )
  {
    FD_CLR( desc, &in_set );
    FD_CLR( desc, &out_set );
    log_string( "Bad FD caught and disposed." );
    return TRUE;
  }
  return FALSE;
}


void accept_new( SOCKET ctrl )
{
  static struct timeval null_time;
  DESCRIPTOR_DATA *d = NULL;
  int result = 0;

  /*
   * Poll all active descriptors.
   */
  FD_ZERO( &in_set );
  FD_ZERO( &out_set );
  FD_ZERO( &exc_set );
  FD_SET( ctrl, &in_set );
  maxdesc = ctrl;
  newdesc = 0;

  for( d = first_descriptor; d; d = d->next )
  {
    maxdesc = UMAX( maxdesc, d->descriptor );
    FD_SET( d->descriptor, &in_set );
    FD_SET( d->descriptor, &out_set );
    FD_SET( d->descriptor, &exc_set );

    if( d == last_descriptor )
      break;
  }

#if defined(AMIGA) || defined(__MORPHOS__)
  result =
    WaitSelect( maxdesc + 1, &in_set, &out_set, &exc_set, &null_time, 0 );
#else
  result = select( maxdesc + 1, &in_set, &out_set, &exc_set, &null_time );
#endif

  if( result == SOCKET_ERROR )
  {
    perror( "accept_new: select: poll" );
    exit( 1 );
  }

  if( FD_ISSET( ctrl, &exc_set ) )
  {
    bug( "Exception raise on controlling descriptor %d", ctrl );
    FD_CLR( ctrl, &in_set );
    FD_CLR( ctrl, &out_set );
  }
  else if( FD_ISSET( ctrl, &in_set ) )
  {
    newdesc = ctrl;
    new_descriptor( newdesc );
  }
}

static void flush_logfile( void )
{
  static int num = 0;

  if( ( num % 20 ) == 0 )
    fflush( out_stream );

  ++num;
}

void game_loop()
{
  struct timeval last_time;
  char cmdline[MAX_INPUT_LENGTH];
  DESCRIPTOR_DATA *d = NULL;

#if !defined(AMIGA) && !defined(__MORPHOS__) && !defined(_WIN32)
  signal( SIGPIPE, SIG_IGN );
  signal( SIGALRM, caught_alarm );
  /* signal( SIGSEGV, SegVio ); */
#endif

  gettimeofday( &last_time, NULL );
  current_time = ( time_t ) last_time.tv_sec;

  /* Main loop */
  while( !mud_down )
  {
    accept_new( control );
    /*
     * Kick out descriptors with raised exceptions
     * or have been idle, then check for input.
     */
    for( d = first_descriptor; d; d = d_next )
    {
      if( d == d->next )
      {
	bug( "descriptor_loop: loop found & fixed" );
	d->next = NULL;
      }

      d_next = d->next;

      d->idle++;		/* make it so a descriptor can idle out */

      if( FD_ISSET( d->descriptor, &exc_set ) )
      {
	FD_CLR( d->descriptor, &in_set );
	FD_CLR( d->descriptor, &out_set );

	if( d->character
	    && ( d->connected == CON_PLAYING
	      || d->connected == CON_EDITING ) )
	{
	  save_char_obj( d->character );
	}

	d->outtop = 0;
	close_socket( d, TRUE );
	continue;
      }
      else if( ( !d->character && d->idle > 360 )	/* 2 mins */
	  || ( d->connected != CON_PLAYING && d->idle > 1200 )	/* 5 mins */
	  || d->idle > 28800 )	/* 2 hrs  */
      {
	write_to_descriptor( d, "Idle timeout... disconnecting.\r\n", 0 );
	d->outtop = 0;
	close_socket( d, TRUE );
	continue;
      }
      else
      {
	d->fcommand = FALSE;

	if( FD_ISSET( d->descriptor, &in_set ) )
	{
	  d->idle = 0;

	  if( d->character )
	    d->character->timer = 0;

	  if( !read_from_descriptor( d ) )
	  {
	    FD_CLR( d->descriptor, &out_set );

	    if( d->character
		&& ( d->connected == CON_PLAYING
		  || d->connected == CON_EDITING ) )
	    {
	      save_char_obj( d->character );
	    }

	    d->outtop = 0;
	    close_socket( d, FALSE );
	    continue;
	  }
	}

	if( d->character && d->character->wait > 0 )
	{
	  --d->character->wait;
	  continue;
	}

	read_from_buffer( d );

	if( d->incomm[0] != '\0' )
	{
	  d->fcommand = TRUE;
	  stop_idling( d->character );

	  strcpy( cmdline, d->incomm );
	  d->incomm[0] = '\0';

	  if( d->character )
	    set_cur_char( d->character );

	  if( d->pagepoint )
	    set_pager_input( d, cmdline );
	  else
	    switch ( d->connected )
	    {
	      default:
		nanny( d, cmdline );
		break;
	      case CON_PLAYING:
		interpret( d->character, cmdline );
		break;
	      case CON_EDITING:
		edit_buffer( d->character, cmdline );
		break;
	    }
	}
      }

      if( d == last_descriptor )
	break;
    }

#ifdef SWR2_USE_IMC
    imc_loop();
#endif

    /*
     * Autonomous game motion.
     */
    update_handler();

    /*
     * Output.
     */
    for( d = first_descriptor; d; d = d_next )
    {
      d_next = d->next;

      if( ( d->fcommand || d->outtop > 0 )
	  && FD_ISSET( d->descriptor, &out_set ) )
      {
	if( d->pagepoint )
	{
	  if( !pager_output( d ) )
	  {
	    if( d->character
		&& ( d->connected == CON_PLAYING
		  || d->connected == CON_EDITING ) )
	    {
	      save_char_obj( d->character );
	    }

	    d->outtop = 0;
	    close_socket( d, FALSE );
	  }
	}
	else if( !flush_buffer( d, TRUE ) )
	{
	  if( d->character
	      && ( d->connected == CON_PLAYING
		|| d->connected == CON_EDITING ) )
	  {
	    save_char_obj( d->character );
	  }

	  d->outtop = 0;
	  close_socket( d, FALSE );
	}
      }
      if( d == last_descriptor )
      {
	break;
      }
    }

    /*
     * Synchronize to a clock.
     * Sleep( last_time + 1/PULSE_PER_SECOND - now ).
     * Careful here of signed versus unsigned arithmetic.
     */
    {
      struct timeval now_time;
      long secDelta = 0;
      long usecDelta = 0;

      gettimeofday( &now_time, NULL );
      usecDelta =
	( ( int ) last_time.tv_usec ) - ( ( int ) now_time.tv_usec ) +
	1000000 / PULSE_PER_SECOND;
      secDelta = ( ( int ) last_time.tv_sec ) - ( ( int ) now_time.tv_sec );

      while( usecDelta < 0 )
      {
	usecDelta += 1000000;
	secDelta -= 1;
      }

      while( usecDelta >= 1000000 )
      {
	usecDelta -= 1000000;
	secDelta += 1;
      }

      if( secDelta > 0 || ( secDelta == 0 && usecDelta > 0 ) )
      {
	struct timeval stall_time;
#ifdef _WIN32
	fd_set dummy_set;
	FD_ZERO( &dummy_set );
	FD_SET( control, &dummy_set );
#endif
	stall_time.tv_usec = usecDelta;
	stall_time.tv_sec = secDelta;

#if defined(AMIGA) || defined(__MORPHOS__)
	if( WaitSelect( 0, 0, 0, 0, &stall_time, 0 ) == SOCKET_ERROR )
#elif defined(_WIN32)
	  if( select( 0, NULL, NULL, &dummy_set, &stall_time ) ==
	      SOCKET_ERROR )
#else
	    if( select( 0, NULL, NULL, NULL, &stall_time ) == SOCKET_ERROR )
#endif
	    {
	      perror( "game_loop: select: stall" );
	      exit( 1 );
	    }
      }
    }

    gettimeofday( &last_time, NULL );
    current_time = ( time_t ) last_time.tv_sec;

    flush_logfile();
  }
}

void init_descriptor( DESCRIPTOR_DATA * dnew, SOCKET desc )
{
  dnew->next = NULL;
  dnew->descriptor = desc;
  dnew->connected = CON_GET_NAME;
  dnew->outsize = 2000;
  dnew->idle = 0;
  dnew->newstate = 0;
  dnew->prevcolor = 0x07;
  dnew->original = NULL;
  dnew->character = NULL;
  dnew->terminal_type = STRALLOC( "" );
  dnew->teltop = 0;
  dnew->cols = 0;
  dnew->rows = 0;
  dnew->mccp = NULL;
  CREATE( dnew->outbuf, char, dnew->outsize );
}

void new_descriptor( SOCKET new_desc )
{
  char buf[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *dnew = NULL;
  struct hostent *from = NULL;
  struct sockaddr_in sock;
  SOCKET desc = 0;
  socklen_t size = 0;

  set_alarm( 20 );
  size = sizeof( sock );
  if( check_bad_desc( new_desc ) )
  {
    set_alarm( 0 );
    return;
  }
  set_alarm( 20 );
  if( ( desc =
	accept( new_desc, ( struct sockaddr * ) &sock,
	  &size ) ) == INVALID_SOCKET )
  {
    /*perror( "New_descriptor: accept"); */
    set_alarm( 0 );
    return;
  }
  if( check_bad_desc( new_desc ) )
  {
    set_alarm( 0 );
    return;
  }
#if !defined(FNDELAY)
#define FNDELAY O_NDELAY
#endif

  set_alarm( 20 );

  if( set_nonblocking( desc ) == SOCKET_ERROR )
  {
    perror( "New_descriptor: fcntl: FNDELAY" );
    set_alarm( 0 );
    return;
  }
  if( check_bad_desc( new_desc ) )
    return;

  CREATE( dnew, DESCRIPTOR_DATA, 1 );

  init_descriptor( dnew, desc );
  dnew->port = ntohs( sock.sin_port );

#if defined(AMIGA) || defined(__MORPHOS__)
  strcpy( buf, Inet_NtoA( *( ( unsigned long * ) &sock.sin_addr ) ) );
#else
  strcpy( buf, inet_ntoa( sock.sin_addr ) );
#endif

  sprintf( log_buf, "Sock.sinaddr:  %s, port %d.", buf, dnew->port );
  log_string_plus( log_buf, LOG_COMM );

  dnew->host = STRALLOC( buf );

  from = gethostbyaddr( (char*) &sock.sin_addr, sizeof( sock.sin_addr ), AF_INET );

  if( !sysdata.NO_NAME_RESOLVING )
  {
    STRFREE( dnew->host );
    dnew->host = STRALLOC( ( char * ) ( from ? from->h_name : buf ) );
  }

  /*
   * Init descriptor data.
   */

  if( !last_descriptor && first_descriptor )
  {
    DESCRIPTOR_DATA *d = NULL;

    bug
      ( "New_descriptor: last_desc is NULL, but first_desc is not! ...fixing" );
    for( d = first_descriptor; d; d = d->next )
      if( !d->next )
	last_descriptor = d;
  }

  LINK( dnew, first_descriptor, last_descriptor, next, prev );

  /*
   * Send supported protocols.
   */
  announce_support( dnew );

  /*
   * Send the greeting.
   */
  {
    extern char *help_greeting;
    if( help_greeting[0] == '.' )
      write_to_buffer( dnew, help_greeting + 1, 0 );
    else
      write_to_buffer( dnew, help_greeting, 0 );
  }

  if( ++num_descriptors > sysdata.maxplayers )
    sysdata.maxplayers = num_descriptors;
  if( sysdata.maxplayers > sysdata.alltimemax )
  {
    if( sysdata.time_of_max )
      DISPOSE( sysdata.time_of_max );
    sprintf( buf, "%24.24s", ctime( &current_time ) );
    sysdata.time_of_max = str_dup( buf );
    sysdata.alltimemax = sysdata.maxplayers;
    sprintf( log_buf, "Broke all-time maximum player record: %d",
	sysdata.alltimemax );
    log_string_plus( log_buf, LOG_COMM );
    to_channel( log_buf, CHANNEL_MONITOR, "Monitor", 2 );
    save_sysdata();
  }
  set_alarm( 0 );
}

void free_desc( DESCRIPTOR_DATA * d )
{
  closesocket( d->descriptor );
  STRFREE( d->host );
  DISPOSE( d->outbuf );
  STRFREE( d->terminal_type );

  if( d->pagebuf )
    DISPOSE( d->pagebuf );

  DISPOSE( d );
  --num_descriptors;
}

void close_socket( DESCRIPTOR_DATA * dclose, bool force )
{
  CHAR_DATA *ch = NULL;
  DESCRIPTOR_DATA *d = NULL;
  bool DoNotUnlink = FALSE;

  /* flush outbuf */
  if( !force && dclose->outtop > 0 )
    flush_buffer( dclose, FALSE );

  /* say bye to whoever's snooping this descriptor */
  if( dclose->snoop_by )
    write_to_buffer( dclose->snoop_by,
	"Your victim has left the game.\r\n", 0 );

  /* stop snooping everyone else */
  for( d = first_descriptor; d; d = d->next )
    if( d->snoop_by == dclose )
      d->snoop_by = NULL;

  /* Check for switched people who go link-dead. -- Altrag */
  if( dclose->original )
  {
    if( ( ch = dclose->character ) != NULL )
      do_return( ch, STRLIT_EMPTY );
    else
    {
      bug( "Close_socket: dclose->original without character %s",
	  ( dclose->original->name ? dclose->original->
	    name : "unknown" ) );
      dclose->character = dclose->original;
      dclose->original = NULL;
    }
  }

  ch = dclose->character;

  /* sanity check :( */
  if( !dclose->prev && dclose != first_descriptor )
  {
    DESCRIPTOR_DATA *dp = NULL, *dn = NULL;
    bug( "Close_socket: %s desc:%p != first_desc:%p and desc->prev = NULL!",
	ch ? ch->name : d->host, dclose, first_descriptor );

    for( d = first_descriptor; d; d = dn )
    {
      dn = d->next;
      if( d == dclose )
      {
	bug
	  ( "Close_socket: %s desc:%p found, prev should be:%p, fixing.",
	    ch ? ch->name : d->host, dclose, dp );
	dclose->prev = dp;
	break;
      }
      dp = d;
    }
    if( !dclose->prev )
    {
      bug( "Close_socket: %s desc:%p could not be found!.",
	  ch ? ch->name : dclose->host, dclose );
      DoNotUnlink = TRUE;
    }
  }
  if( !dclose->next && dclose != last_descriptor )
  {
    DESCRIPTOR_DATA *dp = NULL, *dn = NULL;
    bug( "Close_socket: %s desc:%p != last_desc:%p and desc->next = NULL!",
	ch ? ch->name : d->host, dclose, last_descriptor );

    for( d = last_descriptor; d; d = dp )
    {
      dp = d->prev;
      if( d == dclose )
      {
	bug
	  ( "Close_socket: %s desc:%p found, next should be:%p, fixing.",
	    ch ? ch->name : d->host, dclose, dn );
	dclose->next = dn;
	break;
      }
      dn = d;
    }
    if( !dclose->next )
    {
      bug( "Close_socket: %s desc:%p could not be found!.",
	  ch ? ch->name : dclose->host, dclose );
      DoNotUnlink = TRUE;
    }
  }

  if( dclose->character )
  {
    sprintf( log_buf, "Closing link to %s.", ch->name );
    log_string_plus( log_buf, LOG_COMM );
    if( dclose->connected == CON_PLAYING
	|| dclose->connected == CON_EDITING )
    {
      act( AT_ACTION, "$n has lost $s link.", ch, NULL, NULL, TO_ROOM );
      ch->desc = NULL;
    }
    else
    {
      /* clear descriptor pointer to get rid of bug message in log */
      dclose->character->desc = NULL;
      free_char( dclose->character );
    }
  }

  if( dclose->mccp )
    {
      end_compress(dclose);
    }

  if( !DoNotUnlink )
  {
    /* make sure loop doesn't get messed up */
    if( d_next == dclose )
      d_next = d_next->next;
    UNLINK( dclose, first_descriptor, last_descriptor, next, prev );
  }

  if( dclose->descriptor == maxdesc )
    --maxdesc;

  free_desc( dclose );
  return;
}


bool read_from_descriptor( DESCRIPTOR_DATA * d )
{
  size_t iStart = 0;

  /* Hold horses if pending command already. */
  if( d->incomm[0] != '\0' )
    return TRUE;

  /* Check for overflow. */
  iStart = strlen( d->inbuf );

  if( iStart >= sizeof( d->inbuf ) - 10 )
  {
    sprintf( log_buf, "%s input overflow!", d->host );
    log_string( log_buf );
    write_to_descriptor( d, "\r\n*** PUT A LID ON IT!!! ***\r\n", 0 );
    return FALSE;
  }

  for( ;; )
  {
    char bufin[MAX_INPUT_LENGTH];
#if defined(AMIGA) || defined(__MORPHOS__)
    ssize_t nRead = recv( d->descriptor, ( UBYTE * ) bufin,
			  sizeof( bufin ) - 10 - iStart, 0 );
#else
    ssize_t nRead = recv( d->descriptor, bufin,
			  sizeof( bufin ) - 10 - iStart, 0 );
#endif

    if( nRead == 0 )
    {
      log_string_plus( "EOF encountered on read.", LOG_COMM );
      return FALSE;
    }

    if( nRead == SOCKET_ERROR )
    {
      if( GETERROR == EWOULDBLOCK || GETERROR == EAGAIN )
      {
	break;
      }
      else
      {
	log_string_plus( strerror( GETERROR ), LOG_COMM );
	return FALSE;
      }
    }

    iStart += translate_telopts( d, bufin, nRead, d->inbuf + iStart );

    if( d->inbuf[iStart - 1] == '\n' || d->inbuf[iStart - 1] == '\r' )
      break;
  }

  d->inbuf[iStart] = '\0';
  return TRUE;
}

/*
 * Transfer one line from input buffer to input line.
 */
void read_from_buffer( DESCRIPTOR_DATA * d )
{
  int i = 0, j = 0, k = 0;

  /*
   * Hold horses if pending command already.
   */
  if( d->incomm[0] != '\0' )
    return;

  /*
   * Look for at least one new line.
   */
  for( i = 0;
      d->inbuf[i] != '\n' && d->inbuf[i] != '\r' && i < MAX_INBUF_SIZE; i++ )
  {
    if( d->inbuf[i] == '\0' )
      return;
  }

  /*
   * Canonical input processing.
   */
  for( i = 0, k = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++ )
  {
    if( k >= 254 )
    {
      write_to_descriptor( d, "Line too long.\r\n", 0 );

      /* skip the rest of the line */
      /*
	 for ( ; d->inbuf[i] != '\0' || i>= MAX_INBUF_SIZE ; i++ )
	 {
	 if ( d->inbuf[i] == '\n' || d->inbuf[i] == '\r' )
	 break;
	 }
	 */
      d->inbuf[i] = '\n';
      d->inbuf[i + 1] = '\0';
      break;
    }

    if( d->inbuf[i] == '\b' && k > 0 )
      --k;
    else if( isprint( ( int ) d->inbuf[i] ) )
      d->incomm[k++] = d->inbuf[i];
  }

  /*
   * Finish off the line.
   */
  if( k == 0 )
    d->incomm[k++] = ' ';

  d->incomm[k] = '\0';

  /*
   * Deal with bozos with #repeat 1000 ...
   */
  if( k > 1 || d->incomm[0] == '!' )
  {
    if( d->incomm[0] != '!' && strcmp( d->incomm, d->inlast ) )
    {
      d->repeat = 0;
    }
    else
    {
      if( ++d->repeat >= 20 )
      {
	/*                sprintf( log_buf, "%s input spamming!", d->host );
			  log_string( log_buf );
			  */
	write_to_descriptor( d, "\r\n*** PUT A LID ON IT!!! ***\r\n", 0 );
      }
    }
  }

  /*
   * Do '!' substitution.
   */
  if( d->incomm[0] == '!' )
    strcpy( d->incomm, d->inlast );
  else
    strcpy( d->inlast, d->incomm );

  /*
   * Shift the input buffer.
   */
  while( d->inbuf[i] == '\n' || d->inbuf[i] == '\r' )
    i++;

  for( j = 0; ( d->inbuf[j] = d->inbuf[i + j] ) != '\0'; j++ )
    ;
}



/*
 * Low level output function.
 */
bool flush_buffer( DESCRIPTOR_DATA * d, bool fPrompt )
{
  char buf[MAX_INPUT_LENGTH];
  CHAR_DATA *ch = NULL;

  ch = d->original ? d->original : d->character;
  if( ch && ch->fighting && ch->fighting->who )
    show_condition( ch, ch->fighting->who );

  /*
   * If buffer has more than 4K inside, spit out .5K at a time   -Thoric
   */
  if( !mud_down && d->outtop > 4096 )
  {
    memcpy( buf, d->outbuf, 512 );
    memmove( d->outbuf, d->outbuf + 512, d->outtop - 512 );
    d->outtop -= 512;
    if( d->snoop_by )
    {
      char snoopbuf[MAX_INPUT_LENGTH];

      buf[512] = '\0';
      if( d->character && d->character->name )
      {
	if( d->original && d->original->name )
	  sprintf( snoopbuf, "%s (%s)", d->character->name,
	      d->original->name );
	else
	  sprintf( snoopbuf, "%s", d->character->name );
	write_to_buffer( d->snoop_by, snoopbuf, 0 );
      }
      write_to_buffer( d->snoop_by, "% ", 2 );
      write_to_buffer( d->snoop_by, buf, 0 );
    }
    if( !write_to_descriptor( d, buf, 512 ) )
    {
      d->outtop = 0;
      return FALSE;
    }
    return TRUE;
  }


  /*
   * Bust a prompt.
   */
  if( fPrompt && !mud_down && d->connected == CON_PLAYING )
  {
    ch = d->original ? d->original : d->character;
    if( IS_SET( ch->act, PLR_BLANK ) )
      write_to_buffer( d, "\r\n", 2 );

    if( IS_SET( ch->act, PLR_PROMPT ) )
      display_prompt( d );
#ifndef _WIN32
    if( IS_SET( ch->act, PLR_TELNET_GA ) )
      write_to_buffer( d, go_ahead_str, 0 );
#endif
  }

  /*
   * Short-circuit if nothing to write.
   */
  if( d->outtop == 0 )
    return TRUE;

  /*
   * Snoop-o-rama.
   */
  if( d->snoop_by )
  {
    /* without check, 'force mortal quit' while snooped caused crash, -h */
    if( d->character && d->character->name )
    {
      /* Show original snooped names. -- Altrag */
      if( d->original && d->original->name )
	sprintf( buf, "%s (%s)", d->character->name, d->original->name );
      else
	sprintf( buf, "%s", d->character->name );
      write_to_buffer( d->snoop_by, buf, 0 );
    }
    write_to_buffer( d->snoop_by, "% ", 2 );
    write_to_buffer( d->snoop_by, d->outbuf, d->outtop );
  }

  /*
   * OS-dependent output.
   */
  if( !write_to_descriptor( d, d->outbuf, d->outtop ) )
  {
    d->outtop = 0;
    return FALSE;
  }
  else
  {
    d->outtop = 0;
    return TRUE;
  }
}



/*
 * Append onto an output buffer.
 */
void write_to_buffer( DESCRIPTOR_DATA * d, const char *txt, size_t length )
{
  if( !d )
  {
    bug( "Write_to_buffer: NULL descriptor" );
    return;
  }

  /*
   * Normally a bug... but can happen if loadup is used.
   */
  if( !d->outbuf )
    return;

  /*
   * Find length in case caller didn't.
   */
  if( length <= 0 )
    length = strlen( txt );

  /* Uncomment if debugging or something
     if ( length != strlen(txt) )
     {
     bug( "Write_to_buffer: length(%d) != strlen(txt)!", length );
     length = strlen(txt);
     }
     */

  /*
   * Initial \r\n if needed.
   */
  if( d->outtop == 0 && !d->fcommand )
  {
    d->outbuf[0] = '\n';
    d->outbuf[1] = '\r';
    d->outtop = 2;
  }

  /*
   * Expand the buffer as needed.
   */
  while( d->outtop + length >= d->outsize )
  {
    if( d->outsize > 32000 )
    {
      /* empty buffer */
      d->outtop = 0;
      close_socket( d, TRUE );
      bug( "Buffer overflow. Closing (%s).",
	  d->character ? d->character->name : "???" );
      return;
    }
    d->outsize *= 2;
    RECREATE( d->outbuf, char, d->outsize );
  }

  /*
   * Copy.
   */
  strncpy( d->outbuf + d->outtop, txt, length );
  d->outtop += length;
  d->outbuf[d->outtop] = '\0';
  return;
}


/*
 * Lowest level output function.
 * Write a block of text to the file descriptor.
 * If this gives errors on very long blocks (like 'ofind all'),
 *   try lowering the max block size.
 */
bool write_to_descriptor( DESCRIPTOR_DATA *d, const char *txt, int length )
{
  int iStart = 0;
  ssize_t nWrite = 0;
  int nBlock = 0;

  if( d->mccp )
    {
      write_compressed(d);
      return TRUE;
    }

  if( length <= 0 )
    length = strlen( txt );

  for( iStart = 0; iStart < length; iStart += nWrite )
  {
    nBlock = UMIN( length - iStart, 4096 );

#if defined(AMIGA) || defined(__MORPHOS__)
    if( ( nWrite = send( d->descriptor, (char*) txt + iStart, nBlock, 0 ) ) == SOCKET_ERROR )
#else
      if( ( nWrite = send( d->descriptor, txt + iStart, nBlock, 0 ) ) == SOCKET_ERROR )
#endif
      {
	perror( "Write_to_descriptor" );
	return FALSE;
      }
  }

  return TRUE;
}

/*
 * Parse a name for acceptability.
 */
bool check_parse_name( const char *name )
{
  static const char *const illegal_names =
    "all auto someone immortal self god supreme demigod dog guard cityguard cat cornholio spock hicaine hithoric death ass fuck shit piss crap quit luke darth vader skywalker han solo liea leia emporer palpatine chewie chewbacca lando anakin boba fett obiwan kenobi durga";

  /*
   * Reserved words.
   */
  if( is_name( name, illegal_names ) )
    return FALSE;

  /*
   * Length restrictions.
   */
  if( strlen( name ) < 3 )
    return FALSE;

  if( strlen( name ) > 12 )
    return FALSE;

  /*
   * Alphanumerics only.
   * Lock out IllIll twits.
   */
  {
    const char *pc = NULL;
    bool fIll = FALSE;

    fIll = TRUE;
    for( pc = name; *pc != '\0'; pc++ )
    {
      if( !isalpha( ( int ) *pc ) )
	return FALSE;
      if( LOWER( *pc ) != 'i' && LOWER( *pc ) != 'l' )
	fIll = FALSE;
    }

    if( fIll )
      return FALSE;
  }

  /*
   * Code that followed here used to prevent players from naming
   * themselves after mobs... this caused much havoc when new areas
   * would go in...
   */

  return TRUE;
}



/*
 * Look for link-dead player to reconnect.
 */
bool check_reconnect( DESCRIPTOR_DATA * d, const char *name, bool fConn )
{
  CHAR_DATA *ch = NULL;

  for( ch = first_char; ch; ch = ch->next )
  {
    if( !IS_NPC( ch )
	&& ( !fConn || !ch->desc )
	&& ch->name && !str_cmp( name, ch->name ) )
    {
      if( fConn && ch->switched )
      {
	write_to_buffer( d, "Already playing.\r\nName: ", 0 );
	d->connected = CON_GET_NAME;
	if( d->character )
	{
	  /* clear descriptor pointer to get rid of bug message in log */
	  d->character->desc = NULL;
	  free_char( d->character );
	  d->character = NULL;
	}
	return BERR;
      }
      if( fConn == FALSE )
      {
	DISPOSE( d->character->pcdata->pwd );
	d->character->pcdata->pwd = str_dup( ch->pcdata->pwd );
      }
      else
      {
	/* clear descriptor pointer to get rid of bug message in log */
	d->character->desc = NULL;
	free_char( d->character );
	d->character = ch;
	ch->desc = d;
	ch->timer = 0;
	send_to_char( "Reconnecting.\r\n", ch );
	act( AT_ACTION, "$n has reconnected.", ch, NULL, NULL,
	    TO_ROOM );
	sprintf( log_buf, "%s@%s reconnected.", ch->name, d->host );
	log_string_plus( log_buf, LOG_COMM );
	d->connected = CON_PLAYING;
      }
      return TRUE;
    }
  }

  return FALSE;
}



/*
 * Check if already playing.
 */

bool check_multi( DESCRIPTOR_DATA * d, const char *name )
{
  DESCRIPTOR_DATA *dold = NULL;

  for( dold = first_descriptor; dold; dold = dold->next )
  {
    if( dold != d
	&& ( dold->character || dold->original )
	&& str_cmp( name, dold->original
	  ? dold->original->name : dold->character->name )
	&& !str_cmp( dold->host, d->host ) )
    {
      write_to_buffer( d,
	  "Sorry multi-playing is not allowed ... have you other character quit first.\r\n",
	  0 );
      sprintf( log_buf, "%s attempting to multiplay with %s.",
	  dold->original ? dold->original->name : dold->character->
	  name, d->character->name );
      log_string_plus( log_buf, LOG_COMM );
      d->character = NULL;
      free_char( d->character );
      return TRUE;
    }
  }

  return FALSE;
}

bool check_playing( DESCRIPTOR_DATA * d, const char *name, bool kick )
{
  CHAR_DATA *ch = NULL;
  DESCRIPTOR_DATA *dold = NULL;
  int cstate = 0;

  for( dold = first_descriptor; dold; dold = dold->next )
  {
    if( dold != d
	&& ( dold->character || dold->original )
	&& !str_cmp( name, dold->original
	  ? dold->original->name : dold->character->name ) )
    {
      cstate = dold->connected;
      ch = dold->original ? dold->original : dold->character;
      if( !ch->name
	  || ( cstate != CON_PLAYING && cstate != CON_EDITING ) )
      {
	write_to_buffer( d, "Already connected - try again.\r\n", 0 );
	sprintf( log_buf, "%s already connected.", ch->name );
	log_string_plus( log_buf, LOG_COMM );
	return BERR;
      }
      if( !kick )
	return TRUE;
      write_to_buffer( d,
	  "Already playing... Kicking off old connection.\r\n",
	  0 );
      write_to_buffer( dold, "Kicking off old connection... bye!\r\n",
	  0 );
      close_socket( dold, FALSE );
      /* clear descriptor pointer to get rid of bug message in log */
      d->character->desc = NULL;
      free_char( d->character );
      d->character = ch;
      ch->desc = d;
      ch->timer = 0;

      if( ch->switched )
	do_return( ch->switched, STRLIT_EMPTY );

      ch->switched = NULL;
      send_to_char( "Reconnecting.\r\n", ch );
      act( AT_ACTION, "$n has reconnected, kicking off old link.",
	  ch, NULL, NULL, TO_ROOM );
      sprintf( log_buf, "%s@%s reconnected, kicking off old link.",
	  ch->name, d->host );
      log_string_plus( log_buf, LOG_COMM );
      d->connected = cstate;
      return TRUE;
    }
  }

  return FALSE;
}



void stop_idling( CHAR_DATA * ch )
{
  if( !ch
      || !ch->desc
      || ch->desc->connected != CON_PLAYING
      || !ch->was_in_room
      || ch->in_room != get_room_index( ROOM_VNUM_LIMBO ) )
    return;

  ch->timer = 0;
  char_from_room( ch );
  char_to_room( ch, ch->was_in_room );
  ch->was_in_room = NULL;
  act( AT_ACTION, "$n has returned from the void.", ch, NULL, NULL, TO_ROOM );
  return;
}

/*
 * Same as above, but converts &color codes to ANSI sequences..
 */
void send_to_char_color( const char *txt, const CHAR_DATA * ch )
{
  DESCRIPTOR_DATA *d = NULL;
  const char *colstr = NULL;
  const char *prevstr = txt;
  char colbuf[20];
  int ln = 0;

  if( !ch )
  {
    bug( "Send_to_char_color: NULL *ch" );
    return;
  }
  if( !txt || !ch->desc )
    return;
  d = ch->desc;
  /* Clear out old color stuff */
  /*  make_color_sequence(NULL, NULL, NULL);*/
  while( ( colstr = strpbrk( prevstr, "&^" ) ) != NULL )
  {
    if( colstr > prevstr )
      write_to_buffer( d, prevstr, ( colstr - prevstr ) );
    ln = make_color_sequence( colstr, colbuf, d );
    if( ln < 0 )
    {
      prevstr = colstr + 1;
      break;
    }
    else if( ln > 0 )
      write_to_buffer( d, colbuf, ln );
    prevstr = colstr + 2;
  }
  if( *prevstr )
    write_to_buffer( d, prevstr, 0 );
  return;
}

void write_to_pager( DESCRIPTOR_DATA * d, const char *txt, size_t length )
{
  if( length <= 0 )
    length = strlen( txt );

  if( length == 0 )
    return;

  if( !d->pagebuf )
  {
    d->pagesize = MAX_STRING_LENGTH;
    CREATE( d->pagebuf, char, d->pagesize );
  }
  if( !d->pagepoint )
  {
    d->pagepoint = d->pagebuf;
    d->pagetop = 0;
    d->pagecmd = '\0';
  }
  if( d->pagetop == 0 && !d->fcommand )
  {
    d->pagebuf[0] = '\n';
    d->pagebuf[1] = '\r';
    d->pagetop = 2;
  }
  while( d->pagetop + length >= d->pagesize )
  {
    if( d->pagesize > 32000 )
    {
      bug( "Pager overflow.  Ignoring.\r\n" );
      d->pagetop = 0;
      d->pagepoint = NULL;
      DISPOSE( d->pagebuf );
      d->pagesize = MAX_STRING_LENGTH;
      return;
    }
    d->pagesize *= 2;
    RECREATE( d->pagebuf, char, d->pagesize );
  }
  strncpy( d->pagebuf + d->pagetop, txt, length );
  d->pagetop += length;
  d->pagebuf[d->pagetop] = '\0';
  return;
}

void send_to_pager_color( const char *txt, const CHAR_DATA * ch )
{
  DESCRIPTOR_DATA *d = NULL;
  const char *colstr = NULL;
  const char *prevstr = txt;
  char colbuf[20];
  int ln = 0;

  if( !ch )
  {
    bug( "Send_to_pager_color: NULL *ch" );
    return;
  }
  if( !txt || !ch->desc )
    return;
  d = ch->desc;
  ch = d->original ? d->original : d->character;
  if( IS_NPC( ch ) || !IS_SET( ch->pcdata->flags, PCFLAG_PAGERON ) )
  {
    send_to_char_color( txt, d->character );
    return;
  }
  /* Clear out old color stuff */
  /*  make_color_sequence(NULL, NULL, NULL);*/
  while( ( colstr = strpbrk( prevstr, "&^" ) ) != NULL )
  {
    if( colstr > prevstr )
      write_to_pager( d, prevstr, ( colstr - prevstr ) );
    ln = make_color_sequence( colstr, colbuf, d );
    if( ln < 0 )
    {
      prevstr = colstr + 1;
      break;
    }
    else if( ln > 0 )
      write_to_pager( d, colbuf, ln );
    prevstr = colstr + 2;
  }
  if( *prevstr )
    write_to_pager( d, prevstr, 0 );
  return;
}

void set_char_color( short AType, CHAR_DATA * ch )
{
  char buf[16];
  CHAR_DATA *och = NULL;

  if( !ch || !ch->desc )
    return;

  och = ( ch->desc->original ? ch->desc->original : ch );
  if( !IS_NPC( och ) && IS_SET( och->act, PLR_ANSI ) )
  {
    if( AType == 7 )
      strcpy( buf, "\033[m" );
    else
      sprintf( buf, "\033[0;%d;%s%dm", ( AType & 8 ) == 8,
	  ( AType > 15 ? "5;" : "" ), ( AType & 7 ) + 30 );
    write_to_buffer( ch->desc, buf, strlen( buf ) );
  }
  return;
}

void set_pager_color( short AType, CHAR_DATA * ch )
{
  char buf[16];
  CHAR_DATA *och = NULL;

  if( !ch || !ch->desc )
    return;

  och = ( ch->desc->original ? ch->desc->original : ch );
  if( !IS_NPC( och ) && IS_SET( och->act, PLR_ANSI ) )
  {
    if( AType == 7 )
      strcpy( buf, "\033[m" );
    else
      sprintf( buf, "\033[0;%d;%s%dm", ( AType & 8 ) == 8,
	  ( AType > 15 ? "5;" : "" ), ( AType & 7 ) + 30 );
    send_to_pager( buf, ch );
    ch->desc->pagecolor = AType;
  }
  return;
}


/* source: EOD, by John Booth <???> */
void ch_printf( const CHAR_DATA * ch, const char *fmt, ... )
{
  char buf[MAX_STRING_LENGTH * 2];	/* better safe than sorry */
  va_list args;

  va_start( args, fmt );
  vsprintf( buf, fmt, args );
  va_end( args );

  send_to_char( buf, ch );
}

void pager_printf( const CHAR_DATA * ch, const char *fmt, ... )
{
  char buf[MAX_STRING_LENGTH * 2];
  va_list args;

  va_start( args, fmt );
  vsprintf( buf, fmt, args );
  va_end( args );

  send_to_pager( buf, ch );
}

char *default_prompt( CHAR_DATA * ch )
{
  static char buf[MAX_STRING_LENGTH];
  strcpy( buf, "" );
  strcat( buf, "&BYou are &C%h &Band &C%v" );
  strcat( buf, "&B >&w" );
  return buf;
}

int getcolor( char clr )
{
  static const char *colors = "xrgObpcwzRGYBPCW";
  size_t r = 0;

  for( r = 0; r < strlen( colors ); r++ )
    if( clr == colors[r] )
      return r;
  return -1;
}

void display_prompt( DESCRIPTOR_DATA * d )
{
  CHAR_DATA *ch = d->character;
  CHAR_DATA *och = ( d->original ? d->original : d->character );
  bool ansi = ( !IS_NPC( och ) && IS_SET( och->act, PLR_ANSI ) );
  const char *prompt = NULL;
  char buf[MAX_STRING_LENGTH];
  char *pbuf = buf;
  unsigned int p_stat = 0;

  if( !ch )
  {
    bug( "display_prompt: NULL ch" );
    return;
  }

  if( !IS_NPC( ch ) && ch->substate != SUB_NONE && ch->pcdata->subprompt
      && ch->pcdata->subprompt[0] != '\0' )
    prompt = ch->pcdata->subprompt;
  else if( IS_NPC( ch ) || !ch->pcdata->prompt || !*ch->pcdata->prompt )
    prompt = default_prompt( ch );
  else
    prompt = ch->pcdata->prompt;

  if( ansi )
  {
    strcpy( pbuf, "\033[m" );
    d->prevcolor = 0x07;
    pbuf += 3;
  }
  /* Clear out old color stuff */
  /*  make_color_sequence(NULL, NULL, NULL);*/
  for( ; *prompt; prompt++ )
  {
    /*
     * '&' = foreground color/intensity bit
     * '^' = background color/blink bit
     * '%' = prompt commands
     * Note: foreground changes will revert background to 0 (black)
     */
    if( *prompt != '&' && *prompt != '^' && *prompt != '%' )
    {
      *( pbuf++ ) = *prompt;
      continue;
    }
    ++prompt;
    if( !*prompt )
      break;
    if( *prompt == *( prompt - 1 ) )
    {
      *( pbuf++ ) = *prompt;
      continue;
    }
    switch ( *( prompt - 1 ) )
    {
      default:
	bug( "Display_prompt: bad command char '%c'.", *( prompt - 1 ) );
	break;
      case '&':
      case '^':
	p_stat = make_color_sequence( &prompt[-1], pbuf, d );

      if( (int) p_stat < 0 )
	  --prompt;
	else if( p_stat > 0 )
	  pbuf += p_stat;
	break;
      case '%':
	*pbuf = '\0';
	p_stat = 0x80000000;
	switch ( *prompt )
	{
	  case '%':
	    *pbuf++ = '%';
	    *pbuf = '\0';
	    break;
	  case 'a':
	    if( IS_GOOD( ch ) )
	      strcpy( pbuf, "good" );
	    else if( IS_EVIL( ch ) )
	      strcpy( pbuf, "evil" );
	    else
	      strcpy( pbuf, "neutral" );
	    break;
	  case 'H':
	  case 'h':
	    if( ch->hit >= 100 )
	      strcpy( pbuf, "healthy" );
	    else if( ch->hit > 90 )
	      strcpy( pbuf, "scratched" );
	    else if( ch->hit >= 75 )
	      strcpy( pbuf, "hurt" );
	    else if( ch->hit >= 50 )
	      strcpy( pbuf, "in pain" );
	    else if( ch->hit >= 35 )
	      strcpy( pbuf, "severely wounded" );
	    else if( ch->hit >= 20 )
	      strcpy( pbuf, "writhing in pain" );
	    else if( ch->hit >= 0 )
	      strcpy( pbuf, "barely conscious" );
	    else if( ch->hit >= -100 )
	      strcpy( pbuf, "unconscious" );
	    else
	      strcpy( pbuf, "dead" );
	    break;
	  case 'u':
	    p_stat = num_descriptors;
	    break;
	  case 'U':
		  p_stat = sysdata.alltimemax;
	    break;
	  case 'v':
	  case 'V':
	    if( ch->move > 500 )
	      strcpy( pbuf, "energetic" );
	    else if( ch->move > 100 )
	      strcpy( pbuf, "rested" );
	    else if( ch->move > 50 )
	      strcpy( pbuf, "worn out" );
	    else if( ch->move > 0 )
	      strcpy( pbuf, "exhausted" );
	    else
	      strcpy( pbuf, "too tired to move" );
	    break;
	  case 'g':
	    p_stat = ch->gold;
	    break;
	  case 'r':
	    if( IS_IMMORTAL( och ) )
	      p_stat = ch->in_room->vnum;
	    break;
	  case 'R':
	    if( IS_SET( och->act, PLR_ROOMVNUM ) )
	      sprintf( pbuf, "<#%ld> ", ch->in_room->vnum );
	    break;
	  case 'i':
	    if( ( !IS_NPC( ch ) && IS_SET( ch->act, PLR_WIZINVIS ) ) ||
		( IS_NPC( ch ) && IS_SET( ch->act, ACT_MOBINVIS ) ) )
	      sprintf( pbuf, "(Invis %d) ",
		  ( IS_NPC( ch ) ? ch->mobinvis : ch->pcdata->
		    wizinvis ) );
	    else if( IS_AFFECTED( ch, AFF_INVISIBLE ) )
	      sprintf( pbuf, "(Invis) " );
	    break;
	  case 'I':
	    p_stat =
	      ( IS_NPC( ch )
		? ( IS_SET( ch->act, ACT_MOBINVIS ) ? ch->
		  mobinvis : 0 ) : ( IS_SET( ch->act,
		      PLR_WIZINVIS ) ? ch->pcdata->
		    wizinvis : 0 ) );
	    break;
	}

	if( p_stat != 0x80000000 )
	  sprintf( pbuf, "%d", p_stat );

	pbuf += strlen( pbuf );
	break;
    }
  }
  *pbuf = '\0';
  write_to_buffer( d, buf, ( pbuf - buf ) );
  return;
}

int make_color_sequence( const char *col, char *buf, DESCRIPTOR_DATA * d )
{
  int ln = 0;
  const char *ctype = col;
  unsigned char cl = 0;
  CHAR_DATA *och = ( d->original ? d->original : d->character );
  bool ansi = ( !IS_NPC( och ) && IS_SET( och->act, PLR_ANSI ) );

  col++;

  if( !*col )
    {
      ln = -1;
    }
  else if( *ctype != '&' && *ctype != '^' )
    {
      bug( "Make_color_sequence: command '%c' not '&' or '^'.", *ctype );
      ln = -1;
    }
  else if( *col == *ctype )
    {
      buf[0] = *col;
      buf[1] = '\0';
      ln = 1;
    }
  else if( !ansi )
    {
      ln = 0;
    }
  else
    {
      cl = d->prevcolor;

      switch ( *ctype )
	{
	default:
	  bug( "Make_color_sequence: bad command char '%c'.", *ctype );
	  ln = -1;
	  break;
	case '&':
	  if( *col == '-' )
	    {
	      buf[0] = '~';
	      buf[1] = '\0';
	      ln = 1;
	      break;
	    }
	case '^':
	  {
	    int newcol = 0;

	    if( ( newcol = getcolor( *col ) ) < 0 )
	      {
		ln = 0;
		break;
	      }
	    else if( *ctype == '&' )
	      {
		cl = ( cl & 0xF0 ) | newcol;
	      }
	    else
	      {
		cl = ( cl & 0x0F ) | ( newcol << 4 );
	      }
	  }

	  if( cl == d->prevcolor )
	    {
	      ln = 0;
	      break;
	    }

	  strcpy( buf, "\033[" );

	  if( ( cl & 0x88 ) != ( d->prevcolor & 0x88 ) )
	    {
	      strcat( buf, "m\033[" );

	      if( ( cl & 0x08 ) )
		{
		  strcat( buf, "1;" );
		}

	      if( ( cl & 0x80 ) )
		{
		  strcat( buf, "5;" );
		}

	      d->prevcolor = 0x07 | ( cl & 0x88 );
	      ln = strlen( buf );
	    }
	else
	  {
	    ln = 2;
	  }

	  if( ( cl & 0x07 ) != ( d->prevcolor & 0x07 ) )
	    {
	      sprintf( buf + ln, "3%d;", cl & 0x07 );
	      ln += 3;
	    }

	  if( ( cl & 0x70 ) != ( d->prevcolor & 0x70 ) )
	    {
	      sprintf( buf + ln, "4%d;", ( cl & 0x70 ) >> 4 );
	      ln += 3;
	    }

	  if( buf[ln - 1] == ';' )
	    {
	      buf[ln - 1] = 'm';
	    }
	  else
	    {
	      buf[ln++] = 'm';
	      buf[ln] = '\0';
	    }

	  d->prevcolor = cl;
	}
    }

  if( ln <= 0 )
    {
      *buf = '\0';
    }

  return ln;
}

void set_pager_input( DESCRIPTOR_DATA * d, char *argument )
{
  while( isspace( ( int ) *argument ) )
    argument++;

  d->pagecmd = *argument;
}

bool pager_output( DESCRIPTOR_DATA * d )
{
  register char *last = NULL;
  CHAR_DATA *ch = NULL;
  int pclines = 0;
  register int lines = 0;
  bool ret = FALSE;

  if( !d || !d->pagepoint || d->pagecmd == -1 )
    return TRUE;

  ch = d->original ? d->original : d->character;
  pclines = UMAX( ch->pcdata->pagerlen, 5 ) - 1;

  switch ( LOWER( d->pagecmd ) )
  {
    default:
      lines = 0;
      break;

    case 'b':
      lines = -1 - ( pclines * 2 );
      break;

    case 'r':
      lines = -1 - pclines;
      break;

    case 'q':
      d->pagetop = 0;
      d->pagepoint = NULL;
      flush_buffer( d, TRUE );
      DISPOSE( d->pagebuf );
      d->pagesize = MAX_STRING_LENGTH;
      return TRUE;
  }

  while( lines < 0 && d->pagepoint >= d->pagebuf )
    if( *( --d->pagepoint ) == '\n' )
      ++lines;

  if( *d->pagepoint == '\n' && *( ++d->pagepoint ) == '\r' )
    ++d->pagepoint;

  if( d->pagepoint < d->pagebuf )
    d->pagepoint = d->pagebuf;

  for( lines = 0, last = d->pagepoint; lines < pclines; ++last )
    if( !*last )
      break;
    else if( *last == '\n' )
      ++lines;

  if( *last == '\r' )
    ++last;

  if( last != d->pagepoint )
  {
    if( !write_to_descriptor( d, d->pagepoint, ( last - d->pagepoint ) ) )
      return FALSE;
    d->pagepoint = last;
  }

  while( isspace( ( int ) *last ) )
    ++last;

  if( !*last )
  {
    d->pagetop = 0;
    d->pagepoint = NULL;
    flush_buffer( d, TRUE );
    DISPOSE( d->pagebuf );
    d->pagesize = MAX_STRING_LENGTH;
    return TRUE;
  }

  d->pagecmd = -1;

  if( IS_SET( ch->act, PLR_ANSI ) )
    if( write_to_descriptor( d, "\033[1;36m", 7 ) == FALSE )
      return FALSE;

  if( ( ret = write_to_descriptor( d,
	  "(C)ontinue, (R)efresh, (B)ack, (Q)uit: [C] ", 0 ) ) == FALSE )
    return FALSE;

  if( IS_SET( ch->act, PLR_ANSI ) )
  {
    char buf[32];

    if( d->pagecolor == 7 )
      strcpy( buf, "\033[m" );
    else
      sprintf( buf, "\033[0;%d;%s%dm", ( d->pagecolor & 8 ) == 8,
	  ( d->pagecolor > 15 ? "5;" : "" ),
	  ( d->pagecolor & 7 ) + 30 );
    ret = write_to_descriptor( d, buf, 0 );
  }

  return ret;
}
