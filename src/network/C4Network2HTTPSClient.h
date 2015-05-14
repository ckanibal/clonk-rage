#pragma once

#include "C4Network2HTTPClient.h"
#include "C4NetIO.h"

#define POLARSSL_CONFIG_FILE <config-suite-b.h>

#if !defined(POLARSSL_CONFIG_FILE)
#include <polarssl/config.h>
#else
#include POLARSSL_CONFIG_FILE
#endif

#include "polarssl/net.h"
#include "polarssl/debug.h"
#include "polarssl/ssl.h"
#include "polarssl/entropy.h"
#include "polarssl/ctr_drbg.h"
#include "polarssl/error.h"
#include "polarssl/certs.h"

#include <iostream>

// mini HTTPS client
class C4Network2HTTPSClient : public C4NetIO {
public:
	C4Network2HTTPSClient();

//	virtual bool Execute(int iMaxTime = TO_INF);

	virtual ~C4Network2HTTPSClient();
protected:
	// Overridden
	virtual bool Send(const C4NetIOPacket &rPacket);
	virtual bool Connect(const addr_t &addr);

	virtual int32_t GetDefaultPort() { return 443; }
private:
	entropy_context entropy;
	ctr_drbg_context ctr_drbg;
	ssl_context ssl;
	x509_crt cacert;

	int server_fd;

	static void my_debug(void *ctx, int level, const char *str);
	static int my_verify(void *ctx, x509_crt *cert, int depth, int *flags);
};