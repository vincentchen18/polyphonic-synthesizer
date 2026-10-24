#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <I2S.h>

// key pins :)

const uint8_t COL_PINS[5]  = {2, 3, 4, 5, 6};
const uint8_t ROW_PINS[5] = {7, 8, 9, 10, 11};
const uint8_t MUX_PINS[3] = {12, 13, 14};
const uint8_t KNOB_PIN = 26;
// not keys anymore but still pins
const uint8_t OLED_SDA = 16;
const uint8_t OLED_SCL = 17;
const uint8_t I2S_BCK = 18;
const uint8_t I2S_DIN = 20; 

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

// audio stuff
const uint32_t SAMPLE_RATE = 22050;
const int32_t ENV_MAX = 1 << 20;

volatile int32_t sharedVolume = 128; // 0-256
volatile int32_t sharedAttackStep = ENV_MAX;
volatile int32_t sharedReleaseStep = ENV_MAX / 1000;

I2S i2s(OUTPUT);
const uint8_t NUM_VOICES = 8;
int16_t sineTable[256];

enum VoiceState {V_OFF, V_ATTACK, V_SUSTAIN, V_RELEASE};
struct Voice {
  VoiceState state = V_OFF;
  uint8_t key=255;
  uint32_t phase = 0;
  uint32_t phaseInc = 0;
  int32_t env=0;
};
Voice voices[NUM_VOICES];


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
          rp2040.fifo.push_nb((1u << 31) | (k << 8) | heldNote[k]); // which note is on
        } else {
          Serial.print("UP   ");
          rp2040.fifo.push_nb(k << 8);
        }
        printNote(heldNote[k]); // i hate semicolons sm
        screenDirty = true;
      }
    }

    digitalWrite(COL_PINS[c], LOW);
  }
}

int32_t envStep(float v, float minMs, float maxMs) {
  float ms = minMs * powf(maxMs / minMs, v);
  float samples = ms * SAMPLE_RATE / 1000.0f;
  int32_t step = (int32_t)(ENV_MAX / samples);
  return step = 1 ? 1: step;
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

  sharedVolume = (int32_t)(knob[K_VOLUME] * 256);
  sharedAttackStep = envStep(knob[K_ATTACK], 2, 1500);
  sharedReleaseStep = envStep(knob[K_RELEASE], 20, 3000);
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

void setup1() {
  for (int i=0; i<256; i++) {
    sineTable[i] = (int16_t)(32767 * sinf(2*PI*i/256.0f));
  }
  i2s.setBCLK(I2S_BCK);
  i2s.setDATA(I2S_DIN);
  i2s.setBitsPerSample(16);
  i2s.begin(SAMPLE_RATE);
}

void noteOn(uint8_t key, uint8_t note) {
  int pick = -1;
  for (int v=0; v<NUM_VOICES; v++) {
    if (voices[v].state == V_OFF) {
      pick = v;
      break;
    }
  }
  if (pick < 0) {
    int32_t quietest = ENV_MAX + 1;
    for (int v=0; v<NUM_VOICES; v++) {
      if (voices[v].state == V_RELEASE && voices[v].env < quietest) {
        quietest = voices[v].env;
        pick = v;
      }
    }
  }
  if (pick < 0) pick = 0;

  float frequency = 440.0f * powf(2.0f, (note-69) / 12.0f);
  Voice &vo = voices[pick];
  vo.key = key;
  vo.phase = 0;
  vo.phaseInc = (uint32_t)(frequency * 4294967296.0f / SAMPLE_RATE);
  vo.env = 0;
  vo.state = V_ATTACK;

}

void noteOff(uint8_t key) {
  for (int v=0; v<NUM_VOICES; v++) {
    if (voices[v].key == key && (voices[v].state == V_ATTACK || voices[v].state == V_SUSTAIN)) {
      voices[v].state = V_RELEASE;
    }
  }
}

void loop1() {
  uint32_t msg;
  while (rp2040.fifo.pop_nb(&msg)) {
    uint8_t key = (msg >> 8) & 0xFF;
    if (msg >> 31) noteOn(key, msg&0xFF);
    else noteOff(key);
  }
  int32_t attackStep = sharedAttackStep;
  int32_t releaseStep = sharedReleaseStep;
  int32_t volume = sharedVolume;

  for (int n=0; n<64; n++) {
    int32_t mix = 0;
    for (int v=0; v < NUM_VOICES; v++) {
      Voice &vo = voices[v];
      if (vo.state == V_OFF) continue;
      if (vo.state == V_ATTACK) {
        vo.env += attackStep;
        if (vo.env >= ENV_MAX) {
          vo.env = ENV_MAX; 
          vo.state = V_SUSTAIN;
        }
      } else if (vo.state == V_RELEASE) {
        vo.env -= releaseStep;
        if (vo.env <= 0) {
          vo.env=0;
          vo.state = V_OFF;
          continue;
        } 
      }

      // sine oscillator
      int32_t s = sineTable[vo.phase >> 24];
      vo.phase += vo.phaseInc;
      mix += (s*(vo.env>>5)) >> 15;
    }
    mix = (mix >> 2) * volume >> 8;
    // safety limits
    if (mix > 32767) mix  = 32767;
    if (mix<-32768) mix  = -32768;
    i2s.write16((int16_t)mix, (int16_t)mix);
  }
}