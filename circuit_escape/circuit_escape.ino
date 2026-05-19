#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// PINOS DA TELA
#define TFT_CS    10
#define TFT_DC    9
#define TFT_RST   14
#define TFT_MOSI  11
#define TFT_SCK   12
#define TFT_MISO  13  
#define TFT_LED   16  
#define BUZZER    15 // Pino I/O ligado ao módulo Low Level

// BOTÕES DO D-PAD E AÇÃO
#define BTN_UP    3   
#define BTN_DOWN  4
#define BTN_LEFT  5
#define BTN_RIGHT 6
#define BTN_A     8 

// BOTÕES DE INTERAÇÃO DO MENU
#define BTN_X     7   // Navega/Troca (Menu ou Seleção)
#define BTN_Y     21  // Confirma / Avança Diálogo

// PALETA DE CORES (RGB565)
#define COLOR_UI_BLUE   0x035F 
#define PCB_DARK        0x0100 
#define SLOT_GRAY       0x5AEB 
#define CURSOR_GOLD     0xFDC0 
#define TEXT_WHITE      0xFFFF
#define TEXT_YELLOW     0xFFE0
#define TEXT_CYAN       0x07FF
#define COLOR_RED       0xF800
#define COLOR_YELLOW    0xFEE0 // Amarelo para o bloco BZR

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
        {"Ola Berta! Pronto para a Fase 1?", "Lembra: resistor e OBRIGATORIO!", "Se ligar LED direto na bateria...", "CUIDADO COM A FUMACA! :D", "Bora montar, prodigio baiano!"}, 5, 
        "PRODIGIO", "Excelente! Circuito OK com a bateria!"
    },
    {
        "Fase 2 - Botao", 
        {"Fase 2 liberada, genio!", "O botao (BTN) apareceu na grade!", "Ele esta fixo esperando conexao.", "Intercepte a corrente com ele!", "Nao aperta muito forte, hein?"}, 5, 
        "MESTRE DO CONTROLE!", "O botao foi interligado perfeitamente!"
    },
    {
        "Fase 3 - Buzzer", 
        {"Fase 3: hora do som!", "Agora o Buzzer (BZR) esta na grade!", "Apenas Bat, Resistor e o bloco BZR.", "O botao descansará nesta fase.", "Bora fazer barulho, baiano!"}, 5, 
        "BARULHO PERFEITO!", "Circuito com som funcionando!"
    },
    {
        "Fase 4 - Completo", 
        {"ULTIMA FASE, campeao!", "O DESAFIO FINAL!", "BTN e BZR estao separados!", "Ligue Bat + Res + LED + BTN + BZR", "Se funcionar... ESCAPOU!"}, 5, 
        "REI DOS CIRCUITOS!", "Circuito perfeito completo!"
    }
};

// Variáveis de Controle do Jogo
int faseAtual = 0;
int selectedPhase = 0;
int dialogoIndex = 0;
bool unlocked[4] = {true, false, false, false};

// Configurações da Grade 5x4
// 0=Vazio, 1=RES, 2=BAT, 3=WIRE, 4=LED, 8=BZR (Fixo amarelo), 9=BTN (Fixo branco)
int gradeCircuitos[5][4] = {0}; 
int componenteSelecionado = 1; 
int componenteFocadoMenu = 1;

int cursorX = 0; 
int cursorY = 0;
const int offsetGridX = 52;   
const int offsetGridY = 25; 
const int slotSize = 43;      

