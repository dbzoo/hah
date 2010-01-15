#include <openssl/ssl.h>
#include <openssl/opensslv.h>

int main()
{
    long ver;

    /* check openssl version */
    SSL_SESSION_new();

    if (OPENSSL_VERSION_NUMBER < 0x00907000L) {
        printf("OpenSSL version 0.9.7 or better is required!\n");
        return 1;
    }

    /* test linking to crypto */
    ver = SSLeay();
    
    return 0;
}
