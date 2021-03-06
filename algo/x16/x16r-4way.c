/**
 * x16r algo implementation
 *
 * Implementation by tpruvot@github Jan 2018
 * Optimized by JayDDee@github Jan 2018
 */
#include "x16r-gate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "algo/blake/blake-hash-4way.h"
#include "algo/bmw/bmw-hash-4way.h"
#include "algo/groestl/aes_ni/hash-groestl.h"
#include "algo/groestl/aes_ni/hash-groestl.h"
#include "algo/skein/skein-hash-4way.h"
#include "algo/jh/jh-hash-4way.h"
#include "algo/keccak/keccak-hash-4way.h"
#include "algo/shavite/sph_shavite.h"
#include "algo/luffa/luffa-hash-2way.h"
#include "algo/cubehash/cube-hash-2way.h"
#include "algo/cubehash/cubehash_sse2.h"
#include "algo/simd/simd-hash-2way.h"
#include "algo/echo/aes_ni/hash_api.h"
#include "algo/hamsi/hamsi-hash-4way.h"
#include "algo/fugue/sph_fugue.h"
#include "algo/shabal/shabal-hash-4way.h"
#include "algo/whirlpool/sph_whirlpool.h"
#include "algo/sha/sha-hash-4way.h"
#if defined(__VAES__)
  #include "algo/groestl/groestl512-hash-4way.h"
  #include "algo/shavite/shavite-hash-4way.h"
  #include "algo/echo/echo-hash-4way.h"
#endif

static __thread uint32_t s_ntime = UINT32_MAX;
static __thread char hashOrder[X16R_HASH_FUNC_COUNT + 1] = { 0 };

#if defined (X16R_8WAY)

union _x16r_8way_context_overlay
{
    blake512_8way_context   blake;
    bmw512_8way_context     bmw;
    skein512_8way_context   skein;
    jh512_8way_context      jh;
    keccak512_8way_context  keccak;
    luffa_4way_context      luffa;
    cube_4way_context       cube;
    simd_4way_context       simd;
    hamsi512_8way_context   hamsi;
    sph_fugue512_context    fugue;
    shabal512_8way_context  shabal;
    sph_whirlpool_context   whirlpool;
    sha512_8way_context     sha512;
#if defined(__VAES__)
    groestl512_4way_context groestl;
    shavite512_4way_context shavite;
    echo_4way_context       echo;
#else
    hashState_groestl       groestl;
    sph_shavite512_context  shavite;
    hashState_echo          echo;
#endif
} __attribute__ ((aligned (64)));

typedef union _x16r_8way_context_overlay x16r_8way_context_overlay;


