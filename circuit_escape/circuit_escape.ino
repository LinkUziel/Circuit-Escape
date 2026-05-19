#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// PINOS DA TELA [cite: 1]
#define TFT_CS    10
#define TFT_DC    9
#define TFT_RST   14
#define TFT_MOSI  11
#define TFT_SCK   12
#define TFT_MISO  13  
#define TFT_LED   16  
#define BUZZER    15 // Pino I/O ligado ao módulo Low Level [cite: 1]

// BOTÕES DO D-PAD E AÇÃO [cite: 1]
#define BTN_UP    3   
#define BTN_DOWN  4
#define BTN_LEFT  5
#define BTN_RIGHT 6
#define BTN_A     8 

// BOTÕES DE INTERAÇÃO DO MENU [cite: 1]
#define BTN_X     7   // Navega/Troca (Menu ou Seleção) [cite: 1]
#define BTN_Y     21  // Confirma / Avança Diálogo [cite: 2]

// PALETA DE CORES (RGB565) [cite: 2]
#define COLOR_UI_BLUE   0x035F 
#define PCB_DARK        0x0100 
#define SLOT_GRAY       0x5AEB 
#define CURSOR_GOLD     0xFDC0 
#define TEXT_WHITE      0xFFFF
#define TEXT_YELLOW     0xFFE0
#define TEXT_CYAN       0x07FF
#define COLOR_RED       0xF800

// ESTADOS DO JOGO
enum GameState { TELA_DESLIGADA, MENU, PHASE_SELECT, DIALOGO, PUZZLE, SUCESSO, FINAL };
GameState estado = TELA_DESLIGADA; 

// ESTRUTURA DAS FASES
struct PhaseData {
    const char* titulo;
    const char* intro[5]; 
    int introCount;
    const char* sucessoTitulo;
    const char* sucessoTexto;
};

PhaseData fases[4] = {
    {
        "Fase 1 - Protecao", 
        {"Ola Berta! Pronto para a Fase 1?", "Lembra: resistor e OBRIGATORIO!", "Se ligar LED direto...", "CUIDADO COM A FUMACA! :D", "Bora montar, prodigio baiano!"}, 5, 
        "PRODIGIO", "Excelente! Circuito OK!"
    },
    {
        "Fase 2 - Botao", 
        {"Fase 2 liberada, genio!", "Agora use o Push Button!", "O botao fica no caminho...", "Senao o LED fica sempre aceso!", "Nao aperta muito forte, hein?"}, 5, 
        "MESTRE DO CONTROLE!", "O botao liga/desliga perfeito!"
    },
    {
        "Fase 3 - Buzzer", 
        {"Fase 3: hora do barulho!", "Coloque o Buzzer Passivo!", "Resistor continua obrigatorio.", "Senao o buzzer vai gritar!", "Bora fazer barulho baiano!"}, 5, 
        "BARULHO PERFEITO!", "LED aceso + buzzer tocando!"
    },
    {
        "Fase 4 - Completo", 
        {"ULTIMA FASE, campeao!", "Use TUDO que aprendeu!", "Bat + res + LED + bot + buz", "Se funcionar... ESCAPOU!", "Tesla e eu orgulhosos!"}, 5, 
        "REI DOS CIRCUITOS!", "Circuito perfeito!"
    }
};

// Variáveis de Controle do Jogo
int faseAtual = 0;
int selectedPhase = 0;
int dialogoIndex = 0;
bool unlocked[4] = {true, false, false, false};

// Configurações da Grade 5x4 [cite: 2]
int gradeCircuitos[5][4] = {0}; // [cite: 2]
int componenteSelecionado = 1; // [cite: 3]
int componenteFocadoMenu = 1; // [cite: 3]

int cursorX = 0; // [cite: 4]
int cursorY = 0; // [cite: 4]
const int offsetGridX = 52;   // [cite: 5]
const int offsetGridY = 25; // [cite: 5]
const int slotSize = 43;      // [cite: 6]

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST); // [cite: 6]

// Protótipos das Funções
void desenharGrade();
void desenharSlot(int x, int y);
void desenharCursor(int x, int y, uint16_t cor);
void desenharUI();
void atualizarVisualMenu();
void reiniciarFase();
int contarVizinhosConectados(int x, int y);
bool validarCircuito();
void renderizarEstado();
void emitirSom(int delayUrb, int repeticoes);

