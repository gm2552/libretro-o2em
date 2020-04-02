#ifndef _MSC_VER
#include <stdbool.h>
#include <sched.h>
#endif
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#pragma pack(1)
#endif

#include "libretro.h"

#include "audio.h"
#include "config.h"
#include "cpu.h"
#include "crc32.h"
#include "debug.h"
#include "keyboard.h"
#include "score.h"
#include "vdc.h"
#include "vmachine.h"
#include "voice.h"
#include "vpp.h"

#include "wrapalleg.h"

static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_environment_t environ_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;

void retro_set_environment(retro_environment_t cb) { environ_cb = cb; }
void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { audio_cb = cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

void retro_destroybmp(void);

unsigned short int mbmp[TEX_WIDTH * TEX_HEIGHT];
uint8_t soundBuffer[1056];
int SND;
int RLOOP=0;
int joystick_data[2][5]={{0,0,0,0,0},{0,0,0,0,0}};

int contax, o2flag, g74flag, c52flag, jopflag, helpflag;

unsigned long crcx = ~0;

static char scshot[MAXC],
odyssey2[MAXC],
file_v[MAXC],scorefile[MAXC], statefile[MAXC];

extern uint8_t intRAM[];

const uint8_t o2rom[1024] =  {0x84, 0x00, 0x00, 0x84, 0x02, 0x00, 0x00, 0x84, 0x04, 0xc5, 0xad, 0x09, 0xae, 0x14, 0xe7, 0xb8, 0xa1, 0x80, 0x72, 0x18, 
0xfe, 0x39, 0xfd, 0x93, 0x84, 0x06, 0xa5, 0xb5, 0xb9, 0xa2, 0xb8, 0x3d, 0x81, 0xa0, 0x18, 0x10, 0xf0, 0x53, 0x3f, 0xd3, 
0x3c, 0x96, 0x2f, 0xf0, 0x53, 0xc0, 0xa0, 0x18, 0xf0, 0x37, 0xf2, 0x3a, 0x37, 0x53, 0x7f, 0xa0, 0x14, 0x89, 0xb9, 0x3f, 
0xf1, 0x37, 0xd2, 0x14, 0xeb, 0x14, 0x84, 0x0a, 0xfc, 0xe3, 0xa9, 0x1c, 0xfc, 0xe3, 0xaa, 0xf9, 0xf2, 0x5c, 0xd2, 0x73, 
0xb2, 0x7d, 0x92, 0x84, 0xb9, 0x3f, 0xf1, 0x53, 0xbf, 0xa1, 0x04, 0x14, 0x53, 0x7f, 0xab, 0xb8, 0xaa, 0x27, 0x90, 0xb8, 
0xa7, 0xb9, 0x03, 0xfa, 0xe3, 0x90, 0x18, 0x1a, 0xe9, 0x67, 0xfa, 0xe3, 0xaa, 0x04, 0x76, 0x53, 0x3f, 0xab, 0x1c, 0xb8, 
0xaa, 0xfa, 0x90, 0x04, 0x14, 0x53, 0x1f, 0xab, 0xba, 0x00, 0x04, 0x77, 0xfa, 0xac, 0x84, 0x0a, 0x00, 0xb8, 0x7f, 0xb9, 
0xa0, 0x81, 0x53, 0xd6, 0x91, 0x89, 0x7c, 0x99, 0xe7, 0x80, 0x96, 0xa3, 0x89, 0xbc, 0x99, 0xb7, 0xb9, 0xa0, 0x81, 0x43, 
0x28, 0x91, 0x83, 0xaa, 0xc8, 0x80, 0xa9, 0xc8, 0x80, 0x91, 0xc8, 0x19, 0xea, 0xa8, 0x04, 0x95, 0x15, 0xc5, 0x09, 0xae, 
0x89, 0xbc, 0x99, 0xbb, 0xb8, 0xf0, 0xba, 0x06, 0xf8, 0x3a, 0x0a, 0x92, 0xda, 0xba, 0x30, 0xa8, 0x0a, 0xd8, 0x96, 0xdf, 
0xea, 0xc4, 0xf8, 0x47, 0x77, 0xd3, 0x07, 0x53, 0x3f, 0xa9, 0xff, 0x53, 0x3f, 0xd9, 0x96, 0xe5, 0x04, 0xdf, 0x18, 0xea, 
0xbc, 0xbf, 0xff, 0xff, 0x43, 0xc0, 0xaf, 0x64, 0x78, 0x64, 0x76, 0x89, 0xbc, 0x99, 0xb7, 0x83, 0x89, 0xbc, 0x99, 0xaf, 
0x83, 0x89, 0xbc, 0x99, 0xb7, 0x23, 0x20, 0xa8, 0xa9, 0x27, 0xa0, 0x18, 0xe9, 0xfa, 0x14, 0xec, 0xb9, 0xff, 0x91, 0xe9, 
0x02, 0x91, 0x14, 0xe7, 0x34, 0x1c, 0xb8, 0xff, 0xb9, 0x80, 0x27, 0x90, 0xc8, 0xe9, 0x0f, 0x23, 0xf8, 0x90, 0xe8, 0x15, 
0x90, 0x34, 0x27, 0x83, 0x15, 0xc5, 0xb8, 0xa0, 0x80, 0x53, 0xd6, 0x90, 0xd5, 0x05, 0x83, 0x15, 0xc5, 0xb8, 0xa0, 0x80, 
0x43, 0x28, 0x90, 0xd5, 0x05, 0x83, 0x27, 0x90, 0xb8, 0x3f, 0xf0, 0x43, 0x80, 0xa0, 0x14, 0xe7, 0x83, 0x34, 0x76, 0x14, 
0xb0, 0xf2, 0x3d, 0x23, 0x56, 0x34, 0xa2, 0xc5, 0xff, 0xd5, 0x83, 0xfe, 0x53, 0xfe, 0xae, 0xfd, 0x97, 0xf7, 0xf7, 0xf7, 
0xad, 0x27, 0xf7, 0x4e, 0xae, 0xfc, 0x67, 0x37, 0x17, 0x6d, 0xad, 0xf6, 0x6a, 0xfe, 0x37, 0x53, 0x01, 0x2e, 0x53, 0xfe, 
0x4e, 0xae, 0x83, 0x23, 0xf8, 0xb8, 0x10, 0xba, 0x30, 0x90, 0x18, 0xea, 0x71, 0x83, 0xa5, 0x05, 0x76, 0x75, 0x24, 0x78, 
0x14, 0xec, 0xaa, 0xb8, 0x7f, 0x23, 0x08, 0x90, 0xc8, 0xf9, 0x90, 0xc8, 0xfa, 0x47, 0x53, 0x0f, 0xad, 0x34, 0x97, 0xfa, 
0x53, 0x0f, 0xad, 0x34, 0x97, 0x24, 0x32, 0xfc, 0x90, 0xc8, 0xfb, 0x90, 0xc8, 0x03, 0x08, 0xab, 0x44, 0x2c, 0x15, 0xc5, 
0xac, 0xbb, 0x01, 0xb8, 0x3f, 0xf0, 0x43, 0x40, 0xa0, 0xd5, 0x05, 0x83, 0xb8, 0x3e, 0xf0, 0xf2, 0x3a, 0x53, 0x3f, 0xd3, 
0x3b, 0x96, 0x3a, 0x14, 0xec, 0xf0, 0xb8, 0x02, 0xb9, 0x01, 0xd2, 0xe2, 0xba, 0x99, 0x80, 0xc6, 0xce, 0x6a, 0x57, 0x90, 
0x24, 0xfc, 0x81, 0xc6, 0xda, 0x23, 0x59, 0x90, 0x81, 0x6a, 0x57, 0x91, 0x24, 0xfc, 0xb8, 0x3e, 0xf0, 0x43, 0x80, 0xa0, 
0x24, 0x3a, 0xba, 0x01, 0x80, 0xd3, 0x59, 0xc6, 0xef, 0x80, 0x6a, 0x57, 0x90, 0x24, 0xfc, 0x90, 0x81, 0xd3, 0x59, 0x96, 
0xf8, 0x91, 0x24, 0xfc, 0x81, 0x6a, 0x57, 0x91, 0xb8, 0x7f, 0x54, 0x35, 0x23, 0x42, 0x90, 0xc8, 0xb9, 0x01, 0x81, 0x47, 
0x54, 0x29, 0x54, 0x35, 0x23, 0x52, 0x90, 0xc8, 0x81, 0x54, 0x29, 0x54, 0x35, 0x23, 0x56, 0x90, 0xc8, 0x19, 0x81, 0x47, 
0x54, 0x29, 0x54, 0x35, 0x23, 0x4a, 0x90, 0xc8, 0x81, 0x54, 0x29, 0x24, 0x32, 0x53, 0x0f, 0xad, 0x34, 0x4b, 0xfd, 0x90, 
0xc8, 0xfe, 0x90, 0xc8, 0x83, 0x23, 0x02, 0x90, 0xc8, 0x83, 0xb8, 0x40, 0xb9, 0x50, 0xfc, 0x90, 0x91, 0x18, 0x19, 0xfb, 
0x90, 0x03, 0x08, 0x91, 0xbd, 0x0a, 0xb8, 0x46, 0x54, 0x61, 0xbd, 0x0c, 0xb8, 0x4e, 0x54, 0x61, 0xbd, 0x0c, 0xb8, 0x5a, 
0x54, 0x61, 0xbd, 0x0c, 0xb8, 0x5e, 0x54, 0x61, 0x83, 0x34, 0x4b, 0xfd, 0x90, 0x18, 0xfe, 0x90, 0x18, 0x83, 0x07, 0xba, 
0xf8, 0x6a, 0xc9, 0xf6, 0x6d, 0x6a, 0x53, 0x07, 0x17, 0xaa, 0x27, 0x97, 0xa7, 0xf7, 0xea, 0x79, 0xaa, 0x81, 0x5a, 0x83, 
0x54, 0x6a, 0xc6, 0x89, 0x81, 0xda, 0x91, 0x27, 0x17, 0x83, 0x54, 0x6a, 0x96, 0x92, 0x81, 0xda, 0x91, 0x27, 0x83, 0x42, 
0xac, 0x53, 0x0f, 0xad, 0x54, 0xa4, 0x2a, 0xaf, 0xfc, 0x53, 0xf0, 0x47, 0xad, 0x54, 0xa4, 0x83, 0xfd, 0x03, 0xb3, 0xa3, 
0x96, 0xac, 0xad, 0xcd, 0x1d, 0xab, 0x6f, 0xf6, 0xa4, 0xfb, 0x83, 0x0f, 0x0e, 0x0d, 0x02, 0x0a, 0x05, 0x0b, 0x03, 0x06, 
0x0c, 0x09, 0x01, 0x04, 0x08, 0x07, 0x00, 0xbf, 0xff, 0xd5, 0x14, 0xf1, 0xb9, 0xf2, 0xb8, 0x10, 0xba, 0x0b, 0xbb, 0x28, 
0xbc, 0x70, 0xbe, 0x04, 0x34, 0x1c, 0xf9, 0xa3, 0xad, 0x74, 0xea, 0x19, 0x1e, 0x1e, 0xea, 0xd6, 0x34, 0x27, 0x23, 0x4a, 
0x34, 0xa2, 0x34, 0x3d, 0xa9, 0x34, 0x1c, 0x34, 0x6b, 0x34, 0x27, 0xf9, 0x84, 0x08, 0x19, 0x12, 0x0e, 0x12, 0x23, 0x14, 
0x0c, 0x1c, 0x20, 0x26, 0x12, 0x20, 0x20, 0x20, 0x00, 0x0f, 0xff, 0xcf, 0x03, 0xf0, 0x3f, 0xcf, 0x0f, 0x0f, 0x0f, 0xcf, 
0x1c, 0x71, 0xc7, 0xcf, 0x33, 0x33, 0x33, 0xcf, 0x03, 0xf0, 0x3f, 0xef, 0x0f, 0x0f, 0x0f, 0xef, 0x1c, 0x71, 0xc7, 0xef, 
0x33, 0x33, 0x33, 0xef, 0x55, 0x55, 0x55, 0xef, 0x90, 0x10, 0x94, 0x00, 0x21, 0x00, 0x56, 0xdf, 0x53, 0xdd, 0x50, 0xda, 
0x4d, 0xd8, 0x4a, 0xd5, 0x47, 0xd2, 0x21, 0x00, 0x92, 0x20, 0x4a, 0xcd, 0x52, 0xeb, 0x4a, 0xc9, 0x52, 0xe7, 0x4a, 0xc5, 
0x21, 0x00, 0x85, 0x04, 0x85, 0x0c, 0x85, 0x14, 0x85, 0x1c, 0x85, 0x24, 0x21, 0x00, 0x82, 0x1c, 0x21, 0x00, 0x8a, 0x00, 
0x21, 0x00, 0x85, 0x24, 0x85, 0x1c, 0x85, 0x14, 0x85, 0x0c, 0x85, 0x04, 0x21, 0x00, 0x44, 0xff, 0x4d, 0xdd, 0x48, 0xda, 
0x44, 0xd5, 0x42, 0xd3, 0x21, 0x00, 0xf9, 0xaf, 0xfe, 0x39, 0xff, 0xd5, 0x05, 0x83, 0x20, 0x99, 0xfe, 0x84, 0x08, 0x99, 
0xfd, 0x84, 0x08, 0x99, 0xfc, 0x84, 0x08, 0x89, 0x03, 0x84, 0x08, 0x09, 0xa8, 0x89, 0xbc, 0x99, 0x9b, 0xf9, 0x43, 0xf0, 
0x3a, 0x85, 0x27, 0xaa, 0xab, 0x08, 0xa9, 0x12, 0xa2, 0xcb, 0x32, 0xa5, 0x1a, 0x52, 0xa8, 0x1b, 0x72, 0xab, 0xca, 0x92, 
0xae, 0x95, 0xf8, 0x39, 0x83, 0xfb, 0x53, 0x0f, 0xa8, 0xfa, 0x53, 0x0f, 0x47, 0x48, 0xa8, 0xb9, 0x07, 0x23, 0xc7, 0x69, 
0xa3, 0xd8, 0xc6, 0xc6, 0xe9, 0xbd, 0x83, 0x10, 0x1f, 0x0f, 0xff, 0xf0, 0xf1, 0x01, 0x11, 0xbc, 0xff, 0xfb, 0x37, 0x17, 
0xae, 0xfa, 0x6e, 0x1c, 0xf6, 0xd6, 0x6b, 0xad, 0x83, 0xfa, 0x96, 0xe1, 0x83, 0xfb, 0x96, 0xe5, 0x83, 0x27, 0x6b, 0xea, 
0xe6, 0x83, 0xfc, 0x90, 0x18, 0xf8, 0xd2, 0xf9, 0xfb, 0x90, 0x03, 0x08, 0xab, 0x18, 0x54, 0x61, 0x83, 0x53, 0x0f, 0x07, 
0xc6, 0xf0, 0x64, 0xf5};

static int does_file_exist(const char *filename)
{
   struct stat st;
   int result = stat(filename, &st);
   return result == 0;
}

static long filesize(FILE *stream)
{
   long curpos, length;
   curpos = ftell(stream);
   fseek(stream, 0L, SEEK_END);
   length = ftell(stream);
   fseek(stream, curpos, SEEK_SET);
   return length;
}

static bool load_bios(const char *biosname)
{
   //FILE *fn;
   unsigned long crc;
   int i;

   
   memcpy(rom_table[0], o2rom, 1024);
   
   for (i=1; i<8; i++)
      memcpy(rom_table[i],rom_table[0],1024);

   crc = crc32_buf(rom_table[0],1024);

   if (crc==0x8016A315) {
      printf("Magnavox Odyssey2 BIOS ROM loaded (G7000 model)\n");
      app_data.vpp = 0;
      app_data.bios = ROM_O2;
   } else if (crc==0xE20A9F41) {
      printf("Phillips Videopac+ European BIOS ROM loaded (G7400 model)\n");
      app_data.vpp = 1;
      app_data.bios = ROM_G7400;
   } else if (crc==0xA318E8D6) {
      if (!((!o2flag)&&(c52flag))) printf("Phillips Videopac+ French BIOS ROM loaded (G7000 model)\n"); else printf("Ok\n");
      app_data.vpp = 0;
      app_data.bios = ROM_C52;
   } else if (crc==0x11647CA5) {
      if (g74flag) printf("Phillips Videopac+ French BIOS ROM loaded (G7400 model)\n"); else printf(" Ok\n");
      app_data.vpp = 1;
      app_data.bios = ROM_JOPAC;
   } else {
      printf("Bios ROM loaded (unknown version)\n");
      app_data.vpp = 0;
      app_data.bios = ROM_UNKNOWN;
   }

   return true;
}

static bool load_cart(const char *file)
{
   FILE *fn;
   long l;
   int i, nb;

   app_data.crc = crc32_file(file);
   if (app_data.crc == 0xAFB23F89)
      app_data.exrom = 1;  /* Musician */
   if (app_data.crc == 0x3BFEF56B)
      app_data.exrom = 1;  /* Four in 1 Row! */
   if (app_data.crc == 0x9B5E9356)
      app_data.exrom = 1;  /* Four in 1 Row! (french) */

   if (((app_data.crc == 0x975AB8DA) || (app_data.crc == 0xE246A812)) && (!app_data.debug))
   {
      fprintf(stderr,"Error: file %s is an incomplete ROM dump\n",file_v);
      return false;
   }

   fn=fopen(file,"rb");
   if (!fn) {
      fprintf(stderr,"Error loading %s\n",file_v);
      return false;
   }
   printf("Loading: \"%s\"  Size: ",file_v);
   l = filesize(fn);

   if ((l % 1024) != 0) {
      fprintf(stderr,"Error: file %s is an invalid ROM dump\n",file_v);
      return false;
   }

   /* special MegaCART design by Soeren Gust */
   if ((l == 32768) || (l == 65536) || (l == 131072) || (l == 262144) || (l == 524288) || (l == 1048576))
   {
      app_data.megaxrom = 1;
      app_data.bank = 1;
      megarom = malloc(1048576);

      if (megarom == NULL)
      {
         fprintf(stderr, "Out of memory loading %s\n", file);
         return false;
      }
      if (fread(megarom, l, 1, fn) != 1)
      {
         fprintf(stderr,"Error loading %s\n",file);
         return false;
      }

      /* mirror shorter files into full megabyte */
      if (l < 65536)
         memcpy(megarom+32768,megarom,32768);
      if (l < 131072)
         memcpy(megarom+65536,megarom,65536);
      if (l < 262144)
         memcpy(megarom+131072,megarom,131072);
      if (l < 524288)
         memcpy(megarom+262144,megarom,262144);
      if (l < 1048576)
         memcpy(megarom+524288,megarom,524288);
      /* start in bank 0xff */
      memcpy(&rom_table[0][1024], megarom + 4096*255 + 1024, 3072);
      printf("MegaCart %ldK", l / 1024);
      nb = 1;
   }
   else if (((l % 3072) == 0))
   {
      app_data.three_k = 1;
      nb = l/3072;

      for (i=nb-1; i>=0; i--)
      {
         if (fread(&rom_table[i][1024],3072,1,fn) != 1)
         {
            fprintf(stderr,"Error loading %s\n",file);
            return false;
         }
      }
      printf("%dK",nb*3);
  
   } else {

      nb = l/2048;

      if ((nb == 2) && (app_data.exrom))
      {

         if (fread(&extROM[0], 1024,1,fn) != 1)
         {
            fprintf(stderr,"Error loading %s\n",file);
            return false;
         }
         if (fread(&rom_table[0][1024],3072,1,fn) != 1)
         {
            fprintf(stderr,"Error loading %s\n",file);
            return false;
         }
         printf("3K EXROM");

      }
      else
      {
         for (i=nb-1; i>=0; i--)
         {
            if (fread(&rom_table[i][1024],2048,1,fn) != 1)
            {
               fprintf(stderr,"Error loading %s\n",file);
               return false;
            }
            memcpy(&rom_table[i][3072],&rom_table[i][2048],1024); /* simulate missing A10 */
         }
         printf("%dK",nb*2);
      }
   }
   fclose(fn);
   rom = rom_table[0];
   if (nb==1)
      app_data.bank = 1;
   else if (nb==2)
      app_data.bank = app_data.exrom ? 1 : 2;
   else if (nb==4)
      app_data.bank = 3;
   else
      app_data.bank = 4;

   if ((rom_table[nb-1][1024+12]=='O') && (rom_table[nb-1][1024+13]=='P') && (rom_table[nb-1][1024+14]=='N') && (rom_table[nb-1][1024+15]=='B'))
      app_data.openb=1;

   printf("  CRC: %08lX\n",app_data.crc);

   return true;
}

void update_joy(void)
{
}

static void update_input(void)
{
   if (!input_poll_cb)
      return;

   input_poll_cb();

   // Joystick
   // Player 1
   joystick_data[0][0]= input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP);
   joystick_data[0][1]= input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN);
   joystick_data[0][2]= input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT);
   joystick_data[0][3]= input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT);
   joystick_data[0][4]= input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A); // "Action" button on the joystick
   // Player 2
   joystick_data[1][0]= input_state_cb(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP);
   joystick_data[1][1]= input_state_cb(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN);
   joystick_data[1][2]= input_state_cb(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT);
   joystick_data[1][3]= input_state_cb(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT);
   joystick_data[1][4]= input_state_cb(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A); // "Action" button on the joystick

   // Numeric and Alpha
   key[RETROK_0] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_0);
   key[RETROK_1] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_1)
                   | input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y);
   key[RETROK_2] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_2)
                   | input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X);
   key[RETROK_3] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_3)
                   | input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L);
   key[RETROK_4] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_4)
                   | input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R);
   key[RETROK_5] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_5);
   key[RETROK_6] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_6);
   key[RETROK_7] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_7);
   key[RETROK_8] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_8);
   key[RETROK_9] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_9);
   key[RETROK_a] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_a);
   key[RETROK_b] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_b);
   key[RETROK_c] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_c);
   key[RETROK_d] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_d);
   key[RETROK_e] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_e);
   key[RETROK_f] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_f);
   key[RETROK_g] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_g);
   key[RETROK_h] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_h);
   key[RETROK_i] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_i);
   key[RETROK_j] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_j);
   key[RETROK_k] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_k);
   key[RETROK_l] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_l);
   key[RETROK_m] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_m);
   key[RETROK_n] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_n);
   key[RETROK_o] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_o);
   key[RETROK_p] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_p);
   key[RETROK_q] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_q);
   key[RETROK_r] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_r);
   key[RETROK_s] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_s);
   key[RETROK_t] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_t);
   key[RETROK_u] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_u);
   key[RETROK_v] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_v);
   key[RETROK_w] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_w);
   key[RETROK_x] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_x);
   key[RETROK_y] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_y);
   key[RETROK_z] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_z);
   key[RETROK_SPACE] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_SPACE);       // Space
   key[RETROK_QUESTION] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_QUESTION); // ?
   key[RETROK_PERIOD] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_PERIOD);     // .
   key[RETROK_END] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_END);           // "Clear"
   key[RETROK_RETURN] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RETURN);     // "Enter"
   key[RETROK_MINUS] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_MINUS);       // -
   key[RETROK_ASTERISK] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_ASTERISK); // Multiply sign
   key[RETROK_SLASH] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_SLASH);       // Divide sign
   key[RETROK_EQUALS] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_EQUALS);     // =
   key[RETROK_PLUS] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_PLUS);         // +
}