void x16r_8way_hash( void* output, const void* input )
{
   uint32_t vhash[24*8] __attribute__ ((aligned (128)));
   uint32_t hash0[24] __attribute__ ((aligned (64)));
   uint32_t hash1[24] __attribute__ ((aligned (64)));
   uint32_t hash2[24] __attribute__ ((aligned (64)));
   uint32_t hash3[24] __attribute__ ((aligned (64)));
   uint32_t hash4[24] __attribute__ ((aligned (64)));
   uint32_t hash5[24] __attribute__ ((aligned (64)));
   uint32_t hash6[24] __attribute__ ((aligned (64)));
   uint32_t hash7[24] __attribute__ ((aligned (64)));
   x16r_8way_context_overlay ctx;
   void *in0 = (void*) hash0;
   void *in1 = (void*) hash1;
   void *in2 = (void*) hash2;
   void *in3 = (void*) hash3;
   void *in4 = (void*) hash4;
   void *in5 = (void*) hash5;
   void *in6 = (void*) hash6;
   void *in7 = (void*) hash7;
   int size = 80;

   dintrlv_8x64( hash0, hash1, hash2, hash3, hash4, hash5, hash6, hash7,
                 input, 640 );

   for ( int i = 0; i < 16; i++ )
   {
      const char elem = hashOrder[i];
      const uint8_t algo = elem >= 'A' ? elem - 'A' + 10 : elem - '0';

      switch ( algo )
      {
         case BLAKE:
            if ( i == 0 )
               blake512_8way_full( &ctx.blake, vhash, input, size );
            else
            {
               intrlv_8x64( vhash, in0, in1, in2, in3, in4, in5, in6, in7, 
                            size<<3 );
               blake512_8way_full( &ctx.blake, vhash, vhash, size );
            }
            dintrlv_8x64_512( hash0, hash1, hash2, hash3, hash4, hash5,
                                 hash6, hash7, vhash );
         break;
         case BMW:
            bmw512_8way_init( &ctx.bmw );
            if ( i == 0 )
               bmw512_8way_update( &ctx.bmw, input, size );
            else
            {
               intrlv_8x64( vhash, in0, in1, in2, in3, in4, in5, in6, in7,
                            size<<3 );
            bmw512_8way_update( &ctx.bmw, vhash, size );
            }
            bmw512_8way_close( &ctx.bmw, vhash );
            dintrlv_8x64_512( hash0, hash1, hash2, hash3, hash4, hash5, hash6,
                          hash7, vhash );
         break;
         case GROESTL:
#if defined(__VAES__)
            intrlv_4x128( vhash, in0, in1, in2, in3, size<<3 );
            groestl512_4way_full( &ctx.groestl, vhash, vhash, size );
            dintrlv_4x128_512( hash0, hash1, hash2, hash3, vhash );
            intrlv_4x128( vhash, in4, in5, in6, in7, size<<3 );
            groestl512_4way_full( &ctx.groestl, vhash, vhash, size );
            dintrlv_4x128_512( hash4, hash5, hash6, hash7, vhash );
#else
            groestl512_full( &ctx.groestl, (char*)hash0, (char*)in0, size<<3 );
            groestl512_full( &ctx.groestl, (char*)hash1, (char*)in1, size<<3 );
            groestl512_full( &ctx.groestl, (char*)hash2, (char*)in2, size<<3 );
            groestl512_full( &ctx.groestl, (char*)hash3, (char*)in3, size<<3 );
            groestl512_full( &ctx.groestl, (char*)hash4, (char*)in4, size<<3 );
            groestl512_full( &ctx.groestl, (char*)hash5, (char*)in5, size<<3 );
            groestl512_full( &ctx.groestl, (char*)hash6, (char*)in6, size<<3 );
            groestl512_full( &ctx.groestl, (char*)hash7, (char*)in7, size<<3 );
#endif
         break;
         case SKEIN:
            skein512_8way_init( &ctx.skein );
            if ( i == 0 )
               skein512_8way_update( &ctx.skein, input, size );
            else
            {
               intrlv_8x64( vhash, in0, in1, in2, in3, in4, in5, in6, in7,
                            size<<3 );
               skein512_8way_update( &ctx.skein, vhash, size );
            }
            skein512_8way_close( &ctx.skein, vhash );
            dintrlv_8x64_512( hash0, hash1, hash2, hash3, hash4, hash5, hash6,
                          hash7, vhash );
         break;
         case JH:
            jh512_8way_init( &ctx.jh );
            if ( i == 0 )
               jh512_8way_update( &ctx.jh, input, size );
            else
            {
               intrlv_8x64( vhash, in0, in1, in2, in3, in4, in5, in6, in7, 
                            size<<3 );
               jh512_8way_update( &ctx.jh, vhash, size );
            }
            jh512_8way_close( &ctx.jh, vhash );
            dintrlv_8x64_512( hash0, hash1, hash2, hash3, hash4, hash5, hash6,
                          hash7, vhash );
         break;
         case KECCAK:
            keccak512_8way_init( &ctx.keccak );
            if ( i == 0 )
               keccak512_8way_update( &ctx.keccak, input, size );
            else
            {
               intrlv_8x64( vhash, in0, in1, in2, in3, in4, in5, in6, in7, 
                            size<<3 );
               keccak512_8way_update( &ctx.keccak, vhash, size );
            }
            keccak512_8way_close( &ctx.keccak, vhash );
            dintrlv_8x64_512( hash0, hash1, hash2, hash3, hash4, hash5, hash6,
                          hash7, vhash );
         break;
         case LUFFA:
            intrlv_4x128( vhash, in0, in1, in2, in3, size<<3 );
            luffa512_4way_full( &ctx.luffa, vhash, vhash, size );
            dintrlv_4x128_512( hash0, hash1, hash2, hash3, vhash );
            intrlv_4x128( vhash, in4, in5, in6, in7, size<<3 );
            luffa512_4way_full( &ctx.luffa, vhash, vhash, size );
            dintrlv_4x128_512( hash4, hash5, hash6, hash7, vhash );
         break;
         case CUBEHASH:
            intrlv_4x128( vhash, in0, in1, in2, in3, size<<3 );
            cube_4way_full( &ctx.cube, vhash, 512, vhash, size );
            dintrlv_4x128_512( hash0, hash1, hash2, hash3, vhash );
            intrlv_4x128( vhash, in4, in5, in6, in7, size<<3 );
            cube_4way_full( &ctx.cube, vhash, 512, vhash, size );
            dintrlv_4x128_512( hash4, hash5, hash6, hash7, vhash );
         break;
         case SHAVITE:
#if defined(__VAES__)
            intrlv_4x128( vhash, in0, in1, in2, in3, size<<3 );
            shavite512_4way_full( &ctx.shavite, vhash, vhash, size );
            dintrlv_4x128_512( hash0, hash1, hash2, hash3, vhash );
            intrlv_4x128( vhash, in4, in5, in6, in7, size<<3 );
            shavite512_4way_full( &ctx.shavite, vhash, vhash, size );
            dintrlv_4x128_512( hash4, hash5, hash6, hash7, vhash );
#else
            sph_shavite512_init( &ctx.shavite );
            sph_shavite512( &ctx.shavite, in0, size );
            sph_shavite512_close( &ctx.shavite, hash0 );
            sph_shavite512_init( &ctx.shavite );
            sph_shavite512( &ctx.shavite, in1, size );
            sph_shavite512_close( &ctx.shavite, hash1 );
            sph_shavite512_init( &ctx.shavite );
            sph_shavite512( &ctx.shavite, in2, size );
            sph_shavite512_close( &ctx.shavite, hash2 );
            sph_shavite512_init( &ctx.shavite );
            sph_shavite512( &ctx.shavite, in3, size );
            sph_shavite512_close( &ctx.shavite, hash3 );
            sph_shavite512_init( &ctx.shavite );
            sph_shavite512( &ctx.shavite, in4, size );
            sph_shavite512_close( &ctx.shavite, hash4 );
            sph_shavite512_init( &ctx.shavite );
            sph_shavite512( &ctx.shavite, in5, size );
            sph_shavite512_close( &ctx.shavite, hash5 );
            sph_shavite512_init( &ctx.shavite );
            sph_shavite512( &ctx.shavite, in6, size );
            sph_shavite512_close( &ctx.shavite, hash6 );
            sph_shavite512_init( &ctx.shavite );
            sph_shavite512( &ctx.shavite, in7, size );
            sph_shavite512_close( &ctx.shavite, hash7 );
#endif
         break;
         case SIMD:
            intrlv_4x128( vhash, in0, in1, in2, in3, size<<3 );
            simd512_4way_full( &ctx.simd, vhash, vhash, size );
            dintrlv_4x128_512( hash0, hash1, hash2, hash3, vhash );
            intrlv_4x128( vhash, in4, in5, in6, in7, size<<3 );
            simd512_4way_full( &ctx.simd, vhash, vhash, size );
            dintrlv_4x128_512( hash4, hash5, hash6, hash7, vhash );
         break;
         case ECHO:
#if defined(__VAES__)
            intrlv_4x128( vhash, in0, in1, in2, in3, size<<3 );
            echo_4way_full( &ctx.echo, vhash, 512, vhash, size );
            dintrlv_4x128_512( hash0, hash1, hash2, hash3, vhash );
            intrlv_4x128( vhash, in4, in5, in6, in7, size<<3 );
            echo_4way_full( &ctx.echo, vhash, 512, vhash, size );
            dintrlv_4x128_512( hash4, hash5, hash6, hash7, vhash );
#else
            echo_full( &ctx.echo, (BitSequence *)hash0, 512,
                              (const BitSequence *)in0, size );
            echo_full( &ctx.echo, (BitSequence *)hash1, 512,
                              (const BitSequence *)in1, size );
            echo_full( &ctx.echo, (BitSequence *)hash2, 512,
                              (const BitSequence *)in2, size );
            echo_full( &ctx.echo, (BitSequence *)hash3, 512,
                              (const BitSequence *)in3, size );
            echo_full( &ctx.echo, (BitSequence *)hash4, 512,
                              (const BitSequence *)in4, size );
            echo_full( &ctx.echo, (BitSequence *)hash5, 512,
                              (const BitSequence *)in5, size );
            echo_full( &ctx.echo, (BitSequence *)hash6, 512,
                              (const BitSequence *)in6, size );
            echo_full( &ctx.echo, (BitSequence *)hash7, 512,
                              (const BitSequence *)in7, size );
#endif
         break;
         case HAMSI:
             intrlv_8x64( vhash, in0, in1, in2, in3, in4, in5, in6, in7,
                            size<<3 );

             hamsi512_8way_init( &ctx.hamsi );
             hamsi512_8way_update( &ctx.hamsi, vhash, size );
             hamsi512_8way_close( &ctx.hamsi, vhash );
             dintrlv_8x64_512( hash0, hash1, hash2, hash3, hash4, hash5, hash6,
                          hash7, vhash );
         break;
         case FUGUE:
             sph_fugue512_init( &ctx.fugue );
             sph_fugue512( &ctx.fugue, in0, size );
             sph_fugue512_close( &ctx.fugue, hash0 );
             sph_fugue512_init( &ctx.fugue );
             sph_fugue512( &ctx.fugue, in1, size );
             sph_fugue512_close( &ctx.fugue, hash1 );
             sph_fugue512_init( &ctx.fugue );
             sph_fugue512( &ctx.fugue, in2, size );
             sph_fugue512_close( &ctx.fugue, hash2 );
             sph_fugue512_init( &ctx.fugue );
             sph_fugue512( &ctx.fugue, in3, size );
             sph_fugue512_close( &ctx.fugue, hash3 );
             sph_fugue512_init( &ctx.fugue );
             sph_fugue512( &ctx.fugue, in4, size );
             sph_fugue512_close( &ctx.fugue, hash4 );
             sph_fugue512_init( &ctx.fugue );
             sph_fugue512( &ctx.fugue, in5, size );
             sph_fugue512_close( &ctx.fugue, hash5 );
             sph_fugue512_init( &ctx.fugue );
             sph_fugue512( &ctx.fugue, in6, size );
             sph_fugue512_close( &ctx.fugue, hash6 );
             sph_fugue512_init( &ctx.fugue );
             sph_fugue512( &ctx.fugue, in7, size );
             sph_fugue512_close( &ctx.fugue, hash7 );
         break;
         case SHABAL:
             intrlv_8x32( vhash, in0, in1, in2, in3, in4, in5, in6, in7,
                          size<<3 );
             shabal512_8way_init( &ctx.shabal );
             shabal512_8way_update( &ctx.shabal, vhash, size );
             shabal512_8way_close( &ctx.shabal, vhash );
             dintrlv_8x32_512( hash0, hash1, hash2, hash3, hash4, hash5, hash6,
                          hash7, vhash );
         break;
         case WHIRLPOOL:
             sph_whirlpool_init( &ctx.whirlpool );
             sph_whirlpool( &ctx.whirlpool, in0, size );
             sph_whirlpool_close( &ctx.whirlpool, hash0 );
             sph_whirlpool_init( &ctx.whirlpool );
             sph_whirlpool( &ctx.whirlpool, in1, size );
             sph_whirlpool_close( &ctx.whirlpool, hash1 );
             sph_whirlpool_init( &ctx.whirlpool );
             sph_whirlpool( &ctx.whirlpool, in2, size );
             sph_whirlpool_close( &ctx.whirlpool, hash2 );
             sph_whirlpool_init( &ctx.whirlpool );
             sph_whirlpool( &ctx.whirlpool, in3, size );
             sph_whirlpool_close( &ctx.whirlpool, hash3 );
             sph_whirlpool_init( &ctx.whirlpool );
             sph_whirlpool( &ctx.whirlpool, in4, size );
             sph_whirlpool_close( &ctx.whirlpool, hash4 );
             sph_whirlpool_init( &ctx.whirlpool );
             sph_whirlpool( &ctx.whirlpool, in5, size );
             sph_whirlpool_close( &ctx.whirlpool, hash5 );
             sph_whirlpool_init( &ctx.whirlpool );
             sph_whirlpool( &ctx.whirlpool, in6, size );
             sph_whirlpool_close( &ctx.whirlpool, hash6 );
             sph_whirlpool_init( &ctx.whirlpool );
             sph_whirlpool( &ctx.whirlpool, in7, size );
             sph_whirlpool_close( &ctx.whirlpool, hash7 );
         break;
         case SHA_512:
             sha512_8way_init( &ctx.sha512 );
             if ( i == 0 )
                sha512_8way_update( &ctx.sha512, input, size );
             else
             {
                intrlv_8x64( vhash, in0, in1, in2, in3, in4, in5, in6, in7,
                             size<<3 );
                sha512_8way_update( &ctx.sha512, vhash, size );
             }
             sha512_8way_close( &ctx.sha512, vhash );
             dintrlv_8x64_512( hash0, hash1, hash2, hash3, hash4, hash5, hash6,
                               hash7, vhash );
         break;
      }
      size = 64;
   }

   memcpy( output,     hash0, 32 );
   memcpy( output+32,  hash1, 32 );
   memcpy( output+64,  hash2, 32 );
   memcpy( output+96,  hash3, 32 );
   memcpy( output+128, hash4, 32 );
   memcpy( output+160, hash5, 32 );
   memcpy( output+192, hash6, 32 );
   memcpy( output+224, hash7, 32 );
}

