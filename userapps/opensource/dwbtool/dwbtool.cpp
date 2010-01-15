/*
	Firmware tool for Livebox Inventel
	by Rafael Vuijk (aka DarkFader)

	History
		1.00 - initial version
		1.01 - added DSA

	Build
		release
			g++ -Wall -O3 -s -o dwbtool dwbtool.cpp -ltomcrypt -ltommath -DLTM_DESC -L crypto -I crypto
		verify test
			dwbtool -v Firmware_v5.05.5-fr.dwb pubkey_inventel_bootloader_only
		extract test
			dwbtool -x Firmware_v5.05.5-fr.dwb Firmware_v5.05.5-fr.script Firmware_v5.05.5-fr.cramfs
		create test
			dwbtool -c Firmware_v5.05.5-fr.dwb2 Firmware_v5.05.5-fr.script Firmware_v5.05.5-fr.cramfs
		verify after create
			dwbtool -v Firmware_v5.05.5-fr.dwb2 mykey
*/

#include <stdio.h>
#include <string.h>

#define VER				"1.01"
#define DEBUG			if (0)

#include "crypto.h"

#define MAGIC			0x1885DAAB
#define MAX_KEY_SIZE	0x1000
#define MAX_DWB_SIZE	8*0x100000
#define HASH_SIZE		64				// SHA-512
#define SIG_ALIGN		0x1000
#define ALIGN4(x)		(((x) + 3) &~ 3)
#define ALIGNSIG(x)		(((x) + (SIG_ALIGN-1)) &~ (SIG_ALIGN-1))

/*
 * globals
 */
unsigned char *dwb_buffer;
unsigned int dwb_size;
prng_state yarrow_prng_state;

/*
 * PrngInit
 */
int PrngInit()
{
	static bool already = false;
	if (already) return 0; else already = true;
	register_prng(&yarrow_desc);
	int res;
	if ((res = rng_make_prng(128, find_prng("yarrow"), &yarrow_prng_state, NULL))) { fprintf(stderr, "rng_make_prng failed: %s\n", error_to_string(res)); return res; }
	return 0;
}

/*
 * GenerateKey
 */
int GenerateKey(dsa_key *key)
{
	int res;
	if ((res = PrngInit())) return res;
	if ((res = dsa_make_key(&yarrow_prng_state, find_prng("yarrow"), 20, 128, key))) { fprintf(stderr, "dsa_make_key error\n"); return res; }
	return 0;
}

/*
 * ImportKey
 */
int ImportKey(const char *filename, bool generate, dsa_key *key)
{
	int res;
	FILE *f1 = fopen(filename, "rb");
	if (f1)
	{
		unsigned char buf_key[MAX_KEY_SIZE];
		int keylen = fread(buf_key, 1, sizeof(buf_key), f1);
		fclose(f1);
		if ((res = dsa_import2(buf_key, keylen, key))) { printf("dsa_import2 error %d\n", res); return res; }
	}
	else if (generate)
	{
		if ((res = GenerateKey(key))) return res;

		// export...
		
		unsigned char buf_key[MAX_KEY_SIZE];
		unsigned long outlen;
		int res;
		if ((res = dsa_export2(buf_key, &outlen, key))) { printf("dsa_export2 error %d\n", res); return res; }

		FILE *f1 = fopen(filename, "wb");
		fwrite(buf_key, 1, outlen, f1);
		fclose(f1);
		
		printf("Generated key written to file '%s'.\n", filename);
	}
	return 0;
}

/*
 * ReadDwb
 */
int ReadDwb(char *filename)
{
	FILE *f = fopen(filename, "rb");
	if (!f) { fprintf(stderr, "Could not open input file '%s'.\n", filename); return 1; }
	dwb_size = fread(dwb_buffer, 1, MAX_DWB_SIZE, f);
	fclose(f);
	DEBUG printf("dwb size = %X\n", dwb_size);
	return 0;
}

/*
 * WriteDwb
 */
int WriteDwb(char *filename)
{
	FILE *f = fopen(filename, "wb");
	if (!f) { fprintf(stderr, "Could not open output file '%s'.\n", filename); return 1; }
	fwrite(dwb_buffer, 1, dwb_size, f);
	fclose(f);
	DEBUG printf("dwb size = %X\n", dwb_size);
	return 0;
}

/*
 * XorWithSignature
 */
int XorWithSignature()
{
	unsigned int tag_offset = dwb_size & ~(SIG_ALIGN - 1);
	DEBUG printf("tag offset = %X\n", tag_offset);
	unsigned char *begin_tag = dwb_buffer + tag_offset;
	unsigned int siglen = GetBigInt(begin_tag);
	if (GetBigInt(begin_tag + 4 + ALIGN4(siglen)) != MAGIC) { fprintf(stderr, "Could not find magic.\n"); return 1; }
	unsigned int i = 0;
	unsigned char *sig = begin_tag + 4;
	for (unsigned char *p = dwb_buffer; p < begin_tag; p++)
	{
		*p ^= sig[i];
		if (++i >= siglen) i = 0;
	}
	return 0;
}

