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

/*
test.cpp

Revision History

Version		Change											Engineer	Date
=======		======											========	======
1.0 		created											bbb
1.1			refactor, fix an err checking time remapping	zal			6/23/2016

*/

#include "test.h"
// #include "Server.hpp"

#include <iostream>
#include <fstream>
#include <thread>
#include <future>
#include <chrono>
#include <string>
#include <mutex>
#include <stdio.h>

#ifdef AE_OS_WIN
#include <windows.h>
#endif

// Connection connection;

// This one saves globally the layer that we're currently operating on
AEGP_LayerH selected_layer;

A_FpLong framerate = 0;

static AEGP_Command	S_dump_proj_cmd = 0,
						S_other_cmd = 0;
static AEGP_PluginID	S_my_id		= 0;
static SPBasicSuite		*sP		    = 0;


static std::string convert_char_array(A_UTF16Char *filePath) {
    // TODO: currently this only works for ASCII, if you put a UTF-8 character anywhere in the filepath
    //  or in the file name this will immediately eat it & die
    std::string parsed = "";
    for (A_UTF16Char i = 0; i<AEGP_MAX_PATH_SIZE; i++) {
        A_UTF16Char current_char = filePath[i];

        if (current_char == NULL) {
            printf("Null");
            break;
        } else {
            printf("%c ", filePath[i]);
            parsed += filePath[i];
        }
    }
    
    std::cout << std::endl;

    std::cout << parsed;

    return parsed;
}


// Function to convert and copy string literals to A_UTF16Char.
// On Win: Pass the input directly to the output
// On Mac: All conversion happens through the CFString format
// TODO: Use this function instead of the home rolled one
static void
copyConvertStringLiteralIntoUTF16(
                                  const wchar_t* inputString,
                                  A_UTF16Char* destination)
{
#ifdef AE_OS_MAC
          int length = static_cast<int>(wcslen(inputString));
 CFRange          range = {0, AEGP_MAX_PATH_SIZE};
          range.length = length;
 CFStringRef inputStringCFSR = CFStringCreateWithBytes( kCFAllocatorDefault,
                                                          reinterpret_cast<const UInt8 *>(inputString),
                                                          length * sizeof(wchar_t),
                                                          kCFStringEncodingUTF32LE,
                                                          FALSE);
          CFStringGetBytes(          inputStringCFSR,
                     range,
                     kCFStringEncodingUTF16,
                     0,
                     FALSE,
                     reinterpret_cast<UInt8 *>(destination),
                     length * (sizeof (A_UTF16Char)),
                     NULL);
          destination[length] = 0; // Set NULL-terminator, since CFString calls don't set it
          CFRelease(inputStringCFSR);
#elif defined AE_OS_WIN
          size_t length = wcslen(inputString);
          wcscpy_s(reinterpret_cast<wchar_t*>(destination), length + 1, inputString);
#endif
}


static std::string openFileBrowser() {
    char* filepath = tinyfd_openFileDialog("Select JSON Mask to Import", "", 0, NULL, "", 0);
    return filepath != NULL ? std::string(filepath) : "";
}


static json readJSON(std::string &filename) {
    std::ifstream f(filename);
    json input_data = json::parse(f);
    
    return input_data;
}


static A_Err getFootageFramerate(PF_InData *in_data, AEGP_ItemH active_itemH) {
    AEGP_FootageInterp footage_interp;
    A_Err                err = A_Err_NONE;
    
    AEGP_SuiteHandler    suites(in_data->pica_basicP);
    
    AEGP_ItemType item_type;
    ERR(suites.ItemSuite6()->AEGP_GetItemType(active_itemH, &item_type));
    
    std::cout << item_type << std::endl;
    
    ERR(suites.FootageSuite5()->AEGP_GetFootageInterpretation(active_itemH, false, &footage_interp));
    
    framerate = footage_interp.native_fpsF;
    std::cout << "framerate: " << framerate << std::endl;
    
    return err;
}