int scanhash_x16r_8way( struct work *work, uint32_t max_nonce,
                        uint64_t *hashes_done, struct thr_info *mythr)
{
   uint32_t hash[8*16] __attribute__ ((aligned (128)));
   uint32_t vdata[24*8] __attribute__ ((aligned (64)));
   uint32_t bedata1[2] __attribute__((aligned(64)));
   uint32_t *pdata = work->data;
   uint32_t *ptarget = work->target;
   const uint32_t Htarg = ptarget[7];
   const uint32_t first_nonce = pdata[19];
   const uint32_t last_nonce = max_nonce - 8;
   uint32_t n = first_nonce;
    __m512i  *noncev = (__m512i*)vdata + 9;   // aligned
   int thr_id = mythr->id;
   volatile uint8_t *restart = &(work_restart[thr_id].restart);

   if ( opt_benchmark )
      ptarget[7] = 0x0cff;

   mm512_bswap32_intrlv80_8x64( vdata, pdata );

   bedata1[0] = bswap_32( pdata[1] );
   bedata1[1] = bswap_32( pdata[2] );
   const uint32_t ntime = bswap_32( pdata[17] );
   if ( s_ntime != ntime )
   {
      x16_r_s_getAlgoString( (const uint8_t*)bedata1, hashOrder );
      s_ntime = ntime;
      if ( opt_debug && !thr_id )
              applog( LOG_INFO, "hash order %s (%08x)", hashOrder, ntime );
   }

   do
   {
      *noncev = mm512_intrlv_blend_32( mm512_bswap_32(
           _mm512_set_epi32( n+7, 0, n+6, 0, n+5, 0, n+4, 0,
                             n+3, 0, n+2, 0, n+1, 0, n,   0 ) ), *noncev );

      x16r_8way_hash( hash, vdata );
      pdata[19] = n;

      for ( int i = 0; i < 8; i++ )
      if ( unlikely( (hash+(i<<3))[7] <= Htarg ) )
      if( likely( fulltest( hash+(i<<3), ptarget ) && !opt_benchmark ) )
      {
         pdata[19] = n+i;
         submit_lane_solution( work, hash+(i<<3), mythr, i );
      }
      n += 8;
   } while ( likely( ( n < last_nonce ) && !(*restart) ) );

   *hashes_done = n - first_nonce;
   return 0;
}


