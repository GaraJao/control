#include <AESLib.h>
#include <arduino_base64.hpp>
#include <SoftwareSerial.h>
#include <RTClib.h>

#define CODE_LENGTH 32
#define PRESS_INTERVAL 1000
#define BUTTON 8

SoftwareSerial HC12(2, 3);
RTC_DS1307 RTC;

extern uint8_t key[];
extern uint8_t iv[];
unsigned long seed, random_number, previous_time = 0;

void setup() {
  Serial.begin(9600);
  HC12.begin(9600);

  pinMode(BUTTON, INPUT_PULLUP);

  if (!RTC.begin()) {
    Serial.println("DS1307 não encontrado");
    while (true) {}
  }

  // RTC.adjust(DateTime(F(__DATE__), F("19:09:10")));

  resetSeed();
}

void loop() {
  unsigned long current_time = millis();

  // Send code on type s in serial monitor
  if (digitalRead(BUTTON) == LOW && current_time - previous_time > PRESS_INTERVAL) {
    String code = generateCode();
    code = paddingString(code);

    Serial.print("Descriptografado: ");
    Serial.println(code);

    code = encryptCode(code);

    Serial.print("Criptografado: ");
    Serial.println(code);
    Serial.println();

    char *send_hc12 = code.c_str();
    HC12.write(send_hc12);

    previous_time = current_time;
  }

  if (Serial.available()) {
    String c = Serial.readStringUntil('\n');

    if (c[0] == 't') {  // Set time by serial monitor
      c = strtok(c.c_str(), " ");
      c = strtok(NULL, " ");

      RTC.adjust(DateTime(__DATE__, c.c_str()));
    } else if (c[0] == 'r')  // Reset seed on type r in serial monitor
      resetSeed();
    else if (c[0] == 'h')  // Print hour on type h in serial monitor
      printHour();
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
  DateTime now = RTC.now();

  do {
    seed = now.unixtime() / millis() % 65535;
  } while (seed == 0 || seed == 1);

  randomSeedRefactored(seed);
}

// Generate code with structure GJ-00000-00000-0000000000
String generateCode() {
  DateTime now = RTC.now();
  long number = randomRefactored(10000, 99999);

  String code = "GJ-";
  code.concat(number);
  code.concat("-");
  code.concat(seed);
  code.concat("-");
  code.concat(now.unixtime());

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

  aes128_cbc_enc(key, iv, code_char, sizeof(code_char));

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

  aes128_cbc_dec(key, iv, decrypted_code, sizeof(decrypted_code));

  return decrypted_code;
}

// Refactoring of the randomization algorithm based on Linear congruential generator
long randomRefactored(long howbig) {
  if (howbig == 0)
    return 0;

  random_number = random_number * 1103515245 + 12345;
  return (unsigned int)(random_number / 65536) % howbig;
}

// Refactoring of the randomization algorithm based on Linear congruential generator
long randomRefactored(long howsmall, long howbig) {
  if (howsmall >= howbig)
    return howsmall;

  long diff = howbig - howsmall;
  return randomRefactored(diff) + howsmall;
}

// Refactoring of the randomization algorithm based on Linear congruential generator
void randomSeedRefactored(long value) {
  random_number = value;
}

// Print date and hour in serial monitor
void printHour() {
  DateTime now = RTC.now();

  Serial.print("Data: ");
  Serial.print(now.day(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.year(), DEC);
  Serial.print(" / Hora: ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
}