static	A_Err AddPositionKeyframe(
        PF_InData               *in_data,
        AEGP_AddKeyframesInfoH	addKeyframesInfoH,
        AEGP_StreamRefH			position_stream,
        A_Time					key_timeT,
        A_FpLong                x,
        A_FpLong                y,
        A_FpLong                z)
{
    A_long              new_indexL          = 0;
    AEGP_StreamValue2   val;
    A_Err				err					= A_Err_NONE,
                        err2				= A_Err_NONE;
    
    AEGP_SuiteHandler	suites(in_data->pica_basicP);

    AEFX_CLR_STRUCT(val);

    ERR(suites.KeyframeSuite4()->AEGP_AddKeyframes(		addKeyframesInfoH,
            AEGP_LTimeMode_LayerTime,
            &key_timeT,
            &new_indexL));

    ERR(suites.StreamSuite3()->AEGP_GetNewStreamValue(	S_my_id,
            position_stream,
            AEGP_LTimeMode_LayerTime,
            &key_timeT,
            TRUE,
            &val));

    if (!err) {
        val.val.three_d.x = x;
        val.val.three_d.y = y;
        val.val.three_d.z = z;
        ERR(suites.KeyframeSuite4()->AEGP_SetAddKeyframe(addKeyframesInfoH, new_indexL, &val));
    }
    ERR2(suites.StreamSuite3()->AEGP_DisposeStreamValue(&val));

    return err;
}

//A_Err addTrackKeyFrames(std::vector<std::vector<float>> input) {
//    AEGP_SuiteHandler suites(sP);
//    AEGP_AddKeyframesInfoH    addKeyframesInfoH    = NULL;
//    A_FloatPoint cameraStartPoint = {0, 0};
//    AEGP_CompH parentComp = NULL;
//    A_UTF16Char cameraName = NULL;
//    AEGP_ItemH selectedLayerItem = NULL;
//    AEGP_StreamRefH position_stream = NULL;
//    AEGP_AddKeyframesInfoH add_keyframes_info_h = NULL;
//
//    A_long comp_width = NULL;
//    A_long comp_height = NULL;
//
//    const wchar_t *nameLiteral = L"Camera";
//    copyConvertStringLiteralIntoUTF16(nameLiteral, &cameraName);
//
//    // Get the parent and the item the selected object refers to
//    suites.LayerSuite8()->AEGP_GetLayerParentComp(selected_layer, &parentComp);
//    // Some undefined behavior is happening here, can crash if the line when the camera is created moves - it seems like the name literal is always different so its probably
//    //  due to AE always listening to some random parts of memory
//    suites.CompSuite11()->AEGP_CreateCameraInComp(&cameraName, cameraStartPoint, parentComp, &selected_layer);
//
//    // These lines calculate the center point of the camera but getting the layer source item breaks it (maybe because it actually returns a layer from an item (no way right))
////    suites.LayerSuite8()->AEGP_GetLayerSourceItem(selected_layer, &selectedLayerItem);
////    suites.ItemSuite9()->AEGP_GetItemDimensions(selectedLayerItem, &comp_width, &comp_height);
////
////    double d_width = comp_width/2;
////    double d_height = comp_height/2;
////
////    A_FloatPoint cameraStartPoint = {d_width, d_height};
//
//    suites.StreamSuite2()->AEGP_GetNewLayerStream(S_my_id,
//                                                  selected_layer,
//                                                  AEGP_LayerStream_POSITION,
//                                                  &position_stream);
//
//    suites.KeyframeSuite4()->AEGP_StartAddKeyframes(position_stream, &add_keyframes_info_h);
//
//    A_Time time_zero = {0, 10000};
//    for (int i = 0; i<10; i++) {
//        AddPositionKeyframe(add_keyframes_info_h, position_stream, time_zero, i, i, i);
//        // Add a new frame at each second for 10 seconds
//        time_zero = {i * 10000, 10000};
//    }
//
//    suites.KeyframeSuite4()->AEGP_EndAddKeyframes(TRUE, add_keyframes_info_h);
//
//    return A_Err_NONE;
//}

