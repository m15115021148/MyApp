/**
 * @file BleListener.cc
 * @author Shujie.Li
 * @version 1.0
 * @brief listener of ble
 * @detail listener of ble
 */
#ifndef _BLE_LISTENER_CC_
#define _BLE_LISTENER_CC_ 1

#include "cubic_inc.h"
#include "CUtil.cc"
#include "CThread.cc"
#include <iostream>
#include <termios.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/buffer.h>
#include <libserialport.h>

#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "BleListener"


#define CUBIC_DEV_PATH_BLE "/dev/ttyHSL1"
#define CUBIC_DEV_PATH_BLE_POWER "/sys/class/leds/BTenable/brightness"
#define CUBIC_DEV_PATH_BLE_RESET "/sys/class/leds/BTreset/brightness"
#define CUBIC_FILE_PATH_BLE_FW "/etc/ble580.bin"

#define BT_POWER_ON_RETRY     5
#define BOOT_WITHOUT_FIRMWARE 1

using namespace std;

class IBleMsgHandler
{
public:
    virtual int process( const char *json_req, int size_req, char *json_rsp, int size_rsp ) = 0;
};

class BleListener : protected CThread
{
private:
    static const int MAX_DATA_SIZE = 4096;
    static const int BLOCK_SIZE = 16;
    static const int VECTOR_SIZE = BLOCK_SIZE;
    static const int KEY_SIZE = 32;
    static const int SEND_STEP_SZ = 20;
    static const int SERIAL_BUF_SZ = 128;
    static const int BLE_NAME_LEN = 6;
    static const int BLE_ADDR_LEN = 12;


    int             m_fd;
    uint8_t        *m_buf;
    int             m_buf_sz;
    uint8_t         m_serial_buf[SERIAL_BUF_SZ + 4];
    uint8_t         m_serial_buf_sz;
    int             m_offset;
    IBleMsgHandler  *m_handler;
    uint8_t         m_psk_decode[KEY_SIZE + 4];
    uint8_t         m_psk_encode[KEY_SIZE + 4];
    uint8_t         m_ble_work_status;
    struct sp_port *m_serial_port;
    pthread_mutex_t m_mutex;

