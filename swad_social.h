// swad_social.c: social networking (timeline)

#ifndef _SWAD_SOC
#define _SWAD_SOC
/*
    SWAD (Shared Workspace At a Distance in Spanish),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2016 Antonio Ca�as Vargas

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
/********************************** Headers **********************************/
/*****************************************************************************/

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/******************************** Public types *******************************/
/*****************************************************************************/

#define Soc_NUM_PUB_TYPES	4
// If the numbers assigned to each event type change,
// it is necessary to change old numbers to new ones in database table social_notes
typedef enum
  {
   Soc_PUB_UNKNOWN		= 0,
   Soc_PUB_ORIGINAL_NOTE	= 1,
   Soc_PUB_SHARED_NOTE		= 2,
   Soc_PUB_COMMENT_TO_NOTE	= 3,
  } Soc_PubType_t;

#define Soc_NUM_NOTE_TYPES	13
// If the numbers assigned to each event type change,
// it is necessary to change old numbers to new ones in database table social_notes
typedef enum
  {
   Soc_NOTE_UNKNOWN		=  0,

   /* Institution tab */
   Soc_NOTE_INS_DOC_PUB_FILE	=  1,
   Soc_NOTE_INS_SHA_PUB_FILE	=  2,

   /* Centre tab */
   Soc_NOTE_CTR_DOC_PUB_FILE	=  3,
   Soc_NOTE_CTR_SHA_PUB_FILE	=  4,

   /* Degree tab */
   Soc_NOTE_DEG_DOC_PUB_FILE	=  5,
   Soc_NOTE_DEG_SHA_PUB_FILE	=  6,

   /* Course tab */
   Soc_NOTE_CRS_DOC_PUB_FILE	=  7,
   Soc_NOTE_CRS_SHA_PUB_FILE	=  8,

   /* Assessment tab */
   Soc_NOTE_EXAM_ANNOUNCEMENT	=  9,

   /* Users tab */

   /* Social tab */
   Soc_NOTE_SOCIAL_POST		= 10,
   Soc_NOTE_FORUM_POST		= 11,

   /* Messages tab */
   Soc_NOTE_NOTICE		= 12,

   /* Statistics tab */

   /* Profile tab */

  } Soc_NoteType_t;

/*****************************************************************************/
/****************************** Public prototypes ****************************/
/*****************************************************************************/

void Soc_ShowTimelineGbl (void);
void Soc_ShowTimelineUsr (void);

void Soc_RefreshNewTimelineGbl (void);

void Soc_RefreshOldTimelineGbl (void);
void Soc_RefreshOldTimelineUsr (void);

long Soc_StoreAndPublishSocialNote (Soc_NoteType_t NoteType,long Cod);
void Soc_MarkSocialNoteAsUnavailableUsingNotCod (long NotCod);
void Soc_MarkSocialNoteAsUnavailableUsingNoteTypeAndCod (Soc_NoteType_t NoteType,long Cod);
void Soc_MarkSocialNoteOneFileAsUnavailable (const char *Path);
void Soc_MarkSocialNotesChildrenOfFolderAsUnavailable (const char *Path);

void Soc_ReceiveSocialPostGbl (void);
void Soc_ReceiveSocialPostUsr (void);

void Soc_ReceiveCommentGbl (void);
void Soc_ReceiveCommentUsr (void);

void Soc_ShareSocialNoteGbl (void);
void Soc_ShareSocialNoteUsr (void);
void Soc_FavSocialNoteGbl (void);
void Soc_FavSocialNoteUsr (void);
void Soc_FavSocialCommentGbl (void);
void Soc_FavSocialCommentUsr (void);

void Soc_UnshareSocialNoteGbl (void);
void Soc_UnshareSocialNoteUsr (void);
void Soc_UnfavSocialNoteGbl (void);
void Soc_UnfavSocialNoteUsr (void);
void Soc_UnfavSocialCommentGbl (void);
void Soc_UnfavSocialCommentUsr (void);

void Soc_RequestRemSocialNoteGbl (void);
void Soc_RequestRemSocialNoteUsr (void);
void Soc_RemoveSocialNoteGbl (void);
void Soc_RemoveSocialNoteUsr (void);

void Soc_RequestRemSocialComGbl (void);
void Soc_RequestRemSocialComUsr (void);
void Soc_RemoveSocialComGbl (void);
void Soc_RemoveSocialComUsr (void);

void Soc_RemoveUsrSocialContent (long UsrCod);

void Soc_ClearOldTimelinesDB (void);

void Soc_GetNotifNewSocialPost (char *SummaryStr,char **ContentStr,long PstCod,
                                unsigned MaxChars,bool GetContent);

#endif
