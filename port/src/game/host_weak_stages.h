/* Stages the host may not build, forced into ground.c by the build.
 *
 * ground.c's stage table names every stage's data, so linking it would need
 * all of them.  Weak references let the table hold NULL for a stage that is
 * not in the link, which the game already treats as a stage without data:
 * Ground_801C06B8 returns before touching a NULL entry.  The two preload hooks
 * below are only called for stages whose data exists.
 *
 * A weak reference does not pull a member out of a static library.  A stage
 * that is ported needs something to refer to it strongly, or its entry stays
 * NULL even though its code was compiled.
 */

#ifndef MELEE_HOST_WEAK_STAGES_H
#define MELEE_HOST_WEAK_STAGES_H

#pragma weak grTe_StageData
#pragma weak grCs_StageData
#pragma weak grRc_StageData
#pragma weak grKg_StageData
#pragma weak grGd_StageData
#pragma weak grGb_StageData
#pragma weak grSh_StageData
#pragma weak grZe_StageData
#pragma weak grKr_StageData
#pragma weak grSt_StageData
#pragma weak grYt_StageData
#pragma weak grIz_StageData
#pragma weak grGr_StageData
#pragma weak grCn_StageData
#pragma weak grVe_StageData
#pragma weak grPs_StageData
#pragma weak grPu_StageData
#pragma weak grMc_StageData
#pragma weak grBb_StageData
#pragma weak grOt_StageData
#pragma weak grFs_StageData
#pragma weak grIm_StageData
#pragma weak grI1_StageData
#pragma weak grI2_StageData
#pragma weak grFz_StageData
#pragma weak grOp_StageData
#pragma weak grOy_StageData
#pragma weak grOk_StageData
#pragma weak grNKr_StageData
#pragma weak grSh_Route_StageData
#pragma weak grZe_Route_StageData
#pragma weak grBb_Route_StageData
#pragma weak grNBa_StageData
#pragma weak grNLa_StageData
#pragma weak grFigureGet_StageData
#pragma weak grPushOn_StageData
#pragma weak grTMr_StageData
#pragma weak grTCa_StageData
#pragma weak grTCLink_StageData
#pragma weak grTDk_StageData
#pragma weak grTDr_StageData
#pragma weak grTFc_StageData
#pragma weak grTFx_StageData
#pragma weak grTIc_StageData
#pragma weak grTKb_StageData
#pragma weak grTKp_StageData
#pragma weak grTLk_StageData
#pragma weak grTLg_StageData
#pragma weak grTMs_StageData
#pragma weak grTMewtwo_StageData
#pragma weak grTNs_StageData
#pragma weak grTPe_StageData
#pragma weak grTPc_StageData
#pragma weak grTPk_StageData
#pragma weak grTPr_StageData
#pragma weak grTSs_StageData
#pragma weak grTSk_StageData
#pragma weak grTYs_StageData
#pragma weak grTZd_StageData
#pragma weak grTGw_StageData
#pragma weak grTFe_StageData
#pragma weak grTGn_StageData
#pragma weak grHeal_StageData
#pragma weak grHr_StageData
#pragma weak grEF1_StageData
#pragma weak grEF2_StageData
#pragma weak grEF3_StageData

#pragma weak grIzumi_801CD2D4
#pragma weak grStadium_801D511C

#endif
