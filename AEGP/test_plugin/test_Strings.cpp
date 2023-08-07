//Mangler_Strings.cpp

#include "test.h"

typedef struct {
    unsigned long    index;
    char            str[256];
} TableString;

TableString        g_strs[StrID_NUMTYPES] = {
    StrID_NONE,                            "",
    StrID_Name,                            "test",
    StrID_Description,                    "test plug-in.",
    StrID_SuiteError,                    "Error acquiring suite.",
    StrID_InitError,                    "Error initializing dialog.",
    StrID_AddCommandError,                "Error adding command.",
    StrID_UndoError,                    "Error starting undo group.",
    StrID_URL,                            "http://www.adobe.com",
    StrID_NoActiveItem,            "Please select a layer in the timeline to use with Shade.",
    StrID_CameraName,         "test Camera",
    StrID_WrongType,               "Your selection is not a footage type.",
    StrID_Create_Button_Param_Name,        "",
    StrID_Create_Button_Label_Name,        "Create AE Mask",
    StrID_Import_Button_Param_Name,        "",
    StrID_Import_Button_Label_Name,        "Import Mask",
    StrID_Button_Message,           "Button hit!"
};

char    *GetStringPtr(int strNum) {
    return g_strs[strNum].str;
}