/*
 * PopSignature
 * decreases dwb_size
 */
int PopSignature(mp_int **r, mp_int **s)
{
	unsigned int tag_offset = (dwb_size - 1) & ~(SIG_ALIGN - 1);
	DEBUG printf("tag_offset = %X\n", tag_offset);
	unsigned int p = tag_offset;
	int res;
	if ((res = ltc_init_multi((void **)r, (void **)s, NULL))) return res;
	unsigned int siglen = GetBigInt(dwb_buffer + p); p += 4;
	DEBUG printf("siglen = %d\n", siglen);
	unsigned int flags = GetBigInt(dwb_buffer + p); p += 4;	// version etc
	DEBUG printf("flags = %08X\n", flags);
	unsigned int r_len = (unsigned int)dwb_buffer[p]<<8 | dwb_buffer[p+1]; p += 2;
	if ((res = ltc_mp.unsigned_read(*r, dwb_buffer + p, r_len))) return res;
	DEBUG printf("%02X %02X ...\n", dwb_buffer[0], dwb_buffer[p+1]);
	p += r_len;
	unsigned int s_len = (unsigned int)dwb_buffer[p]<<8 | dwb_buffer[p+1]; p += 2;
	if ((res = ltc_mp.unsigned_read(*s, dwb_buffer + p, s_len))) return res;
	DEBUG printf("%02X %02X ...\n", dwb_buffer[p], dwb_buffer[p+1]);
	p += s_len;
	p = ALIGN4(p);
	unsigned int magic = GetBigInt(dwb_buffer + p); p += 4;	// magic
	if (magic != MAGIC) { fprintf(stderr, "magic number not found\n"); return 1; }
	DEBUG printf("r %d s %d\n", r_len, s_len);
	dwb_size = tag_offset;
	DEBUG printf("new dwb_size = %X\n", dwb_size);
	return 0;
}

/*
 * PushSignature
 * dwb_size should already have been aligned
 * increases dwb_size
 */
int PushSignature(mp_int *r, mp_int *s)
{
	unsigned int tag_offset = dwb_size;
	unsigned int r_len = ltc_mp.unsigned_size(r);
	unsigned int s_len = ltc_mp.unsigned_size(s);
	DEBUG printf("siglen = %d\n", 4+2+r_len+2+s_len);
	unsigned int p = tag_offset;
	PutBigInt(dwb_buffer + p, 4+2+r_len+2+s_len); p += 4;
	DEBUG printf("flags = %08X\n", 0x94000302);
	PutBigInt(dwb_buffer + p, 0x94000302); p += 4;	// flags
	dwb_buffer[p++] = r_len>>8; dwb_buffer[p++] = r_len;
	int res;
	if ((res = ltc_mp.unsigned_write(r, dwb_buffer + p))) return res;
	DEBUG printf("%02X %02X ...\n", dwb_buffer[p], dwb_buffer[p+1]);
	p += r_len;
	dwb_buffer[p++] = s_len>>8; dwb_buffer[p++] = s_len;
	if ((res = ltc_mp.unsigned_write(s, dwb_buffer + p))) return res;
	DEBUG printf("%02X %02X ...\n", dwb_buffer[p], dwb_buffer[p+1]);
	p += s_len;
	p = ALIGN4(p);
	DEBUG printf("r %d s %d\n", r_len, s_len);
	PutBigInt(dwb_buffer + p, MAGIC); p += 4;	// magic
	dwb_size = p;		// now at the end
	return 0;
}

/*
 * CalculateHash
 */
int CalculateHash(unsigned begin_offset, unsigned end_offset, unsigned char *hash_buffer)
{
	DEBUG printf("hashing %X bytes\n", end_offset - begin_offset);
	hash_state md;
	sha512_init(&md);
	sha512_process(&md, dwb_buffer + begin_offset, end_offset - begin_offset);
	sha512_done(&md, hash_buffer);
	DEBUG { for (int i=0; i<HASH_SIZE; i++) printf("%02X", hash_buffer[i]); printf("\n"); }
	return 0;
}

/*
 * main
 */
