//
// RecurringEvent.hh
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

#include "Timer.hh"
#include "Stopwatch.hh"

namespace litecore {
    class RecurringEvent
    {
    public:
        // The units we count in are microseconds.
        static constexpr unsigned kTicksPerSec = 1000000;
        
        void stop() { _timer.stop(); }
    protected:
        int64_t _lastFired {0};

        RecurringEvent(uint64_t interval);

        virtual void performEvent() = 0;
        void schedule();
    private:
        actor::Timer _timer;
        fleece::Stopwatch _st;
        uint64_t _interval;
        bool _cancelled {false};

        int64_t _timeElapsed() const;
        void _timerCallback();
    };
}