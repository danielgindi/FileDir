//
//  FileDirController.m
//  FileDir
//
//  Created by Daniel Cohen Gindi on 6/24/14.
//  Copyright (c) 2013 Daniel Cohen Gindi. All rights reserved.
//
//  https://github.com/danielgindi/FileDir
//
//  The MIT License (MIT)
//
//  Copyright (c) 2014 Daniel Cohen Gindi (danielgindi@gmail.com)
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.
//

#include "FileDirController.h"
#include "FileDir.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma warning (disable : 4996)
#else
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#endif

#ifdef WIN32

#ifdef FILE_ATTRIBUTE_INTEGRITY_STREAM
#define IS_REGULAR_FILE_HAS_ATTRIBUTE_INTEGRITY_STREAM(dwFileAttributes) (!!(dwFileAttributes & FILE_ATTRIBUTE_INTEGRITY_STREAM))
#else
#define IS_REGULAR_FILE_HAS_ATTRIBUTE_INTEGRITY_STREAM(dwFileAttributes) false
#endif

#ifdef FILE_ATTRIBUTE_NO_SCRUB_DATA
#define IS_REGULAR_FILE_HAS_ATTRIBUTE_NO_SCRUB_DATA(dwFileAttributes) (!!(dwFileAttributes & FILE_ATTRIBUTE_NO_SCRUB_DATA))
#else
#define IS_REGULAR_FILE_HAS_ATTRIBUTE_NO_SCRUB_DATA(dwFileAttributes) false
#endif

