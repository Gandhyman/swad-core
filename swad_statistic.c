// swad_statistic.c: statistics

/*
    SWAD (Shared Workspace At a Distance),
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
#include <linux/stddef.h>	// For NULL
#include <math.h>		// For log10, floor, ceil, modf, sqrt...
#include <stdlib.h>		// For system, getenv, etc.
#include <string.h>		// For string functions
#include <sys/wait.h>		// For the macro WEXITSTATUS
#include <unistd.h>		// For unlink

#include "swad_action.h"
#include "swad_config.h"
#include "swad_course.h"
#include "swad_database.h"
#include "swad_file_browser.h"
#include "swad_forum.h"
#include "swad_global.h"
#include "swad_ID.h"
#include "swad_logo.h"
#include "swad_network.h"
#include "swad_notice.h"
#include "swad_notification.h"
#include "swad_parameter.h"
#include "swad_statistic.h"
#include "swad_tab.h"
#include "swad_web_service.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;
extern struct Act_Actions Act_Actions[Act_NUM_ACTIONS];

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

#define Sta_SECONDS_IN_RECENT_LOG ((time_t)(Cfg_DAYS_IN_RECENT_LOG*24UL*60UL*60UL))	// Remove entries in recent log oldest than this time

/*****************************************************************************/
/******************************* Private types *******************************/
/*****************************************************************************/

struct Sta_SizeOfFileZones
  {
   int NumCrss;	// -1 stands for not aplicable
   int NumGrps;	// -1 stands for not aplicable
   int NumUsrs;	// -1 stands for not aplicable
   unsigned MaxLevels;
   unsigned long NumFolders;
   unsigned long NumFiles;
   unsigned long long int Size;	// Total size in bytes
  };

struct Sta_StatsForum
  {
   unsigned NumForums;
   unsigned NumThreads;
   unsigned NumPosts;
   unsigned NumUsrsToBeNotifiedByEMail;
  };

/*****************************************************************************/
/***************************** Internal prototypes ***************************/
/*****************************************************************************/

static void Sta_PutFormToRequestAccessesCrs (void);

static void Sta_PutSeeAccessesButton (void);
static void Sta_WriteSelectorCountType (void);
static void Sta_WriteSelectorAction (void);
static bool Sta_SeeAccesses (void);
static void Sta_ShowDetailedAccessesList (unsigned long NumRows,MYSQL_RES *mysql_res);
static void Sta_WriteLogComments (long LogCod);
static void Sta_ShowNumAccessesPerUsr (unsigned long NumRows,MYSQL_RES *mysql_res);
static void Sta_ShowNumAccessesPerDays (unsigned long NumRows,MYSQL_RES *mysql_res);
static void Sta_ShowDistrAccessesPerDaysAndHour (unsigned long NumRows,MYSQL_RES *mysql_res);
static Sta_ColorType_t Sta_GetStatColorType (void);
static void Sta_DrawBarColors (Sta_ColorType_t ColorType,float MaxPagesGenerated);
static void Sta_DrawAccessesPerHourForADay (Sta_ColorType_t ColorType,float NumPagesGenerated[24],float MaxPagesGenerated);
static void Sta_SetColor (Sta_ColorType_t ColorType,float NumPagesGenerated,float MaxPagesGenerated,unsigned *R,unsigned *G,unsigned *B);
static void Sta_ShowNumAccessesPerWeeks (unsigned long NumRows,MYSQL_RES *mysql_res);
static void Sta_ShowNumAccessesPerMonths (unsigned long NumRows,MYSQL_RES *mysql_res);
static void Sta_ShowNumAccessesPerHour (unsigned long NumRows,MYSQL_RES *mysql_res);
static void Sta_WriteAccessHour (unsigned Hour,float NumPagesGenerated,float MaxPagesGenerated,float TotalPagesGenerated,unsigned ColumnWidth);
static void Sta_ShowAverageAccessesPerMinute (unsigned long NumRows,MYSQL_RES *mysql_res);
static void Sta_WriteLabelsXAxisAccMin (float IncX,const char *Format);
static void Sta_WriteAccessMinute (unsigned Minute,float NumPagesGenerated,float MaxX);
static void Sta_ShowNumAccessesPerAction (unsigned long NumRows,MYSQL_RES *mysql_res);
static void Sta_ShowNumAccessesPerPlugin (unsigned long NumRows,MYSQL_RES *mysql_res);
static void Sta_ShowNumAccessesPerWSFunction (unsigned long NumRows,MYSQL_RES *mysql_res);
static void Sta_ShowNumAccessesPerBanner (unsigned long NumRows,MYSQL_RES *mysql_res);
static void Sta_ShowNumAccessesPerDegree (unsigned long NumRows,MYSQL_RES *mysql_res);
static void Sta_ShowNumAccessesPerCourse (unsigned long NumRows,MYSQL_RES *mysql_res);
static void Sta_WriteDegree (long DegCod);
static void Sta_DrawBarNumClicks (char Color,float NumPagesGenerated,float MaxPagesGenerated,float TotalPagesGenerated,unsigned MaxBarWidth);
static void Sta_WriteSelectedRangeOfDates (unsigned NumDays);
static void Sta_WriteFloatNum (float Number);

static void Sta_GetAndShowDegCrsStats (void);
static void Sta_WriteHeadDegsCrssInSWAD (void);
static void Sta_GetAndShowNumCtysInSWAD (void);
static void Sta_GetAndShowNumInssInSWAD (void);
static void Sta_GetAndShowNumCtrsInSWAD (void);
static void Sta_GetAndShowNumDegsInSWAD (void);
static void Sta_GetAndShowNumCrssInSWAD (void);

static void Sta_GetAndShowUsersStats (void);

static void Sta_GetAndShowFileBrowsersStats (void);
static void Sta_WriteStatsExpTreesTableHead (void);
static void Sta_WriteRowStatsFileBrowsers (Brw_FileBrowser_t FileZone,const char *NameOfFileZones);
static void Sta_GetSizeOfFileZoneFromDB (Sco_Scope_t Scope,
                                         Brw_FileBrowser_t FileBrowser,
                                         struct Sta_SizeOfFileZones *SizeOfFileZones);

static void Sta_GetAndShowOERsStats (void);
static void Sta_GetNumberOfOERsFromDB (Sco_Scope_t Scope,Brw_License_t License,unsigned long NumFiles[2]);

static void Sta_GetAndShowAssignmentsStats (void);
static void Sta_GetAndShowTestsStats (void);
static void Sta_GetAndShowNoticesStats (void);
static void Sta_GetAndShowMsgsStats (void);

static void Sta_GetAndShowForumStats (void);
static void Sta_ShowStatOfAForumType (For_ForumType_t ForumType,
                                      long InsCod,long CtrCod,long DegCod,long CrsCod,
                                      struct Sta_StatsForum *StatsForum);
static void Sta_WriteForumTitleAndStats (For_ForumType_t ForumType,
                                         long InsCod,long CtrCod,long DegCod,long CrsCod,
                                         const char *Icon,struct Sta_StatsForum *StatsForum,
                                         const char *ForumName1,const char *ForumName2);
static void Sta_WriteForumTotalStats (struct Sta_StatsForum *StatsForum);

static void Sta_GetAndShowSurveysStats (void);
static void Sta_GetAndShowNumUsrsPerLanguage (void);
static void Sta_GetAndShowNumUsrsPerLayout (void);
static void Sta_GetAndShowNumUsrsPerTheme (void);
static void Sta_GetAndShowNumUsrsPerIconSet (void);
static void Sta_GetAndShowNumUsrsPerMenu (void);
static void Sta_GetAndShowNumUsrsPerSideColumns (void);
static void Sta_GetAndShowNumUsrsPerNotifyEvent (void);

/*****************************************************************************/
/*************** Read CGI environment variable REMOTE_ADDR *******************/
/*****************************************************************************/
/*
CGI Environment Variables:
REMOTE_ADDR
The IP address of the remote host making the request.
*/
void Sta_GetRemoteAddr (void)
  {
   if (getenv ("REMOTE_ADDR"))
     {
      strncpy (Gbl.IP,getenv ("REMOTE_ADDR"),Cns_MAX_LENGTH_IP);
      Gbl.IP[Cns_MAX_LENGTH_IP] = '\0';
     }
   else
      Gbl.IP[0] = '\0';
  }

/*****************************************************************************/
/********* If this click is made from the same IP too fast, abort ************/
/*****************************************************************************/
/*
void Sta_ExitIfTooFast (void)
  {
   extern const char *Txt_STR_LANG_ID[Txt_NUM_LANGUAGES];
   extern const char *Txt_Too_fast;
   extern const char *Txt_Go_back;
   char Query[512];
   unsigned NumClicks;	// Very recent clicks from this IP (0 or 1)

   ***** Some actions can be made fast *****
   if (Gbl.CurrentAct == ActRefCon    ||	// Refresh connected
       Gbl.CurrentAct == ActRefLstClk ||	// Refresh last clicks
       Gbl.CurrentAct == ActAutUsrChgLan)	// Change my language automatically just after log in
      return;

   ***** Get if a click/refresh is made from the same IP very recently *****
   sprintf (Query,"SELECT COUNT(*) FROM IP_last"
                  " WHERE IP='%s' AND UNIX_TIMESTAMP(LastClick)>UNIX_TIMESTAMP()-%u",
            Gbl.IP,Cfg_MIN_TIME_BETWEEN_2_CLICKS_FROM_THE_SAME_IP);
   NumClicks = (unsigned) DB_QueryCOUNT (Query,"can not get the number of very recent clicks");

   ***** Remove old clicks/refreshes *****
   sprintf (Query,"DELETE FROM IP_last"
                  " WHERE UNIX_TIMESTAMP(LastClick)<=UNIX_TIMESTAMP()-%u",
                  Cfg_MIN_TIME_BETWEEN_2_CLICKS_FROM_THE_SAME_IP);
   DB_QueryDELETE (Query,"can not remove old last IP");

   ***** Replace last click/refresh from this IP *****
   sprintf (Query,"REPLACE INTO IP_last"
                  " (IP,LastClick)"
                  " VALUES ('%s',NOW())",
	    Gbl.IP);
   DB_QueryREPLACE (Query,"can not update last IP");

   ***** If too fast, write warning and exit *****
   if (NumClicks)	// Too fast
     {
      ***** Write header to standard output to avoid timeout *****
      // Two \r\n are necessary
      fprintf (stdout,"Content-type: text/html; charset=windows-1252\r\n\r\n"
                      "<!DOCTYPE html>\n"
                      "<html lang=\"%s\">\n"
		      "<head>"
		      "<title>%s</title>"
		      "</head>"
		      "<body>"
		      "<h1 style=\"text-align:center;\">%s</h1>"
		      "<h2 style=\"text-align:center;\">"
		      "<a href=\"javascript:window.history.back();\">&larr; %s</a>"
		      "</h2>"
		      "</body>"
		      "</html>",
	       Txt_STR_LANG_ID[Gbl.Prefs.Language],
	       Cfg_PLATFORM_FULL_NAME,
	       Txt_Too_fast,
	       Txt_Go_back);
      exit (0);
      // sleep (Cfg_MIN_TIME_BETWEEN_2_CLICKS_FROM_THE_SAME_IP);	// Sleep those seconds
     }
  }
*/
/*****************************************************************************/
/**************************** Log access in database *************************/
/*****************************************************************************/

void Sta_LogAccess (const char *Comments)
  {
   char Query[2048];
   long LogCod;
   Rol_Role_t RoleToStore = (Gbl.CurrentAct == ActLogOut) ? Gbl.Usrs.Me.LoggedRoleBeforeCloseSession :
                                                            Gbl.Usrs.Me.LoggedRole;

   /***** Insert access into database *****/
   /* Log access in historical log */
   sprintf (Query,"INSERT INTO log (ActCod,DegCod,CrsCod,UsrCod,"
	          "Role,ClickTime,TimeToGenerate,TimeToSend,IP)"
                  " VALUES ('%ld','%ld','%ld','%ld',"
                  "'%u',NOW(),'%ld','%ld','%s')",
            Act_Actions[Gbl.CurrentAct].ActCod,
            Gbl.CurrentDeg.Deg.DegCod,
            Gbl.CurrentCrs.Crs.CrsCod,
            Gbl.Usrs.Me.UsrDat.UsrCod,
            (unsigned) RoleToStore,
            Gbl.TimeGenerationInMicroseconds,
            Gbl.TimeSendInMicroseconds,
            Gbl.IP);
   if (Gbl.WebService.IsWebService)
     {
      if (mysql_query (&Gbl.mysql,Query))
         Svc_Exit ("can not log access (historical)");
      LogCod = (long) mysql_insert_id (&Gbl.mysql);
     }
   else
      LogCod = DB_QueryINSERTandReturnCode (Query,"can not log access (historical)");

   /* Log access in recent log */
   sprintf (Query,"INSERT INTO log_recent (LogCod,ActCod,DegCod,CrsCod,UsrCod,"
	          "Role,ClickTime,TimeToGenerate,TimeToSend,IP)"
                  " VALUES ('%ld','%ld','%ld','%ld','%ld',"
                  "'%u',NOW(),'%ld','%ld','%s')",
            LogCod,Act_Actions[Gbl.CurrentAct].ActCod,
            Gbl.CurrentDeg.Deg.DegCod,
            Gbl.CurrentCrs.Crs.CrsCod,
            Gbl.Usrs.Me.UsrDat.UsrCod,
            (unsigned) RoleToStore,
            Gbl.TimeGenerationInMicroseconds,
            Gbl.TimeSendInMicroseconds,
            Gbl.IP);
   if (Gbl.WebService.IsWebService)
     {
      if (mysql_query (&Gbl.mysql,Query))
         Svc_Exit ("can not log access (recent)");
     }
   else
      DB_QueryINSERT (Query,"can not log access (recent)");

   if (Comments)
     {
      /* Log comments */
      sprintf (Query,"INSERT INTO log_comments (LogCod,Comments)"
                     " VALUES ('%ld','",
	       LogCod);
      Str_AddStrToQuery (Query,Comments,sizeof (Query));
      strcat (Query,"')");

      if (Gbl.WebService.IsWebService)
        {
         if (mysql_query (&Gbl.mysql,Query))
            Svc_Exit ("can not log access (comments)");
        }
      else
         DB_QueryINSERT (Query,"can not log access (comments)");
     }

   if (Gbl.WebService.IsWebService)
     {
      /* Log web service plugin and function */
      sprintf (Query,"INSERT INTO log_ws (LogCod,PlgCod,FunCod)"
                     " VALUES ('%ld','%ld','%u')",
	       LogCod,Gbl.WebService.PlgCod,(unsigned) Gbl.WebService.Function);

      if (mysql_query (&Gbl.mysql,Query))
         Svc_Exit ("can not log access (comments)");
     }
   else if (Gbl.Banners.BanCodClicked > 0)
     {
      /* Log banner clicked */
      sprintf (Query,"INSERT INTO log_banners (LogCod,BanCod)"
                     " VALUES ('%ld','%ld')",
	       LogCod,Gbl.Banners.BanCodClicked);
      DB_QueryINSERT (Query,"can not log banner clicked");
     }
  }

/*****************************************************************************/
/************ Sometimes, we delete old entries in recent log table ***********/
/*****************************************************************************/

void Sta_RemoveOldEntriesRecentLog (void)
  {
   char Query[512];

   /***** Remove all expired clipboards *****/
   sprintf (Query,"DELETE LOW_PRIORITY FROM log_recent"
                  " WHERE UNIX_TIMESTAMP(ClickTime)<UNIX_TIMESTAMP()-%ld",
            Sta_SECONDS_IN_RECENT_LOG);
   DB_QueryDELETE (Query,"can not remove old entries from recent log");
  }

/*****************************************************************************/
/*************** Write a form to go to result of users' tests ****************/
/*****************************************************************************/

static void Sta_PutFormToRequestAccessesCrs (void)
  {
   extern const char *The_ClassFormul[The_NUM_THEMES];
   extern const char *Txt_Visits_to_course;

   Act_FormStart (ActReqAccCrs);
   Act_LinkFormSubmit (Txt_Visits_to_course,The_ClassFormul[Gbl.Prefs.Theme]);
   Lay_PutSendIcon ("stats",Txt_Visits_to_course,Txt_Visits_to_course);
   fprintf (Gbl.F.Out,"</form>");
  }

/*****************************************************************************/
/******************** Show a form to make a query of clicks ******************/
/*****************************************************************************/

void Sta_AskSeeCrsAccesses (void)
  {
   extern const char *The_ClassFormul[The_NUM_THEMES];
   extern const char *Txt_No_teachers_or_students_found;
   extern const char *Txt_distributed_by;
   extern const char *Txt_STAT_CLICK_STAT_TYPES[Sta_NUM_TYPES_CLICK_STATS];
   extern const char *Txt_results_per_page;
   static unsigned long RowsPerPage[] = {10,20,30,40,50,100,500,1000,5000,10000,50000,100000};
#define NUM_OPTIONS_ROWS_PER_PAGE (sizeof (RowsPerPage) / sizeof (RowsPerPage[0]))
   Sta_ClicksStatType_t ClicksStatType;
   unsigned long i;

   /***** Get and update type of list, number of columns in class photo and preference about view photos *****/
   Usr_GetAndUpdatePrefsAboutUsrList ();

   /***** Show form to select the grupos *****/
   Grp_ShowFormToSelectSeveralGroups (ActReqAccCrs);

   /***** Form to select type of list used for select several users *****/
   Usr_ShowFormsToSelectUsrListType (ActReqAccCrs);

   /***** Get and order the lists of users of this course *****/
   Usr_GetUsrsLst (Rol_ROLE_TEACHER,Sco_SCOPE_COURSE,NULL,false);
   Usr_GetUsrsLst (Rol_ROLE_STUDENT,Sco_SCOPE_COURSE,NULL,false);

   if (Gbl.Usrs.LstTchs.NumUsrs ||
       Gbl.Usrs.LstStds.NumUsrs)
     {
      if (Usr_GetIfShowBigList (Gbl.Usrs.LstTchs.NumUsrs +
	                        Gbl.Usrs.LstStds.NumUsrs))
        {
         /***** Get list of selected users *****/
         Usr_GetListSelectedUsrs ();

         Act_FormStart (ActSeeAccCrs);
         Grp_PutParamsCodGrps ();
         Par_PutHiddenParamLong ("FirstRow",0);
         Par_PutHiddenParamLong ("LastRow",0);

         /***** Draw two class photographs with the users: one for teachers of the course and another one for students *****/
         /* Start the table */
         fprintf (Gbl.F.Out,"<table style=\"margin:0 auto; border-spacing:4px;\">"
                            "<tr>"
                            "<td colspan=\"2\" style=\"text-align:left;\">");
         Lay_StartRoundFrameTable10 (NULL,0,NULL);

         /* Put list of users to select some users */
         Usr_ListUsersToSelect (Rol_ROLE_TEACHER);
         Usr_ListUsersToSelect (Rol_ROLE_STUDENT);

         /* End the table */
         Lay_EndRoundFrameTable10 ();
         fprintf (Gbl.F.Out,"</td>"
                            "</tr>");

         /***** Initial and final dates of the search *****/
         Dat_WriteFormIniEndDates ();

         /***** Selection of action *****/
         Sta_WriteSelectorAction ();

         /***** Selection of count type (number of pages generated, accesses per user, etc.) *****/
         Sta_WriteSelectorCountType ();

         /***** Type of statistic *****/
         fprintf (Gbl.F.Out,"<tr>"
                            "<td class=\"%s\""
                            " style=\"text-align:right; vertical-align:top;\">"
                            "%s:"
                            "</td>"
                            "<td class=\"DAT\""
                            " style=\"text-align:left; vertical-align:top;\">",
                  The_ClassFormul[Gbl.Prefs.Theme],Txt_distributed_by);
         if ((Gbl.Stat.ClicksStatType < Sta_ACC_CRS_PER_USR ||
              Gbl.Stat.ClicksStatType > Sta_ACC_CRS_PER_ACTION) &&
              Gbl.Stat.ClicksStatType != Sta_ACC_CRS_LISTING)
            Gbl.Stat.ClicksStatType = Sta_ACC_CRS_PER_USR;
         for (ClicksStatType = Sta_ACC_CRS_PER_USR;
              ClicksStatType <= Sta_ACC_CRS_PER_ACTION;
              ClicksStatType++)
           {
            fprintf (Gbl.F.Out,"<input type=\"radio\" name=\"ClickStatType\" value=\"%u\"",
                     (unsigned) ClicksStatType);
            if (ClicksStatType == Gbl.Stat.ClicksStatType)
               fprintf (Gbl.F.Out," checked=\"checked\"");
            fprintf (Gbl.F.Out," onclick=\"disableRowsPage()\" />%s<br />",
                     Txt_STAT_CLICK_STAT_TYPES[ClicksStatType]);
           }
         fprintf (Gbl.F.Out,"</td>"
                            "</tr>");

         /* Listing of clicks to this course  */
         fprintf (Gbl.F.Out,"<tr>"
                            "<td colspan=\"2\" class=\"%s\""
                            " style=\"text-align:center; vertical-align:middle;\">"
                            "<input type=\"radio\" name=\"ClickStatType\" value=\"%u\"",
                  The_ClassFormul[Gbl.Prefs.Theme],(unsigned) Sta_ACC_CRS_LISTING);
         if (Gbl.Stat.ClicksStatType == Sta_ACC_CRS_LISTING)
            fprintf (Gbl.F.Out," checked=\"checked\"");
         fprintf (Gbl.F.Out," onclick=\"enableRowsPage()\" />%s",
                  Txt_STAT_CLICK_STAT_TYPES[Sta_ACC_CRS_LISTING]);

         /* Number of rows per page */
         // To use getElementById in Firefox, it's necessary to have the id attribute
         fprintf (Gbl.F.Out," (%s: <select id=\"RowsPage\" name=\"RowsPage\"",
                  Txt_results_per_page);
         if (Gbl.Stat.ClicksStatType != Sta_ACC_CRS_LISTING)
            fprintf (Gbl.F.Out," disabled=\"disabled\"");
         fprintf (Gbl.F.Out,">");
         for (i = 0;
              i < NUM_OPTIONS_ROWS_PER_PAGE;
              i++)
           {
            fprintf (Gbl.F.Out,"<option");
            if (RowsPerPage[i] == Gbl.Stat.RowsPerPage)
	       fprintf (Gbl.F.Out," selected=\"selected\"");
            fprintf (Gbl.F.Out,">%lu",RowsPerPage[i]);
           }
         fprintf (Gbl.F.Out,"</select>)"
                            "</td>"
                            "</tr>");

         /***** Submit button *****/
         Sta_PutSeeAccessesButton ();

         /***** Form end *****/
         fprintf (Gbl.F.Out,"</table>"
                            "</form>");

         /* Free the memory used by the list of users */
         Usr_FreeListsEncryptedUsrCods ();
        }
     }
   else	// No teachers nor students found
      Lay_ShowAlert (Lay_WARNING,Txt_No_teachers_or_students_found);

   /* Free memory used by the lists */
   Usr_FreeUsrsList (&Gbl.Usrs.LstTchs);
   Usr_FreeUsrsList (&Gbl.Usrs.LstStds);

   /* Free memory for list of selected groups */
   Grp_FreeListCodSelectedGrps ();
  }

/*****************************************************************************/
/********** Show a form to select the type of global stat of clics ***********/
/*****************************************************************************/

void Sta_AskSeeGblAccesses (void)
  {
   extern const char *The_ClassFormul[The_NUM_THEMES];
   extern const char *Txt_Users;
   extern const char *Txt_ROLE_STATS[Sta_NUM_ROLES_STAT];
   extern const char *Txt_Scope;
   extern const char *Txt_distributed_by;
   extern const char *Txt_STAT_CLICK_STAT_TYPES[Sta_NUM_TYPES_CLICK_STATS];
   Sta_Role_t RoleStat;
   Sta_ClicksStatType_t ClicksStatType;

   /***** Put form to go to test edition and configuration *****/
   if (Gbl.CurrentCrs.Crs.CrsCod > 0 &&			// Course selected
       (Gbl.Usrs.Me.LoggedRole == Rol_ROLE_TEACHER ||
        Gbl.Usrs.Me.LoggedRole == Rol_ROLE_SUPERUSER))
     {
      fprintf (Gbl.F.Out,"<div style=\"padding-bottom:10px; text-align:center;\">");
      Sta_PutFormToRequestAccessesCrs ();
      fprintf (Gbl.F.Out,"</div>");
     }

   /***** Start form *****/
   Act_FormStart (ActSeeAccGbl);
   fprintf (Gbl.F.Out,"<table class=\"CELLS_PAD_2\" style=\"margin:0 auto;\">");

   /***** Start and end dates for the search *****/
   Dat_WriteFormIniEndDates ();

   /***** Users' roles whose accesses we want to see *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"%s\""
                      " style=\"text-align:right; vertical-align:middle;\">"
                      "%s:"
                      "</td>"
                      "<td style=\"text-align:left; vertical-align:middle;\">"
                      "<select name=\"Role\">",
            The_ClassFormul[Gbl.Prefs.Theme],Txt_Users);
   for (RoleStat = (Sta_Role_t) 0;
	RoleStat < Sta_NUM_ROLES_STAT;
	RoleStat++)
     {
      fprintf (Gbl.F.Out,"<option value=\"%u\"",(unsigned) RoleStat);
      if (RoleStat == Gbl.Stat.Role)
	 fprintf (Gbl.F.Out," selected=\"selected\"");
      fprintf (Gbl.F.Out,">%s",Txt_ROLE_STATS[RoleStat]);
     }
   fprintf (Gbl.F.Out,"</select>"
	              "</td>"
	              "</tr>");

   /***** Selection of action *****/
   Sta_WriteSelectorAction ();

   /***** Clicks made from anywhere, current centre, current degree or current course *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"%s\""
                      " style=\"text-align:right; vertical-align:middle;\">"
                      "%s:"
                      "</td>"
                      "<td style=\"text-align:left; vertical-align:middle;\">",
            The_ClassFormul[Gbl.Prefs.Theme],Txt_Scope);
   Gbl.Scope.Allowed = 1 << Sco_SCOPE_PLATFORM    |
	               1 << Sco_SCOPE_COUNTRY     |
		       1 << Sco_SCOPE_INSTITUTION |
		       1 << Sco_SCOPE_CENTRE      |
		       1 << Sco_SCOPE_DEGREE      |
		       1 << Sco_SCOPE_COURSE;
   Gbl.Scope.Default = Sco_SCOPE_PLATFORM;
   Sco_GetScope ();
   Sco_PutSelectorScope (false);
   fprintf (Gbl.F.Out,"</td>"
	              "</tr>");

   /***** Count type for the statistic *****/
   Sta_WriteSelectorCountType ();

   /***** Type of statistic *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"%s\""
                      " style=\"text-align:right; vertical-align:top;\">"
                      "%s:"
                      "</td>"
                      "<td class=\"DAT\""
                      " style=\"text-align:left; vertical-align:top;\">",
            The_ClassFormul[Gbl.Prefs.Theme],Txt_distributed_by);
   if (Gbl.Stat.ClicksStatType < Sta_ACC_GBL_PER_DAYS ||
       Gbl.Stat.ClicksStatType > Sta_ACC_GBL_PER_COURSE)
      Gbl.Stat.ClicksStatType = Sta_ACC_GBL_PER_DAYS;
   for (ClicksStatType = Sta_ACC_GBL_PER_DAYS;
	ClicksStatType <= Sta_ACC_GBL_PER_COURSE;
	ClicksStatType++)
     {
      fprintf (Gbl.F.Out,"<input type=\"radio\" name=\"ClickStatType\" value=\"%u\"",
               (unsigned) ClicksStatType);
      if (ClicksStatType == Gbl.Stat.ClicksStatType)
         fprintf (Gbl.F.Out," checked=\"checked\"");
      fprintf (Gbl.F.Out," />%s<br />",
               Txt_STAT_CLICK_STAT_TYPES[ClicksStatType]);
     }
   fprintf (Gbl.F.Out,"</td>"
	              "</tr>");

   /***** Submit button *****/
   Sta_PutSeeAccessesButton ();

   /***** Form end *****/
   fprintf (Gbl.F.Out,"</table>"
                      "</form>");
  }

/*****************************************************************************/
/********************** Put submit button to see accesses ********************/
/*****************************************************************************/

static void Sta_PutSeeAccessesButton (void)
  {
   extern const char *Txt_Show_visits;

   fprintf (Gbl.F.Out,"<tr>"
                      "<td colspan=\"2\" style=\"text-align:center;\">"
                      "<input type=\"submit\" value=\"%s\" />"
                      "</td>"
                      "</tr>",
            Txt_Show_visits);
  }

/*****************************************************************************/
/****** Put selectors for type of access count and for degree or course ******/
/*****************************************************************************/

static void Sta_WriteSelectorCountType (void)
  {
   extern const char *The_ClassFormul[The_NUM_THEMES];
   extern const char *Txt_Show;
   extern const char *Txt_STAT_TYPE_COUNT_SMALL[Sta_NUM_STAT_COUNT_TYPES];
   Sta_CountType_t StatCountType;

   /**** Count type *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"%s\""
                      " style=\"text-align:right; vertical-align:top;\">"
                      "%s:"
                      "</td>"
                      "<td style=\"text-align:left; vertical-align:top;\">"
                      "<select name=\"CountType\" id=\"CountType\">",
            The_ClassFormul[Gbl.Prefs.Theme],Txt_Show);
   for (StatCountType = (Sta_CountType_t) 0;
	StatCountType < Sta_NUM_STAT_COUNT_TYPES;
	StatCountType++)
     {
      fprintf (Gbl.F.Out,"<option value=\"%u\"",(unsigned) StatCountType);
      if (StatCountType == Gbl.Stat.CountType)
	 fprintf (Gbl.F.Out," selected=\"selected\"");
      fprintf (Gbl.F.Out,">%s",Txt_STAT_TYPE_COUNT_SMALL[StatCountType]);
     }
   fprintf (Gbl.F.Out,"</select>"
	              "</td>"
	              "</tr>");
  }

/*****************************************************************************/
/****** Put selectors for type of access count and for degree or course ******/
/*****************************************************************************/

static void Sta_WriteSelectorAction (void)
  {
   extern const char *The_ClassFormul[The_NUM_THEMES];
   extern const char *Txt_Action;
   extern const char *Txt_TABS_SHORT_TXT[Tab_NUM_TABS];
   Act_Action_t NumAction;
   char ActTxt[Act_MAX_LENGTH_ACTION_TXT+1];

   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"%s\""
                      " style=\"text-align:right; vertical-align:top;\">"
                      "%s:"
                      "</td>"
                      "<td style=\"text-align:left; vertical-align:top;\">"
                      "<select name=\"StatAct\" id=\"StatAct\" style=\"width:300px;\">",
            The_ClassFormul[Gbl.Prefs.Theme],Txt_Action);
   for (NumAction = (Act_Action_t) 0;
	NumAction < Act_NUM_ACTIONS;
	NumAction++)
     {
      fprintf (Gbl.F.Out,"<option value=\"%u\"",(unsigned) NumAction);
      if (NumAction == Gbl.Stat.NumAction)
	 fprintf (Gbl.F.Out," selected=\"selected\"");
      fprintf (Gbl.F.Out,">");
      if (NumAction)
         fprintf (Gbl.F.Out,"%u: ",NumAction);
      if (Txt_TABS_SHORT_TXT[Act_Actions[NumAction].Tab])
         fprintf (Gbl.F.Out,"%s &gt; ",
                  Txt_TABS_SHORT_TXT[Act_Actions[NumAction].Tab]);
      fprintf (Gbl.F.Out,"%s",Act_GetActionTextFromDB (Act_Actions[NumAction].ActCod,ActTxt));
     }

   fprintf (Gbl.F.Out,"</select>"
	              "</td>"
	              "</tr>");
  }

/*****************************************************************************/
/************ Set end date to current date                        ************/
/************ and set initial date to end date minus several days ************/
/*****************************************************************************/

void Sta_SetIniEndDates (void)
  {
   extern const unsigned Dat_NumDaysMonth[1+12];			// Declaration in swad_date.c

   Gbl.DateRange.DateEnd.Day   = Gbl.Now.Date.Day;
   Gbl.DateRange.DateEnd.Month = Gbl.Now.Date.Month;
   Gbl.DateRange.DateEnd.Year  = Gbl.Now.Date.Year;

   if (Gbl.DateRange.DateEnd.Day >= Cfg_DAYS_IN_RECENT_LOG)
     {
      Gbl.DateRange.DateIni.Year  = Gbl.DateRange.DateEnd.Year;
      Gbl.DateRange.DateIni.Month = Gbl.DateRange.DateEnd.Month;
      Gbl.DateRange.DateIni.Day   = Gbl.DateRange.DateEnd.Day - (Cfg_DAYS_IN_RECENT_LOG-1);
     }
   else
     {
      Gbl.DateRange.DateIni.Year  = (Gbl.DateRange.DateEnd.Month == 1) ? Gbl.DateRange.DateEnd.Year - 1 :
                                                                         Gbl.DateRange.DateEnd.Year;
      Gbl.DateRange.DateIni.Month = (Gbl.DateRange.DateEnd.Month == 1) ? 12 :
                                                                         Gbl.DateRange.DateEnd.Month - 1;
      Gbl.DateRange.DateIni.Day   = ((Gbl.DateRange.DateIni.Month == 2) ? Dat_GetNumDaysFebruary (Gbl.DateRange.DateIni.Year) :
	                                                                  Dat_NumDaysMonth[Gbl.DateRange.DateIni.Month]) +
                                                                          Gbl.DateRange.DateEnd.Day -
                                                                          (Cfg_DAYS_IN_RECENT_LOG-1);
     }
  }

/*****************************************************************************/
/******************** Compute and show access statistics *********************/
/*****************************************************************************/

void Sta_SeeCrsAccesses (void)
  {
   if (!Sta_SeeAccesses ())
      Sta_AskSeeCrsAccesses ();
  }

void Sta_SeeGblAccesses (void)
  {
   if (!Sta_SeeAccesses ())
      Sta_AskSeeGblAccesses ();
  }

/*****************************************************************************/
/******************** Compute and show access statistics ********************/
/*****************************************************************************/

#define MAX_LENGTH_QUERY_ACCESS (1024 + (10+ID_MAX_LENGTH_USR_ID)*5000)

// Returns false on error

