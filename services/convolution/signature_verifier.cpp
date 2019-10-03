#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <string.h>
#include "signature_verifier.h"
#include "misc.h"


RSA* GPublicRSA = nullptr;
EVP_PKEY* GPublicKey = nullptr;


static RSA* CreatePublicRSA() 
{
	uint32_t pubKeyFileSize = 0;
	void* pubKey = ReadFile("pubkey.pem", pubKeyFileSize);

	BIO* keybio = BIO_new_mem_buf(pubKey, -1);
	if(!keybio) 
	{
		free(pubKey);
		return nullptr;
	}

	RSA* rsa = PEM_read_bio_RSA_PUBKEY(keybio, nullptr, NULL, NULL);
	BIO_free(keybio);
	free(pubKey);
	return rsa;
}


static void* Base64Decode(const char* input, uint32_t& decodedLength)
{
	uint32_t inputLen = strlen(input), padding = 0;
	if (input[inputLen - 1] == '=' && input[inputLen - 2] == '=') //last two chars are =
		padding = 2;
	else if (input[inputLen - 1] == '=') //last char is =
		padding = 1;
	decodedLength = (inputLen * 3) / 4 - padding;

	void* ret = malloc(decodedLength);

    BIO* bioMem = BIO_new_mem_buf((void*)input, -1);
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bioMem = BIO_push(b64, bioMem);
    uint32_t test = BIO_read(bioMem, ret, inputLen);
	if(decodedLength != test)
	{
		printf("!!!!!!!!!!!\n");
		exit(1);
	}
    BIO_free_all(bioMem);

	return ret;
}


bool InitSignatureVerifier()
{
    GPublicRSA = CreatePublicRSA();
    if(!GPublicRSA)
        return false;

    GPublicKey = EVP_PKEY_new();
    if(!GPublicKey)
        return false;
    
	EVP_PKEY_assign_RSA(GPublicKey, GPublicRSA);

    return true;
}


void ShutdownSignatureVerifier()
{
	EVP_PKEY_free(GPublicKey);
}


bool VerifySignature(const char* plainText, const char* signatureBase64)
{
	uint32_t signatureLen = 0;
	void* signature = Base64Decode(signatureBase64, signatureLen);

    EVP_MD_CTX* mdCtx = EVP_MD_CTX_create();

	if(EVP_DigestVerifyInit(mdCtx, NULL, EVP_sha256(), NULL, GPublicKey) <= 0) 
	{
		EVP_MD_CTX_free(mdCtx);
		return false;
	}

	if(EVP_DigestVerifyUpdate(mdCtx, plainText, strlen(plainText)) <= 0) 
	{
		EVP_MD_CTX_free(mdCtx);
		return false;
	}

	bool result = EVP_DigestVerifyFinal(mdCtx, (const unsigned char*)signature, signatureLen) == 1;
	
    EVP_MD_CTX_free(mdCtx);
	free(signature);

	return result;
}