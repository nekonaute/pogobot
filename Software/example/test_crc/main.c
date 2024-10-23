/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/

#include "pogobot.h"

#define		CRC_POLY_16		0xA001
#define		CRC_POLY_32		0xEDB88320ul
#define		CRC_POLY_64		0x42F0E1EBA9EA3693ull
#define		CRC_POLY_CCITT		0x1021
#define		CRC_POLY_DNP		0xA6BC
#define		CRC_POLY_KERMIT		0x8408
#define		CRC_POLY_SICK		0x8005
#define		CRC_START_8		0x00
#define		CRC_START_16		0x0000
#define		CRC_START_MODBUS	0xFFFF
#define		CRC_START_XMODEM	0x0000
#define		CRC_START_CCITT_1D0F	0x1D0F
#define		CRC_START_CCITT_FFFF	0xFFFF
#define		CRC_START_KERMIT	0x0000
#define		CRC_START_SICK		0x0000
#define		CRC_START_DNP		0x0000
#define		CRC_START_32		0xFFFFFFFFul
#define		CRC_START_64_ECMA	0x0000000000000000ull
#define		CRC_START_64_WE		0xFFFFFFFFFFFFFFFFull


static const uint16_t crc_table_16[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7, 0x8108,
    0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef, 0x1231, 0x0210,
    0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6, 0x9339, 0x8318, 0xb37b,
    0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de, 0x2462, 0x3443, 0x0420, 0x1401,
    0x64e6, 0x74c7, 0x44a4, 0x5485, 0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee,
    0xf5cf, 0xc5ac, 0xd58d, 0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6,
    0x5695, 0x46b4, 0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d,
    0xc7bc, 0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b, 0x5af5,
    0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12, 0xdbfd, 0xcbdc,
    0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a, 0x6ca6, 0x7c87, 0x4ce4,
    0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41, 0xedae, 0xfd8f, 0xcdec, 0xddcd,
    0xad2a, 0xbd0b, 0x8d68, 0x9d49, 0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13,
    0x2e32, 0x1e51, 0x0e70, 0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a,
    0x9f59, 0x8f78, 0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e,
    0xe16f, 0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e, 0x02b1,
    0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256, 0xb5ea, 0xa5cb,
    0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d, 0x34e2, 0x24c3, 0x14a0,
    0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 0xa7db, 0xb7fa, 0x8799, 0x97b8,
    0xe75f, 0xf77e, 0xc71d, 0xd73c, 0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657,
    0x7676, 0x4615, 0x5634, 0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9,
    0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882,
    0x28a3, 0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92, 0xfd2e,
    0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 0x7c26, 0x6c07,
    0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1, 0xef1f, 0xff3e, 0xcf5d,
    0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8, 0x6e17, 0x7e36, 0x4e55, 0x5e74,
    0x2e93, 0x3eb2, 0x0ed1, 0x1ef0 };

static uint32_t crc_table_32[256] = {
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
    0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
    0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
    0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
    0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
    0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
    0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
    0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
    0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
    0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
    0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
    0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
    0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
    0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
    0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
    0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
    0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
    0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
    0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
    0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
    0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
    0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
    0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
    0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
    0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
    0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
    0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
    0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
    0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
    0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
    0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
    0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
    0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
    0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
    0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
    0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
    0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
    0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
    0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
    0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
    0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
    0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
    0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
    0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
    0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
    0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
    0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
    0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
    0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
    0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
    0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
    0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
    0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
    0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
    0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
    0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
    0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
    0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
    0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
    0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4,
  };


uint16_t crc_tab16[256];
uint32_t crc_tab32[256];
uint64_t crc_tab64[256];
uint16_t crc_tabccitt[256];


static uint16_t
calc_crc_ccitt_16( uint8_t byte, uint16_t crc_old ) {
    uint8_t index;
    uint16_t crc;

    index = ( uint8_t )( byte ^ ( crc_old >> 8 ) );
    crc = ( uint16_t )( crc_table_16[index] ^ ( crc_old << 8 ) );

    return crc;
}

static void init_crc16_tab( void ) {

	uint16_t i;
	uint16_t j;
	uint16_t crc;
	uint16_t c;

	for (i=0; i<256; i++) {

		crc = 0;
		c   = i;

		for (j=0; j<8; j++) {

			if ( (crc ^ c) & 0x0001 ) crc = ( crc >> 1 ) ^ CRC_POLY_16;
			else                      crc =   crc >> 1;

			c = c >> 1;
		}

		crc_tab16[i] = crc;
	}

}  /* init_crc16_tab */

