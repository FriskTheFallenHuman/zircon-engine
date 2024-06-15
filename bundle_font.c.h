// bundle_font.c.h

#include "roboto.medium.ttf.bin.h"
dp_font_t *zdev_dpfont;

WARP_X_ (fs_selfpack fonts_mempool font_shutdown)

extern dllhandle_t ft2_dll;


	#if 0
		// Baker: Fills with black, testing.
		SDL_Surface *sdl_surface_icon = SDL_CreateRGBSurface(0,/*wh*/ 64,64,32,0,0,0,0);
	#endif

	WARP_X_ (mousepointer_crosshair_2 Vid_Cursor_Set)


void GoogleRobotoFont_Check(void)
{
#ifdef CONFIG_MENU
	bgra4 *LoadPNG (ccs *imagepath, size_t *pfilesizet);
		
	Vid_Cursor_Load_To_Slot_Maybe ("engine/icons/colsize36.png", mousepointer_baker_column_20);
//		"C:\zircs\_zirc_qualker\bundle\engine\icons\colsize.png"

	if (zdev_dpfont == NULL) {
	#ifndef DP_FREETYPE_STATIC
		if (ft2_dll)
	#endif
		{
			extern mempool_t *fonts_mempool;

			WARP_X_ (Font_UnloadFont)
			zdev_dpfont = (dp_font_t *)Mem_Alloc(fonts_mempool, sizeof(dp_font_t) * ONE_CHAR_1);

			size_t mem_length = sizeof(roboto_medium_bin);	
			const byte *mem = (const byte *)roboto_medium_bin; // const byte *mem = Bundle_Pointer (3, &mem_length);
			byte *font_memcopy = (byte *)Mem_DupZ (fonts_mempool, mem, mem_length); // Font system expects to free this and it uses the fonts mempool		

			// Font size 72 is huge. 83 MB ... 			
			LoadFontDPEx (zdev_dpfont, "fonts/Roboto-Medium.ttf", "10 24 36 56", font_memcopy, mem_length);
		} // ft2 dll
	}
#endif // #ifdef CONFIG_MENU
}




// Baker: 
// Q: What is our reason for keeping Roboto as a separate .h instead of using bundle.pak?
//
// A: We use the font outside of the DarkPlaces font organization system.
//    DarkPlaces has names like "user2" and such, we don't want it as part of that hierarchy.
//    Even if we decoupled, we would still have to custom load.
//
// Q: Ok ... but why not reload as part of font system?
//
// A: Because the font loading can be slow.
//
//    So let's "just in time" load it and it is not part of Quake
//    And someone might have the bin64 folder missing so no TrueType
//    So let's keep it simple and optional

// Q: Do the fonts need to upload a texture?
//
// A: Yes, It is basically a font sheet.  Beware, if I use font size 72 the upload is 70+ MB
//    so try to limit font size to 36 point (which is 20 MB).

// Q: Why memdup the font?

// A: The font shutdown process expects to free it.  It's 165 KB, so not really that big.
