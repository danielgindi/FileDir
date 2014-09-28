//
//  FileDir.m
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

#include "FileDir.h"

#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifndef FILEDIR_CHAR

#ifdef WIN32
#define FILEDIR_CHAR wchar_t
#define ustrrchr wcsrchr
#define ustrlen wcslen
#define ustrdup _wcsdup
#else
#define FILEDIR_CHAR char
#define ustrrchr strrchr
#define ustrlen strlen
#define ustrdup strdup
#endif

#endif

FileDir::FileDir(void)
{
	_fullPath = NULL;
	_fileName = NULL;
	_cachedExtension = _cachedFileNameWithoutExtension = _cachedBasePath = NULL;
	_isFolder = _isFile = false;
	_hasTimes = false;
}

FileDir::~FileDir(void)
{
	if (_fullPath)
	{
		free(_fullPath);
		_fullPath = NULL;
	}

	if (_fileName)
	{
		free(_fileName);
		_fileName = NULL;
	}

	if (_cachedFileNameWithoutExtension)
	{
		free(_cachedFileNameWithoutExtension);
		_cachedFileNameWithoutExtension = NULL;
	}

	if (_cachedBasePath)
	{
		free(_cachedBasePath);
		_cachedBasePath = NULL;
	}

	_cachedExtension = NULL;
}

void FileDir::SetFullPath(const FILEDIR_CHAR *fullPath)
{
	if (_fullPath)
	{
		free(_fullPath);
		_fullPath = NULL;
	}

	if (_fileName)
	{
		free(_fileName);
		_fileName = NULL;
	}

	if (_cachedFileNameWithoutExtension)
	{
		free(_cachedFileNameWithoutExtension);
		_cachedFileNameWithoutExtension = NULL;
	}

	if (_cachedBasePath)
	{
		free(_cachedBasePath);
		_cachedBasePath = NULL;
	}

	_cachedExtension = NULL;

	_hasTimes = false;

	if (fullPath)
	{
		_fullPath = ustrdup(fullPath);

		const FILEDIR_CHAR *separator = ustrrchr(_fullPath, '/');
		if (!separator) separator = ustrrchr(_fullPath, '\\');
		if (separator && separator[1] == '\0')
		{
			while (separator > _fullPath && --separator && (separator[0] != '/' && separator[0] != '\\'));
		}
		if (separator && !(separator == _fullPath && separator[1] == '\0'))
		{
			_fileName = ustrdup(separator + 1);
		}
		else 
		{
			_fileName = ustrdup(_fullPath);
		}
	}
}

const FILEDIR_CHAR * FileDir::GetExtension()
{
	if (!_fullPath) return NULL;

	if (!_cachedExtension)
	{
		FILEDIR_CHAR *period = ustrrchr(_fullPath, '.');
		if (period == NULL)
		{
			_cachedExtension = &(_fullPath[ustrlen(_fullPath)]);
		}
		else
		{
			_cachedExtension = period + 1;
		}
	}

	return _cachedExtension;
}

const FILEDIR_CHAR * FileDir::GetFileNameWithoutExtension()
{
	if (!_fileName) return NULL;

	if (!_cachedFileNameWithoutExtension)
	{
		FILEDIR_CHAR *period = ustrrchr(_fileName, '.');
		if (period == NULL)
		{
			_cachedFileNameWithoutExtension = ustrdup(_fileName);
		}
		else
		{
			int periodIndex = (int)(period - _fileName);
			FILEDIR_CHAR *fileNameWithoutExtension = (FILEDIR_CHAR *)malloc(sizeof(FILEDIR_CHAR) * (periodIndex + 1));
			memcpy(fileNameWithoutExtension, _fileName, sizeof(FILEDIR_CHAR) * periodIndex);
			fileNameWithoutExtension[periodIndex] = '\0';
			_cachedFileNameWithoutExtension = fileNameWithoutExtension;
		}
	}

	return _cachedFileNameWithoutExtension;
}