static uint16_t calc_crc_16( uint8_t byte, uint16_t crc_old ) {
    uint16_t crc;

		crc = (crc_old >> 8) ^ crc_tab16[ (crc_old ^ (uint16_t) byte) & 0x00FF ];

	return crc;

}  /* crc_16 */

static void init_crcccitt_tab( void ) {

	uint16_t i;
	uint16_t j;
	uint16_t crc;
	uint16_t c;

	for (i=0; i<256; i++) {

		crc = 0;
		c   = i << 8;

		for (j=0; j<8; j++) {

			if ( (crc ^ c) & 0x8000 ) crc = ( crc << 1 ) ^ CRC_POLY_CCITT;
			else                      crc =   crc << 1;

			c = c << 1;
		}

		crc_tabccitt[i] = crc;
	}

}  /* init_crcccitt_tab */

static uint16_t calc_crc_ccitt_16_lib ( uint8_t byte, uint16_t crc_old ) {

    return (crc_old << 8) ^ crc_tabccitt[ ((crc_old >> 8) ^ (uint16_t) byte) & 0x00FF ];
} /* calc_crc_ccitt_16_lib */


static void init_crc32_tab( void ) {

	uint32_t i;
	uint32_t j;
	uint32_t crc;

	for (i=0; i<256; i++) {

		crc = i;

		for (j=0; j<8; j++) {

			if ( crc & 0x00000001L ) crc = ( crc >> 1 ) ^ CRC_POLY_32;
			else                     crc =   crc >> 1;
		}

		crc_tab32[i] = crc;
	}

}  /* init_crc32_tab */

static uint32_t calc_crc_32( uint8_t byte, uint32_t crc_old ) {

	uint32_t crc;

	crc = (crc_old >> 8) ^ crc_tab32[ (crc_old ^ (uint32_t) byte) & 0x000000FFul ];

	return (crc ^ 0xFFFFFFFFul);

}  /* crc_32 */


static void init_crc64_tab( void ) {

	uint64_t i;
	uint64_t j;
	uint64_t c;
	uint64_t crc;

	for (i=0; i<256; i++) {

		crc = 0;
		c   = i << 56;

		for (j=0; j<8; j++) {

			if ( ( crc ^ c ) & 0x8000000000000000ull ) crc = ( crc << 1 ) ^ CRC_POLY_64;
			else                                       crc =   crc << 1;

			c = c << 1;
		}

		crc_tab64[i] = crc;
	}

}  /* init_crc64_tab */

static uint64_t calc_crc_64_ecma ( uint8_t byte, uint64_t crc_old ){

	uint64_t crc;

    crc = (crc_old << 8) ^ crc_tab64[ ((crc_old >> 56) ^ (uint64_t) byte) & 0x00000000000000FFull ];

	return crc;

}  /* crc_64_ecma */


#define poly 0x1021
static uint16_t update_good_crc ( uint8_t byte, uint16_t crc_old )
{
    unsigned short i, v, xor_flag;
    uint16_t good_crc = crc_old;

    /*
    Align test bit with leftmost bit of the message byte.
    */
    v = 0x80;

    for (i=0; i<8; i++)
    {
        if (good_crc & 0x8000)
        {
            xor_flag= 1;
        }
        else
        {
            xor_flag= 0;
        }
        good_crc = good_crc << 1;

        if (byte & v)
        {
            /*
            Append next bit of message to end of CRC if it is not zero.
            The zero bit placed there by the shift above need not be
            changed if the next bit of the message is zero.
            */
            good_crc= good_crc + 1;
        }

        if (xor_flag)
        {
            good_crc = good_crc ^ poly;
        }

        /*
        Align test bit with next bit of the message byte.
        */
        v = v >> 1;
    }

    return good_crc;
} 

static uint16_t calc_crc_ccitt_16_test (uint8_t byte, uint16_t crc_old) {
    uint16_t crc = crc_old ^ ((uint16_t) byte << 8);
        
    for (unsigned char j = 0; j < 8; ++j) {
        uint16_t mix = crc & 0x8000;
        crc = (crc << 1);
        if (mix)
            crc = crc ^ 0x1021;
    }

    return crc;
}

static uint32_t calc_crc_ccitt32_updcrc (uint8_t byte, uint32_t crc_old) {

    return ((crc_old<<8)&0xffffff00)^crc_table_32[((crc_old>>24)&0xff)^byte];

}

#define NB_TIME 1000

