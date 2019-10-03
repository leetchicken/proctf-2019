#pragma once


bool InitSignatureVerifier();
void ShutdownSignatureVerifier();
bool VerifySignature(const char* plainText, const char* signatureBase64);