const FILEDIR_CHAR * FileDir::GetBasePath()
{
	if (!_fullPath) return NULL;

	if (!_cachedBasePath)
	{
		FILEDIR_CHAR *separator1 = ustrrchr(_fullPath, '/');
		FILEDIR_CHAR *separator2 = ustrrchr(_fullPath, '\\');
		FILEDIR_CHAR *separator = separator1 > separator2 ? separator1 : separator2;

		if (separator[1] == '\0')
		{
			if (separator == _fullPath)
			{
				separator = NULL;
			}
			else
			{
				separator1 = ustrrchr(separator - 1, '/');
				separator2 = ustrrchr(separator - 1, '\\');
				separator = separator1 > separator2 ? separator1 : separator2;
			}
		}

		if (separator == NULL)
		{
			_cachedBasePath = (FILEDIR_CHAR *)malloc(sizeof(FILEDIR_CHAR));
			_cachedBasePath[0] = '\0';
		}
		else
		{
			int len = (int)(separator - _fullPath + 1);
			_cachedBasePath = (FILEDIR_CHAR *)malloc(sizeof(FILEDIR_CHAR) * (len + 1));
			memcpy(_cachedBasePath, _fullPath, sizeof(FILEDIR_CHAR) * len);
			_cachedBasePath[len] = '\0';
		}
	}

	return _cachedBasePath;
}

#ifdef WIN32

#define FILETIME_TO_TIME_T(FILETIME) (((((__int64)FILETIME.dwLowDateTime) | (((__int64)FILETIME.dwHighDateTime) << 32)) - 116444736000000000L) / 10000000L)

bool windows_readTimes(const FILEDIR_CHAR *fullPath, time_t *creationTime, time_t *lastModificationTime, time_t *lastAccessTime, time_t *lastStatusChangeTime)
{
	bool success = false;
	if (fullPath)
	{
		HANDLE hFile = CreateFile(fullPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if(hFile != INVALID_HANDLE_VALUE)
		{
			FILETIME ftCreate, ftAccess, ftWrite;
			if (GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))
			{
				*creationTime = FILETIME_TO_TIME_T(ftCreate);
				*lastModificationTime = FILETIME_TO_TIME_T(ftWrite);
				*lastAccessTime = FILETIME_TO_TIME_T(ftAccess);
				*lastStatusChangeTime = -1;
				success = true;
			}

			CloseHandle(hFile);
		}
	}
	return success;
}

#endif

time_t FileDir::GetLastModified()
{
#ifdef WIN32
	if (!_hasTimes && _fullPath)
	{
		_hasTimes = windows_readTimes(_fullPath, &_creationTime, &_lastModificationTime, &_lastAccessTime, &_lastStatusChangeTime);
	}
#endif
	if (_hasTimes)
	{
		return _lastModificationTime;
	}
	return -1;
}

time_t FileDir::GetCreationTime()
{
#ifdef WIN32
	if (!_hasTimes && _fullPath)
	{
		_hasTimes = windows_readTimes(_fullPath, &_creationTime, &_lastModificationTime, &_lastAccessTime, &_lastStatusChangeTime);
	}
#endif
	if (_hasTimes)
	{
		return _creationTime;
	}
	return -1;
}

time_t FileDir::GetLastAccessTime()
{
#ifdef WIN32
	if (!_hasTimes && _fullPath)
	{
		_hasTimes = windows_readTimes(_fullPath, &_creationTime, &_lastModificationTime, &_lastAccessTime, &_lastStatusChangeTime);
	}
#endif
	if (_hasTimes)
	{
		return _lastAccessTime;
	}
	return -1;
}

time_t FileDir::GetLastStatusChangeTime()
{
#ifdef WIN32
	if (!_hasTimes && _fullPath)
	{
		_hasTimes = windows_readTimes(_fullPath, &_creationTime, &_lastModificationTime, &_lastAccessTime, &_lastStatusChangeTime);
	}
#endif
	if (_hasTimes)
	{
		return _lastStatusChangeTime;
	}
	return -1;
}