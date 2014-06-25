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

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma warning (disable : 4996)
#else
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#endif

#ifdef _MSC_VER

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

#ifdef _MSC_VER
#define IS_FOLDER(dwFileAttributes) (!!(dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
#else
#define IS_FOLDER(statMode) S_ISDIR(statMode)	
#endif


#ifndef FILEDIR_CHAR

#ifdef _MSC_VER
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

FileDirController::FileDirController(void)
{
	_hasNext = false;
	_basePath = NULL;
	_basePathLength = 0;
#ifdef _MSC_VER
	_winFindHandle = INVALID_HANDLE_VALUE;
	memset(&_winFindData, 0, sizeof(_winFindData));
#else
	_unixFindDir = NULL;
	_unixFindDirEntry = NULL;
#endif
}

FileDirController::~FileDirController(void)
{
	Close();
}

bool FileDirController::EnumerateFilesAtPath(const FILEDIR_CHAR *path)
{
	Close();

	if (!path) return false;

	bool success = false;

	int pathLen = ustrlen(path);

#ifdef _MSC_VER
	_basePath = new FILEDIR_CHAR[pathLen + 3];
	memcpy(_basePath, path, sizeof(FILEDIR_CHAR) * pathLen);
	_basePathLength = pathLen;
	_basePath[_basePathLength] = '\\';
	_basePath[_basePathLength + 1] = '*';
	_basePath[_basePathLength + 2] = '\0';
#else
	_basePath = new FILEDIR_CHAR[pathLen + 1];
	memcpy(_basePath, path, sizeof(FILEDIR_CHAR) * (pathLen + 1));
	_basePathLength = pathLen;
#endif

#ifdef _MSC_VER

	_winFindHandle = FindFirstFileW(_basePath, &_winFindData);

	_basePath[_basePathLength] = '\0';

	while (_winFindHandle != INVALID_HANDLE_VALUE && _winFindData.cFileName[0] == '.' && 
													(_winFindData.cFileName[1] == '\0' || 
													(_winFindData.cFileName[1] == '.' && _winFindData.cFileName[2] == '\0')))
	{
		if (FindNextFileW(_winFindHandle, &_winFindData) == 0)
		{
			FindClose(_winFindHandle);
			_winFindHandle = INVALID_HANDLE_VALUE;
		}
	}

	if (_winFindHandle != INVALID_HANDLE_VALUE)
	{
		success = true;
		_hasNext = true;
	}

#else
	_unixFindDir = opendir(path);
	if (_unixFindDir != NULL)
	{
		success = true;
		_hasNext = true;

		_unixFindDirEntry = readdir(_unixFindDir);

		while (_unixFindDirEntry && _unixFindDirEntry->d_name[0] == '.' && 
			(_unixFindDirEntry->d_name[1] == '\0' || 
			(_unixFindDirEntry->d_name[1] == '.' && _unixFindDirEntry->d_name[2] == '\0')))
		{
			_unixFindDirEntry = readdir(_unixFindDir);
		}

		if (_unixFindDirEntry == NULL)
		{
			_hasNext = false;
		}
	}

#endif

	return success;
}

FileDir * FileDirController::GetFileInfo(const FILEDIR_CHAR *path)
{
	if (!path || path[0] == '\0') return NULL;

#ifdef _MSC_VER
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

#ifdef _MSC_VER
	fileDir->_isFile = IS_REGULAR_FILE(dwFileAttributes);
	fileDir->_isFolder = IS_FOLDER(dwFileAttributes);
#else
	fileDir->_isFile = IS_REGULAR_FILE(fileStat.st_mode);
	fileDir->_isFolder = IS_FOLDER(fileStat.st_mode);
#endif

	return fileDir;
}

void FileDirController::Close()
{
#ifdef _MSC_VER
	if (_winFindHandle != INVALID_HANDLE_VALUE)
	{
		FindClose(_winFindHandle);
	}
	_winFindHandle = INVALID_HANDLE_VALUE;
	memset(&_winFindData, 0, sizeof(_winFindData));
#else
	if (_unixFindDir)
	{
		closedir(_unixFindDir);
	}
	_unixFindDir = NULL;
	_unixFindDirEntry = NULL;
#endif

	if (_basePath)
	{
		delete [] _basePath;
		_basePath = NULL;
		_basePathLength = 0;
	}

	_hasNext = false;
}

FileDir * FileDirController::NextFile()
{
	if (!_hasNext) return NULL;
	
	int fileNameLength;
#ifdef _MSC_VER
	fileNameLength = (int)wcslen(_winFindData.cFileName);
#else
	fileNameLength = (int)strlen(_unixFindDirEntry->d_name);
#endif

	int fullPathLength = _basePathLength + 1 + fileNameLength;
    
    bool addSlash = _basePath[_basePathLength - 1] != '/' && _basePath[_basePathLength - 1] != '\\';
    int slashLength = addSlash ? 1 : 0;

#ifdef _MSC_VER
	wchar_t *filePath = new FILEDIR_CHAR[fullPathLength + 1];
	memcpy(filePath, _basePath, sizeof(FILEDIR_CHAR) * _basePathLength);
    if (addSlash)
    {
        filePath[_basePathLength] = '\\';
    }
	memcpy(filePath + _basePathLength + slashLength, _winFindData.cFileName, sizeof(FILEDIR_CHAR) * fileNameLength);
	filePath[_basePathLength + slashLength + fileNameLength] = '\0';
#else
	char *filePath = new FILEDIR_CHAR[fullPathLength + 1];
	memcpy(filePath, _basePath, sizeof(FILEDIR_CHAR) * _basePathLength);
    if (addSlash)
    {
        filePath[_basePathLength] = '/';
    }
	memcpy(filePath + _basePathLength + slashLength, _unixFindDirEntry->d_name, sizeof(FILEDIR_CHAR) * fileNameLength);
	filePath[_basePathLength + slashLength + fileNameLength] = '\0';
#endif

#ifndef _MSC_VER
	struct stat fileStat;
	if (stat(filePath, &fileStat) == -1)
	{
		delete [] filePath;
		return NULL;
	}
#endif

	FileDir *fileDir = new FileDir();
	fileDir->_fullPath = filePath;
#ifdef _MSC_VER
	fileDir->_fileName = wcsdup(_winFindData.cFileName); // Copy from the struct's memory
#else
	fileDir->_fileName = strdup(_unixFindDirEntry->d_name); // Copy from statically allocated memory
#endif
		
#ifdef _MSC_VER
	fileDir->_isFolder = IS_FOLDER(_winFindData.dwFileAttributes);
#else
	fileDir->_isFolder = IS_FOLDER(fileStat.st_mode);
#endif

#ifdef _MSC_VER
	fileDir->_isFile = IS_REGULAR_FILE(_winFindData.dwFileAttributes);
#else
	fileDir->_isFile = IS_REGULAR_FILE(fileStat.st_mode);
#endif

	// Prepare for the next file
#ifdef _MSC_VER
	if (FindNextFileW(_winFindHandle, &_winFindData) == 0)
	{
		_hasNext = false;
	}
#else
	_unixFindDirEntry = readdir(_unixFindDir);
	if (_unixFindDirEntry == NULL)
	{
		_hasNext = false;
	}
#endif

	return fileDir;
}