/*
 * WPA Supplicant
 *
 * SHA1 hash implementation and interface functions
 * Copyright (c) 2003-2005, Jouni Malinen <jkmaline@cc.hut.fi>
 *
 * This program is free software; distributed under the terms of BSD
 * license.
 */
#include "wf_universal_driver.h"
#include <sys/param.h>
#include <sys/systm.h>

#define SHA1_MAC_LEN 20

typedef struct {
    u_int32_t state[5];
    u_int32_t count[2];
    unsigned char buffer[64];
} SHA1_CTX;

/* ===== start - public domain SHA1 implementation ===== */

/*
 * SHA-1 in C
 * By Steve Reid <sreid@sea-to-sky.net>
 * 100% Public Domain
 */
/* -----------------
 * Modified 7/98
 * By James H. Brown <jbrown@burgoyne.com>
 * Still 100% Public Domain
 *
 * Corrected a problem which generated improper hash values on 16 bit machines
 * Routine SHA1Update changed from
 *     void SHA1Update(SHA1_CTX* context, unsigned char* data, unsigned int len)
 * to
 *     void SHA1Update(SHA1_CTX* context, unsigned char* data, unsigned long len)
 *
 * The 'len' parameter was declared an int which works fine on 32 bit machines.
 * However, on 16 bit machines an int is too small for the shifts being done against
 * it.  This caused the hash function to generate incorrect values if len was
 * greater than 8191 (8K - 1) due to the 'len << 3' on line 3 of SHA1Update().
 *
 * Since the file IO in main() reads 16K at a time, any file 8K or larger would
 * be guaranteed to generate the wrong hash (e.g. Test Vector #3, a million "a"s).
 *
 * I also changed the declaration of variables i & j in SHA1Update to
 * unsigned long from unsigned int for the same reason.
 *
 * These changes should make no difference to any 32 bit implementations since an
 * int and a long are the same size in those environments.
 *
 * --
 * I also corrected a few compiler warnings generated by Borland C.
 * 1. Added #include <process.h> for exit() prototype
 * 2. Removed unused variable 'j' in SHA1Final
 * 3. Changed exit(0) to return(0) at end of main.
 *
 * ALL changes I made can be located by searching for comments containing 'JHB'
 */
/* -----------------
 * Modified 8/98
 * By Steve Reid <sreid@sea-to-sky.net>
 * Still 100% public domain
 *
 * 1- Removed #include <process.h> and used return() instead of exit()
 * 2- Fixed overwriting of finalcount in SHA1Final() (discovered by Chris Hall)
 * 3- Changed email address from steve@edmweb.com to sreid@sea-to-sky.net
 */
/* -----------------
 * Modified 4/01
 * By Saul Kravitz <Saul.Kravitz@celera.com>
 * Still 100% PD
 * Modified to run on Compaq Alpha hardware.
 */
/* -----------------
 * Modified 4/01
 * By Jouni Malinen <jkmaline@cc.hut.fi>
 * Minor changes to match the coding style used in Dynamics.
 *
 * Modified September 24, 2004
 * By Jouni Malinen <jkmaline@cc.hut.fi>
 * Fixed alignment issue in SHA1Transform when SHA1HANDSOFF is defined.
 */

/*
 * Test Vectors (from FIPS PUB 180-1)
 * "abc"
 *   A9993E36 4706816A BA3E2571 7850C26C 9CD0D89D
 * "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
 *   84983E44 1C3BD26E BAAE4AA1 F95129E5 E54670F1
 * A million repetitions of "a"
 *   34AA973C D4C4DAA4 F61EEB2B DBAD2731 6534016F
 */

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

/* blk0() and blk() perform the initial expand. */
/* I got the idea of expanding during the round function from SSLeay */
#if BYTE_ORDER == LITTLE_ENDIAN
#define blk0(i) (block->l[i] = (rol(block->l[i], 24) & 0xFF00FF00) | \
    (rol(block->l[i], 8) & 0x00FF00FF))
