#include <GL/glut.h>
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <string>
#include <cmath> 
#include <windows.h> // Necessário para as threads e funções de som

// --- VARIÁVEIS GLOBAIS GERAIS ---
int pontos = 0;
int vidas = 5; 
int contadorParaVida = 0; 
float naveX = 0.0f;

bool movendoEsquerda = false;
bool movendoDireita = false;

// Tiros do Jogador
float tiroX, tiroY;
bool tiroAtivo = false;

// --- ESTRUTURA PARA MÚLTIPLOS INIMIGOS ---
const int MAX_ALIENS = 15; 
int aliensEmJogo = 1;      

struct Alien {
    bool vivo;
    float x, y;
    
    bool dropAtivo;
    bool eDropDeVida; 
    float dropX, dropY;
    
    bool tiroAtivo;
    float tiroX, tiroY;
};

Alien aliens[MAX_ALIENS];

// --- FUNÇÕES DE SOM ESTILO SUPER BOMBERMAN 4 (SNES 16-BIT) ---

// Som de Explosão (Tiro no Alien) - Simula o estouro clássico da bomba do Bomberman
DWORD WINAPI somAcerto(LPVOID lpParam) {
    Beep(580, 20);
    Beep(320, 25);
    Beep(160, 30);
    Beep(90, 40);
    return 0;
}

// Som de Power-Up (Coletar Drop) - Arpejo saltitante e brilhante de quando se coleta um item
DWORD WINAPI somDrop(LPVOID lpParam) {
    Beep(880, 25);  // Nota Lá
    Beep(1109, 25); // Nota Dó#
    Beep(1318, 25); // Nota Mi
    Beep(1760, 45); // Nota Lá (Oitava acima - Final feliz)
    return 0;
}

// Som de Dano (Tomar Tiro) - O efeito descendente de quando o Bomberman é atingido
DWORD WINAPI somDano(LPVOID lpParam) {
    for (int f = 1600; f > 200; f -= 140) {
        Beep(f, 15); // Deslizamento rápido para frequências graves
    }
    return 0;
}

// --- FUNÇÃO DE SPAWN INTELIGENTE ---
void spawnAlien(int i) {
    bool posicaoValida;
    int tentativas = 0; 
    
    do {
        aliens[i].x = (rand() % 170 - 85) / 100.0f; 
        aliens[i].y = 0.65f + (rand() % 20) / 100.0f; 
        posicaoValida = true;
        
        for (int j = 0; j < aliensEmJogo; j++) {
            if (i != j && aliens[j].vivo) {
                if (fabs(aliens[i].x - aliens[j].x) < 0.15f && fabs(aliens[i].y - aliens[j].y) < 0.15f) {
                    posicaoValida = false;
                    break;
                }
            }
        }
        tentativas++;
    } while (!posicaoValida && tentativas < 20); 
    
    aliens[i].vivo = true;
}

void inicializaAliens() {
    for (int i = 0; i < MAX_ALIENS; i++) {
        aliens[i].vivo = false;
        aliens[i].dropAtivo = false;
        aliens[i].eDropDeVida = false;
        aliens[i].tiroAtivo = false;
    }
}

// --- FUNÇÃO PARA DESENHAR TEXTO ---
void desenhaTexto(float x, float y, std::string texto) {
    glRasterPos2f(x, y);
    for (char c : texto) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }
}

