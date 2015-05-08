/*******************************************************************************#
#           guvcview              http://guvcview.sourceforge.net               #
#                                                                               #
#           Paulo Assis <pj.assis@gmail.com>                                    #
#                                                                               #
# This program is free software; you can redistribute it and/or modify          #
# it under the terms of the GNU General Public License as published by          #
# the Free Software Foundation; either version 2 of the License, or             #
# (at your option) any later version.                                           #
#                                                                               #
# This program is distributed in the hope that it will be useful,               #
# but WITHOUT ANY WARRANTY; without even the implied warranty of                #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 #
# GNU General Public License for more details.                                  #
#                                                                               #
# You should have received a copy of the GNU General Public License             #
# along with this program; if not, write to the Free Software                   #
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA     #
#                                                                               #
********************************************************************************/

#include <glib/gprintf.h>
/* support for internationalization - i18n */
#include <glib/gi18n.h>
#include "globals.h"
#include "avilib.h"
#include "sound.h"
#include "v4l2uvc.h"
#include "string_utils.h"
#include "vcodecs.h"
#include "acodecs.h"

#define __AMUTEX &pdata->mutex
#define __VMUTEX &videoIn->mutex
#define __GMUTEX &global->mutex
#define __FMUTEX &global->file_mutex
#define __GCOND  &global->IO_cond

