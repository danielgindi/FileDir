//
//  FileDirController.h
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

#pragma once

#include "FileDir.h"

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#endif

class FileDirController
{
public:
	FileDirController(void);
	~FileDirController(void);

#ifdef _MSC_VER /* Wide char */
	bool EnumerateFilesAtPath(const wchar_t *path);
#else /* UTF8 */
	bool EnumerateFilesAtPath(const char *path);
#endif
	FileDir * NextFile();
#ifdef _MSC_VER /* Wide char */
	FileDir * GetFileInfo(const wchar_t *path);
#else /* UTF8 */
	FileDir * GetFileInfo(const char *path);
#endif
	void Close();

	inline bool HasNext() { return _hasNext; }

private:
	bool _hasNext;

#ifdef _MSC_VER /* Wide char */
	wchar_t *_basePath;
#else /* UTF8 */
	char *_basePath;
#endif

	int _basePathLength;
	
#ifdef _MSC_VER
	HANDLE _winFindHandle;
	WIN32_FIND_DATAW _winFindData;
#else
	DIR *_unixFindDir;
	dirent *_unixFindDirEntry;
#endif
};

