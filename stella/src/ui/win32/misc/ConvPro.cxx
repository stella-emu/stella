#include "StdAfx.hxx"

//
// Program to convert stella.pro into quick and easy format:
//
// struct PROPSET
// {
//    struct PROPS { char* key; char* value; } props[25];
// } g_propset[] = 
// {
//    { "key", "value", "key", "value", NULL, NULL, },
// };
//
// Jeff Miller 20-Apr-2000

void GetQuotedString(
    char** ppszStart,
    char** ppszEnd
    )
{
    char* pszStart = *ppszStart;
    while ( *pszStart && *pszStart != '"' )
    {
        ++pszStart;
    }
    if ( *pszStart == NULL )
    {
        return;
    }
    ++pszStart;

    char* pszEnd = pszStart;
    while ( *pszEnd && *pszEnd != '"' )
    {
        ++pszEnd;
    }

    if ( pszEnd )
    {
        *pszEnd = '\0';
        ++pszEnd;
    }

    *ppszStart = pszStart;

    if ( ppszEnd )
    {
        *ppszEnd = pszEnd;
    }
}


bool convert(
    const char* pszIn,
    const char* pszOut
    )
{
    bool fRet = false;
    FILE* pfileIn = NULL;
    FILE* pfileOut = NULL;
    const int cchBuffer = 128;
    char pszBuffer[cchBuffer+1];
    bool fNeedNew = true;
    char* pszKey;
    char* pszValue;


    pfileIn = fopen( pszIn, "rt" );
    if ( pfileIn == NULL )
    {
        goto cleanup;
    }

    pfileOut = fopen( pszOut, "wt" );
    if ( pfileOut == NULL )
    {
        goto cleanup;
    }

    //
    // write header to output file
    //

    fprintf( pfileOut, "struct PROPSET\n{\n\tstruct PROPS { char* key; char* value; } props[25];\n} g_propset[] =\n{\n" );

    for ( ; ; )
    {
        if ( ! fgets( pszBuffer, cchBuffer, pfileIn ) )
        {
            //
            // Close up
            //

            fprintf( pfileOut, "\n};\n" );
            break;
        }

        //
        // Remove the newline
        //
        int cch = strlen( pszBuffer ) - 1;
        pszBuffer[ cch ] = '\0';

        if ( cch == 0 )
        {
            //
            // ignore blank lines
            //

            continue;
        }

        if ( cch == 2 && pszBuffer[0] == '"' && pszBuffer[1] == '"' )
        {
            //
            // go to next item in prop set (terminate with two nulls)
            //

            fprintf( pfileOut, " NULL, NULL, },\n" );
            fNeedNew = true;
            continue;
        }


        //
        // Handle comments
        //

        if ( pszBuffer[0] == ';' )
        {
            fprintf( pfileOut, "\t/* %s */\n", pszBuffer );
            continue;
        }

        if ( fNeedNew )
        {
            fprintf( pfileOut, "\t{ " );
            fNeedNew = false;
        }

        //
        // print data
        //

        pszKey = pszBuffer;
        GetQuotedString( &pszKey, &pszValue );

        GetQuotedString( &pszValue, NULL );

        fprintf( pfileOut, "\"%s\", \"%s\", ", pszKey, pszValue );
    }

    fRet = true;

cleanup:

    if ( pfileIn )
    {
        fclose( pfileIn );
    }

    if ( pfileOut )
    {
        fclose( pfileOut );
    }

    return fRet;
}

int main(int argc, char* argv[])
{
    if ( argc != 3 )
    {
        puts( "USAGE: convpro d:\\stella-1.1\\src\\emucore\\stella.pro stellapro.h" );
        return 1;
    }

    if ( ! convert( argv[1], argv[2] ) )
    {
        puts( "convert failed" );
        return 1;
    }

	return 0;
}
