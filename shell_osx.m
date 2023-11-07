// shell_osx.m -- Mac

// CORE_XCODE __OBJC__ // terminal app version does not use Objective C, does not want this.


#import <Cocoa/Cocoa.h> // core_mac.h sort of
#import <ApplicationServices/ApplicationServices.h>


#include "darkplaces.h"

#define CSTRING(_x) [NSString stringWithUTF8String:_x]
#define TO_CSTRING(_x) [_x cStringUsingEncoding:NSASCIIStringEncoding]

///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: FILE MANAGER INTERACTION
///////////////////////////////////////////////////////////////////////////////

// _File_URL_Edit_Reduce_To_Parent_Path
char *File_URL_Edit_Reduce_To_Parent_Path (char *path_to_file);

char *File_URL_GetPathLastElement_Alloc (const char *path_to_file) // Name sucks!
{
    int n;
    int s_len = (int)strlen(path_to_file);
    int first = 0; // We want second / starting from the back

    for (n = s_len - 1; n >= 0; n --) {
        if (path_to_file[n] == '/') {
            //if (first)
                return strndup(&path_to_file[n + 1], n - first - 1); // We have the result.

            first = n;
        }
    }

//Dev_Note ("Test this part on something like mypath/bob")
    if (first)
        return strndup (path_to_file, n -1);
    return NULL; // Fail
}// Folder must exist.  It must be a folder.


int _Shell_Folder_Open_Folder_Must_Exist (const char *path_to_file)
{
#ifdef _CONSOLE
	return false;
#else    
	NSURL 		*fileURL = [NSURL fileURLWithPath:CSTRING(path_to_file) isDirectory:YES];

    [[NSWorkspace sharedWorkspace] openURL:fileURL];        

    return true;
#endif // ! _CONSOLE
}

// File must exist
int _Shell_Folder_Open_Highlight_URL_Must_Exist (const char *path_to_file)
{
#ifdef _CONSOLE
	return false;
#else

#if 1 // May 9 2021 - 7:04 PM
	AUTO_ALLOC___ char *s_path = strdup (path_to_file);
	char *s_path_trim_op = File_URL_Edit_Reduce_To_Parent_Path_Trailing_Slash (s_path);
	
	if (s_path_trim_op == NULL) {
		// There are no slashes in the path
		//DEBUG_ASSERT (s_path_trim_op == NULL /*There are no slashes in the path*/ );  //
	}
		
	AUTO_ALLOC___ char *s_file_last_element = File_URL_GetPathLastElement_Alloc (path_to_file );


#endif

#if 1
    NSURL *fileURL = [NSURL fileURLWithPath:CSTRING(path_to_file) isDirectory:NO];
    
    NSArray *fileURLs = [NSArray arrayWithObjects:fileURL, nil];
    [[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:fileURLs];       
    
    //http://stackoverflow.com/questions/7652928/launch-osx-finder-window-with-specific-files-selected
    
    // May 9 2021 - 6:57 PM supposedly:
    // https://stackoverflow.com/questions/36912226/url-containing-forward-slash-doesnt-work-with-nsworkspace
    // It may not accept slashes in the the url.
    //[[NSWorkspace sharedWorkspace] selectFile:[location stringByAppendingPathComponent:fileName] inFileViewerRootedAtPath:location];

#else // May 9 2021 - 7:04 PM
    //NSURL *fileURL = [NSURL fileURLWithPath:CSTRING(path_to_file) isDirectory:NO];
    //NSURL *fileURL = [NSURL fileURLWithPath:CSTRING(path_to_file) isDirectory:NO];
    
	NSString *location = CSTRING("/Users/toribasher/Desktop/CPrime/aacer_min/bin/armundo_dev");//   s_path   @"Users/Desktop";
	NSString *fileName = CSTRING(s_file_last_element); //s_file_last_element   @"TestFilename/myFile.dmg";
	if ([fileName rangeOfString:@"/"].location != NSNotFound)
	{
		fileName = [fileName stringByReplacingOccurrencesOfString:@"/" withString:@":"];
	}
    
    
    //NSArray *fileURLs = [NSArray arrayWithObjects:fileURL, nil];
    //[[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:fileURLs];       
    [[NSWorkspace sharedWorkspace] selectFile:[location stringByAppendingPathComponent:fileName] 
									inFileViewerRootedAtPath:location];

	AUTO_FREE___ freenull (s_file_last_element);
	AUTO_FREE___ freenull (s_path);

#endif 
    
    return true;
}

#endif // !CORE_XCODE