// --- FUNÇÃO DE COLISÃO AABB ---
bool verificaColisao(float x1, float y1, float w1, float h1, 
                    float x2, float y2, float w2, float h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

// --- DESENHO DOS ELEMENTOS ---
void desenhaCena() {
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(1.0f, 1.0f, 1.0f);
    desenhaTexto(-0.95f, 0.9f, "Pontos: " + std::to_string(pontos));
    desenhaTexto(0.65f, 0.9f, "Vidas: " + std::to_string(vidas));

    // 1. JOGADOR (Verde - Formato Escadinha)
    glColor3f(0.0f, 1.0f, 0.0f); 
    glRectf(naveX - 0.1f, -0.9f, naveX + 0.1f, -0.85f);   
    glRectf(naveX - 0.06f, -0.85f, naveX + 0.06f, -0.8f); 
    glRectf(naveX - 0.02f, -0.8f, naveX + 0.02f, -0.75f); 

    // 2. Tiro do Jogador (Amarelo)
    if (tiroAtivo) {
        glColor3f(1.0f, 1.0f, 0.0f);
        glRectf(tiroX - 0.01f, tiroY, tiroX + 0.01f, tiroY + 0.05f);
    }

    // 3. Loop dos Aliens
    for (int i = 0; i < MAX_ALIENS; i++) {
        if (aliens[i].vivo) {
            glColor3f(0.6f, 0.0f, 0.8f); 
            glRectf(aliens[i].x - 0.02f, aliens[i].y + 0.02f, aliens[i].x + 0.02f, aliens[i].y + 0.05f);
            glRectf(aliens[i].x - 0.06f, aliens[i].y - 0.02f, aliens[i].x + 0.06f, aliens[i].y + 0.02f);
            glRectf(aliens[i].x - 0.06f, aliens[i].y - 0.05f, aliens[i].x - 0.03f, aliens[i].y - 0.02f);
            glRectf(aliens[i].x + 0.03f, aliens[i].y - 0.05f, aliens[i].x + 0.06f, aliens[i].y - 0.02f);
        }

        if (aliens[i].dropAtivo) {
            if (aliens[i].eDropDeVida) glColor3f(1.0f, 0.0f, 0.0f); 
            else glColor3f(1.0f, 1.0f, 0.0f); 

            glRectf(aliens[i].dropX - 0.01f, aliens[i].dropY - 0.03f, aliens[i].dropX + 0.01f, aliens[i].dropY + 0.03f); 
            glRectf(aliens[i].dropX - 0.03f, aliens[i].dropY - 0.01f, aliens[i].dropX + 0.03f, aliens[i].dropY + 0.01f); 
        }

        if (aliens[i].tiroAtivo) {
            glColor3f(1.0f, 0.0f, 0.0f); 
            glRectf(aliens[i].tiroX - 0.01f, aliens[i].tiroY - 0.05f, aliens[i].tiroX + 0.01f, aliens[i].tiroY);
        }
    }

    if (vidas <= 0) {
        glColor3f(1.0f, 1.0f, 1.0f);
        desenhaTexto(-0.2f, 0.0f, "GAME OVER!");
        desenhaTexto(-0.35f, -0.1f, "Pressione ESC para sair");
    }

    glutSwapBuffers();
}

// --- LÓGICA DE ATUALIZAÇÃO ---
void atualizar(int valor) {
    if (vidas > 0) {
        if (movendoEsquerda && naveX > -0.9f) naveX -= 0.02f;
        if (movendoDireita && naveX < 0.9f) naveX += 0.02f;

        aliensEmJogo = 1 + (pontos / 4);
        if (aliensEmJogo > MAX_ALIENS) aliensEmJogo = MAX_ALIENS;

        float velTiroAlien = 0.025f + (pontos * 0.001f); 
        if (velTiroAlien > 0.0625f) velTiroAlien = 0.0625f;

        for (int i = 0; i < aliensEmJogo; i++) {
            if (!aliens[i].vivo && !aliens[i].dropAtivo) spawnAlien(i);
        }

        if (tiroAtivo) {
            tiroY += 0.15f; 
            if (tiroY > 1.0f) tiroAtivo = false;
            
            for (int i = 0; i < MAX_ALIENS; i++) {
                if (aliens[i].vivo && tiroAtivo && verificaColisao(tiroX, tiroY, 0.02f, 0.05f, aliens[i].x - 0.06f, aliens[i].y - 0.05f, 0.12f, 0.1f)) {
                    aliens[i].vivo = false; tiroAtivo = false;
                    aliens[i].dropX = aliens[i].x; aliens[i].dropY = aliens[i].y;
                    aliens[i].dropAtivo = true;

                    // Som de explosão clássica de bloco/bomba
                    CreateThread(NULL, 0, somAcerto, NULL, 0, NULL);

                    contadorParaVida++;
                    if (contadorParaVida >= 20) {
                        aliens[i].eDropDeVida = true;
                        contadorParaVida = 0;
                    } else {
                        aliens[i].eDropDeVida = false;
                    }
                }
            }
        }

        for (int i = 0; i < MAX_ALIENS; i++) {
            if (aliens[i].dropAtivo) {
                aliens[i].dropY -= 0.015f; 
                
                if (verificaColisao(naveX - 0.1f, -0.9f, 0.2f, 0.1f, aliens[i].dropX - 0.03f, aliens[i].dropY - 0.03f, 0.06f, 0.06f)) {
                    if (aliens[i].eDropDeVida) {
                        if (vidas < 5) vidas++; 
                    } else {
                        pontos++; 
                    }
                    
                    // Som de coletar item/power-up do SNES
                    CreateThread(NULL, 0, somDrop, NULL, 0, NULL);

                    aliens[i].dropAtivo = false;
                    spawnAlien(i); 
                }
                else if (aliens[i].dropY < -1.0f) {
                    aliens[i].dropAtivo = false;
                    spawnAlien(i); 
                }
            }

            if (aliens[i].vivo && !aliens[i].tiroAtivo && (rand() % 100 < 1)) {
                aliens[i].tiroX = aliens[i].x; aliens[i].tiroY = aliens[i].y - 0.05f;
                aliens[i].tiroAtivo = true;
            }

            if (aliens[i].tiroAtivo) {
                aliens[i].tiroY -= velTiroAlien;
                if (verificaColisao(naveX - 0.05f, -0.9f, 0.1f, 0.1f, aliens[i].tiroX - 0.01f, aliens[i].tiroY - 0.05f, 0.02f, 0.05f)) {
                    vidas--; 
                    aliens[i].tiroAtivo = false;

                    // Som de quando o personagem toma dano
                    CreateThread(NULL, 0, somDano, NULL, 0, NULL);
                }
                if (aliens[i].tiroY < -1.0f) aliens[i].tiroAtivo = false;
            }
        }
    }
    glutPostRedisplay();
    glutTimerFunc(16, atualizar, 0);
}

// --- CONTROLES ---
void tecladoEspecial(int tecla, int x, int y) {
    if (tecla == GLUT_KEY_LEFT) movendoEsquerda = true;
    if (tecla == GLUT_KEY_RIGHT) movendoDireita = true;
}

void tecladoEspecialSolto(int tecla, int x, int y) {
    if (tecla == GLUT_KEY_LEFT) movendoEsquerda = false;
    if (tecla == GLUT_KEY_RIGHT) movendoDireita = false;
}

void tecladoNormal(unsigned char tecla, int x, int y) {
    if ((tecla == ' ' || tecla == 'z' || tecla == 'w') && !tiroAtivo && vidas > 0) {
        tiroX = naveX; tiroY = -0.75f; tiroAtivo = true;
    }
    if (tecla == 27) exit(0); 
}

int main(int argc, char** argv) {
    srand(time(0));
    inicializaAliens(); 
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Space Catch - Edicao UNIVASF 2026");
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glutDisplayFunc(desenhaCena);
    glutSpecialFunc(tecladoEspecial);
    glutSpecialUpFunc(tecladoEspecialSolto); 
    glutKeyboardFunc(tecladoNormal);
    glutTimerFunc(16, atualizar, 0);
    glutMainLoop();
    return 0;
}