// Posições predeterminadas dos blocos fixos na grade
const int btnFixoX = 1;
const int btnFixoY = 2;
const int bzrFixoX = 3;
const int bzrFixoY = 1;

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST); 

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
  pinMode(TFT_LED, OUTPUT); 
  digitalWrite(TFT_LED, LOW); 
  
  pinMode(BUZZER, OUTPUT); 
  digitalWrite(BUZZER, HIGH); // Módulo Low Level inicia desligado
  
  pinMode(BTN_UP, INPUT_PULLUP); 
  pinMode(BTN_DOWN, INPUT_PULLUP); 
  pinMode(BTN_LEFT, INPUT_PULLUP); 
  pinMode(BTN_RIGHT, INPUT_PULLUP); 
  pinMode(BTN_A, INPUT_PULLUP); 
  pinMode(BTN_X, INPUT_PULLUP); 
  pinMode(BTN_Y, INPUT_PULLUP); 

  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS); 
  tft.begin(); 
  tft.setRotation(1); 
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
      emitirSom(150, 2); 
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
        tft.fillScreen(PCB_DARK); 
        digitalWrite(TFT_LED, LOW); 
        estado = TELA_DESLIGADA;    
        emitirSom(250, 1); 
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
        emitirSom(300, 1); 
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
      emitirSom(20, 1); 
      desenharSlot(oldX, oldY); 
      desenharCursor(cursorX, cursorY, CURSOR_GOLD); 
    }

    if (digitalRead(BTN_A) == LOW) {
      pressionou = true;
      
      // Impede o jogador de apagar ou alterar os blocos fixos gerados pelas regras da fase
      bool celulaBloqueada = false;
      if (faseAtual == 1 && cursorX == btnFixoX && cursorY == btnFixoY) celulaBloqueada = true;
      if (faseAtual == 2 && cursorX == bzrFixoX && cursorY == bzrFixoY) celulaBloqueada = true;
      if (faseAtual == 3 && ((cursorX == btnFixoX && cursorY == btnFixoY) || (cursorX == bzrFixoX && cursorY == bzrFixoY))) celulaBloqueada = true;

      if (celulaBloqueada) {
        emitirSom(300, 1); // Som de erro se tentar sobrescrever componente fixo
      } else {
        emitirSom(60, 1); 
        if (componenteSelecionado == 5) { 
          gradeCircuitos[cursorX][cursorY] = 0; 
        } else {
          gradeCircuitos[cursorX][cursorY] = componenteSelecionado; 
        }
        desenharSlot(cursorX, cursorY); 
        desenharCursor(cursorX, cursorY, CURSOR_GOLD); 
      }
    }

    if (digitalRead(BTN_X) == LOW) {
      pressionou = true;
      emitirSom(40, 1); 
      componenteFocadoMenu++; 
      if (componenteFocadoMenu > 5) componenteFocadoMenu = 1; 
      atualizarVisualMenu(); 
    }

    if (digitalRead(BTN_Y) == LOW) {
      pressionou = true;
      if (digitalRead(BTN_UP) == LOW) { 
        if (validarCircuito()) {
          estado = SUCESSO;
          // Se for a fase 3 ou 4 (fases que envolvem o Buzzer), o hardware apita forte na vitória!
          if (faseAtual >= 2) {
            emitirSom(80, 5);
          } else {
            emitirSom(100, 3);
          }
        } else {
          tft.fillRect(40, 70, 240, 80, COLOR_RED);
          tft.drawRect(40, 70, 240, 80, TEXT_WHITE);
          tft.setCursor(65, 90); tft.setTextColor(TEXT_YELLOW); tft.setTextSize(2);
          tft.print("ERRO ELETRICO!");
          tft.setCursor(55, 120); tft.setTextSize(1);
          tft.print("REVISE COMPONENTES E FIOS!");
          
          emitirSom(600, 1); 
          delay(1400); 
          
          faseAtual = 0;
          selectedPhase = 0;
          estado = MENU; 
        }
      } else {
        emitirSom(60, 1); 
        componenteSelecionado = componenteFocadoMenu; 
        atualizarVisualMenu(); 
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

// --- RENDERIZADORES DE TELA ---
void renderizarEstado() {
  if (estado == TELA_DESLIGADA) return; 
  
  tft.fillScreen(PCB_DARK); 

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
    desenharGrade(); 
    desenharUI(); 
    atualizarVisualMenu(); 
    desenharCursor(cursorX, cursorY, CURSOR_GOLD); 
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

  // Fase 2: Injeta apenas o BTN (ID 9)
  if (faseAtual == 1) {
    gradeCircuitos[btnFixoX][btnFixoY] = 9; 
  }
  // Fase 3: Injeta apenas o BZR (ID 8)
  else if (faseAtual == 2) {
    gradeCircuitos[bzrFixoX][bzrFixoY] = 8; 
  }
  // Fase 4: O Grande Desafio! Injeta ambos separados na grade
  else if (faseAtual == 3) {
    gradeCircuitos[btnFixoX][btnFixoY] = 9; 
    gradeCircuitos[bzrFixoX][bzrFixoY] = 8; 
  }
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
  int countRES = 0, countBAT = 0, countWIRE = 0, countLED = 0, countBTN = 0, countBZR = 0;
  int componentesTotais = 0;
  bool possuiComponenteIsolado = false;

  for (int x = 0; x < 5; x++) {
    for (int y = 0; y < 4; y++) {
      int tipo = gradeCircuitos[x][y];
      
      if (tipo != 0) { 
        componentesTotais++;
        if (tipo == 1) countRES++;
        if (tipo == 2) countBAT++;
        if (tipo == 3) countWIRE++;
        if (tipo == 4) countLED++;
        if (tipo == 8) countBZR++; // Bloco BZR Fixo
        if (tipo == 9) countBTN++; // Bloco BTN Fixo

        if (contarVizinhosConectados(x, y) == 0) {
          possuiComponenteIsolado = true;
        }
      }
    }
  }

  // Validação Elétrica Padrão: Bateria e Resistor são vitais e obrigatórios para todas as fases
  if (possuiComponenteIsolado || componentesTotais < 2 || countBAT < 1 || countRES < 1) {
    return false; 
  }

  switch (faseAtual) {
    case 0: // Fase 1: Bateria + Resistor + LED
      return (countLED >= 1);
      
    case 1: // Fase 2: Bateria + Resistor + LED + Interligado ao BTN fixo
      return (countLED >= 1 && countBTN == 1 && contarVizinhosConectados(btnFixoX, btnFixoY) >= 1);
      
    case 2: // Fase 3: Bateria + Resistor + Interligado ao BZR fixo (Sem exigir LED nem BTN)
      return (countBZR == 1 && contarVizinhosConectados(bzrFixoX, bzrFixoY) >= 1);
      
    case 3: // Fase 4: Desafio Final Integrado (Bat + Res + LED + BTN + BZR todos interconectados)
      return (countLED >= 1 && countBTN == 1 && countBZR == 1 && 
              contarVizinhosConectados(btnFixoX, btnFixoY) >= 1 && 
              contarVizinhosConectados(bzrFixoX, bzrFixoY) >= 1);
      
    default:
      return false;
  }
}

void desenharGrade() {
  tft.fillRoundRect(80, 2, 160, 21, 5, COLOR_UI_BLUE); 
  tft.setCursor(90, 7); tft.setTextColor(TEXT_WHITE); tft.setTextSize(1); 
  tft.print(fases[faseAtual].titulo); 

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
    tft.setCursor(px + 11, py + 16); tft.setTextColor(TEXT_WHITE); tft.print("RES"); 
  } 
  else if (tipo == 2) {  
    tft.fillRect(px + 4, py + 11, 31, 16, 0x7BE0); 
    tft.setCursor(px + 11, py + 15); tft.setTextColor(TEXT_WHITE); tft.print("BAT"); 
  } 
  else if (tipo == 3) {  
    tft.fillRect(px + 3, py + 17, 33, 4, ILI9341_WHITE); 
  } 
  else if (tipo == 4) {  
    tft.fillCircle(px + 19, py + 19, 9, COLOR_RED); 
    tft.setCursor(px + 16, py + 16); tft.setTextColor(TEXT_WHITE); tft.print("L"); 
  }
  else if (tipo == 8) { // Quadrado AMARELO escrito BZR (Buzzer Predeterminado)
    tft.fillRoundRect(px + 2, py + 2, currentSlotSize - 4, currentSlotSize - 4, 3, COLOR_YELLOW);
    tft.setCursor(px + 11, py + 16); tft.setTextColor(PCB_DARK); tft.setTextSize(1);
    tft.print("BZR");
  }
  else if (tipo == 9) { // Quadrado BRANCO escrito BTN (Botão Predeterminado)
    tft.fillRoundRect(px + 2, py + 2, currentSlotSize - 4, currentSlotSize - 4, 3, TEXT_WHITE);
    tft.setCursor(px + 11, py + 16); tft.setTextColor(PCB_DARK); tft.setTextSize(1);
    tft.print("BTN");
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
  const char* labels[] = {"RES", "BAT", "WIRE", "LED", "DEL"}; 
  int btnWidth = 54; 
  for(int i = 0; i < 5; i++) { 
    int startX = 15 + (i * 58); 
    tft.fillRoundRect(startX, 207, btnWidth, 26, 4, 0x3186); 
    tft.setCursor(startX + (btnWidth - (strlen(labels[i]) * 6)) / 2, 216); 
    tft.setTextColor(TEXT_WHITE); tft.print(labels[i]); 
  }
}

void atualizarVisualMenu() {
  desenharUI(); 
  int btnWidth = 54; 

  int idxFoco = componenteFocadoMenu - 1; 
  int startXFoco = 15 + (idxFoco * 58); 
  tft.drawRoundRect(startXFoco - 2, 204, btnWidth + 4, 32, 6, CURSOR_GOLD); 

  int idxAtivo = componenteSelecionado - 1; 
  int startXAtivo = 15 + (idxAtivo * 58); 
  tft.drawRoundRect(startXAtivo, 207, btnWidth, 26, 4, ILI9341_WHITE); 
  tft.drawRoundRect(startXAtivo + 1, 208, btnWidth - 2, 24, 4, ILI9341_WHITE); 
}

void emitirSom(int tempoLigadoMs, int repeticoes) {
  for (int i = 0; i < repeticoes; i++) {
    digitalWrite(BUZZER, LOW);  
    delay(tempoLigadoMs);       
    digitalWrite(BUZZER, HIGH); 
    if (repeticoes > 1) {
      delay(tempoLigadoMs / 2); 
    }
  }
}
