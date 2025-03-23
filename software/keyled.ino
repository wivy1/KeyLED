//note numbering scheme:
//  A  A#  B  C  C#  D  D#  E  F  F#  G  G#
//     Bb        Db     Eb        Gb     Ab
//  0  1   2  3  4   5  6   7  8  9   10 11

#include <FastLED.h>
#include <EEPROMex.h>
#include <LiquidCrystal.h>
#include <Rotary.h>

// LED
const byte MAX_LEDS = 500; //to avoid dynamic memory allocation, we will put a hard cap on the maximum number of LEDS, limited by the RAM of the atmega
const byte DATA_PIN = 6; // pin used for data line
const unsigned long int ROOT_COLOR = 0x42FF00;
const unsigned long int WHITE_KEY_COLOR = 0xFFFFFF;
const unsigned long int BLACK_KEY_COLOR = 0xFF0000;
CRGB leds[MAX_LEDS]; //stores the states of the LEDs

// ROTARY ENCODER
Rotary r = Rotary(8, 9);
volatile byte encoderPos = 0; //this variable stores our current value of encoder position. Change to int or uin16_t instead of byte if you want to record a larger range than 0-255

// EEPROM
const unsigned long interval = 1000; //interval between checks on eeprom contents
unsigned long previousMillis = 0; //dont want our eeprom writing to happen on each loop iteration
byte rootnote_address;
byte mode_address;
byte init_address;
const byte maxAllowedWrites = 500;

// LCD
const byte rs = 7, en = 2, d4 = 3, d5 = 10, d6 = 16, d7 = 14; // LCD mappings
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// BUTTON
const byte BUTTON_PIN = 4; // this is the Arduino pin we are connecting the push button to
byte oldButtonState = HIGH;  // assume switch open because of pull-up resistor
const unsigned long debounceTime = 10;  // milliseconds
const unsigned long holdTime = 1000; //ms, how long to be considered a long hold
unsigned long buttonChangeTime;  // when the switch last changed state
unsigned long buttonPressTime;
boolean buttonPressed = 0;
boolean buttonHeld = 0;
boolean waitingForHold = 0;

// MENUS
const byte NUM_MENUS = 2;
//menu 0 = Mode Menu
//menu 1 = Setup Menu
byte currentMenu; 

const byte NUM_MODE_OPT = 2;
//option 0 = root note
//option 1 = mode selection
byte currentModeOpt; //start on the root selection
byte newModeOpt;

byte currentRootNoteIndex;
byte newRootNoteIndex;

byte currentModeIndex;
byte newModeIndex;
//0 = choosing parameter
//1 = setting parameter

int currentSetupOpt; //start on the first setup option
int newSetupOpt;

const byte NUM_SETUP_MODES = 2;
//mode 0 = set option
//mode 1 = set value
byte currentSetupMode; //start on the first setup option
byte newSetupMode;

byte newSetupOptValue;

//STRUCTS
struct Note {
  char name[3];
  byte number;
};

const Note notes[] =
{
  {"A", 0},
  {"A#", 1},
  {"Bb", 1},
  {"B", 2},
  {"C", 3},
  {"C#", 4},
  {"Db", 4},
  {"D", 5},
  {"D#", 6},
  {"Eb", 6},
  {"E", 7},
  {"F", 8},
  {"F#", 9},
  {"Gb", 9},
  {"G", 10},
  {"G#", 11},
  {"Ab", 11}
};


struct Mode {
  char * name;
  byte steps[11];
  byte numNotes; //number of notes in this mode
  char * intervals; //the tonic intervals of this mode
};

