/*
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 
 * USA
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifndef HAVE_STDINT_H
#define HAVE_STDINT_H
#endif /* HAVE_STDINT_H */

#include <mjpegtools/yuv4mpeg.h>
#include "interlacemodes.h"

// AUTO FIX METHOD ====================

void ilaceautofixoption_to_text(char *string, int autofixoption)
{
	switch(autofixoption) {
	case BC_ILACE_AUTOFIXOPTION_AUTO:	strcpy(string, BC_ILACE_AUTOFIXOPTION_AUTO_T);		return;
	case BC_ILACE_AUTOFIXOPTION_MANUAL:	strcpy(string, BC_ILACE_AUTOFIXOPTION_MANUAL_T);	return;
	}
	strcpy(string, BC_ILACE_UNKNOWN_T);
}

int ilaceautofixoption_from_text(const char *text, int thedefault)
{
	if(!strcasecmp(text, BC_ILACE_AUTOFIXOPTION_AUTO_T))  	return BC_ILACE_AUTOFIXOPTION_AUTO;
	if(!strcasecmp(text, BC_ILACE_AUTOFIXOPTION_MANUAL_T))	return BC_ILACE_AUTOFIXOPTION_MANUAL;
	return thedefault;
}

// INTERLACE MODE ====================

void ilacemode_to_text(char *string, int ilacemode)
{
	switch(ilacemode) {
	case BC_ILACE_MODE_UNDETECTED:     strcpy(string, BC_ILACE_MODE_UNDETECTED_T);      return;
	case BC_ILACE_MODE_TOP_FIRST:      strcpy(string, BC_ILACE_MODE_TOP_FIRST_T);       return;
	case BC_ILACE_MODE_BOTTOM_FIRST:   strcpy(string, BC_ILACE_MODE_BOTTOM_FIRST_T);    return;
	case BC_ILACE_MODE_NOTINTERLACED:  strcpy(string, BC_ILACE_MODE_NOTINTERLACED_T);   return;
	}
	strcpy(string, BC_ILACE_UNKNOWN_T);
}

int ilacemode_from_text(const char *text, int thedefault)
{
	if(!strcasecmp(text, BC_ILACE_MODE_UNDETECTED_T))     return BC_ILACE_MODE_UNDETECTED;
	if(!strcasecmp(text, BC_ILACE_MODE_TOP_FIRST_T))      return BC_ILACE_MODE_TOP_FIRST;
	if(!strcasecmp(text, BC_ILACE_MODE_BOTTOM_FIRST_T))   return BC_ILACE_MODE_BOTTOM_FIRST;
	if(!strcasecmp(text, BC_ILACE_MODE_NOTINTERLACED_T))  return BC_ILACE_MODE_NOTINTERLACED;
	return thedefault;
}

void ilacemode_to_xmltext(char *string, int ilacemode)
{
	switch(ilacemode) {
	case BC_ILACE_MODE_UNDETECTED:     strcpy(string, BC_ILACE_MODE_UNDETECTED_XMLT);      return;
	case BC_ILACE_MODE_TOP_FIRST:      strcpy(string, BC_ILACE_MODE_TOP_FIRST_XMLT);       return;
	case BC_ILACE_MODE_BOTTOM_FIRST:   strcpy(string, BC_ILACE_MODE_BOTTOM_FIRST_XMLT);    return;
	case BC_ILACE_MODE_NOTINTERLACED:  strcpy(string, BC_ILACE_MODE_NOTINTERLACED_XMLT);   return;
	}
	strcpy(string, BC_ILACE_UNKNOWN_T);
}

int ilacemode_from_xmltext(const char *text, int thedefault)
{
	if( text ) {
		if(!strcasecmp(text, BC_ILACE_MODE_UNDETECTED_XMLT))     return BC_ILACE_MODE_UNDETECTED;
		if(!strcasecmp(text, BC_ILACE_MODE_TOP_FIRST_XMLT))      return BC_ILACE_MODE_TOP_FIRST;
		if(!strcasecmp(text, BC_ILACE_MODE_BOTTOM_FIRST_XMLT))   return BC_ILACE_MODE_BOTTOM_FIRST;
		if(!strcasecmp(text, BC_ILACE_MODE_NOTINTERLACED_XMLT))  return BC_ILACE_MODE_NOTINTERLACED;
	}
	return thedefault;
}

// INTERLACE FIX METHOD ====================

void ilacefixmethod_to_text(char *string, int fixmethod)
{
	switch(fixmethod) {
	case BC_ILACE_FIXMETHOD_NONE:   	strcpy(string, BC_ILACE_FIXMETHOD_NONE_T);   	return;
	case BC_ILACE_FIXMETHOD_UPONE:  	strcpy(string, BC_ILACE_FIXMETHOD_UPONE_T);  	return;
	case BC_ILACE_FIXMETHOD_DOWNONE:	strcpy(string, BC_ILACE_FIXMETHOD_DOWNONE_T);	return;
	}
	strcpy(string, BC_ILACE_UNKNOWN_T);
}

