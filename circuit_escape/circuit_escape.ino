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

// BOTÕES DO D-PAD E AÇÃO (Grade 5x4)
#define BTN_UP    3   // <--- Alterado de volta para o pino 3
#define BTN_DOWN  4
#define BTN_LEFT  5
#define BTN_RIGHT 6
#define BTN_A     8 

// BOTÕES DE INTERAÇÃO DO MENU
#define BTN_X     7   // Avança a seleção do menu (Rotativo)
#define BTN_Y     21  // <--- Renomeado de BTN_SEL para BTN_Y (Confirma a opção do menu)

// PALETA DE CORES (RGB565)
#define COLOR_UI_BLUE   0x035F 
#define PCB_DARK        0x0100 
#define SLOT_GRAY       0x5AEB 
#define CURSOR_GOLD     0xFDC0 
#define TEXT_WHITE      0xFFFF

// Matriz do circuito (5 colunas x 4 linhas)
int gradeCircuitos[5][4] = {0}; 

// Estado do Componente (1=RES, 2=CAP, 3=WIRE, 4=LED, 5=DEL)
int componenteSelecionado = 1; // Componente ATIVO para inserção
int componenteFocadoMenu = 1;   // Destacado temporariamente pelo Botão X

// Variáveis do Cursor da Grade
int cursorX = 0; 
int cursorY = 0; 

// CONFIGURAÇÃO DE LAYOUT (y=25 a y=197)
const int offsetGridX = 52;   
const int offsetGridY = 25;   
const int slotSize = 43;      

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

void desenharGrade();
void desenharSlot(int x, int y);
void desenharCursor(int x, int y, uint16_t cor);
void desenharUI();
void atualizarVisualMenu();

void setup() {
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, LOW); 
  
  pinMode(BUZZER, OUTPUT);
  
  // D-Pad e Ação
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_A, INPUT_PULLUP);

  // Botões de controle do Menu
  pinMode(BTN_X, INPUT_PULLUP);
  pinMode(BTN_Y, INPUT_PULLUP);

  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
  
  tft.begin();
  tft.setRotation(1); 

  tft.fillScreen(PCB_DARK); 
  desenharGrade();
  desenharUI();
  atualizarVisualMenu();
  desenharCursor(cursorX, cursorY, CURSOR_GOLD);

  delay(500);
}

void loop() {
  int oldX = cursorX;
  int oldY = cursorY;
  bool moveu = false;

  // 1. LEITURA DO D-PAD (Movimentação restrita à grade 5x4)
  if (digitalRead(BTN_UP) == LOW)    { cursorY--; moveu = true; delay(150); }
  if (digitalRead(BTN_DOWN) == LOW)  { cursorY++; moveu = true; delay(150); }
  if (digitalRead(BTN_LEFT) == LOW)  { cursorX--; moveu = true; delay(150); }
  if (digitalRead(BTN_RIGHT) == LOW) { cursorX++; moveu = true; delay(150); }

  // Trava estritamente os limites dentro da grade 5x4 (0 a 4 e 0 a 3)
  if (cursorX < 0) cursorX = 0; if (cursorX > 4) cursorX = 4;
  if (cursorY < 0) cursorY = 0; if (cursorY > 3) cursorY = 3;

  if (moveu) {
    tone(BUZZER, 800, 15);
    desenharSlot(oldX, oldY);          // Restaura o slot anterior
    desenharCursor(cursorX, cursorY, CURSOR_GOLD); // Desenha o novo cursor
  }

  // 2. AÇÃO DO BOTÃO A (Insere o componente ativo na célula atual da grade)
  if (digitalRead(BTN_A) == LOW) {
    tone(BUZZER, 1200, 40);
    if (componenteSelecionado == 5) {
      gradeCircuitos[cursorX][cursorY] = 0; // Deleta
    } else {
      gradeCircuitos[cursorX][cursorY] = componenteSelecionado; // Adiciona ativo
    }
    desenharSlot(cursorX, cursorY);
    desenharCursor(cursorX, cursorY, CURSOR_GOLD); 
    delay(250);
  }

  // 3. AÇÃO DO BOTÃO X (Navega de forma rotativa pelo Menu Inferior)
  if (digitalRead(BTN_X) == LOW) {
    tone(BUZZER, 900, 20);
    componenteFocadoMenu++;
    if (componenteFocadoMenu > 5) {
      componenteFocadoMenu = 1; // Volta ao primeiro ("RES")
    }
    atualizarVisualMenu();
    delay(250);
  }

  // 4. AÇÃO DO BOTÃO Y (Pino 21 - Ativa o componente focado no menu)
  if (digitalRead(BTN_Y) == LOW) {
    tone(BUZZER, 1500, 60);
    componenteSelecionado = componenteFocadoMenu; // Define o novo componente padrão para uso
    atualizarVisualMenu();
    delay(250);
  }
}