const char IonianNam[] PROGMEM = "Ionian/Major";
const char IonianInt[] PROGMEM = "M2 M3 4 5 M6 M7";
const char DorianNam[] PROGMEM = "Dorian";
const char DorianInt[] PROGMEM = "M2 m3 4 5 M6 m7";
const char PhrygianNam[] PROGMEM = "Phrygian";
const char PhrygianInt[] PROGMEM = "m2 m3 4 5 m6 m7";
const char LydianNam[] PROGMEM = "Lydian";
const char LydianInt[] PROGMEM = "M2 M3 tt 5 M6 M7";
const char MixNam[] PROGMEM = "Mixolydian";
const char MixInt[] PROGMEM = "M2 M3 4 5 M6 m7";
const char AeolianNam[] PROGMEM = "Aeolian/Minor";
const char AeolianInt[] PROGMEM = "M2 m3 4 5 m6 m7";
const char LocrianNam[] PROGMEM = "Locrian";
const char LocrianInt[] PROGMEM = "m2 m3 4 tt m6 m7";
const char MelMinNam[] PROGMEM = "Melodic Minor";
const char MelMinInt[] PROGMEM = "M2 m3 4 5 M6 M7";
const char HarMinNam[] PROGMEM = "Harmonic Min";
const char HarMinInt[] PROGMEM = "M2 m3 4 5 m6 M7";
const char MajPenNam[] PROGMEM = "MajPentatonic";
const char MajPenInt[] PROGMEM = "M2 M3   5 M6";
const char MinPenNam[] PROGMEM = "MinPentatonic";
const char MinPenInt[] PROGMEM = "   m3 4 5    m7";
const char WholeNam[] PROGMEM = "Whole Tone";
const char WholeInt[] PROGMEM = "M2 M3 tt m6 m7";
const char AugNam[] PROGMEM = "Augmented";
const char AugInt[] PROGMEM = "m3 M3 5 m6 M7";
const char ChromNam[] PROGMEM = "Chromatic";
const char ChromInt[] PROGMEM = "";
const char FulDmNam[] PROGMEM = "FulDiminished";
const char FulDmInt[] PROGMEM = "M2m34ttm6M6M7";
const char DomDmNam[] PROGMEM = "DomDiminished";
const char DomDmInt[] PROGMEM = "m2m3M3tt5M6m7";
const char HlfDmNam[] PROGMEM = "HlfDiminished";
const char HlfDmInt[] PROGMEM = "M2 m3 4 tt m6 m7";
const char GypsNam[] PROGMEM = "PhryDom/Gypsy";
const char GypsInt[] PROGMEM = "m2 M3 4 5 m6 m7";
const char Dor4Nam[] PROGMEM = "Dorian #4";
const char Dor4Int[] PROGMEM = "M2 m3 tt 5 M6 m7";