int main(void) {

    pogobot_init();
    printf("init ok\n");

    printf("TEST CRC for %d iterations \n", NB_TIME);

    uint16_t crc_16_itt = CRC_START_CCITT_FFFF;
    uint16_t crc_16_itt_lib = CRC_START_CCITT_FFFF;
    uint16_t crc_16_itt_good = CRC_START_CCITT_FFFF;
    uint16_t crc_16_itt_test = CRC_START_CCITT_FFFF;
    uint16_t crc_16 = CRC_START_16;
    uint32_t crc_32 = CRC_START_32;
    uint32_t crc_32_itt = CRC_START_32;
    uint64_t crc_64 = CRC_START_64_ECMA;
    uint8_t input_byte = 0;
    int32_t time =0;

    init_crc16_tab();
    init_crcccitt_tab();
    init_crc32_tab();
    init_crc64_tab();

    printf(" sizeof uint16_t %d\n", sizeof(uint16_t));
    printf(" sizeof uint32_t %d\n", sizeof(uint32_t));
    printf(" sizeof uint64_t %d\n", sizeof(uint64_t));

    /*for (size_t i = 0; i < 256; i++)
    {
        printf("0x%x,", crc_tabccitt[i]);
    }
    printf("\n");*/

    if (NB_TIME < 20)
    {
        for (size_t i = 0; i < NB_TIME; i++)
        {
            input_byte ++;
            printf("%d ", input_byte);
        }
        input_byte = 0;
    }
    
    

    time_reference_t timer;
    pogobot_stopwatch_reset(&timer);

    for (size_t i = 0; i < NB_TIME; i++)
    {
        input_byte ++;
        crc_16_itt = calc_crc_ccitt_16(input_byte, crc_16_itt);
    }
    
    time = pogobot_stopwatch_get_elapsed_microseconds(&timer);
    printf("%ld us for CRCITT 16 (%d)\n", time, crc_16_itt);

    input_byte = 0;
    pogobot_stopwatch_reset(&timer);

    for (size_t i = 0; i < NB_TIME; i++)
    {
        input_byte ++;
        crc_16 = calc_crc_16(input_byte, crc_16);
    }
    
    time = pogobot_stopwatch_get_elapsed_microseconds(&timer);
    printf("%ld us for CRC16 (%d)\n", time, crc_16);

    input_byte = 0;
    pogobot_stopwatch_reset(&timer);

    for (size_t i = 0; i < NB_TIME; i++)
    {
        input_byte ++;
        crc_32 = calc_crc_32(input_byte, crc_32);
    }
    
    time = pogobot_stopwatch_get_elapsed_microseconds(&timer);
    printf("%ld us for CRC32 (%ld)\n", time, crc_32);

    input_byte = 0;
    pogobot_stopwatch_reset(&timer);

    for (size_t i = 0; i < NB_TIME; i++)
    {
        input_byte ++;
        crc_64 = calc_crc_64_ecma(input_byte, crc_64);
    }
    
    time = pogobot_stopwatch_get_elapsed_microseconds(&timer);
    printf("%ld us for CRC64 (%lld)\n", time, crc_64);

    input_byte = 0;
    pogobot_stopwatch_reset(&timer);

    for (size_t i = 0; i < NB_TIME; i++)
    {
        input_byte ++;
        crc_16_itt_lib = calc_crc_ccitt_16_lib(input_byte, crc_16_itt_lib);
    }
    
    time = pogobot_stopwatch_get_elapsed_microseconds(&timer);
    printf("%ld us for CRCITT 16 LIB (%d)\n", time, crc_16_itt_lib);

    input_byte = 0;
    pogobot_stopwatch_reset(&timer);

    for (size_t i = 0; i < NB_TIME; i++)
    {
        input_byte ++;
        crc_16_itt_good = update_good_crc(input_byte, crc_16_itt_good);
    }
    
    time = pogobot_stopwatch_get_elapsed_microseconds(&timer);
    printf("%ld us for CRCITT 16 GOOD (%d)\n", time, crc_16_itt_good);

    input_byte = 0;
    pogobot_stopwatch_reset(&timer);

    for (size_t i = 0; i < NB_TIME; i++)
    {
        input_byte ++;
        crc_16_itt_test = calc_crc_ccitt_16_test (input_byte, crc_16_itt_test);
    }
    
    time = pogobot_stopwatch_get_elapsed_microseconds(&timer);
    printf("%ld us for CRCITT 16 TEST (%d)\n", time, crc_16_itt_test);

    input_byte = 0;
    pogobot_stopwatch_reset(&timer);

    for (size_t i = 0; i < NB_TIME; i++)
    {
        input_byte ++;
        crc_32_itt = calc_crc_ccitt32_updcrc (input_byte, crc_32_itt);
    }
    
    time = pogobot_stopwatch_get_elapsed_microseconds(&timer);
    printf("%ld us for CRCITT 32 (%ld)\n", time, crc_32_itt);

    input_byte = 0;
    pogobot_stopwatch_reset(&timer);

}