int main(int argc, char *argv[])
{
	printf("dwbtool v"VER" by Rafael Vuijk (aka DarkFader)\n");

	// use LibTomMath
	ltc_mp = ltm_desc;

	// 
	dwb_buffer = new unsigned char[MAX_DWB_SIZE];

	// -?
	if (argc >= 2) if (argv[1][0] == '-') switch (argv[1][1])
	{
		case 'v':	// verify dwb
		{
			char *argv_dwb = argv[2];
			const char *argv_key1 = (argc >= 4) ? argv[3] : "mykey";
			//const char *argv_key2 = (argc >= 5) ? argv[4] : argv_key1;
//printf("%s %s\n", argv_key1, argv_key2);

			int res;
			dsa_key key;
			if ((res = ImportKey(argv_key1, false, &key))) return res;

			if ((res = ReadDwb(argv_dwb))) return res;
			if ((res = XorWithSignature())) return res;

			mp_int *r, *s;
			unsigned char hash_buffer[HASH_SIZE];
			int stat;

			if ((res = PopSignature(&r, &s))) return res;
			if ((res = CalculateHash(0, dwb_size, hash_buffer))) return res;
			if ((res = dsa_verify_hash_raw((void *&)r, (void *&)s, hash_buffer, HASH_SIZE, &stat, &key))) { printf("dsa_verify_hash_raw error %d\n", res); return res; }
			if (!stat) { printf("Signature 1 is incorrect!\n"); return 1; }

			if ((res = PopSignature(&r, &s))) return res;
			if ((res = CalculateHash(0x1000, dwb_size, hash_buffer))) return res;
			if ((res = dsa_verify_hash_raw((void *&)r, (void *&)s, hash_buffer, HASH_SIZE, &stat, &key))) { printf("dsa_verify_hash_raw error %d\n", res); return res; }
			if (!stat) { printf("Signature 2 is incorrect!\n"); return 1; }

			printf("Verification successful.\n");
			return 0;
		}

		case 'x':	// extract dwb/decrypt
		{
			char *argv_dwb = argv[2];
			char *argv_script = argv[3];
			char *argv_cramfs = argv[4];

			int res;
			if ((res = ReadDwb(argv_dwb))) return res;
			if ((res = XorWithSignature())) return res;
			mp_int *r, *s;
			if ((res = PopSignature(&r, &s))) return res;
			if ((res = PopSignature(&r, &s))) return res;

			// install script
			{
				FILE *fo = fopen(argv_script, "wb");
				if (!fo) { fprintf(stderr, "Could not open output file '%s'.\n", argv_script); return 1; }
				for (int i=0; i<0x1000; i++)
				{
					unsigned char c = dwb_buffer[i];
					if (c == 0) break;	// end of script
					fputc(c, fo);
				}
				fclose(fo);
			}

			// cramfs image
			{
				FILE *fo = fopen(argv_cramfs, "wb");
				if (!fo) { fprintf(stderr, "Could not open output file '%s'.\n", argv_cramfs); return 1; }
				fwrite(dwb_buffer+0x1000, 1, dwb_size-0x1000, fo);
				fclose(fo);
			}

			return 0;
		}

		case 'c':	// create dwb/encrypt
		{
			char *argv_dwb = argv[2];
			char *argv_script = argv[3];
			char *argv_cramfs = argv[4];
			
			memset(dwb_buffer, 0, MAX_DWB_SIZE);	// clear unused data

			// install script
			{
				FILE *fi = fopen(argv_script, "rb");
				if (!fi) { fprintf(stderr, "Could not open input file '%s'.\n", argv_script); return 1; }
				fread(dwb_buffer, 1, 0x1000, fi);
				fclose(fi);
			}
			
			// cramfs image
			{
				FILE *fi = fopen(argv_cramfs, "rb");
				if (!fi) { fprintf(stderr, "Could not open input file '%s'.\n", argv_cramfs); return 1; }
				dwb_size = 0x1000 + fread(dwb_buffer+0x1000, 1, MAX_DWB_SIZE-0x1000, fi);
				fclose(fi);
			}

			int res;
			dsa_key key;
			if ((res = ImportKey("mykey", true, &key))) return res;
			if ((res = PrngInit())) return res;		// for signing

			mp_int *r, *s;
			if ((res = ltc_init_multi((void **)&r, (void **)&s, NULL))) return res;
			unsigned char hash_buffer[HASH_SIZE];

			dwb_size = ALIGNSIG(dwb_size);
			DEBUG printf("dwb_size = %X before hashing\n", dwb_size);
			if ((res = CalculateHash(0x1000, dwb_size, hash_buffer))) return res;
			if ((res = dsa_sign_hash_raw(hash_buffer, HASH_SIZE, r, s, &yarrow_prng_state, find_prng("yarrow"), &key))) { fprintf(stderr, "dsa_sign_hash_raw error %d\n", res); return res; }
			if ((res = PushSignature(r, s))) return res;

			dwb_size = ALIGNSIG(dwb_size);
			DEBUG printf("dwb_size = %X before hashing\n", dwb_size);
			if ((res = CalculateHash(0, dwb_size, hash_buffer))) return res;
			if ((res = dsa_sign_hash_raw(hash_buffer, HASH_SIZE, r, s, &yarrow_prng_state, find_prng("yarrow"), &key))) { fprintf(stderr, "dsa_sign_hash_raw error %d\n", res); return res; }
			if ((res = PushSignature(r, s))) return res;

			if ((res = XorWithSignature())) return res;
			if ((res = WriteDwb(argv_dwb))) return res;

			return 0;
		}
		
		default:
		{
			fprintf(stderr, "Unknown command.\n"); return 1;
			break;
		}
	}

	printf("Verify:  dwbtool -v firmware.dwb [mykey]\n");
	printf("Extract: dwbtool -x firmware.dwb install_script cramfs_image\n");
	printf("Create:  dwbtool -c firmware.dwb install_script cramfs_image\n");
	printf("  Create uses the file 'mykey'. It is automatically generated if it doesn't exists.\n");

	return 0;
}