#define IS_REGULAR_FILE(dwFileAttributes) \
	( \
	!!(dwFileAttributes & FILE_ATTRIBUTE_NORMAL) || \
	( \
	!(dwFileAttributes & FILE_ATTRIBUTE_DEVICE) && \
	!(dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && \
	!(dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) && \
	!IS_REGULAR_FILE_HAS_ATTRIBUTE_INTEGRITY_STREAM(dwFileAttributes) && \
	!IS_REGULAR_FILE_HAS_ATTRIBUTE_NO_SCRUB_DATA(dwFileAttributes) && \
	!(dwFileAttributes & FILE_ATTRIBUTE_OFFLINE) && \
	!(dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY) \
	) \
	)
#else
#define IS_REGULAR_FILE(statMode) S_ISREG(statMode)	
#endif

#ifdef WIN32
#define IS_FOLDER(dwFileAttributes) (!!(dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
#else
#define IS_FOLDER(statMode) S_ISDIR(statMode)	
#endif


#ifndef FILEDIR_CHAR

#ifdef WIN32
#define FILEDIR_CHAR wchar_t
#define ustrrchr wcsrchr
#define ustrlen (int)wcslen
#define ustrdup _wcsdup
#else
#define FILEDIR_CHAR char
#define ustrrchr strrchr
#define ustrlen (int)strlen
#define ustrdup strdup
#endif

#endif

#ifdef WIN32
typedef struct _find_data_t {
	_find_data_t()
	{
		handle = INVALID_HANDLE_VALUE;
		memset(&data, 0, sizeof(data));
		hasNext = false;
		basePath = NULL;
		basePathLength = 0;
	}
	void release()
	{
		if (handle != INVALID_HANDLE_VALUE)
		{
			FindClose(handle);
		}
		if (basePath)
		{
			delete [] basePath;
		}
	}

	HANDLE handle;
	WIN32_FIND_DATAW data;
	bool hasNext;
	wchar_t *basePath;
	int basePathLength;
} find_data_t;
#else
typedef struct _find_data_t {
	_find_data_t()
	{
		dir = NULL;
		entry = NULL;
		hasNext = false;
		basePath = NULL;
		basePathLength = 0;
	}
	void release()
	{
		if (dir)
		{
			closedir(dir);
		}
		if (basePath)
		{
			delete [] basePath;
		}
	}

	DIR *dir;
	dirent *entry;
	bool hasNext;
	char *basePath;
	int basePathLength;
} find_data_t;
#endif

static find_data_t *openFolderForSearch(const FILEDIR_CHAR *path)
{
	find_data_t *data = new find_data_t();
	
	int pathLen = ustrlen(path);

#ifdef WIN32
	data->basePath = new FILEDIR_CHAR[pathLen + 3];
	memcpy((void *)data->basePath, path, sizeof(FILEDIR_CHAR) * pathLen);
	data->basePathLength = pathLen;
	data->basePath[data->basePathLength] = '\\';
	data->basePath[data->basePathLength + 1] = '*';
	data->basePath[data->basePathLength + 2] = '\0';
#else
	data->basePath = new FILEDIR_CHAR[pathLen + 1];
	memcpy(data->basePath, path, sizeof(FILEDIR_CHAR) * (pathLen + 1));
	data->basePathLength = pathLen;
#endif

#ifdef WIN32

	data->handle = FindFirstFileW(data->basePath, &data->data);

	data->basePath[data->basePathLength] = '\0';
	data->hasNext = data->handle != INVALID_HANDLE_VALUE;

	while (data->handle != INVALID_HANDLE_VALUE && data->data.cFileName[0] == '.' && 
		(data->data.cFileName[1] == '\0' || 
		(data->data.cFileName[1] == '.' && data->data.cFileName[2] == '\0')))
	{
		if (FindNextFileW(data->handle, &data->data) == 0)
		{
			data->hasNext = false;
			break;
		}
	}

	if (data->handle == INVALID_HANDLE_VALUE)
	{
		data->release();
		delete data;
		data = NULL;
	}

#else

	data->dir = opendir(path);
	if (data->dir != NULL && (data->entry = readdir(data->dir)))
	{
		while (data->entry && data->entry->d_name[0] == '.' && 
			(data->entry->d_name[1] == '\0' || 
			(data->entry->d_name[1] == '.' && data->entry->d_name[2] == '\0')))
		{
			data->entry = readdir(data->dir);
		}

		data->hasNext = data->entry != NULL;
	}
	else
	{
		data->release();
		delete data;
		data = NULL;
	}

#endif

	return data;
}

FileDirController::FileDirController(void)
{
	_isRecursive = false;
	Close();
}

FileDirController::~FileDirController(void)
{
	Close();
}

bool FileDirController::EnumerateFilesAtPath(const FILEDIR_CHAR *path, bool recursive/* = false*/)
{
	Close();

	_isRecursive = recursive;

	if (!path) return false;

	find_data_t *find = openFolderForSearch(path);
	if (find)
	{
		if (find->hasNext)
		{
			_searchTree.push_back((void *)find);
		}
		else
		{
			find->release();
			delete find;
		}

		return true;
	}

	return false;
}

FileDir * FileDirController::GetFileInfo(const FILEDIR_CHAR *path)
{
	if (!path || path[0] == '\0') return NULL;

#ifdef WIN32
	DWORD dwFileAttributes = GetFileAttributes(path);
	if (dwFileAttributes == INVALID_FILE_ATTRIBUTES)
	{
		return NULL;
	}
#else
	struct stat fileStat;
	if (stat(path, &fileStat) == -1)
	{
		return NULL;
	}
#endif
	
	FileDir *fileDir = new FileDir();
	fileDir->_fullPath = ustrdup(path);

	const FILEDIR_CHAR *separator = ustrrchr(path, '/');
	if (!separator) separator = ustrrchr(path, '\\');
	if (separator)
	{
		fileDir->_fileName = ustrdup(separator + 1);
	}
	else
	{
		fileDir->_fileName = ustrdup(path);
	}

#ifdef WIN32
	fileDir->_isFile = IS_REGULAR_FILE(dwFileAttributes);
	fileDir->_isFolder = IS_FOLDER(dwFileAttributes);
#else
	fileDir->_isFile = IS_REGULAR_FILE(fileStat.st_mode);
	fileDir->_isFolder = IS_FOLDER(fileStat.st_mode);

	fileDir->_creationTime = -1;
	fileDir->_lastModificationTime = fileStat.st_mtime;
	fileDir->_lastAccessTime = fileStat.st_atime;
	fileDir->_lastStatusChangeTime = fileStat.st_ctime;
	fileDir->_hasTimes = true;
#endif

	return fileDir;
}

void FileDirController::Close()
{
	for (std::list<void *>::iterator it = _searchTree.begin(), itEnd = _searchTree.end(); it != itEnd; it++)
	{
		find_data_t *data = (find_data_t *)*it;
		data->release();
		delete data;
	}
	_searchTree.clear();
}

FileDir * FileDirController::NextFile()
{
	if (_searchTree.empty()) return NULL;
	
	find_data_t *find = (find_data_t *)_searchTree.back();
		
	int fileNameLength;
#ifdef WIN32
	fileNameLength = (int)wcslen(find->data.cFileName);
#else
	fileNameLength = (int)strlen(find->entry->d_name);
#endif

	bool addSlash = find->basePath[find->basePathLength - 1] != '/' && find->basePath[find->basePathLength - 1] != '\\';
	int slashLength = addSlash ? 1 : 0;

	int fullPathLength = find->basePathLength + slashLength + fileNameLength;

#ifdef WIN32
	wchar_t *filePath = new FILEDIR_CHAR[fullPathLength + 1];
	memcpy(filePath, find->basePath, sizeof(FILEDIR_CHAR) * find->basePathLength);
	if (addSlash)
	{
		filePath[find->basePathLength] = '\\';
	}
	memcpy(filePath + find->basePathLength + slashLength, find->data.cFileName, sizeof(FILEDIR_CHAR) * fileNameLength);
	filePath[fullPathLength] = '\0';
#else
	char *filePath = new FILEDIR_CHAR[fullPathLength + 1];
	memcpy(filePath, find->basePath, sizeof(FILEDIR_CHAR) * find->basePathLength);
	if (addSlash)
	{
		filePath[find->basePathLength] = '/';
	}
	memcpy(filePath + find->basePathLength + slashLength, find->entry->d_name, sizeof(FILEDIR_CHAR) * fileNameLength);
	filePath[fullPathLength] = '\0';
#endif

#ifndef WIN32
	struct stat fileStat;
	if (stat(filePath, &fileStat) == -1)
	{
		delete [] filePath;
		return NULL;
	}
#endif

	FileDir *fileDir = new FileDir();
	fileDir->_fullPath = filePath;
#ifdef WIN32
	fileDir->_fileName = wcsdup(find->data.cFileName); // Copy from the struct's memory
#else
	fileDir->_fileName = strdup(find->entry->d_name); // Copy from statically allocated memory
#endif

#ifdef WIN32
	fileDir->_isFile = IS_REGULAR_FILE(find->data.dwFileAttributes);
	fileDir->_isFolder = IS_FOLDER(find->data.dwFileAttributes);
#else
	fileDir->_isFile = IS_REGULAR_FILE(fileStat.st_mode);
	fileDir->_isFolder = IS_FOLDER(fileStat.st_mode);

	fileDir->_creationTime = -1;
	fileDir->_lastModificationTime = fileStat.st_mtime;
	fileDir->_lastAccessTime = fileStat.st_atime;
	fileDir->_lastStatusChangeTime = fileStat.st_ctime;
	fileDir->_hasTimes = true;
#endif

	// Prepare for the next file
#ifdef WIN32
	if (FindNextFileW(find->handle, &find->data) == 0)
	{
		find->release();
		delete find;
		_searchTree.pop_back();
	}
#else
	find->entry = readdir(find->dir);
	if (find->entry == NULL)
	{
		find->release();
		delete find;
		_searchTree.pop_back();
	}
#endif

	if (_isRecursive && fileDir->_isFolder)
	{
		find = openFolderForSearch(fileDir->GetFullPath());
		if (find)
		{
			if (find->hasNext)
			{
				_searchTree.push_back((void *)find);
			}
			else
			{
				delete find;
			}
		}
	}

	return fileDir;
}