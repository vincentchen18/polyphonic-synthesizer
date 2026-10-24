#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// key pins :)

const uint8_t COL_PINS[5]  = {2, 3, 4, 5, 6};
const uint8_t ROW_PINS[5] = {7, 8, 9, 10, 11};
const uint8_t MUX_PINS[3] = {12, 13, 14};
const uint8_t KNOB_PIN = 26;
// not keys anymore but still pins
const uint8_t OLED_SDA = 16;
const uint8_t OLED_SCL = 17;

// keys

const uint8_t NUM_KEYS = 25;
const uint8_t BASE_NOTE = 48;
bool key_down[NUM_KEYS];
uint8_t heldNote[NUM_KEYS]; // note each key starts on so octave change don't mess up releases stuff.

const char* NOTE_NAMES[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

// knobbys

enum {
  K_VOLUME, K_INSTRUMENT, K_TONE, K_ATTACK, K_RELEASE, K_ECHO, K_OCTAVE, K_TRANSPOSE, NUM_KNOBS
};
const char* KNOB_NAMES[NUM_KNOBS] = {
  "Volume", "Instrument", "Tone", "Attack", "Release", "Echo", "Octave", "Transpose"
};
// so basically the knobs will adjust different stuff. volume with change loudness, 
// instrument will change the tone colour (music theory nerds) 
// tone will change its bright/darkness.
// attack will basically change the fade of the note 
// release will change how long each note rings for. (basically a damper pedal.)
// i mean, do i even need to explain echo
// octave will change the octave range (this will probably get messy for sweaty pianists that use my piano that need to spam octave changes)
// finally transpose will move stuff up/down semitones (since im lazy and get bored of my pieces, i need this for variance :)
const uint8_t num_instruments = 5;
float knob[NUM_KNOBS];
float knobShown[NUM_KNOBS];
const char* INSTRUMENT_NAMES[num_instruments] = {"Piano", "Organ", "Pluck", "Bell", "8-bit"};
int instrument = 0; // range from 0-4
int octave = 0; // range from -2 to 2
int transpose = 0; //theres 13 notes and 12 to transpose to so range is +/- 6
unsigned long lastKnobRead = 0;

// screen stuff
Adafruit_SSD1306 display(128, 64, &Wire, -1);
bool screenDirty = true; 
int lastKnobTurned = -1;
unsigned long knobTurnedAt = 0;
unsigned long lastDraw = 0;


int knobStep(float v, int n) {
  int s = (int)(v*n);
  return s >= n ? n-1 : s;
}

void printNote(uint8_t note) {
  Serial.print(NOTE_NAMES[note % 12]);
  Serial.println(note / 12 - 1);
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); // start conection
  for (uint8_t c = 0; c<5; c++) {
    pinMode(COL_PINS[c], OUTPUT);
    digitalWrite(COL_PINS[c], LOW);
  }
  for (uint8_t r=0; r<5; r++) {
    pinMode(ROW_PINS[r], INPUT_PULLDOWN);
  }
  for (uint8_t i=0; i<3; i++) {
    pinMode(MUX_PINS[i], OUTPUT);
  }
  analogReadResolution(12);
  Wire.setSDA(OLED_SDA);
  Wire.setSCL(OLED_SCL);
  Wire.begin();
  Wire.setClock(400000);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found");
  }
  display.setTextColor(SSD1306_WHITE);
}

// check the keys which are pressed.
void scanKeys() {
  for (uint8_t c=0; c<5; c++) {
    digitalWrite(COL_PINS[c], HIGH);
    delayMicroseconds(5);
    for (uint8_t r=0; r<5; r++) {
      uint8_t k = r*5+c; // map to the correct key.
      bool pressed = digitalRead(ROW_PINS[r]);
      if (pressed != key_down[k]) {
        key_down[k] = pressed;
        if (pressed) {
          heldNote[k] = BASE_NOTE + k + 12 * octave + transpose;
          Serial.print("DOWN ");
        } else {
          Serial.print("UP   ");
        }
        printNote(heldNote[k]); // i hate semicolons sm
        screenDirty = true;
      }
    }

    digitalWrite(COL_PINS[c], LOW);
  }
}

