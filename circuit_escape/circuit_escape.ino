#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// PINOS ATUALIZADOS DA TELA
#define TFT_CS    10
#define TFT_DC    9
#define TFT_RST   14
#define TFT_MOSI  11
#define TFT_SCK   12
#define TFT_MISO  13  
#define TFT_LED   16  
#define BUZZER    15

// BOTÕES (BTN_UP movido do pino 1 para o pino 5 para evitar conflito serial)
#define BTN_UP    21  // <--- Mude o fio do pino 1 para o pino 5
#define BTN_DOWN  4
#define BTN_LEFT  3
#define BTN_RIGHT 2
#define BTN_A     8 

#define COLOR_UI_BLUE   0x035F 
#define COLOR_BERTA_V   0x915F 
#define PCB_GREEN       0x03E0 
#define PCB_DARK        0x0200 
#define SLOT_GRAY       0xAD75 
#define CURSOR_GOLD     0xFDC0 
#define COLOR_OUTLINE   0x0000 

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Variáveis do Cursor
int cursorX = 1; 
int cursorY = 1; 
int offsetGridX = 45;
int offsetGridY = 40;
int slotSize = 46;

void desenharGrade();
void desenharCursor(int x, int y, uint16_t cor);
void desenharUI();

void setup() {
  // Configuração do Backlight (Lógica invertida para Protoboard em GND)
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, LOW); // LOW acende o LED no seu circuito
  
  pinMode(BUZZER, OUTPUT);
  
  // Configuração correta de todos os botões no GND
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_A, INPUT_PULLUP);

  // CORREÇÃO CRÍTICA: Inicialização do SPI com os 4 pinos corretos da tela
  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
  
  tft.begin();
  tft.setRotation(1);

  tft.fillScreen(PCB_DARK); 
  desenharGrade();
  desenharUI();
  desenharCursor(cursorX, cursorY, CURSOR_GOLD);

  delay(500);
}

void loop() {
  int oldX = cursorX;
  int oldY = cursorY;
  bool moveu = false;

  // Movimentação por Grid (Lendo LOW quando pressionado contra o GND)
  if (digitalRead(BTN_UP) == LOW)    { cursorY--; moveu = true; delay(150); }
  if (digitalRead(BTN_DOWN) == LOW)  { cursorY++; moveu = true; delay(150); }
  if (digitalRead(BTN_LEFT) == LOW)  { cursorX--; moveu = true; delay(150); }
  if (digitalRead(BTN_RIGHT) == LOW) { cursorX++; moveu = true; delay(150); }

  // Limites da grade (5x4)
  if (cursorX < 0) cursorX = 0; if (cursorX > 4) cursorX = 4;
  if (cursorY < 0) cursorY = 0; if (cursorY > 3) cursorY = 3;

  if (moveu) {
    tone(BUZZER, 800, 20);
    // Apaga cursor antigo redesenhando o slot
    tft.drawRoundRect(offsetGridX + (oldX * slotSize), offsetGridY + (oldY * slotSize), 42, 42, 4, SLOT_GRAY);
    // Desenha novo cursor
    desenharCursor(cursorX, cursorY, CURSOR_GOLD);
  }

  if (digitalRead(BTN_A) == LOW) {
    tone(BUZZER, 1200, 50);
    tft.fillRect(offsetGridX + (cursorX * slotSize) + 10, offsetGridY + (cursorY * slotSize) + 15, 22, 12, ILI9341_ORANGE);
    tft.setCursor(offsetGridX + (cursorX * slotSize) + 12, offsetGridY + (cursorY * slotSize) + 30);
    tft.setTextColor(ILI9341_WHITE); tft.setTextSize(1); tft.print("RES");
    delay(200);
  }
}

void desenharGrade() {
  tft.fillRoundRect(110, 5, 100, 25, 5, COLOR_UI_BLUE);
  tft.setCursor(125, 12);
  tft.setTextColor(ILI9341_WHITE);
  tft.print("PUZZLE 1");

  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 4; j++) {
      tft.fillRoundRect(offsetGridX + (i * slotSize), offsetGridY + (j * slotSize), 42, 42, 4, 0x2104);
      tft.drawRoundRect(offsetGridX + (i * slotSize), offsetGridY + (j * slotSize), 42, 42, 4, SLOT_GRAY);
    }
  }
}

void desenharCursor(int x, int y, uint16_t cor) {
  int px = offsetGridX + (x * slotSize);
  int py = offsetGridY + (y * slotSize);
  tft.drawRoundRect(px - 1, py - 1, 44, 44, 5, cor);
  tft.drawRoundRect(px - 2, py - 2, 46, 46, 6, cor);
}

void desenharUI() {
  tft.fillRoundRect(10, 200, 300, 35, 5, 0x3186);
  tft.drawRoundRect(10, 200, 300, 35, 5, ILI9341_WHITE);
  
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(25, 212); tft.print("RES");
  tft.setCursor(85, 212); tft.print("CAP");
  tft.setCursor(145, 212); tft.print("WIRE");
  tft.setCursor(205, 212); tft.print("LED");
  tft.setCursor(265, 212); tft.print("DEL");
}