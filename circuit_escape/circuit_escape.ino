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

// BOTÕES
#define BTN_UP    21  
#define BTN_DOWN  4
#define BTN_LEFT  3
#define BTN_RIGHT 2
#define BTN_A     8 

// PALETA DE CORES (RGB565)
#define COLOR_UI_BLUE   0x035F 
#define PCB_DARK        0x0100 // Verde escuro de fundo bem elegante
#define SLOT_GRAY       0x5AEB // Cinza médio para os slots desocupados
#define CURSOR_GOLD     0xFDC0 
#define TEXT_WHITE      0xFFFF

// Matriz para guardar o que foi colocado na grade (5 colunas x 4 linhas)
// 0 = Vazio, 1 = RES, 2 = CAP, 3 = WIRE, 4 = LED
int gradeCircuitos[5][4] = {0}; 

// Estado do Componente Selecionado na UI Inferior (Começa com Resistor = 1)
int componenteSelecionado = 1; 

// Variáveis do Cursor (Indexados em 0)
int cursorX = 0; 
int cursorY = 0; 

// Configuração de Layout para Tela de 2.8" (320x240)
const int offsetGridX = 45;   // (320 - (5 * 46)) / 2 = Centralizado perfeitamente
const int offsetGridY = 40;   // Margem superior para o título
const int slotSize = 46;      // Tamanho do slot (42px) + espaçamento (4px)

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

void desenharGrade();
void desenharSlot(int x, int y);
void desenharCursor(int x, int y, uint16_t cor);
void desenharUI();
void atualizarSelecaoMenu();

void setup() {
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, LOW); // LOW acende o LED na sua placa
  
  pinMode(BUZZER, OUTPUT);
  
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_A, INPUT_PULLUP);

  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
  
  tft.begin();
  tft.setRotation(1); // Modo Paisagem (320x240)

  tft.fillScreen(PCB_DARK); 
  desenharGrade();
  desenharUI();
  atualizarSelecaoMenu();
  desenharCursor(cursorX, cursorY, CURSOR_GOLD);

  delay(500);
}

void loop() {
  int oldX = cursorX;
  int oldY = cursorY;
  bool moveu = false;

  // Leitura dos botões
  if (digitalRead(BTN_UP) == LOW)    { cursorY--; moveu = true; delay(150); }
  if (digitalRead(BTN_DOWN) == LOW)  { cursorY++; moveu = true; delay(150); }
  if (digitalRead(BTN_LEFT) == LOW)  { cursorX--; moveu = true; delay(150); }
  if (digitalRead(BTN_RIGHT) == LOW) { cursorX++; moveu = true; delay(150); }

  // Se mover, limita as bordas. Note que a linha 4 (y=4) é o menu inferior!
  if (cursorY < 0) cursorY = 0;
  
  if (cursorY == 4) {
    // Se está no menu inferior, limita o X para os 5 botões de componentes
    if (cursorX < 0) cursorX = 0;
    if (cursorX > 4) cursorX = 4;
  } else {
    // Se está na grade do circuito
    if (cursorX < 0) cursorX = 0;
    if (cursorX > 4) cursorX = 4;
    if (cursorY > 3) cursorY = 4; // Entra no menu inferior se colocar pra baixo na última linha
  }

  if (moveu) {
    tone(BUZZER, 800, 15);
    
    // Apaga o cursor antigo redesenhando o que estava embaixo dele
    if (oldY == 4) {
      desenharUI(); // Redesenha a barra para limpar a borda dourada
      atualizarSelecaoMenu();
    } else {
      desenharSlot(oldX, oldY);
    }
    
    // Desenha o cursor na nova posição
    if (cursorY == 4) {
      // Destaca o botão do menu inferior
      int btnWidth = 54;
      int startX = 15 + (cursorX * 58);
      tft.drawRoundRect(startX - 2, 198, btnWidth + 4, 34, 6, CURSOR_GOLD);
    } else {
      desenharCursor(cursorX, cursorY, CURSOR_GOLD);
    }
  }

  // AÇÃO DO BOTÃO A
  if (digitalRead(BTN_A) == LOW) {
    tone(BUZZER, 1200, 40);
    
    if (cursorY == 4) {
      // Seleciona o componente baseado na coluna atual do menu
      componenteSelecionado = cursorX + 1; // 1=RES, 2=CAP, 3=WIRE, 4=LED, 5=DEL
      atualizarSelecaoMenu();
    } else {
      // Aplica a ação na grade de circuitos
      if (componenteSelecionado == 5) {
        gradeCircuitos[cursorX][cursorY] = 0; // Deleta o componente
      } else {
        gradeCircuitos[cursorX][cursorY] = componenteSelecionado; // Posiciona o componente ativo
      }
      desenharSlot(cursorX, cursorY);
      desenharCursor(cursorX, cursorY, CURSOR_GOLD); // Redesenha o cursor por cima
    }
    delay(250);
  }
}

