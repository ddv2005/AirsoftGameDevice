// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "vfs_api.h"

extern "C" {
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>
}

#include "FS_EX.h"

using namespace fs;

class FSEXImpl : public VFSImpl
{
public:
    FSEXImpl();
    virtual ~FSEXImpl() { }
    virtual bool exists(const char* path);
};

FSEXImpl::FSEXImpl()
{
}

bool FSEXImpl::exists(const char* path)
{
    File f = open(path, "r");
    return (f == true) && !f.isDirectory();
}

FS_EX::FS_EX() : FS(FSImplPtr(new FSEXImpl()))
{

}

bool FS_EX::begin(const char * basePath)
{
    _impl->mountpoint(basePath);
    return true;
}

void FS_EX::end()
{
   _impl->mountpoint(NULL);
}

FS_EX FSEX;