const Mode modes[] =
{
  {IonianNam, {2, 2, 1, 2, 2, 2}, 7, IonianInt},
  {DorianNam, {2, 1, 2, 2, 2, 1}, 7, DorianInt},
  {PhrygianNam, {1, 2, 2, 2, 1, 2}, 7, PhrygianInt},
  {LydianNam, {2, 2, 2, 1, 2, 2}, 7, LydianInt},
  {MixNam, {2, 2, 1, 2, 2, 1}, 7, MixInt},
  {AeolianNam, {2, 1, 2, 2, 1, 2}, 7, AeolianInt},
  {LocrianNam, {1, 2, 2, 1, 2, 2}, 7, LocrianInt},
  {MelMinNam, {2, 1, 2, 2, 2, 2}, 7, MelMinInt},
  {HarMinNam, {2, 1, 2, 2, 1, 3}, 7, HarMinInt},
  {MajPenNam, {2, 2, 3, 2}, 5, MajPenInt},
  {MinPenNam, {3, 2, 2, 3}, 5, MinPenInt},
  {WholeNam, {2, 2, 2, 2, 2}, 6, WholeInt},
  {AugNam, {3, 1, 3, 1, 3}, 6, AugInt},
  {ChromNam, {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, 12, ChromInt},
  {FulDmNam, {2, 1, 2, 1, 2, 1, 2}, 8, FulDmInt},
  {DomDmNam, {1, 2, 1, 2, 1, 2, 1}, 8, DomDmInt},
  {HlfDmNam, {2, 1, 2, 1, 2, 2}, 7, HlfDmInt},
  {GypsNam, {1, 3, 1, 2, 1, 2}, 7, GypsInt},
  {Dor4Nam, {2, 1, 3, 1, 2, 1}, 7, Dor4Int}
};

const char opt_numkeys[] PROGMEM = "# of keys";
const char opt_firstnote[] PROGMEM = "First note";
const char opt_bright[] PROGMEM = "LED Brightness";
const char opt_lednumtot[] PROGMEM = "# LEDs total";
const char opt_firstled[] PROGMEM = "First LED pos'n";
const char opt_ledperkey[] PROGMEM = "# LEDs per key";
const char opt_skipled1[] PROGMEM = "LED compress A";
const char opt_skipled2[] PROGMEM = "LED compress B";

struct SetupOpt {
  char * text;
  byte val;
  byte defVal;
  byte minVal;
  byte maxVal;
  byte address;
};

SetupOpt SetupOpt[] = 
{
  {opt_numkeys, 61, 61, 1, 155},
  {opt_firstnote, 4, 4, 0, 16},
  {opt_bright, 8, 8, 1, 50},
  {opt_lednumtot, 144, 144, 1, MAX_LEDS},
  {opt_firstled, 1, 1, 1},
  {opt_ledperkey, 2, 2, 1, 3},
  {opt_skipled1, 0, 0, 1},
  {opt_skipled2, 0, 0, 1}
  
};

// keep this updated. This must match the order of the SetupOptp[] array, this allows me to reference options indirectly,
// so I can reorder them logically later and not need to update the references
enum SetupOptions {
  OPT_NUMKEYS,
  OPT_FIRSTNOTE,
  OPT_BRIGHT,
  OPT_LEDNUMTOT,
  OPT_FIRSTLED,
  OPT_LEDPERKEY,
  OPT_SKIPLED1,
  OPT_SKIPLED2
};

//to store the configuration values in eeprom
byte SetupOptAddr[(sizeof(SetupOpt) / sizeof(SetupOpt[0]))];

void updateOutput(boolean forceRefresh = 0) {

  // Modes menu
  if (currentMenu == 0) {

    if (newRootNoteIndex != currentRootNoteIndex || forceRefresh == 1) {
      lcd.setCursor(0, 0);
      lcd.print(notes[newRootNoteIndex].name);
      String tempstring = notes[newRootNoteIndex].name;
      for (int i = 0; i < (2 - tempstring.length()); i++) {
        lcd.print(" ");
      }

      lcd.setCursor(0, 0);
      if (newRootNoteIndex != currentRootNoteIndex) {
        currentRootNoteIndex = newRootNoteIndex;
      }
    }

    if (newModeIndex != currentModeIndex || forceRefresh == 1) {
      lcd.setCursor(3, 0);
      lcd.print(getString(modes[newModeIndex].name));
      String tempstring = getString(modes[newModeIndex].name);
      for (int i = 0; i < (13 - tempstring.length()); i++) {
        lcd.print(" ");
      }

      lcd.setCursor(0, 1);
      lcd.print(getString(modes[newModeIndex].intervals));
      String tempstring2 = getString(modes[newModeIndex].intervals);
      for (int i = 0; i < (16 - tempstring2.length()); i++) {
        lcd.print(" ");
      }

      lcd.setCursor(3, 0);
      if (newModeIndex != currentModeIndex) {
        currentModeIndex = newModeIndex;
      }
    }

    if (newModeOpt != currentModeOpt) {
      switch (newModeOpt) {
        case 0:
          lcd.setCursor(0, 0);
          encoderPos = currentRootNoteIndex;
          break;
        case 1:
          lcd.setCursor(3, 0);
          encoderPos = currentModeIndex;
          break;
        default:
          break;
      }
      currentModeOpt = newModeOpt;
    }
  }

  // Setup menu
  if (currentMenu == 1) {

    if (newSetupOpt != currentSetupOpt || forceRefresh) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(getString(SetupOpt[newSetupOpt].text));
      lcd.setCursor(0, 1);
      if (newSetupOpt == OPT_FIRSTNOTE){
        lcd.print(notes[SetupOpt[newSetupOpt].val].name);
      } else {
        lcd.print(SetupOpt[newSetupOpt].val);
      }
      lcd.setCursor(0, 0);
          
      if (newSetupOpt != currentSetupOpt) {
        currentSetupOpt = newSetupOpt;
      }
    }

    if (newSetupMode != currentSetupMode || forceRefresh) {
      switch (newSetupMode) {
        case 0:
          lcd.setCursor(0, 0);
          encoderPos = currentSetupOpt;
          break;
        case 1:
          lcd.setCursor(0, 1);
          encoderPos = SetupOpt[currentSetupOpt].val;
          break;
        default:
          break;
      }
      if (newSetupMode != currentSetupMode) {
        currentSetupMode = newSetupMode;
      }
    }

  }
}


