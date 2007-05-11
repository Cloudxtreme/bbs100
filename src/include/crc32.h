/*
	crc32.h	WJ107
*/

#ifndef CRC32_H_WJ107
#define CRC32_H_WJ105	1

void gen_crc32_table(void);
unsigned long update_crc32(unsigned long, char *, int);

#endif	/* CRC32_H_WJ105 */

/* EOB */
