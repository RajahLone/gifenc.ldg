
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

/* structures */

typedef struct gif_mem_file {
  uint8_t  *data;
  uint32_t size;
  uint32_t offset;
} gif_mem_file;

/* global variables */

static gif_mem_file gif_mf;

GifFileType *gif_write = NULL;

/* internal functions */

static int gifldg_write(GifFileType* gif_ptr, const GifByteType* data, int count)
{
  gif_mem_file *mf = (gif_mem_file *) gif_ptr->UserData;
    
  uint32_t new_size;
  uint8_t * new_mem;

  if (mf->offset + count > mf->size)
  {
    new_size = 2 * mf->size;
      
    if (mf->offset + count > new_size) { new_size = (((mf->offset + count + 15) >> 4) << 4); }
      
    // start realloc
    new_mem = (uint8_t *)ldg_Malloc(new_size);
    
    if (new_mem == NULL) { return 0; }
    
    memcpy(new_mem, mf->data, mf->offset);
     
    ldg_Free(mf->data);

    mf->data = new_mem;
    mf->size = new_size;
    // end realloc
  }
    
  memcpy(mf->data + mf->offset, data, count);
  mf->offset += count;
    
  return count;
}

/* functions */

const char * CDECL gifenc_get_lib_version() { return VERSION_LIB(GIFLIB_MAJOR, GIFLIB_MINOR, GIFLIB_RELEASE); }

int32_t CDECL gifenc_close()
{
  int err = 0;
  
  if (gif_write != NULL) { EGifCloseFile(gif_write, &err); }

  gif_write = NULL;
  
  ldg_Free(gif_mf.data);

  gif_mf.data = NULL;
  gif_mf.size = 0;
  gif_mf.offset = 0;

  return GIF_OK;
}
uint32_t CDECL gifenc_open(int width, int height, int bckgrnd, int colors, const uint8_t *palette)
{
  if (gif_write != NULL) { gifenc_close(); }
  
  int size = (width * height) + 1024 + 7; size &= ~7;
  
  gif_mf.data = ldg_Malloc(size);
  gif_mf.size = size;
  gif_mf.offset = 0;

  if (gif_mf.data == NULL) { return GIF_ERROR; }

  int err = 0;

  gif_write = EGifOpen(&gif_mf, gifldg_write, &err);
    
  if (gif_write)
  {
    gif_write->SWidth = width;
    gif_write->SHeight = height;
    gif_write->SColorResolution = 8;
    gif_write->SBackGroundColor = MAX(0, MIN(bckgrnd, 255));
    
    if ((colors > 0) && (palette != NULL))
    {
      gif_write->SColorMap = GifMakeMapObject(colors, NULL);
         
      if (gif_write->SColorMap)
      {
        for (int c = 0; c < colors; ++c)
        {
          gif_write->SColorMap->Colors[c].Red = *palette++;
          gif_write->SColorMap->Colors[c].Green = *palette++;
          gif_write->SColorMap->Colors[c].Blue = *palette++;
        }
      }
    }
  }
  
  if (gif_write) { return GIF_OK; } else { return GIF_ERROR; }
}

int32_t CDECL gifenc_set_loops(int loops)
{
  if (gif_write == NULL) { return GIF_ERROR; }

  if (loops >= 0 && loops <= 0xFFFF)
  {
    unsigned char netscape[12] = "NETSCAPE2.0";
            
    if (GifAddExtensionBlock(&gif_write->ExtensionBlockCount, &gif_write->ExtensionBlocks, APPLICATION_EXT_FUNC_CODE, 11, netscape) == GIF_OK)
    {
      unsigned char data[3];
      
      data[0] = 0x01;
      data[1] = loops & 0xFF;
      data[2] = (loops >> 8) & 0xFF;
            
      return GifAddExtensionBlock(&gif_write->ExtensionBlockCount, &gif_write->ExtensionBlocks, CONTINUE_EXT_FUNC_CODE, 3, data);
    }
  }
  
  return GIF_ERROR;
}

