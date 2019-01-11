#ifndef CO_GRAPHICS_H
#define CO_GRAPHICS_H
#define VERMEER_YELLOW 0xFE24

//everything in header file goes here
typedef struct equippedTDM {
  unsigned int   width;
  unsigned int   height;
  unsigned int   bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */ 
  unsigned char  pixel_data[143L * 55L * 2L + 1L];
} equippedTDM_t;

typedef struct headerBarBlack {
  unsigned int   width;
  unsigned int   height;
  unsigned int   bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */ 
  unsigned char  pixel_data[240L * 32L * 2L + 1L];
} headerBarBlack_t;

typedef struct headerBarLE {
  unsigned int   width;
  unsigned int   height;
  unsigned int   bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */ 
  unsigned char  pixel_data[240L * 33L * 2L + 1L];
} headerBarLE_t;

extern const equippedTDM_t CO_Startup;
extern const headerBarBlack_t CO_HeaderBarBlack;
extern const headerBarLE_t CO_HeaderBarLE;
#endif