void desenharGrade() {
  // Cabeçalho Centralizado (320px de largura)
  tft.fillRoundRect(110, 5, 100, 25, 5, COLOR_UI_BLUE);
  tft.setCursor(132, 13);
  tft.setTextColor(TEXT_WHITE);
  tft.setTextSize(1);
  tft.print("PUZZLE 1");

  // Renderiza toda a grade inicial
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 4; j++) {
      desenharSlot(i, j);
    }
  }
}

// Renderiza um slot específico baseado no seu estado atual na matriz
void desenharSlot(int x, int y) {
  int px = offsetGridX + (x * slotSize);
  int py = offsetGridY + (y * slotSize);
  
  // Limpa a área interna do slot
  tft.fillRoundRect(px, py, 42, 42, 4, 0x18C3); // Fundo do slot desocupado
  tft.drawRoundRect(px, py, 42, 42, 4, SLOT_GRAY); // Borda padrão

  int tipo = gradeCircuitos[x][y];
  
  if (tipo == 1) { // RESISTOR
    tft.fillRect(px + 6, py + 16, 30, 10, ILI9341_ORANGE);
    tft.setCursor(px + 12, py + 18);
    tft.setTextColor(TEXT_WHITE); tft.setTextSize(1);
    tft.print("RES");
  } 
  else if (tipo == 2) { // CAPACITOR
    tft.fillRect(px + 6, py + 16, 30, 10, ILI9341_BLUE);
    tft.setCursor(px + 12, py + 18);
    tft.setTextColor(TEXT_WHITE); tft.setTextSize(1);
    tft.print("CAP");
  } 
  else if (tipo == 3) { // WIRE
    tft.fillRect(px + 4, py + 19, 34, 4, ILI9341_WHITE); // Linha central branca
  } 
  else if (tipo == 4) { // LED
    tft.fillCircle(px + 21, py + 21, 10, ILI9341_RED);
    tft.setCursor(px + 13, py + 18);
    tft.setTextColor(TEXT_WHITE); tft.setTextSize(1);
    tft.print("L");
  }
}

void desenharCursor(int x, int y, uint16_t cor) {
  int px = offsetGridX + (x * slotSize);
  int py = offsetGridY + (y * slotSize);
  tft.drawRoundRect(px - 1, py - 1, 44, 44, 5, cor);
  tft.drawRoundRect(px - 2, py - 2, 46, 46, 6, cor);
}

void desenharUI() {
  // Caixa externa do Menu Inferior
  tft.fillRoundRect(10, 195, 300, 40, 6, 0x2104);
  tft.drawRoundRect(10, 195, 300, 40, 6, SLOT_GRAY);
  
  // Desenha os 5 "botões" fixos da barra de ferramentas
  const char* labels[] = {"RES", "CAP", "WIRE", "LED", "DEL"};
  int btnWidth = 54;
  
  for(int i = 0; i < 5; i++) {
    int startX = 15 + (i * 58);
    tft.fillRoundRect(startX, 200, btnWidth, 30, 4, 0x3186);
    tft.setCursor(startX + (btnWidth - (strlen(labels[i]) * 6)) / 2, 211);
    tft.setTextColor(TEXT_WHITE);
    tft.setTextSize(1);
    tft.print(labels[i]);
  }
}

// Aplica uma borda branca ou destaca o componente que está ativo para uso
void atualizarSelecaoMenu() {
  int idx = componenteSelecionado - 1;
  int btnWidth = 54;
  int startX = 15 + (idx * 58);
  // Pinta uma borda branca fina indicando que este componente está selecionado para colocar na tela
  tft.drawRoundRect(startX, 200, btnWidth, 30, 4, ILI9341_WHITE);
  tft.drawRoundRect(startX + 1, 201, btnWidth - 2, 28, 4, ILI9341_WHITE);
}
