/* http://www.libpng.org/pub/png/libpng-manual.txt 

icons:
  http://ionicons.com/
  http://ionicons.com/cheatsheet.html
  
  https://github.com/iconic/open-iconic
  https://github.com/iconic/open-iconic/blob/master/font/css/open-iconic.styl
  
  gibts nicht mehr:
  https://www.joomlart.com/blog/news-updates/freebie-meals-vector-set-120-icons-png-svg  
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

#define PNG_DEBUG 3
#include <png.h>

/*================================================*/
char *bdf_file_name = NULL;
char *bdf_font_name = NULL;
char *bdf_copyright = NULL;
FILE *bdf_fp = NULL;
long encoding = 48;


/*================================================*/

int width, height;

png_byte color_type;
png_byte bit_depth;
uint8_t min, max;

png_structp png_ptr;
png_infop info_ptr;
png_bytep * row_pointers;

void print_info(const char* file_name)
{
  printf("%s: png_get_image_width(png_ptr, info_ptr)=%d\n", file_name, png_get_image_width(png_ptr, info_ptr));
  printf("%s: png_get_image_height(png_ptr, info_ptr)=%d\n", file_name, png_get_image_height(png_ptr, info_ptr));

  /*
#define PNG_COLOR_MASK_PALETTE    1
#define PNG_COLOR_MASK_COLOR      2
#define PNG_COLOR_MASK_ALPHA      4
  
#define PNG_COLOR_TYPE_GRAY 0
#define PNG_COLOR_TYPE_PALETTE  (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_PALETTE)
#define PNG_COLOR_TYPE_RGB        (PNG_COLOR_MASK_COLOR)
#define PNG_COLOR_TYPE_RGB_ALPHA  (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_ALPHA)
#define PNG_COLOR_TYPE_GRAY_ALPHA (PNG_COLOR_MASK_ALPHA)
*/
  
  printf("%s: png_get_color_type(png_ptr, info_ptr)=%d\n", file_name, png_get_color_type(png_ptr, info_ptr));
  printf("%s: png_get_bit_depth(png_ptr, info_ptr)=%d\n", file_name, png_get_bit_depth(png_ptr, info_ptr));
  printf("%s: png_get_rowbytes(png_ptr, info_ptr)=%lu\n", file_name, png_get_rowbytes(png_ptr, info_ptr));
  printf("%s: range: %d .. %d\n", file_name, min, max);
  
}

void print_short_info(const char* file_name)
{
  printf("%s enc=%ld w=%d h=%d\n", file_name, encoding, width, height);
}

uint8_t get_gray(int x, int y)
{
  
  uint8_t *p;
  uint8_t g;
  if ( color_type & PNG_COLOR_MASK_ALPHA )    
  {
    p = row_pointers[y] + x*4;
    g = ((int)p[0] + (int)p[1] + (int)p[2])/3;  
    g = ((int)g+(int)p[3])/2;
    return g;
  }
  p = row_pointers[y] + x*3;
  return ((int)p[0] + (int)p[1] + (int)p[2])/3;  
}

uint8_t get_pixel(int x, int y)
{
  if ( x < 0 )
    return 0;
  if ( x >= width )
    return 0;
  if ( y < 0 )
    return 0;
  if ( y >= height )
    return 0;
  if ( get_gray(x, y) > 128/2 )
    return 1;
  return 0;
}

uint8_t get_pixel_byte(int x, int y)
{
  uint8_t b = 0;
  int i;
  for( i = 0; i < 8; i++ )
  {
      b <<= 1;
      b |= get_pixel(x + i,y);
  }
  return b;
}

void write_4bit(int x)
{
  if ( x < 10 )
    fprintf(bdf_fp, "%c", x + '0' );
  else
    fprintf(bdf_fp, "%c", x - 10 + 'a'  );    
}

void write_byte(int x)
{
  write_4bit(x >> 4);
  write_4bit(x & 15);
}

void write_bdf_bitmap(void)
{
  int x, y;
  
  fprintf(bdf_fp, "STARTCHAR %ld\n", encoding);
  fprintf(bdf_fp, "ENCODING %ld\n", encoding);
  fprintf(bdf_fp, "DWIDTH %d 0\n", width);
  fprintf(bdf_fp, "BBX %d %d 0 0\n", width, height);
  fprintf(bdf_fp, "BITMAP\n");
  
  for( y = 0; y < height; y++ )
  {
    x = 0;
    while( x < width )
    {
      write_byte(get_pixel_byte(x,y));
      x+=8;
    }
    fprintf(bdf_fp, "\n"  );    
  }  
  fprintf(bdf_fp, "ENDCHAR\n");
  encoding++;
}


void show_ascii(void)
{
  int x, y;
  for( y = 0; y < height; y++ )
  {
    for( x = 0; x < width; x++ )
    {
      if ( get_gray(x, y) > 128/2 )
	printf("#");
      else
	printf(".");
    }
    printf("\n");
  }
}