static A_Err getTimeInfo(PF_InData *in_data) {
    A_Err err = A_Err_NONE;
    AEGP_SuiteHandler suites(in_data->pica_basicP);
    
    A_Time begin_time;
    ERR(suites.LayerSuite8()->AEGP_GetLayerInPoint(selected_layer, AEGP_LTimeMode_LayerTime, &begin_time));
    std::cout << "First visible frame is " << (A_FpLong(begin_time.value)/A_FpLong(begin_time.scale)*framerate) << " frames after start!" << std::endl;
    
    A_Time duration;
    ERR(suites.LayerSuite8()->AEGP_GetLayerDuration(selected_layer, AEGP_LTimeMode_LayerTime, &duration));
    std::cout << "Layer is " << (A_FpLong(duration.value)/A_FpLong(duration.scale)*framerate) << " frames long!" << std::endl;
    
    return err;
}

A_Err createInitialMask(
        PF_InData *in_data,
        AEGP_StreamValue2 &mask_stream_val,
        AEGP_MaskOutlineValH &mask_outline_valH,
        json &mask_data)
{
    AEGP_VertexIndex vertex_index = 0;
    AEGP_MaskVertex mask_vertex;
    A_Err err = A_Err_NONE;
    
    AEGP_SuiteHandler suites(in_data->pica_basicP);
    
    for (const auto &point : mask_data) {
        ERR(suites.MaskOutlineSuite3()->AEGP_CreateVertex(mask_outline_valH, vertex_index));
        
        auto &handle = point["handle"];
        mask_vertex.x = handle["x"];
        mask_vertex.y = handle["y"];
        auto &control_point_backward = point["control_point_backward"];
        mask_vertex.tan_in_x = int(control_point_backward["x"]) - int(handle["x"]);
        mask_vertex.tan_in_y = int(control_point_backward["y"]) - int(handle["y"]);
        auto &control_point_forward = point["control_point_forward"];
        mask_vertex.tan_out_x = int(control_point_forward["x"]) - int(handle["x"]);
        mask_vertex.tan_out_y = int(control_point_forward["y"]) - int(handle["y"]);
        
        ERR(suites.MaskOutlineSuite3()->AEGP_SetMaskOutlineVertexInfo(mask_outline_valH, vertex_index, &mask_vertex));
        vertex_index++;
    }
    std::cout << vertex_index << std::endl;
    
    return err;
}


// TODO: ensure points match up with each other inside input vector/JSON
A_Err createNextMask(
        PF_InData *in_data,
        AEGP_StreamValue2 &mask_stream_val,
        AEGP_MaskOutlineValH &mask_outline_valH,
        json &mask_data)
{
    AEGP_VertexIndex vertex_index = 0;
    AEGP_MaskVertex mask_vertex;
    A_Err err = A_Err_NONE;
    
    AEGP_SuiteHandler suites(in_data->pica_basicP);
    
    for (const auto &point : mask_data) {
        auto &handle = point["handle"];
        mask_vertex.x = handle["x"];
        mask_vertex.y = handle["y"];
        auto &control_point_backward = point["control_point_backward"];
        mask_vertex.tan_in_x = int(control_point_backward["x"]) - int(handle["x"]);
        mask_vertex.tan_in_y = int(control_point_backward["y"]) - int(handle["y"]);
        auto &control_point_forward = point["control_point_forward"];
        mask_vertex.tan_out_x = int(control_point_forward["x"]) - int(handle["x"]);
        mask_vertex.tan_out_y = int(control_point_forward["y"]) - int(handle["y"]);
        
        ERR(suites.MaskOutlineSuite3()->AEGP_SetMaskOutlineVertexInfo(mask_outline_valH, vertex_index, &mask_vertex));
        vertex_index++;
    }
    std::cout << vertex_index << std::endl;

    return err;
}


