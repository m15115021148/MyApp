/**
 * @file CDataObject.cc
 * @author Lele.Zhou
 * @version 1.0
 */
#ifndef _CDATA_OBJECT_CC_
#define _CDATA_OBJECT_CC_ 1
#include <iostream>

using namespace std;

class CDataObject
{
public:
    bool isAvaiable( string clientId )
    {
        return true;
    }
    virtual int handler( const char *json_req, int size_req, char *json_rsp, int size_rsp ) = 0;
};


#endif //_CDATA_OBJECT_CC_