void desenharGrade() {
  tft.fillRoundRect(110, 2, 100, 21, 5, COLOR_UI_BLUE);
  tft.setCursor(132, 9);
  tft.setTextColor(TEXT_WHITE);
  tft.setTextSize(1);
  tft.print("PUZZLE 1");

  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 4; j++) {
      desenharSlot(i, j);
    }
  }
}

void desenharSlot(int x, int y) {
  int px = offsetGridX + (x * slotSize);
  int py = offsetGridY + (y * slotSize);
  int currentSlotSize = 39; 
  
  tft.fillRect(px - 2, py - 2, currentSlotSize + 4, currentSlotSize + 4, PCB_DARK);
  tft.fillRoundRect(px, py, currentSlotSize, currentSlotSize, 4, 0x18C3); 
  tft.drawRoundRect(px, py, currentSlotSize, currentSlotSize, 4, SLOT_GRAY); 

  int tipo = gradeCircuitos[x][y];
  
  if (tipo == 1) { 
    tft.fillRect(px + 4, py + 14, 31, 10, ILI9341_ORANGE);
    tft.setCursor(px + 11, py + 16);
    tft.setTextColor(TEXT_WHITE); tft.setTextSize(1);
    tft.print("RES");
  } 
  else if (tipo == 2) { 
    tft.fillRect(px + 4, py + 14, 31, 10, ILI9341_BLUE);
    tft.setCursor(px + 11, py + 16);
    tft.setTextColor(TEXT_WHITE); tft.setTextSize(1);
    tft.print("CAP");
  } 
  else if (tipo == 3) { 
    tft.fillRect(px + 3, py + 17, 33, 4, ILI9341_WHITE); 
  } 
  else if (tipo == 4) { 
    tft.fillCircle(px + 19, py + 19, 9, ILI9341_RED);
    tft.setCursor(px + 16, py + 16);
    tft.setTextColor(TEXT_WHITE); tft.setTextSize(1);
    tft.print("L");
  }
}

void desenharCursor(int x, int y, uint16_t cor) {
  int px = offsetGridX + (x * slotSize);
  int py = offsetGridY + (y * slotSize);
  int currentSlotSize = 39;
  tft.drawRoundRect(px - 1, py - 1, currentSlotSize + 2, currentSlotSize + 2, 5, cor);
  tft.drawRoundRect(px - 2, py - 2, currentSlotSize + 4, currentSlotSize + 4, 6, cor);
}

void desenharUI() {
  tft.fillRoundRect(10, 203, 300, 34, 6, 0x2104);
  tft.drawRoundRect(10, 203, 300, 34, 6, SLOT_GRAY);
  
  const char* labels[] = {"RES", "CAP", "WIRE", "LED", "DEL"};
  int btnWidth = 54;
  
  for(int i = 0; i < 5; i++) {
    int startX = 15 + (i * 58);
    tft.fillRoundRect(startX, 207, btnWidth, 26, 4, 0x3186);
    tft.setCursor(startX + (btnWidth - (strlen(labels[i]) * 6)) / 2, 216);
    tft.setTextColor(TEXT_WHITE);
    tft.setTextSize(1);
    tft.print(labels[i]);
  }
}

void atualizarVisualMenu() {
  desenharUI();

  int btnWidth = 54;

  // 1. Borda Dourada: Foco atual do Botão X
  int idxFoco = componenteFocadoMenu - 1;
  int startXFoco = 15 + (idxFoco * 58);
  tft.drawRoundRect(startXFoco - 2, 204, btnWidth + 4, 32, 6, CURSOR_GOLD);

  // 2. Borda Branca Dupla: Componente ativo confirmado pelo Botão Y
  int idxAtivo = componenteSelecionado - 1;
  int startXAtivo = 15 + (idxAtivo * 58);
  tft.drawRoundRect(startXAtivo, 207, btnWidth, 26, 4, ILI9341_WHITE);
  tft.drawRoundRect(startXAtivo + 1, 208, btnWidth - 2, 24, 4, ILI9341_WHITE);
}