int ilacefixmethod_from_text(const char *text, int thedefault)
{
	if(!strcasecmp(text, BC_ILACE_FIXMETHOD_NONE_T))   	return BC_ILACE_FIXMETHOD_NONE;
	if(!strcasecmp(text, BC_ILACE_FIXMETHOD_UPONE_T))  	return BC_ILACE_FIXMETHOD_UPONE;
	if(!strcasecmp(text, BC_ILACE_FIXMETHOD_DOWNONE_T))	return BC_ILACE_FIXMETHOD_DOWNONE;
	return thedefault; 
}

void ilacefixmethod_to_xmltext(char *string, int fixmethod)
{
	switch(fixmethod) {
	case BC_ILACE_FIXMETHOD_NONE:   	strcpy(string, BC_ILACE_FIXMETHOD_NONE_XMLT);   	return;
	case BC_ILACE_FIXMETHOD_UPONE:  	strcpy(string, BC_ILACE_FIXMETHOD_UPONE_XMLT);  	return;
	case BC_ILACE_FIXMETHOD_DOWNONE:	strcpy(string, BC_ILACE_FIXMETHOD_DOWNONE_XMLT);	return;
	}
	strcpy(string, BC_ILACE_UNKNOWN_T);
}

int ilacefixmethod_from_xmltext(const char *text, int thedefault)
{
	if(!strcasecmp(text, BC_ILACE_FIXMETHOD_NONE_XMLT))   	return BC_ILACE_FIXMETHOD_NONE;
	if(!strcasecmp(text, BC_ILACE_FIXMETHOD_UPONE_XMLT))  	return BC_ILACE_FIXMETHOD_UPONE;
	if(!strcasecmp(text, BC_ILACE_FIXMETHOD_DOWNONE_XMLT))	return BC_ILACE_FIXMETHOD_DOWNONE;
	return thedefault; 
}

int  ilaceautofixmethod(int projectmode, int assetmode) 
{
	if (projectmode == assetmode)
		return BC_ILACE_FIXMETHOD_NONE;
	if( (projectmode == BC_ILACE_MODE_BOTTOM_FIRST && assetmode == BC_ILACE_MODE_TOP_FIRST ) ||
	    (projectmode == BC_ILACE_MODE_TOP_FIRST  && assetmode == BC_ILACE_MODE_BOTTOM_FIRST) )
		return BC_ILACE_FIXDEFAULT;
	// still to implement anything else...
	return BC_ILACE_FIXMETHOD_NONE;
}

int  ilaceautofixmethod2(int projectilacemode, int assetautofixoption, int assetilacemode, int assetfixmethod)
{
	if (assetautofixoption == BC_ILACE_AUTOFIXOPTION_AUTO)
		return (ilaceautofixmethod(projectilacemode, assetilacemode));
	return (assetfixmethod);
}

int ilace_bc_to_yuv4mpeg(int ilacemode)
{
	switch (ilacemode) {
	case BC_ILACE_MODE_UNDETECTED:	return(Y4M_UNKNOWN);
	case BC_ILACE_MODE_TOP_FIRST:	return(Y4M_ILACE_TOP_FIRST);
	case BC_ILACE_MODE_BOTTOM_FIRST: return(Y4M_ILACE_BOTTOM_FIRST);
	case BC_ILACE_MODE_NOTINTERLACED: return(Y4M_ILACE_NONE);
	}
	return(Y4M_UNKNOWN);
}

int ilace_yuv4mpeg_to_bc(int ilacemode)
{
	switch (ilacemode) {
	case Y4M_UNKNOWN:		return (BC_ILACE_MODE_UNDETECTED);
	case Y4M_ILACE_NONE:		return (BC_ILACE_MODE_NOTINTERLACED);
	case Y4M_ILACE_TOP_FIRST:	return (BC_ILACE_MODE_TOP_FIRST);
	case Y4M_ILACE_BOTTOM_FIRST:	return (BC_ILACE_MODE_BOTTOM_FIRST);
//	case Y4M_ILACE_MIXED:		return (BC_ILACE_MODE_UNDETECTED);  // fixme!!
	}
	return (BC_ILACE_MODE_UNDETECTED);
}


void ilace_yuv4mpeg_mode_to_text(char *string, int ilacemode)
{
	switch(ilacemode) {
	case Y4M_UNKNOWN:             strcpy(string, BC_ILACE_Y4M_UKNOWN_T);       return;
	case Y4M_ILACE_NONE:          strcpy(string, BC_ILACE_Y4M_NONE_T);         return;
	case Y4M_ILACE_TOP_FIRST:     strcpy(string, BC_ILACE_Y4M_TOP_FIRST_T);    return;
	case Y4M_ILACE_BOTTOM_FIRST:  strcpy(string, BC_ILACE_Y4M_BOTTOM_FIRST_T); return;
//	case Y4M_ILACE_MIXED:         strcpy(string, BC_ILACE_Y4M_MIXED_T);        return;
	}
	strcpy(string, BC_ILACE_UNKNOWN_T);
}