A_Err AddMaskKeyframe(
        PF_InData *in_data,
        AEGP_AddKeyframesInfoH addKeyframesInfoH,
        AEGP_StreamRefH &mask_stream,
        AEGP_StreamValue2 &mask_stream_val,
        const A_Time key_timeT)
{
    A_long keyframe_index = 0;
    A_Err err = A_Err_NONE;
    
    AEGP_SuiteHandler suites(in_data->pica_basicP);

    ERR(suites.KeyframeSuite4()->AEGP_AddKeyframes(addKeyframesInfoH, AEGP_LTimeMode_LayerTime, &key_timeT, &keyframe_index));

    ERR(suites.KeyframeSuite4()->AEGP_SetAddKeyframe(addKeyframesInfoH, keyframe_index, &mask_stream_val));

    return err;
}


A_Err addTrackKeyframes(PF_InData *in_data, std::string &filename) {
    AEGP_MaskRefH mask_refH;
    A_long mask_index;
    AEGP_StreamRefH mask_stream = NULL;
    AEGP_AddKeyframesInfoH add_keyframes_info_h = NULL;
    AEGP_StreamValue2 mask_stream_val;
    AEGP_MaskOutlineValH mask_outline_valH;
    A_Err err = A_Err_NONE;
    
    AEGP_SuiteHandler suites(in_data->pica_basicP);
    
    AEFX_CLR_STRUCT(mask_stream_val);
    
    // parse json input data
    json input_data = readJSON(filename);
    
    // loop through each mask
    for (auto &mask : input_data.items()) {
        // create new mask on current layer and get its stream
        ERR(suites.MaskSuite6()->AEGP_CreateNewMask(selected_layer, &mask_refH, &mask_index));
        ERR(suites.StreamSuite5()->AEGP_GetNewMaskStream(S_my_id, mask_refH, AEGP_MaskStream_OUTLINE, &mask_stream));
        
        // set mask name
        std::string mask_name = mask.key();
        suites.MaskSuite4()->AEGP_SetMaskName(mask_refH, &mask_name[0]);
                
        // get stream value and mask outline value
        // time doesn't matter for AEGP_GetNewStreamValue here, since the mask stream doesn't have keyframes yet so is essentially constant!
        A_Time time = {0,1};
        ERR(suites.StreamSuite5()->AEGP_GetNewStreamValue(S_my_id, mask_stream, AEGP_LTimeMode_LayerTime, &time, TRUE, &mask_stream_val));
        mask_outline_valH = mask_stream_val.val.mask;
        
        // tell AE to begin expecting new keyframes
        ERR(suites.KeyframeSuite4()->AEGP_StartAddKeyframes(mask_stream, &add_keyframes_info_h));
        
        // loop through each frame in data
        A_long frame_count = 0;
        for (auto &frame : mask.value().items()) {
            // finding initial frame manually for now
            if (frame_count == 0) {
                // create first initial mask
                createInitialMask(in_data, mask_stream_val, mask_outline_valH, frame.value());
            }
            else {
                // create next mask
                createNextMask(in_data, mask_stream_val, mask_outline_valH, frame.value());
            }
            
            // add mask to keyframe
            A_Time time_keyframe = {stoi(frame.key()), (A_u_long)(framerate+0.5)};
            AddMaskKeyframe(in_data, add_keyframes_info_h, mask_stream, mask_stream_val, time_keyframe);
            frame_count++;
        }
        
        // tell AE to stop expecting keyframes
        ERR(suites.KeyframeSuite4()->AEGP_EndAddKeyframes(TRUE, add_keyframes_info_h));
        
        // dispose things that need it - missing anything else?
        ERR(suites.StreamSuite5()->AEGP_DisposeStreamValue(&mask_stream_val));
        ERR(suites.StreamSuite5()->AEGP_DisposeStream(mask_stream));
        ERR(suites.MaskSuite6()->AEGP_DisposeMask(mask_refH));
    }
    
    return err;
}


