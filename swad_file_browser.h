// swad_file_browser.h: file browsers

#ifndef _SWAD_FILE_BROWSER
#define _SWAD_FILE_BROWSER
/*
    SWAD (Shared Workspace At a Distance in Spanish),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2015 Antonio Ca�as Vargas

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*****************************************************************************/
/********************************* Headers ***********************************/
/*****************************************************************************/

#include <linux/limits.h>	// For PATH_MAX

#include "swad_group.h"

/*****************************************************************************/
/******************************* Public types ********************************/
/*****************************************************************************/

#define Brw_NUM_TYPES_FILE_BROWSER 25
// The following types are stored in clipboard, expanded_folders, file_browser_size tables as numeric fields, so don't change numbers!
typedef enum
  {
   Brw_UNKNOWN        =  0,
   Brw_SHOW_DOCUM_CRS =  1,
   Brw_SHOW_MARKS_CRS =  2,
   Brw_ADMI_DOCUM_CRS =  3,
   Brw_ADMI_SHARE_CRS =  4,
   Brw_ADMI_SHARE_GRP =  5,
   Brw_ADMI_WORKS_USR =  6,
   Brw_ADMI_WORKS_CRS =  7,
   Brw_ADMI_MARKS_CRS =  8,
   Brw_ADMI_BRIEF_USR =  9,
   Brw_SHOW_DOCUM_GRP = 10,
   Brw_ADMI_DOCUM_GRP = 11,
   Brw_SHOW_MARKS_GRP = 12,
   Brw_ADMI_MARKS_GRP = 13,
   Brw_ADMI_ASSIG_USR = 14,
   Brw_ADMI_ASSIG_CRS = 15,
   Brw_SHOW_DOCUM_DEG = 16,
   Brw_ADMI_DOCUM_DEG = 17,
   Brw_SHOW_DOCUM_CTR = 18,
   Brw_ADMI_DOCUM_CTR = 19,
   Brw_SHOW_DOCUM_INS = 20,
   Brw_ADMI_DOCUM_INS = 21,
   Brw_ADMI_SHARE_DEG = 22,
   Brw_ADMI_SHARE_CTR = 23,
   Brw_ADMI_SHARE_INS = 24,
  } Brw_FileBrowser_t;

// The following types are stored in files and clipboard tables as numeric fields, so don't change numbers!
#define Brw_NUM_FILE_TYPES	4
typedef enum
  {
   Brw_IS_UNKNOWN	= 0,
   Brw_IS_FILE		= 1,
   Brw_IS_FOLDER	= 2,
   Brw_IS_LINK		= 3,
  } Brw_FileType_t;

#define Brw_NUM_UPLOAD_TYPES 2
typedef enum
  {
   Brw_DROPZONE_UPLOAD,
   Brw_CLASSIC_UPLOAD,
  } Brw_UploadType_t;

#define Brw_NUM_LICENSES 8
typedef enum	// Don't change these values! They are stored in database as numbers
  {
   Brw_LICENSE_UNKNOWN             = 0,	// Unknown license / other license
   Brw_LICENSE_ALL_RIGHTS_RESERVED = 1,	// All Rights Reserved
   Brw_LICENSE_CC_BY               = 2,	// Creative Commons Attribution License
   Brw_LICENSE_CC_BY_SA            = 3,	// Creative Commons Attribution-ShareAlike License
   Brw_LICENSE_CC_BY_ND            = 4,	// Creative Commons Attribution-NoDerivs License
   Brw_LICENSE_CC_BY_NC            = 5,	// Creative Commons Attribution-NonCommercial License
   Brw_LICENSE_CC_BY_NC_SA         = 6,	// Creative Commons Attribution-NonCommercial-ShareAlike License
   Brw_LICENSE_CC_BY_NC_ND         = 7,	// Creative Commons Attribution-NonCommercial-NoDerivs License
  } Brw_License_t;
#define Brw_LICENSE_DEFAULT Brw_LICENSE_ALL_RIGHTS_RESERVED

struct FileMetadata
  {
   long FilCod;
   Brw_FileBrowser_t FileBrowser;
   long Cod;	// Code of institution, centre, degree, course or group
   long ZoneUsrCod;
   long PublisherUsrCod;
   char Path[PATH_MAX+1];
   bool IsHidden;
   bool IsPublic;
   Brw_License_t License;
   Brw_FileType_t FileType;
   off_t Size;
   time_t Time;
   unsigned NumMyViews;
   unsigned NumPublicViews;
   unsigned NumViewsFromLoggedUsrs;
   unsigned NumLoggedUsrs;
  };

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

#define Brw_MAX_DIR_LEVELS	10	// Maximum number of subdirectory levels in file browsers

#define Brw_MAX_BYTES_MIME_TYPE	256	// Maximum length of "image/jpeg", "text/html", etc.

