#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int main() {
    ifstream entrada("img.ppm");
    ofstream salida("img_azul.ppm");

    if (!entrada.is_open()) {
        cout << "Error" << endl;
        return 1;
    }

    string tipo;
    int ancho, alto, maxVal;
    entrada >> tipo >> ancho >> alto >> maxVal;

    salida << tipo << endl;
    salida << ancho << " " << alto << endl;
    salida << maxVal << endl;

    cout << "Procesando imagen..." << endl;

    int r, g, b;
    int contador = 0;

    while (entrada >> r >> g >> b) {
        
        b = b + 50;
        if (b > 255) b = 255;

        salida << r << " " << g << " " << b << endl;
        contador++;
    }

    cout << "Pixeles procesados: " << contador << endl;
    
    if (contador != ancho * alto) {
        cout << "ADVERTENCIA" << endl;
    } else {
        cout << "Imagen generada" << endl;
    }

    entrada.close();
    salida.close();
    return 0;
}