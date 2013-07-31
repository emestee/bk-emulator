/* emu2149.h */
#ifndef _EMU2149_H_
#define _EMU2149_H_
#include "emutypes.h"

#ifdef EMU2149_DLL_EXPORTS
  #define EMU2149_API __declspec(dllexport)
#elif  EMU2149_DLL_IMPORTS
  #define EMU2149_API __declspec(dllimport)
#else
  #define EMU2149_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {

	/* Volume Table */
	e_uint32 *voltbl ;

	e_uint8 reg[0x20] ;
	e_int32 out ;

	e_uint32 count[3] ;
	e_uint32 volume[3] ;
	e_uint32 freq[3] ;
	e_uint32 edge[3] ;
	e_uint32 tmask[3] ;
	e_uint32 nmask[3] ;
  e_uint32 mute[3] ;

  e_uint32 base_count ;

	e_uint32 env_volume ;
	e_uint32 env_ptr ;
	e_uint32 env_enable ;
	e_uint32 env_reverse ;
	e_uint32 env_hold ;
	e_uint32 env_alt ;
	e_uint32 env_freq ;
	e_uint32 env_count ;

	e_uint32 noise_seed ;
	e_uint32 noise_count ;
	e_uint32 noise_freq ;

	/* rate converter */
	e_uint32 realstep ;
	e_uint32 psgtime ;
	e_uint32 psgstep ;

  /* I/O Ctrl */
  e_uint32 adr ;

} PSG ;
	
EMU2149_API void PSG_init(e_uint32 clk, e_uint32 rate) ;
EMU2149_API void PSG_set_quality(e_uint32 q) ;
EMU2149_API void PSG_close(void) ;
EMU2149_API PSG *PSG_new(void) ;
EMU2149_API void PSG_reset(PSG *) ;
EMU2149_API void PSG_delete(PSG *) ;
EMU2149_API void PSG_writeReg(PSG *, e_uint32 reg, e_uint32 val) ;
EMU2149_API void PSG_writeIO(PSG *psg, e_uint32 adr, e_uint32 val) ;
EMU2149_API e_uint8 PSG_readReg(PSG *psg, e_uint32 reg) ;
EMU2149_API e_uint8 PSG_readIO(PSG *psg) ;
EMU2149_API e_int16 PSG_calc(PSG *) ;
EMU2149_API void PSG_setVolumeMode(PSG *psg, int type) ;

#ifdef __cplusplus
}
#endif

#endif