static A_Err
Keyframer(
    PF_InData             *in_data,
    AEGP_ItemH            itemH,
    AEGP_ItemH            parent_itemH0,
    std::string           &filename)
{
    A_Err                 err        = A_Err_NONE;
    AEGP_SuiteHandler     suites(in_data->pica_basicP);
    
    // TODO: there are probably some major memory issues allocating all of these up front and not deallocating
    AEGP_LayerH active_layer = NULL;
    AEGP_ItemH active_item = NULL;
    AEGP_ItemType active_item_type = NULL;
    
    ERR(suites.LayerSuite8()->AEGP_GetActiveLayer(&active_layer));
    
    if (active_layer != NULL) {
        ERR(suites.LayerSuite8()->AEGP_GetLayerSourceItem(active_layer, &active_item));
        ERR(suites.ItemSuite6()->AEGP_GetItemType(active_item, &active_item_type));
        
        if (!err && (active_item_type == AEGP_ItemType_FOOTAGE)) {
            AEGP_FootageH         footageH = NULL;
            A_UTF16Char           *filePath = NULL;
            AEGP_MemHandle        pathH = NULL;
            
            ERR(suites.FootageSuite5()->AEGP_GetMainFootageFromItem(active_item, &footageH));
            ERR(suites.FootageSuite5()->AEGP_GetFootagePath(footageH, 0, AEGP_FOOTAGE_MAIN_FILE_INDEX, &pathH));
            
            ERR(suites.MemorySuite1()->AEGP_LockMemHandle(pathH, reinterpret_cast<void**>(&filePath)));  // now path should be accessible via filePath!
            std::string final_output = convert_char_array(filePath);
            ERR(suites.MemorySuite1()->AEGP_UnlockMemHandle(pathH));
            ERR(suites.MemorySuite1()->AEGP_FreeMemHandle(pathH));
            
            // TODO add a new thread here
            // std::async(std::launch::async, startConnection, &paths);
            selected_layer = active_layer;
            std::cout << "Would have started connection!" << std::endl;
            
            // set framerate
            getFootageFramerate(in_data, active_item);
            
            // track time of first actual visible frame within start of layer
            getTimeInfo(in_data);
            
            addTrackKeyframes(in_data, filename);
            
//            connection.updatePaths(final_output);
//            connection.connect();
        }
        else {
            ERR(suites.UtilitySuite3()->AEGP_ReportInfo(S_my_id, STR(StrID_WrongType)));
        }
    } else {
        ERR(suites.UtilitySuite3()->AEGP_ReportInfo(S_my_id, STR(StrID_NoActiveItem)));
    }
    
    return err;
}


static A_Err
extractFilePaths(PF_InData *in_data)
{
	A_Err 				err 	      = A_Err_NONE,
						err2 	      = A_Err_NONE;
	AEGP_ProjectH		projH	      = NULL;
	AEGP_ItemH			root_folderPH = NULL;
	A_char				proj_nameAC[AEGP_MAX_PROJ_NAME_SIZE];
    
	AEGP_SuiteHandler	suites(in_data->pica_basicP);
//	ERR(suites.UtilitySuite3()->AEGP_StartUndoGroup("Keepin' On"));

	if (!err) {
		ERR(suites.ProjSuite5()->AEGP_GetProjectByIndex(0, &projH)); // "as of CS6, there will only ever be one open project"
		ERR(suites.ProjSuite5()->AEGP_GetProjectRootFolder(projH, &root_folderPH));

		if (!err && root_folderPH) {

            hardcoded filename for now
            std::string filename = "spline_multiple.json";
            err = Keyframer(in_data, root_folderPH, 0, filename);
			err = suites.ProjSuite5()->AEGP_GetProjectName(projH, proj_nameAC);
		}
	}
//	ERR2(suites.UtilitySuite3()->AEGP_EndUndoGroup());

	return err;
}


