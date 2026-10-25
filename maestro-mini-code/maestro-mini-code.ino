#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Display Config
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// MCP23017 Config
Adafruit_MCP23X17 mcp;

#define AUDIO_PIN D3 

// MCP23017 Pin Mappings for the Buttons
const int PIN_C = 0;
const int PIN_D = 1;
const int PIN_E = 2;
const int PIN_F = 3;
const int PIN_G = 4;
const int PIN_A = 5;
const int PIN_B = 6;
const int PIN_FL = 8;
const int PIN_SH = 9;
const int PIN_CTRL = 10;

const int baseNotePins[7] = {PIN_C, PIN_D, PIN_E, PIN_F, PIN_G, PIN_A, PIN_B};
const char* baseNoteNames[7] = {"C", "D", "E", "F", "G", "A", "B"};

// The frequencies for each note, first one flat, second natural, third sharp, from C to B
const int freqs[7][3] = {
  {247, 262, 277},
  {277, 294, 311},
  {311, 330, 349},
  {330, 349, 370},
  {370, 392, 415},
  {415, 440, 466},
  {466, 494, 523}
};

int currentNoteIndex = -1;
int currentModifier = 1;   // Flat, Natural or Sharp, from 0 to 2
bool synthMode = true;     // Note playing or tune playing, true or false
bool lastCtrlState = true; 

void setup() {
  Serial.begin(115200);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  updateDisplay("Synth Ready!", "Press a note", 0);

  if (!mcp.begin_I2C()) {
    Serial.println("Error initializing MCP23017.");
    while (1);
  }

  for (int i = 0; i < 7; i++) mcp.pinMode(baseNotePins[i], INPUT_PULLUP);
  mcp.pinMode(PIN_FL, INPUT_PULLUP);
  mcp.pinMode(PIN_SH, INPUT_PULLUP);
  mcp.pinMode(PIN_CTRL, INPUT_PULLUP);

  pinMode(AUDIO_PIN, OUTPUT);
}

void loop() {
  bool ctrlState = mcp.digitalRead(PIN_CTRL);
  if (ctrlState == LOW && lastCtrlState == HIGH) {
    synthMode = !synthMode; 
    noTone(AUDIO_PIN);   
    currentNoteIndex = -1; 
    
    if (!synthMode) {
      updateDisplay("Auto Play", "Mary Had a Lamb", 0);
      playTune();
      synthMode = true; 
      updateDisplay("Synth Mode", "Press a note", 0);
    }
  }
  lastCtrlState = ctrlState;

  if (synthMode) {
   
    for (int i = 0; i < 7; i++) {
      if (mcp.digitalRead(baseNotePins[i]) == LOW) {
        if (currentNoteIndex != i) {
          currentNoteIndex = i;
          currentModifier = 1;
          playCurrentState();
        }
      }
    }

    if (mcp.digitalRead(PIN_SH) == LOW && currentNoteIndex != -1 && currentModifier != 2) {
      currentModifier = 2;
      playCurrentState();
    }

    if (mcp.digitalRead(PIN_FL) == LOW && currentNoteIndex != -1 && currentModifier != 0) {
      currentModifier = 0;
      playCurrentState();
    }
  }
  
  delay(40);
}

void playCurrentState() {
  if (currentNoteIndex == -1) return;
  
  int freq = freqs[currentNoteIndex][currentModifier];
  tone(AUDIO_PIN, freq);
  
  String modString = "";
  if (currentModifier == 0) modString = " Flat";
  if (currentModifier == 2) modString = " Sharp";
  
  String noteText = String(baseNoteNames[currentNoteIndex]) + modString;
  updateDisplay("Playing:", noteText, freq);
}

void updateDisplay(String line1, String line2, int freq) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  
  display.setCursor(0, 10);
  display.println(line1);
  
  display.setTextSize(2);
  display.setCursor(0, 25);
  display.println(line2);
  
  if (freq > 0) {
    display.setTextSize(1);
    display.setCursor(0, 50);
    display.print(freq);
    display.println(" Hz");
  }
  
  display.display();
}

// Mary Had a Little Lamb
void playTune() {
  int melody[] = {330, 294, 262, 294, 330, 330, 330, 0, 294, 294, 294, 0, 330, 392, 392};
  int durations[] = {400, 400, 400, 400, 400, 400, 800, 100, 400, 400, 800, 100, 400, 400, 800};
  
  for (int i = 0; i < 15; i++) {
    if (melody[i] == 0) {
      noTone(AUDIO_PIN);
    } else {
      tone(AUDIO_PIN, melody[i]);
    }
    delay(durations[i]);
    
    if (mcp.digitalRead(PIN_CTRL) == LOW) {
      delay(200);
      break;
    }
  }
  noTone(AUDIO_PIN);
}