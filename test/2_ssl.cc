// created by lcc 12/20/2021

#include <openssl/ssl.h>

#include <iostream>

int main() {
  OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS, nullptr);
  const auto ssl_ctx = SSL_CTX_new(TLS_client_method());
  if (const auto ssl = SSL_new(ssl_ctx); !ssl) {
    std::cerr << "Error creating SSL.\n";
    return 1;
  }
}
