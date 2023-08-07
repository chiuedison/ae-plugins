/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2007 Adobe Systems Incorporated                       */
/* All Rights Reserved.                                            */
/*                                                                 */
/* NOTICE:  All information contained herein is, and remains the   */
/* property of Adobe Systems Incorporated and its suppliers, if    */
/* any.  The intellectual and technical concepts contained         */
/* herein are proprietary to Adobe Systems Incorporated and its    */
/* suppliers and may be covered by U.S. and Foreign Patents,       */
/* patents in process, and are protected by trade secret or        */
/* copyright law.  Dissemination of this information or            */
/* reproduction of this material is strictly forbidden unless      */
/* prior written permission is obtained from Adobe Systems         */
/* Incorporated.                                                   */
/*                                                                 */
/*******************************************************************/

#include "AEConfig.h"
#include "entry.h"

#ifdef AE_OS_WIN
	#include <windows.h>
	#include <stdio.h>
	#include <string.h>
#elif defined AE_OS_MAC
	#include <wchar.h>
#endif

#include "AE_GeneralPlug.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "A.h"
#include "AE_EffectUI.h"
#include "SPSuites.h"
#include "AE_AdvEffectSuites.h"
#include "AE_EffectCBSuites.h"
#include "AEGP_SuiteHandler.h"
#include "String_Utils.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "test_Strings.hpp"
#include "json.hpp"
#include <vector>
#include "tinyfiledialogs.h"

using json = nlohmann::json;


/* Versioning information */

#define    MAJOR_VERSION    2
#define    MINOR_VERSION    0
#define    BUG_VERSION        0
#define    STAGE_VERSION    PF_Stage_DEVELOP
#define    BUILD_VERSION    1

#define ERROR_MISSING_FOOTAGE	"Footage not found!"
#define DUMP_SUCCEEDED_WIN		"Project dumped to C:\\Windows\\Temp\\"
#define DUMP_SUCCEEDED_MAC		"Project dumped to root of primary hard drive."
#define DUMP_FAILED				"Project dump failed."


A_Err addTrackKeyframes(PF_InData *in_data, std::string &filename);

A_Err createInitialMask(PF_InData *in_data, AEGP_StreamValue2 &mask_stream_val, AEGP_MaskOutlineValH &mask_outline_valH, json &mask_data);

A_Err createNextMask(PF_InData *in_data, AEGP_StreamValue2 &mask_stream_val, AEGP_MaskOutlineValH &mask_outline_valH, json &mask_data);

A_Err AddMaskKeyframe(PF_InData *in_data, AEGP_AddKeyframesInfoH addKeyframesInfoH, AEGP_StreamRefH &mask_stream, AEGP_StreamValue2 &mask_stream_val, const A_Time key_timeT);

// This entry point is exported through the PiPL (.r file)
//extern "C" DllExport AEGP_PluginInitFuncPrototype EntryPointFunc;

enum
{
    PARAMARAMA_INPUT = 0,
    CREATE_BUTTON,
    IMPORT_BUTTON,
    NUM_PARAMS
};

enum
{
    BUTTON_DISK_ID
};

extern "C" {

    DllExport
    PF_Err
    EffectMain(
        PF_Cmd            cmd,
        PF_InData        *in_data,
        PF_OutData        *out_data,
        PF_ParamDef        *params[],
        PF_LayerDef        *output,
        void            *extra);

}