#else
#define blk0(i) block->l[i]
#endif
#define blk(i) (block->l[i & 15] = rol(block->l[(i + 13) & 15] ^ \
    block->l[(i + 8) & 15] ^ block->l[(i + 2) & 15] ^ block->l[i & 15], 1))

/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define R0(v,w,x,y,z,i) \
    z += ((w & (x ^ y)) ^ y) + blk0(i) + 0x5A827999 + rol(v, 5); \
    w = rol(w, 30);
#define R1(v,w,x,y,z,i) \
    z += ((w & (x ^ y)) ^ y) + blk(i) + 0x5A827999 + rol(v, 5); \
    w = rol(w, 30);
#define R2(v,w,x,y,z,i) \
    z += (w ^ x ^ y) + blk(i) + 0x6ED9EBA1 + rol(v, 5); w = rol(w, 30);
#define R3(v,w,x,y,z,i) \
    z += (((w | x) & y) | (w & x)) + blk(i) + 0x8F1BBCDC + rol(v, 5); \
    w = rol(w, 30);
#define R4(v,w,x,y,z,i) \
    z += (w ^ x ^ y) + blk(i) + 0xCA62C1D6 + rol(v, 5); \
    w=rol(w, 30);

#ifdef VERBOSE  /* SAK */
static void SHAPrintContext(SHA1_CTX *context, char *msg)
{
    printf("%s (%d,%d) %x %x %x %x %x\n",
           msg,
           context->count[0], context->count[1],
           context->state[0],
           context->state[1],
           context->state[2],
           context->state[3],
           context->state[4]);
}
#endif

/*
 * Hash a single 512-bit block. This is the core of the algorithm.
 * Parameters:
 *  state - SHA-1 state
 *  data - Input data for the SHA-1 transform
 *
 * This function is used to implement random number generation specified in
 * NIST FIPS Publication 186-2 for EAP-SIM. This PRF uses a function that is
 * similar to SHA-1, but has different message padding and as such, access to
 * just part of the SHA-1 is needed.
 */
static void SHA1Transform(u_int32_t state[5], const unsigned char buffer[64])
{
    u_int32_t a, b, c, d, e;
    typedef union {
        unsigned char c[64];
        u_int32_t l[16];
    } CHAR64LONG16;
    CHAR64LONG16* block;

    u_int32_t workspace[16];
    block = (CHAR64LONG16 *) workspace;
    bcopy((u_int8_t*)buffer, (u_int8_t*)block, 64);
    /* Copy context->state[] to working vars */
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    /* 4 rounds of 20 operations each. Loop unrolled. */
    R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
    R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
    R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
    R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
    R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
    R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
    R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
    R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
    R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
    R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
    R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
    R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
    R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
    R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
    R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
    R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
    R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
    R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
    R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
    R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);
    /* Add the working vars back into context.state[] */
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    /* Wipe variables */
    a = b = c = d = e = 0;

    bzero((u_int8_t*)block, 64);
}

/*
 * Run your data through this.
 */
static void SHA1Update(SHA1_CTX* context, const void *_data, u_int32_t len)
{
    u_int32_t i, j;
    const unsigned char *data = _data;

#ifdef VERBOSE
    SHAPrintContext(context, "before");
#endif
    j = (context->count[0] >> 3) & 63;
    if ((context->count[0] += len << 3) < (len << 3))
        context->count[1]++;
    context->count[1] += (len >> 29);
    if ((j + len) > 63) {
        bcopy((u_int8_t*)data, (u_int8_t*)(&context->buffer[j]), (i = 64-j));
        SHA1Transform(context->state, context->buffer);
        for ( ; i + 63 < len; i += 64) {
            SHA1Transform(context->state, &data[i]);
        }
        j = 0;
    }
    else i = 0;
    bcopy((u_int8_t*)(&data[i]), (u_int8_t*)(&context->buffer[j]), len - i);
#ifdef VERBOSE
    SHAPrintContext(context, "after ");
#endif
}

/*
 * Add padding and return the message digest.
 */
