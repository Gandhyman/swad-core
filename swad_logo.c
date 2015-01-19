// swad_logo.c: logo of institution, centre or degree

/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2015 Antonio Ca�as Vargas

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General 3 License as
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
/*********************************** Headers *********************************/
/*****************************************************************************/

#include <string.h>		// For string functions

#include "swad_action.h"
#include "swad_global.h"
#include "swad_scope.h"
#include "swad_theme.h"

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Internal constants ****************************/
/*****************************************************************************/

/*****************************************************************************/
/****************************** Internal types *******************************/
/*****************************************************************************/

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/************************* Internal global variables *************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Internal prototypes ***************************/
/*****************************************************************************/

/*****************************************************************************/
/****************************** Draw degree logo *****************************/
/*****************************************************************************/

void Log_DrawLogo (Sco_Scope_t Scope,long Cod,const char *AltText,
                   unsigned Size,const char *Style,bool PutIconIfNotExists)
  {
   const char *Folder;
   const char *Icon;
   char PathLogo[PATH_MAX+1];
   bool LogoExists;

   /***** Set variables depending on scope *****/
   switch (Scope)
     {
      case Sco_SCOPE_INSTITUTION:
	 Folder = Cfg_FOLDER_INS;
	 Icon = "ins";
	 break;
      case Sco_SCOPE_CENTRE:
	 Folder = Cfg_FOLDER_CTR;
	 Icon = "ctr";
	 break;
      case Sco_SCOPE_DEGREE:
	 Folder = Cfg_FOLDER_DEG;
	 Icon = "deg";
	 break;
      default:
	 return;	// Nothing to do
     }

   /***** Path to logo *****/
   if (Cod > 0)
     {
      sprintf (PathLogo,"%s/%s/%02u/%u/logo/%u.png",
	       Cfg_PATH_SWAD_PUBLIC,Folder,
	       (unsigned) (Cod % 100),
	       (unsigned) Cod,
	       (unsigned) Cod);
      LogoExists = Fil_CheckIfPathExists (PathLogo);
     }
   else
      LogoExists = false;

   if (LogoExists || PutIconIfNotExists)
     {
      /***** Draw logo *****/
      fprintf (Gbl.F.Out,"<img src=\"");
      if (LogoExists)
	 fprintf (Gbl.F.Out,"%s/%s/%02u/%u/logo/%u.png",
		  Cfg_HTTPS_URL_SWAD_PUBLIC,Folder,
		  (unsigned) (Cod % 100),
		  (unsigned) Cod,
		  (unsigned) Cod);
      else
	 fprintf (Gbl.F.Out,"%s/%s64x64.gif",
		  Gbl.Prefs.IconsURL,Icon);
      fprintf (Gbl.F.Out,"\" alt=\"%s\" class=\"ICON%ux%u\"",
	       AltText,Size,Size);
      if (Style)
	 if (Style[0])
	    fprintf (Gbl.F.Out," style=\"%s\"",Style);
      fprintf (Gbl.F.Out," />");
     }
  }

/*****************************************************************************/
/*************** Put a link to the action used to request  *******************/
/*************** the logo of institution, centre or degree *******************/
/*****************************************************************************/

void Log_PutFormToChangeLogo (Sco_Scope_t Scope)
  {
   extern const char *The_ClassFormul[The_NUM_THEMES];
   extern const char *Txt_Change_logo;
   extern const char *Txt_Upload_logo;
   Act_Action_t Action;
   long Cod;
   const char *Folder;
   const char *Icon;
   const char *Msg;
   char PathLogo[PATH_MAX+1];
   bool LogoExists;

   /***** Set variables depending on scope *****/
   switch (Scope)
     {
      case Sco_SCOPE_INSTITUTION:
	 Action = ActReqInsLog;
	 Cod = Gbl.CurrentIns.Ins.InsCod;
	 Folder = Cfg_FOLDER_INS;
	 Icon = "ins";
	 break;
      case Sco_SCOPE_CENTRE:
	 Action = ActReqCtrLog;
	 Cod = Gbl.CurrentCtr.Ctr.CtrCod;
	 Folder = Cfg_FOLDER_CTR;
	 Icon = "ctr";
	 break;
      case Sco_SCOPE_DEGREE:
	 Action = ActReqDegLog;
	 Cod = Gbl.CurrentDeg.Deg.DegCod;
	 Folder = Cfg_FOLDER_DEG;
	 Icon = "deg";
	 break;
      default:
	 return;	// Nothing to do
     }

   /***** Check if logo exists *****/
   sprintf (PathLogo,"%s/%s/%02u/%u/logo/%u.png",
	    Cfg_PATH_SWAD_PUBLIC,Folder,
	    (unsigned) (Cod % 100),
	    (unsigned) Cod,
	    (unsigned) Cod);
   LogoExists = Fil_CheckIfPathExists (PathLogo);

   /***** Link for changing / uploading the logo *****/
   Act_FormStart (Action);
   Msg = LogoExists ? Txt_Change_logo :
		      Txt_Upload_logo;
   Act_LinkFormSubmit (Msg,The_ClassFormul[Gbl.Prefs.Theme]);
   Lay_PutSendIcon (Icon,Msg,Msg);
   fprintf (Gbl.F.Out,"</form>");
  }

/*****************************************************************************/
/**** Show a form for sending a logo of the institution, centre or degree ****/
/*****************************************************************************/