void setup() {
  pinMode(TFT_LED, OUTPUT); // [cite: 7]
  digitalWrite(TFT_LED, LOW); // Começa desligada [cite: 7]
  
  // Como o módulo é Low Level, colocamos em HIGH imediatamente para ele iniciar em silêncio
  pinMode(BUZZER, OUTPUT); // [cite: 8]
  digitalWrite(BUZZER, HIGH); 
  
  pinMode(BTN_UP, INPUT_PULLUP); // [cite: 8]
  pinMode(BTN_DOWN, INPUT_PULLUP); // [cite: 8]
  pinMode(BTN_LEFT, INPUT_PULLUP); // [cite: 8]
  pinMode(BTN_RIGHT, INPUT_PULLUP); // [cite: 8]
  pinMode(BTN_A, INPUT_PULLUP); // [cite: 8]
  pinMode(BTN_X, INPUT_PULLUP); // [cite: 9]
  pinMode(BTN_Y, INPUT_PULLUP); // [cite: 9]

  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS); // [cite: 9]
  tft.begin(); // [cite: 9]
  tft.setRotation(1); // [cite: 9]
}

void loop() {
  static unsigned long lastInput = 0;
  if (millis() - lastInput < 180) return; 

  bool pressionou = false;

  if (estado == TELA_DESLIGADA) {
    if (digitalRead(BTN_A) == LOW) {
      pressionou = true;
      digitalWrite(TFT_LED, HIGH); 
      estado = MENU;               
      selectedPhase = 0;
      emitirSom(150, 2); // Som de boot: bipes rápidos em Low Level
    }
  }

  else if (estado == MENU) {
    if (digitalRead(BTN_UP) == LOW || digitalRead(BTN_DOWN) == LOW || digitalRead(BTN_X) == LOW) {
      selectedPhase = (selectedPhase == 0) ? 1 : 0; 
      pressionou = true;
      emitirSom(40, 1);
    }
    if (digitalRead(BTN_Y) == LOW || digitalRead(BTN_A) == LOW) {
      pressionou = true;
      if (selectedPhase == 0) {
        estado = PHASE_SELECT;
        selectedPhase = 0; 
        emitirSom(100, 1);
      } else {
        // --- SELEÇÃO DE SAIR COM DEEP SLEEP ---
        tft.fillScreen(PCB_DARK); 
        digitalWrite(TFT_LED, LOW); // Apaga a tela
        emitirSom(250, 1);          // Som de desligamento
        delay(500);                 // Pequena pausa para garantir que o som acabou
        
        // Configura o pino 8 (BTN_A) para acordar o ESP32 quando for para nível BAIXO (LOW)
        esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_A, 0); 
        
        // Coloca o ESP32 no modo de desligamento profundo
        esp_deep_sleep_start(); 
      }
    }
  }

  else if (estado == PHASE_SELECT) {
    if (digitalRead(BTN_LEFT) == LOW || digitalRead(BTN_UP) == LOW) {
      if (selectedPhase > 0) { selectedPhase--; pressionou = true; emitirSom(40, 1); }
    }
    if (digitalRead(BTN_RIGHT) == LOW || digitalRead(BTN_DOWN) == LOW || digitalRead(BTN_X) == LOW) {
      if (selectedPhase < 3) { selectedPhase++; pressionou = true; emitirSom(40, 1); }
    }
    if (digitalRead(BTN_Y) == LOW || digitalRead(BTN_A) == LOW) {
      pressionou = true;
      if (unlocked[selectedPhase]) {
        faseAtual = selectedPhase;
        dialogoIndex = 0; 
        estado = DIALOGO;
        emitirSom(80, 2);
      } else {
        emitirSom(300, 1); // Erro de travado
      }
    }
  }

  else if (estado == DIALOGO) {
    if (digitalRead(BTN_Y) == LOW || digitalRead(BTN_A) == LOW || digitalRead(BTN_X) == LOW) {
      pressionou = true;
      dialogoIndex++;
      emitirSom(50, 1);
      
      if (dialogoIndex >= fases[faseAtual].introCount) {
        reiniciarFase(); 
        estado = PUZZLE;
      }
    }
  }

  else if (estado == PUZZLE) {
    int oldX = cursorX;
    int oldY = cursorY;
    bool moveu = false;

    if (digitalRead(BTN_UP) == LOW)    { cursorY--; moveu = true; } 
    if (digitalRead(BTN_DOWN) == LOW)  { cursorY++; moveu = true; } 
    if (digitalRead(BTN_LEFT) == LOW)  { cursorX--; moveu = true; } 
    if (digitalRead(BTN_RIGHT) == LOW) { cursorX++; moveu = true; } 

    if (cursorX < 0) cursorX = 0; 
    if (cursorX > 4) cursorX = 4; 
    if (cursorY < 0) cursorY = 0; 
    if (cursorY > 3) cursorY = 3; 

    if (moveu) {
      pressionou = true;
      emitirSom(20, 1); // Clique sutil de movimento
      desenharSlot(oldX, oldY); // [cite: 17]
      desenharCursor(cursorX, cursorY, CURSOR_GOLD); // [cite: 18]
    }

    if (digitalRead(BTN_A) == LOW) {
      pressionou = true;
      emitirSom(60, 1); 
      if (componenteSelecionado == 5) { // [cite: 20]
        gradeCircuitos[cursorX][cursorY] = 0; // [cite: 20]
      } else {
        gradeCircuitos[cursorX][cursorY] = componenteSelecionado; // [cite: 21]
      }
      desenharSlot(cursorX, cursorY); // [cite: 22]
      desenharCursor(cursorX, cursorY, CURSOR_GOLD); // [cite: 22]
    }

    if (digitalRead(BTN_X) == LOW) {
      pressionou = true;
      emitirSom(40, 1); 
      componenteFocadoMenu++; // [cite: 24]
      if (componenteFocadoMenu > 5) componenteFocadoMenu = 1; // [cite: 24]
      atualizarVisualMenu(); // [cite: 25]
    }

    if (digitalRead(BTN_Y) == LOW) {
      pressionou = true;
      if (digitalRead(BTN_UP) == LOW) { 
        if (validarCircuito()) {
          estado = SUCESSO;
          emitirSom(100, 3); // Fanfarra de vitória (3 bipes rápidos)
        } else {
          tft.fillRect(40, 70, 240, 80, COLOR_RED);
          tft.drawRect(40, 70, 240, 80, TEXT_WHITE);
          tft.setCursor(65, 90); tft.setTextColor(TEXT_YELLOW); tft.setTextSize(2);
          tft.print("SEM RESISTOR!");
          tft.setCursor(75, 120); tft.setTextSize(1);
          tft.print("OU CIRCUITO ISOLADO!");
          
          emitirSom(600, 1); // Som bem longo simulando a explosão (600ms ligado em LOW)
          delay(1400); // Complementa o tempo total de resposta
          
          faseAtual = 0;
          selectedPhase = 0;
          estado = MENU; 
        }
      } else {
        emitirSom(60, 1); 
        componenteSelecionado = componenteFocadoMenu; // [cite: 27]
        atualizarVisualMenu(); // [cite: 27]
      }
    }
  }

  else if (estado == SUCESSO) {
    if (digitalRead(BTN_Y) == LOW || digitalRead(BTN_A) == LOW || digitalRead(BTN_X) == LOW) {
      pressionou = true;
      unlocked[faseAtual] = true;
      if (faseAtual < 3) unlocked[faseAtual + 1] = true;
      
      faseAtual++;
      if (faseAtual >= 4) {
        estado = FINAL;
      } else {
        selectedPhase = faseAtual;
        estado = PHASE_SELECT;
      }
      emitirSom(100, 1);
    }
  }

  else if (estado == FINAL) {
    if (digitalRead(BTN_Y) == LOW || digitalRead(BTN_A) == LOW || digitalRead(BTN_X) == LOW) {
      pressionou = true;
      faseAtual = 0;
      estado = MENU;
      unlocked[1] = false; unlocked[2] = false; unlocked[3] = false; 
      emitirSom(150, 2);
    }
  }

  if (pressionou) {
    lastInput = millis();
    renderizarEstado();
  }
}

