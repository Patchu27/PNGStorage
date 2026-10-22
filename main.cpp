#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <SFML/Graphics/Image.hpp>
#include <math.h>
#include <iostream>


size_t my_fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    std::cout << std::endl;
    if (ptr == NULL || stream == NULL) return 0;
    if (size == 0 || nmemb == 0) return 0;

    size_t total_bytes = size * nmemb;
    unsigned char *dst = (unsigned char *)ptr;
    size_t bytes_read = 0;

    while (bytes_read < total_bytes) {
        int c = fgetc(stream);
        if (c == EOF) break;           // EOF or error; ferror/feof set by fgetc
        dst[bytes_read++] = (unsigned char)c;
        std::cout << "\r    Progress [" << (100.f*(float)bytes_read)/nmemb << "]          ";
    }
        std::cout << std::endl;
    // return number of complete items read
    return bytes_read / size;
}

class CI{//ColorInts//be sure to draw with blendmodenone
public:
constexpr static std::uint32_t intMax = std::numeric_limits<std::uint32_t>::max();

static std::uint32_t colorToInt(const sf::Color& color){
    return (color.r << 24) + (color.g << 16) + (color.b << 8) + color.a - 1;//0,0,0,0 is invalid unless otherwise specified//this fixes blank spaces but keep in mind White will wrap around to 0,0,0,0.
}

static sf::Color intToColor(std::uint32_t input){
       ++input;//this is to ensure that an alpha of 0 is never selected. If you don't do this, empty space will map to an index of 0.
       return sf::Color((input >> 24), (input >> 16), (input >> 8), input);
}
};

long file_length(FILE* file) {
    if (!file) return -1;

    fseek(file, 0, SEEK_END);//remember this function fseek for later it's useful
    long size = ftell(file);
    rewind(file);
    //we do not close the file because that is not the job of this function

    return size;

}

int write_bytes(const char *outPath, const char *inPath) {


    sf::Image tempImg;
    if (!tempImg.loadFromFile(inPath)) std::cout << "Failed to load image\n";    

    size_t length = (size_t)CI::colorToInt(tempImg.getPixel({0,0})) | CI::colorToInt(tempImg.getPixel({1,0}));//this is the true length of the file not the pixel array
   // std::cout << length << std::endl;    

    FILE* out = fopen(outPath, "wb");
    if (!out) return -1;

    for (size_t i = 0; i < length; ++i) {
        if (fputc(static_cast<const int>(*(tempImg.getPixelsPtr()+i+2*4)), out) == EOF) {
            fclose(out);
            return -1;
        }
    }

    fclose(out);
    return 0;
}


int main(void) {
    
    std::cout << "[PNGStorage BEGIN]\n" << std::endl;    

    const char *folder = "./input";
    DIR *dir = opendir(folder);

    if (!dir) {
        perror("opendir");
        return 1;
    }

    struct dirent *entry;
    char filepath[512];

    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        // Build full path
        snprintf(filepath, sizeof(filepath), "%s/%s", folder, entry->d_name);

        // Try opening the file
        FILE *fp = fopen(filepath, "rb");
        if (!fp) {
            perror("fopen");
            continue;   // Skip unreadable files
        }

        printf("Opened: %s", filepath);
    
        const size_t fileLength = file_length(fp);
        std::cout << " (" + std::to_string(fileLength) + " bytes)" << std::endl;
      //std::cout << "File size " + fileLength << std::endl;//why does this cause a segmentation fault??
    
        unsigned int imgSideLength = ceil(sqrtf((float)(fileLength+2*4)/4.f));//this might be off by a bit//rounding to the nearest whole number and dividing by 4..this might not work but check  

        std::uint8_t* pixelArray = new std::uint8_t[fileLength+2*4];//this might need +1 for a terminating character. Check sizes between iterations to make sure it isn't shrinking
        
        rewind(fp);
        
        if (1)
        fread(pixelArray+(2*4), 1, fileLength, fp);
        else
        my_fread(pixelArray+(2*4), 1, fileLength, fp);//this is much slower, and it isn't mine I cheated and used copilot so I could check progress. But I did modify it at least
        //std::cout << imgSideLength << std::endl;

        sf::Image outImage({imgSideLength,imgSideLength},&pixelArray[0]);
       // sf::Image outImage({imgSideLength,imgSideLength},sf::Color(0,0,0,0));
        outImage.setPixel({0,0},CI::intToColor(fileLength>>32));
        outImage.setPixel({1,0},CI::intToColor((fileLength<<32)>>32));

        char outPath[512] = "./output/";//added the "/" to the end
        strncat(outPath, entry->d_name, sizeof(outPath) - strlen(outPath) - 1);

        std::cout << "Attempting to save image to disk... " << std::endl;
        
    
        if ((fileLength+2*4) > 1000000000)//this is 1GB... That may be the limit internally but I can't confirm that. I should probably make a test binary
        strncat(outPath, ".bmp", sizeof(outPath) - strlen(outPath) - 1);
        else 
        strncat(outPath, ".png", sizeof(outPath) - strlen(outPath) - 1);

        if (outImage.saveToFile(outPath)){
            std::cout << "Image saved to disk successfully" << std::endl;
            }
            else
            {
            std::cout << "Image failed to save to disk, aborting" << std::endl; 
            return 1;
            }
        
   

         fclose(fp);
        //end of writing to png
        
        char revertedPath[512] = "./reverted output/";
        strncat(revertedPath, entry->d_name, sizeof(revertedPath) - strlen(revertedPath) - 1);
           
        write_bytes(revertedPath,outPath);
        std::cout << std::endl;
        // Use the file here...
        // e.g., read data, process it, etc.

    }

    closedir(dir);

    std::cout << "[PNGStorage END]" << std::endl;

    return 0;
}
