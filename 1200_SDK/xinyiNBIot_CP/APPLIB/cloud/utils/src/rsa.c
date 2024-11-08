#include "rsa.h"

long long rsa_modExp(long long b, long long e, long long m)
{
    if(b < 0 || e < 0 || m <= 0)
    {
        return 0;
    }
    
    b = b % m;
    
    if(e == 0)
    {
        return 1;
    }
    
    if(e == 1)
    {
        return b;
    }
    
    if(e % 2 == 0)
    {
        return (rsa_modExp(b * b % m, e/2, m) % m);
    }
    else
    {
        return (b * rsa_modExp(b, (e-1), m) % m);
    }
}

void rsa_public_encrypt(const char *message, const unsigned long message_size, const PUBLIC_KEY_Def *pub, long long *encrypted)
{
    unsigned long i = 0;
    
    for(i = 0; i < message_size; i++)
    {
        encrypted[i] = rsa_modExp(message[i], pub->e, pub->n);
    }
    
    return;
}

void rsa_public_decrypt(const long *message, const unsigned long message_size, const PUBLIC_KEY_Def *pub, char *decrypted)
{
    unsigned long i = 0;
    
    if(message_size % sizeof(long long) != 0)
    {
        return;
    }

    for(i = 0; i < message_size/4; i++)
    {
        decrypted[i] = rsa_modExp((unsigned long long)message[i], pub->e, pub->n);
    }
}

void rsa_private_decrypt(const long long *message, const unsigned long message_size, const PRIVATE_KEY_Def *priv, char *decrypted)
{
    unsigned long i = 0;
    
    if(message_size % sizeof(long long) != 0)
    {
        return;
    }

    for(i = 0; i < message_size/8; i++)
    {
        decrypted[i] = rsa_modExp(message[i], priv->d, priv->n);
    }
}

void rsa_private_encrypt(const char *message, const unsigned long message_size, const PRIVATE_KEY_Def *priv, long long *encrypted)
{
    unsigned long i = 0;
    
    for(i = 0; i < message_size; i++)
    {
        encrypted[i] = rsa_modExp(message[i], priv->d, priv->n);
    }
    
    return;
}