// --- FUNÇÃO CUSTOMIZADA DE ÁUDIO PARA LOW LEVEL TRIGGER ---
// Gera bipes de teste sem travar a temporização global por onda quadrada em hardware
void emitirSom(int tempoLigadoMs, int repeticoes) {
  for (int i = 0; i < repeticoes; i++) {
    digitalWrite(BUZZER, LOW);  // Põe em 0V -> LIGA o módulo Low Level
    delay(tempoLigadoMs);       // Aguarda o som ser gerado
    digitalWrite(BUZZER, HIGH); // Põe em 3.3V -> DESLIGA o módulo Low Level
    if (repeticoes > 1) {
      delay(tempoLigadoMs / 2); // Intervalo entre bipes se houver repetição
    }
  }
}

// --- RENDERIZADORES DE TELA ---
void renderizarEstado() {
  if (estado == TELA_DESLIGADA) return; 
  
  tft.fillScreen(PCB_DARK); // [cite: 9]

  if (estado == MENU) {
    tft.setCursor(35, 50); tft.setTextColor(TEXT_CYAN); tft.setTextSize(3);
    tft.print("CIRCUIT ESCAPE");
    
    tft.setCursor(45, 120); tft.setTextSize(2);
    if (selectedPhase == 0) {
      tft.setTextColor(TEXT_YELLOW); tft.print("> INICIAR JOGO");
      tft.setCursor(45, 160); tft.setTextColor(TEXT_WHITE); tft.print("  SAIR");
    } else {
      tft.setTextColor(TEXT_WHITE); tft.print("  INICIAR JOGO");
      tft.setCursor(45, 160); tft.setTextColor(TEXT_YELLOW); tft.print("> SAIR");
    }
    tft.setCursor(20, 220); tft.setTextSize(1); tft.setTextColor(SLOT_GRAY);
    tft.print("[X/DPAD] Mover  [Y/A] Confirmar");
  }

  else if (estado == PHASE_SELECT) {
    tft.setCursor(35, 20); tft.setTextColor(TEXT_WHITE); tft.setTextSize(2);
    tft.print("SELECIONE A FASE");

    int cardW = 60;
    int cardH = 80;
    int startX = 20;

    for (int i = 0; i < 4; i++) {
      int x = startX + i * (cardW + 12);
      int y = 70;

      if (i == selectedPhase) {
        tft.drawRect(x - 2, y - 2, cardW + 4, cardH + 4, CURSOR_GOLD);
        tft.drawRect(x - 3, y - 3, cardW + 6, cardH + 6, CURSOR_GOLD);
      }

      uint16_t cardColor = unlocked[i] ? COLOR_UI_BLUE : 0x2104;
      tft.fillRect(x, y, cardW, cardH, cardColor);
      tft.drawRect(x, y, cardW, cardH, SLOT_GRAY);

      tft.setCursor(x + 12, y + 25);
      tft.setTextColor(unlocked[i] ? TEXT_WHITE : SLOT_GRAY); tft.setTextSize(1);
      tft.print("FASE");
      tft.setCursor(x + 24, y + 40);
      tft.print(i + 1);

      if (!unlocked[i]) {
        tft.setCursor(x + 4, y + 60); tft.setTextColor(COLOR_RED); tft.setTextSize(1);
        tft.print("TRAVADA");
      }
    }
    tft.setCursor(15, 180); tft.setTextColor(TEXT_CYAN); tft.setTextSize(1);
    tft.print(fases[selectedPhase].titulo);
  }

  else if (estado == DIALOGO) {
    tft.drawRect(10, 110, 300, 110, TEXT_CYAN);
    tft.fillRect(12, 112, 296, 106, 0x0183);
    
    tft.setCursor(20, 122); tft.setTextColor(TEXT_YELLOW); tft.setTextSize(1);
    tft.print("Prof. Leo:");
    
    tft.setCursor(20, 145); tft.setTextColor(TEXT_WHITE); tft.setTextSize(1);
    tft.print(fases[faseAtual].intro[dialogoIndex]);

    tft.setCursor(190, 200); tft.setTextColor(SLOT_GRAY);
    tft.print("[Y] Proximo");
  }

  else if (estado == PUZZLE) {
    desenharGrade(); // [cite: 9]
    desenharUI(); // [cite: 9]
    atualizarVisualMenu(); // [cite: 10]
    desenharCursor(cursorX, cursorY, CURSOR_GOLD); // [cite: 10]
  }

  else if (estado == SUCESSO) {
    tft.setCursor(40, 50); tft.setTextColor(TEXT_YELLOW); tft.setTextSize(2);
    tft.print(fases[faseAtual].sucessoTitulo);
    
    tft.setCursor(20, 110); tft.setTextColor(TEXT_CYAN); tft.setTextSize(1);
    tft.print(fases[faseAtual].sucessoTexto);
    
    tft.setCursor(40, 180); tft.setTextColor(TEXT_WHITE); tft.setTextSize(1);
    tft.print("Pressione [Y] para Avancar!");
  }

  else if (estado == FINAL) {
    tft.setCursor(25, 30); tft.setTextColor(TEXT_YELLOW); tft.setTextSize(2);
    tft.print("PARABENS! ESCAPOU!");
    tft.setCursor(50, 60); tft.setTextColor(TEXT_CYAN);
    tft.print("Voce merece um 10!");

    tft.fillRect(20, 90, 280, 110, 0x10A2);
    tft.drawRect(20, 90, 280, 110, SLOT_GRAY);
    tft.setCursor(30, 105); tft.setTextColor(TEXT_WHITE); tft.setTextSize(1);
    tft.print("1. 10graca       2. 10orientado");
    tft.setCursor(30, 125);
    tft.print("3. 10controlado  4. 10ajustado");
    tft.setCursor(30, 145);
    tft.print("5. 10miolado     6. 10regulado");
    tft.setCursor(30, 165);
    tft.print("7. 10virado      8. 10esperado");

    tft.setCursor(20, 215); tft.setTextColor(TEXT_YELLOW);
    tft.print("[Y] Reiniciar a jornada, mestre!");
  }
}

