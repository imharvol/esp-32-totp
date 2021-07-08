// https://www.rfc-editor.org/rfc/rfc4226
// https://www.rfc-editor.org/rfc/rfc6238
// https://en.wikipedia.org/wiki/Google_Authenticator

byte* hmac (const byte *key, const byte *input, const int key_length, const int input_length, const int hmac_size = SHA1_SIZE) {
    byte *output = new byte[hmac_size];
    
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA1;
    
    mbedtls_md_init(&ctx);
    if (mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1)   != 0) Serial.println("Error (mbedtls_md_setup)");
    if (mbedtls_md_hmac_starts(&ctx, (const byte *)key, key_length)     != 0) Serial.println("Error (mbedtls_md_hmac_starts)");
    if (mbedtls_md_hmac_update(&ctx, (const byte *)input, input_length) != 0) Serial.println("Error (mbedtls_md_hmac_update)");
    if (mbedtls_md_hmac_finish(&ctx, output)                            != 0) Serial.println("Error (mbedtls_md_hmac_finish)");
    mbedtls_md_free(&ctx);
    
    return output;
}

void printHmac (const byte *hmac, const int hmac_size = SHA1_SIZE) {
  for(int i= 0; i< hmac_size; i++) {
    char str[3];
    sprintf(str, "%02x", (int)hmac[i]);
    Serial.print(str);
  }
  Serial.println();
}

unsigned int hotp (const byte *key, unsigned long counter, const int key_length) {
  short input_length = 8;
  // Put counter into "text byte array". I don't quite understand this part. Specially I don't understand why I can't change the value input_length to other than 8.
  byte* input = new byte[input_length];
  for (int i = input_length - 1; i >= 0; i--) {
    input[i] = (byte) (counter & 0xff);
    counter >>= input_length;
  }

  byte *output = hmac((const byte *)key, (const byte *)input, key_length, input_length);

  int truncateOffset = output[SHA1_SIZE-1] & 0xf;
  int bin_code = (output[truncateOffset] & 0x7f) << 24
    | (output[truncateOffset+1] & 0xff) << 16
    | (output[truncateOffset+2] & 0xff) << 8
    | (output[truncateOffset+3] & 0xff);

  delete output;

  return bin_code%(int)pow(10, TOTP_CODE_DIGITS);
}

unsigned int totp (const byte *key, const int key_length) {
  time_t now;
  unsigned long timestamp = time(&now);
  
  unsigned long counter = timestamp/TOTP_TIME_STEP;
  
  return hotp(key, counter, key_length);
}