static bool Sta_SeeAccesses (void)
  {
   extern const char *Txt_User;
   extern const char *Txt_Users;
   extern const char *Txt_ROLE_STATS[Sta_NUM_ROLES_STAT];
   extern const char *Txt_Action;
   extern const char *Txt_The_graph_shows_the_NUMBER;
   extern const char *Txt_STAT_TYPE_COUNT_SMALL[Sta_NUM_STAT_COUNT_TYPES];
   extern const char *Txt_distributed_by;
   extern const char *Txt_STAT_CLICK_STAT_TYPES[Sta_NUM_TYPES_CLICK_STATS];
   extern const char *Txt_You_must_select_one_ore_more_users;
   extern const char *Txt_There_is_no_knowing_how_many_users_not_logged_have_accessed;
   extern const char *Txt_The_date_range_must_be_less_than_or_equal_to_X_days;
   extern const char *Txt_There_are_no_accesses_with_the_selected_search_criteria;
   extern const char *Txt_List_of_detailed_clicks_in_the_course_X;
   extern const char *Txt_Statistics_of_all_visits;
   extern const char *Txt_Statistics_of_visits_to_the_institution_X;
   extern const char *Txt_Statistics_of_visits_to_the_centre_X;
   extern const char *Txt_Statistics_of_visits_to_the_degree_X;
   extern const char *Txt_Statistics_of_visits_to_the_course_X;
   enum {STAT_GLOBAL,STAT_COURSE} StatsGlobalOrCourse;
   char Query[MAX_LENGTH_QUERY_ACCESS+1];
   char QueryAux[512];
   long LengthQuery;
   MYSQL_RES *mysql_res;
   unsigned long NumRows;
   char UnsignedStr[10+1];
   unsigned UnsignedNum;
   const char *LogTable;
   struct UsrData UsrDat;
   unsigned NumUsr = 0;
   const char *Ptr;
   char StrRole[16+1];
   char StrQueryCountType[256];
   unsigned NumDays;
   char ActTxt[Act_MAX_LENGTH_ACTION_TXT+1];

   /***** Initialize data structure of the user *****/
   Usr_UsrDataConstructor (&UsrDat);

   /***** Get initial and ending dates *****/
   Dat_GetIniEndDatesFromForm ();

   /***** Set table where to find depending on initial date *****/
   // If initial day is older than current day minus Cfg_DAYS_IN_RECENT_LOG, then use recent log table, else use historic log table */
   LogTable = (Dat_GetNumDaysBetweenDates (&Gbl.DateRange.DateIni,&Gbl.Now.Date) <= Cfg_DAYS_IN_RECENT_LOG) ? "log_recent" :
	                                                                                                      "log";

   /***** Get the type of stat of clicks ******/
   Par_GetParToText ("ClickStatType",UnsignedStr,10);
   if (sscanf (UnsignedStr,"%u",&UnsignedNum) != 1)
      Lay_ShowErrorAndExit ("Type of query of accesses is missing.");
   if (UnsignedNum >= Sta_NUM_TYPES_CLICK_STATS)
      Lay_ShowErrorAndExit ("Type of query of accesses is missing.");
   Gbl.Stat.ClicksStatType = (Sta_ClicksStatType_t) UnsignedNum;

   /***** Get the type of count of clicks *****/
   if (Gbl.Stat.ClicksStatType != Sta_ACC_CRS_LISTING)
     {
      Par_GetParToText ("CountType",UnsignedStr,10);
      if (sscanf (UnsignedStr,"%u",&UnsignedNum) != 1)
         Lay_ShowErrorAndExit ("Type of count is missing.");
      if (UnsignedNum >= Sta_NUM_STAT_COUNT_TYPES)
         Lay_ShowErrorAndExit ("Type of count is missing.");
      Gbl.Stat.CountType = (Sta_CountType_t) UnsignedNum;
     }

   /***** Get action *****/
   Par_GetParToText ("StatAct",UnsignedStr,10);
   if (sscanf (UnsignedStr,"%u",&UnsignedNum) != 1)
      Lay_ShowErrorAndExit ("Action is missing.");
   if (UnsignedNum >= Act_NUM_ACTIONS)
      Lay_ShowErrorAndExit ("Action is missing.");
   Gbl.Stat.NumAction = (Act_Action_t) UnsignedNum;

   switch (Gbl.Stat.ClicksStatType)
     {
      case Sta_ACC_GBL_PER_DAYS:
      case Sta_ACC_GBL_PER_DAYS_AND_HOUR:
      case Sta_ACC_GBL_PER_WEEKS:
      case Sta_ACC_GBL_PER_MONTHS:
      case Sta_ACC_GBL_PER_HOUR:
      case Sta_ACC_GBL_PER_MINUTE:
      case Sta_ACC_GBL_PER_ACTION:
      case Sta_ACC_GBL_PER_PLUGIN:
      case Sta_ACC_GBL_PER_Svc_FUNCTION:
      case Sta_ACC_GBL_PER_BANNER:
      case Sta_ACC_GBL_PER_DEGREE:
      case Sta_ACC_GBL_PER_COURSE:
         StatsGlobalOrCourse = STAT_GLOBAL;
	 break;
      default:
         StatsGlobalOrCourse = STAT_COURSE;
         break;
     }

   if (Gbl.CurrentAct == ActSeeAccCrs)
     {
      if (Gbl.Stat.ClicksStatType == Sta_ACC_CRS_LISTING)
	{
	 /****** Get the number of the first row to show ******/
	 Par_GetParToText ("FirstRow",UnsignedStr,10);
	 if (sscanf (UnsignedStr,"%lu",&Gbl.Stat.FirstRow) != 1)
	    Lay_ShowErrorAndExit ("Number of start row is missing.");

	 /****** Get the number of the last row to show ******/
	 Par_GetParToText ("LastRow",UnsignedStr,10);
	 if (sscanf (UnsignedStr,"%lu",&Gbl.Stat.LastRow) != 1)
	    Lay_ShowErrorAndExit ("Number of end row is missing.");

	 /****** Get the number of rows per page ******/
	 Par_GetParToText ("RowsPage",UnsignedStr,10);
	 if (sscanf (UnsignedStr,"%lu",&Gbl.Stat.RowsPerPage) != 1)
	    Lay_ShowErrorAndExit ("Number of rows per page is missing.");
	}

      /****** Get list of selected users ******/
      Usr_GetListSelectedUsrs ();

      /* Check the number of users whose clicks will be shown */
      if (!Usr_CountNumUsrsInEncryptedList ())	// If there are no users selected...
	{					// ...write warning message and show the form again
	 Lay_ShowAlert (Lay_WARNING,Txt_You_must_select_one_ore_more_users);
	 return false;
	}
     }
   else // Gbl.CurrentAct == ActSeeAccGbl
     {
      /***** Get the type of user of clicks *****/
      Par_GetParToText ("Role",UnsignedStr,10);
      if (sscanf (UnsignedStr,"%u",&UnsignedNum) != 1)
         Lay_ShowErrorAndExit ("Type of users is missing.");
      if (UnsignedNum >= Sta_NUM_ROLES_STAT)
         Lay_ShowErrorAndExit ("Type of users is missing.");
      Gbl.Stat.Role = (Sta_Role_t) UnsignedNum;
      /* The following types of query will never give a valid result */
      if ((Gbl.Stat.Role == Sta_ALL_USRS ||
           Gbl.Stat.Role == Sta_UNKNOWN_USRS) &&
          (Gbl.Stat.CountType == Sta_DISTINCT_USRS ||
           Gbl.Stat.CountType == Sta_CLICKS_PER_USR))
        {
         Lay_ShowAlert (Lay_WARNING,Txt_There_is_no_knowing_how_many_users_not_logged_have_accessed);
         Usr_UsrDataDestructor (&UsrDat);
         return false;
        }

      /***** Get users range for access statistics *****/
      Gbl.Scope.Allowed = 1 << Sco_SCOPE_PLATFORM    |
	                  1 << Sco_SCOPE_COUNTRY     |
		          1 << Sco_SCOPE_INSTITUTION |
	                  1 << Sco_SCOPE_CENTRE      |
                          1 << Sco_SCOPE_DEGREE      |
                          1 << Sco_SCOPE_COURSE;
      Gbl.Scope.Default = Sco_SCOPE_PLATFORM;
      Sco_GetScope ();
     }

   /***** Check if range of dates is forbidden for me *****/
   NumDays = Dat_GetNumDaysBetweenDates (&Gbl.DateRange.DateIni,&Gbl.DateRange.DateEnd);
   if (!(Gbl.Usrs.Me.LoggedRole == Rol_ROLE_SUPERUSER ||
         (Gbl.Usrs.Me.LoggedRole == Rol_ROLE_TEACHER && StatsGlobalOrCourse == STAT_COURSE)))
      if (NumDays > Cfg_DAYS_IN_RECENT_LOG)
        {
         sprintf (Gbl.Message,Txt_The_date_range_must_be_less_than_or_equal_to_X_days,
                  Cfg_DAYS_IN_RECENT_LOG);
         Lay_ShowAlert (Lay_WARNING,Gbl.Message);	// ...write warning message and show the form again
         return false;
        }

   /***** Query depending on the type of count *****/
   switch (Gbl.Stat.CountType)
     {
      case Sta_TOTAL_CLICKS:
         strcpy (StrQueryCountType,"COUNT(*)");
	 break;
      case Sta_DISTINCT_USRS:
         sprintf (StrQueryCountType,"COUNT(DISTINCT(%s.UsrCod))",LogTable);
	 break;
      case Sta_CLICKS_PER_USR:
         sprintf (StrQueryCountType,"COUNT(*)/GREATEST(COUNT(DISTINCT(%s.UsrCod)),1)+0.000000",LogTable);
	 break;
      case Sta_GENERATION_TIME:
         sprintf (StrQueryCountType,"(AVG(%s.TimeToGenerate)/1E6)+0.000000",LogTable);
	 break;
      case Sta_SEND_TIME:
         sprintf (StrQueryCountType,"(AVG(%s.TimeToSend)/1E6)+0.000000",LogTable);
	 break;
     }

   /***** Select clicks from the table of log *****/
   /* Start the query */
   switch (Gbl.Stat.ClicksStatType)
     {
      case Sta_ACC_CRS_LISTING:
   	 sprintf (Query,"SELECT SQL_NO_CACHE LogCod,UsrCod,Role,DATE_FORMAT(ClickTime,'%%Y%%m%%d%%H%%i%%S') AS F,ActCod FROM %s",
                  LogTable);
	 break;
      case Sta_ACC_CRS_PER_USR:
	 sprintf (Query,"SELECT SQL_NO_CACHE UsrCod,%s AS Num FROM %s",
                  StrQueryCountType,LogTable);
	 break;
      case Sta_ACC_CRS_PER_DAYS:
      case Sta_ACC_GBL_PER_DAYS:
         sprintf (Query,"SELECT SQL_NO_CACHE DATE_FORMAT(ClickTime,'%%Y%%m%%d') AS Day,%s FROM %s",
                  StrQueryCountType,LogTable);
	 break;
      case Sta_ACC_CRS_PER_DAYS_AND_HOUR:
      case Sta_ACC_GBL_PER_DAYS_AND_HOUR:
         sprintf (Query,"SELECT SQL_NO_CACHE DATE_FORMAT(ClickTime,'%%Y%%m%%d') AS Day,DATE_FORMAT(ClickTime,'%%H') AS Hour,%s FROM %s",
                  StrQueryCountType,LogTable);
	 break;
      case Sta_ACC_CRS_PER_WEEKS:
      case Sta_ACC_GBL_PER_WEEKS:
	 /* With %v the weeks always are counted from monday to sunday.
	    01/01/2006 was sunday => it's counted in the week 52 of 2005, that goes from 26/12/2005 (monday) to 01/01/2006 (sunday).
	    The week 1 of 2006 goes from 02/01/2006 (monday) to 08/01/2006 (sunday) */
         sprintf (Query,"SELECT SQL_NO_CACHE DATE_FORMAT(ClickTime,'%%x%%v') AS Week,%s FROM %s",
                  StrQueryCountType,LogTable);
	 break;
      case Sta_ACC_CRS_PER_MONTHS:
      case Sta_ACC_GBL_PER_MONTHS:
         sprintf (Query,"SELECT SQL_NO_CACHE DATE_FORMAT(ClickTime,'%%Y%%m') AS Month,%s FROM %s",
                  StrQueryCountType,LogTable);
	 break;
      case Sta_ACC_CRS_PER_HOUR:
      case Sta_ACC_GBL_PER_HOUR:
         sprintf (Query,"SELECT SQL_NO_CACHE DATE_FORMAT(ClickTime,'%%H') AS Hour,%s FROM %s",
                  StrQueryCountType,LogTable);
	 break;
      case Sta_ACC_CRS_PER_MINUTE:
      case Sta_ACC_GBL_PER_MINUTE:
         sprintf (Query,"SELECT SQL_NO_CACHE DATE_FORMAT(ClickTime,'%%H%%i') AS Minute,%s FROM %s",
                  StrQueryCountType,LogTable);
	 break;
      case Sta_ACC_CRS_PER_ACTION:
      case Sta_ACC_GBL_PER_ACTION:
         sprintf (Query,"SELECT SQL_NO_CACHE ActCod,%s AS Num FROM %s",
                  StrQueryCountType,LogTable);
	 break;
      case Sta_ACC_GBL_PER_PLUGIN:
         sprintf (Query,"SELECT SQL_NO_CACHE log_ws.PlgCod,%s AS Num FROM %s,log_ws",
                  StrQueryCountType,LogTable);
         break;
      case Sta_ACC_GBL_PER_Svc_FUNCTION:
         sprintf (Query,"SELECT SQL_NO_CACHE log_ws.FunCod,%s AS Num FROM %s,log_ws",
                  StrQueryCountType,LogTable);
         break;
      case Sta_ACC_GBL_PER_BANNER:
         sprintf (Query,"SELECT SQL_NO_CACHE log_banners.BanCod,%s AS Num FROM %s,log_banners",
                  StrQueryCountType,LogTable);
         break;
      case Sta_ACC_GBL_PER_DEGREE:
         sprintf (Query,"SELECT SQL_NO_CACHE DegCod,%s AS Num FROM %s",
                  StrQueryCountType,LogTable);
	 break;
      case Sta_ACC_GBL_PER_COURSE:
	 sprintf (Query,"SELECT SQL_NO_CACHE CrsCod,%s AS Num FROM %s",
                  StrQueryCountType,LogTable);
	 break;
     }
   sprintf (QueryAux," WHERE %s.ClickTime >= '%04u%02u%02u' AND %s.ClickTime <= '%04u%02u%02u235959'",
            LogTable,
            Gbl.DateRange.DateIni.Year,
            Gbl.DateRange.DateIni.Month,
            Gbl.DateRange.DateIni.Day,
            LogTable,
            Gbl.DateRange.DateEnd.Year,
            Gbl.DateRange.DateEnd.Month,
            Gbl.DateRange.DateEnd.Day);
   strcat (Query,QueryAux);
   switch (StatsGlobalOrCourse)
     {
      case STAT_GLOBAL:
	 /* Scope */
         if (Gbl.Scope.Current == Sco_SCOPE_INSTITUTION &&
             Gbl.CurrentIns.Ins.InsCod > 0)
           {
	    sprintf (QueryAux," AND %s.DegCod IN"
		              " (SELECT degrees.DegCod"
		              " FROM centres,degrees"
		              " WHERE centres.InsCod='%ld'"
		              " AND centres.CtrCod=degrees.CtrCod)",
                     LogTable,Gbl.CurrentIns.Ins.InsCod);
            strcat (Query,QueryAux);
           }
         else if (Gbl.Scope.Current == Sco_SCOPE_CENTRE &&
             Gbl.CurrentCtr.Ctr.CtrCod > 0)
           {
	    sprintf (QueryAux," AND %s.DegCod"
		              " IN (SELECT DegCod"
		              " FROM degrees"
		              " WHERE CtrCod='%ld')",
                     LogTable,Gbl.CurrentCtr.Ctr.CtrCod);
            strcat (Query,QueryAux);
           }
         else if (Gbl.Scope.Current == Sco_SCOPE_DEGREE &&
             Gbl.CurrentDeg.Deg.DegCod > 0)
           {
	    sprintf (QueryAux," AND %s.DegCod='%ld'",
                     LogTable,Gbl.CurrentDeg.Deg.DegCod);
            strcat (Query,QueryAux);
           }
         else if (Gbl.Scope.Current == Sco_SCOPE_COURSE &&
                  Gbl.CurrentCrs.Crs.CrsCod > 0)
           {
	    sprintf (QueryAux," AND %s.CrsCod='%ld'",
                     LogTable,Gbl.CurrentCrs.Crs.CrsCod);
            strcat (Query,QueryAux);
           }
         /* Type of users */
	 switch (Gbl.Stat.Role)
	   {
	    case Sta_IDENTIFIED_USRS:
               sprintf (StrRole," AND %s.Role<>'%u'",
                        LogTable,(unsigned) Rol_ROLE_UNKNOWN);
	       break;
	    case Sta_ALL_USRS:
               switch (Gbl.Stat.CountType)
                 {
                  case Sta_TOTAL_CLICKS:
                  case Sta_GENERATION_TIME:
                  case Sta_SEND_TIME:
                     StrRole[0] = '\0';
	             break;
                  case Sta_DISTINCT_USRS:
                  case Sta_CLICKS_PER_USR:
                     sprintf (StrRole," AND %s.Role<>'%u'",
                              LogTable,(unsigned) Rol_ROLE_UNKNOWN);
                     break;
                    }
	       break;
	    case Sta_INS_ADMINS:
               sprintf (StrRole," AND %s.Role='%u'",
                        LogTable,(unsigned) Rol_ROLE_INS_ADMIN);
	       break;
	    case Sta_CTR_ADMINS:
               sprintf (StrRole," AND %s.Role='%u'",
                        LogTable,(unsigned) Rol_ROLE_CTR_ADMIN);
	       break;
	    case Sta_DEG_ADMINS:
               sprintf (StrRole," AND %s.Role='%u'",
                        LogTable,(unsigned) Rol_ROLE_DEG_ADMIN);
	       break;
	    case Sta_TEACHERS:
               sprintf (StrRole," AND %s.Role='%u'",
                        LogTable,(unsigned) Rol_ROLE_TEACHER);
	       break;
	    case Sta_STUDENTS:
               sprintf (StrRole," AND %s.Role='%u'",
                        LogTable,(unsigned) Rol_ROLE_STUDENT);
	       break;
	    case Sta_VISITORS:
               sprintf (StrRole," AND %s.Role='%u'",
                        LogTable,(unsigned) Rol_ROLE_VISITOR);
               break;
	    case Sta_GUESTS:
               sprintf (StrRole," AND %s.Role='%u'",
                        LogTable,(unsigned) Rol_ROLE_GUEST);
               break;
	    case Sta_UNKNOWN_USRS:
               sprintf (StrRole," AND %s.Role='%u'",
                        LogTable,(unsigned) Rol_ROLE_UNKNOWN);
               break;
	    case Sta_ME:
               sprintf (StrRole," AND %s.UsrCod='%ld'",
                        LogTable,Gbl.Usrs.Me.UsrDat.UsrCod);
	       break;
	   }
         strcat (Query,StrRole);

         switch (Gbl.Stat.ClicksStatType)
           {
            case Sta_ACC_GBL_PER_PLUGIN:
            case Sta_ACC_GBL_PER_Svc_FUNCTION:
               sprintf (QueryAux," AND %s.LogCod=log_ws.LogCod",
                        LogTable);
               strcat (Query,QueryAux);
               break;
            case Sta_ACC_GBL_PER_BANNER:
               sprintf (QueryAux," AND %s.LogCod=log_banners.LogCod",
                        LogTable);
               strcat (Query,QueryAux);
               break;
            default:
               break;
           }
	 break;
      case STAT_COURSE:
         sprintf (QueryAux," AND %s.CrsCod='%ld'",
                  LogTable,Gbl.CurrentCrs.Crs.CrsCod);
	 strcat (Query,QueryAux);
	 LengthQuery = strlen (Query);
	 NumUsr = 0;
	 Ptr = Gbl.Usrs.Select.All;
	 while (*Ptr)
	   {
	    Par_GetNextStrUntilSeparParamMult (&Ptr,UsrDat.EncryptedUsrCod,Cry_LENGTH_ENCRYPTED_STR_SHA256_BASE64);
            Usr_GetUsrCodFromEncryptedUsrCod (&UsrDat);
	    if (UsrDat.UsrCod > 0)
	      {
	       LengthQuery = LengthQuery + 25 + 10 + 1;
	       if (LengthQuery > MAX_LENGTH_QUERY_ACCESS - 128)
                  Lay_ShowErrorAndExit ("Query is too large.");
               sprintf (QueryAux,
                        NumUsr ? " OR %s.UsrCod='%ld'" :
                                 " AND (%s.UsrCod='%ld'",
                        LogTable,UsrDat.UsrCod);
	       strcat (Query,QueryAux);
	       NumUsr++;
	      }
	   }
	 strcat (Query,")");
	 break;
     }

   /* Select action */
   if (Gbl.Stat.NumAction != ActAll)
     {
      sprintf (QueryAux," AND %s.ActCod='%ld'",
               LogTable,Act_Actions[Gbl.Stat.NumAction].ActCod);
      strcat (Query,QueryAux);
     }

   /* End the query */
   switch (Gbl.Stat.ClicksStatType)
     {
      case Sta_ACC_CRS_LISTING:
	 strcat (Query," ORDER BY F");
	 break;
      case Sta_ACC_CRS_PER_USR:
	 sprintf (QueryAux," GROUP BY %s.UsrCod ORDER BY Num DESC",LogTable);
         strcat (Query,QueryAux);
	 break;
     case Sta_ACC_CRS_PER_DAYS:
     case Sta_ACC_GBL_PER_DAYS:
	 strcat (Query," GROUP BY Day DESC");
	 break;
      case Sta_ACC_CRS_PER_DAYS_AND_HOUR:
      case Sta_ACC_GBL_PER_DAYS_AND_HOUR:
	 strcat (Query," GROUP BY Day DESC,Hour");
	 break;
     case Sta_ACC_CRS_PER_WEEKS:
     case Sta_ACC_GBL_PER_WEEKS:
	 strcat (Query," GROUP BY Week DESC");
	 break;
     case Sta_ACC_CRS_PER_MONTHS:
     case Sta_ACC_GBL_PER_MONTHS:
	 strcat (Query," GROUP BY Month DESC");
	 break;
      case Sta_ACC_CRS_PER_HOUR:
      case Sta_ACC_GBL_PER_HOUR:
	 strcat (Query," GROUP BY Hour");
	 break;
      case Sta_ACC_CRS_PER_MINUTE:
      case Sta_ACC_GBL_PER_MINUTE:
	 strcat (Query," GROUP BY Minute");
	 break;
      case Sta_ACC_CRS_PER_ACTION:
      case Sta_ACC_GBL_PER_ACTION:
	 sprintf (QueryAux," GROUP BY %s.ActCod ORDER BY Num DESC",LogTable);
         strcat (Query,QueryAux);
	 break;
      case Sta_ACC_GBL_PER_PLUGIN:
         strcat (Query," GROUP BY log_ws.PlgCod ORDER BY Num DESC");
         break;
      case Sta_ACC_GBL_PER_Svc_FUNCTION:
         strcat (Query," GROUP BY log_ws.FunCod ORDER BY Num DESC");
         break;
      case Sta_ACC_GBL_PER_BANNER:
         strcat (Query," GROUP BY log_banners.BanCod ORDER BY Num DESC");
         break;
      case Sta_ACC_GBL_PER_DEGREE:
	 sprintf (QueryAux," GROUP BY %s.DegCod ORDER BY Num DESC",LogTable);
         strcat (Query,QueryAux);
	 break;
      case Sta_ACC_GBL_PER_COURSE:
	 sprintf (QueryAux," GROUP BY %s.CrsCod ORDER BY Num DESC",LogTable);
         strcat (Query,QueryAux);
	 break;
     }
   /***** Write query for debug *****/
/*
   if (Gbl.Usrs.Me.LoggedRole == Rol_ROLE_SUPERUSER)
      Lay_ShowAlert (Lay_INFO,Query);
*/
   /***** Make the query *****/
   NumRows = DB_QuerySELECT (Query,&mysql_res,"can not get clicks");

   /***** Count the number of rows in result *****/
   if (NumRows == 0)
      Lay_ShowAlert (Lay_INFO,Txt_There_are_no_accesses_with_the_selected_search_criteria);
   else
     {
      if (Gbl.Stat.ClicksStatType == Sta_ACC_CRS_LISTING)
        {
         sprintf (Gbl.Message,Txt_List_of_detailed_clicks_in_the_course_X,
                  Gbl.CurrentCrs.Crs.FullName);
         Lay_WriteTitle (Gbl.Message);
         Sta_WriteSelectedRangeOfDates (NumDays);
        }
      else
        {
         switch (Gbl.Stat.ClicksStatType)
           {
            case Sta_ACC_CRS_PER_USR:
            case Sta_ACC_CRS_PER_DAYS:
            case Sta_ACC_CRS_PER_DAYS_AND_HOUR:
            case Sta_ACC_CRS_PER_WEEKS:
            case Sta_ACC_CRS_PER_MONTHS:
            case Sta_ACC_CRS_PER_HOUR:
            case Sta_ACC_CRS_PER_MINUTE:
            case Sta_ACC_CRS_PER_ACTION:
               sprintf (Gbl.Message,Txt_Statistics_of_visits_to_the_course_X,
                        Gbl.CurrentCrs.Crs.FullName);
               Lay_WriteTitle (Gbl.Message);
               Sta_WriteSelectedRangeOfDates (NumDays);
	       break;
            default:
               switch (Gbl.Scope.Current)
                 {
                  case Sco_SCOPE_PLATFORM:
                     strcpy (Gbl.Message,Txt_Statistics_of_all_visits);
                     break;
                  case Sco_SCOPE_INSTITUTION:
                     sprintf (Gbl.Message,Txt_Statistics_of_visits_to_the_institution_X,
                              Gbl.CurrentIns.Ins.ShortName);
                     break;
                  case Sco_SCOPE_CENTRE:
                     sprintf (Gbl.Message,Txt_Statistics_of_visits_to_the_centre_X,
                              Gbl.CurrentCtr.Ctr.ShortName);
                     break;
                  case Sco_SCOPE_DEGREE:
                     sprintf (Gbl.Message,Txt_Statistics_of_visits_to_the_degree_X,
                              Gbl.CurrentDeg.Deg.ShortName);
                     break;
                  case Sco_SCOPE_COURSE:
                     sprintf (Gbl.Message,Txt_Statistics_of_visits_to_the_course_X,
                              Gbl.CurrentCrs.Crs.ShortName);
                     break;
		  default:
		     Lay_ShowErrorAndExit ("Wrong scope.");
		     break;
                 }
               Lay_WriteTitle (Gbl.Message);
               Sta_WriteSelectedRangeOfDates (NumDays);
               fprintf (Gbl.F.Out,"<p class=\"DAT\" style=\"text-align:center;\">");
               if (Gbl.Stat.Role == Sta_ME)
                  fprintf (Gbl.F.Out,"%s: %s",
                           Txt_User,
                           Gbl.Usrs.Me.UsrDat.FullName);
               else
                  fprintf (Gbl.F.Out,"%s: %s",
                           Txt_Users,
                           Txt_ROLE_STATS[Gbl.Stat.Role]);
               fprintf (Gbl.F.Out,"</p>");
               break;
           }

         fprintf (Gbl.F.Out,"<p class=\"DAT\"style=\"text-align:center;\">%s: %s</p>",
                  Txt_Action,
                  Act_GetActionTextFromDB (Act_Actions[Gbl.Stat.NumAction].ActCod,ActTxt));

         sprintf (Gbl.Message,"%s %s, %s %s",
                  Txt_The_graph_shows_the_NUMBER,
                  Txt_STAT_TYPE_COUNT_SMALL[Gbl.Stat.CountType],
                  Txt_distributed_by,
                  Txt_STAT_CLICK_STAT_TYPES[Gbl.Stat.ClicksStatType]);
         Lay_WriteTitle (Gbl.Message);
  	}

      /***** Put the table with the clicks *****/
      /* Write start of table frame */
      Lay_StartRoundFrameTable10 (Gbl.Stat.ClicksStatType == Sta_ACC_CRS_LISTING ? "100%" :
	                                                                           NULL,
	                          0,NULL);
      switch (Gbl.Stat.ClicksStatType)
	{
	 case Sta_ACC_CRS_LISTING:
	    Sta_ShowDetailedAccessesList (NumRows,mysql_res);
	    break;
	 case Sta_ACC_CRS_PER_USR:
	    Sta_ShowNumAccessesPerUsr (NumRows,mysql_res);
	    break;
	 case Sta_ACC_CRS_PER_DAYS:
	 case Sta_ACC_GBL_PER_DAYS:
	    Sta_ShowNumAccessesPerDays (NumRows,mysql_res);
	    break;
	 case Sta_ACC_CRS_PER_DAYS_AND_HOUR:
	 case Sta_ACC_GBL_PER_DAYS_AND_HOUR:
	    Sta_ShowDistrAccessesPerDaysAndHour (NumRows,mysql_res);
	    break;
	 case Sta_ACC_CRS_PER_WEEKS:
	 case Sta_ACC_GBL_PER_WEEKS:
	    Sta_ShowNumAccessesPerWeeks (NumRows,mysql_res);
	    break;
	 case Sta_ACC_CRS_PER_MONTHS:
	 case Sta_ACC_GBL_PER_MONTHS:
	    Sta_ShowNumAccessesPerMonths (NumRows,mysql_res);
	    break;
	 case Sta_ACC_CRS_PER_HOUR:
	 case Sta_ACC_GBL_PER_HOUR:
	    Sta_ShowNumAccessesPerHour (NumRows,mysql_res);
	    break;
	 case Sta_ACC_CRS_PER_MINUTE:
	 case Sta_ACC_GBL_PER_MINUTE:
	    Sta_ShowAverageAccessesPerMinute (NumRows,mysql_res);
	    break;
	 case Sta_ACC_CRS_PER_ACTION:
	 case Sta_ACC_GBL_PER_ACTION:
	    Sta_ShowNumAccessesPerAction (NumRows,mysql_res);
	    break;
         case Sta_ACC_GBL_PER_PLUGIN:
            Sta_ShowNumAccessesPerPlugin (NumRows,mysql_res);
            break;
         case Sta_ACC_GBL_PER_Svc_FUNCTION:
            Sta_ShowNumAccessesPerWSFunction (NumRows,mysql_res);
            break;
         case Sta_ACC_GBL_PER_BANNER:
            Sta_ShowNumAccessesPerBanner (NumRows,mysql_res);
            break;
         case Sta_ACC_GBL_PER_DEGREE:
	    Sta_ShowNumAccessesPerDegree (NumRows,mysql_res);
	    break;
	 case Sta_ACC_GBL_PER_COURSE:
	    Sta_ShowNumAccessesPerCourse (NumRows,mysql_res);
	    break;
	}

      /* End of frame */
      Lay_EndRoundFrameTable10 ();
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   /***** Free the memory used by the list of users *****/
   if (Gbl.CurrentAct == ActSeeAccCrs)
      Usr_FreeListsEncryptedUsrCods ();

   /***** Free memory used by the data of the user *****/
   Usr_UsrDataDestructor (&UsrDat);

   return true;	// No error
  }

/*****************************************************************************/
/******************* Show a listing of detailed clicks ***********************/
/*****************************************************************************/

static void Sta_ShowDetailedAccessesList (unsigned long NumRows,MYSQL_RES *mysql_res)
  {
   extern const char *Txt_Show_previous_X_clicks;
   extern const char *Txt_PAGES_Previous;
   extern const char *Txt_Clicks;
   extern const char *Txt_of_PART_OF_A_TOTAL;
   extern const char *Txt_page;
   extern const char *Txt_Show_next_X_clicks;
   extern const char *Txt_PAGES_Next;
   extern const char *Txt_No_INDEX;
   extern const char *Txt_User_ID;
   extern const char *Txt_Name;
   extern const char *Txt_Type;
   extern const char *Txt_Date;
   extern const char *Txt_Action;
   extern const char *Txt_LOG_More_info;
   extern const char *Txt_ROLES_SINGULAR_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   unsigned long NumRow;
   unsigned long FirstRow;	// First row to show
   unsigned long LastRow;	// Last rows to show
   unsigned long NumPagesBefore;
   unsigned long NumPagesAfter;
   unsigned long NumPagsTotal;
   struct UsrData UsrDat;
   MYSQL_ROW row;
   long LogCod;
   Rol_Role_t RoleFromLog;
   long ActCod;
   char ActTxt[Act_MAX_LENGTH_ACTION_TXT+1];

   /***** Initialize estructura of data of the user *****/
   Usr_UsrDataConstructor (&UsrDat);

   /***** Compute the first and the last row to show *****/
   FirstRow = Gbl.Stat.FirstRow;
   LastRow  = Gbl.Stat.LastRow;
   if (FirstRow == 0 && LastRow == 0) // Call from main form
     {
      // Show last clicks
      FirstRow = (NumRows / Gbl.Stat.RowsPerPage - 1) * Gbl.Stat.RowsPerPage + 1;
      if ((FirstRow + Gbl.Stat.RowsPerPage - 1) < NumRows)
	 FirstRow += Gbl.Stat.RowsPerPage;
      LastRow = NumRows;
     }
   if (FirstRow < 1) // For security reasons; really it should never be less than 1
      FirstRow = 1;
   if (LastRow > NumRows)
      LastRow = NumRows;
   if ((LastRow - FirstRow) >= Gbl.Stat.RowsPerPage) // For if there have been clicks that have increased the number of rows
      LastRow = FirstRow + Gbl.Stat.RowsPerPage - 1;

   /***** Compute the number total of pages *****/
   /* Number of pages before the current one */
   NumPagesBefore = (FirstRow-1) / Gbl.Stat.RowsPerPage;
   if (NumPagesBefore * Gbl.Stat.RowsPerPage < (FirstRow-1))
      NumPagesBefore++;
   /* Number of pages after the current one */
   NumPagesAfter = (NumRows - LastRow) / Gbl.Stat.RowsPerPage;
   if (NumPagesAfter * Gbl.Stat.RowsPerPage < (NumRows - LastRow))
      NumPagesAfter++;
   /* Count the total number of pages */
   NumPagsTotal = NumPagesBefore + 1 + NumPagesAfter;

   /***** Put heading with backward and forward buttons *****/
   fprintf (Gbl.F.Out,"<tr>"
	              "<td colspan=\"7\" style=\"text-align:left;\">"
                      "<table class=\"CELLS_PAD_2\" style=\"width:100%%;\">"
                      "<tr>");

   /* Put link to jump to previous page (older clicks) */
   if (FirstRow > 1)
     {
      Act_FormStart (ActSeeAccCrs);
      Sta_WriteParamsDatesSeeAccesses ();
      Par_PutHiddenParamUnsigned ("ClickStatType",(unsigned) Sta_ACC_CRS_LISTING);
      Par_PutHiddenParamUnsigned ("StatAct",(unsigned) Gbl.Stat.NumAction);
      Par_PutHiddenParamLong ("FirstRow",FirstRow-Gbl.Stat.RowsPerPage);
      Par_PutHiddenParamLong ("LastRow",FirstRow-1);
      Par_PutHiddenParamLong ("RowsPage",Gbl.Stat.RowsPerPage);
      Usr_PutHiddenParUsrCodAll (ActSeeAccCrs,Gbl.Usrs.Select.All);
     }
   fprintf (Gbl.F.Out,"<td style=\"width:20%%; text-align:left;\">");
   if (FirstRow > 1)
     {
      sprintf (Gbl.Title,Txt_Show_previous_X_clicks,
               Gbl.Stat.RowsPerPage);
      Act_LinkFormSubmit (Gbl.Title,"TIT_TBL");
      fprintf (Gbl.F.Out,"<strong>&lt;%s</strong></a>",
               Txt_PAGES_Previous);
     }
   fprintf (Gbl.F.Out,"</td>");
   if (FirstRow > 1)
      fprintf (Gbl.F.Out,"</form>");

   /* Write number of current page */
   fprintf (Gbl.F.Out,"<td class=\"TIT_TBL\" style=\"width:60%%; text-align:center;\">"
                      "<strong>"
                      "%s %lu-%lu %s %lu (%s %ld %s %lu)"
                      "</strong>"
                      "</td>",
            Txt_Clicks,
            FirstRow,LastRow,Txt_of_PART_OF_A_TOTAL,NumRows,
            Txt_page,NumPagesBefore+1,Txt_of_PART_OF_A_TOTAL,NumPagsTotal);

   /* Put link to jump to next page (more recent clicks) */
   if (LastRow < NumRows)
     {
      Act_FormStart (ActSeeAccCrs);
      Sta_WriteParamsDatesSeeAccesses ();
      Par_PutHiddenParamUnsigned ("ClickStatType",(unsigned) Sta_ACC_CRS_LISTING);
      Par_PutHiddenParamUnsigned ("StatAct",(unsigned) Gbl.Stat.NumAction);
      Par_PutHiddenParamUnsigned ("FirstRow",(unsigned) (LastRow+1));
      Par_PutHiddenParamUnsigned ("LastRow",(unsigned) (LastRow+Gbl.Stat.RowsPerPage));
      Par_PutHiddenParamUnsigned ("RowsPage",(unsigned) Gbl.Stat.RowsPerPage);
      Usr_PutHiddenParUsrCodAll (ActSeeAccCrs,Gbl.Usrs.Select.All);
     }
   fprintf (Gbl.F.Out,"<td style=\"width:20%%; text-align:right;\">");
   if (LastRow < NumRows)
     {
      sprintf (Gbl.Title,Txt_Show_next_X_clicks,
               Gbl.Stat.RowsPerPage);
      Act_LinkFormSubmit (Gbl.Title,"TIT_TBL");
      fprintf (Gbl.F.Out,"<strong>%s&gt;</strong></a>",
               Txt_PAGES_Next);
     }
   fprintf (Gbl.F.Out,"</td>");
   if (LastRow < NumRows)
      fprintf (Gbl.F.Out,"</form>");

   fprintf (Gbl.F.Out,"</tr>"
	              "</table>"
	              "</td>"
	              "</tr>");

   /***** Write heading *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<th class=\"TIT_TBL\""
                      " style=\"text-align:right; vertical-align:top;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\""
                      " style=\"text-align:center; vertical-align:top;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\""
                      " style=\"text-align:left; vertical-align:top;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\""
                      " style=\"text-align:center; vertical-align:top;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\""
                      " style=\"text-align:center; vertical-align:top;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\""
                      " style=\"text-align:left; vertical-align:top;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"width:10%%;"
                      " text-align:left; vertical-align:top;\">"
                      "%s"
                      "</th>"
                      "</tr>",
            Txt_No_INDEX,
            Txt_User_ID,
            Txt_Name,
            Txt_Type,
            Txt_Date,
            Txt_Action,
            Txt_LOG_More_info);

   /***** Write rows back *****/
   for (NumRow = LastRow, Gbl.RowEvenOdd = 0;
	NumRow >= FirstRow;
	NumRow--, Gbl.RowEvenOdd = 1 - Gbl.RowEvenOdd)
     {
      mysql_data_seek (mysql_res, (my_ulonglong) (NumRow-1));
      row = mysql_fetch_row (mysql_res);

      /* Get log code */
      LogCod = Str_ConvertStrCodToLongCod (row[0]);

      /* Get user's data of the database */
      UsrDat.UsrCod = Str_ConvertStrCodToLongCod (row[1]);
      Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&UsrDat);

      /* Get logged role */
      if (sscanf (row[2],"%u",&RoleFromLog) != 1)
	 Lay_ShowErrorAndExit ("Wrong user's role.");

      /* Write the number of row */
      fprintf (Gbl.F.Out,"<tr>"
	                 "<td class=\"LOG\" style=\"text-align:right;"
	                 " vertical-align:top; background-color:%s;\">"
	                 "%ld&nbsp;"
	                 "</td>",
	       Gbl.ColorRows[Gbl.RowEvenOdd],NumRow);

      /* Write the user's ID if user is a student */
      fprintf (Gbl.F.Out,"<td class=\"LOG\" style=\"text-align:center;"
	                 " vertical-align:top; background-color:%s;\">",
	       Gbl.ColorRows[Gbl.RowEvenOdd]);
      ID_WriteUsrIDs (&UsrDat,(RoleFromLog == Rol_ROLE_STUDENT));
      fprintf (Gbl.F.Out,"&nbsp;</td>");

      /* Write the first name and the surnames */
      fprintf (Gbl.F.Out,"<td class=\"LOG\" style=\"text-align:left;"
	                 " vertical-align:top; background-color:%s;\">"
	                 "%s&nbsp;"
	                 "</td>",
	       Gbl.ColorRows[Gbl.RowEvenOdd],UsrDat.FullName);

      /* Write the user's role */
      fprintf (Gbl.F.Out,"<td class=\"LOG\" style=\"text-align:center;"
	                 " vertical-align:top; background-color:%s;\">"
	                 "%s&nbsp;"
	                 "</td>",
	       Gbl.ColorRows[Gbl.RowEvenOdd],
	       RoleFromLog < Rol_NUM_ROLES ? Txt_ROLES_SINGULAR_Abc[RoleFromLog][UsrDat.Sex] :
		                             "?");

      /* Write the date (in row[3] is the date in YYYYMMDDHHMMSS format) */
      fprintf (Gbl.F.Out,"<td class=\"LOG\" style=\"text-align:center;"
	                 " vertical-align:top; background-color:%s;\">",
	       Gbl.ColorRows[Gbl.RowEvenOdd]);
      Dat_WriteDate (row[3]);
      fprintf (Gbl.F.Out,"&nbsp;");
      Dat_WriteHourMinute (&row[3][8]);
      fprintf (Gbl.F.Out,":%c%c&nbsp;</td>",
               row[3][12],row[3][13]);

      /* Write the action */
      if (sscanf (row[4],"%ld",&ActCod) != 1)
	 Lay_ShowErrorAndExit ("Wrong action code.");
      if (ActCod >= 0)
         fprintf (Gbl.F.Out,"<td class=\"LOG\" style=\"text-align:left;"
	                    " vertical-align:top; background-color:%s;\">"
                            "%s&nbsp;"
                            "</td>",
	          Gbl.ColorRows[Gbl.RowEvenOdd],
	          Act_GetActionTextFromDB (ActCod,ActTxt));
      else
         fprintf (Gbl.F.Out,"<td class=\"LOG\" style=\"text-align:left;"
	                    " vertical-align:top; background-color:%s;\">"
                            "?&nbsp;"
                            "</td>",
	          Gbl.ColorRows[Gbl.RowEvenOdd]);
      /* Write the comments of the access */
      fprintf (Gbl.F.Out,"<td class=\"LOG\" style=\"text-align:left;"
	                 " vertical-align:top; background-color:%s;\">",
               Gbl.ColorRows[Gbl.RowEvenOdd]);
      Sta_WriteLogComments (LogCod);
      fprintf (Gbl.F.Out,"</td>"
	                 "</tr>");
     }

   /***** Free memory used by the data of the user *****/
   Usr_UsrDataDestructor (&UsrDat);
  }

