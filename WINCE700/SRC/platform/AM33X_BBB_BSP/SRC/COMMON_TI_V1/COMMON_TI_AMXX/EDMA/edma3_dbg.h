
#ifndef _EDMA3_BDG_H_
#define _EDMA3_BDG_H_


#ifdef __cplusplus
extern "C" {
#endif

void EDMA3_DRV_ShowQSTAT(EDMA3_DRV_Handle hEdma);
void EDMA3_DRV_ShowCCCFG(EDMA3_DRV_Handle hEdma);
void EDMA3_DRV_ShowIRs (EDMA3_DRV_Handle hEdma);
void EDMA3_DRV_ShowShadowIRs (EDMA3_DRV_Handle hEdma);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* _EDMA3_BDG_H_ */
