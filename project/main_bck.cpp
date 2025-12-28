#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <iomanip>

// WINDOWS
#include <windows.h>
#include <commdlg.h> 

// STB IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" 

// CIMG
#define cimg_display 2 
#include "CImg.h"

using namespace cimg_library;
using namespace std;
using namespace std::chrono;

bool usarMultithread = false;
bool usarGPU = false;

// TERMINAL
class LogTerminal {
    vector<string> historial;
    int maxLineasVisibles = 9;
    int scrollOffset = 0;
public:
    void log(string msg) {
        historial.push_back("> " + msg);
        if (historial.size() > maxLineasVisibles) {
            scrollOffset = historial.size() - maxLineasVisibles;
        } else {
            scrollOffset = 0;
        }
    }
    void logTiempo(string accion, double segundos) {
        stringstream ss;
        ss << fixed << setprecision(4) << segundos;
        log(accion + ": " + ss.str() + " s");
    }
    void moverScroll(int delta) {
        if (historial.empty()) return;
        
        scrollOffset -= delta;
        if (scrollOffset < 0) scrollOffset = 0;
        int maxScroll = (int)historial.size() - maxLineasVisibles;
        if (maxScroll < 0) maxScroll = 0;
        
        if (scrollOffset > maxScroll) scrollOffset = maxScroll;
    }
    void dibujar(CImg<unsigned char>& canvas, int x, int y, int w, int h) {
        unsigned char bg[] = {20, 20, 20};
        unsigned char borde[] = {100, 100, 100};
        unsigned char texto[] = {0, 255, 0};
        unsigned char white[]={255,255,255};
        unsigned char barraColor[] = {80, 80, 80};
        canvas.draw_rectangle(x, y, x + w, y + h, bg);
        canvas.draw_rectangle(x, y, x + w, y + h, borde, 1.0f, ~0U);
        canvas.draw_text(x + 5, y - 12, "LOG / STATUS", white, 0, 1, 11);
        int textY = y + 5;
        for (int i = 0; i < maxLineasVisibles; i++) {
            int indiceReal = scrollOffset + i;
            
            if (indiceReal < historial.size()) {
                canvas.draw_text(x + 5, textY, historial[indiceReal].c_str(), texto, 0, 1, 13);
                textY += 15;
            }
        }
        if (historial.size() > maxLineasVisibles) {
            int barWidth = 6;
            int barX = x + w - barWidth - 2;
            int barY = y + 2;
            int barH = h - 4;

            float ratio = (float)maxLineasVisibles / historial.size();
            int thumbH = (int)(barH * ratio);
            if (thumbH < 10) thumbH = 10;

            float scrollPercent = (float)scrollOffset / (historial.size() - maxLineasVisibles);
            int thumbY = barY + (int)(scrollPercent * (barH - thumbH));

            canvas.draw_rectangle(barX, barY, barX + barWidth, barY + barH, bg);
            canvas.draw_rectangle(barX, thumbY, barX + barWidth, thumbY + thumbH, barraColor);
        }
    }
};

// UTILIDADES
string abrirArchivoDialogo() {
    OPENFILENAME ofn;
    char szFile[260];
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Images\0*.jpg;*.jpeg;*.png;*.bmp;*.ppm\0Todos\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileName(&ofn) == TRUE) return string(ofn.lpstrFile);
    return "";
}

CImg<unsigned char> cargarImagenCompatible(const char* filename) {
    int w, h, channels;
    unsigned char* data = stbi_load(filename, &w, &h, &channels, 3);
    if (!data) throw runtime_error("Error reading file.");
    CImg<unsigned char> img(w, h, 1, 3);
    cimg_forXY(img, x, y) {
        int idx = (y * w + x) * 3;
        img(x, y, 0) = data[idx + 0];
        img(x, y, 1) = data[idx + 1];
        img(x, y, 2) = data[idx + 2];
    }
    stbi_image_free(data);
    return img;
}

void redimensionarProporcional(CImg<unsigned char>& img, int maxW, int maxH) {
    if (img.width() <= maxW && img.height() <= maxH) return;
    double scale = std::min((double)maxW / img.width(), (double)maxH / img.height());
    img.resize((int)(img.width()*scale), (int)(img.height()*scale), -100, -100, 3); 
}