// --- CONTROLES AUXILIARES DO PUZZLE ---
void reiniciarFase() {
  memset(gradeCircuitos, 0, sizeof(gradeCircuitos)); 
  cursorX = 0; 
  cursorY = 0;
  componenteSelecionado = 1;
  componenteFocadoMenu = 1;
}

int contarVizinhosConectados(int x, int y) {
  int vizinhos = 0;
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};

  for (int i = 0; i < 4; i++) {
    int nx = x + dx[i];
    int ny = y + dy[i];
    if (nx >= 0 && nx < 5 && ny >= 0 && ny < 4) {
      if (gradeCircuitos[nx][ny] != 0) { 
        vizinhos++;
      }
    }
  }
  return vizinhos;
}

bool validarCircuito() {
  int countRES = 0, countLED = 0, countCAP = 0, countWIRE = 0;
  int componentesTotais = 0;
  bool possuiComponenteIsolado = false;

  for (int x = 0; x < 5; x++) {
    for (int y = 0; y < 4; y++) {
      int tipo = gradeCircuitos[x][y];
      
      if (tipo != 0) { 
        componentesTotais++;
        if (tipo == 1) countRES++;
        if (tipo == 2) countCAP++;
        if (tipo == 3) countWIRE++;
        if (tipo == 4) countLED++;

        if (contarVizinhosConectados(x, y) == 0) {
          possuiComponenteIsolado = true;
        }
      }
    }
  }

  if (possuiComponenteIsolado || componentesTotais < 2) {
    return false; 
  }

  switch (faseAtual) {
    case 0: 
      return (countRES >= 1 && countLED >= 1);
    case 1: 
      return (countRES >= 1 && countLED >= 1 && countWIRE >= 1);
    case 2: 
      return (countRES >= 1 && countLED >= 1 && countCAP >= 1);
    case 3: 
      return (countRES >= 1 && countLED >= 1 && countWIRE >= 1 && countCAP >= 1);
    default:
      return false;
  }
}

