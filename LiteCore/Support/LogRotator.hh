//
// LogRotator.hh
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

#include "LogEncoder.hh"
#include "Timer.hh"
#include "RecurringEvent.hh"
#include <string>
#include <fstream>
#include <mutex>

namespace litecore {
    class LogRotator : public RecurringEvent
    {
    public:
        LogRotator(const std::string filename, 
            size_t maxBytes = 10 * 1024 * 1024,
            size_t maxCount = 10, 
            uint64_t checkInterval = kCheckInterval);

        void log(int8_t level, const char *domain, LogEncoder::ObjectRef object, const char *format, ...) __printflike(5, 6);
        void vlog(int8_t level, const char *domain, LogEncoder::ObjectRef object, const char *format, va_list args);

        LogEncoder::ObjectRef registerObject(std::string description);

        void flush() { _encoder->flush(); }
    protected:
        virtual void performEvent() override;

    private:
        static const uint64_t kCheckInterval = 60 * RecurringEvent::kTicksPerSec;

        LogRotator(LogRotator &&) = delete;
        LogRotator(const LogRotator &) = delete;

        std::unique_ptr<LogEncoder> _encoder;
        std::ofstream _fileOut;
        std::mutex _mutex;
        const size_t _maxBytes;
        const size_t _maxCount;
        const std::string _filepath;

        void _doRotate();
    };
}
