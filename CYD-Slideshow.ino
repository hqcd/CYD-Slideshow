#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <XPT2046_Touchscreen.h>
#include <vector>

// --- Pin Definitions ---
#define CYD_MOSI 23
#define CYD_MISO 19
#define CYD_SCK  18
#define CYD_CS    5

#define TOUCH_CS 33
#define TOUCH_IRQ 36
#define TFT_CS   15

// --- Constants ---
const unsigned long SLIDE_DELAY = 10000; // 10 Seconds
const int SCREEN_WIDTH = 320;

// --- Globals ---
TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

std::vector<String> imageFiles;
int currentIndex = 0;
unsigned long lastSlideTime = 0;

// ==========================================
//   OUTPUT FUNCTION
// ==========================================
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

// ==========================================
//   FILE LOADER
// ==========================================
void getImages(fs::FS &fs) {
  Serial.println("Scanning SD...");
  File root = fs.open("/");
  if (!root || !root.isDirectory()) return;

  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String fileName = file.name();
      // Ensure leading slash
      if (!fileName.startsWith("/")) fileName = "/" + fileName;

      // Check extensions
      if (fileName.endsWith(".jpg") || fileName.endsWith(".JPG") || 
          fileName.endsWith(".jpeg") || fileName.endsWith(".JPEG")) {
        // Ignore Mac hidden files
        if (fileName.indexOf("._") == -1) {
          imageFiles.push_back(fileName);
        }
      }
    }
    file = root.openNextFile();
  }
  Serial.print("Found "); Serial.print(imageFiles.size()); Serial.println(" images.");
}

// ==========================================
//   SHOW IMAGE
// ==========================================
void showCurrentImage() {
  if (imageFiles.empty()) return;

  // Wrap index logic
  if (currentIndex >= (int)imageFiles.size()) currentIndex = 0;
  if (currentIndex < 0) currentIndex = (int)imageFiles.size() - 1;

  Serial.print("Loading: "); Serial.println(imageFiles[currentIndex]);
  
  // Draw Image from SD
  TJpgDec.drawSdJpg(0, 0, imageFiles[currentIndex].c_str());
  
  lastSlideTime = millis();
}

// ==========================================
//   SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(500);

  // 1. Silence all SPI devices (Prevents bus conflicts)
  pinMode(TFT_CS, OUTPUT);   digitalWrite(TFT_CS, HIGH);
  pinMode(TOUCH_CS, OUTPUT); digitalWrite(TOUCH_CS, HIGH);
  pinMode(CYD_CS, OUTPUT);   digitalWrite(CYD_CS, HIGH);
  pinMode(TOUCH_IRQ, INPUT_PULLUP); 

  // 2. Init Screen
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  // 3. Init SD
  SPI.begin(CYD_SCK, CYD_MISO, CYD_MOSI, CYD_CS);
  if (!SD.begin(CYD_CS)) {
    tft.setTextColor(TFT_RED);
    tft.println("SD ERROR");
    while(1);
  }

  // 4. Init Touch
  ts.begin();
  ts.setRotation(1);

  // 5. Init JPEG Decoder
  // FIX: Scale 1 = 100% size (320x240)
  TJpgDec.setJpgScale(1); 
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);
  
  // 6. Load & Start
  getImages(SD);
  if (imageFiles.size() > 0) {
    showCurrentImage();
  } else {
    tft.println("No JPGs found.");
  }
}

// ==========================================
//   LOOP
// ==========================================
void loop() {
  // 1. TOUCH CHECK
  // We check the IRQ pin first to avoid SPI conflicts
  if (digitalRead(TOUCH_IRQ) == LOW) {
    if (ts.touched()) {
      
      Serial.println("Action: NEXT");
      currentIndex++;
      showCurrentImage();
      
      // Wait for release (Debounce)
      delay(250);
      while(digitalRead(TOUCH_IRQ) == LOW) delay(50);
    }
  }

  // 2. AUTO TIMER
  if (millis() - lastSlideTime > SLIDE_DELAY) {
    currentIndex++;
    showCurrentImage();
  }
}