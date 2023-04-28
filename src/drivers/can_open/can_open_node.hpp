/****************************************************************************
 *
 *   Copyright (c) 2014-2023 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in
 *	the documentation and/or other materials provided with the
 *	distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *	used to endorse or promote products derived from this software
 *	without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

// CanOpenNode implementation for PX4

#pragma once

#include <px4_platform_common/px4_config.h>
#include <px4_platform_common/atomic.h>
#include <px4_platform_common/defines.h>
#include <px4_platform_common/module.h>
#include <px4_platform_common/module_params.h>
#include <px4_platform_common/px4_work_queue/ScheduledWorkItem.hpp>

#include <lib/mixer_module/mixer_module.hpp>
#include <lib/parameters/param.h>
#include <lib/perf/perf_counter.h>

#include "CO_application.h"

#include "OD.h"

#include "CANopen.h"
#include "CO_storage_nuttx.h"

using namespace time_literals;


typedef struct {
        uint16_t error_code;
        uint8_t error_register;
        uint8_t error_bit;
        uint32_t info_code;
} can_open_node_error_s;

class CanOpenNode : public OutputModuleInterface
{
        /*
         * Base interval, has to be complemented with events from the CAN driver
         * and uORB topics sending data, to decrease response time.
         */
        static constexpr uint32_t SCHEDULE_INTERVAL = 5_ms;

public:

        CanOpenNode(uint8_t node_id, int32_t bitrate);
        ~CanOpenNode() override;

        static int start(uint8_t nodeId, int32_t bitrate);

        void print_info();

        static CanOpenNode *instance() { return _instance; }

private:
        static void can_open_error_cb(const uint16_t ident, const uint16_t errorCode, const uint8_t errorRegister, const uint8_t errorBit, const uint32_t infoCode);
        void init();
        void Run() override;

        MixingOutput _mixing_output{"CO_EC", 5, *this, MixingOutput::SchedulingPolicy::Disabled, false, false};


        bool updateOutputs(bool stop_motors, uint16_t outputs[MAX_ACTUATORS],
                           unsigned num_outputs, unsigned num_control_groups_updated) override;

        void CO_high_pri_work(uint32_t time_difference_us);
        void CO_low_pri_work(uint32_t time_difference_us);
        void CO_app_process(CO_t *co, uint32_t time_difference_us);

        px4::atomic_bool _task_should_exit{false};	///< flag to indicate to tear down the CAN driver

        CO_t *_CO_instance{nullptr};
#if (CO_CONFIG_STORAGE) & CO_CONFIG_STORAGE_ENABLE
        CO_storage_t _storage;
        CO_storage_entry_t _storage_entries[1];
#endif
        uint32_t _err_info_stack{0};
        uint32_t _err_info_app{0};
        uint32_t _storage_error{0};
        uint32_t _storage_error_prev{0};
        uint32_t _heap_memory_used{0};
        uint8_t  _node_id{0};
        int32_t _bitrate{0};

        bool _initialized{false};
        bool _received_motor_telem{false};

        static CanOpenNode *_instance;
        static can_open_node_error_s _can_open_node_error[2];

        hrt_abstime _run_end{0};
        uint32_t _medium_pri_timer{0};
        uint32_t _low_pri_timer{0};

        // Parameters
        DEFINE_PARAMETERS(
                (ParamInt<px4::params::CANOPEN_ENABLE>) _param_canopen_enable,
                (ParamInt<px4::params::CANOPEN_BITRATE>) _param_canopen_bitrate,
                (ParamInt<px4::params::CANOPEN_NODE_ID>) _param_canopen_nodeid

        )

        perf_counter_t _cycle_perf{perf_alloc(PC_ELAPSED, MODULE_NAME": cycle time")};
        perf_counter_t _interval_perf{perf_alloc(PC_INTERVAL, MODULE_NAME": cycle interval")};


};