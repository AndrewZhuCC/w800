#ifndef __RESAMPLE__
#define __RESAMPLE__
// made by Guan Haixin, @Unisound, 12/13/2017
typedef void* RSMPHD;

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif


enum {
	RESP_16K_8K,
	RESP_8K_16K
};

#if defined(__cplusplus)
extern "C"
{
#endif

	/************************************************************************\
	 FUNCTION:  resample_init
	 DESCRIPTION:
	   Do initialization.
	 PARAMETERS: 
	   No.
	 RETURN:
	  Not NULL: Success.
	  NULL: Errors.
	\************************************************************************/
	RSMPHD resample_init(int len_up, int len_down);

	/************************************************************************\
	 FUNCTION:  resample_run_16k48k
	 DESCRIPTION:
	   Do process.
	 PARAMETERS: 
	   phd:       handle.[In|Out]
	   psdatain:  input data.[In]
	   nlen:      input data length, must be divided by 3, and should < 1024.[In]
	   psdataout: output data, the buffer should be > nlen/3 for downsample, > nlen x 3 for upsample.[Out]
	   nsign:     the resample type, like the enum define.[In]
	 RETURN:
	   the output data number.
	\************************************************************************/
	int    resample_run_8k16k(RSMPHD phd, short* psdatain, int nlen, short* psdataout, int nsign);

	/************************************************************************\
	 FUNCTION:  resample_reset
	 DESCRIPTION:
	   reset parameters.
	 PARAMETERS: 
	   phd:     Handle.[In|Output]
	 RETURN:
	  No.
	\************************************************************************/
	void   resample_reset(RSMPHD phd);

	/************************************************************************\
	 FUNCTION:  resample_release
	 DESCRIPTION:
	   Release memory.
	 PARAMETERS: 
	   pmclphd:     Handle.[In|Output]
	 RETURN:
	  No.
	\************************************************************************/
	void   resample_release(RSMPHD phd);

#if defined(__cplusplus)
}
#endif


#endif