void setup() {
  // for debugging
  Serial.begin(115200); // start the serial monitor link
  lcd.begin(16, 2);

  // set up momentary button
  pinMode (BUTTON_PIN, INPUT_PULLUP); // setup the button pin

  // set up rotary encoder
  r.begin();
  PCICR |= (1 << PCIE0);
  PCMSK0 |= (1 << PCINT4) | (1 << PCINT5);
  sei();

  //initialize system
  EEPROM.setMaxAllowedWrites(maxAllowedWrites);
  rootnote_address = EEPROM.getAddress(sizeof(byte));
  mode_address = EEPROM.getAddress(sizeof(byte));
  init_address = EEPROM.getAddress(sizeof(byte));
  for (int i = 0; i < (sizeof(SetupOpt) / sizeof(SetupOpt[0])); i++) {
    SetupOpt[i].address = EEPROM.getAddress(sizeof(byte));
    if (EEPROM.readByte(init_address) != 1){
      SetupOpt[i].val = SetupOpt[i].defVal;
      EEPROM.writeByte(SetupOpt[i].address, SetupOpt[i].val);
    } else {
      SetupOpt[i].val = EEPROM.readByte(SetupOpt[i].address);
    }    
  }
  if (EEPROM.readByte(init_address) != 1){
    SetupOpt[OPT_SKIPLED1].val = SetupOpt[OPT_LEDNUMTOT].val + 1;
    EEPROM.writeByte(SetupOpt[OPT_SKIPLED1].address, SetupOpt[OPT_SKIPLED1].val);
    SetupOpt[OPT_SKIPLED2].val = SetupOpt[OPT_LEDNUMTOT].val + 1;
    EEPROM.writeByte(SetupOpt[OPT_SKIPLED2].address, SetupOpt[OPT_SKIPLED2].val);
    newRootNoteIndex = 0;
    currentRootNoteIndex = 0;
    newModeIndex = 0;
    currentModeIndex = 0;
  } else {
    newRootNoteIndex = EEPROM.readByte(rootnote_address);
    currentRootNoteIndex = newRootNoteIndex;
    newModeIndex = EEPROM.readByte(mode_address);
    currentModeIndex = newModeIndex;
    encoderPos = newRootNoteIndex; //need to make sure we don't reset the rootnote when we first hit the lcd update
  }
  //set the EEPROM initialization flag so that we don't revert to defaults anymore
  EEPROM.writeByte(init_address,1);  
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, SetupOpt[OPT_LEDNUMTOT].val);
  FastLED.setBrightness(SetupOpt[OPT_BRIGHT].val);
  currentModeOpt = 0;
  newModeOpt = 0;
  currentSetupOpt = 0;
  newSetupOpt = 0;
  currentSetupMode = 0;
  newSetupMode = 0;
  updateOutput(1);
  updateLED();
  lcd.cursor();
  lcd.setCursor(0,0);
}

void loop() {
  rotaryMenu();
  // store current root note and mode to EEPROM, if changed
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    if (currentRootNoteIndex != EEPROM.readByte(rootnote_address)) {
      EEPROM.writeByte(rootnote_address, currentRootNoteIndex);
    }
    if (currentModeIndex != EEPROM.readByte(mode_address)) {
      EEPROM.writeByte(mode_address, currentModeIndex);
    }
    for (int i = 0; i < (sizeof(SetupOpt) / sizeof(SetupOpt[0])); i++) {
      byte tempbyte = EEPROM.readByte(SetupOpt[i].address);
      if (tempbyte != SetupOpt[i].val){
        EEPROM.writeByte(SetupOpt[i].address,SetupOpt[i].val);
      }    
    }
  }
}

