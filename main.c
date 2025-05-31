
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <ldg.h>
#include <gif_lib.h>

#define STRINGIFY(x) #x
#define VERSION_LIB(A,B,C) STRINGIFY(A) "." STRINGIFY(B) "." STRINGIFY(C)
#define VERSION_LDG(A,B,C) "GIF encoder from The GIFLib Project (" STRINGIFY(A) "." STRINGIFY(B) "." STRINGIFY(C) ")"

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

/* variables */

int error;

/* functions */

const char * CDECL gifenc_get_lib_version() { return VERSION_LIB(GIFLIB_MAJOR, GIFLIB_MINOR, GIFLIB_RELEASE); }

GifFileType * CDECL gifenc_open(const char *fileName, int width, int height, int bckgrnd, int colors, GifColorType* palette)
{
  GifFileType *gif = EGifOpenFileName(fileName, true, &error);
    
  if (gif)
  {
    gif->SWidth = width;
    gif->SHeight = height;
    gif->SColorResolution = 8;
    gif->SBackGroundColor = MAX(0, MIN(bckgrnd, 255));
    gif->SColorMap = GifMakeMapObject(colors, palette);
  }
  
  return gif;
}

int32_t CDECL gifenc_set_loops(GifFileType *gif, int loops)
{
  if (loops >= 0 && loops <= 0xFFFF)
  {
    unsigned char netscape[12] = "NETSCAPE2.0";
            
    if (GifAddExtensionBlock(&gif->ExtensionBlockCount, &gif->ExtensionBlocks, APPLICATION_EXT_FUNC_CODE, 11, netscape) == GIF_OK)
    {
      unsigned char data[3];
      
      data[0] = 0x01;
      data[1] = loops & 0xFF;
      data[2] = (loops >> 8) & 0xFF;
            
      return GifAddExtensionBlock(&gif->ExtensionBlockCount, &gif->ExtensionBlocks, CONTINUE_EXT_FUNC_CODE, 3, data);
    }
  }
  
  return GIF_ERROR;
}

int32_t CDECL gifenc_add_image(GifFileType *gif, int left, int top, int width, int height, int colors, GifColorType* palette, uint8_t* chunky)
{
  SavedImage frm = {0};
  
  frm.ImageDesc.Left = MAX(0, MIN(left, gif->SWidth - 1));
  frm.ImageDesc.Top = MAX(0, MIN(top, gif->SHeight - 1));
  frm.ImageDesc.Width = ((frm.ImageDesc.Left + width) > gif->SWidth) ? (gif->SWidth - frm.ImageDesc.Left) : width;
  frm.ImageDesc.Height = ((frm.ImageDesc.Top + height) > gif->SHeight) ? (gif->SHeight - frm.ImageDesc.Top) : height;
  frm.ImageDesc.Interlace = false;
  
  if (colors > 0 || !palette)
  {
    frm.ImageDesc.ColorMap = GifMakeMapObject(colors, palette);
  }
  
  frm.RasterBits = (GifByteType*)chunky;
  
  if (GifMakeSavedImage(gif, &frm)) { return GIF_OK; }
  
  return GIF_ERROR;
}

int32_t CDECL gifenc_set_special(GifFileType *gif, int frame_idx, int trnsprnt, int disposal, int delay)
{
  if (trnsprnt > -1 || disposal > 0 || delay > 0)
  {
    GraphicsControlBlock gcb = {0};

    gcb.DisposalMode = disposal;
    gcb.UserInputFlag = false;
    gcb.DelayTime = delay;
    gcb.TransparentColor = trnsprnt;

    return EGifGCBToSavedExtension(&gcb, gif, frame_idx);
  }
  
  return GIF_ERROR;
}

int32_t CDECL gifenc_write(GifFileType *gif) { return (int32_t)EGifSpew(gif); }

int32_t CDECL gifenc_close(GifFileType *gif) { return EGifCloseFile(gif, &error); }

const char * CDECL gifenc_get_last_error(GifFileType *gif) { return GifErrorString(gif->Error); }


/* populate functions list and info for the LDG */

PROC LibFunc[] =
{
  {"gifenc_get_lib_version", "const char* gifenc_get_lib_version();\n", gifenc_get_lib_version},
   
  {"gifenc_open", "GifFileType* gifenc_open(const char *fileName, int width, int height, int bckgrnd, int colors, uint8_t* palette);\n", gifenc_open},
	
	{"gifenc_set_loops", "int32_t gifenc_set_loops(GifFileType *gif, int loops);\n", gifenc_set_loops},

  {"gifenc_add_image", "int32_t gifenc_add_image(GifFileType *gif, int left, int top, int width, int height, int colors, uint8_t* palette, uint8_t* chunky);\n", gifenc_add_image},
  {"gifenc_set_special", "int32_t gifenc_set_special(GifFileType *gif, int frame_idx, int trnsprnt, int disposal, int delay);\n", gifenc_set_special},
  
  {"gifenc_write", "int32_t gifenc_write(GifFileType *gif);\n", gifenc_write},
  {"gifenc_close", "int32_t gifenc_close(GifFileType *gif);\n", gifenc_close},

  {"gifenc_get_last_error", "const char* gifenc_get_last_error(GifFileType *gif);\n", gifenc_get_last_error},
};

LDGLIB LibLdg[] = { { 0x0001, 8, LibFunc, VERSION_LDG(GIFLIB_MAJOR, GIFLIB_MINOR, GIFLIB_RELEASE), 1} };

/*  */

int main(void)
{
  ldg_init(LibLdg);
  return 0;
}