#define Brw_INTERNAL_NAME_ROOT_FOLDER_DOCUMENTS		"doc"
#define Brw_INTERNAL_NAME_ROOT_FOLDER_SHARED_FILES	"sha"
#define Brw_INTERNAL_NAME_ROOT_FOLDER_DOWNLOAD		"descarga"		// TODO: It should be "doc"
#define Brw_INTERNAL_NAME_ROOT_FOLDER_SHARED		"comun"			// TODO: It should be "sha"
#define Brw_INTERNAL_NAME_ROOT_FOLDER_ASSIGNMENTS	"actividades"		// TODO: It should be "asg"
#define Brw_INTERNAL_NAME_ROOT_FOLDER_WORKS		"trabajos"		// TODO: It should be "wrk"
#define Brw_INTERNAL_NAME_ROOT_FOLDER_MARKS		"calificaciones"	// TODO: It should be "mrk"
#define Brw_INTERNAL_NAME_ROOT_FOLDER_BRIEF		"maletin"		// TODO: It should be "brf"

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void Brw_GetParAndInitFileBrowser (void);
void Brw_PutParamsPathAndFile (Brw_FileType_t FileType,const char *PathInTree,const char *FileFolderName);
void Brw_InitializeFileBrowser (void);
bool Brw_CheckIfExistsFolderAssigmentForAnyUsr (const char *FolderName);
bool Brw_UpdateFoldersAssigmentsIfExistForAllUsrs (const char *OldFolderName,const char *NewFolderName);
void Brw_RemoveFoldersAssignmentsIfExistForAllUsrs (const char *FolderName);
void Brw_ShowFileBrowserOrWorks (void);
void Brw_ShowAgainFileBrowserOrWorks (void);

void Brw_RemoveInsFilesFromDB (long InsCod);
void Brw_RemoveCtrFilesFromDB (long CtrCod);
void Brw_RemoveDegFilesFromDB (long DegCod);
void Brw_RemoveCrsFilesFromDB (long CrsCod);
void Brw_RemoveGrpFilesFromDB (long GrpCod);
void Brw_RemoveSomeInfoAboutCrsUsrFilesFromDB (long CrsCod,long UsrCod);
void Brw_RemoveWrkFilesFromDB (long CrsCod,long UsrCod);
void Brw_RemoveUsrFilesFromDB (long UsrCod);

void Brw_CreateDirDownloadTmp (void);
void Brw_AskEditWorksCrs (void);
void Brw_AskRemFileFromTree (void);
void Brw_RemFileFromTree (void);
void Brw_RemFolderFromTree (void);
void Brw_ExpandFileTree (void);
void Brw_ContractFileTree (void);
void Brw_CopyFromFileBrowser (void);
void Brw_PasteIntoFileBrowser (void);
void Brw_RemSubtreeInFileBrowser (void);
void Brw_ShowFormFileBrowser (void);
void Brw_RecFolderFileBrowser (void);
void Brw_RenFolderFileBrowser (void);
void Brw_RcvFileInFileBrwDropzone (void);
void Brw_RcvFileInFileBrwClassic (void);
void Brw_RecLinkFileBrowser (void);
void Brw_SetDocumentAsVisible (void);
void Brw_SetDocumentAsHidden (void);
bool Brw_CheckIfFileOrFolderIsSetAsHiddenInDB (Brw_FileType_t FileType,const char *Path);
bool Brw_CheckIfFileOrFolderIsHidden (struct FileMetadata *FileMetadata);
void Brw_ShowFileMetadata (void);
void Brw_DownloadFile (void);
void Brw_GetLinkToDownloadFile (const char *PathInTree,const char *FileName,char *URL);
void Brw_ChgFileMetadata (void);
long Brw_GetFilCodByPath (const char *Path);
void Brw_GetFileMetadataByPath (struct FileMetadata *FileMetadata);
void Brw_GetFileMetadataByCod (struct FileMetadata *FileMetadata);
bool Brw_GetFileTypeSizeAndDate (struct FileMetadata *FileMetadata);
void Brw_GetAndUpdateFileViews (struct FileMetadata *FileMetadata);
void Brw_UpdateMyFileViews (long FilCod);
unsigned long Brw_GetNumFileViewsUsr (long UsrCod);
unsigned Brw_GetNumFilesUsr (long UsrCod);
unsigned Brw_GetNumPublicFilesUsr (long UsrCod);

long Brw_GetCodForFiles (void);
void Brw_GetCrsGrpFromFileMetadata (Brw_FileBrowser_t FileBrowser,long Cod,
                                    long *CrsCod,long *GrpCod);

long Brw_AddPathToDB (long PublisherUsrCod,Brw_FileType_t FileType,
                      const char *Path,bool IsPublic,Brw_License_t License);

void Brw_RemoveExpiredExpandedFolders (void);

void Brw_RemoveTree (const char *Path);
void Brw_CalcSizeOfDir (char *Path);

void Brw_SetFullPathInTree (const char *PathInTreeUntilFileOrFolder,const char *FilFolLnkName);

void Brw_ParamListFiles (Brw_FileType_t FileType,const char *PathInTree,const char *FileName);

void Brw_RemoveZonesOfGroupsOfType (long GrpTypCod);
void Brw_RemoveGrpZonesVerbose (struct GroupData *GrpDat);
void Brw_RemoveGrpZones (long CrsCod,long GrpCod);

void Brw_RemoveUsrWorksInCrs (struct UsrData *UsrDat,struct Course *Crs,Cns_QuietOrVerbose_t QuietOrVerbose);
void Brw_RemoveUsrWorksInAllCrss (struct UsrData *UsrDat,Cns_QuietOrVerbose_t QuietOrVerbose);

void Brw_GetSummaryAndContentOrSharedFile (char *SummaryStr,char **ContentStr,
                                           long FilCod,unsigned MaxChars,bool GetContent);

unsigned Brw_ListDocsFound (const char *Query,const char *Title);

void Brw_AskRemoveOldFiles (void);
void Brw_RemoveOldFiles (void);

#endif