    static void thir_byte_to_hexstr( uint8_t in, uint8_t *out )
    {
        const char HEX_TO_CHAR[0x10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
        out[0] = HEX_TO_CHAR[0x0F & ( in >> 4 )];
        out[1] = HEX_TO_CHAR[0x0F & in];
    };


    static uint8_t thir_hexchar_to_val( char c )
    {
        switch( c )
        {
        case '0':
            return 0x00;

        case '1':
            return 0x01;

        case '2':
            return 0x02;

        case '3':
            return 0x03;

        case '4':
            return 0x04;

        case '5':
            return 0x05;

        case '6':
            return 0x06;

        case '7':
            return 0x07;

        case '8':
            return 0x08;

        case '9':
            return 0x09;

        case 'a':
        case 'A':
            return 0x0A;

        case 'b':
        case 'B':
            return 0x0B;

        case 'c':
        case 'C':
            return 0x0C;

        case 'd':
        case 'D':
            return 0x0D;

        case 'e':
        case 'E':
            return 0x0E;

        case 'f':
        case 'F':
            return 0x0F;
        }

        return 0;
    };

    static uint8_t thir_hexstr_to_byte( uint8_t *in )
    {
        uint8_t out = 0;
        out = thir_hexchar_to_val( in[0] );
        out <<= 4;
        out |= thir_hexchar_to_val( in[1] );
        return out;
    };


    static int openssl_err( const char *str, size_t len, void *u )
    {
        LOGD( "openssl_err: %s", str );
        return TRUE;
    }

    int decrypt( unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
                 unsigned char *iv, unsigned char *plaintext )
    {
        EVP_CIPHER_CTX *ctx = NULL;
        int len = 0;
        int plaintext_len = 0;
        int ret = -1;

        do
        {
            /* Create and initialise the context */
            BREAKIF( !( ctx = EVP_CIPHER_CTX_new() ) );
            /* Initialise the decryption operation. IMPORTANT - ensure you use a key
            * and IV size appropriate for your cipher
            * In this example we are using 256 bit AES (i.e. a 256 bit key). The
            * IV size for *most* modes is the same as the block size. For AES this
            * is 128 bits */
            BREAKIF( 1 != EVP_DecryptInit_ex( ctx, EVP_aes_256_cbc(), NULL, key, iv ) );
            //          EVP_CIPHER_CTX_set_padding(ctx, 0);
            /* Provide the message to be decrypted, and obtain the plaintext output.
            * EVP_DecryptUpdate can be called multiple times if necessary
            */
            BREAKIF( 1 != EVP_DecryptUpdate( ctx, plaintext, &len, ciphertext, ciphertext_len ) );
            plaintext_len = len;
            /* Finalise the decryption. Further plaintext bytes may be written at
            * this stage.
            */
            BREAKIF( 1 != EVP_DecryptFinal_ex( ctx, plaintext + len, &len ) );
            plaintext_len += len;
            ret = plaintext_len;
        }
        while( 0 );

        if( ret < 0 )
        {
            ERR_print_errors_cb( openssl_err, NULL );
        }

        /* Clean up */
        EVP_CIPHER_CTX_free( ctx );
        return ret;
    };

    int encrypt( unsigned char *plaintext, int plaintext_len, unsigned char *key,
                 unsigned char *iv, unsigned char *ciphertext )
    {
        EVP_CIPHER_CTX *ctx = NULL;
        int len = 0;
        int ciphertext_len = 0;
        int ret = -1;

        do
        {
            /* Create and initialise the context */
            BREAKIF( !( ctx = EVP_CIPHER_CTX_new() ) );
            /* Initialise the encryption operation. IMPORTANT - ensure you use a key
            * and IV size appropriate for your cipher
            * In this example we are using 256 bit AES (i.e. a 256 bit key). The
            * IV size for *most* modes is the same as the block size. For AES this
            * is 128 bits */
            BREAKIF( 1 != EVP_EncryptInit_ex( ctx, EVP_aes_256_cbc(), NULL, key, iv ) );
            /* Provide the message to be encrypted, and obtain the encrypted output.
            * EVP_EncryptUpdate can be called multiple times if necessary
            */
            BREAKIF( 1 != EVP_EncryptUpdate( ctx, ciphertext, &len, plaintext, plaintext_len ) );
            ciphertext_len = len;
            /* Finalise the encryption. Further ciphertext bytes may be written at
            * this stage.
            */
            BREAKIF( 1 != EVP_EncryptFinal_ex( ctx, ciphertext + len, &len ) );
            ciphertext_len += len;
            ret = ciphertext_len;
        }
        while( 0 );

        if( ret < 0 )
        {
            ERR_print_errors_cb( openssl_err, NULL );
        }

        /* Clean up */
        EVP_CIPHER_CTX_free( ctx );
        return ret;
    }

    int decrypt_buf()
    {
        uint8_t vector[VECTOR_SIZE];
        uint8_t *buf = NULL;
        int ret = -1;

        do
        {
            buf = ( uint8_t * )malloc( MAX_DATA_SIZE + VECTOR_SIZE );
            BREAKIF_LOGE( buf == NULL, "decrypt_buf fail to alloc buffer" );
            memset( buf, 0, MAX_DATA_SIZE + VECTOR_SIZE );
            memcpy( vector, m_buf, VECTOR_SIZE );
            ret = decrypt( m_buf + VECTOR_SIZE, m_buf_sz - VECTOR_SIZE, m_psk_decode, vector, buf );
            BREAKIF_LOGE( ret < 0, "decrypt_buf fail to decrypt" );
            memset( m_buf, 0, MAX_DATA_SIZE + VECTOR_SIZE );
            memcpy( m_buf, buf, ret );
            m_buf_sz = ret;
            ret = 0;
        }
        while( 0 );

        FREE( buf );
        return ret;
    };

    void generate_iv( uint8_t *buf )
    {
        srand( time( NULL ) );

        for( int i = 0; i + 3 < VECTOR_SIZE; i += 4 )
        {
            uint32_t n = rand();
            memcpy( buf + i, &n, 4 );
        }
    };

    int encrypt_buf()
    {
        uint8_t vector[VECTOR_SIZE + 4];
        uint8_t *buf = NULL;
        int ret = -1;

        do
        {
            buf = ( uint8_t * )malloc( MAX_DATA_SIZE + VECTOR_SIZE );
            BREAKIF_LOGE( buf == NULL, "encrypt_buf fail to alloc buffer" );
            memset( buf, 0, MAX_DATA_SIZE + VECTOR_SIZE );
            generate_iv( vector );
            ret = encrypt( m_buf, m_buf_sz, m_psk_encode, vector, buf );
            BREAKIF_LOGE( ret < 0, "encrypt_buf fail to encrypt" );
            memset( m_buf, 0, MAX_DATA_SIZE + VECTOR_SIZE );
            memcpy( m_buf, vector, VECTOR_SIZE );
            memcpy( m_buf + VECTOR_SIZE, buf, ret );
            m_buf_sz = ret + VECTOR_SIZE;
            ret = 0;
        }
        while( 0 );

        FREE( buf );
        return ret;
    };


    void send_one_pack( int offset )
    {
        static const int SEND_BUF_SZ = SEND_STEP_SZ + 2; // include "O:"
        uint8_t send_buf[SEND_BUF_SZ + 4]; // include "\r\n"
        int i = 0;
        int j = 0;
        LOGI( "send_one_pack offset=%d", offset );
        send_buf[j++] = 'O';
        send_buf[j++] = ':';

        for( ; j + 1 < SEND_BUF_SZ && i + offset < m_buf_sz; j += 2, i++ )
        {
            thir_byte_to_hexstr( m_buf[i + offset], send_buf + j );
        }

        send_buf[j++] = '\r';
        send_buf[j++] = '\n';
        send_buf[j] = '\0';
        LOGI( "send_one_pack:%d, %s", j, send_buf );
        sp_blocking_write( m_serial_port, send_buf, j, 500 );
        //write( m_fd, send_buf, j );
    }

    void send_reponse()
    {
        pthread_mutex_lock( &m_mutex );
        // notice resp is ready
        sp_blocking_write( m_serial_port, "O<\r\n", 4, 500 );

        for( int offset = 0; offset < m_buf_sz; offset += ( SEND_STEP_SZ >> 1 ) )
        {
            send_one_pack( offset );
            usleep( 10000 );
        }

        sp_blocking_write( m_serial_port, "O>\r\n", 4, 500 );
        pthread_mutex_unlock( &m_mutex );
    }

    void send_check_request()
    {
        m_ble_work_status = 0;
        pthread_mutex_lock( &m_mutex );
        sp_blocking_write( m_serial_port, "O?\r\n", 4, 500 );
        pthread_mutex_unlock( &m_mutex );
    };


    static const uint8_t BLE_CMD_STX = 0x02;
    static const uint8_t BLE_CMD_SOH = 0x01;
    static const uint8_t BLE_CMD_ACK = 0x06;
    //        static const uint8_t BLE_CMD_NACK = 0x15;

    int check_power_on()
    {
        RETNIF( m_serial_port == NULL, -1 );
        static const int BUF_SZ = 128;
        uint8_t buf[BUF_SZ] = {0};
        int count = 0;
        int ret = sp_blocking_read( m_serial_port, buf, BUF_SZ, 1000 );

        for( int i = 0; i < ret; i++ )
        {
            LOGI( "check_power_on: 0x%X", buf[i] );

            if( buf[i] == BLE_CMD_STX )
            {
                count ++;
            }
        };

        return count;
    };

    int setup_sn_and_addr()
    {
        static const int BLE_RESET_TIMEOUT = 1000;
        RETNIF( m_serial_port == NULL, -1 );
        string addr = CubicCfgGetStr( CUBIC_CFG_bt_addr );
        string sn = CubicCfgGetStr( CUBIC_CFG_serial_num );

        while( addr.length() < BLE_ADDR_LEN || sn.length() < BLE_NAME_LEN )
        {
            sleep( 1 );
            addr = CubicCfgGetStr( CUBIC_CFG_bt_addr );
            sn = CubicCfgGetStr( CUBIC_CFG_serial_num );
        }

        string cmd = "O#";
        cmd += sn.substr( sn.length() - BLE_NAME_LEN, BLE_NAME_LEN );
        cmd += addr.substr( addr.length() - BLE_ADDR_LEN, BLE_ADDR_LEN );
        cmd += "\r\n";
        return sp_blocking_write( m_serial_port, cmd.c_str(), cmd.length(), BLE_RESET_TIMEOUT );;
    };

    int power_up_and_send_fw()
    {
        RETNIF( m_serial_port == NULL, -1 );
#if BOOT_WITHOUT_FIRMWARE
        static const uint8_t BLE_CMD_THIR_START = 0x27;
        static const uint8_t BLE_CMD_THIR_START2 = 0x33;
#endif //BOOT_WITHOUT_FIRMWARE
        static const int BLE_FW_SZ_MAX = 1024 * 32;
        static const int BLE_PWR_TIMEOUT = 10;
        static const int BLE_PWR_RETRY = 1024;
        static const uint8_t BLE_CMD_STX = 0x02;
        static const uint8_t BLE_CMD_SOH = 0x01;
        static const uint8_t BLE_CMD_ACK = 0x06;
        static const uint8_t BLE_CMD_LEN_SOH = 3;
        uint16_t fw_len = 0;
        uint8_t *fw_buf = NULL;
        int ret = -1;
        int success = -1;
        int fd = -1;
        uint8_t c = 0;
        uint8_t soh[BLE_CMD_LEN_SOH];

        do
        {
            fd = open( CUBIC_FILE_PATH_BLE_FW, O_RDONLY | O_EXCL );
            BREAKIF_LOGE( fd <= 0, "fail to open firmware file: %s", CUBIC_FILE_PATH_BLE_FW );
            // check fw file size
            struct stat fst;
            ret = fstat( fd, &fst );
            BREAKIF_LOGE( ret < 0, "fail to open get file stat: %s", CUBIC_FILE_PATH_BLE_FW );
            BREAKIF_LOGE( fst.st_size > BLE_FW_SZ_MAX, "file size is out of limit: %s", CUBIC_FILE_PATH_BLE_FW );
            // alloc buffer
            fw_buf = ( uint8_t * )malloc( BLE_FW_SZ_MAX + BLE_CMD_LEN_SOH + 4 );
            BREAKIF_LOGE( fw_buf == NULL, "fail to alloc buffer for BT firm" );
            memset( fw_buf, 0, BLE_FW_SZ_MAX + BLE_CMD_LEN_SOH + 4 );
            // flush uart
            sp_flush( m_serial_port, SP_BUF_BOTH );
            // power up
            ret = CUtil::WriteFile( CUBIC_DEV_PATH_BLE_POWER, "1", 1 );
            BREAKIF_LOGE( ret != 1, "fail to set power for ble !" );
            usleep( 100000 );
            // do reset chipset status
            ret = CUtil::WriteFile( CUBIC_DEV_PATH_BLE_RESET, "1", 1 );
            BREAKIF_LOGE( ret != 1, "fail to poll up reset of ble !" );
            usleep( 200000 );
            ret = CUtil::WriteFile( CUBIC_DEV_PATH_BLE_RESET, "0", 1 );
            BREAKIF_LOGE( ret != 1, "fail to set power for ble !" );
            usleep( 100000 );
            // loading all fw
            ret = read( fd, fw_buf, BLE_FW_SZ_MAX );
            BREAKIF_LOGE( ret != fst.st_size, "read fw file failed !" );
            fw_len = ret;
            // prepare soh
            fw_len += BLE_CMD_LEN_SOH;
            soh[0] = BLE_CMD_SOH;
            soh[1] = ( uint8_t )( 0xFF & fw_len );
            soh[2] = ( uint8_t )( 0xFF & ( fw_len >> 8 ) );
            memcpy( fw_buf + fw_len - BLE_CMD_LEN_SOH, soh, BLE_CMD_LEN_SOH );
            c = 0;

            // wait for STX
            while( 1 )
            {
                for( int i = 0; c != BLE_CMD_STX && i < BLE_PWR_RETRY; i++ )
                {
#if BOOT_WITHOUT_FIRMWARE

                    if( c == BLE_CMD_THIR_START )
                    {
                        ret = sp_blocking_read( m_serial_port, &c, 1, BLE_PWR_TIMEOUT );
                        CONTINUEIF( c <= 0 );
                        LOGD( "START2? 0x%X", c );
                        CONTINUEIF( c != BLE_CMD_THIR_START2 );
                        success = 0;
                        break;
                    }

#endif //BOOT_WITHOUT_FIRMWARE
                    ret = sp_blocking_read( m_serial_port, &c, 1, BLE_PWR_TIMEOUT );

                    if( ret > 0 )
                    {
                        LOGD( "STX? 0x%X", c );
                    }
                }

                BREAKIF_LOGE( c != BLE_CMD_STX, "fail to wait for STX" );
                // send SOH
                ret = sp_blocking_write( m_serial_port, &soh, BLE_CMD_LEN_SOH, BLE_PWR_TIMEOUT );
                BREAKIF_LOGE( ret != BLE_CMD_LEN_SOH, "fail to send SOH" );
                // wait for ACK or NACK
                c = 0;

                for( int i = 0; c != BLE_CMD_ACK && c != BLE_CMD_STX && i < BLE_PWR_RETRY; i++ )
                {
                    sp_blocking_read( m_serial_port, &c, 1, BLE_PWR_TIMEOUT );
                    LOGD( "ACK? 0x%X", c );
                }

                CONTINUEIF( c == BLE_CMD_STX );
                BREAKIF_LOGE( c != BLE_CMD_ACK, "fail to wait for ACK" );
                // send fw
                ret = sp_blocking_write( m_serial_port, fw_buf, fw_len, BLE_PWR_TIMEOUT * BLE_PWR_RETRY );
                BREAKIF_LOGE( ret != fw_len, "fail to send fw" );
                // wait for CRC
                ret = sp_blocking_read( m_serial_port, &c, 1, BLE_PWR_TIMEOUT * BLE_PWR_RETRY );
                BREAKIF_LOGE( ret != 1, "fail to wait CRC" );
                CONTINUEIF( c == BLE_CMD_STX );
                // check CRC and send ACK
                LOGD( "crc_bt:0x%x", c );
                sp_blocking_write( m_serial_port, &BLE_CMD_ACK, 1, BLE_PWR_TIMEOUT );
                break;
            }

            success = 0;
        }
        while( 0 );

        FREE( fw_buf );
        CLOSE( fd );

        if( success < 0 )
        {
            // power off
            CUtil::WriteFile( CUBIC_DEV_PATH_BLE_POWER, "0", 1 );
            return success;
        }

        return 0;
    };
public:
    BleListener()
        : m_fd( -1 )
        , m_buf( NULL )
        , m_buf_sz( 0 )
        , m_serial_buf()
        , m_serial_buf_sz( 0 )
        , m_offset( 0 )
        , m_handler( NULL )
        , m_psk_decode()
        , m_psk_encode()
        , m_ble_work_status( 0 )
        , m_serial_port( NULL )
    {
        OpenSSL_add_all_algorithms();
        //OPENSSL_config(NULL);
        m_buf = ( uint8_t * )malloc( MAX_DATA_SIZE + VECTOR_SIZE + 4 );
        memset( m_serial_buf, 0, sizeof( m_serial_buf ) );
        pthread_mutex_init( &m_mutex, NULL );
        ERR_load_crypto_strings();
    };

    virtual ~BleListener()
    {
        stop();
        pthread_mutex_destroy( &m_mutex );
        DELETE( m_buf );
        ERR_free_strings();
        EVP_cleanup();
    };

    int start( IBleMsgHandler *handler )
    {
        RETNIF( CThread::isRunning(), 0 );
        m_handler = handler;
        m_buf_sz = 0;
        m_serial_buf_sz = 0;
        int ret = 0;
        ret = sp_get_port_by_name( CUBIC_DEV_PATH_BLE, &m_serial_port );
        RETNIF_LOGE( SP_OK != ret || m_serial_port == NULL, -1, "can not found port" );
        ret = sp_open( m_serial_port, SP_MODE_READ_WRITE );
        RETNIF_LOGE( SP_OK != ret, -2, "fail to open port" );
        sp_set_baudrate(    m_serial_port, 57600 );
        sp_set_bits(        m_serial_port, 8 );
        sp_set_parity(      m_serial_port, SP_PARITY_NONE );
        sp_set_stopbits(    m_serial_port, 1 );
        sp_set_flowcontrol( m_serial_port, SP_FLOWCONTROL_NONE );
        const uint8_t PSK_ENCODE_KEY[KEY_SIZE] =
        {
            0xa2, 0xe3, 0xfc, 0x8f, 0x90, 0x27, 0x1a, 0x92,
            0x6f, 0xbc, 0xd4, 0xc1, 0x80, 0x9b, 0xdc, 0xea,
            0xbe, 0x51, 0x57, 0x14, 0xfe, 0xf2, 0x0a, 0x9b,
            0x4d, 0x6f, 0x22, 0x23, 0x09, 0x81, 0x96, 0x4f,
        }; //ouP8j5AnGpJvvNTBgJvc6r5RVxT+8gqbTW8iIwmBlk8=
        memcpy( m_psk_encode, PSK_ENCODE_KEY, KEY_SIZE );
        const uint8_t PSK_DECODE_KEY[KEY_SIZE] =
        {
            0x0f, 0xbe, 0x0f, 0xe3, 0xc2, 0xb3, 0x1f, 0x1b,
            0xf8, 0xa1, 0x81, 0x79, 0xd3, 0xe2, 0x91, 0xaf,
            0xd3, 0x9c, 0x6f, 0x04, 0x92, 0x3d, 0x86, 0xee,
            0x85, 0xe3, 0x82, 0xf1, 0xe8, 0xd0, 0x8c, 0x01,
        }; //D74P48KzHxv4oYF50+KRr9OcbwSSPYbuheOC8ejQjAE=
        memcpy( m_psk_decode, PSK_DECODE_KEY, KEY_SIZE );

        do
        {
            // power off it anyway
            CUtil::WriteFile( CUBIC_DEV_PATH_BLE_POWER, "0", 1 );
            usleep( 100000 );
            int retry = 0;

            for( ; retry < BT_POWER_ON_RETRY && power_up_and_send_fw() != 0; retry++ )
            {
                LOGE( "fail to powr on bt, retry = %d", retry );
                sleep( 1 );
            }

            RETNIF_LOGE( retry >= BT_POWER_ON_RETRY, -2, "fail to powr on bt, and out of max retry !" );
            // check if power on
            ret = check_power_on();
        }
        while( ret > 20 );

        sp_set_baudrate( m_serial_port, 115200 );
        sleep( 1 );
        setup_sn_and_addr();
        return CThread::start() ? 0 : -3;
    }


    void stop()
    {
        CThread::stop();

        if( m_serial_port )
        {
            sp_close( m_serial_port );
            sp_free_port( m_serial_port );
            m_serial_port = NULL;
        }

        CUtil::WriteFile( CUBIC_DEV_PATH_BLE_POWER, "0", 1 );
    };


    void handle_serial_data( char *buf, int sz )
    {
        int ret = 0;
        RETIF_LOGE( buf == NULL && sz <= 0, "buffer is not available !" );

        for( int idx = 0; idx < sz; idx++ )
        {
            putchar( buf[idx] );
            m_serial_buf[m_serial_buf_sz] = buf[idx];

            if( m_serial_buf_sz >= SERIAL_BUF_SZ )
            {
                memcpy( m_serial_buf, m_serial_buf + 1, SERIAL_BUF_SZ );
                m_serial_buf[SERIAL_BUF_SZ] = 0;
            }
            else
            {
                m_serial_buf_sz++;
            }

            // check if there is '\r' '\n'
            uint8_t *p = NULL;

            for( uint8_t *pnt = m_serial_buf; *pnt != 0; pnt++ )
            {
                if( *pnt == '\r' || *pnt == '\n' )
                {
                    p = pnt;
                    break;
                }
            }

            CONTINUEIF( p == NULL );
            *p = 0;
            ret = p - m_serial_buf;

            //LOGD("read:%d buf:%s", ret, m_serial_buf);
            if( memcmp( m_serial_buf, "I<", 2 ) == 0 )
            {
                LOGI( "START REQUEST" );
                m_buf_sz = 0;
                memset( m_buf, 0, MAX_DATA_SIZE );
            }

            if( memcmp( m_serial_buf, "I:", 2 ) == 0 )
            {
                int j = 2;
                LOGI( "BUF REQUEST when:%d input:%s", m_buf_sz, m_serial_buf );

                while( m_buf_sz < MAX_DATA_SIZE && j + 1 < ret )
                {
                    m_buf[m_buf_sz] = thir_hexstr_to_byte( m_serial_buf + j );
                    m_buf_sz++;
                    j += 2;
                }

                m_buf[m_buf_sz] = 0;
                LOGI( "BUF REQUEST to:%d", m_buf_sz );
            }

            if( memcmp( m_serial_buf, "I>", 2 ) == 0 )
            {
                do
                {
                    LOGI( "END REQUEST m_buf_sz=%d", m_buf_sz );
                    BREAKIF_LOGE( m_handler == NULL, "NO Handler" );
                    // decrypt data
                    BREAKIF_LOGE( decrypt_buf() != 0, "decrypt failed" );
                    // process
                    char *resp_data = ( char * )malloc( MAX_DATA_SIZE );
                    BREAKIF_LOGE( resp_data == 0, "malloc resp buffer failed" );
                    memset( resp_data, 0, MAX_DATA_SIZE );
                    int resp_len = m_handler->process( ( char * )m_buf, m_buf_sz, resp_data, MAX_DATA_SIZE );

                    if( resp_len > 0 && resp_len < MAX_DATA_SIZE )
                    {
                        memcpy( m_buf, resp_data, resp_len );
                        m_buf[resp_len] = 0;
                        m_buf_sz = resp_len;
                    }

                    FREE( resp_data );
                    BREAKIF_LOGE( resp_len <= 0, "process failed" );
                    // encrypt response
                    BREAKIF_LOGE( encrypt_buf() != 0, "encrypt failed" );
                    LOGI( "after encrypt_buf m_buf_sz=%d", m_buf_sz );
                    send_reponse();
                }
                while( 0 );
            }

            if( memcmp( m_serial_buf, "I!", 2 ) == 0 )
            {
                m_ble_work_status = 1;
            }

            p ++;
            ret ++;

            for( ; *p == '\r' || *p == '\n'; p++, ret++ );

            m_serial_buf_sz = m_serial_buf_sz - ret;
            memcpy( m_serial_buf, p, m_serial_buf_sz );
            memset( m_serial_buf + m_serial_buf_sz, 0, sizeof( m_serial_buf ) - m_serial_buf_sz );
        }
    };

    virtual RunRet run( void *user )
    {
        RETNIF( m_serial_port == NULL, RUN_END );
        RETNIF( m_buf == NULL, RUN_END );
        static const int BUF_SZ = 1024;
        int ret = 0;
        char buf[BUF_SZ + 4];
        memset( buf, 0, sizeof( buf ) );
        ret = sp_blocking_read_next( m_serial_port, buf, BUF_SZ, 10 );
        RETNIF( needAbort() || ret <= 0, RUN_CONTINUE );
        LOGI( "run r=%d, buf=%s", ret, buf );
        handle_serial_data( buf, ret );
        return RUN_CONTINUE;
    };

    void doCheck()
    {
        send_check_request();
    };

    unsigned int getCheckStat()
    {
        return m_ble_work_status;
    };
};


#endif //_BLE_LISTENER_CC_
