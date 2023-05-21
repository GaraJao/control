#include <AESLib.h>
#include <arduino_base64.hpp>
#include <SoftwareSerial.h>

extern uint8_t key[];
SoftwareSerial HC12(2, 3);
bool run_seed = true;
long seed;

void setup() {
  Serial.begin(9600);
  HC12.begin(9600);
}

void loop() {
  char c = Serial.read();

  if (c == 'a') {
    String code = generateCode();
    code = paddingString(code);

    Serial.print("Descriptografado: ");
    Serial.println(code);

    if (code != "") {
      code = encryptCode(code);

      Serial.print("Criptografado: ");
      Serial.println(code);
      Serial.println();

      char *send_hc12 = code.c_str();
      HC12.write(send_hc12);
    } else {
      Serial.print("Texto muito longo, não suportado.");
    }
  }

  if (c == 'b') {
    HC12.write("TwONQYpV05R7C83NrvE0NQ==");
  }

  if (HC12.available()) {
    String message = HC12.readStringUntil("\n");
    message = decryptCode(message);
    message.trim();

    // Serial.print("Descriptografado: ");
    // Serial.println(message);

    if(message == "resetSeed"){
      resetSeed();
      Serial.println("--- Seed resetada ---");
      Serial.println();
    }
  }
}

// Reset seed with time function micros()
void resetSeed() {
  do {
    // seed = 301;
    seed = micros() % 65535;
  } while (seed == 0);

  randomSeed(seed);
}

// Generate code with structure GJ-00000-0000
String generateCode() {
  if (run_seed) {
    resetSeed();
    run_seed = false;
  }
  long number = random(10000, 99999);

  String code = "GJ-";
  code.concat(number);
  code.concat("-");
  code.concat(seed);

  char *code_char = code.c_str();

  return code_char;
}

// Pads with spaces until the string is 16 characters long
String paddingString(String text) {
  char *text_char = text.c_str();

  if (strlen(text_char) <= 16) {
    for (int i = strlen(text_char); i < 16; i++)
      text.concat(" ");

    return text;
  } else
    return "";
}

// Encrypts a string with AES and base64
String encryptCode(String code) {
  char code_char[16];

  for (int i = 0; i < 16; i++) {
    code_char[i] = code.c_str()[i];
  }

  aes128_enc_single(key, code_char);

  auto code_length = sizeof(code_char);
  char encrypted_code[base64::encodeLength(code_length)];
  base64::encode(code_char, code_length, encrypted_code);

  return encrypted_code;
}

// Decrypts a string with base64 and AES
String decryptCode(String code) {
  const char *code_char = code.c_str();

  uint8_t decrypted_code[base64::decodeLength(code_char)];
  base64::decode(code_char, decrypted_code);

  aes128_dec_single(key, decrypted_code);

  return decrypted_code;
}