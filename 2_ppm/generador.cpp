#include <iostream>
#include <fstream>

using namespace std;

int main() {
    ofstream imagen("img.ppm");

    if (!imagen.is_open()) {
        cout << "Error" << endl;
        return 1;
    }

    int ancho = 255;
    int alto = 255;

    imagen << "P3" << endl;
    imagen << ancho << " " << alto << endl;
    imagen << "255" << endl;

    for (int y = 0; y < alto; y++) {
        for (int x = 0; x < ancho; x++) {
            int r = x % 255;
            int g = y % 255;
            int b = (x * y) % 255;
            
            imagen << r << " " << g << " " << b << endl;
        }
    }

    imagen.close();
    return 0;
}