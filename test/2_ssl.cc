// created by lcc 12/20/2021

#include <cassert>
#include <iomanip>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/md5.h>
#include <openssl/ssl.h>

#include <iostream>
#include <sstream>

std::string bytes_to_hex(const unsigned char* bytes, const size_t len) {
  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  for (size_t i = 0; i < len; ++i) {
    ss << std::setw(2) << static_cast<int>(bytes[i]);
  }
  return ss.str();
}

int main() {
  // Test MD5
  const std::string text = "abc-123";
  unsigned char md5_digest[MD5_DIGEST_LENGTH];
  MD5(reinterpret_cast<const unsigned char*>(text.c_str()), text.length(), md5_digest);
  assert("6351623c8cef86fefabfa7da046fc619" == bytes_to_hex(md5_digest, MD5_DIGEST_LENGTH));

  // Test AES
  constexpr unsigned char iv[] = { '1','2','3','a','a','a','4','5','6','o','o','o','7','8','9','0' };
  constexpr unsigned char key[] = {
    '1','2','3','a','a','a','4','5','6','o','o','o','7','8','9','0',
    '1','2','3','a','a','a','4','5','6','o','o','o','7','8','9','0',
    '1','2','3','a','a','a','4','5','6','o','o','o','7','8','9','0',
    '1','2','3','a','a','a','4','5','6','o','o','o','7','8','9','0',
    '1','2','3','a','a','a','4','5','6','o','o','o','7','8','9','0',
    '1','2','3','a','a','a','4','5','6','o','o','o','7','8','9','0',
    '1','2','3','a','a','a','4','5','6','o','o','o','7','8','9','0',
    '1','2','3','a','a','a','4','5','6','o','o','o','7','8','9','0',
  };
  constexpr int key_len = sizeof(key);

  unsigned char data[AES_BLOCK_SIZE * 32];
  constexpr int data_len = sizeof(data);

  if (RAND_bytes(data, sizeof(data)) != 1) {
    std::cerr << "RAND_bytes failed" << std::endl;
    return 1;
  }

  // Pad the data to be a multiple of AES_BLOCK_SIZE
  // ...
  constexpr int padded_data_len = data_len;

  // --- Encryption ---
  AES_KEY aes_encrypt_key;
  if (AES_set_encrypt_key(key, key_len, &aes_encrypt_key) < 0) {
    std::cerr << "Error setting AES encrypt key." << std::endl;
    return 1;
  }
  unsigned char current_iv_encrypt[AES_BLOCK_SIZE];
  memcpy(current_iv_encrypt, iv, AES_BLOCK_SIZE);

  unsigned char cipher_data[padded_data_len];
  AES_cbc_encrypt(data, cipher_data, padded_data_len,
    &aes_encrypt_key, current_iv_encrypt, AES_ENCRYPT);

  // --- Decryption ---
  AES_KEY aes_decrypt_key;
  if (AES_set_decrypt_key(key, key_len, &aes_decrypt_key) < 0) {
    std::cerr << "Error setting AES decrypt key." << std::endl;
    return 1;
  }

  unsigned char current_iv_decrypt[AES_BLOCK_SIZE];
  memcpy(current_iv_decrypt, iv, AES_BLOCK_SIZE);

  unsigned char decrypted_data[padded_data_len];
  AES_cbc_encrypt(cipher_data, decrypted_data, padded_data_len,
    &aes_decrypt_key, current_iv_decrypt, AES_DECRYPT);

  // check decrypted_data
  for (int i = 0; i < data_len; i++) {
    assert(data[i] == decrypted_data[i]);
  }

  OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS, nullptr);
  const auto ssl_ctx = SSL_CTX_new(TLS_client_method());
  if (const auto ssl = SSL_new(ssl_ctx); !ssl) {
    std::cerr << "Error creating SSL.\n";
    return 1;
  }

  return 0;
}