void rotaryMenu() {
  // Button reading with non-delay() debounce
  byte buttonState = digitalRead (BUTTON_PIN);
  if (buttonState != oldButtonState) {
    if (millis () - buttonChangeTime >= debounceTime) {
      buttonChangeTime = millis ();
      oldButtonState =  buttonState;
      if (buttonState == LOW) {
        buttonPressed = 1;
        waitingForHold = 1;
        buttonPressTime = millis ();
      }
      else {
        buttonPressed = 0;
        waitingForHold = 0;
      }
    }
  }

  if ((waitingForHold) && (millis () - buttonPressTime >= holdTime)) {
    currentMenu = (currentMenu + 1) % NUM_MENUS;
    lcd.clear();
    if (currentMenu==0){
      switch (currentModeOpt) {
        case 0:
          encoderPos = currentRootNoteIndex;
          break;
        case 1:
          encoderPos = currentModeIndex;
          break;
        default:
          break;
      }
    }

    if (currentMenu==1){
      switch (currentSetupMode) {
        case 0:
          encoderPos = currentSetupOpt;
          break;
        case 1:
          encoderPos = SetupOpt[currentSetupOpt].val;
          break;
        default:
          break;
      }
    }
    
    updateOutput(1);
    waitingForHold = 0;
  }

  // Modes menu
  if (currentMenu == 0) {

    // pressing the button cycles through the options
    if (buttonPressed) {
      newModeOpt = (currentModeOpt + 1) % NUM_MODE_OPT;
      updateOutput();
      buttonPressed = 0;
    }

    switch (currentModeOpt) {
      case 0:
        newRootNoteIndex = encoderPos % (sizeof(notes) / sizeof(notes[0]));
        if (newRootNoteIndex != currentRootNoteIndex) {
          updateOutput();
          updateLED();
        }
        break;
      case 1:
        newModeIndex = encoderPos % (sizeof(modes) / sizeof(modes[0]));
        if (newModeIndex != currentModeIndex) {
          updateOutput();
          updateLED();
        }
        break;
      default:
        break;
    }

  }

  // Setup menu
  if (currentMenu == 1) {
    // pressing the button cycles through the modes
    if (buttonPressed) {
      newSetupMode = (currentSetupMode + 1) % NUM_SETUP_MODES;
      updateOutput();
      buttonPressed = 0;
    }

    if (currentSetupMode == 0) {
      newSetupOpt = encoderPos % (sizeof(SetupOpt) / sizeof(SetupOpt[0]));
      if (newSetupOpt != currentSetupOpt) {
        updateOutput();
      }
    }

    if (currentSetupMode == 1) {
      switch (currentSetupOpt) {
        
        case OPT_NUMKEYS:
          newSetupOptValue = encoderPos;
          if (newSetupOptValue > SetupOpt[OPT_NUMKEYS].maxVal){
            newSetupOptValue=SetupOpt[OPT_NUMKEYS].maxVal;
            encoderPos=SetupOpt[OPT_NUMKEYS].maxVal;
          }
          if (newSetupOptValue < SetupOpt[OPT_NUMKEYS].minVal){
            newSetupOptValue=SetupOpt[OPT_NUMKEYS].minVal;
            encoderPos=SetupOpt[OPT_NUMKEYS].minVal;
          }
          if (newSetupOptValue != SetupOpt[OPT_NUMKEYS].val){
            SetupOpt[OPT_NUMKEYS].val = newSetupOptValue;
            updateOutput(1);
            FastLED.clear();
            updateLED();
          }
          break;

        case OPT_FIRSTNOTE:
          newSetupOptValue = encoderPos;
          if (newSetupOptValue > SetupOpt[OPT_FIRSTNOTE].maxVal){
            newSetupOptValue=SetupOpt[OPT_FIRSTNOTE].maxVal;
            encoderPos=SetupOpt[OPT_FIRSTNOTE].maxVal;
          }
          if (newSetupOptValue < SetupOpt[OPT_FIRSTNOTE].minVal){
            newSetupOptValue=SetupOpt[OPT_FIRSTNOTE].minVal;
            encoderPos=SetupOpt[OPT_FIRSTNOTE].minVal;
          }
          if (newSetupOptValue != SetupOpt[OPT_FIRSTNOTE].val){
            SetupOpt[OPT_FIRSTNOTE].val = newSetupOptValue;
            updateOutput(1);
            FastLED.clear();
            updateLED();
          }
          break;
        
        case OPT_BRIGHT:
          newSetupOptValue = encoderPos;
          if (newSetupOptValue > SetupOpt[OPT_BRIGHT].maxVal){
            newSetupOptValue=SetupOpt[OPT_BRIGHT].maxVal;
            encoderPos=SetupOpt[OPT_BRIGHT].maxVal;
          }
          if (newSetupOptValue < SetupOpt[OPT_BRIGHT].minVal){
            newSetupOptValue=SetupOpt[OPT_BRIGHT].minVal;
            encoderPos=SetupOpt[OPT_BRIGHT].minVal;
          }
          if (newSetupOptValue != SetupOpt[OPT_BRIGHT].val){
            SetupOpt[OPT_BRIGHT].val = newSetupOptValue;
            FastLED.setBrightness(SetupOpt[OPT_BRIGHT].val);
            FastLED.show();
            updateOutput(1);
          }
          break;
          
        case OPT_LEDNUMTOT:
          newSetupOptValue = encoderPos;
          if (newSetupOptValue > SetupOpt[OPT_LEDNUMTOT].maxVal){
            newSetupOptValue=SetupOpt[OPT_LEDNUMTOT].maxVal;
            encoderPos=SetupOpt[OPT_LEDNUMTOT].maxVal;
          }
          if (newSetupOptValue < SetupOpt[OPT_LEDNUMTOT].minVal){
            newSetupOptValue=SetupOpt[OPT_LEDNUMTOT].minVal;
            encoderPos=SetupOpt[OPT_LEDNUMTOT].minVal;
          }
          if (newSetupOptValue != SetupOpt[OPT_LEDNUMTOT].val){
            SetupOpt[OPT_LEDNUMTOT].val = newSetupOptValue;
            updateOutput(1);
            FastLED.clear();
            updateLED();
          }
          break;

        case OPT_FIRSTLED:
          newSetupOptValue = encoderPos;
          if (newSetupOptValue > SetupOpt[OPT_LEDNUMTOT].val){
            newSetupOptValue=SetupOpt[OPT_LEDNUMTOT].val;
            encoderPos=SetupOpt[OPT_LEDNUMTOT].val;
          }
          if (newSetupOptValue < SetupOpt[OPT_FIRSTLED].minVal){
            newSetupOptValue=SetupOpt[OPT_FIRSTLED].minVal;
            encoderPos=SetupOpt[OPT_FIRSTLED].minVal;
          }
          if (newSetupOptValue != SetupOpt[OPT_FIRSTLED].val){
            SetupOpt[OPT_FIRSTLED].val = newSetupOptValue;
            updateOutput(1);
            FastLED.clear();
            updateLED();
          }
          break;
                    
        case OPT_LEDPERKEY:
          newSetupOptValue = encoderPos;
          if (newSetupOptValue > SetupOpt[OPT_LEDPERKEY].maxVal){
            newSetupOptValue=SetupOpt[OPT_LEDPERKEY].maxVal;
            encoderPos=SetupOpt[OPT_LEDPERKEY].maxVal;
          }
          if (newSetupOptValue < SetupOpt[OPT_LEDPERKEY].minVal){
            newSetupOptValue=SetupOpt[OPT_LEDPERKEY].minVal;
            encoderPos=SetupOpt[OPT_LEDPERKEY].minVal;
          }
          if (newSetupOptValue != SetupOpt[OPT_LEDPERKEY].val){
            SetupOpt[OPT_LEDPERKEY].val = newSetupOptValue;
            updateOutput(1);
            FastLED.clear();
            updateLED();
          }
          break;

        case OPT_SKIPLED1:
          newSetupOptValue = encoderPos;
          if (newSetupOptValue > SetupOpt[OPT_LEDNUMTOT].val + 1){
            newSetupOptValue=SetupOpt[OPT_LEDNUMTOT].val + 1;
            encoderPos=SetupOpt[OPT_LEDNUMTOT].val + 1;
          }
          if (newSetupOptValue < SetupOpt[OPT_SKIPLED1].minVal){
            newSetupOptValue=SetupOpt[OPT_SKIPLED1].minVal;
            encoderPos=SetupOpt[OPT_SKIPLED1].minVal;
          }
          if (newSetupOptValue != SetupOpt[OPT_SKIPLED1].val){
            SetupOpt[OPT_SKIPLED1].val = newSetupOptValue;
            updateOutput(1);
            FastLED.clear();
            updateLED();
          }
          break;

        case OPT_SKIPLED2:
          newSetupOptValue = encoderPos;
          if (newSetupOptValue > SetupOpt[OPT_LEDNUMTOT].val + 1){
            newSetupOptValue=SetupOpt[OPT_LEDNUMTOT].val + 1;
            encoderPos=SetupOpt[OPT_LEDNUMTOT].val + 1;
          }
          if (newSetupOptValue < SetupOpt[OPT_SKIPLED2].minVal){
            newSetupOptValue=SetupOpt[OPT_SKIPLED2].minVal;
            encoderPos=SetupOpt[OPT_SKIPLED2].minVal;
          }
          if (newSetupOptValue != SetupOpt[OPT_SKIPLED2].val){
            SetupOpt[OPT_SKIPLED2].val = newSetupOptValue;
            updateOutput(1);
            FastLED.clear();
            updateLED();
          }
          break;
        
        default:
          break;
      }
    }    
  }
}