void desenharGrade() {
  tft.fillRoundRect(80, 2, 160, 21, 5, COLOR_UI_BLUE); // [cite: 28]
  tft.setCursor(90, 7); tft.setTextColor(TEXT_WHITE); tft.setTextSize(1); // [cite: 28]
  tft.print(fases[faseAtual].titulo); // [cite: 28]

  for (int i = 0; i < 5; i++) { // [cite: 29]
    for (int j = 0; j < 4; j++) { // [cite: 29]
      desenharSlot(i, j); // [cite: 29]
    }
  }
}

void desenharSlot(int x, int y) {
  int px = offsetGridX + (x * slotSize); // [cite: 30]
  int py = offsetGridY + (y * slotSize); // [cite: 31]
  int currentSlotSize = 39; // [cite: 31]
  tft.fillRect(px - 2, py - 2, currentSlotSize + 4, currentSlotSize + 4, PCB_DARK); // [cite: 32]
  tft.fillRoundRect(px, py, currentSlotSize, currentSlotSize, 4, 0x18C3); // [cite: 32]
  tft.drawRoundRect(px, py, currentSlotSize, currentSlotSize, 4, SLOT_GRAY);  // [cite: 33]

  int tipo = gradeCircuitos[x][y]; // [cite: 33]
  if (tipo == 1) {  // [cite: 34]
    tft.fillRect(px + 4, py + 14, 31, 10, ILI9341_ORANGE); // [cite: 34]
    tft.setCursor(px + 11, py + 16); tft.setTextColor(TEXT_WHITE); tft.print("RES"); // [cite: 35]
  } 
  else if (tipo == 2) {  // [cite: 35]
    tft.fillRect(px + 4, py + 14, 31, 10, ILI9341_BLUE); // [cite: 35]
    tft.setCursor(px + 11, py + 16); tft.setTextColor(TEXT_WHITE); tft.print("CAP"); // [cite: 36]
  } 
  else if (tipo == 3) {  // [cite: 36]
    tft.fillRect(px + 3, py + 17, 33, 4, ILI9341_WHITE); // [cite: 36]
  } 
  else if (tipo == 4) {  // [cite: 37]
    tft.fillCircle(px + 19, py + 19, 9, COLOR_RED); // [cite: 37]
    tft.setCursor(px + 16, py + 16); tft.setTextColor(TEXT_WHITE); tft.print("L"); // [cite: 38]
  }
}

