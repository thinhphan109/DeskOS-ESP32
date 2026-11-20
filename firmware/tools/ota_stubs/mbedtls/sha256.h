#pragma once
#include <cstddef>
#include <cstdint>
#include <openssl/evp.h>

typedef struct { EVP_MD_CTX *ctx = nullptr; } mbedtls_sha256_context;

inline void mbedtls_sha256_init(mbedtls_sha256_context *c) { c->ctx = nullptr; }
inline void mbedtls_sha256_free(mbedtls_sha256_context *c) {
  if (c->ctx) EVP_MD_CTX_free(c->ctx);
  c->ctx = nullptr;
}
inline int mbedtls_sha256_starts(mbedtls_sha256_context *c, int) {
  c->ctx = EVP_MD_CTX_new();
  return c->ctx && EVP_DigestInit_ex(c->ctx, EVP_sha256(), nullptr) == 1 ? 0 : -1;
}
inline int mbedtls_sha256_update(mbedtls_sha256_context *c, const uint8_t *p, size_t n) {
  return EVP_DigestUpdate(c->ctx, p, n) == 1 ? 0 : -1;
}
inline int mbedtls_sha256_finish(mbedtls_sha256_context *c, uint8_t *digest) {
  unsigned int bytes = 0;
  return EVP_DigestFinal_ex(c->ctx, digest, &bytes) == 1 && bytes == 32 ? 0 : -1;
}