#elif defined (X16R_4WAY)

union _x16r_4way_context_overlay
{
    blake512_4way_context   blake;
    bmw512_4way_context     bmw;
    hashState_echo          echo;
    hashState_groestl       groestl;
    skein512_4way_context   skein;
    jh512_4way_context      jh;
    keccak512_4way_context  keccak;
    luffa_2way_context      luffa;
    cubehashParam           cube;
    sph_shavite512_context  shavite;
    simd_2way_context       simd;
    hamsi512_4way_context   hamsi;
    sph_fugue512_context    fugue;
    shabal512_4way_context  shabal;
    sph_whirlpool_context   whirlpool;
    sha512_4way_context     sha512;
} __attribute__ ((aligned (64)));
typedef union _x16r_4way_context_overlay x16r_4way_context_overlay;

void x16r_4way_hash( void* output, const void* input )
{
   uint32_t vhash[24*4] __attribute__ ((aligned (128)));
   uint32_t hash0[24] __attribute__ ((aligned (64)));
   uint32_t hash1[24] __attribute__ ((aligned (64)));
   uint32_t hash2[24] __attribute__ ((aligned (64)));
   uint32_t hash3[24] __attribute__ ((aligned (64)));
   x16r_4way_context_overlay ctx;
   void *in0 = (void*) hash0;
   void *in1 = (void*) hash1;
   void *in2 = (void*) hash2;
   void *in3 = (void*) hash3;
   int size = 80;

   dintrlv_4x64( hash0, hash1, hash2, hash3, input, 640 );

   for ( int i = 0; i < 16; i++ )
   {
      const char elem = hashOrder[i];
      const uint8_t algo = elem >= 'A' ? elem - 'A' + 10 : elem - '0';

      switch ( algo )
      {
         case BLAKE:
            if ( i == 0 )
               blake512_4way_full( &ctx.blake, vhash, input, size );
            else
            {
               intrlv_4x64( vhash, in0, in1, in2, in3, size<<3 );
               blake512_4way_full( &ctx.blake, vhash, vhash, size );
            }
            dintrlv_4x64_512( hash0, hash1, hash2, hash3, vhash );
         break;
         case BMW:
            bmw512_4way_init( &ctx.bmw );
            if ( i == 0 )
               bmw512_4way_update( &ctx.bmw, input, size );
            else
            {
               intrlv_4x64( vhash, in0, in1, in2, in3, size<<3 );
               bmw512_4way_update( &ctx.bmw, vhash, size );
            }
            bmw512_4way_close( &ctx.bmw, vhash );
            dintrlv_4x64_512( hash0, hash1, hash2, hash3, vhash );
         break;
         case GROESTL:
            groestl512_full( &ctx.groestl, (char*)hash0, (char*)in0, size<<3 );
            groestl512_full( &ctx.groestl, (char*)hash1, (char*)in1, size<<3 );
            groestl512_full( &ctx.groestl, (char*)hash2, (char*)in2, size<<3 );
            groestl512_full( &ctx.groestl, (char*)hash3, (char*)in3, size<<3 );
         break;
         case SKEIN:
            skein512_4way_init( &ctx.skein );
            if ( i == 0 )
               skein512_4way_update( &ctx.skein, input, size );
            else
            {
               intrlv_4x64( vhash, in0, in1, in2, in3, size<<3 );
               skein512_4way_update( &ctx.skein, vhash, size );
            }
            skein512_4way_close( &ctx.skein, vhash );
            dintrlv_4x64_512( hash0, hash1, hash2, hash3, vhash );
         break;
         case JH:
            jh512_4way_init( &ctx.jh );
            if ( i == 0 )
               jh512_4way_update( &ctx.jh, input, size );
            else
            {
               intrlv_4x64( vhash, in0, in1, in2, in3, size<<3 );
               jh512_4way_update( &ctx.jh, vhash, size );
            }
            jh512_4way_close( &ctx.jh, vhash );
            dintrlv_4x64_512( hash0, hash1, hash2, hash3, vhash );
         break;
         case KECCAK:
            keccak512_4way_init( &ctx.keccak );
            if ( i == 0 )
               keccak512_4way_update( &ctx.keccak, input, size );
            else
            {
               intrlv_4x64( vhash, in0, in1, in2, in3, size<<3 );
               keccak512_4way_update( &ctx.keccak, vhash, size );
            }
            keccak512_4way_close( &ctx.keccak, vhash );
            dintrlv_4x64_512( hash0, hash1, hash2, hash3, vhash );
         break;
         case LUFFA:
            intrlv_2x128( vhash, in0, in1, size<<3 );
            luffa512_2way_full( &ctx.luffa, vhash, vhash, size );
            dintrlv_2x128_512( hash0, hash1, vhash );
            intrlv_2x128( vhash, in2, in3, size<<3 );
            luffa512_2way_full( &ctx.luffa, vhash, vhash, size );
            dintrlv_2x128_512( hash2, hash3, vhash );
         break;
         case CUBEHASH:
            cubehashInit( &ctx.cube, 512, 16, 32 );
            cubehashUpdateDigest( &ctx.cube, (byte*) hash0,
                                  (const byte*)in0, size );
            cubehashInit( &ctx.cube, 512, 16, 32 );
            cubehashUpdateDigest( &ctx.cube, (byte*) hash1,
                                  (const byte*)in1, size );
            cubehashInit( &ctx.cube, 512, 16, 32 );
            cubehashUpdateDigest( &ctx.cube, (byte*) hash2,
                                  (const byte*)in2, size );
            cubehashInit( &ctx.cube, 512, 16, 32 );
            cubehashUpdateDigest( &ctx.cube, (byte*) hash3,
                                  (const byte*)in3, size );
         break;
         case SHAVITE:
            sph_shavite512_init( &ctx.shavite );
            sph_shavite512( &ctx.shavite, in0, size );
            sph_shavite512_close( &ctx.shavite, hash0 );
            sph_shavite512_init( &ctx.shavite );
            sph_shavite512( &ctx.shavite, in1, size );
            sph_shavite512_close( &ctx.shavite, hash1 );
            sph_shavite512_init( &ctx.shavite );
            sph_shavite512( &ctx.shavite, in2, size );
            sph_shavite512_close( &ctx.shavite, hash2 );
            sph_shavite512_init( &ctx.shavite );
            sph_shavite512( &ctx.shavite, in3, size );
            sph_shavite512_close( &ctx.shavite, hash3 );
         break;
         case SIMD:
            intrlv_2x128( vhash, in0, in1, size<<3 );
            simd512_2way_full( &ctx.simd, vhash, vhash, size );
            dintrlv_2x128_512( hash0, hash1, vhash );
            intrlv_2x128( vhash, in2, in3, size<<3 );
            simd512_2way_full( &ctx.simd, vhash, vhash, size );
            dintrlv_2x128_512( hash2, hash3, vhash );
         break;
         case ECHO:
            echo_full( &ctx.echo, (BitSequence *)hash0, 512,
                              (const BitSequence *)in0, size );
            echo_full( &ctx.echo, (BitSequence *)hash1, 512,
                              (const BitSequence *)in1, size );
            echo_full( &ctx.echo, (BitSequence *)hash2, 512,
                              (const BitSequence *)in2, size );
            echo_full( &ctx.echo, (BitSequence *)hash3, 512,
                              (const BitSequence *)in3, size );
         break;
         case HAMSI:
             intrlv_4x64( vhash, in0, in1, in2, in3, size<<3 );
             hamsi512_4way_init( &ctx.hamsi );
             hamsi512_4way_update( &ctx.hamsi, vhash, size );
             hamsi512_4way_close( &ctx.hamsi, vhash );
             dintrlv_4x64_512( hash0, hash1, hash2, hash3, vhash );
         break;
         case FUGUE:
             sph_fugue512_init( &ctx.fugue );
             sph_fugue512( &ctx.fugue, in0, size );
             sph_fugue512_close( &ctx.fugue, hash0 );
             sph_fugue512_init( &ctx.fugue );
             sph_fugue512( &ctx.fugue, in1, size );
             sph_fugue512_close( &ctx.fugue, hash1 );
             sph_fugue512_init( &ctx.fugue );
             sph_fugue512( &ctx.fugue, in2, size );
             sph_fugue512_close( &ctx.fugue, hash2 );
             sph_fugue512_init( &ctx.fugue );
             sph_fugue512( &ctx.fugue, in3, size );
             sph_fugue512_close( &ctx.fugue, hash3 );
         break;
         case SHABAL:
             intrlv_4x32( vhash, in0, in1, in2, in3, size<<3 );
             shabal512_4way_init( &ctx.shabal );
             shabal512_4way_update( &ctx.shabal, vhash, size );
             shabal512_4way_close( &ctx.shabal, vhash );
             dintrlv_4x32_512( hash0, hash1, hash2, hash3, vhash );
         break;
         case WHIRLPOOL:
             sph_whirlpool_init( &ctx.whirlpool );
             sph_whirlpool( &ctx.whirlpool, in0, size );
             sph_whirlpool_close( &ctx.whirlpool, hash0 );
             sph_whirlpool_init( &ctx.whirlpool );
             sph_whirlpool( &ctx.whirlpool, in1, size );
             sph_whirlpool_close( &ctx.whirlpool, hash1 );
             sph_whirlpool_init( &ctx.whirlpool );
             sph_whirlpool( &ctx.whirlpool, in2, size );
             sph_whirlpool_close( &ctx.whirlpool, hash2 );
             sph_whirlpool_init( &ctx.whirlpool );
             sph_whirlpool( &ctx.whirlpool, in3, size );
             sph_whirlpool_close( &ctx.whirlpool, hash3 );
         break;
         case SHA_512:
             intrlv_4x64( vhash, in0, in1, in2, in3, size<<3 );
             sha512_4way_init( &ctx.sha512 );
             sha512_4way_update( &ctx.sha512, vhash, size );
             sha512_4way_close( &ctx.sha512, vhash );
             dintrlv_4x64_512( hash0, hash1, hash2, hash3, vhash );
         break;
      }
      size = 64;
   }
   memcpy( output,    hash0, 32 );
   memcpy( output+32, hash1, 32 );
   memcpy( output+64, hash2, 32 );
   memcpy( output+96, hash3, 32 );
}

