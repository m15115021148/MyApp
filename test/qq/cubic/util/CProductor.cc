/**
 * @file CProductor.cc
 * @author lishujie
 * @version 1.0
 * @brief productor template for factory
 * @detail productor template for factory
 */
#ifndef _C_PRODUCTOR_CC_
#define _C_PRODUCTOR_CC_ 1

#include "CFactory.cc"

template<typename PRODUCT, typename SUPER>
class CProductor : public CFactory<SUPER>::IProductor
{
private:
    const int mId;
    const char *mName;
public:
    CProductor( int id, const char *name )
        : mId( id )
        , mName( name )
    {
        CFactory<SUPER>::getInstance().regProductor( this );
    };

    virtual inline int getId()
    {
        return mId;
    };
    virtual inline const char *getName()
    {
        return mName;
    };
    virtual inline SUPER *create()
    {
        return ( SUPER * )( new PRODUCT() );
    };
};

#define REGISTER_PRODUCTOR( i, super, id )\
    static CProductor<i,super> sProductor##i(id,#i);

#define REGISTER_PRODUCTOR_NAME( i, super, id, name )\
    static CProductor<i,super> sProductor##i(id,name);

#endif //_C_PRODUCTOR_CC_
