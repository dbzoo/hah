#include <tomcrypt.h>

unsigned int GetBigInt(unsigned char *p)
{
	unsigned int r = *p++ << 24;
	r |= *p++ << 16;
	r |= *p++ << 8;
	r |= *p++;
	return r;
}

void PutBigInt(unsigned char *p, unsigned int r)
{
	*p++ = r >> 24;
	*p++ = r >> 16;
	*p++ = r >> 8;
	*p++ = r;
}

// the infamous mp_int structure
typedef struct  {
    int used, alloc, sign;
    unsigned long *dp;	// mp_digit
} mp_int;

inline int INPUT_BIGNUM(mp_int *num, const unsigned char *in, unsigned long &y)
{
	unsigned long len;
	LOAD32L(len, in+y);
	y += 4;
	int res;
	if ((res = ltc_mp.unsigned_read(num, (unsigned char *)in+y, (int)len))) return res;
	y += len;
	return 0;
}

inline int OUTPUT_BIGNUM(mp_int *num, unsigned char *out, unsigned long &y)
{
	unsigned long len = (unsigned long)ltc_mp.unsigned_size(num);
	STORE32L(len, out+y);
	y += 4;
	int res;
	if ((res = ltc_mp.unsigned_write(num, out+y))) return res;
	y += len;
	return 0;
}

/*
 * dsa_import2
 */
int dsa_import2(const unsigned char *in, unsigned long inlen, dsa_key *key)
{
	int res;
	if ((res = ltc_init_multi(&key->p, &key->g, &key->q, &key->x, &key->y, NULL))) return res;
	
	unsigned long y = 4;
	key->type = in[y++];
	DEBUG printf("key type = %d\n", key->type);
	key->qord = ((unsigned int)in[y]<<8) | ((unsigned int)in[y+1]); y += 2;
	DEBUG printf("key qord = %d\n", key->qord);
	
	if ((res = INPUT_BIGNUM((mp_int *)key->g, in, y))) return res;
	if ((res = INPUT_BIGNUM((mp_int *)key->p, in, y))) return res;
	if ((res = INPUT_BIGNUM((mp_int *)key->q, in, y))) return res;
	if ((res = INPUT_BIGNUM((mp_int *)key->y, in, y))) return res;
	if (y < inlen)		// has private key
	{
		if ((res = INPUT_BIGNUM((mp_int *)key->x, in, y))) return res;
	}
	
	return 0;
}

/*
 * dsa_export2
 */
int dsa_export2(unsigned char *out, unsigned long *outlen, dsa_key *key)
{
	unsigned long y = 0;
	PutBigInt(out, 0x94000300); y += 4;
	out[y++] = key->type;		// whatever
	DEBUG printf("key type = %d\n", key->type);
	out[y] = key->qord >> 8; out[y+1] = key->qord; y += 2;
	DEBUG printf("key qord = %d\n", key->qord);
	
	int res;
	if ((res = OUTPUT_BIGNUM((mp_int *)key->g, out, y))) return res;
	if ((res = OUTPUT_BIGNUM((mp_int *)key->p, out, y))) return res;
	if ((res = OUTPUT_BIGNUM((mp_int *)key->q, out, y))) return res;
	if ((res = OUTPUT_BIGNUM((mp_int *)key->y, out, y))) return res;
	if ((res = OUTPUT_BIGNUM((mp_int *)key->x, out, y))) return res;

	*outlen = y;
	return 0;
}