int scanhash_x16r_4way( struct work *work, uint32_t max_nonce,
                        uint64_t *hashes_done, struct thr_info *mythr)
{
   uint32_t hash[4*16] __attribute__ ((aligned (64)));
   uint32_t vdata[24*4] __attribute__ ((aligned (64)));
   uint32_t bedata1[2] __attribute__((aligned(64)));
   uint32_t *pdata = work->data;
   uint32_t *ptarget = work->target;
   const uint32_t Htarg = ptarget[7];
   const uint32_t first_nonce = pdata[19];
   const uint32_t last_nonce = max_nonce - 4;
   uint32_t n = first_nonce;
    __m256i  *noncev = (__m256i*)vdata + 9;   // aligned
   int thr_id = mythr->id;
   volatile uint8_t *restart = &(work_restart[thr_id].restart);

   if ( opt_benchmark )
      ptarget[7] = 0x0cff;

   mm256_bswap32_intrlv80_4x64( vdata, pdata );

   bedata1[0] = bswap_32( pdata[1] );
   bedata1[1] = bswap_32( pdata[2] );
   const uint32_t ntime = bswap_32( pdata[17] );
   if ( s_ntime != ntime )
   {
      x16_r_s_getAlgoString( (const uint8_t*)bedata1, hashOrder );
      s_ntime = ntime;
      if ( opt_debug && !thr_id )
              applog( LOG_INFO, "hash order %s (%08x)", hashOrder, ntime );
   }

   do
   {
      *noncev = mm256_intrlv_blend_32( mm256_bswap_32(
               _mm256_set_epi32( n+3, 0, n+2, 0, n+1, 0, n, 0 ) ), *noncev );

      x16r_4way_hash( hash, vdata );
      pdata[19] = n;

      for ( int i = 0; i < 4; i++ )
      if ( unlikely( (hash+(i<<3))[7] <= Htarg ) )
      if( likely( fulltest( hash+(i<<3), ptarget ) && !opt_benchmark ) )
      {
         pdata[19] = n+i;
         submit_lane_solution( work, hash+(i<<3), mythr, i );
      }
      n += 4;
   } while ( likely( ( n < last_nonce ) && !(*restart) ) );

   *hashes_done = n - first_nonce;
   return 0;
}

#endif
