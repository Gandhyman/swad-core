// swad_help.c: contextual help to enhance usability

/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2016 Antonio Ca�as Vargas

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

#include <stdlib.h>		// For exit, system, malloc, free, rand, etc.

#include "swad_action.h"
#include "swad_config.h"
#include "swad_global.h"
#include "swad_help.h"
#include "swad_user.h"

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private constants *****************************/
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
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void Hlp_ShowRowHelpWhatWouldYouLikeToDo (const char *Description,
                                                 Act_Action_t Action,
                                                 const char *ClassButton,
                                                 const char *TxtButton);

/*****************************************************************************/
/*************************** Show contextual help ****************************/
/*****************************************************************************/

void Hlp_ShowHelpWhatWouldYouLikeToDo (void)
  {
   extern const char *Txt_You_are_not_enrolled_in_any_course[Usr_NUM_SEXS];
   extern const char *Txt_You_can_search_for_courses_select_them_and_request_your_enrollment_in_them;
   extern const char *Txt_If_you_can_not_find_your_institution_your_centre_your_degree_or_your_courses_you_can_create_them;
   extern const char *Txt_What_would_you_like_to_do;
   extern const char *Txt_Register_students_in_the_course_X;
   extern const char *Txt_Register_students;
   extern const char *Txt_Go_to_one_of_my_courses;
   extern const char *Txt_My_courses;
   extern const char *Txt_Sign_up;
   extern const char *Txt_Remove_me_from_the_course_X;
   extern const char *Txt_Remove_me;
   extern const char *Txt_Register_me_in_X;
   extern const char *Txt_Select_or_create_another_course_in_X;
   extern const char *Txt_Select_or_create_one_course_in_X;
   extern const char *Txt_Courses;
   extern const char *Txt_Select_or_create_another_degree_in_X;
   extern const char *Txt_Select_or_create_one_degree_in_X;
   extern const char *Txt_Degrees;
   extern const char *Txt_Select_or_create_another_centre_in_X;
   extern const char *Txt_Select_or_create_one_centre_in_X;
   extern const char *Txt_Centres;
   extern const char *Txt_Select_or_create_another_institution_in_X;
   extern const char *Txt_Select_or_create_one_institution_in_X;
   extern const char *Txt_Institutions;
   extern const char *Txt_Select_another_country;
   extern const char *Txt_Select_one_country;
   extern const char *Txt_Countries;
   extern const char *Txt_Upload_my_picture;
   extern const char *Txt_Upload_photo;
   extern const char *Txt_Log_in;
   extern const char *Txt_New_on_PLATFORM_Sign_up;
   extern const char *Txt_Create_account;

   /***** Alert message *****/
   if (Gbl.Usrs.Me.Logged &&
       !Gbl.Usrs.Me.MyCrss.Num)
     {
      sprintf (Gbl.Message,"%s<br />%s<br />%s",
	       Txt_You_are_not_enrolled_in_any_course[Gbl.Usrs.Me.UsrDat.Sex],
	       Txt_You_can_search_for_courses_select_them_and_request_your_enrollment_in_them,
	       Txt_If_you_can_not_find_your_institution_your_centre_your_degree_or_your_courses_you_can_create_them);
      Lay_ShowAlert (Lay_INFO,Gbl.Message);
     }

   /***** Contextual buttons *****/
   /* Start table */
   Lay_StartRoundFrameTable (NULL,2,Txt_What_would_you_like_to_do);

   if (Gbl.Usrs.Me.Logged)		// I am logged
     {
      if (Gbl.CurrentCrs.Crs.CrsCod > 0)	// Course selected
	{
	 if (Gbl.Usrs.Me.IBelongToCurrentCrs)	// I belong to this course
	   {
	    if (Gbl.Action.Act != ActAutUsrInt &&
		Gbl.Action.Act != ActAutUsrNew &&
                Gbl.Action.Act != ActAutUsrChgLan)	// I am not just logged
	      {
	       /* Request my removing from this course */
	       sprintf (Gbl.Title,Txt_Remove_me_from_the_course_X,
			Gbl.CurrentCrs.Crs.ShrtName);
	       Hlp_ShowRowHelpWhatWouldYouLikeToDo (Gbl.Title,
						    ActReqMdfOneStd,
						    "BT_REMOVE",Txt_Remove_me);
	      }
	   }
	 else					// I do not belong to this course
	   {
	    /* Request my registration in this course */
	    sprintf (Gbl.Title,Txt_Register_me_in_X,
		     Gbl.CurrentCrs.Crs.ShrtName);
	    Hlp_ShowRowHelpWhatWouldYouLikeToDo (Gbl.Title,
						 ActReqSignUp,
						 "BT_CREATE",Txt_Sign_up);
	   }
	}

      if (Gbl.Usrs.Me.MyCrss.Num)	// I am enrolled in some courses
	{
	 if (Gbl.CurrentCrs.Crs.CrsCod > 0 &&				// Course selected
	     !Gbl.CurrentCrs.Crs.NumStds &&				// Current course has no students
	     Gbl.Usrs.Me.UsrDat.RoleInCurrentCrsDB == Rol_TEACHER)	// I am a teacher in current course
	   {
	    /* Request students enrollment */
	    sprintf (Gbl.Title,Txt_Register_students_in_the_course_X,
		     Gbl.CurrentCrs.Crs.ShrtName);
	    Hlp_ShowRowHelpWhatWouldYouLikeToDo (Gbl.Title,
						 ActReqEnrSevStd,
						 "BT_CONFIRM",Txt_Register_students);
	   }

	 if (Gbl.Action.Act != ActMyCrs)	// I am not seeing the action to list my courses
	    /* Request list my courses */
	    Hlp_ShowRowHelpWhatWouldYouLikeToDo (Txt_Go_to_one_of_my_courses,
						 ActMyCrs,
						 "BT_CONFIRM",Txt_My_courses);
	}

      if (Gbl.CurrentDeg.Deg.DegCod > 0)	// Degree selected
	{
	 /* Select a course */
	 sprintf (Gbl.Title,Gbl.CurrentCrs.Crs.CrsCod > 0 ? Txt_Select_or_create_another_course_in_X :
							    Txt_Select_or_create_one_course_in_X,
		  Gbl.CurrentDeg.Deg.ShrtName);
	 Hlp_ShowRowHelpWhatWouldYouLikeToDo (Gbl.Title,
				              ActSeeCrs,
				              "BT_CONFIRM",Txt_Courses);
	}
      else if (Gbl.CurrentCtr.Ctr.CtrCod > 0)	// Centre selected
	{
	 /* Select a degree */
	 sprintf (Gbl.Title,Gbl.CurrentDeg.Deg.DegCod > 0 ? Txt_Select_or_create_another_degree_in_X :
							    Txt_Select_or_create_one_degree_in_X,
		  Gbl.CurrentCtr.Ctr.ShrtName);
	 Hlp_ShowRowHelpWhatWouldYouLikeToDo (Gbl.Title,
				              ActSeeDeg,
				              "BT_CONFIRM",Txt_Degrees);
	}
      else if (Gbl.CurrentIns.Ins.InsCod > 0)	// Institution selected
	{
	 /* Select a centre */
	 sprintf (Gbl.Title,Gbl.CurrentCtr.Ctr.CtrCod > 0 ? Txt_Select_or_create_another_centre_in_X :
							    Txt_Select_or_create_one_centre_in_X,
		  Gbl.CurrentIns.Ins.ShrtName);
	 Hlp_ShowRowHelpWhatWouldYouLikeToDo (Gbl.Title,
				              ActSeeCtr,
				              "BT_CONFIRM",Txt_Centres);
	}
      else if (Gbl.CurrentCty.Cty.CtyCod > 0)	// Country selected
	{
	 /* Select an institution */
	 sprintf (Gbl.Title,Gbl.CurrentIns.Ins.InsCod > 0 ? Txt_Select_or_create_another_institution_in_X :
							    Txt_Select_or_create_one_institution_in_X,
		  Gbl.CurrentCty.Cty.Name[Gbl.Prefs.Language]);
	 Hlp_ShowRowHelpWhatWouldYouLikeToDo (Gbl.Title,
				              ActSeeIns,
				              "BT_CONFIRM",Txt_Institutions);
	}
      else
	 /* Select a country */
	 Hlp_ShowRowHelpWhatWouldYouLikeToDo (Gbl.CurrentCty.Cty.CtyCod > 0 ? Txt_Select_another_country :
						                              Txt_Select_one_country,
				              ActSeeCty,
				              "BT_CONFIRM",Txt_Countries);

      if (!Gbl.Usrs.Me.MyPhotoExists)		// I have no photo
	 Hlp_ShowRowHelpWhatWouldYouLikeToDo (Txt_Upload_my_picture,
				              ActReqMyPho,
				              "BT_CREATE",Txt_Upload_photo);
     }
   else					// I am not logged
     {
      /* Log in */
      Hlp_ShowRowHelpWhatWouldYouLikeToDo (Txt_Log_in,
					   ActFrmLogIn,
					   "BT_CONFIRM",Txt_Log_in);

      /* Sign up */
      sprintf (Gbl.Title,Txt_New_on_PLATFORM_Sign_up,
               Cfg_PLATFORM_SHORT_NAME);
      Hlp_ShowRowHelpWhatWouldYouLikeToDo (Gbl.Title,
					   ActFrmMyAcc,
					   "BT_CREATE",Txt_Create_account);
     }

   /* End table */
   Lay_EndRoundFrameTable ();
  }

/*****************************************************************************/
/*************************** Show contextual help ****************************/
/*****************************************************************************/

static void Hlp_ShowRowHelpWhatWouldYouLikeToDo (const char *Description,
                                                 Act_Action_t Action,
                                                 const char *ClassButton,
                                                 const char *TxtButton)
  {
   /***** Description *****/
   fprintf (Gbl.F.Out,"<tr>"
		      "<td class=\"DAT RIGHT_MIDDLE\">"
		      "%s:"
		      "</td>",
            Description);

   /***** Button *****/
   fprintf (Gbl.F.Out,"<td class=\"LEFT_MIDDLE\">");
   Act_FormStart (Action);
   fprintf (Gbl.F.Out,"<button type=\"submit\" class=\"BT_SUBMIT_INLINE %s\">"
		      "%s"
		      "</button>",
	    ClassButton,TxtButton);
   Act_FormEnd ();
   fprintf (Gbl.F.Out,"</td>"
		      "</tr>");
  }