/*****************************************************************************/
/******** Show a listing of with the number of clicks of each user ***********/
/*****************************************************************************/

static void Sta_WriteLogComments (long LogCod)
  {
   char Query[512];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;

   /***** Get log comments from database *****/
   sprintf (Query,"SELECT Comments FROM log_comments WHERE LogCod='%ld'",
            LogCod);
   if (DB_QuerySELECT (Query,&mysql_res,"can not get log comments"))
     {
      /***** Get and write comments *****/
      row = mysql_fetch_row (mysql_res);
      fprintf (Gbl.F.Out,"%s",row[0]);
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/********* Show a listing of with the number of clicks of each user **********/
/*****************************************************************************/

static void Sta_ShowNumAccessesPerUsr (unsigned long NumRows,MYSQL_RES *mysql_res)
  {
   extern const char *Txt_No_INDEX;
   extern const char *Txt_Photo;
   extern const char *Txt_ID;
   extern const char *Txt_Name;
   extern const char *Txt_Type;
   extern const char *Txt_STAT_TYPE_COUNT_CAPS[Sta_NUM_STAT_COUNT_TYPES];
   extern const char *Txt_ROLES_SINGULAR_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   MYSQL_ROW row;
   unsigned long NumRow;
   float NumPagesGenerated,MaxPagesGenerated = 0.0;
   unsigned BarWidth;
   struct UsrData UsrDat;
   char PhotoURL[PATH_MAX+1];
   bool ShowPhoto;

   /***** Initialize user's data *****/
   Usr_UsrDataConstructor (&UsrDat);

   /***** Write heading *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:right; vertical-align:top;\">"
                      "%s"
                      "</td>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:center; vertical-align:top;\">"
                      "%s"
                      "</td>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:center; vertical-align:top;\">"
                      "%s"
                      "</td>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:left; vertical-align:top;\">"
                      "%s"
                      "</td>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:center; vertical-align:top;\">"
                      "%s"
                      "</td>"
                      "<td colspan=\"2\" class=\"TIT_TBL\""
                      " style=\"text-align:left; vertical-align:top;\">"
                      "%s"
                      "</td>"
                      "</tr>",
            Txt_No_INDEX,
            Txt_Photo,
            Txt_ID,
            Txt_Name,
            Txt_Type,
            Txt_STAT_TYPE_COUNT_CAPS[Gbl.Stat.CountType]);

   /***** Write rows *****/
   for (NumRow = 1, Gbl.RowEvenOdd = 0;
	NumRow <= NumRows;
	NumRow++, Gbl.RowEvenOdd = 1 - Gbl.RowEvenOdd)
     {
      row = mysql_fetch_row (mysql_res);

      /* Get user's data from the database */
      UsrDat.UsrCod = Str_ConvertStrCodToLongCod (row[0]);
      Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&UsrDat);	// Get the data of the user from the database

      /* Write the number of row */
      fprintf (Gbl.F.Out,"<tr>"
	                 "<td class=\"LOG\" style=\"text-align:right;"
	                 " vertical-align:top; background-color:%s;\">"
	                 "%ld&nbsp;"
	                 "</td>",
	       Gbl.ColorRows[Gbl.RowEvenOdd],NumRow);

      /* Show the photo */
      fprintf (Gbl.F.Out,"<td style=\"text-align:center;"
	                 " vertical-align:top; background-color:%s;\">",
	       Gbl.ColorRows[Gbl.RowEvenOdd]);
      ShowPhoto = Pho_ShowUsrPhotoIsAllowed (&UsrDat,PhotoURL);
      Pho_ShowUsrPhoto (&UsrDat,ShowPhoto ? PhotoURL :
                                            NULL,
                        "PHOTO12x16",true);
      fprintf (Gbl.F.Out,"</td>");

      /* Write the user's ID if user is a student in current course */
      fprintf (Gbl.F.Out,"<td class=\"LOG\" style=\"text-align:center;"
	                 " vertical-align:top; background-color:%s;\">",
	       Gbl.ColorRows[Gbl.RowEvenOdd]);
      ID_WriteUsrIDs (&UsrDat,(UsrDat.RoleInCurrentCrsDB == Rol_ROLE_STUDENT));
      fprintf (Gbl.F.Out,"&nbsp;</td>");

      /* Write the name and the surnames */
      fprintf (Gbl.F.Out,"<td class=\"LOG\" style=\"text-align:left;"
	                 " vertical-align:top; background-color:%s;\">"
	                 "%s&nbsp;"
	                 "</td>",
	       Gbl.ColorRows[Gbl.RowEvenOdd],UsrDat.FullName);

      /* Write user's role */
      fprintf (Gbl.F.Out,"<td class=\"LOG\" style=\"text-align:center;"
	                 " vertical-align:top; background-color:%s;\">"
	                 "%s&nbsp;"
	                 "</td>",
	       Gbl.ColorRows[Gbl.RowEvenOdd],
	       Txt_ROLES_SINGULAR_Abc[UsrDat.RoleInCurrentCrsDB][UsrDat.Sex]);

      /* Write the number of clicks */
      NumPagesGenerated = Str_GetFloatNumFromStr (row[1]);
      if (NumRow == 1)
	 MaxPagesGenerated = NumPagesGenerated;
      if (MaxPagesGenerated > 0.0)
        {
         BarWidth = (unsigned) (((NumPagesGenerated * 300.0) / MaxPagesGenerated) + 0.5);
         if (BarWidth == 0)
            BarWidth = 1;
        }
      else
         BarWidth = 0;
      fprintf (Gbl.F.Out,"<td class=\"LOG\" style=\"text-align:left;"
	                 " vertical-align:top; background-color:%s;\">",
	       Gbl.ColorRows[Gbl.RowEvenOdd]);
      if (BarWidth)
	 fprintf (Gbl.F.Out,"<img src=\"%s/%c1x14.gif\" alt=\"\""
	                    " style=\"width:%upx; height:14px;"
	                    " vertical-align:top;\" />"
	                    "&nbsp;",
		  Gbl.Prefs.IconsURL,
		  UsrDat.RoleInCurrentCrsDB == Rol_ROLE_STUDENT ? 'c' :
			                                          'v',
		  BarWidth);
      Sta_WriteFloatNum (NumPagesGenerated);
      fprintf (Gbl.F.Out,"&nbsp;</td>"
	                 "</tr>");
     }

   /***** Free memory used by the data of the user *****/
   Usr_UsrDataDestructor (&UsrDat);
  }

/*****************************************************************************/
/********** Show a listing of with the number of clicks in each date *********/
/*****************************************************************************/

static void Sta_ShowNumAccessesPerDays (unsigned long NumRows,MYSQL_RES *mysql_res)
  {
   extern const char *Txt_Date;
   extern const char *Txt_Day;
   extern const char *Txt_STAT_TYPE_COUNT_CAPS[Sta_NUM_STAT_COUNT_TYPES];
   extern const char *Txt_DAYS_SMALL[7];
   unsigned long NumRow;
   struct Date ReadDate,LastDate,Date;
   unsigned D,NumDaysFromLastDateToCurrDate,NumDayWeek;
   float NumPagesGenerated,MaxPagesGenerated = 0.0,TotalPagesGenerated = 0.0;
   MYSQL_ROW row;

   /***** Initialize LastDate *****/
   Dat_AssignDate (&LastDate,&(Gbl.DateRange.DateEnd));

   /***** Write heading *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:center; vertical-align:top;\">"
                      "%s&nbsp;"
                      "</td>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:center; vertical-align:top;\">"
                      "%s&nbsp;"
                      "</td>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:left; vertical-align:top;\">"
                      "%s"
                      "</td>"
                      "</tr>",
            Txt_Date,
            Txt_Day,
            Txt_STAT_TYPE_COUNT_CAPS[Gbl.Stat.CountType]);

   /***** Compute maximum number of pages generated per day *****/
   for (NumRow = 1;
	NumRow <= NumRows;
	NumRow++)
     {
      row = mysql_fetch_row (mysql_res);

      /* Get number of pages generated */
      NumPagesGenerated = Str_GetFloatNumFromStr (row[1]);
      if (NumPagesGenerated > MaxPagesGenerated)
	 MaxPagesGenerated = NumPagesGenerated;
      TotalPagesGenerated += NumPagesGenerated;
     }

   /***** Write rows beginning by the most recent day and ending by the oldest *****/
   mysql_data_seek (mysql_res,0);
   for (NumRow = 1;
	NumRow <= NumRows;
	NumRow++)
     {
      row = mysql_fetch_row (mysql_res);

      /* Get year, month and day (row[0] holds the date in YYYYMMDD format) */
      if (!(Dat_GetDateFromYYYYMMDD (&ReadDate,row[0])))
	 Lay_ShowErrorAndExit ("Wrong date.");

      /* Get number of pages generated (in row[1]) */
      NumPagesGenerated = Str_GetFloatNumFromStr (row[1]);

      Dat_AssignDate (&Date,&LastDate);
      NumDaysFromLastDateToCurrDate = Dat_GetNumDaysBetweenDates (&ReadDate,&LastDate);
      /* In the next loop (NumDaysFromLastDateToCurrDate-1) d�as (the more recent) with 0 clicks are shown
         and a last day (the oldest) with NumPagesGenerated */
      for (D = 1;
	   D <= NumDaysFromLastDateToCurrDate;
	   D++)
        {
         NumDayWeek = Dat_GetDayOfWeek (Date.Year,Date.Month,Date.Day);

         /* Write the date */
         fprintf (Gbl.F.Out,"<tr>"
                            "<td class=\"%s\""
                            " style=\"text-align:right; vertical-align:top;\">"
                            "%02u/%02u/%02u&nbsp;"
                            "</td>",
	          NumDayWeek == 6 ? "LOG_R" :
	        	            "LOG",
	          Date.Day,Date.Month,Date.Year % 100);

         /* Write the day of the week */
         fprintf (Gbl.F.Out,"<td class=\"%s\""
               " style=\"text-align:left; vertical-align:top;\">"
                            "%s&nbsp;"
                            "</td>",
                  NumDayWeek == 6 ? "LOG_R" :
                	            "LOG",
                  Txt_DAYS_SMALL[NumDayWeek]);
         /* Draw bar proportional to number of pages generated */
         Sta_DrawBarNumClicks (NumDayWeek == 6 ? 'r' :
                                                 'c',
                               D == NumDaysFromLastDateToCurrDate ? NumPagesGenerated :
                        	                                    0.0,
                               MaxPagesGenerated,TotalPagesGenerated,400);

         /* Decrease day */
         Dat_GetDateBefore (&Date,&Date);
        }
      Dat_AssignDate (&LastDate,&Date);
     }
   NumDaysFromLastDateToCurrDate = Dat_GetNumDaysBetweenDates (&Gbl.DateRange.DateIni,&LastDate);

   /***** Finally NumDaysFromLastDateToCurrDate days are shown with 0 clicks
          (the oldest days from the requested initial day until the first with clicks) *****/
   for (D = 1;
	D <= NumDaysFromLastDateToCurrDate;
	D++)
     {
      NumDayWeek = Dat_GetDayOfWeek (Date.Year,Date.Month,Date.Day);

      /* Write the date */
      fprintf (Gbl.F.Out,"<tr>"
	                 "<td class=\"%s\""
	                 " style=\"text-align:right; vertical-align:top;\">"
	                 "%02u/%02u/%02u&nbsp;"
	                 "</td>",
               NumDayWeek == 6 ? "LOG_R" :
        	                 "LOG",
               Date.Day,Date.Month,Date.Year % 100);

      /* Write the day of the week */
      fprintf (Gbl.F.Out,"<td class=\"%s\""
	                 " style=\"text-align:left; vertical-align:top;\">"
	                 "%s&nbsp;"
	                 "</td>",
               NumDayWeek == 6 ? "LOG_R" :
        	                 "LOG",
               Txt_DAYS_SMALL[NumDayWeek]);

      /* Draw bar proportional to number of pages generated */
      Sta_DrawBarNumClicks (NumDayWeek == 6 ? 'r' :
	                                      'c',
	                    0.0,MaxPagesGenerated,TotalPagesGenerated,400);

      /* Decrease day */
      Dat_GetDateBefore (&Date,&Date);
     }
  }

/*****************************************************************************/
/************ Show graphic of number of pages generated per hour *************/
/*****************************************************************************/

#define GRAPH_DISTRIBUTION_PER_HOUR_HOUR_WIDTH 20
#define GRAPH_DISTRIBUTION_PER_HOUR_TOTAL_WIDTH (GRAPH_DISTRIBUTION_PER_HOUR_HOUR_WIDTH*24)

static void Sta_ShowDistrAccessesPerDaysAndHour (unsigned long NumRows,MYSQL_RES *mysql_res)
  {
   extern const char *The_ClassFormul[The_NUM_THEMES];
   extern const char *Txt_Color_of_the_graphic;
   extern const char *Txt_STAT_COLOR_TYPES[Sta_NUM_COLOR_TYPES];
   extern const char *Txt_Date;
   extern const char *Txt_Day;
   extern const char *Txt_STAT_TYPE_COUNT_CAPS[Sta_NUM_STAT_COUNT_TYPES];
   extern const char *Txt_DAYS_SMALL[7];
   Sta_ColorType_t ColorType,SelectedColorType;
   unsigned long NumRow;
   struct Date PreviousReadDate,CurrentReadDate,LastDate,Date;
   unsigned D,NumDaysFromLastDateToCurrDate=1,NumDayWeek;
   unsigned Hour,ReadHour=0;
   float NumPagesGenerated,MaxPagesGenerated = 0.0;
   float NumAccPerHour[24],NumAccPerHourZero[24];
   MYSQL_ROW row;

   /***** Get selected color type *****/
   SelectedColorType = Sta_GetStatColorType ();

   /***** Put a selector for the type of color *****/
   fprintf (Gbl.F.Out,"<tr>"
	              "<td colspan=\"26\" class=\"%s\""
	              " style=\"text-align:center;\">",
            The_ClassFormul[Gbl.Prefs.Theme]);

   Act_FormStart (Gbl.CurrentAct);
   Sta_WriteParamsDatesSeeAccesses ();
   Par_PutHiddenParamUnsigned ("ClickStatType",(unsigned) Gbl.Stat.ClicksStatType);
   Par_PutHiddenParamUnsigned ("CountType",(unsigned) Gbl.Stat.CountType);
   Par_PutHiddenParamUnsigned ("StatAct",(unsigned) Gbl.Stat.NumAction);
   if (Gbl.CurrentAct == ActSeeAccCrs)
      Usr_PutHiddenParUsrCodAll (ActSeeAccCrs,Gbl.Usrs.Select.All);
   else // Gbl.CurrentAct == ActSeeAccGbl
     {
      Par_PutHiddenParamUnsigned ("Role",(unsigned) Gbl.Stat.Role);
      Sco_PutParamScope (Gbl.Scope.Current);
     }

   fprintf (Gbl.F.Out,"%s: ",Txt_Color_of_the_graphic);
   fprintf (Gbl.F.Out,"<select name=\"ColorType\""
                      " onchange=\"javascript:document.getElementById('%s').submit();\">",
            Gbl.FormId);
   for (ColorType = (Sta_ColorType_t) 0;
	ColorType < Sta_NUM_COLOR_TYPES;
	ColorType++)
     {
      fprintf (Gbl.F.Out,"<option value=\"%u\"",(unsigned) ColorType);
      if (ColorType == SelectedColorType)
         fprintf (Gbl.F.Out," selected=\"selected\"");
      fprintf (Gbl.F.Out,">%s",Txt_STAT_COLOR_TYPES[ColorType]);
     }
   fprintf (Gbl.F.Out,"</select>"
	              "</form>"
	              "</td>"
	              "</tr>");

   /***** Compute maximum number of pages generated per day-hour *****/
   for (NumRow = 1;
	NumRow <= NumRows;
	NumRow++)
     {
      row = mysql_fetch_row (mysql_res);

      /* Get number of pages generated */
      NumPagesGenerated = Str_GetFloatNumFromStr (row[2]);
      if (NumPagesGenerated > MaxPagesGenerated)
	 MaxPagesGenerated = NumPagesGenerated;
     }

   /***** Initialize LastDate *****/
   Dat_AssignDate (&LastDate,&(Gbl.DateRange.DateEnd));

   /***** Reset number of pages generated per hour *****/
   for (Hour = 0;
	Hour < 24;
	Hour++)
      NumAccPerHour[Hour] = NumAccPerHourZero[Hour] = 0.0;

   /***** Write heading *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td rowspan=\"3\" class=\"TIT_TBL\""
                      " style=\"text-align:center; vertical-align:top;\">"
                      "%s&nbsp;"
                      "</td>"
                      "<td rowspan=\"3\" class=\"TIT_TBL\""
                      " style=\"text-align:center; vertical-align:top;\">"
                      "%s&nbsp;"
                      "</td>"
                      "<td colspan=\"24\" class=\"TIT_TBL\" style=\"width:%upx;"
                      " text-align:left; vertical-align:top;\">"
                      "%s"
                      "</td>"
                      "</tr>",
            Txt_Date,
            Txt_Day,
            GRAPH_DISTRIBUTION_PER_HOUR_TOTAL_WIDTH,Txt_STAT_TYPE_COUNT_CAPS[Gbl.Stat.CountType]);
   fprintf (Gbl.F.Out,"<tr>"
	              "<td colspan=\"24\""
	              " style=\"width:%upx; text-align:left;\">",
	    GRAPH_DISTRIBUTION_PER_HOUR_TOTAL_WIDTH);
   Sta_DrawBarColors (SelectedColorType,MaxPagesGenerated);
   fprintf (Gbl.F.Out,"</td>"
	              "</tr>"
	              "<tr>");
   for (Hour = 0;
	Hour < 24;
	Hour++)
      fprintf (Gbl.F.Out,"<td class=\"LOG\" style=\"width:%upx;"
	                 " text-align:center; vertical-align:top;\">"
	                 "%02uh"
	                 "</td>",
               GRAPH_DISTRIBUTION_PER_HOUR_HOUR_WIDTH,Hour);
   fprintf (Gbl.F.Out,"</tr>");

   /***** Write rows beginning by the most recent day and ending by the oldest one *****/
   mysql_data_seek (mysql_res,0);

   for (NumRow = 1;
	NumRow <= NumRows;
	NumRow++)
     {
      row = mysql_fetch_row (mysql_res);

      /* Get year, month and day (row[0] holds the date in YYYYMMDD format) */
      if (!(Dat_GetDateFromYYYYMMDD (&CurrentReadDate,row[0])))
	 Lay_ShowErrorAndExit ("Wrong date.");

      /* Get the hour (in row[1] is the hour in formato HH) */
      if (sscanf (row[1],"%02u",&ReadHour) != 1)
	 Lay_ShowErrorAndExit ("Wrong hour.");

      /* Get number of pages generated (in row[2]) */
      NumPagesGenerated = Str_GetFloatNumFromStr (row[2]);

      /* If this is the first read date, initialize PreviousReadDate */
      if (NumRow == 1)
         Dat_AssignDate (&PreviousReadDate,&CurrentReadDate);

      /* Update number of pages generated per hour */
      if (PreviousReadDate.Year  != CurrentReadDate.Year  ||
          PreviousReadDate.Month != CurrentReadDate.Month ||
          PreviousReadDate.Day   != CurrentReadDate.Day)	// Current read date (CurrentReadDate) is older than previous read date (PreviousReadDate) */
        {
         /* In the next loop we show (NumDaysFromLastDateToCurrDate-1) days (the menos antiguos) with 0 clicks
            and a last day (older) with NumPagesGenerated */
         Dat_AssignDate (&Date,&LastDate);
         NumDaysFromLastDateToCurrDate = Dat_GetNumDaysBetweenDates (&PreviousReadDate,&LastDate);
         for (D = 1;
              D <= NumDaysFromLastDateToCurrDate;
              D++)
           {
            NumDayWeek = Dat_GetDayOfWeek (Date.Year,Date.Month,Date.Day);

            /* Write the date */
            fprintf (Gbl.F.Out,"<tr>"
        	               "<td class=\"%s\" style=\"text-align:right;"
        	               " vertical-align:top;\">"
        	               "%02u/%02u/%02u&nbsp;"
        	               "</td>",
	             NumDayWeek == 6 ? "LOG_R" :
	        	               "LOG",
	             Date.Day,Date.Month,Date.Year % 100);

            /* Write the day of the week */
            fprintf (Gbl.F.Out,"<td class=\"%s\" style=\"text-align:left;"
        	               " vertical-align:top;\">"
        	               "%s&nbsp;"
        	               "</td>",
                     NumDayWeek == 6 ? "LOG_R" :
                	               "LOG",
                     Txt_DAYS_SMALL[NumDayWeek]);

            /* Draw a cell with the color proportional to the number of clicks */
            if (D == NumDaysFromLastDateToCurrDate)
               Sta_DrawAccessesPerHourForADay (SelectedColorType,NumAccPerHour,MaxPagesGenerated);
            else	// D < NumDaysFromLastDateToCurrDate
               Sta_DrawAccessesPerHourForADay (SelectedColorType,NumAccPerHourZero,MaxPagesGenerated);
            fprintf (Gbl.F.Out,"</tr>");

            /* Decrease day */
            Dat_GetDateBefore (&Date,&Date);
           }
         Dat_AssignDate (&LastDate,&Date);
         Dat_AssignDate (&PreviousReadDate,&CurrentReadDate);

         /* Reset number of pages generated per hour */
         for (Hour = 0;
              Hour < 24;
              Hour++)
            NumAccPerHour[Hour] = 0.0;
        }
      NumAccPerHour[ReadHour] = NumPagesGenerated;
     }

   /***** Show the clicks of the oldest day with clicks *****/
   /* In the next loop we show (NumDaysFromLastDateToCurrDate-1) days (more recent) with 0 clicks
      and a last day (older) with NumPagesGenerated clicks */
   Dat_AssignDate (&Date,&LastDate);
   NumDaysFromLastDateToCurrDate = Dat_GetNumDaysBetweenDates (&PreviousReadDate,&LastDate);
   for (D = 1;
	D <= NumDaysFromLastDateToCurrDate;
	D++)
     {
      NumDayWeek = Dat_GetDayOfWeek (Date.Year,Date.Month,Date.Day);

      /* Write the date */
      fprintf (Gbl.F.Out,"<tr>"
	                 "<td class=\"%s\""
	                 " style=\"text-align:right; vertical-align:top;\">"
	                 "%02u/%02u/%02u&nbsp;"
	                 "</td>",
               NumDayWeek == 6 ? "LOG_R" :
        	                 "LOG",
               Date.Day,Date.Month,Date.Year % 100);

      /* Write the day of the week */
      fprintf (Gbl.F.Out,"<td class=\"%s\""
	                 " style=\"text-align:left; vertical-align:top;\">"
	                 "%s&nbsp;"
	                 "</td>",
               NumDayWeek == 6 ? "LOG_R" :
        	                 "LOG",
               Txt_DAYS_SMALL[NumDayWeek]);

      /* Draw the color proporcional al number of clicks */
      if (D == NumDaysFromLastDateToCurrDate)
         Sta_DrawAccessesPerHourForADay (SelectedColorType,NumAccPerHour,MaxPagesGenerated);
      else	// D < NumDaysFromLastDateToCurrDate
         Sta_DrawAccessesPerHourForADay (SelectedColorType,NumAccPerHourZero,MaxPagesGenerated);
      fprintf (Gbl.F.Out,"</tr>");

      /* Decrease day */
      Dat_GetDateBefore (&Date,&Date);
     }

   /***** Finally NumDaysFromLastDateToCurrDate days are shown with 0 clicks
          (the oldest days since the initial day requested by the user until the first with clicks) *****/
   Dat_AssignDate (&LastDate,&Date);
   NumDaysFromLastDateToCurrDate = Dat_GetNumDaysBetweenDates (&Gbl.DateRange.DateIni,&LastDate);
   for (D = 1;
	D <= NumDaysFromLastDateToCurrDate;
	D++)
     {
      NumDayWeek = Dat_GetDayOfWeek (Date.Year,Date.Month,Date.Day);

      /* Write the date */
      fprintf (Gbl.F.Out,"<tr>"
	                 "<td class=\"%s\""
	                 " style=\"text-align:right; vertical-align:top;\">"
	                 "%02u/%02u/%02u&nbsp;"
	                 "</td>",
               NumDayWeek == 6 ? "LOG_R" :
        	                 "LOG",
               Date.Day,Date.Month,Date.Year % 100);

      /* Write the day of the week */
      fprintf (Gbl.F.Out,"<td class=\"%s\""
	                 " style=\"text-align:left; vertical-align:top;\">"
	                 "%s&nbsp;"
	                 "</td>",
               NumDayWeek == 6 ? "LOG_R" :
        	                 "LOG",
               Txt_DAYS_SMALL[NumDayWeek]);

      /* Draw the color proportional to number of clicks */
      Sta_DrawAccessesPerHourForADay (SelectedColorType,NumAccPerHourZero,MaxPagesGenerated);
      fprintf (Gbl.F.Out,"</tr>");

      /* Decrease day */
      Dat_GetDateBefore (&Date,&Date);
     }
  }

/*****************************************************************************/
/********************** Get type of color for statistics *********************/
/*****************************************************************************/

static Sta_ColorType_t Sta_GetStatColorType (void)
  {
   char UnsignedStr[10+1];
   unsigned UnsignedNum;

   Par_GetParToText ("ColorType",UnsignedStr,10);
   if (UnsignedStr[0])
     {
      if (sscanf (UnsignedStr,"%u",&UnsignedNum) != 1)
         Lay_ShowErrorAndExit ("Type of color is missing.");
      if (UnsignedNum >= Sta_NUM_COLOR_TYPES)
         Lay_ShowErrorAndExit ("Type of color is missing.");
      return (Sta_ColorType_t) UnsignedNum;
     }
   return (Sta_ColorType_t) 0;
  }

/*****************************************************************************/
/************************* Draw a bar with colors ****************************/
/*****************************************************************************/

static void Sta_DrawBarColors (Sta_ColorType_t ColorType,float MaxPagesGenerated)
  {
   unsigned Interval,NumColor,R,G,B;

   /***** Write numbers from 0 to MaxPagesGenerated *****/
   fprintf (Gbl.F.Out,"<table style=\"width:100%%;\">"
                      "<tr>"
	              "<td colspan=\"%u\" class=\"LOG\" style=\"width:%upx;"
	              " text-align:left; vertical-align:bottom;\">"
	              "0"
	              "</td>",
            (GRAPH_DISTRIBUTION_PER_HOUR_TOTAL_WIDTH/5)/2,
            (GRAPH_DISTRIBUTION_PER_HOUR_TOTAL_WIDTH/5)/2);
   for (Interval = 1;
	Interval <= 4;
	Interval++)
     {
      fprintf (Gbl.F.Out,"<td colspan=\"%u\" class=\"LOG\" style=\"width:%upx;"
	                 " text-align:center; vertical-align:bottom;\">",
               GRAPH_DISTRIBUTION_PER_HOUR_TOTAL_WIDTH/5,
               GRAPH_DISTRIBUTION_PER_HOUR_TOTAL_WIDTH/5);
      Sta_WriteFloatNum ((float) Interval * MaxPagesGenerated / 5.0);
      fprintf (Gbl.F.Out,"</td>");
     }
   fprintf (Gbl.F.Out,"<td colspan=\"%u\" class=\"LOG\" style=\"width:%upx;"
	              " text-align:right; vertical-align:bottom;\">",
            (GRAPH_DISTRIBUTION_PER_HOUR_TOTAL_WIDTH/5)/2,
            (GRAPH_DISTRIBUTION_PER_HOUR_TOTAL_WIDTH/5)/2);
   Sta_WriteFloatNum (MaxPagesGenerated);
   fprintf (Gbl.F.Out,"</td>"
	              "</tr>"
	              "<tr>");

   /***** Draw colors *****/
   for (NumColor = 0;
	NumColor < GRAPH_DISTRIBUTION_PER_HOUR_TOTAL_WIDTH;
	NumColor++)
     {
      Sta_SetColor (ColorType,(float) NumColor,(float) GRAPH_DISTRIBUTION_PER_HOUR_TOTAL_WIDTH,&R,&G,&B);
      fprintf (Gbl.F.Out,"<td style=\"width:1px; text-align:left;"
	                 " background-color:#%02X%02X%02X;\">"
	                 "<img src=\"%s/tr1x14.gif\" alt=\"\" />"
	                 "</td>",
               R,G,B,Gbl.Prefs.IconsURL);
     }
   fprintf (Gbl.F.Out,"</tr>"
	              "</table>");
  }

/*****************************************************************************/
/********************* Draw accesses per hour for a day **********************/
/*****************************************************************************/

static void Sta_DrawAccessesPerHourForADay (Sta_ColorType_t ColorType,float NumPagesGenerated[24],float MaxPagesGenerated)
  {
   unsigned Hour,R,G,B;

   for (Hour = 0;
	Hour < 24;
	Hour++)
     {
      Sta_SetColor (ColorType,NumPagesGenerated[Hour],MaxPagesGenerated,&R,&G,&B);
      fprintf (Gbl.F.Out,"<td class=\"LOG\" title=\"");
      Sta_WriteFloatNum (NumPagesGenerated[Hour]);
      fprintf (Gbl.F.Out,"\" style=\"width:%upx; text-align:left;"
	                 " background-color:#%02X%02X%02X;\">"
                         "</td>",
               GRAPH_DISTRIBUTION_PER_HOUR_HOUR_WIDTH,R,G,B);
     }
  }

/*****************************************************************************/
/************************* Set color depending on ratio **********************/
/*****************************************************************************/
// MaxPagesGenerated must be > 0
/*
Black         Blue         Cyan        Green        Yellow        Red
  +------------+------------+------------+------------+------------+
  |     0.2    |     0.2    |     0.2    |     0.2    |     0.2    |
  +------------+------------+------------+------------+------------+
 0.0          0.2          0.4          0.6          0.8          1.0
*/

static void Sta_SetColor (Sta_ColorType_t ColorType,float NumPagesGenerated,float MaxPagesGenerated,unsigned *R,unsigned *G,unsigned *B)
  {
   float Result = (NumPagesGenerated / MaxPagesGenerated);

   switch (ColorType)
     {
      case Sta_COLOR:
         if (Result < 0.2)		// Black -> Blue
           {
            *R = 0;
            *G = 0;
            *B = (unsigned) (Result * 256.0 / 0.2 + 0.5);
            if (*B == 256)
               *B = 255;
           }
         else if (Result < 0.4)	// Blue -> Cyan
           {
            *R = 0;
            *G = (unsigned) ((Result-0.2) * 256.0 / 0.2 + 0.5);
            if (*G == 256)
               *G = 255;
            *B = 255;
           }
         else if (Result < 0.6)	// Cyan -> Green
           {
            *R = 0;
            *G = 255;
            *B = 256 - (unsigned) ((Result-0.4) * 256.0 / 0.2 + 0.5);
            if (*B == 256)
               *B = 255;
           }
         else if (Result < 0.8)	// Green -> Yellow
           {
            *R = (unsigned) ((Result-0.6) * 256.0 / 0.2 + 0.5);
            if (*R == 256)
               *R = 255;
            *G = 255;
            *B = 0;
           }
         else			// Yellow -> Red
           {
            *R = 255;
            *G = 256 - (unsigned) ((Result-0.8) * 256.0 / 0.2 + 0.5);
            if (*G == 256)
               *G = 255;
            *B = 0;
           }
         break;
      case Sta_BLACK_TO_WHITE:
         *B = (unsigned) (Result * 256.0 + 0.5);
         if (*B == 256)
            *B = 255;
         *R = *G = *B;
         break;
      case Sta_WHITE_TO_BLACK:
         *B = 256 - (unsigned) (Result * 256.0 + 0.5);
         if (*B == 256)
            *B = 255;
         *R = *G = *B;
         break;
     }
  }

/*****************************************************************************/
/********** Show listing with number of pages generated per week *************/
/*****************************************************************************/

static void Sta_ShowNumAccessesPerWeeks (unsigned long NumRows,MYSQL_RES *mysql_res)
  {
   extern const char *Txt_Week;
   extern const char *Txt_STAT_TYPE_COUNT_CAPS[Sta_NUM_STAT_COUNT_TYPES];
   unsigned long NumRow;
   struct Date ReadDate,LastDate,Date;
   unsigned W,NumWeeksBetweenLastDateAndCurrentDate;
   float NumPagesGenerated,MaxPagesGenerated = 0.0,TotalPagesGenerated = 0.0;
   MYSQL_ROW row;

   /***** Initialize LastDate to avoid warning *****/
   Dat_CalculateWeekOfYear (&Gbl.DateRange.DateEnd);	// Changes Week and Year
   Dat_AssignDate (&LastDate,&(Gbl.DateRange.DateEnd));

   /***** Write heading *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:center; vertical-align:top;\">"
                      "%s&nbsp;"
                      "</td>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:left; vertical-align:top;\">"
                      "%s"
                      "</td>"
                      "</tr>",
            Txt_Week,
            Txt_STAT_TYPE_COUNT_CAPS[Gbl.Stat.CountType]);

   /***** Compute maximum number of pages generated per week *****/
   for (NumRow = 1;
	NumRow <= NumRows;
	NumRow++)
     {
      row = mysql_fetch_row (mysql_res);

      /* Get number of pages generated */
      NumPagesGenerated = Str_GetFloatNumFromStr (row[1]);
      if (NumPagesGenerated > MaxPagesGenerated)
	 MaxPagesGenerated = NumPagesGenerated;
      TotalPagesGenerated += NumPagesGenerated;
     }

   /***** Write rows *****/
   mysql_data_seek (mysql_res,0);
   for (NumRow = 1;
	NumRow <= NumRows;
	NumRow++)
     {
      row = mysql_fetch_row (mysql_res);

      /* Get year and week (row[0] holds date in YYYYWW format) */
      if (sscanf (row[0],"%04u%02u",&ReadDate.Year,&ReadDate.Week) != 2)
	 Lay_ShowErrorAndExit ("Wrong date.");

      /* Get number of pages generated (in row[1]) */
      NumPagesGenerated = Str_GetFloatNumFromStr (row[1]);

      Dat_AssignDate (&Date,&LastDate);
      NumWeeksBetweenLastDateAndCurrentDate = Dat_GetNumWeeksBetweenDates (&ReadDate,&LastDate);
      for (W = 1;
	   W <= NumWeeksBetweenLastDateAndCurrentDate;
	   W++)
        {
         /* Write week */
         fprintf (Gbl.F.Out,"<tr>"
                            "<td class=\"LOG\""
                            " style=\"text-align:right; vertical-align:top;\">"
                            "%02u/%02u&nbsp;"
                            "</td>",
	          Date.Week,Date.Year % 100);

         /* Draw bar proportional to number of pages generated */
         Sta_DrawBarNumClicks ('c',
                               W == NumWeeksBetweenLastDateAndCurrentDate ? NumPagesGenerated :
                        	                                            0.0,
                               MaxPagesGenerated,TotalPagesGenerated,400);

         /* Decrement week */
         Dat_GetWeekBefore (&Date,&Date);
        }
      Dat_AssignDate (&LastDate,&Date);
     }

  /***** Finally, show the old weeks without pages generated *****/
  Dat_CalculateWeekOfYear (&Gbl.DateRange.DateIni);	// Changes Week and Year
  NumWeeksBetweenLastDateAndCurrentDate = Dat_GetNumWeeksBetweenDates (&Gbl.DateRange.DateIni,&LastDate);
  for (W = 1;
       W <= NumWeeksBetweenLastDateAndCurrentDate;
       W++)
    {
     /* Write week */
     fprintf (Gbl.F.Out,"<tr>"
	                "<td class=\"LOG\""
	                " style=\"text-align:right; vertical-align:top;\">"
	                "%02u/%02u&nbsp;"
	                "</td>",
              Date.Week,Date.Year % 100);

     /* Draw bar proportional to number of pages generated */
     Sta_DrawBarNumClicks ('c',0.0,MaxPagesGenerated,TotalPagesGenerated,400);

     /* Decrement week */
     Dat_GetWeekBefore (&Date,&Date);
    }
  }

/*****************************************************************************/
/********** Show a graph with the number of clicks in each month *************/
/*****************************************************************************/

static void Sta_ShowNumAccessesPerMonths (unsigned long NumRows,MYSQL_RES *mysql_res)
  {
   extern const char *Txt_Month;
   extern const char *Txt_STAT_TYPE_COUNT_CAPS[Sta_NUM_STAT_COUNT_TYPES];
   unsigned long NumRow;
   struct Date ReadDate,LastDate,Date;
   unsigned M,NumMesesEntreLastDateYAct;
   float NumPagesGenerated,MaxPagesGenerated = 0.0,TotalPagesGenerated = 0.0;
   MYSQL_ROW row;

   /***** Initialize LastDate *****/
   Dat_AssignDate (&LastDate,&(Gbl.DateRange.DateEnd));

   /***** Write heading *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:center; vertical-align:top;\">"
                      "%s&nbsp;"
                      "</td>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:left; vertical-align:top;\">"
                      "%s"
                      "</td>"
                      "</tr>",
            Txt_Month,
            Txt_STAT_TYPE_COUNT_CAPS[Gbl.Stat.CountType]);

   /***** Compute maximum number of pages generated per month *****/
   for (NumRow = 1;
	NumRow <= NumRows;
	NumRow++)
     {
      row = mysql_fetch_row (mysql_res);

      /* Get number of pages generated */
      NumPagesGenerated = Str_GetFloatNumFromStr (row[1]);
      if (NumPagesGenerated > MaxPagesGenerated)
	 MaxPagesGenerated = NumPagesGenerated;
      TotalPagesGenerated += NumPagesGenerated;
     }

   /***** Write rows *****/
   mysql_data_seek (mysql_res,0);
   for (NumRow = 1;
	NumRow <= NumRows;
	NumRow++)
     {
      row = mysql_fetch_row (mysql_res);

      /* Get the year and the month (in row[0] is the date in YYYYMM format) */
      if (sscanf (row[0],"%04u%02u",&ReadDate.Year,&ReadDate.Month) != 2)
	 Lay_ShowErrorAndExit ("Wrong date.");

      /* Get number of pages generated (in row[1]) */
      NumPagesGenerated = Str_GetFloatNumFromStr (row[1]);

      Dat_AssignDate (&Date,&LastDate);
      NumMesesEntreLastDateYAct = Dat_GetNumMonthsBetweenDates (&ReadDate,&LastDate);
      for (M = 1;
	   M <= NumMesesEntreLastDateYAct;
	   M++)
        {
         /* Write the month */
         fprintf (Gbl.F.Out,"<tr>"
                            "<td class=\"LOG\""
                            " style=\"text-align:right; vertical-align:top;\">"
                            "%02u/%02u&nbsp;"
                            "</td>",
	          Date.Month,Date.Year % 100);

         /* Draw bar proportional to number of pages generated */
         Sta_DrawBarNumClicks ('c',
                               M == NumMesesEntreLastDateYAct ? NumPagesGenerated :
                        	                                0.0,
                               MaxPagesGenerated,TotalPagesGenerated,400);

         /* Decrease month */
         Dat_GetMonthBefore (&Date,&Date);
        }
      Dat_AssignDate (&LastDate,&Date);
     }

  /***** Finally, show the oldest months without clicks *****/
  NumMesesEntreLastDateYAct = Dat_GetNumMonthsBetweenDates (&Gbl.DateRange.DateIni,&LastDate);
  for (M = 1;
       M <= NumMesesEntreLastDateYAct;
       M++)
    {
     /* Write the month */
     fprintf (Gbl.F.Out,"<tr>"
	                "<td class=\"LOG\""
	                " style=\"text-align:right; vertical-align:top;\">"
	                "%02u/%02u&nbsp;"
	                "</td>",
              Date.Month,Date.Year % 100);

     /* Draw bar proportional to number of pages generated */
     Sta_DrawBarNumClicks ('c',0.0,MaxPagesGenerated,TotalPagesGenerated,400);

     /* Decrease month */
     Dat_GetMonthBefore (&Date,&Date);
    }
  }

/*****************************************************************************/
/**************** Show graphic of number of pages generated per hour ***************/
/*****************************************************************************/

#define DIGIT_WIDTH 5

static void Sta_ShowNumAccessesPerHour (unsigned long NumRows,MYSQL_RES *mysql_res)
  {
   unsigned long NumRow;
   float NumPagesGenerated,MaxPagesGenerated = 0.0,TotalPagesGenerated = 0.0;
   unsigned NumDays;
   unsigned Hour=0,ReadHour=0,H;
   unsigned NumDigits,ColumnWidth;
   MYSQL_ROW row;

   if ((NumDays = Dat_GetNumDaysBetweenDates (&Gbl.DateRange.DateIni,&Gbl.DateRange.DateEnd)))
     {
      /***** Compute maximum number of pages generated per hour *****/
      for (NumRow = 1;
	   NumRow <= NumRows;
	   NumRow++)
	{
	 row = mysql_fetch_row (mysql_res);

	 /* Get number of pages generated */
	 NumPagesGenerated = Str_GetFloatNumFromStr (row[1]) / (float) NumDays;
	 if (NumPagesGenerated > MaxPagesGenerated)
	    MaxPagesGenerated = NumPagesGenerated;
	 TotalPagesGenerated += NumPagesGenerated;
	}

      /***** Compute width of columns (one for each hour) *****/
      /* Maximum number of d�gits. If less than 4, set it to 4 to ensure a minimum width */
      NumDigits = (MaxPagesGenerated >= 1000) ? (unsigned) floor (log10 ((double) MaxPagesGenerated)) + 1 :
	                                        4;
      ColumnWidth = NumDigits * DIGIT_WIDTH + 2;

      /***** Draw the graphic *****/
      mysql_data_seek (mysql_res, 0);
      NumRow = 1;
      fprintf (Gbl.F.Out,"<tr>");
      while (Hour < 24)
	{
	 if (NumRow <= NumRows)	// If not read yet all the results of the query
	   {
	    row = mysql_fetch_row (mysql_res); // Get next result
	    NumRow++;
	    if (sscanf (row[0],"%02u",&ReadHour) != 1)   // In row[0] is the date in HH format
	       Lay_ShowErrorAndExit ("Wrong hour.");
	    NumPagesGenerated = Str_GetFloatNumFromStr (row[1]) / (float) NumDays;
	    for (H = Hour;
		 H < ReadHour;
		 H++, Hour++)
	       Sta_WriteAccessHour (H,0.0,MaxPagesGenerated,TotalPagesGenerated,ColumnWidth);
	    Sta_WriteAccessHour (ReadHour,NumPagesGenerated,MaxPagesGenerated,TotalPagesGenerated,ColumnWidth);
	    Hour++;
	   }
	 else
	    for (H = ReadHour + 1;
		 H < 24;
		 H++, Hour++)
	       Sta_WriteAccessHour (H,0.0,MaxPagesGenerated,TotalPagesGenerated,ColumnWidth);
	}
      fprintf (Gbl.F.Out,"</tr>");
     }
  }

/*****************************************************************************/
/**** Write a column of the graphic of the number of clicks in each hour *****/
/*****************************************************************************/

static void Sta_WriteAccessHour (unsigned Hour,float NumPagesGenerated,float MaxPagesGenerated,float TotalPagesGenerated,unsigned ColumnWidth)
  {
   unsigned AltoBarra;

   fprintf (Gbl.F.Out,"<td class=\"DAT_SMALL\" style=\"width:%upx;"
	              " text-align:center; vertical-align:bottom;\">",
	    ColumnWidth);

   /* Draw bar with a height porportional to the number of clicks */
   if (NumPagesGenerated > 0.0)
     {
      fprintf (Gbl.F.Out,"%u%%<br />",
	       (unsigned) (((NumPagesGenerated * 100.0) /
		            TotalPagesGenerated) + 0.5));
      Sta_WriteFloatNum (NumPagesGenerated);
      fprintf (Gbl.F.Out,"<br />");
      AltoBarra = (unsigned) (((NumPagesGenerated * 400.0) / MaxPagesGenerated) + 0.5);
      if (AltoBarra == 0)
         AltoBarra = 1;
      fprintf (Gbl.F.Out,"<img src=\"%s/c8x1.gif\" alt=\"\""
	                 " style=\"width:8px; height:%upx;\" /><br />",
	       Gbl.Prefs.IconsURL,AltoBarra);
     }
   else
      fprintf (Gbl.F.Out,"0%%<br />0<br />");

   /* Write the hour */
   fprintf (Gbl.F.Out,"%uh</td>",Hour);
  }

/*****************************************************************************/
/**** Show a listing with the number of clicks in every minute of the day ***/
/*****************************************************************************/

#define NUM_MINUTES_PER_DAY		(60*24)			// 1440 minutes in a day
#define WIDTH_SEMIDIVISION_GRAPHIC	24
#define NUM_DIVISIONS_X			10

static void Sta_ShowAverageAccessesPerMinute (unsigned long NumRows,MYSQL_RES *mysql_res)
  {
   unsigned long NumRow=1;
   MYSQL_ROW row;
   unsigned NumDays;
   unsigned MinuteDay=0,ReadHour,MinuteRead,MinuteDayRead=0,i;
   float NumPagesGenerated;
   float NumClicksPerMin[NUM_MINUTES_PER_DAY],MaxPagesGenerated = 0.0,
         Power10LeastOrEqual,MaxX,IncX;
   char *Format;

   if ((NumDays = Dat_GetNumDaysBetweenDates (&Gbl.DateRange.DateIni,&Gbl.DateRange.DateEnd)))
     {
      /***** Compute number of clicks (and m�ximo) in every minute *****/
      while (MinuteDay < NUM_MINUTES_PER_DAY)
	{
	 if (NumRow <= NumRows)	// If not all the result of the query are yet read
	   {
	    row = mysql_fetch_row (mysql_res); // Get next result
	    NumRow++;
	    if (sscanf (row[0],"%02u%02u",&ReadHour,&MinuteRead) != 2)   // In row[0] is the date in formato HHMM
	       Lay_ShowErrorAndExit ("Wrong hour-minute.");
	    /* Get number of pages generated */
	    NumPagesGenerated = Str_GetFloatNumFromStr (row[1]);
	    MinuteDayRead = ReadHour*60 + MinuteRead;
	    for (i = MinuteDay;
		 i < MinuteDayRead;
		 i++, MinuteDay++)
	       NumClicksPerMin[i] = 0.0;
	    NumClicksPerMin[MinuteDayRead] = NumPagesGenerated / (float) NumDays;
	    if (NumClicksPerMin[MinuteDayRead] > MaxPagesGenerated)
	       MaxPagesGenerated = NumClicksPerMin[MinuteDayRead];
	    MinuteDay++;
	   }
	 else
	    for (i = MinuteDayRead + 1;
		 i < NUM_MINUTES_PER_DAY;
		 i++, MinuteDay++)
	       NumClicksPerMin[i] = 0.0;
	}

      /***** Compute the maximum value of X and the increment of the X axis *****/
      if (MaxPagesGenerated <= 0.000001)
	 MaxX = 0.000001;
      else
	{
	 Power10LeastOrEqual = (float) pow (10.0,floor (log10 ((double) MaxPagesGenerated)));
	 MaxX = ceil (MaxPagesGenerated / Power10LeastOrEqual) * Power10LeastOrEqual;
	}
      IncX = MaxX / (float) NUM_DIVISIONS_X;
      if (IncX >= 1.0)
	 Format = "%.0f";
      else if (IncX >= 0.1)
	 Format = "%.1f";
      else if (IncX >= 0.01)
	 Format = "%.2f";
      else if (IncX >= 0.001)
	 Format = "%.3f";
      else
	 Format = "%f";

      /***** X axis tags *****/
      Sta_WriteLabelsXAxisAccMin (IncX,Format);

      /***** Y axis and graphic *****/
      for (i = 0;
	   i < NUM_MINUTES_PER_DAY;
	   i++)
	 Sta_WriteAccessMinute (i,NumClicksPerMin[i],MaxX);

      /***** X axis *****/
      /* First division (left) */
      fprintf (Gbl.F.Out,"<tr>"
	                 "<td style=\"width:%upx;text-align:left;\">"
	                 "<img src=\"%s/ejexizq24x1.gif\" alt=\"\""
	                 " style=\"display:block; width:%upx; height:1px;\" />"
	                 "</td>",
	       WIDTH_SEMIDIVISION_GRAPHIC,Gbl.Prefs.IconsURL,
	       WIDTH_SEMIDIVISION_GRAPHIC);
      /* All the intermediate divisions */
      for (i = 0;
	   i < NUM_DIVISIONS_X*2;
	   i++)
	 fprintf (Gbl.F.Out,"<td style=\"width:%upx; text-align:left;\">"
	                    "<img src=\"%s/ejex24x1.gif\" alt=\"\""
	                    " style=\"display:block;"
	                    " width:%upx; height:1px;\" />"
	                    "</td>",
		  WIDTH_SEMIDIVISION_GRAPHIC,Gbl.Prefs.IconsURL,
		  WIDTH_SEMIDIVISION_GRAPHIC);
      /* Last division (right) */
      fprintf (Gbl.F.Out,"<td style=\"width:%upx; text-align:left;\">"
	                 "<img src=\"%s/tr24x1.gif\" alt=\"\""
	                 " style=\"display:block; width:%upx; height:1px;\" />"
	                 "</td>"
	                 "</tr>",
	       WIDTH_SEMIDIVISION_GRAPHIC,Gbl.Prefs.IconsURL,
	       WIDTH_SEMIDIVISION_GRAPHIC);

      /***** Write again the labels of the X axis *****/
      Sta_WriteLabelsXAxisAccMin (IncX,Format);
     }
  }

/*****************************************************************************/
/****** Write labels of the X axis in the graphic of clicks per minute *******/
/*****************************************************************************/

#define WIDTH_DIVISION_GRAPHIC	(WIDTH_SEMIDIVISION_GRAPHIC*2)	// 48

static void Sta_WriteLabelsXAxisAccMin (float IncX,const char *Format)
  {
   unsigned i;
   float NumX;

   fprintf (Gbl.F.Out,"<tr>");
   for (i = 0, NumX = 0;
	i <= NUM_DIVISIONS_X;
	i++, NumX += IncX)
     {
      fprintf (Gbl.F.Out,"<td colspan=\"2\" class=\"LOG\" style=\"width:%upx;"
	                 " text-align:center; vertical-align:bottom;\">",
               WIDTH_DIVISION_GRAPHIC);
      fprintf (Gbl.F.Out,Format,NumX);
      fprintf (Gbl.F.Out,"</td>");
     }
   fprintf (Gbl.F.Out,"</tr>");
  }

/*****************************************************************************/
/***** Write a row of the graphic with number of clicks in every minute ******/
/*****************************************************************************/

#define WIDTH_GRAPHIC		(WIDTH_DIVISION_GRAPHIC*NUM_DIVISIONS_X)	// 48*10=480

static void Sta_WriteAccessMinute (unsigned Minute,float NumPagesGenerated,float MaxX)
  {
   unsigned BarWidth;

   /***** Start row *****/
   fprintf (Gbl.F.Out,"<tr>");

   /***** Labels of the Y axis, and Y axis *****/
   if (!Minute)
      // If minute 0
      fprintf (Gbl.F.Out,"<td rowspan=\"30\" class=\"LOG\" style=\"width:%upx;"
	                 " text-align:left; vertical-align:top;"
	                 " background-image:url('%s/ejey24x30.gif');"
	                 " background-repeat:repeat;\">"
	                 "00h"
	                 "</td>",
               WIDTH_SEMIDIVISION_GRAPHIC,Gbl.Prefs.IconsURL);
   else if (Minute == (NUM_MINUTES_PER_DAY - 30))
      // If 23:30
      fprintf (Gbl.F.Out,"<td rowspan=\"30\" class=\"LOG\" style=\"width:%upx;"
	                 " text-align:left; vertical-align:bottom;"
	                 " background-image:url('%s/ejey24x30.gif');"
	                 " background-repeat:repeat;\">"
	                 "24h"
	                 "</td>",
               WIDTH_SEMIDIVISION_GRAPHIC,Gbl.Prefs.IconsURL);
   else if (!(Minute % 30) && (Minute % 60))
      // If minute is multiple of 30 but not of 60 (i.e.: 30, 90, 150...)
      fprintf (Gbl.F.Out,"<td rowspan=\"60\" class=\"LOG\" style=\"width:%upx;"
	                 " text-align:left; vertical-align:middle;"
	                 " background-image:url('%s/ejey24x60.gif');"
	                 " background-repeat:repeat;\">"
	                 "%02uh"
	                 "</td>",
               WIDTH_SEMIDIVISION_GRAPHIC,Gbl.Prefs.IconsURL,(Minute + 30) / 60);

   /***** Start of cell for the graphic *****/
   fprintf (Gbl.F.Out,"<td colspan=\"%u\" style=\"width:%upx; height:1px;"
	              " text-align:left; vertical-align:bottom;"
	              " background-image:url('%s/malla%c48x1.gif');"
	              " background-repeat:repeat;\">",
	    NUM_DIVISIONS_X*2,WIDTH_GRAPHIC,Gbl.Prefs.IconsURL,
	    (Minute % 60) == 0 ? 'v' :
		                 'h');

   /***** Draw bar with anchura proporcional al number of clicks *****/
   if (NumPagesGenerated != 0.0)
      if ((BarWidth = (unsigned) (((NumPagesGenerated * (float) WIDTH_GRAPHIC / MaxX)) + 0.5)) != 0)
	 fprintf (Gbl.F.Out,"<img src=\"%s/b%c1x1.gif\" alt=\"\""
	                    " style=\"display:block;"
	                    " width:%upx; height:1px;\" />",
                  Gbl.Prefs.IconsURL,
                  (Minute % 60) == 0 ? 'g' :
                	               'b',
                  BarWidth);

   /***** End of cell of graphic and end of row *****/
   fprintf (Gbl.F.Out,"</td>"
	              "</tr>");
  }

/*****************************************************************************/
/**** Show a listing of accesses with the number of clicks a each action *****/
/*****************************************************************************/

static void Sta_ShowNumAccessesPerAction (unsigned long NumRows,MYSQL_RES *mysql_res)
  {
   extern const char *Txt_Action;
   extern const char *Txt_STAT_TYPE_COUNT_CAPS[Sta_NUM_STAT_COUNT_TYPES];
   unsigned long NumRow;
   float NumPagesGenerated;
   float MaxPagesGenerated = 0.0;
   float TotalPagesGenerated = 0.0;
   MYSQL_ROW row;
   long ActCod;
   char ActTxt[Act_MAX_LENGTH_ACTION_TXT+1];

   /***** Write heading *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:right; vertical-align:top;\">"
                      "%s&nbsp;"
                      "</td>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:left; vertical-align:top;\">"
                      "%s"
                      "</td>"
                      "</tr>",
            Txt_Action,
            Txt_STAT_TYPE_COUNT_CAPS[Gbl.Stat.CountType]);

   /***** Compute maximum number of pages generated per day *****/
   for (NumRow = 1;
	NumRow <= NumRows;
	NumRow++)
     {
      row = mysql_fetch_row (mysql_res);

      /* Get number of pages generated */
      NumPagesGenerated = Str_GetFloatNumFromStr (row[1]);
      if (NumRow == 1)
	 MaxPagesGenerated = NumPagesGenerated;
      TotalPagesGenerated += NumPagesGenerated;
     }

   /***** Write rows *****/
   mysql_data_seek (mysql_res, 0);
   for (NumRow = 1;
	NumRow <= NumRows;
	NumRow++)
     {
      row = mysql_fetch_row (mysql_res);

      /* Write the action */
      if (sscanf (row[0],"%ld",&ActCod) != 1)
	 Lay_ShowErrorAndExit ("Wrong action code.");
      if (ActCod >= 0)
         fprintf (Gbl.F.Out,"<tr>"
                            "<td class=\"LOG\""
                            " style=\"text-align:right; vertical-align:top;\">"
                            "%s&nbsp;"
                            "</td>",
                  Act_GetActionTextFromDB (ActCod,ActTxt));
      else
         fprintf (Gbl.F.Out,"<tr>"
                            "<td class=\"LOG\""
                            " style=\"text-align:right; vertical-align:top;\">"
                            "?&nbsp;"
                            "</td>");

      /* Draw bar proportional to number of pages generated */
      NumPagesGenerated = Str_GetFloatNumFromStr (row[1]);
      Sta_DrawBarNumClicks ('c',NumPagesGenerated,MaxPagesGenerated,TotalPagesGenerated,400);
     }
  }

/*****************************************************************************/
/*************** Show number of clicks distributed by plugin *****************/
/*****************************************************************************/

static void Sta_ShowNumAccessesPerPlugin (unsigned long NumRows,MYSQL_RES *mysql_res)
  {
   extern const char *Txt_Plugin;
   extern const char *Txt_STAT_TYPE_COUNT_CAPS[Sta_NUM_STAT_COUNT_TYPES];
   unsigned long NumRow;
   float NumPagesGenerated;
   float MaxPagesGenerated = 0.0;
   float TotalPagesGenerated = 0.0;
   MYSQL_ROW row;
   struct Plugin Plg;

   /***** Write heading *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:right; vertical-align:top;\">"
                      "%s&nbsp;"
                      "</td>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:left; vertical-align:top;\">"
                      "%s"
                      "</td>"
                      "</tr>",
            Txt_Plugin,
            Txt_STAT_TYPE_COUNT_CAPS[Gbl.Stat.CountType]);

   /***** Compute maximum number of pages generated per plugin *****/
   for (NumRow = 1;
	NumRow <= NumRows;
	NumRow++)
     {
      row = mysql_fetch_row (mysql_res);

      /* Get number of pages generated */
      NumPagesGenerated = Str_GetFloatNumFromStr (row[1]);
      if (NumRow == 1)
	 MaxPagesGenerated = NumPagesGenerated;
      TotalPagesGenerated += NumPagesGenerated;
     }

   /***** Write rows *****/
   mysql_data_seek (mysql_res, 0);
   for (NumRow = 1;
	NumRow <= NumRows;
	NumRow++)
     {
      row = mysql_fetch_row (mysql_res);

      /* Write the plugin */
      if (sscanf (row[0],"%ld",&Plg.PlgCod) != 1)
	 Lay_ShowErrorAndExit ("Wrong plugin code.");
      fprintf (Gbl.F.Out,"<tr>"
	                 "<td class=\"LOG\""
	                 " style=\"text-align:right; vertical-align:top;\">");
      if (Plg_GetDataOfPluginByCod (&Plg))
         fprintf (Gbl.F.Out,"%s",Plg.Name);
      else
         fprintf (Gbl.F.Out,"?");
      fprintf (Gbl.F.Out,"&nbsp;</td>");

      /* Draw bar proportional to number of pages generated */
      NumPagesGenerated = Str_GetFloatNumFromStr (row[1]);
      Sta_DrawBarNumClicks ('c',NumPagesGenerated,MaxPagesGenerated,TotalPagesGenerated,400);
     }
  }

/*****************************************************************************/
/******** Show number of clicks distributed by web service function **********/
/*****************************************************************************/

static void Sta_ShowNumAccessesPerWSFunction (unsigned long NumRows,MYSQL_RES *mysql_res)
  {
   extern const char *Txt_Function;
   extern const char *Txt_STAT_TYPE_COUNT_CAPS[Sta_NUM_STAT_COUNT_TYPES];
   unsigned long NumRow;
   float NumPagesGenerated;
   float MaxPagesGenerated = 0.0;
   float TotalPagesGenerated = 0.0;
   MYSQL_ROW row;
   long FunCod;

   /***** Write heading *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:left; vertical-align:top;\">"
                      "%s&nbsp;"
                      "</td>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:left; vertical-align:top;\">"
                      "%s"
                      "</td>"
                      "</tr>",
            Txt_Function,
            Txt_STAT_TYPE_COUNT_CAPS[Gbl.Stat.CountType]);

   /***** Compute maximum number of pages generated per function *****/
   for (NumRow = 1;
	NumRow <= NumRows;
	NumRow++)
     {
      row = mysql_fetch_row (mysql_res);

      /* Get number of pages generated */
      NumPagesGenerated = Str_GetFloatNumFromStr (row[1]);
      if (NumRow == 1)
	 MaxPagesGenerated = NumPagesGenerated;
      TotalPagesGenerated += NumPagesGenerated;
     }

   /***** Write rows *****/
   mysql_data_seek (mysql_res, 0);
   for (NumRow = 1;
	NumRow <= NumRows;
	NumRow++)
     {
      row = mysql_fetch_row (mysql_res);

      /* Write the plugin */
      if (sscanf (row[0],"%ld",&FunCod) != 1)
	 Lay_ShowErrorAndExit ("Wrong function code.");
      fprintf (Gbl.F.Out,"<tr>"
	                 "<td class=\"LOG\""
	                 " style=\"text-align:left; vertical-align:top;\">"
	                 "%s&nbsp;"
	                 "</td>",
               Svc_GetFunctionNameFromFunCod (FunCod));

      /* Draw bar proportional to number of pages generated */
      NumPagesGenerated = Str_GetFloatNumFromStr (row[1]);
      Sta_DrawBarNumClicks ('c',NumPagesGenerated,MaxPagesGenerated,TotalPagesGenerated,400);
     }
  }

/*****************************************************************************/
/******** Show number of clicks distributed by web service function **********/
/*****************************************************************************/

static void Sta_ShowNumAccessesPerBanner (unsigned long NumRows,MYSQL_RES *mysql_res)
  {
   extern const char *Txt_Banner;
   extern const char *Txt_STAT_TYPE_COUNT_CAPS[Sta_NUM_STAT_COUNT_TYPES];
   unsigned long NumRow;
   float NumClicks;
   float MaxClicks = 0.0;
   float TotalClicks = 0.0;
   MYSQL_ROW row;
   struct Banner Ban;

   /***** Write heading *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:center; vertical-align:top;\">"
                      "%s&nbsp;"
                      "</td>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:left; vertical-align:top;\">"
                      "%s"
                      "</td>"
                      "</tr>",
            Txt_Banner,
            Txt_STAT_TYPE_COUNT_CAPS[Gbl.Stat.CountType]);

   /***** Compute maximum number of clicks per banner *****/
   for (NumRow = 1;
	NumRow <= NumRows;
	NumRow++)
     {
      row = mysql_fetch_row (mysql_res);

      /* Get number of pages generated */
      NumClicks = Str_GetFloatNumFromStr (row[1]);
      if (NumRow == 1)
	 MaxClicks = NumClicks;
      TotalClicks += NumClicks;
     }

   /***** Write rows *****/
   mysql_data_seek (mysql_res, 0);
   for (NumRow = 1;
	NumRow <= NumRows;
	NumRow++)
     {
      row = mysql_fetch_row (mysql_res);

      /* Write the banner */
      if (sscanf (row[0],"%ld",&(Ban.BanCod)) != 1)
	 Lay_ShowErrorAndExit ("Wrong banner code.");
      Ban_GetDataOfBannerByCod (&Ban);
      fprintf (Gbl.F.Out,"<tr>"
                         "<td class=\"LOG\""
                         " style=\"text-align:left; vertical-align:top;\">"
                         "<a href=\"%s\" title=\"%s\" class=\"DAT\" target=\"_blank\">"
                         "<img src=\"%s/%s/%s\" alt=\"%s\""
                         " style=\"width:60px; height:20px;"
                         " margin:0 8px 4px 0;\" />"
                         "</a>",
               Ban.WWW,
               Ban.FullName,
               Cfg_HTTPS_URL_SWAD_PUBLIC,Cfg_FOLDER_BANNER,
               Ban.Img,
               Ban.ShortName);

      /* Draw bar proportional to number of clicks */
      NumClicks = Str_GetFloatNumFromStr (row[1]);
      Sta_DrawBarNumClicks ('c',NumClicks,MaxClicks,TotalClicks,400);
     }
  }

/*****************************************************************************/
/****** Show a listing with the number of clicks distributed by degree *******/
/*****************************************************************************/

static void Sta_ShowNumAccessesPerDegree (unsigned long NumRows,MYSQL_RES *mysql_res)
  {
   extern const char *Txt_No_INDEX;
   extern const char *Txt_Degree;
   extern const char *Txt_STAT_TYPE_COUNT_CAPS[Sta_NUM_STAT_COUNT_TYPES];
   unsigned long NumRow,Ranking;
   float NumPagesGenerated;
   float MaxPagesGenerated = 0.0;
   float TotalPagesGenerated = 0.0;
   MYSQL_ROW row;
   long DegCod;

   /***** Write heading *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:center; vertical-align:top;\">"
                      "%s"
                      "</td>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:center; vertical-align:top;\">"
                      "%s"
                      "</td>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:left; vertical-align:top;\">"
                      "%s"
                      "</td>"
                      "</tr>",
            Txt_No_INDEX,
            Txt_Degree,
            Txt_STAT_TYPE_COUNT_CAPS[Gbl.Stat.CountType]);

   /***** Compute maximum number of pages generated per degree *****/
   for (NumRow = 1;
	NumRow <= NumRows;
	NumRow++)
     {
      row = mysql_fetch_row (mysql_res);

      /* Get number of pages generated */
      NumPagesGenerated = Str_GetFloatNumFromStr (row[1]);

      if (NumRow == 1)
	 MaxPagesGenerated = NumPagesGenerated;
      TotalPagesGenerated += NumPagesGenerated;
     }

   /***** Write rows *****/
   mysql_data_seek (mysql_res, 0);
   for (NumRow = 1, Ranking = 0;
	NumRow <= NumRows;
	NumRow++)
     {
      /* Get degree */
      row = mysql_fetch_row (mysql_res);

      /* Get degree code */
      DegCod = Str_ConvertStrCodToLongCod (row[0]);

      /* Write ranking of this degree */
      fprintf (Gbl.F.Out,"<tr>"
	                 "<td class=\"LOG\""
	                 " style=\"text-align:right; vertical-align:top;\">");
      if (DegCod > 0)
         fprintf (Gbl.F.Out,"%lu",++Ranking);
      fprintf (Gbl.F.Out,"&nbsp;"
	                 "</td>");

      /* Write degree */
      Sta_WriteDegree (DegCod);

      /* Draw bar proportional to number of pages generated */
      NumPagesGenerated = Str_GetFloatNumFromStr (row[1]);
      Sta_DrawBarNumClicks ('c',NumPagesGenerated,MaxPagesGenerated,TotalPagesGenerated,300);
     }
  }

/*****************************************************************************/
/********* Show a listing with the number of clicks to each course ***********/
/*****************************************************************************/

static void Sta_ShowNumAccessesPerCourse (unsigned long NumRows,MYSQL_RES *mysql_res)
  {
   extern const char *Txt_No_INDEX;
   extern const char *Txt_Degree;
   extern const char *Txt_Year_OF_A_DEGREE;
   extern const char *Txt_Course;
   extern const char *Txt_STAT_TYPE_COUNT_CAPS[Sta_NUM_STAT_COUNT_TYPES];
   extern const char *Txt_Go_to_X;
   extern const char *Txt_YEAR_OF_DEGREE[1+Deg_MAX_YEARS_PER_DEGREE];	// Declaration in swad_degree.c
   unsigned long NumRow;
   unsigned long Ranking;
   float NumPagesGenerated;
   float MaxPagesGenerated = 0.0;
   float TotalPagesGenerated = 0.0;
   MYSQL_ROW row;
   bool CrsOK;
   struct Course Crs;

   /***** Write heading *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:center; vertical-align:top;\">"
                      "%s"
                      "</td>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:center; vertical-align:top;\">"
                      "%s"
                      "</td>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:center; vertical-align:top;\">"
                      "%s"
                      "</td>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:center; vertical-align:top;\">"
                      "%s"
                      "</td>"
                      "<td class=\"TIT_TBL\""
                      " style=\"text-align:left; vertical-align:top;\">"
                      "%s"
                      "</td>"
                      "</tr>",
            Txt_No_INDEX,
            Txt_Degree,
            Txt_Year_OF_A_DEGREE,
            Txt_Course,
            Txt_STAT_TYPE_COUNT_CAPS[Gbl.Stat.CountType]);

   /***** Compute maximum number of pages generated per course *****/
   for (NumRow = 1;
	NumRow <= NumRows;
	NumRow++)
     {
      row = mysql_fetch_row (mysql_res);

      /* Get number of pages generated */
      NumPagesGenerated = Str_GetFloatNumFromStr (row[1]);

      if (NumRow == 1)
	 MaxPagesGenerated = NumPagesGenerated;
      TotalPagesGenerated += NumPagesGenerated;
     }

   /***** Write rows *****/
   mysql_data_seek (mysql_res, 0);
   for (NumRow = 1, Ranking=0;
	NumRow <= NumRows;
	NumRow++)
     {
      /* Get degree, the year and the course */
      row = mysql_fetch_row (mysql_res);

      /* Get course code */
      Crs.CrsCod = Str_ConvertStrCodToLongCod (row[0]);

      /* Get data of current degree */
      CrsOK = Crs_GetDataOfCourseByCod (&Crs);

      /* Write ranking of this course */
      fprintf (Gbl.F.Out,"<tr>"
	                 "<td class=\"LOG\""
	                 " style=\"text-align:right; vertical-align:top;\">");
      if (CrsOK)
         fprintf (Gbl.F.Out,"%lu",++Ranking);
      fprintf (Gbl.F.Out,"&nbsp;</td>");

      /* Write degree */
      Sta_WriteDegree (Crs.DegCod);

      /* Write degree year */
      fprintf (Gbl.F.Out,"<td class=\"LOG\""
	                 " style=\"text-align:center; vertical-align:top;\">"
	                 "%s&nbsp;"
	                 "</td>",
               CrsOK ? Txt_YEAR_OF_DEGREE[Crs.Year] :
        	       "-");

      /* Write course, including link */
      fprintf (Gbl.F.Out,"<td class=\"LOG\""
	                 " style=\"text-align:left; vertical-align:top;\">");
      if (CrsOK)
        {
         Act_FormGoToStart (ActSeeCrsInf);
         Crs_PutParamCrsCod (Crs.CrsCod);
         sprintf (Gbl.Title,Txt_Go_to_X,Crs.FullName);
         Act_LinkFormSubmit (Gbl.Title,"LOG");
         fprintf (Gbl.F.Out,"%s</a>",Crs.ShortName);
        }
      else
         fprintf (Gbl.F.Out,"-");
      fprintf (Gbl.F.Out,"&nbsp;");
      if (CrsOK)
         fprintf (Gbl.F.Out,"</form>");
      fprintf (Gbl.F.Out,"</td>");

      /* Draw bar proportional to number of pages generated */
      NumPagesGenerated = Str_GetFloatNumFromStr (row[1]);
      Sta_DrawBarNumClicks ('c',NumPagesGenerated,MaxPagesGenerated,TotalPagesGenerated,300);
     }
  }

/*****************************************************************************/
/************************* Write degree with an icon *************************/
/*****************************************************************************/

static void Sta_WriteDegree (long DegCod)
  {
   extern const char *Txt_Clicks_without_degree_selected;
   struct Degree Deg;

   fprintf (Gbl.F.Out,"<td class=\"LOG\""
	              " style=\"text-align:left; vertical-align:middle;\""
	              " title=\"");
   if (DegCod > 0)
     {
      Deg.DegCod = DegCod;
      Deg_GetDataOfDegreeByCod (&Deg);
      fprintf (Gbl.F.Out,"%s\">"
                         "<a href=\"%s\" class=\"LOG\" target=\"_blank\">",
               Deg.FullName,Deg.WWW);
      Log_DrawLogo (Sco_SCOPE_DEGREE,Deg.DegCod,Deg.ShortName,
                    16,"vertical-align:top;",true);
      fprintf (Gbl.F.Out,"&nbsp;%s&nbsp;</a>",
               Deg.ShortName);
     }
   else
      fprintf (Gbl.F.Out,"%s\">"
                         "&nbsp;-&nbsp;",
               Txt_Clicks_without_degree_selected);
   fprintf (Gbl.F.Out,"</td>");
  }

/*****************************************************************************/
/******************** Draw a bar with the number of clicks *******************/
/*****************************************************************************/

static void Sta_DrawBarNumClicks (char Color,float NumPagesGenerated,float MaxPagesGenerated,float TotalPagesGenerated,unsigned MaxBarWidth)
  {
   unsigned BarWidth;

   fprintf (Gbl.F.Out,"<td class=\"LOG\""
	              " style=\"text-align:left; vertical-align:middle;\">");
   if (NumPagesGenerated != 0.0)
     {
      /* Draw bar with a with proportional to the number of clicks */
      BarWidth = (unsigned) (((NumPagesGenerated * (float) MaxBarWidth) / MaxPagesGenerated) + 0.5);
      if (BarWidth == 0)
         BarWidth = 1;
      fprintf (Gbl.F.Out,"<img src=\"%s/%c1x14.gif\" alt=\"\""
	                 " style=\"width:%upx; height:14px;"
	                 " vertical-align:top;\" />"
                         "&nbsp;",
	       Gbl.Prefs.IconsURL,Color,BarWidth);

      /* Write the number of clicks */
      Sta_WriteFloatNum (NumPagesGenerated);
      fprintf (Gbl.F.Out,"&nbsp;(%u",
               (unsigned) (((NumPagesGenerated * 100.0) /
        	            TotalPagesGenerated) + 0.5));
     }
   else
      /* Write the number of clicks */
      fprintf (Gbl.F.Out,"0&nbsp;(0");
   fprintf (Gbl.F.Out,"%%)&nbsp;"
	              "</td>"
	              "</tr>");
  }

/*****************************************************************************/
/**** Write parameters of initial and final dates in the query of clicks *****/
/*****************************************************************************/

void Sta_WriteParamsDatesSeeAccesses (void)
  {
   Par_PutHiddenParamUnsigned ("StartDay"  ,Gbl.DateRange.DateIni.Day);
   Par_PutHiddenParamUnsigned ("StartMonth",Gbl.DateRange.DateIni.Month);
   Par_PutHiddenParamUnsigned ("StartYear" ,Gbl.DateRange.DateIni.Year);

   Par_PutHiddenParamUnsigned ("EndDay"    ,Gbl.DateRange.DateEnd.Day);
   Par_PutHiddenParamUnsigned ("EndMonth"  ,Gbl.DateRange.DateEnd.Month);
   Par_PutHiddenParamUnsigned ("EndYear"   ,Gbl.DateRange.DateEnd.Year);
  }

/*****************************************************************************/
/******************** Write the selected range of dates **********************/
/*****************************************************************************/

static void Sta_WriteSelectedRangeOfDates (unsigned NumDays)
  {
   extern const char *Txt_Date;
   extern const char *Txt_Dates;
   extern const char *Txt_DATES_RANGE;
   extern const char *Txt_one_day;
   extern const char *Txt_days;
   char StrDateIni[2+1+2+1+4+1];
   char StrDateEnd[2+1+2+1+4+1];
   char StrDatesRange[1024];

   sprintf (StrDateIni,"%02u/%02u/%04u",Gbl.DateRange.DateIni.Day,Gbl.DateRange.DateIni.Month,Gbl.DateRange.DateIni.Year);
   fprintf (Gbl.F.Out,"<p class=\"DAT\" style=\"text-align:center;\">");
   if (NumDays == 1)
      fprintf (Gbl.F.Out,"%s: %s (%s)",Txt_Date,StrDateIni,Txt_one_day);
   else
     {
      sprintf (StrDateEnd,"%02u/%02u/%04u",Gbl.DateRange.DateEnd.Day,Gbl.DateRange.DateEnd.Month,Gbl.DateRange.DateEnd.Year);
      sprintf (StrDatesRange,Txt_DATES_RANGE,StrDateIni,StrDateEnd);
      fprintf (Gbl.F.Out,"%s: %s (%u %s)",
               Txt_Dates,StrDatesRange,NumDays,Txt_days);
     }
   fprintf (Gbl.F.Out,"</p>");
  }

/*****************************************************************************/
/******** Write a number in floating point with the correct accuracy *********/
/*****************************************************************************/

static void Sta_WriteFloatNum (float Number)
  {
   double IntegerPart,FractionaryPart;
   char *Format;

   FractionaryPart = modf ((double) Number,&IntegerPart);

   if (FractionaryPart == 0.0)
      fprintf (Gbl.F.Out,"%.0f",IntegerPart);
   else
     {
      if (IntegerPart != 0.0 || FractionaryPart >= 0.1)
         Format = "%.2f";
      else if (FractionaryPart >= 0.01)
         Format = "%.3f";
      else if (FractionaryPart >= 0.001)
         Format = "%.4f";
      else if (FractionaryPart >= 0.0001)
         Format = "%.5f";
      else if (FractionaryPart >= 0.00001)
         Format = "%.6f";
      else if (FractionaryPart >= 0.000001)
         Format = "%.7f";
      else
         Format = "%e";
      fprintf (Gbl.F.Out,Format,Number);
     }
  }

/*****************************************************************************/
/************************** Show use of the platform *************************/
/*****************************************************************************/

void Sta_ReqUseOfPlatform (void)
  {
   extern const char *The_ClassFormul[The_NUM_THEMES];
   extern const char *Txt_Scope;
   extern const char *Txt_Statistic;
   extern const char *Txt_STAT_USE_STAT_TYPES[Sta_NUM_TYPES_USE_STATS];
   extern const char *Txt_Show_statistic;
   Sta_UseStatType_t UseStatType;

   /***** Start form *****/
   fprintf (Gbl.F.Out,"<div style=\"padding-bottom:10px;"
	              " text-align:center;\">");
   Act_FormStart (ActSeeUseGbl);

   /***** Compute stats for anywhere, degree or course? *****/
   fprintf (Gbl.F.Out,"<div class=\"%s\">"
	              "%s: ",
            The_ClassFormul[Gbl.Prefs.Theme],Txt_Scope);
   Gbl.Scope.Allowed = 1 << Sco_SCOPE_PLATFORM    |
	               1 << Sco_SCOPE_COUNTRY     |
	               1 << Sco_SCOPE_INSTITUTION |
		       1 << Sco_SCOPE_CENTRE      |
		       1 << Sco_SCOPE_DEGREE      |
		       1 << Sco_SCOPE_COURSE;
   Gbl.Scope.Default = Sco_SCOPE_PLATFORM;
   Sco_GetScope ();
   Sco_PutSelectorScope (false);

   /***** Type of statistic *****/
   fprintf (Gbl.F.Out,"<br />"
	              "%s: <select name=\"UseStatType\">",
	    Txt_Statistic);
   for (UseStatType = (Sta_UseStatType_t) 0;
	UseStatType < Sta_NUM_TYPES_USE_STATS;
	UseStatType++)
     {
      fprintf (Gbl.F.Out,"<option value=\"%u\"",
               (unsigned) UseStatType);
      if (UseStatType == Gbl.Stat.UseStatType)
         fprintf (Gbl.F.Out," selected=\"selected\"");
      fprintf (Gbl.F.Out," />"
	                 "%s"
	                 "</option>",
               Txt_STAT_USE_STAT_TYPES[UseStatType]);
     }
   fprintf (Gbl.F.Out,"</select>"
                      "</div>");

   /***** Submit button *****/
   fprintf (Gbl.F.Out,"<input type=\"submit\" value=\"%s\" />",
            Txt_Show_statistic);

   /***** Form end *****/
   fprintf (Gbl.F.Out,"</form>"
	              "</div>");
  }

/*****************************************************************************/
/************************** Show use of the platform *************************/
/*****************************************************************************/

void Sta_ShowUseOfPlatform (void)
  {
   char UnsignedStr[10+1];
   unsigned UnsignedNum;

   /***** Get the type of stat of use ******/
   Par_GetParToText ("UseStatType",UnsignedStr,10);
   if (sscanf (UnsignedStr,"%u",&UnsignedNum) != 1)
      Lay_ShowErrorAndExit ("Type of stat is missing.");
   if (UnsignedNum >= Sta_NUM_TYPES_USE_STATS)
      Lay_ShowErrorAndExit ("Type of stat is missing.");
   Gbl.Stat.UseStatType = (Sta_UseStatType_t) UnsignedNum;

   /***** Show again the form to see use of the platform *****/
   Sta_ReqUseOfPlatform ();

   /***** Show the stat of use selected by user *****/
   switch (Gbl.Stat.UseStatType)
     {
      case Sta_DEGREES_AND_COURSES:
         /***** Number of degrees and courses *****/
	 Sta_GetAndShowDegCrsStats ();
         break;
      case Sta_USERS:
	 /***** Number of users *****/
         Sta_GetAndShowUsersStats ();
         break;
      case Sta_SOCIAL_NETWORKS:
	 /***** Number of users in social networks *****/
         Net_ShowWebAndSocialNetworksStats ();
         break;
      case Sta_FOLDERS_AND_FILES:
         /***** File browsers (folders and files) *****/
	 // TODO: add links to statistic
         Sta_GetAndShowFileBrowsersStats ();
         break;
      case Sta_OER:
	 /***** Number of Open Educational Resources (OERs) *****/
	 Sta_GetAndShowOERsStats ();
	 break;
      case Sta_ASSIGNMENTS:
         /***** Number of assignments *****/
         Sta_GetAndShowAssignmentsStats ();
         break;
      case Sta_TESTS:
         /***** Number of tests *****/
         Sta_GetAndShowTestsStats ();
         break;
      case Sta_NOTIFY_EVENTS:
         /***** Number of users who want to be notified by e-mail on each event *****/
         Sta_GetAndShowNumUsrsPerNotifyEvent ();
         break;
      case Sta_NOTICES:
         /***** Number of notices *****/
         Sta_GetAndShowNoticesStats ();
         break;
      case Sta_MSGS_BETWEEN_USERS:
         /***** Number of sent and received messages *****/
         Sta_GetAndShowMsgsStats ();
         break;
      case Sta_FORUMS:
         /***** Number of forums, threads and posts *****/
         Sta_GetAndShowForumStats ();
         break;
      case Sta_SURVEYS:
         /***** Number of surveys *****/
         Sta_GetAndShowSurveysStats ();
         break;
      case Sta_LANGUAGES:
         /***** Number of users who have chosen a language *****/
         Sta_GetAndShowNumUsrsPerLanguage ();
         break;
      case Sta_LAYOUTS:
         /***** Number of users who have chosen a layout *****/
         Sta_GetAndShowNumUsrsPerLayout ();
         break;
      case Sta_THEMES:
         /***** Number of users who have chosen a theme *****/
         Sta_GetAndShowNumUsrsPerTheme ();
         break;
      case Sta_ICON_SETS:
         /***** Number of users who have chosen an icon set *****/
         Sta_GetAndShowNumUsrsPerIconSet ();
         break;
      case Sta_MENUS:
         /***** Number of users who have chosen a menu *****/
         Sta_GetAndShowNumUsrsPerMenu ();
         break;
      case Sta_SIDE_COLUMNS:
         /***** Number of users who have chosen a layout of columns *****/
         Sta_GetAndShowNumUsrsPerSideColumns ();
         break;
     }
  }

/*****************************************************************************/
/*************** Get and show stats about degrees and courses ****************/
/*****************************************************************************/

static void Sta_GetAndShowDegCrsStats (void)
  {
   extern const char *Txt_STAT_USE_STAT_TYPES[Sta_NUM_TYPES_USE_STATS];

   Lay_StartRoundFrameTable10 (NULL,2,Txt_STAT_USE_STAT_TYPES[Sta_DEGREES_AND_COURSES]);
   Sta_WriteHeadDegsCrssInSWAD ();
   Sta_GetAndShowNumCtysInSWAD ();
   Sta_GetAndShowNumInssInSWAD ();
   Sta_GetAndShowNumCtrsInSWAD ();
   Sta_GetAndShowNumDegsInSWAD ();
   Sta_GetAndShowNumCrssInSWAD ();
   Lay_EndRoundFrameTable10 ();
  }

/*****************************************************************************/
/******************* Write head table degrees and courses ********************/
/*****************************************************************************/

static void Sta_WriteHeadDegsCrssInSWAD (void)
  {
   extern const char *Txt_Total;
   extern const char *Txt_With_institutions;
   extern const char *Txt_With_centres;
   extern const char *Txt_With_degrees;
   extern const char *Txt_With_courses;
   extern const char *Txt_With_teachers;
   extern const char *Txt_With_students;

   fprintf (Gbl.F.Out,"<tr>"
                      "<th></th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "</tr>",
            Txt_Total,
            Txt_With_institutions,
            Txt_With_centres,
            Txt_With_degrees,
            Txt_With_courses,
            Txt_With_teachers,
            Txt_With_students);
  }

/*****************************************************************************/
/******************* Get and show total number of countries ******************/
/*****************************************************************************/

static void Sta_GetAndShowNumCtysInSWAD (void)
  {
   extern const char *Txt_Countries;
   char SubQuery[128];
   unsigned NumCtysTotal = 0;
   unsigned NumCtysWithInss = 0;
   unsigned NumCtysWithCtrs = 0;
   unsigned NumCtysWithDegs = 0;
   unsigned NumCtysWithCrss = 0;
   unsigned NumCtysWithTchs = 0;
   unsigned NumCtysWithStds = 0;

   /***** Get number of countries *****/
   switch (Gbl.Scope.Current)
     {
      case Sco_SCOPE_PLATFORM:
	 NumCtysTotal = Cty_GetNumCtysTotal ();
         NumCtysWithInss = Cty_GetNumCtysWithInss ("");
	 NumCtysWithCtrs = Cty_GetNumCtysWithCtrs ("");
	 NumCtysWithDegs = Cty_GetNumCtysWithDegs ("");
	 NumCtysWithCrss = Cty_GetNumCtysWithCrss ("");
         NumCtysWithTchs = Cty_GetNumCtysWithUsrs (Rol_ROLE_TEACHER,"");
	 NumCtysWithStds = Cty_GetNumCtysWithUsrs (Rol_ROLE_STUDENT,"");
         SubQuery[0] = '\0';
         break;
      case Sco_SCOPE_INSTITUTION:
	 NumCtysTotal = 1;
	 NumCtysWithInss = 1;
         sprintf (SubQuery,"institutions.InsCod='%ld' AND ",
                  Gbl.CurrentIns.Ins.InsCod);
	 NumCtysWithCtrs = Cty_GetNumCtysWithCtrs (SubQuery);
	 NumCtysWithDegs = Cty_GetNumCtysWithDegs (SubQuery);
	 NumCtysWithCrss = Cty_GetNumCtysWithCrss (SubQuery);
         NumCtysWithTchs = Cty_GetNumCtysWithUsrs (Rol_ROLE_TEACHER,SubQuery);
	 NumCtysWithStds = Cty_GetNumCtysWithUsrs (Rol_ROLE_STUDENT,SubQuery);
         break;
      case Sco_SCOPE_CENTRE:
	 NumCtysTotal = 1;
	 NumCtysWithInss = 1;
	 NumCtysWithCtrs = 1;
         sprintf (SubQuery,"centres.CtrCod='%ld' AND ",
                  Gbl.CurrentCtr.Ctr.CtrCod);
	 NumCtysWithDegs = Cty_GetNumCtysWithDegs (SubQuery);
	 NumCtysWithCrss = Cty_GetNumCtysWithCrss (SubQuery);
         NumCtysWithTchs = Cty_GetNumCtysWithUsrs (Rol_ROLE_TEACHER,SubQuery);
	 NumCtysWithStds = Cty_GetNumCtysWithUsrs (Rol_ROLE_STUDENT,SubQuery);
	 break;
      case Sco_SCOPE_DEGREE:
	 NumCtysTotal = 1;
	 NumCtysWithInss = 1;
	 NumCtysWithCtrs = 1;
	 NumCtysWithDegs = 1;
         sprintf (SubQuery,"degrees.DegCod='%ld' AND ",
                  Gbl.CurrentDeg.Deg.DegCod);
	 NumCtysWithCrss = Cty_GetNumCtysWithCrss (SubQuery);
         NumCtysWithTchs = Cty_GetNumCtysWithUsrs (Rol_ROLE_TEACHER,SubQuery);
	 NumCtysWithStds = Cty_GetNumCtysWithUsrs (Rol_ROLE_STUDENT,SubQuery);
	 break;
     case Sco_SCOPE_COURSE:
	 NumCtysTotal = 1;
	 NumCtysWithInss = 1;
	 NumCtysWithCtrs = 1;
	 NumCtysWithDegs = 1;
	 NumCtysWithCrss = 1;
         sprintf (SubQuery,"courses.CrsCod='%ld' AND ",
                  Gbl.CurrentCrs.Crs.CrsCod);
         NumCtysWithTchs = Cty_GetNumCtysWithUsrs (Rol_ROLE_TEACHER,SubQuery);
	 NumCtysWithStds = Cty_GetNumCtysWithUsrs (Rol_ROLE_STUDENT,SubQuery);
	 break;
      default:
	 Lay_ShowErrorAndExit ("Wrong scope.");
	 break;
     }

   /***** Write number of countries *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"TIT_TBL\" style=\"text-align:left;\">"
                      "%s:"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "</tr>",
            Txt_Countries,
            NumCtysTotal,
            NumCtysWithInss,
            NumCtysWithCtrs,
            NumCtysWithDegs,
            NumCtysWithCrss,
            NumCtysWithTchs,
            NumCtysWithStds);
  }

/*****************************************************************************/
/***************** Get and show total number of institutions *****************/
/*****************************************************************************/

static void Sta_GetAndShowNumInssInSWAD (void)
  {
   extern const char *Txt_Institutions;
   char SubQuery[128];
   unsigned NumInssTotal = 0;
   unsigned NumInssWithCtrs = 0;
   unsigned NumInssWithDegs = 0;
   unsigned NumInssWithCrss = 0;
   unsigned NumInssWithTchs = 0;
   unsigned NumInssWithStds = 0;

   /***** Get number of institutions *****/
   switch (Gbl.Scope.Current)
     {
      case Sco_SCOPE_PLATFORM:
	 NumInssTotal = Ins_GetNumInssTotal ();
	 NumInssWithCtrs = Ins_GetNumInssWithCtrs ("");
	 NumInssWithDegs = Ins_GetNumInssWithDegs ("");
	 NumInssWithCrss = Ins_GetNumInssWithCrss ("");
         NumInssWithTchs = Ins_GetNumInssWithUsrs (Rol_ROLE_TEACHER,"");
	 NumInssWithStds = Ins_GetNumInssWithUsrs (Rol_ROLE_STUDENT,"");
         SubQuery[0] = '\0';
         break;
      case Sco_SCOPE_INSTITUTION:
	 NumInssTotal = 1;
         sprintf (SubQuery,"institutions.InsCod='%ld' AND ",
                  Gbl.CurrentIns.Ins.InsCod);
	 NumInssWithCtrs = Ins_GetNumInssWithCtrs (SubQuery);
	 NumInssWithDegs = Ins_GetNumInssWithDegs (SubQuery);
	 NumInssWithCrss = Ins_GetNumInssWithCrss (SubQuery);
         NumInssWithTchs = Ins_GetNumInssWithUsrs (Rol_ROLE_TEACHER,SubQuery);
	 NumInssWithStds = Ins_GetNumInssWithUsrs (Rol_ROLE_STUDENT,SubQuery);
         break;
      case Sco_SCOPE_CENTRE:
	 NumInssTotal = 1;
	 NumInssWithCtrs = 1;
         sprintf (SubQuery,"centres.CtrCod='%ld' AND ",
                  Gbl.CurrentCtr.Ctr.CtrCod);
	 NumInssWithDegs = Ins_GetNumInssWithDegs (SubQuery);
	 NumInssWithCrss = Ins_GetNumInssWithCrss (SubQuery);
         NumInssWithTchs = Ins_GetNumInssWithUsrs (Rol_ROLE_TEACHER,SubQuery);
	 NumInssWithStds = Ins_GetNumInssWithUsrs (Rol_ROLE_STUDENT,SubQuery);
	 break;
      case Sco_SCOPE_DEGREE:
	 NumInssTotal = 1;
	 NumInssWithCtrs = 1;
	 NumInssWithDegs = 1;
         sprintf (SubQuery,"degrees.DegCod='%ld' AND ",
                  Gbl.CurrentDeg.Deg.DegCod);
	 NumInssWithCrss = Ins_GetNumInssWithCrss (SubQuery);
         NumInssWithTchs = Ins_GetNumInssWithUsrs (Rol_ROLE_TEACHER,SubQuery);
	 NumInssWithStds = Ins_GetNumInssWithUsrs (Rol_ROLE_STUDENT,SubQuery);
	 break;
     case Sco_SCOPE_COURSE:
	 NumInssTotal = 1;
	 NumInssWithCtrs = 1;
	 NumInssWithDegs = 1;
	 NumInssWithCrss = 1;
         sprintf (SubQuery,"courses.CrsCod='%ld' AND ",
                  Gbl.CurrentCrs.Crs.CrsCod);
         NumInssWithTchs = Ins_GetNumInssWithUsrs (Rol_ROLE_TEACHER,SubQuery);
	 NumInssWithStds = Ins_GetNumInssWithUsrs (Rol_ROLE_STUDENT,SubQuery);
	 break;
      default:
	 Lay_ShowErrorAndExit ("Wrong scope.");
	 break;
     }

   /***** Write number of institutions *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"TIT_TBL\" style=\"text-align:left;\">"
                      "%s:"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td></td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "</tr>",
            Txt_Institutions,
            NumInssTotal,
            NumInssWithCtrs,
            NumInssWithDegs,
            NumInssWithCrss,
            NumInssWithTchs,
            NumInssWithStds);
  }

/*****************************************************************************/
/********************* Get and show total number of centres ******************/
/*****************************************************************************/

static void Sta_GetAndShowNumCtrsInSWAD (void)
  {
   extern const char *Txt_Centres;
   char SubQuery[128];
   unsigned NumCtrsTotal = 0;
   unsigned NumCtrsWithDegs = 0;
   unsigned NumCtrsWithCrss = 0;
   unsigned NumCtrsWithTchs = 0;
   unsigned NumCtrsWithStds = 0;

   /***** Get number of centres *****/
   switch (Gbl.Scope.Current)
     {
      case Sco_SCOPE_PLATFORM:
	 NumCtrsTotal = Ctr_GetNumCtrsTotal ();
	 NumCtrsWithDegs = Ctr_GetNumCtrsWithDegs ("");
	 NumCtrsWithCrss = Ctr_GetNumCtrsWithCrss ("");
         NumCtrsWithTchs = Ctr_GetNumCtrsWithUsrs (Rol_ROLE_TEACHER,"");
	 NumCtrsWithStds = Ctr_GetNumCtrsWithUsrs (Rol_ROLE_STUDENT,"");
         SubQuery[0] = '\0';
         break;
      case Sco_SCOPE_INSTITUTION:
	 NumCtrsTotal = Ctr_GetNumCtrsInIns (Gbl.CurrentIns.Ins.InsCod);
         sprintf (SubQuery,"institutions.InsCod='%ld' AND ",
                  Gbl.CurrentIns.Ins.InsCod);
	 NumCtrsWithDegs = Ctr_GetNumCtrsWithDegs (SubQuery);
	 NumCtrsWithCrss = Ctr_GetNumCtrsWithCrss (SubQuery);
         NumCtrsWithTchs = Ctr_GetNumCtrsWithUsrs (Rol_ROLE_TEACHER,SubQuery);
	 NumCtrsWithStds = Ctr_GetNumCtrsWithUsrs (Rol_ROLE_STUDENT,SubQuery);
         break;
      case Sco_SCOPE_CENTRE:
	 NumCtrsTotal = 1;
         sprintf (SubQuery,"centres.CtrCod='%ld' AND ",
                  Gbl.CurrentCtr.Ctr.CtrCod);
	 NumCtrsWithDegs = Ctr_GetNumCtrsWithDegs (SubQuery);
	 NumCtrsWithCrss = Ctr_GetNumCtrsWithCrss (SubQuery);
         NumCtrsWithTchs = Ctr_GetNumCtrsWithUsrs (Rol_ROLE_TEACHER,SubQuery);
	 NumCtrsWithStds = Ctr_GetNumCtrsWithUsrs (Rol_ROLE_STUDENT,SubQuery);
	 break;
      case Sco_SCOPE_DEGREE:
	 NumCtrsTotal = 1;
	 NumCtrsWithDegs = 1;
         sprintf (SubQuery,"degrees.DegCod='%ld' AND ",
                  Gbl.CurrentDeg.Deg.DegCod);
	 NumCtrsWithCrss = Ctr_GetNumCtrsWithCrss (SubQuery);
         NumCtrsWithTchs = Ctr_GetNumCtrsWithUsrs (Rol_ROLE_TEACHER,SubQuery);
	 NumCtrsWithStds = Ctr_GetNumCtrsWithUsrs (Rol_ROLE_STUDENT,SubQuery);
	 break;
     case Sco_SCOPE_COURSE:
	 NumCtrsTotal = 1;
	 NumCtrsWithDegs = 1;
	 NumCtrsWithCrss = 1;
         sprintf (SubQuery,"courses.CrsCod='%ld' AND ",
                  Gbl.CurrentCrs.Crs.CrsCod);
         NumCtrsWithTchs = Ctr_GetNumCtrsWithUsrs (Rol_ROLE_TEACHER,SubQuery);
	 NumCtrsWithStds = Ctr_GetNumCtrsWithUsrs (Rol_ROLE_STUDENT,SubQuery);
	 break;
      default:
	 Lay_ShowErrorAndExit ("Wrong scope.");
	 break;
     }

   /***** Write number of centres *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"TIT_TBL\" style=\"text-align:left;\">"
                      "%s:"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td></td>"
                      "<td></td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "</tr>",
            Txt_Centres,
            NumCtrsTotal,
            NumCtrsWithDegs,
            NumCtrsWithCrss,
            NumCtrsWithTchs,
            NumCtrsWithStds);
  }

/*****************************************************************************/
/********************* Get and show total number of degrees ******************/
/*****************************************************************************/

static void Sta_GetAndShowNumDegsInSWAD (void)
  {
   extern const char *Txt_Degrees;
   char SubQuery[128];
   unsigned NumDegsTotal = 0;
   unsigned NumDegsWithCrss = 0;
   unsigned NumDegsWithTchs = 0;
   unsigned NumDegsWithStds = 0;

   /***** Get number of degrees *****/
   switch (Gbl.Scope.Current)
     {
      case Sco_SCOPE_PLATFORM:
	 NumDegsTotal = Deg_GetNumDegsTotal ();
	 NumDegsWithCrss = Deg_GetNumDegsWithCrss ("");
         NumDegsWithTchs = Deg_GetNumDegsWithUsrs (Rol_ROLE_TEACHER,"");
	 NumDegsWithStds = Deg_GetNumDegsWithUsrs (Rol_ROLE_STUDENT,"");
         SubQuery[0] = '\0';
         break;
      case Sco_SCOPE_INSTITUTION:
	 NumDegsTotal = Deg_GetNumDegsInIns (Gbl.CurrentIns.Ins.InsCod);
         sprintf (SubQuery,"institutions.InsCod='%ld' AND ",
                  Gbl.CurrentIns.Ins.InsCod);
	 NumDegsWithCrss = Deg_GetNumDegsWithCrss (SubQuery);
         NumDegsWithTchs = Deg_GetNumDegsWithUsrs (Rol_ROLE_TEACHER,SubQuery);
	 NumDegsWithStds = Deg_GetNumDegsWithUsrs (Rol_ROLE_STUDENT,SubQuery);
         break;
      case Sco_SCOPE_CENTRE:
	 NumDegsTotal = Deg_GetNumDegsInCtr (Gbl.CurrentCtr.Ctr.CtrCod);
         sprintf (SubQuery,"centres.CtrCod='%ld' AND ",
                  Gbl.CurrentCtr.Ctr.CtrCod);
	 NumDegsWithCrss = Deg_GetNumDegsWithCrss (SubQuery);
         NumDegsWithTchs = Deg_GetNumDegsWithUsrs (Rol_ROLE_TEACHER,SubQuery);
	 NumDegsWithStds = Deg_GetNumDegsWithUsrs (Rol_ROLE_STUDENT,SubQuery);
	 break;
      case Sco_SCOPE_DEGREE:
	 NumDegsTotal = 1;
         sprintf (SubQuery,"degrees.DegCod='%ld' AND ",
                  Gbl.CurrentDeg.Deg.DegCod);
	 NumDegsWithCrss = Deg_GetNumDegsWithCrss (SubQuery);
         NumDegsWithTchs = Deg_GetNumDegsWithUsrs (Rol_ROLE_TEACHER,SubQuery);
	 NumDegsWithStds = Deg_GetNumDegsWithUsrs (Rol_ROLE_STUDENT,SubQuery);
	 break;
     case Sco_SCOPE_COURSE:
	 NumDegsTotal = 1;
	 NumDegsWithCrss = 1;
         sprintf (SubQuery,"courses.CrsCod='%ld' AND ",
                  Gbl.CurrentCrs.Crs.CrsCod);
         NumDegsWithTchs = Deg_GetNumDegsWithUsrs (Rol_ROLE_TEACHER,SubQuery);
	 NumDegsWithStds = Deg_GetNumDegsWithUsrs (Rol_ROLE_STUDENT,SubQuery);
	 break;
      default:
	 Lay_ShowErrorAndExit ("Wrong scope.");
	 break;
     }

   /***** Write number of degrees *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"TIT_TBL\" style=\"text-align:left;\">"
                      "%s:"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td></td>"
                      "<td></td>"
                      "<td></td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "</tr>",
            Txt_Degrees,
            NumDegsTotal,
            NumDegsWithCrss,
            NumDegsWithTchs,
            NumDegsWithStds);
  }

/*****************************************************************************/
/****************** Get and show total number of courses *********************/
/*****************************************************************************/

static void Sta_GetAndShowNumCrssInSWAD (void)
  {
   extern const char *Txt_Courses;
   char SubQuery[128];
   unsigned NumCrssTotal = 0;
   unsigned NumCrssWithTchs = 0;
   unsigned NumCrssWithStds = 0;

   /***** Get number of degrees *****/
   switch (Gbl.Scope.Current)
     {
      case Sco_SCOPE_PLATFORM:
	 NumCrssTotal = Crs_GetNumCrssTotal ();
         NumCrssWithTchs = Crs_GetNumCrssWithUsrs (Rol_ROLE_TEACHER,"");
	 NumCrssWithStds = Crs_GetNumCrssWithUsrs (Rol_ROLE_STUDENT,"");
         SubQuery[0] = '\0';
         break;
      case Sco_SCOPE_INSTITUTION:
	 NumCrssTotal = Crs_GetNumCrssInIns (Gbl.CurrentIns.Ins.InsCod);
         sprintf (SubQuery,"institutions.InsCod='%ld' AND ",
                  Gbl.CurrentIns.Ins.InsCod);
         NumCrssWithTchs = Crs_GetNumCrssWithUsrs (Rol_ROLE_TEACHER,SubQuery);
	 NumCrssWithStds = Crs_GetNumCrssWithUsrs (Rol_ROLE_STUDENT,SubQuery);
         break;
      case Sco_SCOPE_CENTRE:
	 NumCrssTotal = Crs_GetNumCrssInCtr (Gbl.CurrentCtr.Ctr.CtrCod);
         sprintf (SubQuery,"centres.CtrCod='%ld' AND ",
                  Gbl.CurrentCtr.Ctr.CtrCod);
         NumCrssWithTchs = Crs_GetNumCrssWithUsrs (Rol_ROLE_TEACHER,SubQuery);
	 NumCrssWithStds = Crs_GetNumCrssWithUsrs (Rol_ROLE_STUDENT,SubQuery);
	 break;
      case Sco_SCOPE_DEGREE:
	 NumCrssTotal = Crs_GetNumCrssInDeg (Gbl.CurrentDeg.Deg.DegCod);
         sprintf (SubQuery,"degrees.DegCod='%ld' AND ",
                  Gbl.CurrentDeg.Deg.DegCod);
         NumCrssWithTchs = Crs_GetNumCrssWithUsrs (Rol_ROLE_TEACHER,SubQuery);
	 NumCrssWithStds = Crs_GetNumCrssWithUsrs (Rol_ROLE_STUDENT,SubQuery);
	 break;
     case Sco_SCOPE_COURSE:
	 NumCrssTotal = 1;
         sprintf (SubQuery,"courses.CrsCod='%ld' AND ",
                  Gbl.CurrentCrs.Crs.CrsCod);
         NumCrssWithTchs = Crs_GetNumCrssWithUsrs (Rol_ROLE_TEACHER,SubQuery);
	 NumCrssWithStds = Crs_GetNumCrssWithUsrs (Rol_ROLE_STUDENT,SubQuery);
	 break;
      default:
	 Lay_ShowErrorAndExit ("Wrong scope.");
	 break;
     }

   /***** Write number of degrees *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"TIT_TBL\" style=\"text-align:left;\">"
                      "%s:"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td></td>"
                      "<td></td>"
                      "<td></td>"
                      "<td></td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "</tr>",
            Txt_Courses,
            NumCrssTotal,
            NumCrssWithTchs,
            NumCrssWithStds);
  }

/*****************************************************************************/
/************************* Get total number of users *************************/
/*****************************************************************************/

unsigned Sta_GetTotalNumberOfUsers (Sco_Scope_t Scope,Rol_Role_t Role)
  {
   char Query[256];

   /***** Get number of users from database *****/
   switch (Scope)
     {
      case Sco_SCOPE_PLATFORM:
         if (Role == Rol_ROLE_UNKNOWN)	// Here Rol_ROLE_UNKNOWN means "all users"
            sprintf (Query,"SELECT COUNT(*) FROM usr_data");
         else
            sprintf (Query,"SELECT COUNT(DISTINCT UsrCod)"
        	           " FROM crs_usr WHERE Role='%u'",
                     (unsigned) Role);
         break;
      case Sco_SCOPE_INSTITUTION:
         if (Role == Rol_ROLE_UNKNOWN)	// Here Rol_ROLE_UNKNOWN means "all users"
            sprintf (Query,"SELECT COUNT(DISTINCT crs_usr.UsrCod)"
        	           " FROM centres,degrees,courses,crs_usr"
                           " WHERE centres.InsCod='%ld'"
                           " AND centres.CtrCod=degrees.CtrCod"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=crs_usr.CrsCod",
                     Gbl.CurrentIns.Ins.InsCod);
         else
            sprintf (Query,"SELECT COUNT(DISTINCT crs_usr.UsrCod)"
        	           " FROM centres,degrees,courses,crs_usr"
                           " WHERE centres.InsCod='%ld'"
                           " AND centres.CtrCod=degrees.CtrCod"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.Role='%u'",
                     Gbl.CurrentIns.Ins.InsCod,(unsigned) Role);
         break;
      case Sco_SCOPE_CENTRE:
         if (Role == Rol_ROLE_UNKNOWN)	// Here Rol_ROLE_UNKNOWN means "all users"
            sprintf (Query,"SELECT COUNT(DISTINCT crs_usr.UsrCod)"
        	           " FROM degrees,courses,crs_usr"
                           " WHERE degrees.CtrCod='%ld'"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=crs_usr.CrsCod",
                     Gbl.CurrentCtr.Ctr.CtrCod);
         else
            sprintf (Query,"SELECT COUNT(DISTINCT crs_usr.UsrCod)"
        	           " FROM degrees,courses,crs_usr"
                           " WHERE degrees.CtrCod='%ld'"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.Role='%u'",
                     Gbl.CurrentCtr.Ctr.CtrCod,(unsigned) Role);
         break;
      case Sco_SCOPE_DEGREE:
         if (Role == Rol_ROLE_UNKNOWN)	// Here Rol_ROLE_UNKNOWN means "all users"
            sprintf (Query,"SELECT COUNT(DISTINCT crs_usr.UsrCod)"
        	           " FROM courses,crs_usr"
                           " WHERE courses.DegCod='%ld'"
                           " AND courses.CrsCod=crs_usr.CrsCod",
                     Gbl.CurrentDeg.Deg.DegCod);
         else
            sprintf (Query,"SELECT COUNT(DISTINCT crs_usr.UsrCod)"
        	            " FROM courses,crs_usr"
                           " WHERE courses.DegCod='%ld'"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.Role='%u'",
                     Gbl.CurrentDeg.Deg.DegCod,(unsigned) Role);
         break;
      case Sco_SCOPE_COURSE:
         if (Role == Rol_ROLE_UNKNOWN)	// Here Rol_ROLE_UNKNOWN means "all users"
            sprintf (Query,"SELECT COUNT(DISTINCT UsrCod) FROM crs_usr"
                           " WHERE CrsCod='%ld'",
                     Gbl.CurrentCrs.Crs.CrsCod);
         else
            sprintf (Query,"SELECT COUNT(DISTINCT UsrCod) FROM crs_usr"
                           " WHERE CrsCod='%ld'"
                           " AND crs_usr.Role='%u'",
                     Gbl.CurrentCrs.Crs.CrsCod,(unsigned) Role);
         break;
      default:
	 Lay_ShowErrorAndExit ("Wrong scope.");
	 break;
     }
   return (unsigned) DB_QueryCOUNT (Query,"can not get number of users");
  }

/*****************************************************************************/
/********************** Show stats about number of users *********************/
/*****************************************************************************/

static void Sta_GetAndShowUsersStats (void)
  {
   extern const char *Txt_STAT_USE_STAT_TYPES[Sta_NUM_TYPES_USE_STATS];
   extern const char *Txt_Users;
   extern const char *Txt_No_of_users;
   extern const char *Txt_Average_number_of_courses_to_which_a_user_belongs;
   extern const char *Txt_Average_number_of_users_belonging_to_a_course;

   /***** Number of users *****/
   Lay_StartRoundFrameTable10 (NULL,2,Txt_STAT_USE_STAT_TYPES[Sta_USERS]);

   fprintf (Gbl.F.Out,"<tr>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "</tr>",
            Txt_Users,
            Txt_No_of_users,
            Txt_Average_number_of_courses_to_which_a_user_belongs,
            Txt_Average_number_of_users_belonging_to_a_course);
   Usr_GetAndShowNumUsrsInPlatform (Rol_ROLE_STUDENT);
   Usr_GetAndShowNumUsrsInPlatform (Rol_ROLE_TEACHER);
   Usr_GetAndShowNumUsrsInPlatform (Rol_ROLE_GUEST);	// Users not beloging to any course

   Lay_EndRoundFrameTable10 ();
  }

/*****************************************************************************/
/********************* Show stats about exploration trees ********************/
/*****************************************************************************/
// TODO: add links to statistic

static void Sta_GetAndShowFileBrowsersStats (void)
  {
   extern const char *Txt_STAT_USE_STAT_TYPES[Sta_NUM_TYPES_USE_STATS];
   extern const char *Txt_STAT_COURSE_FILE_ZONES[];
   extern const char *Txt_Virtual_pendrives;
   static const Brw_FileBrowser_t StatCrsFileZones[Sta_NUM_STAT_CRS_FILE_ZONES] =
     {
      Brw_ADMI_DOCUM_CRS,
      Brw_ADMI_DOCUM_GRP,
      Brw_ADMI_SHARE_CRS,
      Brw_ADMI_SHARE_GRP,
      Brw_ADMI_MARKS_CRS,
      Brw_ADMI_MARKS_GRP,
      Brw_ADMI_ASSIG_USR,
      Brw_ADMI_WORKS_USR,
      Brw_UNKNOWN,
     };
   unsigned NumStat;

   /***** Table start *****/
   Lay_StartRoundFrameTable10 (NULL,2,Txt_STAT_USE_STAT_TYPES[Sta_FOLDERS_AND_FILES]);

   /***** Write table heading *****/
   Sta_WriteStatsExpTreesTableHead ();

   /***** Write sizes of course file zones *****/
   for (NumStat = 0;
	NumStat < Sta_NUM_STAT_CRS_FILE_ZONES;
	NumStat++)
      Sta_WriteRowStatsFileBrowsers (StatCrsFileZones[NumStat],Txt_STAT_COURSE_FILE_ZONES[NumStat]);

   /***** Write table heading *****/
   Sta_WriteStatsExpTreesTableHead ();

   /***** Write size of briefcases *****/
   Sta_WriteRowStatsFileBrowsers (Brw_ADMI_BRIEF_USR,Txt_Virtual_pendrives);

   /***** End table *****/
   Lay_EndRoundFrameTable10 ();
  }

/*****************************************************************************/
/*********** Write table heading for stats of exploration trees **************/
/*****************************************************************************/

static void Sta_WriteStatsExpTreesTableHead (void)
  {
   extern const char *Txt_File_zones;
   extern const char *Txt_Courses;
   extern const char *Txt_Groups;
   extern const char *Txt_Users;
   extern const char *Txt_Max_levels;
   extern const char *Txt_Folders;
   extern const char *Txt_Files;
   extern const char *Txt_Size;
   extern const char *Txt_STAT_COURSE_FILE_ZONES[];
   extern const char *Txt_crs;
   extern const char *Txt_usr;

   fprintf (Gbl.F.Out,"<tr>"
                      "<th class=\"TIT_TBL\" style=\"text-align:left;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s/<br />%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s/<br />%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s/<br />%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s/<br />%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s/<br />%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s/<br />%s"
                      "</th>"
                      "</tr>",
            Txt_File_zones,
            Txt_Courses,
            Txt_Groups,
            Txt_Users,
            Txt_Max_levels,
            Txt_Folders,
            Txt_Files,
            Txt_Size,
            Txt_Folders,Txt_crs,
            Txt_Files,Txt_crs,
            Txt_Size,Txt_crs,
            Txt_Folders,Txt_usr,
            Txt_Files,Txt_usr,
            Txt_Size,Txt_usr);
  }

/*****************************************************************************/
/*************** Write a row of stats of exploration trees *******************/
/*****************************************************************************/

static void Sta_WriteRowStatsFileBrowsers (Brw_FileBrowser_t FileZone,const char *NameOfFileZones)
  {
   char StrNumCrss[10+1];
   char StrNumGrps[10+1];
   char StrNumUsrs[10+1];
   char StrNumFoldersPerCrs[10+1];
   char StrNumFoldersPerUsr[10+1];
   char StrNumFilesPerCrs[10+1];
   char StrNumFilesPerUsr[10+1];
   struct Sta_SizeOfFileZones SizeOfFileZones;
   char *ClassData = (FileZone == Brw_UNKNOWN) ? "DAT_N" :
	                                         "DAT";
   char *StyleTableCell = (FileZone == Brw_UNKNOWN) ? " border-style:solid none none none;"
	                                              " border-width:1px;" :
	                                              "";

   Sta_GetSizeOfFileZoneFromDB (Gbl.Scope.Current,FileZone,&SizeOfFileZones);
   if (SizeOfFileZones.NumCrss == -1)
     {
      strcpy (StrNumCrss         ,"-");
      strcpy (StrNumFoldersPerCrs,"-");
      strcpy (StrNumFilesPerCrs  ,"-");
     }
   else
     {
      sprintf (StrNumCrss,"%d",SizeOfFileZones.NumCrss);
      sprintf (StrNumFoldersPerCrs,"%.1f",
               SizeOfFileZones.NumCrss ? (double) SizeOfFileZones.NumFolders /
        	                         (double) SizeOfFileZones.NumCrss :
        	                         0.0);
      sprintf (StrNumFilesPerCrs,"%.1f",
               SizeOfFileZones.NumCrss ? (double) SizeOfFileZones.NumFiles /
        	                         (double) SizeOfFileZones.NumCrss :
        	                         0.0);
     }

   if (SizeOfFileZones.NumGrps == -1)
      strcpy (StrNumGrps,"-");
   else
      sprintf (StrNumGrps,"%d",SizeOfFileZones.NumGrps);

   if (SizeOfFileZones.NumUsrs == -1)
     {
      strcpy (StrNumUsrs         ,"-");
      strcpy (StrNumFoldersPerUsr,"-");
      strcpy (StrNumFilesPerUsr  ,"-");
     }
   else
     {
      sprintf (StrNumUsrs,"%d",SizeOfFileZones.NumUsrs);
      sprintf (StrNumFoldersPerUsr,"%.1f",
               SizeOfFileZones.NumUsrs ? (double) SizeOfFileZones.NumFolders /
        	                         (double) SizeOfFileZones.NumUsrs :
        	                         0.0);
      sprintf (StrNumFilesPerUsr,"%.1f",
               SizeOfFileZones.NumUsrs ? (double) SizeOfFileZones.NumFiles /
        	                         (double) SizeOfFileZones.NumUsrs :
        	                         0.0);
     }
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"%s\" style=\"text-align:left; %s\">"
                      "%s"
                      "</td>"
                      "<td class=\"%s\" style=\"text-align:right; %s\">"
                      "%s"
                      "</td>"
                      "<td class=\"%s\" style=\"text-align:right; %s\">"
                      "%s"
                      "</td>"
                      "<td class=\"%s\" style=\"text-align:right; %s\">"
                      "%s"
                      "</td>"
                      "<td class=\"%s\" style=\"text-align:right; %s\">"
                      "%u"
                      "</td>"
                      "<td class=\"%s\" style=\"text-align:right; %s\">"
                      "%lu"
                      "</td>"
                      "<td class=\"%s\" style=\"text-align:right; %s\">"
                      "%lu"
                      "</td>"
                      "<td class=\"%s\" style=\"text-align:right; %s\">",
            ClassData,StyleTableCell,NameOfFileZones,
            ClassData,StyleTableCell,StrNumCrss,
            ClassData,StyleTableCell,StrNumGrps,
            ClassData,StyleTableCell,StrNumUsrs,
            ClassData,StyleTableCell,SizeOfFileZones.MaxLevels,
            ClassData,StyleTableCell,SizeOfFileZones.NumFolders,
            ClassData,StyleTableCell,SizeOfFileZones.NumFiles,
            ClassData,StyleTableCell);
   Str_WriteSizeInBytesFull ((double) SizeOfFileZones.Size);
   fprintf (Gbl.F.Out,"</td>"
                      "<td class=\"%s\" style=\"text-align:right; %s\">"
                      "%s"
                      "</td>"
                      "<td class=\"%s\" style=\"text-align:right; %s\">"
                      "%s"
                      "</td>"
                      "<td class=\"%s\" style=\"text-align:right; %s\">",
            ClassData,StyleTableCell,StrNumFoldersPerCrs,
            ClassData,StyleTableCell,StrNumFilesPerCrs,
            ClassData,StyleTableCell);
   if (SizeOfFileZones.NumCrss == -1)
      fprintf (Gbl.F.Out,"-");
   else
      Str_WriteSizeInBytesFull (SizeOfFileZones.NumCrss ? (double) SizeOfFileZones.Size /
	                                                  (double) SizeOfFileZones.NumCrss :
	                                                  0.0);
   fprintf (Gbl.F.Out,"</td>"
                      "<td class=\"%s\" style=\"text-align:right; %s\">"
                      "%s"
                      "</td>"
                      "<td class=\"%s\" style=\"text-align:right; %s\">"
                      "%s"
                      "</td>"
                      "<td class=\"%s\" style=\"text-align:right; %s\">",
            ClassData,StyleTableCell,StrNumFoldersPerUsr,
            ClassData,StyleTableCell,StrNumFilesPerUsr,
            ClassData,StyleTableCell);
   if (SizeOfFileZones.NumUsrs == -1)
      fprintf (Gbl.F.Out,"-");
   else
      Str_WriteSizeInBytesFull (SizeOfFileZones.NumUsrs ? (double) SizeOfFileZones.Size /
	                                                  (double) SizeOfFileZones.NumUsrs :
	                                                  0.0);
   fprintf (Gbl.F.Out,"</td>"
	              "</tr>");
  }

/*****************************************************************************/
/**************** Get the size of a file zone from database ******************/
/*****************************************************************************/

static void Sta_GetSizeOfFileZoneFromDB (Sco_Scope_t Scope,
                                         Brw_FileBrowser_t FileBrowser,
                                         struct Sta_SizeOfFileZones *SizeOfFileZones)
  {
   char Query[2048];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;

   /***** Get the size of a file browser *****/
   switch (Scope)
     {
      /* Scope = the whole platform */
      case Sco_SCOPE_PLATFORM:
	 switch (FileBrowser)
	   {
	    case Brw_UNKNOWN:
	       sprintf (Query,"SELECT COUNT(DISTINCT CrsCod),COUNT(DISTINCT GrpCod)-1,'-1',"
			      "MAX(NumLevels),SUM(NumFolders),SUM(NumFiles),SUM(TotalSize)"
			      " FROM "
	                      "("
	                      "SELECT Cod AS CrsCod,'-1' AS GrpCod,"
			      "NumLevels,NumFolders,NumFiles,TotalSize"
			      " FROM file_browser_size"
			      " WHERE FileBrowser IN ('%u','%u','%u','%u','%u')"
	                      " UNION "
	                      "SELECT crs_grp_types.CrsCod,file_browser_size.Cod AS GrpCod,"
			      "file_browser_size.NumLevels,file_browser_size.NumFolders,file_browser_size.NumFiles,file_browser_size.TotalSize"
			      " FROM crs_grp_types,crs_grp,file_browser_size"
			      " WHERE crs_grp_types.GrpTypCod=crs_grp.GrpTypCod"
			      " AND crs_grp.GrpCod=file_browser_size.Cod"
			      " AND file_browser_size.FileBrowser IN ('%u','%u','%u')"
			      ") AS sizes",
			(unsigned) Brw_ADMI_DOCUM_CRS,
			(unsigned) Brw_ADMI_SHARE_CRS,
			(unsigned) Brw_ADMI_ASSIG_USR,
			(unsigned) Brw_ADMI_WORKS_USR,
			(unsigned) Brw_ADMI_MARKS_CRS,
			(unsigned) Brw_ADMI_DOCUM_GRP,
			(unsigned) Brw_ADMI_SHARE_GRP,
			(unsigned) Brw_ADMI_MARKS_GRP);
	       break;
	    case Brw_ADMI_DOCUM_CRS:
	    case Brw_ADMI_SHARE_CRS:
	    case Brw_ADMI_MARKS_CRS:
	       sprintf (Query,"SELECT COUNT(DISTINCT Cod),'-1','-1',"
			      "MAX(NumLevels),SUM(NumFolders),SUM(NumFiles),SUM(TotalSize)"
			      " FROM file_browser_size"
			      " WHERE FileBrowser='%u'",
			(unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_DOCUM_GRP:
	    case Brw_ADMI_SHARE_GRP:
	    case Brw_ADMI_MARKS_GRP:
	       sprintf (Query,"SELECT COUNT(DISTINCT crs_grp_types.CrsCod),COUNT(DISTINCT file_browser_size.Cod),'-1',"
			      "MAX(file_browser_size.NumLevels),SUM(file_browser_size.NumFolders),SUM(file_browser_size.NumFiles),SUM(file_browser_size.TotalSize)"
			      " FROM crs_grp_types,crs_grp,file_browser_size"
			      " WHERE crs_grp_types.GrpTypCod=crs_grp.GrpTypCod"
			      " AND crs_grp.GrpCod=file_browser_size.Cod"
	                      " AND file_browser_size.FileBrowser='%u'",
			(unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_ASSIG_USR:
	    case Brw_ADMI_WORKS_USR:
	       sprintf (Query,"SELECT COUNT(DISTINCT Cod),'-1',COUNT(DISTINCT ZoneUsrCod),"
			      "MAX(NumLevels),SUM(NumFolders),SUM(NumFiles),SUM(TotalSize)"
			      " FROM file_browser_size"
			      " WHERE FileBrowser='%u'",
			(unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_BRIEF_USR:
	       sprintf (Query,"SELECT '-1','-1',COUNT(DISTINCT ZoneUsrCod),"
			      "MAX(NumLevels),SUM(NumFolders),SUM(NumFiles),SUM(TotalSize)"
			      " FROM file_browser_size"
			      " WHERE FileBrowser='%u'",
			(unsigned) FileBrowser);
	       break;
	    default:
	       Lay_ShowErrorAndExit ("Wrong file browser.");
	       break;
	   }
         break;
      /* Scope = the current country */
      case Sco_SCOPE_COUNTRY:
	 switch (FileBrowser)
	   {
	    case Brw_UNKNOWN:
	       sprintf (Query,"SELECT COUNT(DISTINCT CrsCod),COUNT(DISTINCT GrpCod)-1,'-1',"
			      "MAX(NumLevels),SUM(NumFolders),SUM(NumFiles),SUM(TotalSize)"
			      " FROM "
	                      "("
	                      "SELECT file_browser_size.Cod AS CrsCod,'-1' AS GrpCod,"		// Course zones
			      "file_browser_size.NumLevels,file_browser_size.NumFolders,file_browser_size.NumFiles,file_browser_size.TotalSize"
			      " FROM institutions,centres,degrees,courses,file_browser_size"
			      " WHERE institutions.CtyCod='%ld'"
			      " AND institutions.InsCod=centres.InsCod"
			      " AND centres.CtrCod=degrees.CtrCod"
			      " AND degrees.DegCod=courses.DegCod"
			      " AND courses.CrsCod=file_browser_size.Cod"
			      " AND file_browser_size.FileBrowser IN ('%u','%u','%u','%u','%u')"
	                      " UNION "
	                      "SELECT crs_grp_types.CrsCod,file_browser_size.Cod AS GrpCod,"	// Group zones
			      "file_browser_size.NumLevels,file_browser_size.NumFolders,file_browser_size.NumFiles,file_browser_size.TotalSize"
			      " FROM institutions,centres,degrees,courses,crs_grp_types,crs_grp,file_browser_size"
			      " WHERE institutions.CtyCod='%ld'"
	                      " AND institutions.InsCod=centres.InsCod"
			      " AND centres.CtrCod=degrees.CtrCod"
			      " AND degrees.DegCod=courses.DegCod"
			      " AND courses.CrsCod=crs_grp_types.CrsCod"
			      " AND crs_grp_types.GrpTypCod=crs_grp.GrpTypCod"
			      " AND crs_grp.GrpCod=file_browser_size.Cod"
			      " AND file_browser_size.FileBrowser IN ('%u','%u','%u')"
			      ") AS sizes",
			Gbl.CurrentCty.Cty.CtyCod,
			(unsigned) Brw_ADMI_DOCUM_CRS,
			(unsigned) Brw_ADMI_SHARE_CRS,
			(unsigned) Brw_ADMI_ASSIG_USR,
			(unsigned) Brw_ADMI_WORKS_USR,
			(unsigned) Brw_ADMI_MARKS_CRS,
			Gbl.CurrentCty.Cty.CtyCod,
			(unsigned) Brw_ADMI_DOCUM_GRP,
			(unsigned) Brw_ADMI_SHARE_GRP,
			(unsigned) Brw_ADMI_MARKS_GRP);
	       break;
	    case Brw_ADMI_DOCUM_CRS:
	    case Brw_ADMI_SHARE_CRS:
	    case Brw_ADMI_MARKS_CRS:
	       sprintf (Query,"SELECT COUNT(DISTINCT file_browser_size.Cod),'-1','-1',"
			      "MAX(file_browser_size.NumLevels),SUM(file_browser_size.NumFolders),SUM(file_browser_size.NumFiles),SUM(file_browser_size.TotalSize)"
			      " FROM institutions,centres,degrees,courses,file_browser_size"
			      " WHERE institutions.CtyCod='%ld'"
	                      " AND institutions.InsCod=centres.InsCod"
			      " AND centres.CtrCod=degrees.CtrCod"
			      " AND degrees.DegCod=courses.DegCod"
			      " AND courses.CrsCod=file_browser_size.Cod"
			      " and file_browser_size.FileBrowser='%u'",
			Gbl.CurrentCty.Cty.CtyCod,(unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_DOCUM_GRP:
	    case Brw_ADMI_SHARE_GRP:
	    case Brw_ADMI_MARKS_GRP:
	       sprintf (Query,"SELECT COUNT(DISTINCT crs_grp_types.CrsCod),COUNT(DISTINCT file_browser_size.Cod),'-1',"
			      "MAX(file_browser_size.NumLevels),SUM(file_browser_size.NumFolders),SUM(file_browser_size.NumFiles),SUM(file_browser_size.TotalSize)"
			      " FROM institutions,centres,degrees,courses,crs_grp_types,crs_grp,file_browser_size"
			      " WHERE institutions.CtyCod='%ld'"
	                      " AND institutions.InsCod=centres.InsCod"
			      " AND centres.CtrCod=degrees.CtrCod"
			      " AND degrees.DegCod=courses.DegCod"
			      " AND courses.CrsCod=crs_grp_types.CrsCod"
			      " AND crs_grp_types.GrpTypCod=crs_grp.GrpTypCod"
			      " AND crs_grp.GrpCod=file_browser_size.Cod"
			      " AND file_browser_size.FileBrowser='%u'",
			Gbl.CurrentCty.Cty.CtyCod,(unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_ASSIG_USR:
	    case Brw_ADMI_WORKS_USR:
	       sprintf (Query,"SELECT COUNT(DISTINCT file_browser_size.Cod),'-1',COUNT(DISTINCT file_browser_size.ZoneUsrCod),"
			      "MAX(file_browser_size.NumLevels),SUM(file_browser_size.NumFolders),SUM(file_browser_size.NumFiles),SUM(file_browser_size.TotalSize)"
			      " FROM institutions,centres,degrees,courses,file_browser_size"
			      " WHERE institutions.CtyCod='%ld'"
	                      " AND institutions.InsCod=centres.InsCod"
			      " AND centres.CtrCod=degrees.CtrCod"
			      " AND degrees.DegCod=courses.DegCod"
			      " AND courses.CrsCod=file_browser_size.Cod"
	                      " AND file_browser_size.FileBrowser='%u'",
			Gbl.CurrentCty.Cty.CtyCod,(unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_BRIEF_USR:
	       sprintf (Query,"SELECT '-1','-1',COUNT(DISTINCT file_browser_size.ZoneUsrCod),"
			      "MAX(file_browser_size.NumLevels),SUM(file_browser_size.NumFolders),SUM(file_browser_size.NumFiles),SUM(file_browser_size.TotalSize)"
			      " FROM institutions,centres,degrees,courses,crs_usr,file_browser_size"
			      " WHERE institutions.CtyCod='%ld'"
	                      " AND institutions.InsCod=centres.InsCod"
			      " AND centres.CtrCod=degrees.CtrCod"
			      " AND degrees.DegCod=courses.DegCod"
			      " AND courses.CrsCod=crs_usr.CrsCod"
			      " AND crs_usr.UsrCod=file_browser_size.ZoneUsrCod"
			      " AND file_browser_size.FileBrowser='%u'",
			Gbl.CurrentCty.Cty.CtyCod,(unsigned) FileBrowser);
	       break;
	    default:
	       Lay_ShowErrorAndExit ("Wrong file browser.");
	       break;
	   }
         break;
      /* Scope = the current institution */
      case Sco_SCOPE_INSTITUTION:
	 switch (FileBrowser)
	   {
	    case Brw_UNKNOWN:
	       sprintf (Query,"SELECT COUNT(DISTINCT CrsCod),COUNT(DISTINCT GrpCod)-1,'-1',"
			      "MAX(NumLevels),SUM(NumFolders),SUM(NumFiles),SUM(TotalSize)"
			      " FROM "
	                      "("
	                      "SELECT file_browser_size.Cod AS CrsCod,'-1' AS GrpCod,"		// Course zones
			      "file_browser_size.NumLevels,file_browser_size.NumFolders,file_browser_size.NumFiles,file_browser_size.TotalSize"
			      " FROM centres,degrees,courses,file_browser_size"
			      " WHERE centres.InsCod='%ld'"
			      " AND centres.CtrCod=degrees.CtrCod"
			      " AND degrees.DegCod=courses.DegCod"
			      " AND courses.CrsCod=file_browser_size.Cod"
			      " AND file_browser_size.FileBrowser IN ('%u','%u','%u','%u','%u')"
	                      " UNION "
	                      "SELECT crs_grp_types.CrsCod,file_browser_size.Cod AS GrpCod,"	// Group zones
			      "file_browser_size.NumLevels,file_browser_size.NumFolders,file_browser_size.NumFiles,file_browser_size.TotalSize"
			      " FROM centres,degrees,courses,crs_grp_types,crs_grp,file_browser_size"
			      " WHERE centres.InsCod='%ld'"
			      " AND centres.CtrCod=degrees.CtrCod"
			      " AND degrees.DegCod=courses.DegCod"
			      " AND courses.CrsCod=crs_grp_types.CrsCod"
			      " AND crs_grp_types.GrpTypCod=crs_grp.GrpTypCod"
			      " AND crs_grp.GrpCod=file_browser_size.Cod"
			      " AND file_browser_size.FileBrowser IN ('%u','%u','%u')"
			      ") AS sizes",
			Gbl.CurrentIns.Ins.InsCod,
			(unsigned) Brw_ADMI_DOCUM_CRS,
			(unsigned) Brw_ADMI_SHARE_CRS,
			(unsigned) Brw_ADMI_ASSIG_USR,
			(unsigned) Brw_ADMI_WORKS_USR,
			(unsigned) Brw_ADMI_MARKS_CRS,
			Gbl.CurrentIns.Ins.InsCod,
			(unsigned) Brw_ADMI_DOCUM_GRP,
			(unsigned) Brw_ADMI_SHARE_GRP,
			(unsigned) Brw_ADMI_MARKS_GRP);
	       break;
	    case Brw_ADMI_DOCUM_CRS:
	    case Brw_ADMI_SHARE_CRS:
	    case Brw_ADMI_MARKS_CRS:
	       sprintf (Query,"SELECT COUNT(DISTINCT file_browser_size.Cod),'-1','-1',"
			      "MAX(file_browser_size.NumLevels),SUM(file_browser_size.NumFolders),SUM(file_browser_size.NumFiles),SUM(file_browser_size.TotalSize)"
			      " FROM centres,degrees,courses,file_browser_size"
			      " WHERE centres.InsCod='%ld'"
			      " AND centres.CtrCod=degrees.CtrCod"
			      " AND degrees.DegCod=courses.DegCod"
			      " AND courses.CrsCod=file_browser_size.Cod"
			      " and file_browser_size.FileBrowser='%u'",
			Gbl.CurrentIns.Ins.InsCod,(unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_DOCUM_GRP:
	    case Brw_ADMI_SHARE_GRP:
	    case Brw_ADMI_MARKS_GRP:
	       sprintf (Query,"SELECT COUNT(DISTINCT crs_grp_types.CrsCod),COUNT(DISTINCT file_browser_size.Cod),'-1',"
			      "MAX(file_browser_size.NumLevels),SUM(file_browser_size.NumFolders),SUM(file_browser_size.NumFiles),SUM(file_browser_size.TotalSize)"
			      " FROM centres,degrees,courses,crs_grp_types,crs_grp,file_browser_size"
			      " WHERE centres.InsCod='%ld'"
			      " AND centres.CtrCod=degrees.CtrCod"
			      " AND degrees.DegCod=courses.DegCod"
			      " AND courses.CrsCod=crs_grp_types.CrsCod"
			      " AND crs_grp_types.GrpTypCod=crs_grp.GrpTypCod"
			      " AND crs_grp.GrpCod=file_browser_size.Cod"
			      " AND file_browser_size.FileBrowser='%u'",
			Gbl.CurrentIns.Ins.InsCod,(unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_ASSIG_USR:
	    case Brw_ADMI_WORKS_USR:
	       sprintf (Query,"SELECT COUNT(DISTINCT file_browser_size.Cod),'-1',COUNT(DISTINCT file_browser_size.ZoneUsrCod),"
			      "MAX(file_browser_size.NumLevels),SUM(file_browser_size.NumFolders),SUM(file_browser_size.NumFiles),SUM(file_browser_size.TotalSize)"
			      " FROM centres,degrees,courses,file_browser_size"
			      " WHERE centres.InsCod='%ld'"
			      " AND centres.CtrCod=degrees.CtrCod"
			      " AND degrees.DegCod=courses.DegCod"
			      " AND courses.CrsCod=file_browser_size.Cod"
	                      " AND file_browser_size.FileBrowser='%u'",
			Gbl.CurrentIns.Ins.InsCod,(unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_BRIEF_USR:
	       sprintf (Query,"SELECT '-1','-1',COUNT(DISTINCT file_browser_size.ZoneUsrCod),"
			      "MAX(file_browser_size.NumLevels),SUM(file_browser_size.NumFolders),SUM(file_browser_size.NumFiles),SUM(file_browser_size.TotalSize)"
			      " FROM centres,degrees,courses,crs_usr,file_browser_size"
			      " WHERE centres.InsCod='%ld'"
			      " AND centres.CtrCod=degrees.CtrCod"
			      " AND degrees.DegCod=courses.DegCod"
			      " AND courses.CrsCod=crs_usr.CrsCod"
			      " AND crs_usr.UsrCod=file_browser_size.ZoneUsrCod"
			      " AND file_browser_size.FileBrowser='%u'",
			Gbl.CurrentIns.Ins.InsCod,(unsigned) FileBrowser);
	       break;
	    default:
	       Lay_ShowErrorAndExit ("Wrong file browser.");
	       break;
	   }
         break;
      /* Scope = the current centre */
      case Sco_SCOPE_CENTRE:
	 switch (FileBrowser)
	   {
	    case Brw_UNKNOWN:
	       sprintf (Query,"SELECT COUNT(DISTINCT CrsCod),COUNT(DISTINCT GrpCod)-1,'-1',"
			      "MAX(NumLevels),SUM(NumFolders),SUM(NumFiles),SUM(TotalSize)"
			      " FROM "
	                      "("
	                      "SELECT file_browser_size.Cod AS CrsCod,'-1' AS GrpCod,"		// Course zones
			      "file_browser_size.NumLevels,file_browser_size.NumFolders,file_browser_size.NumFiles,file_browser_size.TotalSize"
			      " FROM degrees,courses,file_browser_size"
			      " WHERE degrees.CtrCod='%ld'"
			      " AND degrees.DegCod=courses.DegCod"
			      " AND courses.CrsCod=file_browser_size.Cod"
			      " AND file_browser_size.FileBrowser IN ('%u','%u','%u','%u','%u')"
	                      " UNION "
	                      "SELECT crs_grp_types.CrsCod,file_browser_size.Cod AS GrpCod,"	// Group zones
			      "file_browser_size.NumLevels,file_browser_size.NumFolders,file_browser_size.NumFiles,file_browser_size.TotalSize"
			      " FROM degrees,courses,crs_grp_types,crs_grp,file_browser_size"
			      " WHERE degrees.CtrCod='%ld'"
			      " AND degrees.DegCod=courses.DegCod"
			      " AND courses.CrsCod=crs_grp_types.CrsCod"
			      " AND crs_grp_types.GrpTypCod=crs_grp.GrpTypCod"
			      " AND crs_grp.GrpCod=file_browser_size.Cod"
			      " AND file_browser_size.FileBrowser IN ('%u','%u','%u')"
			      ") AS sizes",
			Gbl.CurrentCtr.Ctr.CtrCod,
			(unsigned) Brw_ADMI_DOCUM_CRS,
			(unsigned) Brw_ADMI_SHARE_CRS,
			(unsigned) Brw_ADMI_ASSIG_USR,
			(unsigned) Brw_ADMI_WORKS_USR,
			(unsigned) Brw_ADMI_MARKS_CRS,
			Gbl.CurrentCtr.Ctr.CtrCod,
			(unsigned) Brw_ADMI_DOCUM_GRP,
			(unsigned) Brw_ADMI_SHARE_GRP,
			(unsigned) Brw_ADMI_MARKS_GRP);
	       break;
	    case Brw_ADMI_DOCUM_CRS:
	    case Brw_ADMI_SHARE_CRS:
	    case Brw_ADMI_MARKS_CRS:
	       sprintf (Query,"SELECT COUNT(DISTINCT file_browser_size.Cod),'-1','-1',"
			      "MAX(file_browser_size.NumLevels),SUM(file_browser_size.NumFolders),SUM(file_browser_size.NumFiles),SUM(file_browser_size.TotalSize)"
			      " FROM degrees,courses,file_browser_size"
			      " WHERE degrees.CtrCod='%ld'"
			      " AND degrees.DegCod=courses.DegCod"
			      " AND courses.CrsCod=file_browser_size.Cod"
			      " AND file_browser_size.FileBrowser='%u'",
			Gbl.CurrentCtr.Ctr.CtrCod,(unsigned) FileBrowser);
               break;
	    case Brw_ADMI_DOCUM_GRP:
	    case Brw_ADMI_SHARE_GRP:
	    case Brw_ADMI_MARKS_GRP:
	       sprintf (Query,"SELECT COUNT(DISTINCT crs_grp_types.CrsCod),COUNT(DISTINCT file_browser_size.Cod),'-1',"
			      "MAX(file_browser_size.NumLevels),SUM(file_browser_size.NumFolders),SUM(file_browser_size.NumFiles),SUM(file_browser_size.TotalSize)"
			      " FROM degrees,courses,crs_grp_types,crs_grp,file_browser_size"
			      " WHERE degrees.CtrCod='%ld'"
			      " AND degrees.DegCod=courses.DegCod"
			      " AND courses.CrsCod=crs_grp_types.CrsCod"
			      " AND crs_grp_types.GrpTypCod=crs_grp.GrpTypCod"
			      " AND crs_grp.GrpCod=file_browser_size.Cod"
			      " AND file_browser_size.FileBrowser='%u'",
			Gbl.CurrentCtr.Ctr.CtrCod,(unsigned) FileBrowser);
               break;
	    case Brw_ADMI_ASSIG_USR:
	    case Brw_ADMI_WORKS_USR:
	       sprintf (Query,"SELECT COUNT(DISTINCT file_browser_size.Cod),'-1',COUNT(DISTINCT file_browser_size.ZoneUsrCod),"
			      "MAX(file_browser_size.NumLevels),SUM(file_browser_size.NumFolders),SUM(file_browser_size.NumFiles),SUM(file_browser_size.TotalSize)"
			      " FROM degrees,courses,file_browser_size"
			      " WHERE degrees.CtrCod='%ld'"
			      " AND degrees.DegCod=courses.DegCod"
			      " AND courses.CrsCod=file_browser_size.Cod"
			      " AND file_browser_size.FileBrowser='%u'",
			Gbl.CurrentCtr.Ctr.CtrCod,(unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_BRIEF_USR:
	       sprintf (Query,"SELECT '-1','-1',COUNT(DISTINCT file_browser_size.ZoneUsrCod),"
			      "MAX(file_browser_size.NumLevels),SUM(file_browser_size.NumFolders),SUM(file_browser_size.NumFiles),SUM(file_browser_size.TotalSize)"
			      " FROM degrees,courses,crs_usr,file_browser_size"
			      " WHERE degrees.CtrCod='%ld'"
			      " AND degrees.DegCod=courses.DegCod"
			      " AND courses.CrsCod=crs_usr.CrsCod"
			      " AND crs_usr.UsrCod=file_browser_size.ZoneUsrCod"
			      " AND file_browser_size.FileBrowser='%u'",
			Gbl.CurrentCtr.Ctr.CtrCod,(unsigned) FileBrowser);
	       break;
	    default:
	       Lay_ShowErrorAndExit ("Wrong file browser.");
	       break;
	   }
         break;
      /* Scope = the current degree */
      case Sco_SCOPE_DEGREE:
	 switch (FileBrowser)
	   {
	    case Brw_UNKNOWN:
	       sprintf (Query,"SELECT COUNT(DISTINCT CrsCod),COUNT(DISTINCT GrpCod)-1,'-1',"
			      "MAX(NumLevels),SUM(NumFolders),SUM(NumFiles),SUM(TotalSize)"
			      " FROM "
	                      "("
	                      "SELECT file_browser_size.Cod AS CrsCod,'-1' AS GrpCod,"		// Course zones
			      "file_browser_size.NumLevels,file_browser_size.NumFolders,file_browser_size.NumFiles,file_browser_size.TotalSize"
			      " FROM courses,file_browser_size"
			      " WHERE courses.DegCod='%ld'"
			      " AND courses.CrsCod=file_browser_size.Cod"
			      " AND file_browser_size.FileBrowser IN ('%u','%u','%u','%u','%u')"
	                      " UNION "
	                      "SELECT crs_grp_types.CrsCod,file_browser_size.Cod AS GrpCod,"	// Group zones
			      "file_browser_size.NumLevels,file_browser_size.NumFolders,file_browser_size.NumFiles,file_browser_size.TotalSize"
			      " FROM courses,crs_grp_types,crs_grp,file_browser_size"
			      " WHERE courses.DegCod='%ld'"
			      " AND courses.CrsCod=crs_grp_types.CrsCod"
			      " AND crs_grp_types.GrpTypCod=crs_grp.GrpTypCod"
			      " AND crs_grp.GrpCod=file_browser_size.Cod"
			      " AND file_browser_size.FileBrowser IN ('%u','%u','%u')"
			      ") AS sizes",
			Gbl.CurrentDeg.Deg.DegCod,
			(unsigned) Brw_ADMI_DOCUM_CRS,
			(unsigned) Brw_ADMI_SHARE_CRS,
			(unsigned) Brw_ADMI_ASSIG_USR,
			(unsigned) Brw_ADMI_WORKS_USR,
			(unsigned) Brw_ADMI_MARKS_CRS,
			Gbl.CurrentDeg.Deg.DegCod,
			(unsigned) Brw_ADMI_DOCUM_GRP,
			(unsigned) Brw_ADMI_SHARE_GRP,
			(unsigned) Brw_ADMI_MARKS_GRP);
	       break;
	    case Brw_ADMI_DOCUM_CRS:
	    case Brw_ADMI_SHARE_CRS:
	    case Brw_ADMI_MARKS_CRS:
	       sprintf (Query,"SELECT COUNT(DISTINCT file_browser_size.Cod),'-1','-1',"
			      "MAX(file_browser_size.NumLevels),SUM(file_browser_size.NumFolders),SUM(file_browser_size.NumFiles),SUM(file_browser_size.TotalSize)"
			      " FROM courses,file_browser_size"
			      " WHERE courses.DegCod='%ld'"
			      " AND courses.CrsCod=file_browser_size.Cod"
			      " AND file_browser_size.FileBrowser='%u'",
			Gbl.CurrentDeg.Deg.DegCod,(unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_DOCUM_GRP:
	    case Brw_ADMI_SHARE_GRP:
	    case Brw_ADMI_MARKS_GRP:
	       sprintf (Query,"SELECT COUNT(DISTINCT crs_grp_types.CrsCod),COUNT(DISTINCT file_browser_size.Cod),'-1',"
			      "MAX(file_browser_size.NumLevels),SUM(file_browser_size.NumFolders),SUM(file_browser_size.NumFiles),SUM(file_browser_size.TotalSize)"
			      " FROM courses,crs_grp_types,crs_grp,file_browser_size"
			      " WHERE courses.DegCod='%ld'"
			      " AND courses.CrsCod=crs_grp_types.CrsCod"
			      " AND crs_grp_types.GrpTypCod=crs_grp.GrpTypCod"
			      " AND crs_grp.GrpCod=file_browser_size.Cod"
			      " AND file_browser_size.FileBrowser='%u'",
			Gbl.CurrentDeg.Deg.DegCod,(unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_ASSIG_USR:
	    case Brw_ADMI_WORKS_USR:
	       sprintf (Query,"SELECT COUNT(DISTINCT file_browser_size.Cod),'-1',COUNT(DISTINCT file_browser_size.ZoneUsrCod),"
			      "MAX(file_browser_size.NumLevels),SUM(file_browser_size.NumFolders),SUM(file_browser_size.NumFiles),SUM(file_browser_size.TotalSize)"
			      " FROM courses,file_browser_size"
			      " WHERE courses.DegCod='%ld'"
			      " AND courses.CrsCod=file_browser_size.Cod"
			      " AND file_browser_size.FileBrowser='%u'",
			Gbl.CurrentDeg.Deg.DegCod,(unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_BRIEF_USR:
	       sprintf (Query,"SELECT '-1','-1',COUNT(DISTINCT file_browser_size.ZoneUsrCod),"
			      "MAX(file_browser_size.NumLevels),SUM(file_browser_size.NumFolders),SUM(file_browser_size.NumFiles),SUM(file_browser_size.TotalSize)"
			      " FROM courses,crs_usr,file_browser_size"
			      " WHERE courses.DegCod='%ld'"
			      " AND courses.CrsCod=crs_usr.CrsCod"
			      " AND crs_usr.UsrCod=file_browser_size.ZoneUsrCod"
			      " AND file_browser_size.FileBrowser='%u'",
			Gbl.CurrentDeg.Deg.DegCod,(unsigned) FileBrowser);
	       break;
	    default:
	       Lay_ShowErrorAndExit ("Wrong file browser.");
	       break;
	   }
         break;
      /* Scope = the current course */
      case Sco_SCOPE_COURSE:
	 switch (FileBrowser)
	   {
	    case Brw_UNKNOWN:
	       sprintf (Query,"SELECT COUNT(DISTINCT CrsCod),COUNT(DISTINCT GrpCod)-1,'-1',"
			      "MAX(NumLevels),SUM(NumFolders),SUM(NumFiles),SUM(TotalSize)"
			      " FROM "
	                      "("
	                      "SELECT Cod AS CrsCod,'-1' AS GrpCod,"				// Course zones
			      "NumLevels,NumFolders,NumFiles,TotalSize"
			      " FROM file_browser_size"
			      " WHERE Cod='%ld'"
			      " AND FileBrowser IN ('%u','%u','%u','%u','%u')"
	                      " UNION "
	                      "SELECT crs_grp_types.CrsCod,file_browser_size.Cod AS GrpCod,"	// Group zones
			      "file_browser_size.NumLevels,file_browser_size.NumFolders,file_browser_size.NumFiles,file_browser_size.TotalSize"
			      " FROM crs_grp_types,crs_grp,file_browser_size"
			      " WHERE crs_grp_types.CrsCod='%ld'"
			      " AND crs_grp_types.GrpTypCod=crs_grp.GrpTypCod"
			      " AND crs_grp.GrpCod=file_browser_size.Cod"
			      " AND file_browser_size.FileBrowser IN ('%u','%u','%u')"
			      ") AS sizes",
			Gbl.CurrentCrs.Crs.CrsCod,
			(unsigned) Brw_ADMI_DOCUM_CRS,
			(unsigned) Brw_ADMI_SHARE_CRS,
			(unsigned) Brw_ADMI_ASSIG_USR,
			(unsigned) Brw_ADMI_WORKS_USR,
			(unsigned) Brw_ADMI_MARKS_CRS,
			Gbl.CurrentCrs.Crs.CrsCod,
			(unsigned) Brw_ADMI_DOCUM_GRP,
			(unsigned) Brw_ADMI_SHARE_GRP,
			(unsigned) Brw_ADMI_MARKS_GRP);
	       break;
	    case Brw_ADMI_DOCUM_CRS:
	    case Brw_ADMI_SHARE_CRS:
	    case Brw_ADMI_MARKS_CRS:
	       sprintf (Query,"SELECT '1','-1','-1',"
			      "MAX(NumLevels),SUM(NumFolders),SUM(NumFiles),SUM(TotalSize)"
			      " FROM file_browser_size"
			      " WHERE Cod='%ld' AND FileBrowser='%u'",
			Gbl.CurrentCrs.Crs.CrsCod,(unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_DOCUM_GRP:
	    case Brw_ADMI_SHARE_GRP:
	    case Brw_ADMI_MARKS_GRP:
	       sprintf (Query,"SELECT COUNT(DISTINCT crs_grp_types.CrsCod),COUNT(DISTINCT file_browser_size.Cod),'-1',"
			      "MAX(file_browser_size.NumLevels),SUM(file_browser_size.NumFolders),SUM(file_browser_size.NumFiles),SUM(file_browser_size.TotalSize)"
			      " FROM crs_grp_types,crs_grp,file_browser_size"
			      " WHERE crs_grp_types.CrsCod='%ld'"
			      " AND crs_grp_types.GrpTypCod=crs_grp.GrpTypCod"
			      " AND crs_grp.GrpCod=file_browser_size.Cod"
			      " AND file_browser_size.FileBrowser='%u'",
			Gbl.CurrentCrs.Crs.CrsCod,(unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_ASSIG_USR:
	    case Brw_ADMI_WORKS_USR:
	       sprintf (Query,"SELECT '1','-1',COUNT(DISTINCT ZoneUsrCod),"
			      "MAX(NumLevels),SUM(NumFolders),SUM(NumFiles),SUM(TotalSize)"
			      " FROM file_browser_size"
			      " WHERE Cod='%ld' AND FileBrowser='%u'",
			Gbl.CurrentCrs.Crs.CrsCod,(unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_BRIEF_USR:
	       sprintf (Query,"SELECT '-1','-1',COUNT(DISTINCT file_browser_size.ZoneUsrCod),"
			      "MAX(file_browser_size.NumLevels),SUM(file_browser_size.NumFolders),SUM(file_browser_size.NumFiles),SUM(file_browser_size.TotalSize)"
			      " FROM crs_usr,file_browser_size"
			      " WHERE crs_usr.CrsCod='%ld'"
			      " AND crs_usr.UsrCod=file_browser_size.ZoneUsrCod"
			      " AND file_browser_size.FileBrowser='%u'",
			Gbl.CurrentCrs.Crs.CrsCod,(unsigned) FileBrowser);
	       break;
	    default:
	       Lay_ShowErrorAndExit ("Wrong file browser.");
	       break;
	   }
         break;
      default:
	 Lay_ShowErrorAndExit ("Wrong scope.");
	 break;
     }
   DB_QuerySELECT (Query,&mysql_res,"can not get size of a file browser");

   /* Get row */
   row = mysql_fetch_row (mysql_res);

   /* Reset default values to zero */
   SizeOfFileZones->NumCrss = SizeOfFileZones->NumUsrs = 0;
   SizeOfFileZones->MaxLevels = 0;
   SizeOfFileZones->NumFolders = SizeOfFileZones->NumFiles = 0;
   SizeOfFileZones->Size = 0;

   /* Get number of courses (row[0]) */
   if (row[0])
      if (sscanf (row[0],"%u",&(SizeOfFileZones->NumCrss)) != 1)
         Lay_ShowErrorAndExit ("Error when getting number of courses.");

   /* Get number of groups (row[1]) */
   if (row[1])
      if (sscanf (row[1],"%u",&(SizeOfFileZones->NumGrps)) != 1)
         Lay_ShowErrorAndExit ("Error when getting number of groups.");

   /* Get number of users (row[2]) */
   if (row[2])
      if (sscanf (row[2],"%u",&(SizeOfFileZones->NumUsrs)) != 1)
         Lay_ShowErrorAndExit ("Error when getting number of users.");

   /* Get maximum number of levels (row[3]) */
   if (row[3])
      if (sscanf (row[3],"%u",&(SizeOfFileZones->MaxLevels)) != 1)
         Lay_ShowErrorAndExit ("Error when getting maximum number of levels.");

   /* Get number of folders (row[4]) */
   if (row[4])
      if (sscanf (row[4],"%lu",&(SizeOfFileZones->NumFolders)) != 1)
         Lay_ShowErrorAndExit ("Error when getting number of folders.");

   /* Get number of files (row[5]) */
   if (row[5])
      if (sscanf (row[5],"%lu",&(SizeOfFileZones->NumFiles)) != 1)
         Lay_ShowErrorAndExit ("Error when getting number of files.");

   /* Get total size (row[6]) */
   if (row[6])
      if (sscanf (row[6],"%llu",&(SizeOfFileZones->Size)) != 1)
         Lay_ShowErrorAndExit ("Error when getting toal size.");

   /* Free structure that stores the query result */
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/************ Show stats about Open Educational Resources (OERs) *************/
/*****************************************************************************/

static void Sta_GetAndShowOERsStats (void)
  {
   extern const char *Txt_STAT_USE_STAT_TYPES[Sta_NUM_TYPES_USE_STATS];
   extern const char *Txt_License;
   extern const char *Txt_No_of_private_files;
   extern const char *Txt_No_of_public_files;
   extern const char *Txt_LICENSES[Brw_NUM_LICENSES];
   Brw_License_t License;
   unsigned long NumFiles[2];

   /***** Table start *****/
   Lay_StartRoundFrameTable10 (NULL,2,Txt_STAT_USE_STAT_TYPES[Sta_OER]);

   /***** Write table heading *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<th class=\"TIT_TBL\" style=\"text-align:left;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "</tr>",
            Txt_License,
            Txt_No_of_private_files,
            Txt_No_of_public_files);

   for (License = 0;
	License < Brw_NUM_LICENSES;
	License++)
     {
      Sta_GetNumberOfOERsFromDB (Gbl.Scope.Current,License,NumFiles);

      fprintf (Gbl.F.Out,"<tr>"
                         "<td class=\"DAT\" style=\"text-align:left;\">"
                         "%s"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%lu"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%lu"
                         "</td>"
                         "</tr>",
               Txt_LICENSES[License],
               NumFiles[0],
               NumFiles[1]);
     }

   /***** End table *****/
   Lay_EndRoundFrameTable10 ();
  }

/*****************************************************************************/
/**************** Get the size of a file zone from database ******************/
/*****************************************************************************/

static void Sta_GetNumberOfOERsFromDB (Sco_Scope_t Scope,Brw_License_t License,unsigned long NumFiles[2])
  {
   char Query[512];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumRows,NumRow;
   unsigned Public;

   /***** Get the size of a file browser *****/
   switch (Scope)
     {
      case Sco_SCOPE_PLATFORM:
         sprintf (Query,"SELECT Public,COUNT(*)"
                        " FROM files"
                        " WHERE License='%u'"
                        " GROUP BY Public",
                  (unsigned) License);
         break;
      case Sco_SCOPE_INSTITUTION:
         sprintf (Query,"SELECT files.Public,COUNT(*)"
                        " FROM centres,degrees,courses,files"
                        " WHERE centres.InsCod='%ld'"
                        " AND centres.CtrCod=degrees.CtrCod"
                        " AND degrees.DegCod=courses.DegCod"
                        " AND courses.CrsCod=files.Cod"
	                " AND files.FileBrowser IN ('%u','%u')"
                        " AND files.License='%u'"
                        " GROUP BY files.Public",
                  Gbl.CurrentIns.Ins.InsCod,
                  (unsigned) Brw_ADMI_DOCUM_CRS,
		  (unsigned) Brw_ADMI_SHARE_CRS,
                  (unsigned) License);
         break;
      case Sco_SCOPE_CENTRE:
         sprintf (Query,"SELECT files.Public,COUNT(*)"
                        " FROM degrees,courses,files"
                        " WHERE degrees.CtrCod='%ld'"
                        " AND degrees.DegCod=courses.DegCod"
                        " AND courses.CrsCod=files.Cod"
	                " AND files.FileBrowser IN ('%u','%u')"
                        " AND files.License='%u'"
                        " GROUP BY files.Public",
                  Gbl.CurrentCtr.Ctr.CtrCod,
                  (unsigned) Brw_ADMI_DOCUM_CRS,
		  (unsigned) Brw_ADMI_SHARE_CRS,
                  (unsigned) License);
         break;
      case Sco_SCOPE_DEGREE:
         sprintf (Query,"SELECT files.Public,COUNT(*)"
                        " FROM courses,files"
                        " WHERE courses.DegCod='%ld'"
                        " AND courses.CrsCod=files.Cod"
	                " AND files.FileBrowser IN ('%u','%u')"
                        " AND files.License='%u'"
                        " GROUP BY files.Public",
                  Gbl.CurrentDeg.Deg.DegCod,
                  (unsigned) Brw_ADMI_DOCUM_CRS,
		  (unsigned) Brw_ADMI_SHARE_CRS,
                  (unsigned) License);
         break;
      case Sco_SCOPE_COURSE:
         sprintf (Query,"SELECT Public,COUNT(*)"
                        " FROM files"
                        " WHERE CrsCod='%ld'"
                        " AND License='%u'"
                        " GROUP BY Public",
                  Gbl.CurrentCrs.Crs.CrsCod,
                  (unsigned) License);
         break;
      default:
	 Lay_ShowErrorAndExit ("Wrong scope.");
	 break;
     }
   NumRows = (unsigned) DB_QuerySELECT (Query,&mysql_res,"can not get number of OERs");

   /* Reset values to zero */
   NumFiles[0] = NumFiles[1] = 0L;

   for (NumRow = 0;
	NumRow < NumRows;
	NumRow++)
     {
      /* Get row */
      row = mysql_fetch_row (mysql_res);

      /* Get if public (row[0]) */
      Public = (Str_ConvertToUpperLetter (row[0][0]) == 'Y') ? 1 :
	                                                       0;

      /* Get number of files (row[1]) */
      if (sscanf (row[1],"%lu",&NumFiles[Public]) != 1)
         Lay_ShowErrorAndExit ("Error when getting number of files.");
     }

   /* Free structure that stores the query result */
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/************************ Show stats about assignments ***********************/
/*****************************************************************************/

static void Sta_GetAndShowAssignmentsStats (void)
  {
   extern const char *Txt_STAT_USE_STAT_TYPES[Sta_NUM_TYPES_USE_STATS];
   extern const char *Txt_Number_of_BR_assignments;
   extern const char *Txt_Number_of_BR_courses_with_BR_assignments;
   extern const char *Txt_Average_number_BR_of_ASSIG_BR_per_course;
   extern const char *Txt_Number_of_BR_notifications;
   unsigned NumAssignments;
   unsigned NumNotif;
   unsigned NumCoursesWithAssignments = 0;
   float NumAssignmentsPerCourse = 0.0;

   /***** Get the number of assignments from this location
          (all the platform, current degree or current course) *****/
   if ((NumAssignments = Asg_GetNumAssignments (Gbl.Scope.Current,&NumNotif)))
      if ((NumCoursesWithAssignments = Asg_GetNumCoursesWithAssignments (Gbl.Scope.Current)) != 0)
         NumAssignmentsPerCourse = (float) NumAssignments / (float) NumCoursesWithAssignments;

   /***** Table start *****/
   Lay_StartRoundFrameTable10 (NULL,2,Txt_STAT_USE_STAT_TYPES[Sta_ASSIGNMENTS]);

   /***** Write table heading *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "</tr>",
            Txt_Number_of_BR_assignments,
            Txt_Number_of_BR_courses_with_BR_assignments,
            Txt_Average_number_BR_of_ASSIG_BR_per_course,
            Txt_Number_of_BR_notifications);

   /***** Write number of assignments *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%.2f"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "</tr>",
            NumAssignments,
            NumCoursesWithAssignments,
            NumAssignmentsPerCourse,
            NumNotif);

   /***** End table *****/
   Lay_EndRoundFrameTable10 ();
  }

/*****************************************************************************/
/********************** Show stats about test questions **********************/
/*****************************************************************************/

static void Sta_GetAndShowTestsStats (void)
  {
   extern const char *Txt_STAT_USE_STAT_TYPES[Sta_NUM_TYPES_USE_STATS];
   extern const char *Txt_Type_of_BR_answers;
   extern const char *Txt_Number_of_BR_courses_BR_with_test_BR_questions;
   extern const char *Txt_Number_of_BR_courses_with_BR_exportable_BR_test_BR_questions;
   extern const char *Txt_Number_BR_of_test_BR_questions;
   extern const char *Txt_Average_BR_number_BR_of_test_BR_questions_BR_per_course;
   extern const char *Txt_Number_of_BR_times_that_BR_questions_BR_have_been_BR_responded;
   extern const char *Txt_Average_BR_number_of_BR_times_that_BR_questions_BR_have_been_BR_responded_BR_per_course;
   extern const char *Txt_Average_BR_number_of_BR_times_that_BR_a_question_BR_has_been_BR_responded;
   extern const char *Txt_Average_BR_score_BR_per_question_BR_from_0_to_1;
   extern const char *Txt_TST_STR_ANSWER_TYPES[Tst_NUM_ANS_TYPES];
   extern const char *Txt_Total;
   Tst_AnswerType_t AnsType;
   struct Tst_Stats Stats;
   char *StyleTableCell = " border-style:solid none none none;"
	                  " border-width:1px;";

   /***** Table start *****/
   Lay_StartRoundFrameTable10 (NULL,2,Txt_STAT_USE_STAT_TYPES[Sta_TESTS]);

   /***** Write table heading *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<th class=\"TIT_TBL\" style=\"text-align:left;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "</tr>",
            Txt_Type_of_BR_answers,
            Txt_Number_of_BR_courses_BR_with_test_BR_questions,
            Txt_Number_of_BR_courses_with_BR_exportable_BR_test_BR_questions,
            Txt_Number_BR_of_test_BR_questions,
            Txt_Average_BR_number_BR_of_test_BR_questions_BR_per_course,
            Txt_Number_of_BR_times_that_BR_questions_BR_have_been_BR_responded,
            Txt_Average_BR_number_of_BR_times_that_BR_questions_BR_have_been_BR_responded_BR_per_course,
            Txt_Average_BR_number_of_BR_times_that_BR_a_question_BR_has_been_BR_responded,
            Txt_Average_BR_score_BR_per_question_BR_from_0_to_1);

   for (AnsType = (Tst_AnswerType_t) 0;
	AnsType < Tst_NUM_ANS_TYPES;
	AnsType++)
     {
      /***** Get the stats about test questions from this location
             (all the platform, current degree or current course) *****/
      Tst_GetTestStats (AnsType,&Stats);

      /***** Write number of assignments *****/
      fprintf (Gbl.F.Out,"<tr>"
                         "<td class=\"DAT\" style=\"text-align:left;\">"
                         "%s"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%u"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%u (%.1f%%)"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%u"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%.2f"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%lu"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%.2f"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%.2f"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%.2f"
                         "</td>"
                         "</tr>",
               Txt_TST_STR_ANSWER_TYPES[AnsType],
               Stats.NumCoursesWithQuestions,
               Stats.NumCoursesWithPluggableQuestions,
               Stats.NumCoursesWithQuestions ? (float) Stats.NumCoursesWithPluggableQuestions * 100.0 /
        	                               (float) Stats.NumCoursesWithQuestions :
        	                               0.0,
               Stats.NumQsts,
               Stats.AvgQstsPerCourse,
               Stats.NumHits,
               Stats.AvgHitsPerCourse,
               Stats.AvgHitsPerQuestion,
               Stats.AvgScorePerQuestion);
     }

   /***** Get the stats about test questions from this location
          (all the platform, current degree or current course) *****/
   Tst_GetTestStats (Tst_ANS_ALL,&Stats);

   /***** Write number of assignments *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"DAT_N\" style=\"text-align:left; %s\">"
                      "%s"
                      "</td>"
                      "<td class=\"DAT_N\" style=\"text-align:right; %s\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT_N\" style=\"text-align:right; %s\">"
                      "%u (%.1f%%)"
                      "</td>"
                      "<td class=\"DAT_N\" style=\"text-align:right; %s\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT_N\" style=\"text-align:right; %s\">"
                      "%.2f"
                      "</td>"
                      "<td class=\"DAT_N\" style=\"text-align:right; %s\">"
                      "%lu"
                      "</td>"
                      "<td class=\"DAT_N\" style=\"text-align:right; %s\">"
                      "%.2f"
                      "</td>"
                      "<td class=\"DAT_N\" style=\"text-align:right; %s\">"
                      "%.2f"
                      "</td>"
                      "<td class=\"DAT_N\" style=\"text-align:right; %s\">"
                      "%.2f"
                      "</td>"
                      "</tr>",
            StyleTableCell,Txt_Total,
            StyleTableCell,Stats.NumCoursesWithQuestions,
            StyleTableCell,Stats.NumCoursesWithPluggableQuestions,
            Stats.NumCoursesWithQuestions ? (float) Stats.NumCoursesWithPluggableQuestions * 100.0 /
        	                            (float) Stats.NumCoursesWithQuestions :
        	                            0.0,
            StyleTableCell,Stats.NumQsts,
            StyleTableCell,Stats.AvgQstsPerCourse,
            StyleTableCell,Stats.NumHits,
            StyleTableCell,Stats.AvgHitsPerCourse,
            StyleTableCell,Stats.AvgHitsPerQuestion,
            StyleTableCell,Stats.AvgScorePerQuestion);

   /***** End table *****/
   Lay_EndRoundFrameTable10 ();
  }

/*****************************************************************************/
/***************************** Show stats of notices *************************/
/*****************************************************************************/

static void Sta_GetAndShowNoticesStats (void)
  {
   extern const char *Txt_STAT_USE_STAT_TYPES[Sta_NUM_TYPES_USE_STATS];
   extern const char *Txt_NOTICE_Active_BR_notices;
   extern const char *Txt_NOTICE_Obsolete_BR_notices;
   extern const char *Txt_NOTICE_Deleted_BR_notices;
   extern const char *Txt_Total;
   extern const char *Txt_Number_of_BR_notifications;
   Not_Status_t NoticeStatus;
   unsigned NumNotices[Not_NUM_STATUS];
   unsigned NumNoticesDeleted;
   unsigned NumTotalNotices = 0;
   unsigned NumNotif;
   unsigned NumTotalNotifications = 0;

   /***** Get the number of notices active and obsolete
          from this location (all the platform, current degree or current course) *****/
   for (NoticeStatus = (Not_Status_t) 0;
	NoticeStatus < Not_NUM_STATUS;
	NoticeStatus++)
     {
      NumNotices[NoticeStatus] = Not_GetNumNotices (Gbl.Scope.Current,NoticeStatus,&NumNotif);
      NumTotalNotices += NumNotices[NoticeStatus];
      NumTotalNotifications += NumNotif;
     }
   NumNoticesDeleted = Not_GetNumNoticesDeleted (Gbl.Scope.Current,&NumNotif);
   NumTotalNotices += NumNoticesDeleted;
   NumTotalNotifications += NumNotif;

   /***** Table start *****/
   Lay_StartRoundFrameTable10 (NULL,2,Txt_STAT_USE_STAT_TYPES[Sta_NOTICES]);

   /***** Write table heading *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "</tr>",
            Txt_NOTICE_Active_BR_notices,
            Txt_NOTICE_Obsolete_BR_notices,
            Txt_NOTICE_Deleted_BR_notices,
            Txt_Total,
            Txt_Number_of_BR_notifications);

   /***** Write number of notices *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT_N\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "</tr>",
            NumNotices[Not_ACTIVE_NOTICE],
            NumNotices[Not_OBSOLETE_NOTICE],
            NumNoticesDeleted,
            NumTotalNotices,
            NumTotalNotifications);

   /***** End table *****/
   Lay_EndRoundFrameTable10 ();
  }

/*****************************************************************************/
/*************************** Show stats of messages **************************/
/*****************************************************************************/

static void Sta_GetAndShowMsgsStats (void)
  {
   extern const char *Txt_STAT_USE_STAT_TYPES[Sta_NUM_TYPES_USE_STATS];
   extern const char *Txt_Messages;
   extern const char *Txt_MSGS_Not_deleted;
   extern const char *Txt_MSGS_Deleted;
   extern const char *Txt_Total;
   extern const char *Txt_Number_of_BR_notifications;
   extern const char *Txt_MSGS_Sent;
   extern const char *Txt_MSGS_Received;
   unsigned NumMsgsSentNotDeleted,NumMsgsSentDeleted;
   unsigned NumMsgsReceivedNotDeleted,NumMsgsReceivedAndDeleted;
   unsigned NumMsgsReceivedAndNotified;

   /***** Get the number of unique messages sent from this location (all the platform, current degree or current course) *****/
   NumMsgsSentNotDeleted      = Msg_GetNumMsgsSent     (Gbl.Scope.Current,Msg_STATUS_ALL     );
   NumMsgsSentDeleted         = Msg_GetNumMsgsSent     (Gbl.Scope.Current,Msg_STATUS_DELETED );

   NumMsgsReceivedNotDeleted  = Msg_GetNumMsgsReceived (Gbl.Scope.Current,Msg_STATUS_ALL     );
   NumMsgsReceivedAndDeleted  = Msg_GetNumMsgsReceived (Gbl.Scope.Current,Msg_STATUS_DELETED );
   NumMsgsReceivedAndNotified = Msg_GetNumMsgsReceived (Gbl.Scope.Current,Msg_STATUS_NOTIFIED);

   /***** Table start *****/
   Lay_StartRoundFrameTable10 (NULL,2,Txt_STAT_USE_STAT_TYPES[Sta_MSGS_BETWEEN_USERS]);

   /***** Write table heading *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<th class=\"TIT_TBL\" style=\"text-align:left;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "</tr>",
            Txt_Messages,
            Txt_MSGS_Not_deleted,
            Txt_MSGS_Deleted,
            Txt_Total,
            Txt_Number_of_BR_notifications);

   /***** Write number of messages *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"DAT\" style=\"text-align:left;\">"
                      "%s"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT_N\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "-"
                      "</td>"
                      "</tr>"
                      "<tr>"
                      "<td class=\"DAT\" style=\"text-align:left;\">"
                      "%s"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT_N\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "</tr>",
            Txt_MSGS_Sent,
            NumMsgsSentNotDeleted,
            NumMsgsSentDeleted,
            NumMsgsSentNotDeleted + NumMsgsSentDeleted,
            Txt_MSGS_Received,
            NumMsgsReceivedNotDeleted,
            NumMsgsReceivedAndDeleted,
            NumMsgsReceivedNotDeleted + NumMsgsReceivedAndDeleted,
            NumMsgsReceivedAndNotified);

   /***** End table *****/
   Lay_EndRoundFrameTable10 ();
  }

/*****************************************************************************/
/***************************** Show stats of forums **************************/
/*****************************************************************************/

static void Sta_GetAndShowForumStats (void)
  {
   extern const char *Txt_STAT_USE_STAT_TYPES[Sta_NUM_TYPES_USE_STATS];
   extern const char *Txt_Forums;
   extern const char *Txt_No_of_forums;
   extern const char *Txt_No_of_threads;
   extern const char *Txt_No_of_messages;
   extern const char *Txt_Number_of_BR_notifications;
   extern const char *Txt_No_of_threads_BR_per_forum;
   extern const char *Txt_No_of_messages_BR_per_thread;
   extern const char *Txt_No_of_messages_BR_per_forum;
   struct Sta_StatsForum StatsForum;

   /***** Reset total stats *****/
   StatsForum.NumForums           = 0;
   StatsForum.NumThreads          = 0;
   StatsForum.NumPosts            = 0;
   StatsForum.NumUsrsToBeNotifiedByEMail = 0;

   /***** Table start *****/
   Lay_StartRoundFrameTable10 (NULL,2,Txt_STAT_USE_STAT_TYPES[Sta_FORUMS]);

   /***** Write table heading *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<th style=\"width:16px;"
                      " text-align:left; vertical-align:top;\">"
                      "<img src=\"%s/forum16x16.gif\""
                      " class=\"ICON16x16\" style=\"vertical-align:top;\" />"
                      "</th>"
                      "<th class=\"TIT_TBL\""
                      " style=\"text-align:left; vertical-align:top;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\""
                      " style=\"text-align:right; vertical-align:top;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\""
                      " style=\"text-align:right; vertical-align:top;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\""
                      " style=\"text-align:right; vertical-align:top;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\""
                      " style=\"text-align:right; vertical-align:top;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\""
                      " style=\"text-align:right; vertical-align:top;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\""
                      " style=\"text-align:right; vertical-align:top;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\""
                      " style=\"text-align:right; vertical-align:top;\">"
                      "%s"
                      "</th>"
                      "</tr>",
            Gbl.Prefs.IconsURL,
            Txt_Forums,
            Txt_No_of_forums,
            Txt_No_of_threads,
            Txt_No_of_messages,
            Txt_Number_of_BR_notifications,
            Txt_No_of_threads_BR_per_forum,
            Txt_No_of_messages_BR_per_thread,
            Txt_No_of_messages_BR_per_forum);

   /***** Write a row for each type of forum *****/
   switch (Gbl.Scope.Current)
     {
      case Sco_SCOPE_PLATFORM:
         Sta_ShowStatOfAForumType (For_FORUM_GLOBAL_USRS     ,-1L,-1L,-1L,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_GLOBAL_TCHS     ,-1L,-1L,-1L,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_SWAD_USRS       ,-1L,-1L,-1L,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_SWAD_TCHS       ,-1L,-1L,-1L,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_INSTITUTION_USRS,-1L,-1L,-1L,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_INSTITUTION_TCHS,-1L,-1L,-1L,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_CENTRE_USRS     ,-1L,-1L,-1L,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_CENTRE_TCHS     ,-1L,-1L,-1L,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_DEGREE_USRS     ,-1L,-1L,-1L,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_DEGREE_TCHS     ,-1L,-1L,-1L,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_COURSE_USRS     ,-1L,-1L,-1L,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_COURSE_TCHS     ,-1L,-1L,-1L,-1L,&StatsForum);
         break;
      case Sco_SCOPE_INSTITUTION:
         Sta_ShowStatOfAForumType (For_FORUM_INSTITUTION_USRS,Gbl.CurrentIns.Ins.InsCod,-1L,-1L,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_INSTITUTION_TCHS,Gbl.CurrentIns.Ins.InsCod,-1L,-1L,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_CENTRE_USRS     ,Gbl.CurrentIns.Ins.InsCod,-1L,-1L,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_CENTRE_TCHS     ,Gbl.CurrentIns.Ins.InsCod,-1L,-1L,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_DEGREE_USRS     ,Gbl.CurrentIns.Ins.InsCod,-1L,-1L,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_DEGREE_TCHS     ,Gbl.CurrentIns.Ins.InsCod,-1L,-1L,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_COURSE_USRS     ,Gbl.CurrentIns.Ins.InsCod,-1L,-1L,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_COURSE_TCHS     ,Gbl.CurrentIns.Ins.InsCod,-1L,-1L,-1L,&StatsForum);
         break;
      case Sco_SCOPE_CENTRE:
         Sta_ShowStatOfAForumType (For_FORUM_CENTRE_USRS,-1L,Gbl.CurrentCtr.Ctr.CtrCod,-1L,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_CENTRE_TCHS,-1L,Gbl.CurrentCtr.Ctr.CtrCod,-1L,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_DEGREE_USRS,-1L,Gbl.CurrentCtr.Ctr.CtrCod,-1L,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_DEGREE_TCHS,-1L,Gbl.CurrentCtr.Ctr.CtrCod,-1L,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_COURSE_USRS,-1L,Gbl.CurrentCtr.Ctr.CtrCod,-1L,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_COURSE_TCHS,-1L,Gbl.CurrentCtr.Ctr.CtrCod,-1L,-1L,&StatsForum);
         break;
      case Sco_SCOPE_DEGREE:
         Sta_ShowStatOfAForumType (For_FORUM_DEGREE_USRS,-1L,-1L,Gbl.CurrentDeg.Deg.DegCod,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_DEGREE_TCHS,-1L,-1L,Gbl.CurrentDeg.Deg.DegCod,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_COURSE_USRS,-1L,-1L,Gbl.CurrentDeg.Deg.DegCod,-1L,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_COURSE_TCHS,-1L,-1L,Gbl.CurrentDeg.Deg.DegCod,-1L,&StatsForum);
         break;
      case Sco_SCOPE_COURSE:
         Sta_ShowStatOfAForumType (For_FORUM_COURSE_USRS,-1L,-1L,-1L,Gbl.CurrentCrs.Crs.CrsCod,&StatsForum);
         Sta_ShowStatOfAForumType (For_FORUM_COURSE_TCHS,-1L,-1L,-1L,Gbl.CurrentCrs.Crs.CrsCod,&StatsForum);
         break;
      default:
	 Lay_ShowErrorAndExit ("Wrong scope.");
	 break;
     }

   Sta_WriteForumTotalStats (&StatsForum);

   /***** End table *****/
   Lay_EndRoundFrameTable10 ();
  }

/*****************************************************************************/
/************************* Show stats of a forum type ************************/
/*****************************************************************************/

static void Sta_ShowStatOfAForumType (For_ForumType_t ForumType,
                                      long InsCod,long CtrCod,long DegCod,long CrsCod,
                                      struct Sta_StatsForum *StatsForum)
  {
   extern const char *Txt_Courses;
   extern const char *Txt_Degrees;
   extern const char *Txt_Centres;
   extern const char *Txt_Institutions;
   extern const char *Txt_General;
   extern const char *Txt_only_teachers;

   switch (ForumType)
     {
      case For_FORUM_COURSE_USRS:
         Sta_WriteForumTitleAndStats (ForumType,InsCod,CtrCod,DegCod,CrsCod,
                                      "coursesdegree16x16.gif",StatsForum,
                                      Txt_Courses,"");
         break;
      case For_FORUM_COURSE_TCHS:
         Sta_WriteForumTitleAndStats (ForumType,InsCod,CtrCod,DegCod,CrsCod,
                                      "coursesdegree16x16.gif",StatsForum,
                                      Txt_Courses,Txt_only_teachers);
         break;
      case For_FORUM_DEGREE_USRS:
         Sta_WriteForumTitleAndStats (ForumType,InsCod,CtrCod,DegCod,CrsCod,
                                      "grouptypes16x16.gif",StatsForum,
                                      Txt_Degrees,"");
         break;
      case For_FORUM_DEGREE_TCHS:
         Sta_WriteForumTitleAndStats (ForumType,InsCod,CtrCod,DegCod,CrsCod,
                                      "grouptypes16x16.gif",StatsForum,
                                      Txt_Degrees,Txt_only_teachers);
         break;
      case For_FORUM_CENTRE_USRS:
         Sta_WriteForumTitleAndStats (ForumType,InsCod,CtrCod,DegCod,CrsCod,
                                      "house16x16.gif",StatsForum,
                                      Txt_Centres,"");
         break;
      case For_FORUM_CENTRE_TCHS:
         Sta_WriteForumTitleAndStats (ForumType,InsCod,CtrCod,DegCod,CrsCod,
                                      "house16x16.gif",StatsForum,
                                      Txt_Centres,Txt_only_teachers);
         break;
      case For_FORUM_INSTITUTION_USRS:
         Sta_WriteForumTitleAndStats (ForumType,InsCod,CtrCod,DegCod,CrsCod,
                                      "institution16x16.gif",StatsForum,
                                      Txt_Institutions,"");
         break;
      case For_FORUM_INSTITUTION_TCHS:
         Sta_WriteForumTitleAndStats (ForumType,InsCod,CtrCod,DegCod,CrsCod,
                                      "institution16x16.gif",StatsForum,
                                      Txt_Institutions,Txt_only_teachers);
         break;
      case For_FORUM_GLOBAL_USRS:
         Sta_WriteForumTitleAndStats (ForumType,InsCod,CtrCod,DegCod,CrsCod,
                                      "ballon16x16.gif",StatsForum,
                                      Txt_General,"");
         break;
      case For_FORUM_GLOBAL_TCHS:
         Sta_WriteForumTitleAndStats (ForumType,InsCod,CtrCod,DegCod,CrsCod,
                                      "ballon16x16.gif",StatsForum,
                                      Txt_General,Txt_only_teachers);
         break;
      case For_FORUM_SWAD_USRS:
         Sta_WriteForumTitleAndStats (ForumType,InsCod,CtrCod,DegCod,CrsCod,
                                      "swad16x16.gif",StatsForum,
                                      Cfg_PLATFORM_SHORT_NAME,"");
         break;
      case For_FORUM_SWAD_TCHS:
         Sta_WriteForumTitleAndStats (ForumType,InsCod,CtrCod,DegCod,CrsCod,
                                      "swad16x16.gif",StatsForum,
                                      Cfg_PLATFORM_SHORT_NAME,Txt_only_teachers);
         break;
     }
  }

/*****************************************************************************/
/******************* Write title and stats of a forum type *******************/
/*****************************************************************************/

static void Sta_WriteForumTitleAndStats (For_ForumType_t ForumType,
                                         long InsCod,long CtrCod,long DegCod,long CrsCod,
                                         const char *Icon,struct Sta_StatsForum *StatsForum,
                                         const char *ForumName1,const char *ForumName2)
  {
   unsigned NumForums;
   unsigned NumThreads;
   unsigned NumPosts;
   unsigned NumUsrsToBeNotifiedByEMail;
   float NumThrsPerForum;
   float NumPostsPerThread;
   float NumPostsPerForum;

   /***** Compute number of forums, number of threads and number of posts *****/
   NumForums  = For_GetNumTotalForumsOfType       (ForumType,InsCod,CtrCod,DegCod,CrsCod);
   NumThreads = For_GetNumTotalThrsInForumsOfType (ForumType,InsCod,CtrCod,DegCod,CrsCod);
   NumPosts   = For_GetNumTotalPstsInForumsOfType (ForumType,InsCod,CtrCod,DegCod,CrsCod,&NumUsrsToBeNotifiedByEMail);

   /***** Compute number of threads per forum, number of posts per forum and number of posts per thread *****/
   NumThrsPerForum = (NumForums ? (float) NumThreads / (float) NumForums :
	                          0.0);
   NumPostsPerThread = (NumThreads ? (float) NumPosts / (float) NumThreads :
	                             0.0);
   NumPostsPerForum = (NumForums ? (float) NumPosts / (float) NumForums :
	                           0.0);

   /***** Update total stats *****/
   StatsForum->NumForums                  += NumForums;
   StatsForum->NumThreads                 += NumThreads;
   StatsForum->NumPosts                   += NumPosts;
   StatsForum->NumUsrsToBeNotifiedByEMail += NumUsrsToBeNotifiedByEMail;

   /***** Write forum name and stats *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td style=\"width:16px;"
                      " text-align:left; vertical-align:top;\">"
                      "<img src=\"%s/%s\" alt=\"\" class=\"ICON16x16\" />"
                      "</td>"
                      "<td class=\"DAT\""
                      " style=\"text-align:left; vertical-align:top;\">"
                      "%s%s"
                      "</td>"
                      "<td class=\"DAT\""
                      " style=\"text-align:right; vertical-align:top;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\""
                      " style=\"text-align:right; vertical-align:top;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\""
                      " style=\"text-align:right; vertical-align:top;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\""
                      " style=\"text-align:right; vertical-align:top;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\""
                      " style=\"text-align:right; vertical-align:top;\">"
                      "%.2f"
                      "</td>"
                      "<td class=\"DAT\""
                      " style=\"text-align:right; vertical-align:top;\">"
                      "%.2f"
                      "</td>"
                      "<td class=\"DAT\""
                      " style=\"text-align:right; vertical-align:top;\">"
                      "%.2f"
                      "</td>"
                      "</tr>",
            Gbl.Prefs.IconsURL,Icon,
            ForumName1,ForumName2,
            NumForums,NumThreads,NumPosts,NumUsrsToBeNotifiedByEMail,
            NumThrsPerForum,NumPostsPerThread,NumPostsPerForum);
  }

/*****************************************************************************/
/******************* Write title and stats of a forum type *******************/
/*****************************************************************************/

static void Sta_WriteForumTotalStats (struct Sta_StatsForum *StatsForum)
  {
   extern const char *Txt_Total;
   float NumThrsPerForum;
   float NumPostsPerThread;
   float NumPostsPerForum;
   char *StyleTableCell = " border-style:solid none none none;"
	                  " border-width:1px;";

   /***** Compute number of threads per forum, number of posts per forum and number of posts per thread *****/
   NumThrsPerForum  = (StatsForum->NumForums ? (float) StatsForum->NumThreads / (float) StatsForum->NumForums :
	                                       0.0);
   NumPostsPerThread = (StatsForum->NumThreads ? (float) StatsForum->NumPosts / (float) StatsForum->NumThreads :
	                                         0.0);
   NumPostsPerForum = (StatsForum->NumForums ? (float) StatsForum->NumPosts / (float) StatsForum->NumForums :
	                                       0.0);

   /***** Write forum name and stats *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td style=\"width:16px; %s\">"
                      "</td>"
                      "<td class=\"DAT_N\" style=\"text-align:left; vertical-align:top; %s\">"
                      "%s"
                      "</td>"
                      "<td class=\"DAT_N\" style=\"text-align:right; vertical-align:top; %s\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT_N\" style=\"text-align:right; vertical-align:top; %s\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT_N\" style=\"text-align:right; vertical-align:top; %s\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT_N\" style=\"text-align:right; vertical-align:top; %s\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT_N\" style=\"text-align:right; vertical-align:top; %s\">"
                      "%.2f"
                      "</td>"
                      "<td class=\"DAT_N\" style=\"text-align:right; vertical-align:top; %s\">"
                      "%.2f"
                      "</td>"
                      "<td class=\"DAT_N\" style=\"text-align:right; vertical-align:top; %s\">"
                      "%.2f"
                      "</td>"
                      "</tr>",
            StyleTableCell,
            StyleTableCell,Txt_Total,
            StyleTableCell,StatsForum->NumForums,
            StyleTableCell,StatsForum->NumThreads,
            StyleTableCell,StatsForum->NumPosts,
            StyleTableCell,StatsForum->NumUsrsToBeNotifiedByEMail,
            StyleTableCell,NumThrsPerForum,
            StyleTableCell,NumPostsPerThread,
            StyleTableCell,NumPostsPerForum);
  }

/*****************************************************************************/
/***************************** Show stats of surveys *************************/
/*****************************************************************************/

static void Sta_GetAndShowSurveysStats (void)
  {
   extern const char *Txt_STAT_USE_STAT_TYPES[Sta_NUM_TYPES_USE_STATS];
   extern const char *Txt_Number_of_BR_surveys;
   extern const char *Txt_Number_of_BR_courses_with_BR_surveys;
   extern const char *Txt_Average_number_BR_of_surveys_BR_per_course;
   extern const char *Txt_Average_number_BR_of_questions_BR_per_survey;
   extern const char *Txt_Number_of_BR_notifications;
   unsigned NumSurveys;
   unsigned NumNotif;
   unsigned NumCoursesWithSurveys = 0;
   float NumSurveysPerCourse = 0.0;
   float NumQstsPerSurvey = 0.0;

   /***** Get the number of surveys and the average number of questions per survey from this location
          (all the platform, current degree or current course) *****/
   if ((NumSurveys = Svy_GetNumSurveys (Gbl.Scope.Current,&NumNotif)))
     {
      if ((NumCoursesWithSurveys = Svy_GetNumCoursesWithSurveys (Gbl.Scope.Current)) != 0)
         NumSurveysPerCourse = (float) NumSurveys / (float) NumCoursesWithSurveys;
      NumQstsPerSurvey = Svy_GetNumQstsPerSurvey (Gbl.Scope.Current);
     }

   /***** Table start *****/
   Lay_StartRoundFrameTable10 (NULL,2,Txt_STAT_USE_STAT_TYPES[Sta_SURVEYS]);

   /***** Write table heading *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "</tr>",
            Txt_Number_of_BR_surveys,
            Txt_Number_of_BR_courses_with_BR_surveys,
            Txt_Average_number_BR_of_surveys_BR_per_course,
            Txt_Average_number_BR_of_questions_BR_per_survey,
            Txt_Number_of_BR_notifications);

   /***** Write number of surveys *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%.2f"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%.2f"
                      "</td>"
                      "<td class=\"DAT\" style=\"text-align:right;\">"
                      "%u"
                      "</td>"
                      "</tr>",
            NumSurveys,
            NumCoursesWithSurveys,
            NumSurveysPerCourse,
            NumQstsPerSurvey,
            NumNotif);

   /***** End table *****/
   Lay_EndRoundFrameTable10 ();
  }

/*****************************************************************************/
/********* Get and show number of users who have chosen a language ***********/
/*****************************************************************************/

static void Sta_GetAndShowNumUsrsPerLanguage (void)
  {
   extern const char *Txt_STAT_USE_STAT_TYPES[Sta_NUM_TYPES_USE_STATS];
   extern const char *Txt_Language;
   extern const char *Txt_STR_LANG_ID[Txt_NUM_LANGUAGES];
   extern const char *Txt_STR_LANG_NAME[Txt_NUM_LANGUAGES];
   extern const char *Txt_No_of_users;
   extern const char *Txt_PERCENT_of_users;
   Txt_Language_t Lan;
   char Query[1024];
   unsigned NumUsrs[Txt_NUM_LANGUAGES];
   unsigned NumUsrsTotal = 0;

   Lay_StartRoundFrameTable10 (NULL,2,Txt_STAT_USE_STAT_TYPES[Sta_LANGUAGES]);

   /***** Heading row *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<th class=\"TIT_TBL\" style=\"text-align:left;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "</tr>",
            Txt_Language,
            Txt_No_of_users,
            Txt_PERCENT_of_users);

   /***** For each language... *****/
   for (Lan = (Txt_Language_t) 0;
	Lan < Txt_NUM_LANGUAGES;
	Lan++)
     {
      /***** Get the number of users who have chosen this language from database *****/
      switch (Gbl.Scope.Current)
        {
         case Sco_SCOPE_PLATFORM:
            sprintf (Query,"SELECT COUNT(*)"
        	           " FROM usr_data WHERE Language='%s'",
        	     Txt_STR_LANG_ID[Lan]);
            break;
	 case Sco_SCOPE_INSTITUTION:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
        	           " FROM centres,degrees,courses,crs_usr,usr_data"
                           " WHERE centres.InsCod='%ld'"
                           " AND centres.CtrCod=degrees.CtrCod"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.Language='%s'",
                     Gbl.CurrentIns.Ins.InsCod,
                     Txt_STR_LANG_ID[Lan]);
            break;
         case Sco_SCOPE_CENTRE:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
        	           " FROM degrees,courses,crs_usr,usr_data"
                           " WHERE degrees.CtrCod='%ld'"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.Language='%s'",
                     Gbl.CurrentCtr.Ctr.CtrCod,
                     Txt_STR_LANG_ID[Lan]);
            break;
         case Sco_SCOPE_DEGREE:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
        	           " FROM courses,crs_usr,usr_data"
                           " WHERE courses.DegCod='%ld'"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.Language='%s'",
                     Gbl.CurrentDeg.Deg.DegCod,
                     Txt_STR_LANG_ID[Lan]);
            break;
         case Sco_SCOPE_COURSE:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
        	           " FROM crs_usr,usr_data"
                           " WHERE crs_usr.CrsCod='%ld'"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.Language='%s'",
                     Gbl.CurrentCrs.Crs.CrsCod,
                     Txt_STR_LANG_ID[Lan]);
            break;
	 default:
	    Lay_ShowErrorAndExit ("Wrong scope.");
	    break;
        }
      NumUsrs[Lan] = (unsigned) DB_QueryCOUNT (Query,"can not get the number of users who have chosen a language");

      /* Update total number of users */
      NumUsrsTotal += NumUsrs[Lan];
     }

   /***** Write number of users who have chosen each language *****/
   for (Lan = (Txt_Language_t) 0;
	Lan < Txt_NUM_LANGUAGES;
	Lan++)
      fprintf (Gbl.F.Out,"<tr>"
                         "<td class=\"DAT\" style=\"text-align:left;\">"
                         "%s"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%u"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%5.2f%%"
                         "</td>"
                         "</tr>",
               Txt_STR_LANG_NAME[Lan],NumUsrs[Lan],
               NumUsrsTotal ? (float) NumUsrs[Lan] * 100.0 /
        	              (float) NumUsrsTotal :
        	              0);

   Lay_EndRoundFrameTable10 ();
  }

/*****************************************************************************/
/********* Get and show number of users who have chosen a layout *************/
/*****************************************************************************/

static void Sta_GetAndShowNumUsrsPerLayout (void)
  {
   extern const char *Lay_LayoutIcons[Lay_NUM_LAYOUTS];
   extern const char *Txt_STAT_USE_STAT_TYPES[Sta_NUM_TYPES_USE_STATS];
   extern const char *Txt_Layout;
   extern const char *Txt_No_of_users;
   extern const char *Txt_PERCENT_of_users;
   extern const char *Txt_LAYOUT_NAMES[Lay_NUM_LAYOUTS];
   Lay_Layout_t Layout;
   char Query[1024];
   unsigned NumUsrs[Lay_NUM_LAYOUTS];
   unsigned NumUsrsTotal = 0;

   Lay_StartRoundFrameTable10 (NULL,2,Txt_STAT_USE_STAT_TYPES[Sta_LAYOUTS]);

   /***** Heading row *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<th class=\"TIT_TBL\" style=\"text-align:left;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "</tr>",
            Txt_Layout,
            Txt_No_of_users,
            Txt_PERCENT_of_users);

   /***** For each layout... *****/
   for (Layout = (Lay_Layout_t) 0;
	Layout < Lay_NUM_LAYOUTS;
	Layout++)
     {
      /***** Get number of users who have chosen this layout from database *****/
      switch (Gbl.Scope.Current)
        {
         case Sco_SCOPE_PLATFORM:
            sprintf (Query,"SELECT COUNT(*) FROM usr_data"
        	           " WHERE Layout='%u'",
                     (unsigned) Layout);
            break;
	 case Sco_SCOPE_INSTITUTION:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
                           " FROM centres,degrees,courses,crs_usr,usr_data"
                           " WHERE centres.InsCod='%ld'"
                           " AND centres.CtrCod=degrees.CtrCod"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.Layout='%u'",
                     Gbl.CurrentIns.Ins.InsCod,(unsigned) Layout);
            break;
         case Sco_SCOPE_CENTRE:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
                           " FROM degrees,courses,crs_usr,usr_data"
                           " WHERE degrees.CtrCod='%ld'"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.Layout='%u'",
                     Gbl.CurrentCtr.Ctr.CtrCod,(unsigned) Layout);
            break;
         case Sco_SCOPE_DEGREE:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
                           " FROM courses,crs_usr,usr_data"
                           " WHERE courses.DegCod='%ld'"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.Layout='%u'",
                     Gbl.CurrentDeg.Deg.DegCod,(unsigned) Layout);
            break;
         case Sco_SCOPE_COURSE:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
                           " FROM crs_usr,usr_data"
                           " WHERE crs_usr.CrsCod='%ld'"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.Layout='%u'",
                     Gbl.CurrentCrs.Crs.CrsCod,(unsigned) Layout);
            break;
	 default:
	    Lay_ShowErrorAndExit ("Wrong scope.");
	    break;
        }
      NumUsrs[Layout] = (unsigned) DB_QueryCOUNT (Query,"can not get the number of users who have chosen a layout");

      /* Update total number of users */
      NumUsrsTotal += NumUsrs[Layout];
     }

   /***** Write number of users who have chosen each layout *****/
   for (Layout = (Lay_Layout_t) 0;
	Layout < Lay_NUM_LAYOUTS;
	Layout++)
      fprintf (Gbl.F.Out,"<tr>"
                         "<td style=\"text-align:center;\">"
                         "<img src=\"%s/%s32x32.gif\" alt=\"%s\" class=\"ICON32x32\" />"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%u"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%5.2f%%"
                         "</td>"
                         "</tr>",
               Gbl.Prefs.IconsURL,Lay_LayoutIcons[Layout],Txt_LAYOUT_NAMES[Layout],
               NumUsrs[Layout],
               NumUsrsTotal ? (float) NumUsrs[Layout] * 100.0 /
        	              (float) NumUsrsTotal :
        	              0);

   Lay_EndRoundFrameTable10 ();
  }

/*****************************************************************************/
/********** Get and show number of users who have chosen a theme *************/
/*****************************************************************************/

static void Sta_GetAndShowNumUsrsPerTheme (void)
  {
   extern const char *The_ThemeId[The_NUM_THEMES];
   extern const char *The_ThemeNames[The_NUM_THEMES];
   extern const char *Txt_STAT_USE_STAT_TYPES[Sta_NUM_TYPES_USE_STATS];
   extern const char *Txt_Theme_SKIN;
   extern const char *Txt_No_of_users;
   extern const char *Txt_PERCENT_of_users;
   The_Theme_t Theme;
   char Query[1024];
   unsigned NumUsrs[The_NUM_THEMES];
   unsigned NumUsrsTotal = 0;

   Lay_StartRoundFrameTable10 (NULL,2,Txt_STAT_USE_STAT_TYPES[Sta_THEMES]);

   /***** Heading row *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<th class=\"TIT_TBL\" style=\"text-align:left;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "</tr>",
            Txt_Theme_SKIN,
            Txt_No_of_users,
            Txt_PERCENT_of_users);

   /***** For each theme... *****/
   for (Theme = (The_Theme_t) 0;
	Theme < The_NUM_THEMES;
	Theme++)
     {
      /***** Get number of users who have chosen this theme from database *****/
      switch (Gbl.Scope.Current)
        {
         case Sco_SCOPE_PLATFORM:
            sprintf (Query,"SELECT COUNT(*) FROM usr_data"
        	           " WHERE Theme='%s'",
                     The_ThemeId[Theme]);
            break;
	 case Sco_SCOPE_INSTITUTION:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
                           " FROM centres,degrees,courses,crs_usr,usr_data"
                           " WHERE centres.InsCod='%ld'"
                           " AND centres.CtrCod=degrees.CtrCod"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.Theme='%s'",
                     Gbl.CurrentIns.Ins.InsCod,The_ThemeId[Theme]);
            break;
         case Sco_SCOPE_CENTRE:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
                           " FROM degrees,courses,crs_usr,usr_data"
                           " WHERE degrees.CtrCod='%ld'"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.Theme='%s'",
                     Gbl.CurrentCtr.Ctr.CtrCod,The_ThemeId[Theme]);
            break;
         case Sco_SCOPE_DEGREE:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
                           " FROM courses,crs_usr,usr_data"
                           " WHERE courses.DegCod='%ld'"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.Theme='%s'",
                     Gbl.CurrentDeg.Deg.DegCod,The_ThemeId[Theme]);
            break;
         case Sco_SCOPE_COURSE:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
                           " FROM crs_usr,usr_data"
                           " WHERE crs_usr.CrsCod='%ld'"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.Theme='%s'",
                     Gbl.CurrentCrs.Crs.CrsCod,The_ThemeId[Theme]);
            break;
	 default:
	    Lay_ShowErrorAndExit ("Wrong scope.");
	    break;
        }
      NumUsrs[Theme] = (unsigned) DB_QueryCOUNT (Query,"can not get the number of users who have chosen a theme");

      /* Update total number of users */
      NumUsrsTotal += NumUsrs[Theme];
     }

   /***** Write number of users who have chosen each theme *****/
   for (Theme = (The_Theme_t) 0;
	Theme < The_NUM_THEMES;
	Theme++)
      fprintf (Gbl.F.Out,"<tr>"
                         "<td style=\"text-align:center;\">"
                         "<img src=\"%s/%s/%s/theme_32x20.gif\" alt=\"%s\""
                         " style=\"width:32px; height:20px;\" />"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%u"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%5.2f%%"
                         "</td>"
                         "</tr>",
               Gbl.Prefs.IconsURL,Cfg_ICON_FOLDER_THEMES,The_ThemeId[Theme],The_ThemeNames[Theme],
               NumUsrs[Theme],
               NumUsrsTotal ? (float) NumUsrs[Theme] * 100.0 /
        	              (float) NumUsrsTotal :
        	              0);

   Lay_EndRoundFrameTable10 ();
  }

/*****************************************************************************/
/********* Get and show number of users who have chosen an icon set **********/
/*****************************************************************************/

static void Sta_GetAndShowNumUsrsPerIconSet (void)
  {
   extern const char *Ico_IconSetId[Ico_NUM_ICON_SETS];
   extern const char *Ico_IconSetNames[Ico_NUM_ICON_SETS];
   extern const char *Txt_STAT_USE_STAT_TYPES[Sta_NUM_TYPES_USE_STATS];
   extern const char *Txt_Icons;
   extern const char *Txt_No_of_users;
   extern const char *Txt_PERCENT_of_users;
   Ico_IconSet_t IconSet;
   char Query[1024];
   unsigned NumUsrs[Ico_NUM_ICON_SETS];
   unsigned NumUsrsTotal = 0;

   Lay_StartRoundFrameTable10 (NULL,2,Txt_STAT_USE_STAT_TYPES[Sta_ICON_SETS]);

   /***** Heading row *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<th class=\"TIT_TBL\" style=\"text-align:left;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "</tr>",
            Txt_Icons,
            Txt_No_of_users,
            Txt_PERCENT_of_users);

   /***** For each icon set... *****/
   for (IconSet = (Ico_IconSet_t) 0;
	IconSet < Ico_NUM_ICON_SETS;
	IconSet++)
     {
      /***** Get the number of users who have chosen this icon set from database *****/
      switch (Gbl.Scope.Current)
        {
         case Sco_SCOPE_PLATFORM:
            sprintf (Query,"SELECT COUNT(*) FROM usr_data WHERE IconSet='%s'",
        	     Ico_IconSetId[IconSet]);
            break;
	 case Sco_SCOPE_INSTITUTION:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
        	           " FROM centres,degrees,courses,crs_usr,usr_data"
                           " WHERE centres.InsCod='%ld'"
                           " AND centres.CtrCod=degrees.CtrCod"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.IconSet='%s'",
                     Gbl.CurrentIns.Ins.InsCod,Ico_IconSetId[IconSet]);
            break;
         case Sco_SCOPE_CENTRE:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
        	           " FROM degrees,courses,crs_usr,usr_data"
                           " WHERE degrees.CtrCod='%ld'"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.IconSet='%s'",
                     Gbl.CurrentCtr.Ctr.CtrCod,Ico_IconSetId[IconSet]);
            break;
         case Sco_SCOPE_DEGREE:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
        	           " FROM courses,crs_usr,usr_data"
                           " WHERE courses.DegCod='%ld'"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.IconSet='%s'",
                     Gbl.CurrentDeg.Deg.DegCod,Ico_IconSetId[IconSet]);
            break;
         case Sco_SCOPE_COURSE:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
        	           " FROM crs_usr,usr_data"
                           " WHERE crs_usr.CrsCod='%ld'"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.IconSet='%s'",
                     Gbl.CurrentCrs.Crs.CrsCod,Ico_IconSetId[IconSet]);
            break;
	 default:
	    Lay_ShowErrorAndExit ("Wrong scope.");
	    break;
        }
      NumUsrs[IconSet] = (unsigned) DB_QueryCOUNT (Query,"can not get the number of users who have chosen an icon set");

      /* Update total number of users */
      NumUsrsTotal += NumUsrs[IconSet];
     }

   /***** Write number of users who have chosen each icon set *****/
   for (IconSet = (Ico_IconSet_t) 0;
	IconSet < Ico_NUM_ICON_SETS;
	IconSet++)
      fprintf (Gbl.F.Out,"<tr>"
                         "<td style=\"text-align:left;\">"
                         "<img src=\"%s/%s/%s/%s/heart64x64.gif\" alt=\"%s\""
                         " class=\"ICON32x32\" />"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%u"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%5.2f%%"
                         "</td>"
                         "</tr>",
               Gbl.Prefs.IconsURL,
               Cfg_ICON_FOLDER_ICON_SETS,
               Ico_IconSetId[IconSet],
               Cfg_ICON_ACTION,
               Ico_IconSetNames[IconSet],
               NumUsrs[IconSet],
               NumUsrsTotal ? (float) NumUsrs[IconSet] * 100.0 /
        	              (float) NumUsrsTotal :
        	              0);

   Lay_EndRoundFrameTable10 ();
  }

/*****************************************************************************/
/*********** Get and show number of users who have chosen a menu *************/
/*****************************************************************************/

static void Sta_GetAndShowNumUsrsPerMenu (void)
  {
   extern const char *Mnu_MenuIcons[Mnu_NUM_MENUS];
   extern const char *Txt_STAT_USE_STAT_TYPES[Sta_NUM_TYPES_USE_STATS];
   extern const char *Txt_Menu;
   extern const char *Txt_No_of_users;
   extern const char *Txt_PERCENT_of_users;
   extern const char *Txt_MENU_NAMES[Mnu_NUM_MENUS];
   Mnu_Menu_t Menu;
   char Query[1024];
   unsigned NumUsrs[Mnu_NUM_MENUS];
   unsigned NumUsrsTotal = 0;

   Lay_StartRoundFrameTable10 (NULL,2,Txt_STAT_USE_STAT_TYPES[Sta_MENUS]);

   /***** Heading row *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<th class=\"TIT_TBL\" style=\"text-align:left;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "</tr>",
            Txt_Menu,
            Txt_No_of_users,
            Txt_PERCENT_of_users);

   /***** For each menu... *****/
   for (Menu = (Mnu_Menu_t) 0;
	Menu < Mnu_NUM_MENUS;
	Menu++)
     {
      /***** Get number of users who have chosen this menu from database *****/
      switch (Gbl.Scope.Current)
        {
         case Sco_SCOPE_PLATFORM:
            sprintf (Query,"SELECT COUNT(*) FROM usr_data"
        	           " WHERE Menu='%u'",
                     (unsigned) Menu);
            break;
	 case Sco_SCOPE_INSTITUTION:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
                           " FROM centres,degrees,courses,crs_usr,usr_data"
                           " WHERE centres.InsCod='%ld'"
                           " AND centres.CtrCod=degrees.CtrCod"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.Menu='%u'",
                     Gbl.CurrentIns.Ins.InsCod,(unsigned) Menu);
            break;
         case Sco_SCOPE_CENTRE:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
                           " FROM degrees,courses,crs_usr,usr_data"
                           " WHERE degrees.CtrCod='%ld'"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.Menu='%u'",
                     Gbl.CurrentCtr.Ctr.CtrCod,(unsigned) Menu);
            break;
         case Sco_SCOPE_DEGREE:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
                           " FROM courses,crs_usr,usr_data"
                           " WHERE courses.DegCod='%ld'"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.Menu='%u'",
                     Gbl.CurrentDeg.Deg.DegCod,(unsigned) Menu);
            break;
         case Sco_SCOPE_COURSE:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
                           " FROM crs_usr,usr_data"
                           " WHERE crs_usr.CrsCod='%ld'"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.Menu='%u'",
                     Gbl.CurrentCrs.Crs.CrsCod,(unsigned) Menu);
            break;
	 default:
	    Lay_ShowErrorAndExit ("Wrong scope.");
	    break;
        }
      NumUsrs[Menu] = (unsigned) DB_QueryCOUNT (Query,"can not get the number of users who have chosen a menu");

      /* Update total number of users */
      NumUsrsTotal += NumUsrs[Menu];
     }

   /***** Write number of users who have chosen each menu *****/
   for (Menu = (Mnu_Menu_t) 0;
	Menu < Mnu_NUM_MENUS;
	Menu++)
      fprintf (Gbl.F.Out,"<tr>"
                         "<td style=\"text-align:center;\">"
                         "<img src=\"%s/%s32x32.gif\" alt=\"%s\" class=\"ICON32x32\" />"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%u"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%5.2f%%"
                         "</td>"
                         "</tr>",
               Gbl.Prefs.IconsURL,Mnu_MenuIcons[Menu],Txt_MENU_NAMES[Menu],
               NumUsrs[Menu],
               NumUsrsTotal ? (float) NumUsrs[Menu] * 100.0 /
        	              (float) NumUsrsTotal :
        	              0);

   Lay_EndRoundFrameTable10 ();
  }

/*****************************************************************************/
/***** Get and show number of users who have chosen a layout of columns ******/
/*****************************************************************************/

static void Sta_GetAndShowNumUsrsPerSideColumns (void)
  {
   extern const char *Txt_STAT_USE_STAT_TYPES[Sta_NUM_TYPES_USE_STATS];
   extern const char *Txt_Columns;
   extern const char *Txt_No_of_users;
   extern const char *Txt_PERCENT_of_users;
   unsigned SideCols;
   char Query[1024];
   unsigned NumUsrs[4];
   unsigned NumUsrsTotal = 0;
   extern const char *Txt_LAYOUT_SIDE_COLUMNS[4];

   Lay_StartRoundFrameTable10 (NULL,2,Txt_STAT_USE_STAT_TYPES[Sta_SIDE_COLUMNS]);

   /***** Heading row *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<th class=\"TIT_TBL\" style=\"text-align:center;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "</tr>",
            Txt_Columns,
            Txt_No_of_users,
            Txt_PERCENT_of_users);

   /***** For each language... *****/
   for (SideCols = 0;
	SideCols <= Lay_SHOW_BOTH_COLUMNS;
	SideCols++)
     {
      /***** Get the number of users who have chosen this layout of columns from database *****/
      switch (Gbl.Scope.Current)
        {
         case Sco_SCOPE_PLATFORM:
            sprintf (Query,"SELECT COUNT(*) FROM usr_data WHERE SideCols='%u'",
                     SideCols);
            break;
	 case Sco_SCOPE_INSTITUTION:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
        	           " FROM centres,degrees,courses,crs_usr,usr_data"
                           " WHERE centres.InsCod='%ld'"
                           " AND centres.CtrCod=degrees.CtrCod"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.SideCols='%u'",
                     Gbl.CurrentIns.Ins.InsCod,SideCols);
            break;
         case Sco_SCOPE_CENTRE:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
        	           " FROM degrees,courses,crs_usr,usr_data"
                           " WHERE degrees.CtrCod='%ld'"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.SideCols='%u'",
                     Gbl.CurrentCtr.Ctr.CtrCod,SideCols);
            break;
         case Sco_SCOPE_DEGREE:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
        	           " FROM courses,crs_usr,usr_data"
                           " WHERE courses.DegCod='%ld'"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.SideCols='%u'",
                     Gbl.CurrentDeg.Deg.DegCod,SideCols);
            break;
         case Sco_SCOPE_COURSE:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
        	           " FROM crs_usr,usr_data"
                           " WHERE crs_usr.CrsCod='%ld'"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND usr_data.SideCols='%u'",
                     Gbl.CurrentCrs.Crs.CrsCod,SideCols);
            break;
	 default:
	    Lay_ShowErrorAndExit ("Wrong scope.");
	    break;
        }
      NumUsrs[SideCols] = (unsigned) DB_QueryCOUNT (Query,"can not get the number of users who have chosen a layout of columns");

      /* Update total number of users */
      NumUsrsTotal += NumUsrs[SideCols];
     }

   /***** Write number of users who have chosen this layout of columns *****/
   for (SideCols = 0;
	SideCols <= Lay_SHOW_BOTH_COLUMNS;
	SideCols++)
      fprintf (Gbl.F.Out,"<tr>"
                         "<td style=\"text-align:center;\">"
                         "<img src=\"%s/layout%u%u_32x20.gif\" alt=\"%s\""
                         " style=\"width:32px; height:20px;\" />"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%u"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%5.2f%%"
                         "</td>"
                         "</tr>",
               Gbl.Prefs.IconsURL,SideCols >> 1,SideCols & 1,
               Txt_LAYOUT_SIDE_COLUMNS[SideCols],
               NumUsrs[SideCols],
               NumUsrsTotal ? (float) NumUsrs[SideCols] * 100.0 /
        	              (float) NumUsrsTotal :
        	              0);

   Lay_EndRoundFrameTable10 ();
  }

/*****************************************************************************/
/****** Get and show number of users who want to be notified by e-mail *******/
/*****************************************************************************/

static void Sta_GetAndShowNumUsrsPerNotifyEvent (void)
  {
   extern const char *Txt_STAT_USE_STAT_TYPES[Sta_NUM_TYPES_USE_STATS];
   extern const char *Txt_Event;
   extern const char *Txt_NOTIFY_EVENTS_PLURAL[Ntf_NUM_NOTIFY_EVENTS];
   extern const char *Txt_No_of_users;
   extern const char *Txt_PERCENT_of_users;
   extern const char *Txt_Number_of_BR_events;
   extern const char *Txt_Number_of_BR_e_mails;
   extern const char *Txt_Total;
   Ntf_NotifyEvent_t NotifyEvent;
   char Query[1024];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumUsrsTotalInPlatform;
   unsigned NumUsrsTotalWhoWantToBeNotifiedByEMailAboutSomeEvent;
   unsigned NumUsrs[Ntf_NUM_NOTIFY_EVENTS];
   unsigned NumEventsTotal = 0;
   unsigned NumEvents[Ntf_NUM_NOTIFY_EVENTS];
   unsigned NumMailsTotal = 0;
   unsigned NumMails[Ntf_NUM_NOTIFY_EVENTS];
   char *StyleTableCell = " border-style:solid none none none;"
	                  " border-width:1px;";

   Lay_StartRoundFrameTable10 (NULL,2,Txt_STAT_USE_STAT_TYPES[Sta_NOTIFY_EVENTS]);

   /***** Heading row *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<th class=\"TIT_TBL\" style=\"text-align:left;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "<th class=\"TIT_TBL\" style=\"text-align:right;\">"
                      "%s"
                      "</th>"
                      "</tr>",
            Txt_Event,
            Txt_No_of_users,
            Txt_PERCENT_of_users,
            Txt_Number_of_BR_events,
            Txt_Number_of_BR_e_mails);

   /***** Get total number of users in platform *****/
   NumUsrsTotalInPlatform = Sta_GetTotalNumberOfUsers (Gbl.Scope.Current,Rol_ROLE_UNKNOWN);

   /***** Get total number of users who want to be notified by e-mail on some event, from database *****/
   switch (Gbl.Scope.Current)
     {
      case Sco_SCOPE_PLATFORM:
         sprintf (Query,"SELECT COUNT(*) FROM usr_data WHERE EmailNtfEvents<>0");
         break;
      case Sco_SCOPE_INSTITUTION:
         sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
                        " FROM centres,degrees,courses,crs_usr,usr_data"
                        " WHERE centres.InsCod='%ld'"
                        " AND centres.CtrCod=degrees.CtrCod"
                        " AND degrees.DegCod=courses.DegCod"
                        " AND courses.CrsCod=crs_usr.CrsCod"
                        " AND crs_usr.UsrCod=usr_data.UsrCod"
                        " AND usr_data.EmailNtfEvents<>0",
                  Gbl.CurrentIns.Ins.InsCod);
         break;
      case Sco_SCOPE_CENTRE:
         sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
                        " FROM degrees,courses,crs_usr,usr_data"
                        " WHERE degrees.CtrCod='%ld'"
                        " AND degrees.DegCod=courses.DegCod"
                        " AND courses.CrsCod=crs_usr.CrsCod"
                        " AND crs_usr.UsrCod=usr_data.UsrCod"
                        " AND usr_data.EmailNtfEvents<>0",
                  Gbl.CurrentCtr.Ctr.CtrCod);
         break;
      case Sco_SCOPE_DEGREE:
         sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
                        " FROM courses,crs_usr,usr_data"
                        " WHERE courses.DegCod='%ld'"
                        " AND courses.CrsCod=crs_usr.CrsCod"
                        " AND crs_usr.UsrCod=usr_data.UsrCod"
                        " AND usr_data.EmailNtfEvents<>0",
                  Gbl.CurrentDeg.Deg.DegCod);
         break;
      case Sco_SCOPE_COURSE:
         sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
                        " FROM crs_usr,usr_data"
                        " WHERE crs_usr.CrsCod='%ld'"
                        " AND crs_usr.UsrCod=usr_data.UsrCod"
                        " AND usr_data.EmailNtfEvents<>0",
                  Gbl.CurrentCrs.Crs.CrsCod);
         break;
      default:
	 Lay_ShowErrorAndExit ("Wrong scope.");
	 break;
     }
   NumUsrsTotalWhoWantToBeNotifiedByEMailAboutSomeEvent = (unsigned) DB_QueryCOUNT (Query,"can not get the total number of users who want to be notified by e-mail on some event");

   /***** For each notify event... *****/
   for (NotifyEvent = (Ntf_NotifyEvent_t) 1;
	NotifyEvent < Ntf_NUM_NOTIFY_EVENTS;
	NotifyEvent++) // 0 is reserved for Ntf_EVENT_UNKNOWN
     {
      /***** Get the number of users who want to be notified by e-mail on this event, from database *****/
      switch (Gbl.Scope.Current)
        {
         case Sco_SCOPE_PLATFORM:
            sprintf (Query,"SELECT COUNT(*) FROM usr_data WHERE ((EmailNtfEvents & %u)<>0)",
                     (1 << NotifyEvent));
            break;
	 case Sco_SCOPE_INSTITUTION:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
        	           " FROM centres,degrees,courses,crs_usr,usr_data"
                           " WHERE centres.InsCod='%ld'"
                           " AND centres.CtrCod=degrees.CtrCod"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND ((usr_data.EmailNtfEvents & %u)<>0)",
                     Gbl.CurrentIns.Ins.InsCod,(1 << NotifyEvent));
            break;
         case Sco_SCOPE_CENTRE:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
        	           " FROM degrees,courses,crs_usr,usr_data"
                           " WHERE degrees.CtrCod='%ld'"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND ((usr_data.EmailNtfEvents & %u)<>0)",
                     Gbl.CurrentCtr.Ctr.CtrCod,(1 << NotifyEvent));
            break;
         case Sco_SCOPE_DEGREE:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
        	           " FROM courses,crs_usr,usr_data"
                           " WHERE courses.DegCod='%ld'"
                           " AND courses.CrsCod=crs_usr.CrsCod"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND ((usr_data.EmailNtfEvents & %u)<>0)",
                     Gbl.CurrentDeg.Deg.DegCod,(1 << NotifyEvent));
            break;
         case Sco_SCOPE_COURSE:
            sprintf (Query,"SELECT COUNT(DISTINCT usr_data.UsrCod)"
        	           " FROM crs_usr,usr_data"
                           " WHERE crs_usr.CrsCod='%ld'"
                           " AND crs_usr.UsrCod=usr_data.UsrCod"
                           " AND ((usr_data.EmailNtfEvents & %u)<>0)",
                     Gbl.CurrentCrs.Crs.CrsCod,(1 << NotifyEvent));
            break;
	 default:
	    Lay_ShowErrorAndExit ("Wrong scope.");
	    break;
        }
      NumUsrs[NotifyEvent] = (unsigned) DB_QueryCOUNT (Query,"can not get the number of users who want to be notified by e-mail on an event");

      /***** Get number of notifications by e-mail from database *****/
      switch (Gbl.Scope.Current)
        {
         case Sco_SCOPE_PLATFORM:
            sprintf (Query,"SELECT SUM(NumEvents),SUM(NumMails)"
                           " FROM sta_notif"
                           " WHERE NotifyEvent='%u'",
                     (unsigned) NotifyEvent);
            break;
	 case Sco_SCOPE_INSTITUTION:
            sprintf (Query,"SELECT SUM(sta_notif.NumEvents),SUM(sta_notif.NumMails)"
                           " FROM centres,degrees,sta_notif"
                           " WHERE centres.InsCod='%ld'"
                           " AND centres.CtrCod=degrees.CtrCod"
                           " AND degrees.DegCod=sta_notif.DegCod"
                           " AND sta_notif.NotifyEvent='%u'",
                     Gbl.CurrentIns.Ins.InsCod,(unsigned) NotifyEvent);
            break;
         case Sco_SCOPE_CENTRE:
            sprintf (Query,"SELECT SUM(sta_notif.NumEvents),SUM(sta_notif.NumMails)"
                           " FROM degrees,sta_notif"
                           " WHERE degrees.CtrCod='%ld'"
                           " AND degrees.DegCod=sta_notif.DegCod"
                           " AND sta_notif.NotifyEvent='%u'",
                     Gbl.CurrentCtr.Ctr.CtrCod,(unsigned) NotifyEvent);
            break;
         case Sco_SCOPE_DEGREE:
            sprintf (Query,"SELECT SUM(NumEvents),SUM(NumMails)"
                           " FROM sta_notif"
                           " WHERE DegCod='%ld'"
                           " AND NotifyEvent='%u'",
                     Gbl.CurrentDeg.Deg.DegCod,(unsigned) NotifyEvent);
            break;
         case Sco_SCOPE_COURSE:
            sprintf (Query,"SELECT SUM(NumEvents),SUM(NumMails)"
                           " FROM sta_notif"
                           " WHERE CrsCod='%ld'"
                           " AND NotifyEvent='%u'",
                     Gbl.CurrentCrs.Crs.CrsCod,(unsigned) NotifyEvent);
            break;
	 default:
	    Lay_ShowErrorAndExit ("Wrong scope.");
	    break;
        }
      DB_QuerySELECT (Query,&mysql_res,"can not get the number of notifications by e-mail");

      row = mysql_fetch_row (mysql_res);

      /* Get number of events notified */
      if (row[0])
        {
         if (sscanf (row[0],"%u",&NumEvents[NotifyEvent]) != 1)
            Lay_ShowErrorAndExit ("Error when getting the number of notifications by e-mail.");
        }
      else
         NumEvents[NotifyEvent] = 0;

      /* Get number of mails sent */
      if (row[1])
        {
         if (sscanf (row[1],"%u",&NumMails[NotifyEvent]) != 1)
            Lay_ShowErrorAndExit ("Error when getting the number of e-mails to notify events3.");
        }
      else
         NumMails[NotifyEvent] = 0;

      /* Free structure that stores the query result */
      DB_FreeMySQLResult (&mysql_res);

      /* Update total number of events and mails */
      NumEventsTotal += NumEvents[NotifyEvent];
      NumMailsTotal += NumMails[NotifyEvent];
     }

   /***** Write number of users who want to be notified by e-mail on each event *****/
   for (NotifyEvent = (Ntf_NotifyEvent_t) 1;
	NotifyEvent < Ntf_NUM_NOTIFY_EVENTS;
	NotifyEvent++) // 0 is reserved for Ntf_EVENT_UNKNOWN
      fprintf (Gbl.F.Out,"<tr>"
                         "<td class=\"DAT\" style=\"text-align:left;\">"
                         "%s"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%u"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%5.2f%%"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%u"
                         "</td>"
                         "<td class=\"DAT\" style=\"text-align:right;\">"
                         "%u"
                         "</td>"
                         "</tr>",
               Txt_NOTIFY_EVENTS_PLURAL[NotifyEvent],
               NumUsrs[NotifyEvent],
               NumUsrsTotalInPlatform ? (float) NumUsrs[NotifyEvent] * 100.0 /
        	                        (float) NumUsrsTotalInPlatform :
        	                        0.0,
               NumEvents[NotifyEvent],
               NumMails[NotifyEvent]);

   /***** Write total number of users who want to be notified by e-mail on some event *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"DAT_N\" style=\"text-align:left; %s\">"
                      "%s"
                      "</td>"
                      "<td class=\"DAT_N\" style=\"text-align:right; %s\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT_N\" style=\"text-align:right; %s\">"
                      "%5.2f%%"
                      "</td>"
                      "<td class=\"DAT_N\" style=\"text-align:right; %s\">"
                      "%u"
                      "</td>"
                      "<td class=\"DAT_N\" style=\"text-align:right; %s\">"
                      "%u"
                      "</td>"
                      "</tr>",
            StyleTableCell,Txt_Total,
            StyleTableCell,NumUsrsTotalWhoWantToBeNotifiedByEMailAboutSomeEvent,
            StyleTableCell,NumUsrsTotalInPlatform ? (float) NumUsrsTotalWhoWantToBeNotifiedByEMailAboutSomeEvent * 100.0 /
        	                                    (float) NumUsrsTotalInPlatform :
        	                                    0.0,
            StyleTableCell,NumEventsTotal,
            StyleTableCell,NumMailsTotal);

   Lay_EndRoundFrameTable10 ();
  }

/*****************************************************************************/
/**************** Compute the time used to generate the page *****************/
/*****************************************************************************/

void Sta_ComputeTimeToGeneratePage (void)
  {
   if (gettimeofday (&Gbl.tvPageCreated, &Gbl.tz))
      // Error in gettimeofday
      Gbl.TimeGenerationInMicroseconds = 0;
   else
     {
      if (Gbl.tvPageCreated.tv_usec < Gbl.tvStart.tv_usec)
	{
	 Gbl.tvPageCreated.tv_sec--;
	 Gbl.tvPageCreated.tv_usec += 1000000;
	}
      Gbl.TimeGenerationInMicroseconds = (Gbl.tvPageCreated.tv_sec  - Gbl.tvStart.tv_sec) * 1000000L +
                                          Gbl.tvPageCreated.tv_usec - Gbl.tvStart.tv_usec;
     }
  }

/*****************************************************************************/
/****************** Compute the time used to send the page *******************/
/*****************************************************************************/

void Sta_ComputeTimeToSendPage (void)
  {
   if (gettimeofday (&Gbl.tvPageSent, &Gbl.tz))
      // Error in gettimeofday
      Gbl.TimeSendInMicroseconds = 0;
   else
     {
      if (Gbl.tvPageSent.tv_usec < Gbl.tvPageCreated.tv_usec)
	{
	 Gbl.tvPageSent.tv_sec--;
	 Gbl.tvPageSent.tv_usec += 1000000;
	}
      Gbl.TimeSendInMicroseconds = (Gbl.tvPageSent.tv_sec  - Gbl.tvPageCreated.tv_sec) * 1000000L +
                                    Gbl.tvPageSent.tv_usec - Gbl.tvPageCreated.tv_usec;
     }
  }

/*****************************************************************************/
/************** Write the time to generate and send the page *****************/
/*****************************************************************************/

void Sta_WriteTimeToGenerateAndSendPage (void)
  {
   extern const char *Txt_PAGE1_Page_generated_in;
   extern const char *Txt_PAGE2_and_sent_in;
   char StrTimeGenerationInMicroseconds[64];
   char StrTimeSendInMicroseconds[64];

   Sta_WriteTime (StrTimeGenerationInMicroseconds,Gbl.TimeGenerationInMicroseconds);
   Sta_WriteTime (StrTimeSendInMicroseconds,Gbl.TimeSendInMicroseconds);
   fprintf (Gbl.F.Out,"%s %s %s %s",
            Txt_PAGE1_Page_generated_in,StrTimeGenerationInMicroseconds,
            Txt_PAGE2_and_sent_in,StrTimeSendInMicroseconds);
  }

/*****************************************************************************/
/********* Write time (given in microseconds) depending on amount ************/
/*****************************************************************************/

void Sta_WriteTime (char *Str,long TimeInMicroseconds)
  {
   if (TimeInMicroseconds < 1000L)
      sprintf (Str,"%ld &micro;s",TimeInMicroseconds);
   else if (TimeInMicroseconds < 1000000L)
      sprintf (Str,"%ld ms",TimeInMicroseconds / 1000);
   else if (TimeInMicroseconds < (60*1000000L))
      sprintf (Str,"%.1f s",(float) TimeInMicroseconds / 1E6);
   else
      sprintf (Str,"%ld min, %ld s",TimeInMicroseconds / (60*1000000L),(TimeInMicroseconds/1000000L) % 60);
  }
