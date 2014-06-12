#ifndef _CYASSL_AES_H_
#define _CYASSL_AES_H_

typedef unsigned char  byte;
typedef unsigned short word16;
typedef unsigned int   word32;

enum {
    AES_ENC_TYPE   = 1,   /* cipher unique type */
    AES_ENCRYPTION = 0,
    AES_DECRYPTION = 1,
    AES_BLOCK_SIZE = 16
};


typedef struct Aes {
    /* AESNI needs key first, rounds 2nd, not sure why yet */
    word32 key[60];
    word32  rounds;

    word32 reg[AES_BLOCK_SIZE / sizeof(word32)];      /* for CBC mode */
    word32 tmp[AES_BLOCK_SIZE / sizeof(word32)];      /* same         */

#ifdef HAVE_AESGCM
    byte H[AES_BLOCK_SIZE];
#ifdef GCM_TABLE
    /* key-based fast multiplication table. */
    byte M0[256][AES_BLOCK_SIZE];
#endif /* GCM_TABLE */
#endif /* HAVE_AESGCM */
#ifdef CYASSL_AESNI
    byte use_aesni;
#endif /* CYASSL_AESNI */
#ifdef HAVE_CAVIUM
    AesType type;            /* aes key type */
    int     devId;           /* nitrox device id */
    word32  magic;           /* using cavium magic */
    word64  contextHandle;   /* nitrox context memory handle */
#endif
} Aes;

#endif
