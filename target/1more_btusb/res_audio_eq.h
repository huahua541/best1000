#ifndef __RES_AUDIO_EQ_H__
#define __RES_AUDIO_EQ_H__
	
const AUDIO_FIR_COEF RES_AUDIO_EQ_COEF = {
	.len = 384,
	.coef = {
				#include "res/eq/EQ_Coef.txt"
	}	
};

#ifdef __cplusplus
	extern "C" {
#endif
		
#ifdef __cplusplus
	}
#endif
#endif
	