// Función auxiliar para simular Negrita
void dibujarTextoNegrita(CImg<unsigned char>& canvas, int x, int y, 
                         const char* text, const unsigned char* color, int size = 13) {
    canvas.draw_text(x, y, text, color, 0, 1, size);
    canvas.draw_text(x + 1, y, text, color, 0, 1, size);

}

void HsvToRgb(float h, float s, unsigned char v, unsigned char& r, unsigned char& g, unsigned char& b) {
    float c = v * s; // Croma
    float x = c * (1 - abs(fmod(h / 60.0, 2) - 1));
    float m = v - c;

    float r_prime, g_prime, b_prime;

    if (h >= 0 && h < 60) { r_prime = c; g_prime = x; b_prime = 0; }
    else if (h >= 60 && h < 120) { r_prime = x; g_prime = c; b_prime = 0; }
    else if (h >= 120 && h < 180) { r_prime = 0; g_prime = c; b_prime = x; }
    else if (h >= 180 && h < 240) { r_prime = 0; g_prime = x; b_prime = c; }
    else if (h >= 240 && h < 300) { r_prime = x; g_prime = 0; b_prime = c; }
    else { r_prime = c; g_prime = 0; b_prime = x; }

    r = (unsigned char)(r_prime + m);
    g = (unsigned char)(g_prime + m);
    b = (unsigned char)(b_prime + m);
}

// FILTROS
void aplicarFiltroGrises(CImg<unsigned char>& img) {
    #pragma omp parallel for if(usarMultithread)
    cimg_forXY(img, x, y) {
        int r = img(x, y, 0); int g = img(x, y, 1); int b = img(x, y, 2);
        int gray = (r + g + b) / 3;
        img(x, y, 0) = img(x, y, 1) = img(x, y, 2) = gray;
    }
}

void aplicarFiltroInvertir(CImg<unsigned char>& img) {
    #pragma omp parallel for if(usarMultithread)
    cimg_forXY(img, x, y) {
        img(x, y, 0) = 255 - img(x, y, 0);
        img(x, y, 1) = 255 - img(x, y, 1);
        img(x, y, 2) = 255 - img(x, y, 2);
    }
}

void aplicarBlur(CImg<unsigned char>& img) {
    CImg<unsigned char> copia = img;
    int w = img.width(), h = img.height();
    #pragma omp parallel for if(usarMultithread)
    for (int y = 2; y < h - 2; y++) {
        for (int x = 2; x < w - 2; x++) {
            int r=0, g=0, b=0;
            for (int ky = -2; ky <= 2; ky++) 
                for (int kx = -2; kx <= 2; kx++) {
                    r += copia(x + kx, y + ky, 0);
                    g += copia(x + kx, y + ky, 1);
                    b += copia(x + kx, y + ky, 2);
                }
            img(x, y, 0) = r/25; img(x, y, 1) = g/25; img(x, y, 2) = b/25;
        }
    }
}

void aplicarFiltroArcoiris(CImg<unsigned char>& img) {
    int h = img.height();
    
    #pragma omp parallel for if(usarMultithread)
    cimg_forXY(img, x, y) {
        unsigned char intensidad = (img(x, y, 0) + img(x, y, 1) + img(x, y, 2)) / 3;
        float hue = (float)y / (h - 1) * 300.0f;
        
        unsigned char newR, newG, newB;
        HsvToRgb(hue, 1.0f, intensidad, newR, newG, newB);

        img(x, y, 0) = newR;
        img(x, y, 1) = newG;
        img(x, y, 2) = newB;
    }
}

void aplicarFiltroPixelado(CImg<unsigned char>& img) {
    int blockSize = 15;
    int w = img.width();
    int h = img.height();

    #pragma omp parallel for if(usarMultithread)
    for (int y = 0; y < h; y += blockSize) {
        for (int x = 0; x < w; x += blockSize) {
            
            int sumR = 0, sumG = 0, sumB = 0;
            int count = 0;

            for (int by = 0; by < blockSize; by++) {
                for (int bx = 0; bx < blockSize; bx++) {
                    int realX = x + bx;
                    int realY = y + by;

                    if (realX < w && realY < h) {
                        sumR += img(realX, realY, 0);
                        sumG += img(realX, realY, 1);
                        sumB += img(realX, realY, 2);
                        count++;
                    }
                }
            }
            if (count > 0) {
                unsigned char avgR = sumR / count;
                unsigned char avgG = sumG / count;
                unsigned char avgB = sumB / count;

                for (int by = 0; by < blockSize; by++) {
                    for (int bx = 0; bx < blockSize; bx++) {
                        int realX = x + bx;
                        int realY = y + by;
                        if (realX < w && realY < h) {
                            img(realX, realY, 0) = avgR;
                            img(realX, realY, 1) = avgG;
                            img(realX, realY, 2) = avgB;
                        }
                    }
                }
            }
        }
    }
}

