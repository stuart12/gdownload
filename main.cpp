#include <iostream>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>

#include <algorithm>
#include <cctype>
#include <gphoto2/gphoto2-camera.h>
#include <stdlib.h>

#include "samples.h"
#include <gphoto2/gphoto2-camera.h>

#define check(s) (void((s) == GP_OK || failed(#s)))
#define sys(s) (void((s) != -1 || _sys(#s)))

int failed(char const * s)
{
	std::cerr << "failed " << s << std::endl;
	exit(2);
}

int _sys(char const * s)
{
	std::cerr << "failed " << s << ": " << strerror(errno) << std::endl;
	exit(2);
}

void copy(
	std::string folder,
	std::string base,
	Camera *camera,
	GPContext * context
)
{
	std::string const dir = folder.empty() ? "/" : folder + (folder.size() > 1 ? "/" : "") + base;
	CameraList * list;
	check(gp_list_new(&list));
	if (base.find("CANON") == 3 && isdigit(base[0]) && isdigit(base[1]) && isdigit(base[2]))
	{
		check(gp_camera_folder_list_files(camera, dir.c_str(), list, context));
		int const n = gp_list_count(list);
		for (int i = 0; i < n; i++)
		{
			char const * name;
			check(gp_list_get_name(list, i, &name));
			std::string lower(name + 3);
			std::transform(lower.begin(), lower.end(), lower.begin(), tolower);
			std::string src = dir + "/" + name;
			std::string dest = "/disks/shared/junk/" + base.substr(0, 3) + lower;
			std::cout << src << " -> " << dest << std::endl;

			int fd;
			sys(fd = open(dest.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666));
			CameraFile * file;
			check(gp_file_new_from_fd(&file, fd));
			check(gp_camera_file_get(camera, dir.c_str(), name, GP_FILE_TYPE_NORMAL, file, context));
			check(gp_file_free(file));
		}
	}
	else
	{
		std::cout << dir << std::endl;
		check(gp_camera_folder_list_folders(camera, dir.c_str(), list, context));
		int const n = gp_list_count(list);
		for (int i = 0; i < n; i++)
		{
			char const * name;
			check(gp_list_get_name(list, 0, &name));
			copy(dir, name, camera, context);
		}
	}
	gp_list_free (list);
}

main(int argc, char * argv[])
{
	GPContext * context = sample_create_context ();
	Camera *camera;
	gp_camera_new (&camera);
	if (gp_camera_init (camera, context) < GP_OK)
	{
		std::cerr << "No camera auto detected." << std::endl;
		gp_camera_free (camera);
		return 1;
	}

	CameraText text;
	/* Simple query the camera summary text */
	if (gp_camera_get_summary (camera, &text, context) < GP_OK)
	{
		std::cerr << "Camera failed retrieving summary." << std::endl;
		gp_camera_free (camera);
		return 1;
	}
	std::cout << "Summary: " << text.text << std::endl; 

	CameraList	*list;
	check(gp_list_new (&list));

	copy("", "", camera, context);


	gp_camera_exit (camera, context);
	gp_camera_free (camera); 
	std::cout << "done\n";
	return 0;
}
