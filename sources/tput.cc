#include <nan.h>

#include <errno.h>
#include <cstdlib>
#include <cstring>

#include <curses.h>
#include <term.h>

#define TPARM_MAX_PARAM_COUNT   9
#define TPARM_MAX_PARAM_LENGTH  50
#define TPARM_MAX_OUTPUT_LENGTH 50

enum ParameterType {

    PARAMETER_UNKNOWN,

    PARAMETER_STRING,
    PARAMETER_INTEGER

};

static int tiparm_analyse( char const * capability, enum ParameterType ( & parameterTypes )[ TPARM_MAX_PARAM_COUNT ] ) {

    int parameterCount = 0;

    for ( int t = 0; capability[ t ]; ++ t ) {

        if ( capability[ t ] != '%' )
            continue;

        switch ( capability[ ++t ] ) {

            case 'l':
            case 's': {
                parameterTypes[ parameterCount++ ] = PARAMETER_STRING;
            } break;

            case 'p': {
                parameterTypes[ parameterCount++ ] = PARAMETER_INTEGER;
            } break;

        }

    }

    return parameterCount;

}

static v8::Local< v8::Value > compile( char const * capability, Nan::FunctionCallbackInfo< v8::Value > const & args ) {

    if ( ! capability )
        return Nan::Null( );

    enum ParameterType parameterTypes[ TPARM_MAX_PARAM_COUNT ] = { PARAMETER_UNKNOWN };
    int parameterCount = tiparm_analyse( capability, parameterTypes );

    if ( args.Length( ) - 1 < parameterCount )
        Nan::ThrowTypeError( "not enough parameters" );

    if ( args.Length( ) - 1 > parameterCount )
        Nan::ThrowTypeError( "too many parameters" );

    int parametersAsIntegers[ TPARM_MAX_PARAM_COUNT ] = { 0 };
    char parametersAsStrings[ TPARM_MAX_PARAM_COUNT ][ TPARM_MAX_PARAM_LENGTH + 1 ] = { { 0 } };

    for ( int t = 0; t < parameterCount; ++ t ) {
        switch ( parameterTypes[ t ] ) {

            case PARAMETER_STRING: {

                v8::Local< v8::String > asString = args[ t + 1 ]->ToString( );

                if ( asString->Length( ) > TPARM_MAX_PARAM_LENGTH )
                    Nan::ThrowTypeError( "unsupported parameter length" );

                asString->WriteOneByte( reinterpret_cast< uint8_t * >( parametersAsStrings[ t ] ) );

            } break;

            case PARAMETER_INTEGER: {

                v8::Local< v8::Uint32 > asInteger = args[ t + 1 ]->ToUint32( );

                parametersAsIntegers[ t ] = asInteger->Value( );

            } break;

            default: {

                Nan::ThrowTypeError( "unsupported parameter type" );

            } break;

        }
    }

    #define P( N ) ( N < parameterCount ? ( parameterTypes[ N ] == PARAMETER_STRING ? (long) parametersAsStrings[ N ] : parameterTypes[ N ] == PARAMETER_INTEGER ? parametersAsIntegers[ N ] : 0 ) : 0 )
    char const * sequence = tiparm( capability, P( 0 ), P( 1 ), P( 2 ), P( 3 ), P( 4 ), P( 5 ), P( 6 ), P( 7 ), P( 8 ) );

    return Nan::New( sequence ).ToLocalChecked();

}

void wrapper( Nan::FunctionCallbackInfo< v8::Value > const & args ) {

    if ( args.Length( ) < 1 )
        Nan::ThrowTypeError( "missing argument" );

    v8::Local< v8::String > labelAsString = args[ 0 ]->ToString( );

    char label[ labelAsString->Length( ) + 1 ];
    args[ 0 ]->ToString( )->WriteOneByte( reinterpret_cast< uint8_t * >( &label[0] ) );

    char * capability = tigetstr( label );
    if ( capability != reinterpret_cast< char * >( -1 ) ) {
        args.GetReturnValue().Set( compile( capability, args ) );
        return;
    }

    int value = tigetnum( label );
    if ( value != -1 && value != -2 ) {
        args.GetReturnValue().Set( Nan::New( value ) );
        return;
    }

    Nan::ThrowTypeError( "invalid capability label" );

}

void init( v8::Handle< v8::Object > exports, v8::Handle< v8::Object > module ) {

    int setuptermError;

    setupterm( 0, 0, &setuptermError );

    module->Set( Nan::New( "exports" ).ToLocalChecked( ), Nan::New< v8::FunctionTemplate >( wrapper )->GetFunction( ) );

}

NODE_MODULE( tput, init );
