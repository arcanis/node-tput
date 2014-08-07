#include <node.h>
#include <v8.h>

#include <errno.h>
#include <cstdlib>
#include <cstring>

#include <curses.h>
#include <term.h>

using namespace v8;

#define TPARM_MAX 9

#define NODE_THROW( str ) do {                          \
    ThrowException( Exception::TypeError( String::New( str ) ) ); \
    return scope.Close( Undefined( ) );                           \
} while ( 0 )

static int ti_parm_analyse( const char *str, int *piss, int piss_len )
{
    int nparm, lpop;
    char c;

    nparm = 0;
    lpop = -1;

    while ( ( c = *str++ ) != '\0' ) {

        if ( c != '%' )
            continue;

        c = *str++;

        switch ( c ) {

            case 'l':
            case 's':
                if ( lpop > 0 ) {
                    if ( lpop <= piss_len ) {
                        piss[ lpop - 1 ] = 1;
                    } else if ( piss ) {
                        errno = E2BIG;
                    }
                }
            break;

            case 'p':
                c = *str++;
                if ( c < '1' || c > '9' ) {
                    errno = EINVAL;
                    continue;
                } else {
                    lpop = c - '0';
                    if ( lpop > nparm ) {
                        nparm = lpop;
                    }
                }
            break;

            default:
                lpop = -1;
            break ;

        }
    }

    return nparm;
}

static Local< Value > compile( char const * capability, Arguments const & args ) {

    HandleScope scope;

    if ( ! capability )
        return scope.Close( Null( ) );

    int pnum, pdef[ TPARM_MAX ] = { 0 };

    int fu[ TPARM_MAX ];
    pnum = ti_parm_analyse( capability, fu, TPARM_MAX );

    char pstr[ pnum ][ 50 ];
    int  pint[ pnum ];

    for ( int t = 0; t < pnum; ++ t ) {
        if ( pdef[ t ] /*string*/ ) {
            args[ t + 1 ]->ToString( )->WriteAscii( pstr[ t ], 0, sizeof( pstr[ t ] - 1 ) );
        } else /*number*/ {
            pint[ t ] = args[ t + 1 ]->ToUint32( )->Value( );
        }
    }

    #define P( N ) ( N < pnum ? ( pdef[ N ] ? ( long )( & pstr[ N ] ) : pint[ N ] ) : 0 )
    char * sequence = tiparm( capability, P( 0 ), P( 1 ), P( 2 ), P( 3 ), P( 4 ), P( 5 ), P( 6 ), P( 7 ) );

    return scope.Close( String::New( sequence ) );

}

Handle< Value > wrapper( Arguments const & args ) {

    HandleScope scope;

    if ( args.Length( ) < 1 )
        NODE_THROW( "missing argument" );

    char label[ 50 ];

    args[ 0 ]->ToString( )->WriteAscii( label, 0, sizeof( label ) - 1 );

    char * capability = tigetstr( label );
    if ( capability != reinterpret_cast< char * >( -1 ) )
        return scope.Close( compile( capability, args ) );

    int value = tigetnum( label );
    if ( value != -1 && value != -2 )
        return scope.Close( Number::New( value ) );

    NODE_THROW( "invalid capability label" );

}

void init( Handle< Object > exports, Handle< Object > module ) {

    int setuptermError;

    setupterm( 0, 0, &setuptermError );

    module->Set( String::NewSymbol( "exports" ), FunctionTemplate::New( wrapper )->GetFunction( ) );

}

NODE_MODULE( tput, init );