static void SHA1Final(unsigned char digest[20], SHA1_CTX* context)
{
    u_int32_t i;
    unsigned char finalcount[8];

    for (i = 0; i < 8; i++) {
        finalcount[i] = (unsigned char)
            ((context->count[(i >= 4 ? 0 : 1)] >>
              ((3-(i & 3)) * 8) ) & 255);  /* Endian independent */
    }
    SHA1Update(context, (unsigned char *) "\200", 1);
    while ((context->count[0] & 504) != 448) {
        SHA1Update(context, (unsigned char *) "\0", 1);
    }
    SHA1Update(context, finalcount, 8);  /* Should cause a SHA1Transform()
                          */
    for (i = 0; i < 20; i++) {
        digest[i] = (unsigned char)
            ((context->state[i >> 2] >> ((3 - (i & 3)) * 8)) &
             255);
    }
    /* Wipe variables */
    i = 0;
    bzero(context->buffer, 64);
    bzero((u_int8_t*)(context->state), 20);
    bzero((u_int8_t*)(context->count), 8);
    bzero(finalcount, 8);
}
/* ===== end - public domain SHA1 implementation ===== */

/*
 * Initialize new context
 */
static void SHA1Init(SHA1_CTX* context)
{
    /* SHA1 initialization constants */
    context->state[0] = 0x67452301;
    context->state[1] = 0xEFCDAB89;
    context->state[2] = 0x98BADCFE;
    context->state[3] = 0x10325476;
    context->state[4] = 0xC3D2E1F0;
    context->count[0] = context->count[1] = 0;
}

/**
 * sha1_vector - SHA-1 hash for data vector
 * @num_elem: Number of elements in the data vector
 * @addr: Pointers to the data areas
 * @len: Lengths of the data blocks
 * @mac: Buffer for the hash
 */
static void sha1_vector(size_t num_elem, const u_int8_t *addr[], const size_t *len,
         u_int8_t *mac)
{
    SHA1_CTX ctx;
    int i;

    SHA1Init(&ctx);
    for (i = 0; i < num_elem; i++)
        SHA1Update(&ctx, addr[i], len[i]);
    SHA1Final(mac, &ctx);
}

/**
 * hmac_sha1_vector - HMAC-SHA1 over data vector (RFC 2104)
 * @key: Key for HMAC operations
 * @key_len: Length of the key in bytes
 * @num_elem: Number of elements in the data vector
 * @addr: Pointers to the data areas
 * @len: Lengths of the data blocks
 * @mac: Buffer for the hash (20 bytes)
 */
static void hmac_sha1_vector(const u_int8_t *key, size_t key_len, size_t num_elem,
              const u_int8_t *addr[], const size_t *len, u_int8_t *mac)
{
    unsigned char k_pad[64]; /* padding - key XORd with ipad/opad */
    unsigned char tk[20];
    int i;
    const u_int8_t *_addr[6];
    size_t _len[6];

    if (num_elem > 5) {
        /*
         * Fixed limit on the number of fragments to avoid having to
         * allocate memory (which could fail).
         */
        return;
    }

    /* if key is longer than 64 bytes reset it to key = SHA1(key) */
    if (key_len > 64) {
        sha1_vector(1, &key, &key_len, tk);
        key = tk;
        key_len = 20;
    }

    /* the HMAC_SHA1 transform looks like:
     *
     * SHA1(K XOR opad, SHA1(K XOR ipad, text))
     *
     * where K is an n byte key
     * ipad is the byte 0x36 repeated 64 times
     * opad is the byte 0x5c repeated 64 times
     * and text is the data being protected */

    /* start out by storing key in ipad */
    bzero(k_pad, sizeof(k_pad));
    bcopy((u_int8_t*)key, (u_int8_t*)k_pad, key_len);
    /* XOR key with ipad values */
    for (i = 0; i < 64; i++)
        k_pad[i] ^= 0x36;

    /* perform inner SHA1 */
    _addr[0] = k_pad;
    _len[0] = 64;
    for (i = 0; i < num_elem; i++) {
        _addr[i + 1] = addr[i];
        _len[i + 1] = len[i];
    }
    sha1_vector(1 + num_elem, _addr, _len, mac);

    bzero(k_pad, sizeof(k_pad));
    bcopy((u_int8_t*)key, (u_int8_t*)k_pad, key_len);
    /* XOR key with opad values */
    for (i = 0; i < 64; i++)
        k_pad[i] ^= 0x5c;

    /* perform outer SHA1 */
    _addr[0] = k_pad;
    _len[0] = 64;
    _addr[1] = mac;
    _len[1] = SHA1_MAC_LEN;
    sha1_vector(2, _addr, _len, mac);
}

