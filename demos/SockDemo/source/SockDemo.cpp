// This file, and all other source files that make up
// SockDemo, are placed into the public domain.  - Jeremy Friesner
 
#include "SockHopDemoWindow.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <app/Application.h>

// Borrowed from Jon Watte's code posting on bedevtalk..
void chdirToAppPath();
void chdirToAppPath() 
{ 
    image_info iinfo; 
    thread_info tinfo; 
    int32 cookie = 0; 

    /*      figure out our team     */ 
    if (get_thread_info(find_thread(NULL), &tinfo)) 
    {
       printf("chDirToAppPath:  Error in get_thread_info!\n");
       return;
    }

    /*      figure out where the app binary lives   */ 
    while (!get_next_image_info(tinfo.team, &cookie, &iinfo)) 
    { 
       char * ptr = NULL; 
       if (iinfo.type != B_APP_IMAGE) continue; 
       ptr = strrchr(iinfo.name, '/'); 
       if (!ptr) continue; /* should not happen */ 
       *ptr = 0; 
       chdir(iinfo.name);
       return; 
    } 
    printf("chDirToappPath:  Couldn't find current directory?\n");
} 

int main(void)
{
	BApplication		app("application/x-vnd.Sugoi-SockDemo");
	
    chdirToAppPath();	
	SockHopDemoWindow * sdw = new SockHopDemoWindow();
	sdw->Show();
	
	app.Run();
	
	return 0;
}

