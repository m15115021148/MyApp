/**
 * @file: CTcpMessager.cc
 * @date: 2015-08-17
 * @auth: thirchina
 * @brif: a tcp messager impelement
 */

#include "CLock.cc"
#include "CEvent.cc"
#include "CTcpConnection.cc"

typedef enum ErrCode
{
    ERR_NO_ERROR = 0,
    ERR_MEM_ALLOC_FAIL = -1,
    ERR_CONNECTION_NOT_READY = -2,
    ERR_SEND_FAILED = -3,
    ERR_PACKAGE_TOO_LARGE = -4,
    ERR_ITEM_ALREADY_EXIST = -5,
    ERR_WAIT_RESP_FAIL = -6
} EErrCode;

class IMessageHandler
{
public:
    virtual void onBreak() = 0;
    virtual void onTcpMessage( const void  *msg_data, unsigned int  msg_data_sz ) = 0;
};

class CTcpMessager : public ITcpListener
{
private:
#define PROCESS_TIMEOUT 100

    class RspWaiter
    {
    private:
        CEvent m_wake_event;
        void *data;
        int data_lenth;

    public:
        inline CEvent::EErrCode wait( unsigned int n_ms )
        {
            return m_wake_event.wait( n_ms );
        }

        inline void wake( void *p_data, int n_data_sz )
        {
            data = p_data;
            data_lenth = n_data_sz;
            m_wake_event.set();
        }

        void getMessage( void *p_data, int *n_data_sz )
        {
            memcpy( p_data, data, data_lenth );
            *n_data_sz = data_lenth;
        };
    };

    IMessageHandler *m_message_handler;
    CTcpConnection  m_connection;
    int message_pack_size;
    CLock server_lock;
    CLock resp_wait_lock;
    map<int, RspWaiter *> resp_wait_tab;

    EErrCode registerWaiter( int n_session_id, RspWaiter *waiter )
    {
        resp_wait_lock.lock();

        if( resp_wait_tab.end() != resp_wait_tab.find( n_session_id ) )
        {
            resp_wait_lock.unlock();
            return ERR_ITEM_ALREADY_EXIST;
        }

        resp_wait_tab[n_session_id] = waiter;
        resp_wait_lock.unlock();
        return ERR_NO_ERROR;
    };

    void unregisterWaiter( int n_session_id )
    {
        resp_wait_lock.lock();

        if( resp_wait_tab.end() != resp_wait_tab.find( n_session_id ) )
        {
            resp_wait_tab.erase( n_session_id );
        }

        resp_wait_lock.unlock();
    };

public:
    typedef enum MsgType
    {
        MSG_TYPE_REQ,
        MSG_TYPE_RSP,
        MSG_TYPE_NOTICE,
    } EMsgType;

    typedef struct MsgHead
    {
        EMsgType type;
        int uniq_id;
        int package_size;
    } TMsgHead;

    CTcpMessager( IMessageHandler *message_handler, int n_socket, int n_def_pack_size )
        : m_message_handler( message_handler )
        , m_connection( this, n_socket, n_def_pack_size )
        , message_pack_size( n_def_pack_size )
    {
        srand( time( 0 ) );
    };

    ~CTcpMessager()
    {
    };

    EErrCode sendMsg( const void *msg_data,
                      int msg_data_sz,
                      EMsgType msg_type,
                      int uniq_id = 0,
                      void *resp = NULL,
                      int *resp_lenth = NULL )
    {
        if( !m_connection.isConnected() )
        {
            return ERR_CONNECTION_NOT_READY;
        }

        if( msg_data_sz > ( message_pack_size - sizeof( TMsgHead ) ) )
        {
            return ERR_PACKAGE_TOO_LARGE;
        }

        RspWaiter waiter;
        server_lock.lock();
        int send_buf_sz = sizeof( TMsgHead ) + msg_data_sz;
        TMsgHead *head = ( TMsgHead * )malloc( send_buf_sz + 4 );
        memset( head, 0, send_buf_sz + 4 );

        if( !head )
        {
            perror( "Malloc Send Data Buffer" );
            server_lock.unlock();
            return ERR_MEM_ALLOC_FAIL;
        }

        // prepare head
        if( uniq_id )
        {
            head->uniq_id = uniq_id;
        }
        else
        {
            head->uniq_id = rand();
        }

        head->package_size = msg_data_sz;
        head->type = msg_type;

        if( msg_type == MSG_TYPE_REQ )
        {
            // register to waiting list
            EErrCode result = registerWaiter( head->uniq_id, &waiter );

            if( result != ERR_NO_ERROR )
            {
                return result;
            }
        }

        // copy data
        memcpy( ( char * )head + sizeof( TMsgHead ), msg_data, msg_data_sz );
        // send
        int ret = m_connection.send( head, send_buf_sz );

        if( ret < 0 || ( send_buf_sz + 1 ) != ( unsigned int )ret )
        {
            server_lock.unlock();
            return ERR_SEND_FAILED;
        }

        // wait for response
        if( msg_type == MSG_TYPE_REQ )
        {
            CEvent::EErrCode err = waiter.wait( PROCESS_TIMEOUT );

            if( err != CEvent::ERR_NO_ERROR )
            {
                return ERR_WAIT_RESP_FAIL;
            }

            unregisterWaiter( head->uniq_id );
            waiter.getMessage( resp, resp_lenth );
        }

        memset( head, 0, send_buf_sz + 4 );
        free( head );
        head = NULL;
        server_lock.unlock();
        return ERR_NO_ERROR;
    };

    void onDisconnected()
    {
        if( m_message_handler )
        {
            m_message_handler->onBreak();
        }
    }

    int onMessage( void *p_data, int n_data_sz )
    {
        if( p_data == NULL || n_data_sz <= 0 )
        {
            CubicLogE( "receive error" );
        }

        TMsgHead *head = ( TMsgHead * )p_data;

        switch( head->type )
        {
        case MSG_TYPE_REQ:
        case MSG_TYPE_NOTICE:
        {
            m_message_handler->onTcpMessage( p_data, n_data_sz );
            break;
        }

        case MSG_TYPE_RSP:
        {
            resp_wait_lock.lock();

            if( resp_wait_tab.end() != resp_wait_tab.find( head->uniq_id ) )
            {
                resp_wait_tab[head->uniq_id]->wake( ( char * )p_data + sizeof( TMsgHead ), head->package_size );
                resp_wait_tab.erase( head->uniq_id );
            }

            resp_wait_lock.unlock();
            break;
        }

        default:
            break;
        }

        return 0;
    }
};