int initGlobals (struct GLOBAL *global)
{
	__INIT_MUTEX( __GMUTEX );
	__INIT_MUTEX( __FMUTEX );
	__INIT_COND( __GCOND );   /* Initialized video buffer semaphore */

	global->debug = DEBUG;

	const gchar *home = g_get_home_dir();

	global->videodevice = g_strdup("/dev/video0");

	global->confPath = g_strjoin("/", home, ".config", "guvcview", NULL);
	int ret = g_mkdir_with_parents(global->confPath, 0777);
	if(ret)
		fprintf(stderr, "Couldn't create configuration dir: %s \n", global->confPath);

	g_free(global->confPath);
	global->confPath = g_strjoin("/", home, ".config", "guvcview", "video0", NULL);

	global->vidFPath = g_new(pchar, 2);

	global->imgFPath = g_new(pchar, 2);

	global->profile_FPath = g_new(pchar, 2);

	global->vidFPath[1] = g_strdup(home);

	global->imgFPath[1] = g_strdup(home);

	global->profile_FPath[1] = g_strdup(home);

	global->vidFPath[0] = g_strdup("guvcview_video.mkv");
	global->VidFormat = check_video_type(global->vidFPath[0]);

	global->imgFPath[0] = g_strdup("guvcview_image.jpg");
	global->imgFormat = check_image_type(global->imgFPath[0]);

	global->profile_FPath[0] = g_strdup("default.gpfl");

	global->WVcaption = g_new(char, 32);

	g_snprintf(global->WVcaption,10,"GUVCVIdeo");

	global->videoBuff = NULL;
	global->video_buff_size = VIDBUFF_SIZE;

	global->image_inc = 1; //increment filename by default
	global->vid_inc = 1;   //increment filename by default

	global->vid_sleep=0;
	global->vidfile=NULL; /*vid filename passed through argument options with -n */
	global->Capture_time=0; /*vid capture time passed through argument options with -t */
	global->lprofile=0; /* flag for -l command line option*/

	/** try to set video codec default to mpeg4*/
	setVcodecVal ();
	int vcodec = get_list_vcodec_index(AV_CODEC_ID_MPEG4);
	if(vcodec < 0)
		vcodec = 0;
	global->VidCodec= vcodec; /*0-"MJPG"  1-"YUY2" 2-"DIB "(rgb32) 3-...*/
	global->VidCodec_ID = get_vcodec_id(global->VidCodec); // AV_CODEC_ID_MPEG4 or MJPG
	/** try to set audio codec default to mp2*/
	setAcodecVal();
	int acodec = get_list_acodec_index(AV_CODEC_ID_MP2);
	if(acodec < 0)
		acodec = 0;
	global->AudCodec = acodec; /*0-"PCM"  1-"MPG2" 2-...*/

	global->av_drift=0;
	global->Vidstarttime=0;
	global->Vidstoptime=0;
	global->framecount=0;
	global->w_ind=0;
	global->r_ind=0;

	global->Sound_enable=TRUE; /*Enable Sound by Default*/
	global->Sound_IndexDev=NULL;
	global->Sound_SampRate=SAMPLE_RATE;
	global->Sound_SampRateInd=0;
	global->Sound_numInputDev=0;
	global->Sound_Format=WAVE_FORMAT_MPEG12; /*set MP2 by default*/
	global->Sound_DefDev=0;
	global->Sound_UseDev=0;
	global->Sound_NumChan=NUM_CHANNELS;
	global->Sound_NumChanInd=0;
	global->Sound_delay=0;     /*sound delay in nanosec*/
	global->FpsCount=0;

	global->disk_timer_id=0;
	global->timer_id=0;
	global->image_timer_id=0;
	global->image_timer=0;
	global->image_npics=9999;/*default max number of captures*/
	global->image_picn =0;
	global->frmCount=0;
	global->PanStep=2;/*2 degree step for Pan*/
	global->TiltStep=2;/*2 degree step for Tilt*/
	global->DispFps=0;
	global->fps = DEFAULT_FPS;
	global->fps_num = DEFAULT_FPS_NUM;
	global->bpp = 0; //current bytes per pixel
	global->hwaccel = 1; //use hardware acceleration
	global->desktop_w = 0;
	global->desktop_h = 0;
	global->cap_meth = IO_MMAP;//default mmap(1) or read(0)
	global->flg_cap_meth = FALSE;
	global->width = DEFAULT_WIDTH;
	global->height = DEFAULT_HEIGHT;
	global->winwidth=WINSIZEX;
	global->winheight=WINSIZEY;

	global->default_action=0;

	global->mode = g_new(char, 6);
	g_snprintf(global->mode, 5, "mjpg");

	global->format = V4L2_PIX_FMT_MJPEG;

	global->Frame_Flags = YUV_NOFILT;
	global->osdFlags = 0;

    global->no_display = FALSE;
	global->exit_on_close = FALSE;
	global->skip_n=0;
	global->jpeg=NULL;
	global->uvc_h264_unit = 0; //not supported by default

	/* reset with videoIn parameters */
	global->autofocus = FALSE;
	global->AFcontrol = FALSE;
	global->VidButtPress = FALSE;
	global->change_res = FALSE;
	global->add_ctrls = FALSE;
	global->lctl_method = LIST_CTL_METHOD_NEXT_FLAG; //next_ctrl flag method
	return (0);
}

int closeGlobals(struct GLOBAL *global)
{
	if(!global)
		return(-1);

	g_free(global->videodevice);
	g_free(global->confPath);
	g_free(global->vidFPath[1]);
	g_free(global->imgFPath[1]);
	g_free(global->imgFPath[0]);
	g_free(global->vidFPath[0]);
	g_free(global->profile_FPath[1]);
	g_free(global->profile_FPath[0]);
	g_free(global->vidFPath);
	g_free(global->imgFPath);
	g_free(global->profile_FPath);
	g_free (global->WVcaption);
	g_free(global->vidfile);
	g_free(global->mode);
	g_free(global->Sound_IndexDev);
	__CLOSE_MUTEX( __GMUTEX );
	__CLOSE_MUTEX( __FMUTEX );
	__CLOSE_COND( __GCOND );

	global->videodevice=NULL;
	global->confPath=NULL;
	global->vidfile=NULL;
	global->mode=NULL;
	if(global->jpeg) g_free(global->jpeg);
	global->jpeg=NULL;
	g_free(global);
	return (0);
}
