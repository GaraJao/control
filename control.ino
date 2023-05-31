#include <AESLib.h>
#include <arduino_base64.hpp>
#include <SoftwareSerial.h>
#include <RTClib.h>

#define CODE_LENGTH 32
#define BUTTON 13

SoftwareSerial HC12(2, 3);
RTC_DS1307 rtc;

extern uint8_t key[];
bool run_seed = true;
unsigned long seed, date_now = 1685550059;

void setup() {
  Serial.begin(9600);
  HC12.begin(9600);
  
  pinMode(BUTTON, INPUT_PULLUP);
}

void loop() {

  if (digitalRead(BUTTON) == LOW) {
    // Serial.println("Apertou");
  }
  
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
    HC12.write("NaoNBvY1G5fOFB6br9LdvaZ9TpFua41maTKOPTiDzgM=");
  }

  if (c == 'c') {
    resetSeed();
  }

  if (HC12.available()) {
    String message = HC12.readStringUntil("\n");
    message = decryptCode(message);
    message.trim();

    if (message == "resetSeed") {
      resetSeed();
      Serial.println("--- Seed resetada ---");
      Serial.println();
    }
  }
}

// Reset seed with time function micros()
void resetSeed() {
  do {
    seed = micros() % 65535;
  } while (seed == 0 || seed == 1);

  randomSeed(seed);
}

// Generate code with structure GJ-00000-0000-0000000000
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
  code.concat("-");
  code.concat(date_now);

  char *code_char = code.c_str();

  return code_char;
}

// Pads with spaces until the string is mod 16 characters long
String paddingString(String text) {
  char *text_char = text.c_str();

  if (strlen(text_char) <= CODE_LENGTH) {
    for (int i = strlen(text_char); i < CODE_LENGTH; i++)
      text.concat(" ");

    return text;
  } else
    return "";
}

// Encrypts a string with AES and base64
String encryptCode(String code) {
  char code_char[CODE_LENGTH];

  for (int i = 0; i < CODE_LENGTH; i++) {
    code_char[i] = code.c_str()[i];
  }

  aes128_enc_multiple(key, code_char, sizeof(code_char));

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

  aes128_dec_multiple(key, decrypted_code, sizeof(decrypted_code));

  return decrypted_code;
}