void Log_RequestLogo (Sco_Scope_t Scope)
  {
   extern const char *The_ClassFormul[The_NUM_THEMES];
   extern const char *Txt_You_can_send_a_file_with_an_image_in_png_format_transparent_background_and_size_X_Y;
   extern const char *Txt_File_with_the_logo;
   extern const char *Txt_Upload_logo;
   Act_Action_t Action;

   /***** Set action depending on scope *****/
   switch (Scope)
     {
      case Sco_SCOPE_INSTITUTION:
	 Action = ActRecInsLog;
	 break;
      case Sco_SCOPE_CENTRE:
	 Action = ActRecCtrLog;
	 break;
      case Sco_SCOPE_DEGREE:
	 Action = ActRecDegLog;
	 break;
      default:
	 return;	// Nothing to do
     }

   /***** Write help message *****/
   sprintf (Gbl.Message,Txt_You_can_send_a_file_with_an_image_in_png_format_transparent_background_and_size_X_Y,
	    64,64);
   Lay_ShowAlert (Lay_INFO,Gbl.Message);

   /***** Write a form to send logo *****/
   Act_FormStart (Action);
   fprintf (Gbl.F.Out,"<table style=\"margin:0 auto;\">"
                      "<tr>"
                      "<td class=\"%s\" style=\"text-align:right;\">"
                      "%s:"
                      "</td>"
                      "<td style=\"text-align:left;\">"
                      "<input type=\"file\" name=\"%s\" size=\"40\" maxlength=\"100\" value=\"\" />"
                      "</td>"
                      "</tr>"
                      "<tr>"
                      "<td colspan=\"2\" style=\"text-align:center;\">"
                      "<input type=\"submit\" value=\"%s\" accept=\"image/jpeg\" />"
                      "</td>"
                      "</tr>"
                      "</table>"
                      "</form>",
            The_ClassFormul[Gbl.Prefs.Theme],
            Txt_File_with_the_logo,
            Fil_NAME_OF_PARAM_FILENAME_ORG,
            Txt_Upload_logo);
  }

/*****************************************************************************/
/******* Receive the logo of the current institution, centre or degree *******/
/*****************************************************************************/

void Log_ReceiveLogo (Sco_Scope_t Scope)
  {
   extern const char *Txt_The_file_is_not_X;
   long Cod;
   const char *Folder;
   void (*FunctionConfiguration) (void);
   char Path[PATH_MAX+1];
   char FileNameLogoSrc[PATH_MAX+1];
   char MIMEType[Brw_MAX_BYTES_MIME_TYPE+1];
   char FileNameLogo[PATH_MAX+1];        // Full name (including path and .png) of the destination file
   bool WrongType = false;

   /***** Set variables depending on scope *****/
   switch (Scope)
     {
      case Sco_SCOPE_INSTITUTION:
	 Cod = Gbl.CurrentIns.Ins.InsCod;
	 Folder = Cfg_FOLDER_INS;
	 FunctionConfiguration = Ins_ShowConfiguration;
	 break;
      case Sco_SCOPE_CENTRE:
	 Cod = Gbl.CurrentCtr.Ctr.CtrCod;
	 Folder = Cfg_FOLDER_CTR;
	 FunctionConfiguration = Ctr_ShowConfiguration;
	 break;
      case Sco_SCOPE_DEGREE:
	 Cod = Gbl.CurrentDeg.Deg.DegCod;
	 Folder = Cfg_FOLDER_DEG;
	 FunctionConfiguration = Deg_ShowConfiguration;
	 break;
      default:
	 return;	// Nothing to do
     }

   /***** Creates directories if not exist *****/
   sprintf (Path,"%s/%s",
	    Cfg_PATH_SWAD_PUBLIC,Folder);
   Fil_CreateDirIfNotExists (Path);
   sprintf (Path,"%s/%s/%02u",
	    Cfg_PATH_SWAD_PUBLIC,Folder,
	    (unsigned) (Cod % 100));
   Fil_CreateDirIfNotExists (Path);
   sprintf (Path,"%s/%s/%02u/%u",
	    Cfg_PATH_SWAD_PUBLIC,Folder,
	    (unsigned) (Cod % 100),
	    (unsigned) Cod);
   Fil_CreateDirIfNotExists (Path);
   sprintf (Path,"%s/%s/%02u/%u/logo",
	    Cfg_PATH_SWAD_PUBLIC,Folder,
	    (unsigned) (Cod % 100),
	    (unsigned) Cod);
   Fil_CreateDirIfNotExists (Path);

   /***** Copy in disk the file received from stdin (really from Gbl.F.Tmp) *****/
   Fil_StartReceptionOfFile (FileNameLogoSrc,MIMEType);

   /* Check if the file type is image/jpeg or image/pjpeg or application/octet-stream */
   if (strcmp (MIMEType,"image/png"))
      if (strcmp (MIMEType,"image/x-png"))
         if (strcmp (MIMEType,"application/octet-stream"))
            if (strcmp (MIMEType,"application/octetstream"))
               if (strcmp (MIMEType,"application/octet"))
                  WrongType = true;
   if (WrongType)
     {
      sprintf (Gbl.Message,Txt_The_file_is_not_X,"png");
      Lay_ShowAlert (Lay_WARNING,Gbl.Message);
      return;
     }

   /* End the reception of logo in a temporary file */
   sprintf (FileNameLogo,"%s/%s/%02u/%u/logo/%u.png",
	    Cfg_PATH_SWAD_PUBLIC,Folder,
	    (unsigned) (Cod % 100),
	    (unsigned) Cod,
	    (unsigned) Cod);
   if (!Fil_EndReceptionOfFile (FileNameLogo))
     {
      Lay_ShowAlert (Lay_WARNING,"Error uploading file.");
      return;
     }

   /***** Show the institution/centre/degree information again *****/
   FunctionConfiguration ();
  }