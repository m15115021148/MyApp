#ifndef __JSON_INTERFACE_CC__
#define __JSON_INTERFACE_CC__ 1
#include <string.h>
#include <iostream>
#include <fstream>
#include <assert.h>
#include "document.h"
#include "allocators.h"
#include "stringbuffer.h"
#include "writer.h"
#include "cubic_func.h"
#include "cubic_log.h"

#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "JsonInterface"

using namespace std;
using namespace rapidjson;

class JsonInterface
{
public:
    static bool parseJson( const char *json_req, Document &doc )
    {
        doc.Parse<0>( json_req );

        if ( doc.HasParseError() )
        {
            LOGD( "\nError(offset %u): %u\n", ( unsigned )doc.GetErrorOffset(), doc.GetParseError() );
            return false;
        }

        return true;
    };

    static bool getJsonDataFromFile( const char *path, string &str )
    {
        ifstream fin;
        fin.open( path );
        string str_in = "";

        while( getline( fin, str_in ) )   //save to str.
        {
            str = str + str_in + '\n';
        }

        fin.close();
        return true;
    };

    static int checkMember( Document &doc, const char *key, string &str )
    {
        int n_ret = 200;

        if( !doc.HasMember( key ) )
        {
            n_ret = 400;
            str = "Current json has no '";
            str += key;
            str += "' member";
        }

        return n_ret;
    }
    static int checkMember( Value &val, const char *key, string &str )
    {
        int n_ret = 200;

        if( !val.HasMember( key ) )
        {
            n_ret = 400;
            str = "Current json has no '";
            str += key;
            str += "' member";
        }

        return n_ret;
    }

    static int createResultJson( string &method, int result, const char *reason, char *json_rsp, unsigned int size_rsp )
    {
        Document doc;
        string json;
        int ret = 0;
        RETNIF_LOGE( reason == NULL, -1, "reason  is null" );
        RETNIF_LOGE( json_rsp == NULL, -1, "json_rsp  is null" );
        memset( json_rsp, 0, size_rsp );
        doc.SetObject();
        Document::AllocatorType &allocator = doc.GetAllocator();
        doc.AddMember( "method", StringRef( method.c_str(), method.length() ), allocator );
        doc.AddMember( "result", result, allocator );
        doc.AddMember( "reason", StringRef( reason, strlen( reason ) ), allocator );
        StringBuffer buffer;
        Writer<StringBuffer> writer( buffer );
        doc.Accept( writer );
        json = buffer.GetString();
        ret = MIN( json.length(), size_rsp - 1 );
        strncpy( json_rsp, json.c_str(), ret );
        return ret;
    };
};
#endif  //__JSON_INTERFACE_CC__