//
//static A_Err
//CommandHook(
//	AEGP_GlobalRefcon	plugin_refconPV,		/* >> */
//	AEGP_CommandRefcon	refconPV,				/* >> */
//	AEGP_Command		command,				/* >> */
//	AEGP_HookPriority	hook_priority,			/* >> */
//	A_Boolean			already_handledB,		/* >> */
//	A_Boolean			*handledPB)				/* << */
//{
//	A_Err 				err = A_Err_NONE;
//	AEGP_SuiteHandler	suites(sP);
//
//	*handledPB = FALSE;
//
//	if (command == S_dump_proj_cmd) {
//        // first logic function call
//        err = extractFilePaths();
//        *handledPB = TRUE;
//	}
//	return err;
//}

//A_Err
//EntryPointFunc(
//	struct SPBasicSuite		*pica_basicP,			/* >> */
//	A_long				 	major_versionL,			/* >> */
//	A_long					minor_versionL,			/* >> */
//	AEGP_PluginID			aegp_plugin_id,			/* >> */
//	AEGP_GlobalRefcon		*global_refconP)		/* << */
//{
//    S_my_id = aegp_plugin_id;
//	sP = pica_basicP;
//    A_Err err = A_Err_NONE;
//
//    AEGP_SuiteHandler suites(pica_basicP);
//
//	ERR(suites.CommandSuite1()->AEGP_GetUniqueCommand(&S_dump_proj_cmd));
//
//    if (!err && S_dump_proj_cmd){
//        ERR(suites.CommandSuite1()->AEGP_InsertMenuCommand(S_dump_proj_cmd, "test", AEGP_Menu_EDIT, AEGP_MENU_INSERT_SORTED));
//    }
//
//	ERR(suites.RegisterSuite5()->AEGP_RegisterCommandHook(	S_my_id,
//															AEGP_HP_BeforeAE,
//															AEGP_Command_ALL,
//															CommandHook,
//															NULL));
//
//	ERR(suites.RegisterSuite5()->AEGP_RegisterUpdateMenuHook(S_my_id, UpdateMenuHook, NULL));
//	return err;
//}


static PF_Err
About (
    PF_InData        *in_data,
    PF_OutData        *out_data,
    PF_ParamDef        *params[],
    PF_LayerDef        *output )
{
    AEGP_SuiteHandler suites(in_data->pica_basicP);
    
    suites.ANSICallbacksSuite1()->sprintf(out_data->return_msg,
                                "%s v%d.%d\r%s",
                                STR(StrID_Name),
                                MAJOR_VERSION,
                                MINOR_VERSION,
                                STR(StrID_Description));
    return PF_Err_NONE;
}


static PF_Err
GlobalSetup (
    PF_InData        *in_data,
    PF_OutData        *out_data,
    PF_ParamDef        *params[],
    PF_LayerDef        *output )
{
    out_data->my_version = PF_VERSION(  MAJOR_VERSION,
                                        MINOR_VERSION,
                                        BUG_VERSION,
                                        STAGE_VERSION,
                                        BUILD_VERSION);

    out_data->out_flags =     PF_OutFlag_DEEP_COLOR_AWARE;

    out_data->out_flags2 = PF_OutFlag2_SUPPORTS_THREADED_RENDERING;
    
    return PF_Err_NONE;
}


static PF_Err
ParamsSetup (
    PF_InData        *in_data,
    PF_OutData        *out_data,
    PF_ParamDef        *params[],
    PF_LayerDef        *output )
{
    PF_Err         err = PF_Err_NONE;
    PF_ParamDef    def;

    AEFX_CLR_STRUCT(def);


    PF_ADD_BUTTON(STR(StrID_Create_Button_Param_Name),
                    STR(StrID_Create_Button_Label_Name),
                    0,
                    PF_ParamFlag_SUPERVISE,
                    BUTTON_DISK_ID);
    
    PF_ADD_BUTTON(STR(StrID_Import_Button_Param_Name),
                    STR(StrID_Import_Button_Label_Name),
                    0,
                    PF_ParamFlag_SUPERVISE,
                    BUTTON_DISK_ID);

    out_data->num_params = NUM_PARAMS;

    return err;
}


