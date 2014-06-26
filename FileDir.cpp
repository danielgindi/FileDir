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

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifndef FILEDIR_CHAR

#ifdef _MSC_VER
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
	_cachedFullPath = _cachedFileName = _cachedExtension = _cachedFileNameWithoutExtension = NULL;
	_isFolder = _isFile = false;
}

FileDir::~FileDir(void)
{
	if (_fullPath)
	{
		delete [] _fullPath;
		_fullPath = NULL;
	}

	if (_fileName)
	{
		delete [] _fileName;
		_fileName = NULL;
	}

	if (_cachedFileNameWithoutExtension)
	{
		delete [] _cachedFileNameWithoutExtension;
		_cachedFileNameWithoutExtension = NULL;
	}

	_cachedFullPath = _cachedFileName = _cachedExtension = NULL;
}

void FileDir::SetFullPath(const FILEDIR_CHAR *fullPath)
{
	if (_fullPath)
	{
		delete [] _fullPath;
		_fullPath = NULL;
	}

	if (_fileName)
	{
		delete [] _fileName;
		_fileName = NULL;
	}

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
	if (_fullPath != _cachedFullPath)
	{
		_cachedFullPath = _fullPath;
		_cachedExtension = NULL;
	}
	
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
	if (_fileName != _cachedFileName)
	{
		_cachedFileName = _fileName;
		if (_cachedFileNameWithoutExtension)
		{
			delete [] _cachedFileNameWithoutExtension;
			_cachedFileNameWithoutExtension = NULL;
		}
	}

	if (!_fileName) return NULL;
	
	if (!_cachedExtension)
	{
		FILEDIR_CHAR *period = ustrrchr(_fileName, '.');
		if (period == NULL)
		{
			_cachedFileNameWithoutExtension = ustrdup(_fileName);
		}
		else
		{
			int periodIndex = (int)(period - _fileName);
			FILEDIR_CHAR *fileNameWithoutExtension = new FILEDIR_CHAR[periodIndex + 1];
			memcpy(fileNameWithoutExtension, _fileName, sizeof(FILEDIR_CHAR) * periodIndex);
			fileNameWithoutExtension[periodIndex] = '\0';
			_cachedFileNameWithoutExtension = fileNameWithoutExtension;
		}
	}

	return _cachedFileNameWithoutExtension;
}