/**
 * @file CFactory.cc
 * @author lishujie
 * @version 1.0
 * @brief common factory template
 * @detail common  factory template
 */
#ifndef _C_FACTORY_CC_
#define _C_FACTORY_CC_ 1


#include <iostream>
#include <vector>
using namespace std;


template<typename T>
class CFactory
{
public:
    class IProductor
    {
    public:
        virtual int getId() = 0;
        virtual const char *getName() = 0;
        virtual T *create() = 0;
    };

private:
    vector<IProductor *> mProductorTable;

    CFactory() {};

public:
    static CFactory &getInstance()
    {
        static CFactory inst;
        return inst;
    };

    void regProductor( IProductor *productor )
    {
        mProductorTable.push_back( productor );
    };

    T *createById( int id )
    {
        for( unsigned int i = 0; i < mProductorTable.size(); i++ )
        {
            if( id == mProductorTable[i]->getId() )
            {
                return mProductorTable[i]->create();
            }
        }

        return NULL;
    };

    T *createByName( string name )
    {
        for( unsigned int i = 0; i < mProductorTable.size(); i++ )
        {
            if( name == mProductorTable[i]->getName() )
            {
                return mProductorTable[i]->create();
            }
        }

        return NULL;
    };
};

#endif //_C_FACTORY_CC_
