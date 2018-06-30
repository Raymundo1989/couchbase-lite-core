//
// RecurringEvent.cc
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

#include "RecurringEvent.hh"
#include <chrono>
#include <functional>
using namespace std;

namespace litecore {
    RecurringEvent::RecurringEvent(uint64_t interval) :
        _timer(bind(&RecurringEvent::_timerCallback, this)),
        _interval(interval)
    {
        _st.reset();
        schedule();
    }

    void RecurringEvent::schedule()
    {
        _cancelled = false;
        if(!_timer.scheduled()) {
            _timer.fireAfter(chrono::microseconds(_interval));
        }
    }

    int64_t RecurringEvent::_timeElapsed() const
    {
        return int64_t(_st.elapsed() * kTicksPerSec);
    }

    void RecurringEvent::_timerCallback()
    {
        if(_cancelled) {
            return;
        }

        const auto timeSinceSave = _timeElapsed() - _lastFired;
        if (timeSinceSave >= _interval) {
            performEvent();
            _lastFired = _timeElapsed();
            _timer.fireAfter(chrono::microseconds(_interval));
        } else {
            _timer.fireAfter(std::chrono::microseconds(_interval - timeSinceSave));
        }
    }

}