/************************************
 * libretro implementation
 ************************************/

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
	info->library_name = "O2EM";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
	info->library_version = "1.18" GIT_VERSION;
	info->need_fullpath = true;
	info->valid_extensions = "bin";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   memset(info, 0, sizeof(*info));

   info->timing.fps            = (evblclk == EVBLCLK_NTSC) ? 60 : 50;
   info->timing.sample_rate    = 44100;
   info->geometry.base_width   = EMUWIDTH;
   info->geometry.base_height  = EMUHEIGHT;
   info->geometry.max_width    = EMUWIDTH;
   info->geometry.max_height   = EMUHEIGHT;
   info->geometry.aspect_ratio = 4.0 / 3.0;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   (void)port;
   (void)device;
}

size_t retro_serialize_size(void) 
{ 
	return STATE_SIZE;
}

bool retro_serialize(void *data, size_t size)
{
   savestate_to_mem((uint8_t *)data);
   return true;
}

bool retro_unserialize(const void *data, size_t size)
{
   loadstate_from_mem((uint8_t *)data);
   return true;
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}

bool retro_load_game(const struct retro_game_info *info)
{
    char bios_file_path[256];
    const char *full_path, *system_directory_c;
    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
    struct retro_input_descriptor desc[] = {
       { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
       { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up" },
       { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
       { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
       { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "Fire" },
       { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "1" },
       { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "2" },
       { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "3" },
       { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "4" },

       { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
       { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up" },
       { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
       { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
       { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "Action" },

       { 0 },
    };

    if (!info)
       return false;

    if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
    {
        if (log_cb)
            log_cb(RETRO_LOG_INFO, "[O2EM]: RGB565 is not supported.\n");
        return false;
    }

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

    full_path = info->path;
    system_directory_c = NULL;

    // BIOS is required
    environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_directory_c);
    if (!system_directory_c)
    {
       if (log_cb)
          log_cb(RETRO_LOG_WARN, "[O2EM]: no system directory defined, unable to look for o2rom.bin\n");
       return false;
    }
    else
    {
#ifdef _WIN32
      char slash = '\\';
#else
      char slash = '/';
#endif

      /*
       snprintf(bios_file_path, sizeof(bios_file_path), "%s%c%s", system_directory_c, slash, "o2rom.bin");

       if (!does_file_exist(bios_file_path))
       {
          if (log_cb)
             log_cb(RETRO_LOG_WARN, "[O2EM]: o2rom.bin not found, cannot load BIOS\n");
          return false;
       }
       */
    }

    app_data.debug = 0;
    app_data.stick[0] = app_data.stick[1] = 1;
    app_data.sticknumber[0] = app_data.sticknumber[1] = 0;
    set_defjoykeys(0,0);
    set_defjoykeys(1,1);
    set_defsystemkeys();
    app_data.bank = 0;
    app_data.limit = 1;
    app_data.sound_en = 1;
    app_data.speed = 100;
    app_data.wsize = 2;
    app_data.fullscreen = 0;
    app_data.scanlines = 0;
    app_data.voice = 1;
    app_data.window_title = "O2EM v" O2EM_VERSION;
    app_data.svolume = 100;
    app_data.vvolume = 100;
    app_data.filter = 0;
    app_data.exrom = 0;
    app_data.three_k = 0;
    app_data.crc = 0;
    app_data.scshot = scshot;
    app_data.statefile = statefile;
    app_data.euro = 0;
    app_data.openb = 0;
    app_data.vpp = 0;
    app_data.bios = 0;
    app_data.scoretype = 0;
    app_data.scoreaddress = 0;
    app_data.default_highscore = 0;
    app_data.breakpoint = 65535;
    app_data.megaxrom = 0;
    strcpy(scorefile,"highscore.txt");
    //read_default_config();

    init_audio();

    app_data.crc = crc32_file(full_path);

    //suck_bios();
    o2flag = 1;

    crcx = app_data.crc;
    //suck_roms();

    if (!load_bios(bios_file_path))
       return false;

    if (!load_cart(full_path))
       return false;

    //if (app_data.voice) load_voice_samples(path2);

    init_display();

    init_cpu();

    init_system();

    set_score(app_data.scoretype, app_data.scoreaddress, app_data.default_highscore);

    return true;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
   (void)game_type;
   (void)info;
   (void)num_info;
   return false;
}

void retro_unload_game(void) 
{
}

unsigned retro_get_region(void)
{
    return evblclk == EVBLCLK_NTSC ? RETRO_REGION_NTSC : RETRO_REGION_PAL;
}

unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

void *retro_get_memory_data(unsigned id)
{
    if ( id == RETRO_MEMORY_SYSTEM_RAM )
        return intRAM;
    return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
    if ( id == RETRO_MEMORY_SYSTEM_RAM )
        return 64;
    return 0;
}

void retro_init(void)
{
   struct retro_log_callback log;
   unsigned level = 5;

   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;
   else
      log_cb = NULL;

   environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);
   
   memset(mbmp, 0, sizeof(mbmp));
   RLOOP=1;
}

void retro_deinit(void)
{
   close_audio();
   close_voice();
   close_display();
   retro_destroybmp();
}

void retro_reset(void)
{
   init_cpu();
   init_roms();
   init_vpp();
   clearscr();
}

void retro_run(void)
{
   int i, length;
   update_input();

   cpu_exec();
   RLOOP=1;

   video_cb(mbmp, EMUWIDTH, EMUHEIGHT, TEX_WIDTH << 1);
   
   length = (evblclk == EVBLCLK_NTSC) ? 44100 / 60 : 44100 / 50;

   /* Convert 8u to 16s */
   for(i = 0; i < length; i++)
   {
      int16_t sample16 = (soundBuffer[i]-128) << 8;
      audio_cb(sample16, sample16);
   }
}