void calc_min_max(void)
{
  int x, y;
  uint8_t val;
  min = 255;
  max = 0;
  for( y = 0; y < height; y++ )
  {
    for( x = 0; x < width; x++ )
    {
      val = get_gray(x, y);
      if ( max < val )
	max = val;
      if ( min > val )
	min = val;
    }
  }
}

int read_png_file(const char* file_name)
{
        unsigned char header[8];    // 8 is the maximum size that can be checked

        /* open file and test for it being a png */
        FILE *fp = fopen(file_name, "rb");
        if (fp == NULL)
	  return 0;	/* open error */
        fread(header, 1, 8, fp);
        if (png_sig_cmp(header, 0, 8))
	  return fclose(fp), 0;		/* not a png file error */


        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (png_ptr == NULL)
	  return fclose(fp), 0;		/* alloc read struct */

        info_ptr = png_create_info_struct(png_ptr);
        if (info_ptr == NULL )
	  return fclose(fp), 0;		/* alloc info struct */

        if (setjmp(png_jmpbuf(png_ptr)))
	{
	    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
	    return fclose(fp), 0;		/* library error */	  
	}

        png_init_io(png_ptr, fp);
        png_set_sig_bytes(png_ptr, 8);
        png_read_png(png_ptr, info_ptr, 
	  PNG_TRANSFORM_SCALE_16|
	  //PNG_TRANSFORM_STRIP_ALPHA|
	  PNG_TRANSFORM_PACKING|
	  //PNG_TRANSFORM_INVERT_ALPHA |
	  //PNG_TRANSFORM_INVERT_MONO | 
	  PNG_TRANSFORM_GRAY_TO_RGB
	  , NULL);

        width = png_get_image_width(png_ptr, info_ptr);
        height = png_get_image_height(png_ptr, info_ptr);
        color_type = png_get_color_type(png_ptr, info_ptr);
        bit_depth = png_get_bit_depth(png_ptr, info_ptr);
	row_pointers = png_get_rows(png_ptr, info_ptr);
	      
	//calc_min_max();
	//print_info(file_name);
	print_short_info(file_name);
	//show_ascii();
	write_bdf_bitmap();
	
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
	fclose(fp);
	
	return 1;
	
}

void help(void)
{
  printf("png2bdf [options] bdf-file { [options] png-file }\n");
  printf(" BDF options (use before bdf-file):'\n");
  printf("  -f 'name of the font'\n");
  printf("  -c 'copyright note'\n");
  printf(" PNG options (use before png-file):'\n");
  printf("  -e encoding\n");
  
}

void write_bdf_header(void)
{
  fprintf(bdf_fp, "STARTFONT 2.1\n");
  fprintf(bdf_fp, "FONT");
  if ( bdf_font_name == NULL )
    fprintf(bdf_fp, " \"unknown\"\n");
  else
    fprintf(bdf_fp, " \"%s\"\n", bdf_font_name);
    
  fprintf(bdf_fp, "SIZE 16 75 75\n");
  fprintf(bdf_fp, "FONTBOUNDINGBOX 16 16 0 0\n");
  fprintf(bdf_fp, "STARTPROPERTIES 3\n");

  fprintf(bdf_fp, "COPYRIGHT");
  if ( bdf_copyright == NULL )
    fprintf(bdf_fp, " \"unknown\"\n");
  else
    fprintf(bdf_fp, " \"%s\"\n", bdf_copyright);

  fprintf(bdf_fp, "FONT_ASCENT 0\n");
  fprintf(bdf_fp, "FONT_DESCENT 0\n");
  
  fprintf(bdf_fp, "ENDPROPERTIES\n");
  
}

int main(int argc, char **argv)
{
  if ( argc < 2 )
  {
    help();
    return 0;
  }
  argc--; argv++;
  do
  {
    if ( argv[0][0] == '-' )
    {
      if ( strcmp(argv[0], "-f") == 0 )
      {
	argc--; argv++;
	if ( argc > 0 )
	{
	  bdf_font_name = argv[0];
	  argc--; argv++;
	}
      }
      else if ( strcmp(argv[0], "-c") == 0 )
      {
	argc--; argv++;
	if ( argc > 0 )
	{
	  bdf_copyright = argv[0];
	  argc--; argv++;
	}
      }
      else if ( strcmp(argv[0], "-e") == 0 )
      {
	argc--; argv++;
	if ( argc > 0 )
	{
	  encoding = atol(argv[0]);
	  argc--; argv++;
	}
      }
      
      
    }
    else
    {
      if ( bdf_file_name == NULL )
      {
	bdf_file_name = argv[0];
	argc--; argv++;
	bdf_fp = fopen(bdf_file_name, "wb");
	if ( bdf_fp == NULL )
	{
	  perror(bdf_file_name);
	  exit(1);
	}	
	write_bdf_header();
      }
      else
      {
	/* parse png image */
	read_png_file(argv[0]);
	argc--; argv++;
      }
    }
    
    
    
  } while( argc > 0 );
  
  if ( bdf_fp != NULL )
    fclose(bdf_fp);
  
  return 0;
}


