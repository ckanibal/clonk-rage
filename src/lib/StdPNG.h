/* by Sven2, 2001 */
// png file reading functionality

#ifndef INC_STDPNG
#define INC_STDPNG

#include <Standard.h>
#include <png.h>

void PNGAPI CPNGReadFn(png_structp png_ptr, png_bytep data,
                       size_t length);  // reading proc (callback)

class CPNGFile {
 private:
  BYTE *pFile;       // loaded file in mem
  bool fpFileOwned;  // whether file ptr was allocated by this class
  int iFileSize;     // size of file in mem
  int iPixSize;      // size of one pixel in image data mem
  FILE *fp;          // opened file for writing

  BYTE *pFilePtr;  // current pos in file

  bool fWriteMode;      // if set, the following png-structs are write structs
  png_structp png_ptr;  // png main struct
  png_infop info_ptr, end_info;  // png file info

  BYTE *pImageData;  // uncompressed image in memory
  int iRowSize;      // size of one row of data (equals pitch)

  void Read(BYTE *pData, int iLength);  // read from file
  bool DoLoad();  // perform png-file loading after file data ptr has been set
 public:
  unsigned long iWdt, iHgt;                               // image size
  int iBPC, iClrType, iIntrlcType, iCmprType, iFltrType;  // image data info
 public:
  CPNGFile();   // ctor
  ~CPNGFile();  // dtor

  void
  ClearPngStructs();  // clear internal png structs (png_tr, info_ptr etc.);
  void Default();     // zero fields
  void Clear();       // clear loaded file
  bool Load(BYTE *pFile,
            int iSize);          // load from file that is completely in mem
  DWORD GetPix(int iX, int iY);  // get pixel value (rgba) - note that NO BOUNDS
                                 // CHECKS ARE DONE due to performance reasons!
  // Use ONLY for PNG_COLOR_TYPE_RGB_ALPHA!
  const uint32_t *GetRow(int iY) {
    return reinterpret_cast<uint32_t *>(pImageData + iY * iRowSize);
  }
  bool Create(int iWdt, int iHgt, bool fAlpha);  // create empty image
  bool SetPix(int iX, int iY, DWORD dwValue);    // set pixel value
  bool Save(const char *szFilename);  // save current image to file; saving to
                                      // mem is not supported because C4Group
                                      // doesn't support streamed writing
                                      // anyway...

  BYTE *GetImageData() { return pImageData; }  // return raw image data
  int GetBitsPerPixel();  // return number of bits per pixel in raw image data

  friend void PNGAPI
      CPNGReadFn(png_structp png_ptr, png_bytep data, size_t length);
};
#endif  // INC_STDPNG