/**
 * hmac_sha1 - HMAC-SHA1 over data buffer (RFC 2104)
 * @key: Key for HMAC operations
 * @key_len: Length of the key in bytes
 * @data: Pointers to the data area
 * @data_len: Length of the data area
 * @mac: Buffer for the hash (20 bytes)
 */
static void hmac_sha1(const u_int8_t *key, size_t key_len, const u_int8_t *data, size_t data_len,
           u_int8_t *mac)
{
    hmac_sha1_vector(key, key_len, 1, &data, &data_len, mac);
}

static void pbkdf2_sha1_f(const char *passphrase, const char *ssid,
              size_t ssid_len, int iterations, int count,
              u_int8_t *digest)
{
    unsigned char tmp[SHA1_MAC_LEN], tmp2[SHA1_MAC_LEN];
    int i, j;
    unsigned char count_buf[4];
    const u_int8_t *addr[2];
    size_t len[2];
    size_t passphrase_len = strlen(passphrase);

    addr[0] = (u_int8_t *) ssid;
    len[0] = ssid_len;
    addr[1] = count_buf;
    len[1] = 4;

    /* F(P, S, c, i) = U1 xor U2 xor ... Uc
     * U1 = PRF(P, S || i)
     * U2 = PRF(P, U1)
     * Uc = PRF(P, Uc-1)
     */

    count_buf[0] = (count >> 24) & 0xff;
    count_buf[1] = (count >> 16) & 0xff;
    count_buf[2] = (count >> 8) & 0xff;
    count_buf[3] = count & 0xff;
    hmac_sha1_vector((u_int8_t *) passphrase, passphrase_len, 2, addr, len, tmp);
    bcopy(tmp, digest, SHA1_MAC_LEN);

    for (i = 1; i < iterations; i++) {
        u_int8_t* pTemp;


        hmac_sha1((u_int8_t *) passphrase, passphrase_len, ((i&0x01)? tmp:tmp2), SHA1_MAC_LEN,
              ((i&0x01)? tmp2:tmp));
        //bcopy(tmp2, tmp, SHA1_MAC_LEN);

        pTemp = (i&0x01)? tmp2 : tmp;

        for (j = 0; j < SHA1_MAC_LEN; j++)
            digest[j] ^= pTemp[j];
    }
}

/**
 * pbkdf2_sha1 - SHA1-based key derivation function (PBKDF2) for IEEE 802.11i
 * @passphrase: ASCII passphrase
 * @ssid: SSID
 * @ssid_len: SSID length in bytes
 * @interations: Number of iterations to run
 * @buf: Buffer for the generated key
 * @buflen: Length of the buffer in bytes
 *
 * This function is used to derive PSK for WPA-PSK. For this protocol,
 * iterations is set to 4096 and buflen to 32. This function is described in
 * IEEE Std 802.11-2004, Clause H.4. The main construction is from PKCS#5 v2.0.
 */
void pbkdf2_sha1(const char *passphrase, const char *ssid, u_int16_t ssid_len,
         u_int16_t iterations, u_int8_t *buf, u_int16_t buflen)
{
    int count = 0;
    unsigned char *pos = buf;
    size_t left = buflen, plen;
    unsigned char digest[SHA1_MAC_LEN];

    while (left > 0) {
        count++;
        pbkdf2_sha1_f(passphrase, ssid, ssid_len, iterations, count,
                  digest);
        plen = left > SHA1_MAC_LEN ? SHA1_MAC_LEN : left;
        bcopy(digest, pos, plen);
        pos += plen;
        left -= plen;
    }
}