static PF_Err
Render (
    PF_InData        *in_data,
    PF_OutData        *out_data,
    PF_ParamDef        *params[],
    PF_LayerDef        *output )
{
    PF_Err                err        = PF_Err_NONE;
    AEGP_SuiteHandler    suites(in_data->pica_basicP);

    ERR(PF_COPY(&params[0]->u.ld,
                output,
                NULL,
                NULL));
    
    return err;
}


static PF_Err
UserChangedParam(
    PF_InData                        *in_data,
    PF_OutData                        *out_data,
    PF_ParamDef                        *params[],
    const PF_UserChangedParamExtra    *which_hitP)
{
    PF_Err err = PF_Err_NONE;

    if (which_hitP->param_index == CREATE_BUTTON) {
        if (in_data->appl_id != 'PrMr') {
            extractFilePaths(in_data);
        } else {
            // In Premiere Pro, this message will appear in the Events panel
            extractFilePaths(in_data);
        }
    }
    else if (which_hitP->param_index == IMPORT_BUTTON) {
        if (in_data->appl_id != 'PrMr') {
            std::string import_filepath = openFileBrowser();
            if (import_filepath != "") Keyframer(in_data, 0, 0, import_filepath);
        } else {
            // In Premiere Pro, this message will appear in the Events panel
            std::string import_filepath = openFileBrowser();
            if (import_filepath != "") Keyframer(in_data, 0, 0, import_filepath);
        }
    }
    
    return err;
}


static PF_Err
GlobalSetdown(
    PF_InData        *in_data,
    PF_OutData        *out_data,
    PF_ParamDef        *params[],
    PF_LayerDef        *output)
{
    PF_Err    err        =    PF_Err_NONE;

    AEGP_SuiteHandler        suites(in_data->pica_basicP);

    if (in_data->global_data){
        suites.HandleSuite1()->host_dispose_handle(in_data->global_data);
    }
    return err;
}


extern "C" DllExport
PF_Err PluginDataEntryFunction(
    PF_PluginDataPtr inPtr,
    PF_PluginDataCB inPluginDataCallBackPtr,
    SPBasicSuite* inSPBasicSuitePtr,
    const char* inHostName,
    const char* inHostVersion)
{
    PF_Err result = PF_Err_INVALID_CALLBACK;

    result = PF_REGISTER_EFFECT(
        inPtr,
        inPluginDataCallBackPtr,
        "test plugin", // Name
        "test", // Match Name
        "testing", // Category
        AE_RESERVED_INFO); // Reserved Info

    return result;
}


PF_Err
EffectMain(
    PF_Cmd            cmd,
    PF_InData        *in_data,
    PF_OutData        *out_data,
    PF_ParamDef        *params[],
    PF_LayerDef        *output,
    void            *extra)
{
    PF_Err        err = PF_Err_NONE;
    
    try {
        switch (cmd) {
            case PF_Cmd_ABOUT:
                err = About(in_data, out_data, params, output);
                break;
            case PF_Cmd_GLOBAL_SETUP:
                err = GlobalSetup(in_data, out_data, params, output);
                break;
            case PF_Cmd_GLOBAL_SETDOWN:
                err = GlobalSetdown(in_data, out_data, params, output);
            case PF_Cmd_PARAMS_SETUP:
                err = ParamsSetup(in_data, out_data, params, output);
                break;
            case PF_Cmd_RENDER:
                err = Render(in_data, out_data, params, output);
                break;

            case PF_Cmd_USER_CHANGED_PARAM:
                err = UserChangedParam(in_data, out_data, params,
                                        reinterpret_cast<const PF_UserChangedParamExtra *>(extra));
                break;
        }
    } catch(PF_Err &thrown_err) {
        err = thrown_err;
    }
    return err;
}
