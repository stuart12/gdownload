/* This file is part of gdownload. Copyright (C) 2012 Stuart Pook

gdownload is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

    gdownload is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with gdownload.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <vector>
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
	GPContext * context,
	std::vector<std::string> const & seen, std::string const & where,
	std::fstream & record
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
			std::string new_base = base.substr(0, 3) + lower;
			if (binary_search(seen.begin(), seen.end(), new_base))
			{
				//std::cout << "already seen " << new_base << std::endl;
			}
			else
			{
				std::string dest = where + "/" + new_base;
				std::string tmp = dest + ".tmp";
				std::cout << src << " -> " << dest << std::endl;

				int fd;
				sys(fd = open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666));
				CameraFile * file;
				check(gp_file_new_from_fd(&file, fd));
				check(gp_camera_file_get(camera, dir.c_str(), name, GP_FILE_TYPE_NORMAL, file, context));
				sys(fsync(fd));
				check(gp_file_free(file));
				sys(rename(tmp.c_str(), dest.c_str()));
				if (!(record << new_base << std::endl))
				{
					std::cerr << "failed to update record\n";
					exit(1);
				}
			}
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
			copy(dir, name, camera, context, seen, where, record);
		}
	}
	gp_list_free (list);
}

void
get_read(std::vector<std::string> & seen, std::fstream & read)
{
	std::string already_read("/home/stuart/var/photos/seen");
	std::string line;
	//read.open(already_read.c_str(), std::ios::in | std::ios::out);
	read.open(already_read.c_str(), std::ios::in | std::ios::out);
	if (!read)
	{
		std::cerr << "failed to open " << already_read << ": " << strerror(errno) << std::endl;
		exit(1);
	}
	while (getline(read, line))
		seen.push_back(line);
	if (read.bad())
	{
		std::cerr << "failed to read " << already_read << ": " << strerror(errno) << std::endl;
		exit(1);
	}
	sort(seen.begin(), seen.end());
	std::cout << "already read " << seen.size() << std::endl;
	read.clear();
	//copy(read, eof, std::back_inserter(seen));
	read.seekp(0, std::ios::cur);
	if (!(read << "\n"))
	{
		std::cerr << "cannot write " << already_read << ": " << strerror(errno) << std::endl;
		exit(1);
	}
}


main(int argc, char * argv[])
{
	std::string where = (argv[1] == 0) ? "." : argv[1];
	std::fstream record;
	std::vector<std::string> seen;
	get_read(seen, record);

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
	//std::cout << "Summary: " << text.text << std::endl; 

	CameraList	*list;
	check(gp_list_new (&list));

	copy("", "", camera, context, seen, where, record);


	gp_camera_exit (camera, context);
	gp_camera_free (camera); 

	if (!record)
	{
		std::cerr << "failed to update record\n";
		exit(1);
	}
	std::cout << "done\n";
	return 0;
}
