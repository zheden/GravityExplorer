#include <iostream>
#include <fstream>
#include <gl/glut.h>

using namespace std;

class Image
{
public:
   Image(char* ps, int w, int h);
   ~Image();

   char *pixels;
   int width;
   int height;
}; 

Image::Image(char* ps, int w, int h) : pixels(ps), width(w), height(h) {

}

Image::~Image() {

}

namespace {
   //Converts a four-character array to an integer, using little-endian form
   int toInt(const char* bytes) {
      return (int)(((unsigned char)bytes[3] << 24) |
         ((unsigned char)bytes[2] << 16) |
         ((unsigned char)bytes[1] << 8) |
         (unsigned char)bytes[0]);
   }

   //Reads the next four bytes as an integer, using little-endian form
   int readInt(ifstream &input) {
      char buffer[4];
      input.read(buffer, 4);
      return toInt(buffer);
   }
}

Image* loadBMP(const char* filename) {
   ifstream infile(filename, ifstream::binary);
   infile.seekg(10, std::ios::cur);
   int dataOffset = readInt(infile);

   //Read the header
   int headerSize = readInt(infile);
   int width = readInt(infile);
   int height = readInt(infile);

   //Read the data
   int bytesPerRow = ((width * 3 + 3) / 4) * 4 - (width * 3 % 4);
   int size = bytesPerRow * height;
   char* pixels = new char[size];
   infile.seekg(dataOffset, ios_base::beg);
   infile.read(pixels, size);
   infile.close();
   //Get the data into the right format
   char* pixels2 = new char[width * height * 3];
   for(int y = 0; y < height; y++) {
      for(int x = 0; x < width; x++) {
         for(int c = 0; c < 3; c++) {
            pixels2[3*(width*y + x) + c] = pixels[bytesPerRow*y + 3*x + (2 - c)];
         }
      }
   }
   delete[] pixels;
   return new Image(pixels2, width, height);
}

GLuint loadTextureFromImage(Image* image) {
   GLuint textureId;
   glGenTextures(1, &textureId); //Make room for our texture
   glBindTexture(GL_TEXTURE_2D, textureId);
   //Map the image to the texture
   glTexImage2D(GL_TEXTURE_2D, 
      0,
      GL_RGB,
      image->width, image->height,
      0,
      GL_RGB,
      GL_UNSIGNED_BYTE,
      image->pixels);
   return textureId;
}

