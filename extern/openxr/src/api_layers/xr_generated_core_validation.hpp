// *********** THIS FILE IS GENERATED - DO NOT EDIT ***********
//     See validation_layer_generator.py for modifications
// ************************************************************

// Copyright (c) 2017-2019 The Khronos Group Inc.
// Copyright (c) 2017-2019 Valve Corporation
// Copyright (c) 2017-2019 LunarG, Inc.
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
//
// Author: Mark Young <marky@lunarg.com>
//

#pragma once
#include "xr_generated_dispatch_table.h"
#include "validation_utils.h"
#include "api_layer_platform_defines.h"
#include "xr_dependencies.h"
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <vector>
#include <string>
#include <unordered_map>
#include <thread>
#include <mutex>


// Unordered Map associating pointer to a vector of session label information to a session's handle
extern std::unordered_map<XrSession, std::vector<GenValidUsageXrInternalSessionLabel*>*> g_xr_session_labels;


// ---- Core 1.0 commands
XrResult GenValidUsageXrGetInstanceProcAddr(
    XrInstance instance,
    const char* name,
    PFN_xrVoidFunction* function);
XrResult CoreValidationXrCreateInstance(
    const XrInstanceCreateInfo* createInfo,
    XrInstance* instance);
XrResult GenValidUsageInputsXrCreateInstance(const XrInstanceCreateInfo* createInfo, XrInstance* instance);
XrResult GenValidUsageNextXrCreateInstance(
    const XrInstanceCreateInfo* createInfo,
    XrInstance* instance);
XrResult CoreValidationXrDestroyInstance(
    XrInstance instance);
XrResult GenValidUsageInputsXrDestroyInstance(XrInstance instance);
XrResult GenValidUsageNextXrDestroyInstance(
    XrInstance instance);
XrResult CoreValidationXrCreateSession(
    XrInstance instance,
    const XrSessionCreateInfo* createInfo,
    XrSession* session);
XrResult GenValidUsageInputsXrCreateSession(XrInstance instance, const XrSessionCreateInfo* createInfo, XrSession* session);
XrResult GenValidUsageNextXrCreateSession(
    XrInstance instance,
    const XrSessionCreateInfo* createInfo,
    XrSession* session);

// ---- XR_KHR_android_thread_settings extension commands

// ---- XR_KHR_android_surface_swapchain extension commands

// ---- XR_KHR_opengl_enable extension commands

// ---- XR_KHR_opengl_es_enable extension commands

// ---- XR_KHR_vulkan_enable extension commands

// ---- XR_KHR_D3D11_enable extension commands

// ---- XR_KHR_D3D12_enable extension commands

// ---- XR_KHR_visibility_mask extension commands

// ---- XR_KHR_win32_convert_performance_counter_time extension commands

// ---- XR_KHR_convert_timespec_time extension commands

// ---- XR_EXT_performance_settings extension commands

// ---- XR_EXT_thermal_query extension commands

// ---- XR_EXT_debug_utils extension commands
XrResult CoreValidationXrSetDebugUtilsObjectNameEXT(
    XrInstance instance,
    const XrDebugUtilsObjectNameInfoEXT* nameInfo);
XrResult GenValidUsageInputsXrSetDebugUtilsObjectNameEXT(XrInstance instance, const XrDebugUtilsObjectNameInfoEXT* nameInfo);
XrResult GenValidUsageNextXrSetDebugUtilsObjectNameEXT(
    XrInstance instance,
    const XrDebugUtilsObjectNameInfoEXT* nameInfo);
XrResult CoreValidationXrCreateDebugUtilsMessengerEXT(
    XrInstance instance,
    const XrDebugUtilsMessengerCreateInfoEXT* createInfo,
    XrDebugUtilsMessengerEXT* messenger);
XrResult GenValidUsageInputsXrCreateDebugUtilsMessengerEXT(XrInstance instance, const XrDebugUtilsMessengerCreateInfoEXT* createInfo, XrDebugUtilsMessengerEXT* messenger);
XrResult GenValidUsageNextXrCreateDebugUtilsMessengerEXT(
    XrInstance instance,
    const XrDebugUtilsMessengerCreateInfoEXT* createInfo,
    XrDebugUtilsMessengerEXT* messenger);
XrResult CoreValidationXrDestroyDebugUtilsMessengerEXT(
    XrDebugUtilsMessengerEXT messenger);
XrResult GenValidUsageInputsXrDestroyDebugUtilsMessengerEXT(XrDebugUtilsMessengerEXT messenger);
XrResult GenValidUsageNextXrDestroyDebugUtilsMessengerEXT(
    XrDebugUtilsMessengerEXT messenger);
XrResult CoreValidationXrSessionBeginDebugUtilsLabelRegionEXT(
    XrSession session,
    const XrDebugUtilsLabelEXT* labelInfo);
XrResult GenValidUsageInputsXrSessionBeginDebugUtilsLabelRegionEXT(XrSession session, const XrDebugUtilsLabelEXT* labelInfo);
XrResult GenValidUsageNextXrSessionBeginDebugUtilsLabelRegionEXT(
    XrSession session,
    const XrDebugUtilsLabelEXT* labelInfo);
XrResult CoreValidationXrSessionEndDebugUtilsLabelRegionEXT(
    XrSession session);
XrResult GenValidUsageInputsXrSessionEndDebugUtilsLabelRegionEXT(XrSession session);
XrResult GenValidUsageNextXrSessionEndDebugUtilsLabelRegionEXT(
    XrSession session);
XrResult CoreValidationXrSessionInsertDebugUtilsLabelEXT(
    XrSession session,
    const XrDebugUtilsLabelEXT* labelInfo);
XrResult GenValidUsageInputsXrSessionInsertDebugUtilsLabelEXT(XrSession session, const XrDebugUtilsLabelEXT* labelInfo);
XrResult GenValidUsageNextXrSessionInsertDebugUtilsLabelEXT(
    XrSession session,
    const XrDebugUtilsLabelEXT* labelInfo);

// Current API version of the Core Validation API Layer
#define XR_CORE_VALIDATION_API_VERSION XR_CURRENT_API_VERSION

// Externs for Core Validation
extern InstanceHandleInfo g_instance_info;
extern HandleInfo<XrSession> g_session_info;
extern HandleInfo<XrSpace> g_space_info;
extern HandleInfo<XrAction> g_action_info;
extern HandleInfo<XrSwapchain> g_swapchain_info;
extern HandleInfo<XrActionSet> g_actionset_info;
extern HandleInfo<XrDebugUtilsMessengerEXT> g_debugutilsmessengerext_info;void GenValidUsageCleanUpMaps(GenValidUsageXrInstanceInfo *instance_info);


// Function to convert XrObjectType to string
std::string GenValidUsageXrObjectTypeToString(const XrObjectType& type);

// Function to record all the core validation information
extern void CoreValidLogMessage(GenValidUsageXrInstanceInfo *instance_info, const std::string &message_id,
                                GenValidUsageDebugSeverity message_severity, const std::string &command_name,
                                std::vector<GenValidUsageXrObjectInfo> objects_info, const std::string &message);