void desenharCursor(int x, int y, uint16_t cor) {
  int px = offsetGridX + (x * slotSize); // [cite: 38]
  int py = offsetGridY + (y * slotSize); // [cite: 39]
  int currentSlotSize = 39; // [cite: 39]
  tft.drawRoundRect(px - 1, py - 1, currentSlotSize + 2, currentSlotSize + 2, 5, cor); // [cite: 40]
  tft.drawRoundRect(px - 2, py - 2, currentSlotSize + 4, currentSlotSize + 4, 6, cor); // [cite: 41]
}

void desenharUI() {
  tft.fillRoundRect(10, 203, 300, 34, 6, 0x2104); // [cite: 42]
  tft.drawRoundRect(10, 203, 300, 34, 6, SLOT_GRAY); // [cite: 42]
  const char* labels[] = {"RES", "CAP", "WIRE", "LED", "DEL"}; // [cite: 43]
  int btnWidth = 54; // [cite: 43]
  for(int i = 0; i < 5; i++) { // [cite: 44]
    int startX = 15 + (i * 58); // [cite: 45]
    tft.fillRoundRect(startX, 207, btnWidth, 26, 4, 0x3186); // [cite: 45]
    tft.setCursor(startX + (btnWidth - (strlen(labels[i]) * 6)) / 2, 216); // [cite: 45]
    tft.setTextColor(TEXT_WHITE); tft.print(labels[i]); // [cite: 45]
  }
}

void atualizarVisualMenu() {
  desenharUI(); // [cite: 46]
  int btnWidth = 54; // [cite: 46]

  int idxFoco = componenteFocadoMenu - 1; // [cite: 46]
  int startXFoco = 15 + (idxFoco * 58); // [cite: 47]
  tft.drawRoundRect(startXFoco - 2, 204, btnWidth + 4, 32, 6, CURSOR_GOLD); // [cite: 47]

  int idxAtivo = componenteSelecionado - 1; // [cite: 48]
  int startXAtivo = 15 + (idxAtivo * 58); // [cite: 49]
  tft.drawRoundRect(startXAtivo, 207, btnWidth, 26, 4, ILI9341_WHITE); // [cite: 49]
  tft.drawRoundRect(startXAtivo + 1, 208, btnWidth - 2, 24, 4, ILI9341_WHITE); // [cite: 50]
}