void updateLED() {

  int k_offset = 0; //used to shift every LED to the left when hitting the join between segments

  byte notesInMode[modes[currentModeIndex].numNotes];
  notesInMode[0] = notes[currentRootNoteIndex].number;

  for (int i = 0; i < modes[currentModeIndex].numNotes - 1; i++) {
    notesInMode[i + 1] = ( notesInMode[i] + modes[currentModeIndex].steps[i]) % 12;
  }

  for (int i = 0; i < SetupOpt[OPT_NUMKEYS].val; i++) {
    byte currentKey = (notes[SetupOpt[OPT_FIRSTNOTE].val].number + i) % 12;
    bool currentKeyInMode = false;
    for (int j = 0; j < (sizeof(notesInMode) / sizeof(notesInMode[0])); j++) {
      if (notesInMode[j] == currentKey) {
        currentKeyInMode = true;
      }
    }

    for (int k = 0; k < SetupOpt[OPT_LEDPERKEY].val; k++) {

      if ( (SetupOpt[OPT_LEDPERKEY].val * i + (SetupOpt[OPT_FIRSTLED].val - 1) + k + k_offset) < SetupOpt[OPT_LEDNUMTOT].val ) {

        if ( (SetupOpt[OPT_LEDPERKEY].val * i + (SetupOpt[OPT_FIRSTLED].val - 1) + k) == (SetupOpt[OPT_SKIPLED1].val) ) {
          k_offset--; //we will pull everything to the left by 1 LED
        }
        if ( (SetupOpt[OPT_LEDPERKEY].val * i + (SetupOpt[OPT_FIRSTLED].val - 1) + k) == (SetupOpt[OPT_SKIPLED2].val) ) {
          k_offset--; //we will pull everything to the left by 1 LED
        }
        if (currentKey == notes[currentRootNoteIndex].number) {
          leds[SetupOpt[OPT_LEDPERKEY].val * i + (SetupOpt[OPT_FIRSTLED].val - 1) + k + k_offset] = ROOT_COLOR;
        }

        else if (currentKeyInMode) {
          if (currentKey == 0 || currentKey == 2 || currentKey == 3 || currentKey == 5 || currentKey == 7 || currentKey == 8 || currentKey == 10 ) {
            leds[SetupOpt[OPT_LEDPERKEY].val * i + (SetupOpt[OPT_FIRSTLED].val - 1) + k + k_offset] = WHITE_KEY_COLOR;
          }
          else {
            leds[SetupOpt[OPT_LEDPERKEY].val * i + (SetupOpt[OPT_FIRSTLED].val - 1) + k + k_offset] = BLACK_KEY_COLOR;
          }
        }

        else {
          leds[SetupOpt[OPT_LEDPERKEY].val * i + (SetupOpt[OPT_FIRSTLED].val - 1) + k + k_offset] = CRGB::Black; //blank out any LEDs not in use;
        }

      }
    }
  }
  FastLED.show();
}

// to retrieve our strings stored in PROGMEM
char bufferString[16];
char* getString(const char* str) {
  strcpy_P(bufferString, (char*)str);
  return bufferString;
}

ISR(PCINT0_vect) {
  unsigned char result = r.process();
  if (result == DIR_NONE) {
    // do nothing
  }
  else if (result == DIR_CW) {
    encoderPos++;
  }
  else if (result == DIR_CCW) {
    encoderPos--;
  }
}
