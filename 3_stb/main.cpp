#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"

#include <iostream>
#include <vector>
#include <thread>
#include <chrono> 
#include <cstring> 

void processImagePortion(unsigned char* img, int width, int startRow, int endRow, int channels) {
    for (int y = startRow; y < endRow; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = (y * width + x) * channels;
            
            // gray scale filter
            unsigned char r = img[index];
            unsigned char g = img[index + 1];
            unsigned char b = img[index + 2];
            unsigned char gray = static_cast<unsigned char>((r + g + b) / 3);

            img[index] = gray;
            img[index + 1] = gray;
            img[index + 2] = gray;
        }
    }
}

int main() {
    // load image
    int width, height, channels;
    unsigned char *img_seq = stbi_load("input.jpeg", &width, &height, &channels, 3);

    if (!img_seq) {
        std::cerr << "ERROR: 'input.jpeg' not found." << std::endl;
        return 1;
    }

    size_t img_size = width * height * 3;
    unsigned char *img_multi = (unsigned char*)malloc(img_size);
    std::memcpy(img_multi, img_seq, img_size);

    std::cout << "\nSize load image: " << width << "x" << height << std::endl;

    // sequential
    std::cout << "\nSequential processing: " << std::endl;
    auto start_seq = std::chrono::high_resolution_clock::now();
    
    processImagePortion(img_seq, width, 0, height, 3);
    
    auto end_seq = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> dur_seq = end_seq - start_seq;
    std::cout << "Sequential Time: " << dur_seq.count() << " s" << std::endl;
    stbi_write_jpg("sequential.jpg", width, height, 3, img_seq, 100);    // write output img

    // multithread
    int num_threads = std::thread::hardware_concurrency(); 
    if(num_threads == 0) num_threads = 4;

    std::cout << "\nProcessing multithread(" << num_threads << " threads):" << std::endl;
    std::vector<std::thread> threads;
    int rows_per_thread = height / num_threads;

    auto start_multi = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; ++i) {
        int startRow = i * rows_per_thread;
        int endRow = (i == num_threads - 1) ? height : (startRow + rows_per_thread);
        
        threads.emplace_back(processImagePortion, img_multi, width, startRow, endRow, 3);
    }

    for (auto &t : threads) {
        if (t.joinable()) t.join();
    }
    
    auto end_multi = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> dur_multi = end_multi - start_multi;
    std::cout << "Multithread time: " << dur_multi.count() << " s\n" << std::endl;
    stbi_write_jpg("multithread.jpg", width, height, 3, img_multi, 100);    // write output img

    stbi_image_free(img_seq);
    free(img_multi);

    return 0;
}