int32_t CDECL gifenc_add_frame(int left, int top, int width, int height, int colors, const uint8_t *palette, const uint8_t *chunky)
{
  if (gif_write == NULL) { return GIF_ERROR; }
  if (chunky == NULL) { return GIF_ERROR; }

  SavedImage *frm = calloc(1, sizeof(SavedImage));
  SavedImage *ret = NULL;
  
  if (frm)
  {
    frm->ImageDesc.Left = MAX(0, MIN(left, gif_write->SWidth - 1));
    frm->ImageDesc.Top = MAX(0, MIN(top, gif_write->SHeight - 1));
    frm->ImageDesc.Width = ((frm->ImageDesc.Left + width) > gif_write->SWidth) ? (gif_write->SWidth - frm->ImageDesc.Left) : width;
    frm->ImageDesc.Height = ((frm->ImageDesc.Top + height) > gif_write->SHeight) ? (gif_write->SHeight - frm->ImageDesc.Top) : height;
    frm->ImageDesc.Interlace = false;
  
    if ((colors > 0) && (palette != NULL))
    {
      frm->ImageDesc.ColorMap = GifMakeMapObject(colors, NULL);

      if (frm->ImageDesc.ColorMap != NULL)
      {
        for (int c = 0; c < colors; ++c)
        {
          frm->ImageDesc.ColorMap->Colors[c].Red = *palette++;
          frm->ImageDesc.ColorMap->Colors[c].Green = *palette++;
          frm->ImageDesc.ColorMap->Colors[c].Blue = *palette++;
        }
      }
    }
  
    frm->RasterBits = (GifByteType*)chunky;
  
    ret = GifMakeSavedImage(gif_write, frm);
  
    free(frm);
  }
  
  return ret ? GIF_OK : GIF_ERROR;
}

int32_t CDECL gifenc_set_special(int frame_idx, int trnsprnt, int disposal, int delay)
{
  if (gif_write == NULL) { return GIF_ERROR; }

  int ret = GIF_OK;
  
  if (trnsprnt > -1 || disposal > 0 || delay > 0)
  {
    GraphicsControlBlock *gcb = calloc(1, sizeof(GraphicsControlBlock));

    gcb->DisposalMode = disposal;
    gcb->UserInputFlag = false;
    gcb->DelayTime = delay;
    gcb->TransparentColor = trnsprnt;

    ret = EGifGCBToSavedExtension(gcb, gif_write, frame_idx);
    
    free(gcb);
  }
  
  return ret;
}

int32_t CDECL gifenc_write()
{
  if (gif_write == NULL) { return GIF_ERROR; }

  return EGifSpew(gif_write);
}

uint8_t* CDECL gifenc_get_filedata() { if (gif_write == NULL) { return NULL; } return gif_mf.data; }
uint32_t CDECL gifenc_get_filesize() { if (gif_write == NULL) { return 0; } return gif_mf.offset; }

const char * CDECL gifenc_get_last_error() { return GifErrorString(gif_write->Error); }

/* populate functions list and info for the LDG */

PROC LibFunc[] =
{
  {"gifenc_get_lib_version", "const char* gifenc_get_lib_version();\n", gifenc_get_lib_version},
   
  {"gifenc_open", "uint32_t gifenc_open(int width, int height, int bckgrnd, int colors, const uint8_t *palette);\n", gifenc_open},
  
  {"gifenc_set_loops", "int32_t gifenc_set_loops(int loops);\n", gifenc_set_loops},

  {"gifenc_add_frame", "int32_t gifenc_add_image(int left, int top, int width, int height, int colors, const uint8_t *palette, const uint8_t *chunky);\n", gifenc_add_frame},
  {"gifenc_set_special", "int32_t gifenc_set_special(int frame_idx, int trnsprnt, int disposal, int delay);\n", gifenc_set_special},
  
  {"gifenc_write", "int32_t gifenc_write();\n", gifenc_write},
  {"gifenc_get_filedata", "uint8_t* gifenc_get_filedata();\n", gifenc_get_filedata},
  {"gifenc_get_filesize", "uint32_t gifenc_get_filesize();\n", gifenc_get_filesize},
  {"gifenc_close", "int32_t gifenc_close();\n", gifenc_close},

  {"gifenc_get_last_error", "const char* gifenc_get_last_error();\n", gifenc_get_last_error},
};

LDGLIB LibLdg[] = { { 0x0004, 10, LibFunc, VERSION_LDG(GIFLIB_MAJOR, GIFLIB_MINOR, GIFLIB_RELEASE), 1} };

/*  */

int main(void)
{
  ldg_init(LibLdg);
  return 0;
}