void readKnobs() {
  for (uint8_t i = 0; i< NUM_KNOBS; i++) {
    digitalWrite(MUX_PINS[0], i&1);
    digitalWrite(MUX_PINS[1], (i >> 1) & 1);
    digitalWrite(MUX_PINS[2], (i >> 2) & 1);
    delayMicroseconds(10);
    float v = analogRead(KNOB_PIN) / 4095.0f;
    knob[i] += 0.2f * (v - knob[i]);

  }
  int newInstrument = knobStep(knob[K_INSTRUMENT], num_instruments);
  int newOctave = knobStep(knob[K_OCTAVE], 5) - 2;
  int newTranspose = knobStep(knob[K_TRANSPOSE], 13) - 6;
  // change the readings of the knobs
  if (newInstrument != instrument) {
    instrument = newInstrument;
    Serial.print("Instrument: ");
    Serial.println(INSTRUMENT_NAMES[instrument]);
    screenDirty=true;
  }
  if (newOctave != octave) {
    octave = newOctave;
    Serial.print("Octave: ");
    Serial.println(octave);
    screenDirty=true;

  }
  if (newTranspose != transpose) {
    transpose = newTranspose;
    Serial.print("Transpose: ");
    Serial.println(transpose);
    screenDirty=true;

  }

  const uint8_t smooth[] = {K_VOLUME, K_TONE, K_ATTACK, K_RELEASE, K_ECHO};
  for (uint8_t j = 0; j < 5; j++) {
    uint8_t i = smooth[j];
    if (fabsf(knob[i] - knobShown[i]) > 0.02f) {
      knobShown[i] = knob[i];
      Serial.print(KNOB_NAMES[i]);
      Serial.print(": ");
      Serial.print((int)(knob[i] * 100));
      Serial.println("%");
      lastKnobTurned = i;       
      knobTurnedAt = millis();
      screenDirty = true;
    }
  }
}

void printSigned(int v) {
  if (v > 0) display.print('+');
  display.print(v);
}

void drawScreen() {
  display.clearDisplay();
  //instrument name (big :D)
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(INSTRUMENT_NAMES[instrument]);

  // display octave and transposition
  display.setTextSize(1);
  display.setCursor(0, 22);
  display.print("Oct "); // shorten to fit 21 chars on one line
  printSigned(octave);
  display.print("   Trans ");
  printSigned(transpose);

  if (lastKnobTurned >= 0) {
    display.setCursor(0, 38);
    display.print(KNOB_NAMES[lastKnobTurned]);
    display.drawRect(0, 50, 128, 10, SSD1306_WHITE);
    display.fillRect(2, 52, (int)(124 * knob[lastKnobTurned]), 6, SSD1306_WHITE);
  } else {
    display.setCursor(0, 42);
    for (uint8_t k = 0; k < NUM_KEYS; k++) {
      if (key_down[k]) {
        display.print(NOTE_NAMES[heldNote[k] % 12]);
        display.print(heldNote[k] / 12 - 1);
        display.print(' ');
      }
    }
  }
  display.display();
}

void loop() {
  // put your main code here, to run repeatedly:
  scanKeys();
  
  if (millis() - lastKnobRead >= 5) {
    lastKnobRead = millis();
    readKnobs();
  }

  if (lastKnobTurned >= 0 && millis() - knobTurnedAt > 1500) {
    lastKnobTurned = -1;
    screenDirty = true;
  }

  if (screenDirty && millis() - lastDraw >= 50) {
    lastDraw = millis();
    screenDirty = false;
    drawScreen();
  }

  delay(1);
}