// MAIN
int main() {
    const int MENU_W = 240;
    
    CImg<unsigned char> logoImg;
    int alturaLogo = 0;
    try {
        logoImg = cargarImagenCompatible("logo.jpg");
        int targetWidth = 200;
        
        if (logoImg.width() > targetWidth) {
            double ratio = (double)targetWidth / logoImg.width();
            int newHeight = (int)(logoImg.height() * ratio);
            logoImg.resize(targetWidth, newHeight, 1, 3, 3);
        }
        alturaLogo = logoImg.height();
        
    } catch (...) {
        cout << "ADVERTENCIA: No se encontro 'logo.jpg'. Se omitira el logo." << endl;
    }

    const int ALTURA_MENU_SIN_LOGO = 665;
    const int ALTO_TOTAL = ALTURA_MENU_SIN_LOGO + alturaLogo + 5;

    const int MAX_IMG_H = ALTO_TOTAL - 50;
    const int MAX_IMG_W = 450;

    unsigned char colorVacio[] = {51, 51, 51};
    unsigned char colorTexto[] = {150, 150, 150};
    CImg<unsigned char> imgBase(MAX_IMG_W, 400, 1, 3);
    imgBase.draw_rectangle(0, 0, MAX_IMG_W, 400, colorVacio);
    imgBase.draw_text(MAX_IMG_W/2 - 50, 190, "NO IMAGE", colorTexto, 0, 1, 24);

    CImg<unsigned char> imgDisplayOrig = imgBase, imgDisplayProc = imgBase;
    CImg<unsigned char> datosOriginales = imgBase, datosProcesados = imgBase;
    
    LogTerminal terminal;
    terminal.log("Created by: Cristopher Ochoa");
    terminal.log("System started.");
    terminal.log("Waiting for image...");

    CImgDisplay disp(MENU_W + (MAX_IMG_W * 2) + 40, ALTO_TOTAL, "Image Parallel Processing App C++");

    unsigned char white[]={255,255,255}, black[]={26, 23, 28}, orange[]={232, 181, 30}, 
                  green[]={0,200,0}, red[]={207, 102, 121}, persian[]={47, 146, 240}, grey[]={60,60,60}, turquoise[]={26, 185, 141}, cornflower[]={167, 93, 225};

    while (!disp.is_closed()) {
        unsigned char colorFondo[] = {43, 43, 43};
        CImg<unsigned char> canvas(disp.width(), disp.height(), 1, 3);
        canvas.draw_rectangle(0, 0, disp.width(), disp.height(), colorFondo);

        int imgY = 40;

        dibujarTextoNegrita(canvas, MENU_W + 10, 10, "ORIGINAL", white, 18);
        canvas.draw_image(MENU_W + 10, imgY, imgDisplayOrig);
        
        dibujarTextoNegrita(canvas, MENU_W + MAX_IMG_W + 30, 10, "RESULT", white, 18);
        canvas.draw_image(MENU_W + MAX_IMG_W + 30, imgY, imgDisplayProc);

        int bx = 10, bw = 210, bh = 35, y = 20;

        // CARGAR
        canvas.draw_rectangle(bx, y, bx+bw, y+bh, orange);
        dibujarTextoNegrita(canvas, bx+60, y+10, "UPLOAD IMAGE", black);
        if (disp.button()&1 && disp.mouse_x()>=bx && disp.mouse_x()<=bx+bw && disp.mouse_y()>=y && disp.mouse_y()<=y+bh) {
            disp.button();
            string path = abrirArchivoDialogo();
            if(!path.empty()) {
                try {
                    datosOriginales = cargarImagenCompatible(path.c_str());
                    datosProcesados = datosOriginales;
                    imgDisplayOrig = datosOriginales;
                    redimensionarProporcional(imgDisplayOrig, MAX_IMG_W, MAX_IMG_H);
                    imgDisplayProc = imgDisplayOrig;
                    
                    terminal.log("Image uploaded OK.");
                    terminal.log("Size: " + to_string(datosOriginales.width()) + "x" + to_string(datosOriginales.height()));
                } catch(...) { terminal.log("Error loading."); }
            }
            disp.wait(200);
        }
        y += 50;

        // RESET
        canvas.draw_rectangle(bx, y, bx+bw, y+bh, red);
        dibujarTextoNegrita(canvas, bx+60, y+10, "FILTERS RESET", black);
        if (disp.button()&1 && disp.mouse_x()>=bx && disp.mouse_x()<=bx+bw && disp.mouse_y()>=y && disp.mouse_y()<=y+bh) {
            datosProcesados = datosOriginales;
            imgDisplayProc = imgDisplayOrig;
            terminal.log("Filters reset.");
            disp.wait(150);
        }
        y += 65;

        // INTERRUPTORES
        unsigned char* colM = usarMultithread ? turquoise : cornflower;
        canvas.draw_text(bx + 5, y - 12, "PROCESSING OPTIONS", white, 0, 1, 11);
        
        canvas.draw_rectangle(bx, y, bx+bw, y+bh, colM);
        dibujarTextoNegrita(canvas, bx+10, y+10, usarMultithread ? "MULTITHREAD" : "SEQUENTIAL", black);
        if (disp.button()&1 && disp.mouse_x()>=bx && disp.mouse_x()<=bx+bw && disp.mouse_y()>=y && disp.mouse_y()<=y+bh) {
            usarMultithread = !usarMultithread;
            terminal.log(usarMultithread ? "Parallel Mode ON" : "Sequential Mode ACTIVATED");
            disp.wait(200);
        }
        y += 40;

        unsigned char* colG = usarGPU ? turquoise : cornflower;
        canvas.draw_rectangle(bx, y, bx+bw, y+bh, colG);
        dibujarTextoNegrita(canvas, bx+10, y+10, usarGPU ? "GPU (CUDA)" : "CPU", black);
        if (disp.button()&1 && disp.mouse_x()>=bx && disp.mouse_x()<=bx+bw && disp.mouse_y()>=y && disp.mouse_y()<=y+bh) {
            usarGPU = !usarGPU;
            if(usarGPU) terminal.log("GPU Mode Selected");
            else terminal.log("CPU Mode Selected");
            disp.wait(200);
        }
        y += 65;

        // FILTROS
        auto btn = [&](const char* txt, auto func) {
            canvas.draw_rectangle(bx, y, bx+bw, y+bh, persian);
            dibujarTextoNegrita(canvas, bx+70, y+10, txt, black);
            if (disp.button()&1 && disp.mouse_x()>=bx && disp.mouse_x()<=bx+bw && disp.mouse_y()>=y && disp.mouse_y()<=y+bh) {
                if (datosOriginales.width() < 10) {
                    terminal.log("Upload an image first!");
                } else {
                    terminal.log("Processing...");
                    disp.display(canvas);
                    
                    auto t1 = std::chrono::high_resolution_clock::now();
                    
                    if(usarGPU) {
                        terminal.log("Err: There is no CUDA kernel.");
                        func(datosProcesados); 
                    } else {
                        func(datosProcesados);
                    }
                    
                    auto t2 = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double> lapso = t2 - t1;
                    
                    terminal.logTiempo(txt, lapso.count());

                    imgDisplayProc = datosProcesados;
                    redimensionarProporcional(imgDisplayProc, MAX_IMG_W, MAX_IMG_H);
                }
                disp.wait(200);
            }
            y += 45;
        };

        canvas.draw_text(bx + 5, y - 12, "FILTERS", white, 0, 1, 11);
        btn("GRAYSCALE", aplicarFiltroGrises);
        btn("BLUR", aplicarBlur);
        btn("NEGATIVE", aplicarFiltroInvertir);
        btn("RAINBOW", aplicarFiltroArcoiris);
        btn("PIXELATED", aplicarFiltroPixelado);

        int rueda = disp.wheel(); 
        if (rueda != 0) {
            terminal.moverScroll(rueda);
            disp.set_wheel();
        }
        
        // dibujar terminal
        y += 20; 
        terminal.dibujar(canvas, bx, y, bw, 140);
        int termBottom = y + 145;

        if (!logoImg.is_empty()) {
            int logoY = termBottom + 20;
            int logoX = bx + (bw - logoImg.width()) / 2;
            canvas.draw_text(logoX + 5, logoY - 12, "POWERED BY", white, 0, 1, 11);
            canvas.draw_image(logoX, logoY, logoImg);
        }

        disp.display(canvas);
        disp.wait(20);
    }
    return 0;
}