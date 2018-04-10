/**
 * @file CForecast.cc
 * @author shujie.li
 * @version 1.0
 * @brief Cubic Forecast, use exist point to forecast unknown point
 * @detail Cubic Forecast, use exist point to forecast unknown point
 */

#ifndef _C_FORECAST_CC_
#define _C_FORECAST_CC_ 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


class CForecast
{
public:
    typedef struct Point
    {
        double key;
        double val;
    } Point;

private:
    int     m_ref_point_num;
    Point  *m_ref_points;
    double *m_ref_slope;

public:
    // alpha should be 0.0 ~ 1.0, more close to 0.0, more smooth
    CForecast( const Point *ref_pnt, int ref_num, double alpha )
        : m_ref_point_num( 0 )
        , m_ref_points( NULL )
        , m_ref_slope( NULL )
    {
        if( ref_num < 2 )
        {
            fprintf( stderr, "at least 2 points" );
            return;
        }

        // malloc memory
        m_ref_points = ( Point * )malloc( sizeof( Point ) * ref_num );

        if( m_ref_points == NULL )
        {
            perror( "can not malloc reference table" );
            return;
        }

        m_ref_slope = ( double * )malloc( sizeof( double ) * ref_num );

        if( m_ref_slope == NULL )
        {
            perror( "can not malloc slope table" );
            free( m_ref_points );
            return;
        }

        // smooth and create reference table
        m_ref_points[0] = ref_pnt[0];
        m_ref_slope[0] = 0;

        for( int i = 1; i < ref_num; i++ )
        {
            m_ref_points[i].key = ref_pnt[i].key * alpha + m_ref_points[i - 1].key * ( 1 - alpha );
            m_ref_points[i].val = ref_pnt[i].val * alpha + m_ref_points[i - 1].val * ( 1 - alpha );
            m_ref_slope[i] = ( m_ref_points[i].val - m_ref_points[i - 1].val ) / ( m_ref_points[i].key - m_ref_points[i - 1].key );
        }

        m_ref_slope[0] = m_ref_slope[1];
        m_ref_point_num = ref_num;
    };

    ~CForecast()
    {
        if( m_ref_points != NULL )
        {
            free( m_ref_points );
        }

        if( m_ref_slope != NULL )
        {
            free( m_ref_slope );
        }
    };

    double getval( double key )
    {
        if( m_ref_point_num < 2 )
        {
            return 0;
        }

        int base = 1;

        for( base = 1; base < m_ref_point_num && key > m_ref_points[base].key; base++ );

        if( base > m_ref_point_num )
        {
            base = m_ref_point_num;
        }

        // no matter the key is greater than max reference point or less than min
        // base-1 point is base
        // note: the delta can be negative
        return m_ref_points[base - 1].val + ( m_ref_slope[base - 1] * ( key - m_ref_points[base - 1].key ) );
    };

    double operator[]( double key )
    {
        return getval( key );
    };
};

#endif //_C_FORECAST_CC_
