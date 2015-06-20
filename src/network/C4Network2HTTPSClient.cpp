#include "C4Include.h"
#ifdef C4ENGINE
#include "game/C4Game.h"
#endif
#include "C4Version.h"
#include "C4Network2HTTPSClient.h"

#include <wincrypt.h>
#pragma comment(lib, "crypt32.lib")

C4Network2HTTPSClient::C4Network2HTTPSClient() : C4Network2HTTPClient() {
  debug_set_threshold(1000);

  memset(&ssl, 0, sizeof(ssl_context));
  x509_crt_init(&cacert);

  entropy_init(&entropy);
  ctr_drbg_init(&ctr_drbg, entropy_func, &entropy, NULL, 0);

  x509_crt_parse(&cacert, (const unsigned char *)test_ca_list,
                 strlen(test_ca_list));

  ssl_init(&ssl);
  ssl_set_endpoint(&ssl, SSL_IS_CLIENT);

  ssl_set_authmode(&ssl, SSL_VERIFY_OPTIONAL);
  ssl_set_ca_chain(&ssl, &cacert, NULL, NULL);

  ssl_set_rng(&ssl, ctr_drbg_random, &ctr_drbg);
  ssl_set_dbg(&ssl, my_debug, stdout);

  ssl_set_verify(&ssl, my_verify, NULL);
}

bool C4Network2HTTPSClient::Connect(const addr_t &addr) {
  int ret =
      net_connect(&server_fd, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
  ssl_set_bio(&ssl, net_recv, &server_fd, net_send, &server_fd);

  while ((ret = ssl_handshake(&ssl)) != 0) {
    if (ret != POLARSSL_ERR_NET_WANT_READ &&
        ret != POLARSSL_ERR_NET_WANT_WRITE) {
      SetError("SSL handshake failed: bad socket!");
      return false;
    }
  }
  /* In real life, we may want to bail out when ret != 0 */
  if ((ret = ssl_get_verify_result(&ssl)) != 0) {
    const char *reason = "unknown";
    if ((ret & BADCERT_EXPIRED) != 0) reason = "expired";
    if ((ret & BADCERT_REVOKED) != 0) reason = "revoked";
    if ((ret & BADCERT_CN_MISMATCH) != 0) reason = "cn mismatch";
    if ((ret & BADCERT_NOT_TRUSTED) != 0) reason = "not trusted";

    SetError(FormatString("SSL verify failed: bad certificate (%s)!", reason)
                 .getData());
    return true;
    //		return false;
  }

  return (ret == 0);
}

bool C4Network2HTTPSClient::Send(const C4NetIOPacket &rPacket) {
  return (ssl_write(&ssl, (const unsigned char *)rPacket.getPData(),
                    rPacket.getSize()) == 0);
}

void C4Network2HTTPSClient::my_debug(void *ctx, int level, const char *str) {
  Log(str);
}

int C4Network2HTTPSClient::my_verify(void *ctx, x509_crt *cert, int depth,
                                     int *flags) {
  CERT_CHAIN_PARA ChainPara;
  PCCERT_CHAIN_CONTEXT pChainContext = NULL;

  PCCERT_CONTEXT pccert_ctx = CertCreateCertificateContext(
      X509_ASN_ENCODING, cert->raw.p, cert->raw.len);

  ZeroMemory(&ChainPara, sizeof(ChainPara));
  ChainPara.cbSize = sizeof(ChainPara);

  if (!CertGetCertificateChain(HCCE_CURRENT_USER, pccert_ctx, NULL, NULL,
                               &ChainPara, 0, NULL, &pChainContext)) {
    return GetLastError();
  }

  Log("verifying certificate");
  // everything valid
  return 0;
}

C4Network2HTTPSClient::~C4Network2HTTPSClient() {
  if (server_fd != -1) net_close(server_fd);

  x509_crt_free(&cacert);
  ssl_free(&ssl);
  ctr_drbg_free(&ctr_drbg);
  entropy_free(&entropy);

  memset(&ssl, 0, sizeof(ssl));
}