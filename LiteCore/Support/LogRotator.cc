//
// LogRotator.cc
//
// Copyright (c) 2018 Couchbase, Inc All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "LogRotator.hh"
#include "FilePath.hh"
#include "PlatformIO.hh"
#include <fstream>
#include <string>
using namespace std;

namespace litecore {

    LogRotator::LogRotator(const string filepath, size_t maxBytes, size_t maxCount, uint64_t checkInterval) :
        RecurringEvent(checkInterval),
        _fileOut(filepath, ofstream::out|ofstream::trunc|ofstream::binary, _SH_DENYWR),
        _maxBytes(maxBytes),
        _maxCount(maxCount),
        _filepath(filepath)
    {
        _encoder = make_unique<LogEncoder>(_fileOut);
    }

    void LogRotator::log(int8_t level, const char* domain, LogEncoder::ObjectRef object, const char* format, ...)
    {
        lock_guard<mutex> lock(_mutex);
        va_list args;
        va_start(args, format);
        _encoder->vlog(level, domain, object, format, args);
        va_end(args);
    }

    void LogRotator::vlog(int8_t level, const char* domain, LogEncoder::ObjectRef object, const char* format, va_list args)
    {
        lock_guard<mutex> lock(_mutex);
        _encoder->vlog(level, domain, object, format, args);
    }

    LogEncoder::ObjectRef LogRotator::registerObject(std::string description)
    {
        lock_guard<mutex> lock(_mutex);
        return _encoder->registerObject(description);
    }

    void LogRotator::performEvent()
    {
        lock_guard<mutex> lock(_mutex);
        bool needRotate = false;
        size_t maxBytes = _maxBytes;
        _encoder->flush();
        _encoder->withStream([&needRotate, maxBytes](ostream &s)
        {
            needRotate = s.tellp() >= maxBytes;
        });

        if(needRotate) {
            _doRotate();
        }
    }

    void LogRotator::_doRotate()
    {
        // Save state from the encoder
        const std::unordered_map<unsigned, std::string> objects = _encoder->_objects;
        const LogEncoder::ObjectRef lastObjectRef = _encoder->_lastObjectRef;

        _fileOut.close();

        // Rollover files
        auto maxFilename = _filepath + "." + to_string(_maxCount - 1);
        unlink_u8(maxFilename.c_str());
        struct stat s{};
        for(size_t i = _maxCount - 1; i > 0; i--) {
            auto filePath = _filepath + "." + to_string(i);
            auto next = _filepath + "." + to_string(i+1);
            if(stat_u8(filePath.c_str(), &s) == 0) {
                rename_u8(filePath.c_str(), next.c_str());
            }
        }

        // Rollover final file, and reopen LogEncoder
        rename_u8(_filepath.c_str(), maxFilename.c_str());
        
        _fileOut.open(_filepath, ofstream::out|ofstream::trunc|ofstream::binary);
        _encoder = make_unique<LogEncoder>(_fileOut);

        // Restore LogEncoder state
        _encoder->_objects = objects;
        _encoder->_lastObjectRef = lastObjectRef;
    }

}