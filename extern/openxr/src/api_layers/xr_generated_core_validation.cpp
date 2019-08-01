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

#include "xr_generated_core_validation.hpp"

#include "api_layer_platform_defines.h"
#include "hex_and_handles.h"
#include "validation_utils.h"
#include "xr_dependencies.h"
#include "xr_generated_dispatch_table.h"

#include "api_layer_platform_defines.h"
#include "xr_dependencies.h"
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>


// Structure used for indicating status of 'flags' test.
enum ValidateXrFlagsResult {
    VALIDATE_XR_FLAGS_ZERO,
    VALIDATE_XR_FLAGS_INVALID,
    VALIDATE_XR_FLAGS_SUCCESS,
};

// Unordered Map associating pointer to a vector of session label information to a session's handle
std::unordered_map<XrSession, std::vector<GenValidUsageXrInternalSessionLabel*>*> g_xr_session_labels;

InstanceHandleInfo g_instance_info;
HandleInfo<XrSession> g_session_info;
HandleInfo<XrSpace> g_space_info;
HandleInfo<XrAction> g_action_info;
HandleInfo<XrSwapchain> g_swapchain_info;
HandleInfo<XrActionSet> g_actionset_info;
HandleInfo<XrDebugUtilsMessengerEXT> g_debugutilsmessengerext_info;
ValidateXrHandleResult VerifyXrInstanceHandle(const XrInstance* handle_to_check);
ValidateXrHandleResult VerifyXrSessionHandle(const XrSession* handle_to_check);
ValidateXrHandleResult VerifyXrSpaceHandle(const XrSpace* handle_to_check);
ValidateXrHandleResult VerifyXrActionHandle(const XrAction* handle_to_check);
ValidateXrHandleResult VerifyXrSwapchainHandle(const XrSwapchain* handle_to_check);
ValidateXrHandleResult VerifyXrActionSetHandle(const XrActionSet* handle_to_check);
ValidateXrHandleResult VerifyXrDebugUtilsMessengerEXTHandle(const XrDebugUtilsMessengerEXT* handle_to_check);

// Write out prototypes for handle parent verification functions
bool VerifyXrParent(XrObjectType handle1_type, const uint64_t handle1,
                    XrObjectType handle2_type, const uint64_t handle2,
                    bool check_this);

// Function to check if an extension has been enabled
bool ExtensionEnabled(std::vector<std::string> &extensions, const char* const check_extension_name);

// Functions to validate structures
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrApiLayerProperties* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrExtensionProperties* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrApplicationInfo* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrInstanceCreateInfo* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrInstanceProperties* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrEventDataBuffer* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSystemGetInfo* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSystemGraphicsProperties* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSystemTrackingProperties* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSystemProperties* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSessionCreateInfo* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrVector3f* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSpaceVelocity* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrQuaternionf* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrPosef* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrReferenceSpaceCreateInfo* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrExtent2Df* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrActionSpaceCreateInfo* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSpaceLocation* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrViewConfigurationProperties* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrViewConfigurationView* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSwapchainCreateInfo* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSwapchainImageBaseHeader* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSwapchainImageAcquireInfo* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSwapchainImageWaitInfo* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSwapchainImageReleaseInfo* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSessionBeginInfo* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrFrameWaitInfo* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrFrameState* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrFrameBeginInfo* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrCompositionLayerBaseHeader* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrFrameEndInfo* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrViewLocateInfo* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrViewState* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrFovf* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrView* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrActionSetCreateInfo* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrActionCreateInfo* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrActionSuggestedBinding* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrInteractionProfileSuggestedBinding* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSessionActionSetsAttachInfo* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrInteractionProfileState* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrActionStateGetInfo* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrActionStateBoolean* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrActionStateFloat* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrVector2f* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrActionStateVector2f* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrActionStatePose* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrActiveActionSet* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrActionsSyncInfo* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrBoundSourcesForActionEnumerateInfo* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrInputSourceLocalizedNameGetInfo* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrHapticActionInfo* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrHapticBaseHeader* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrBaseInStructure* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrBaseOutStructure* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrOffset2Di* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrExtent2Di* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrRect2Di* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSwapchainSubImage* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrCompositionLayerProjectionView* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrCompositionLayerProjection* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrCompositionLayerQuad* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrEventDataBaseHeader* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrEventDataEventsLost* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrEventDataInstanceLossPending* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrEventDataSessionStateChanged* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrEventDataReferenceSpaceChangePending* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrEventDataInteractionProfileChanged* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrHapticVibration* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrOffset2Df* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrRect2Df* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrVector4f* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrColor4f* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrCompositionLayerCubeKHR* value);
#if defined(XR_USE_PLATFORM_ANDROID)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrInstanceCreateInfoAndroidKHR* value);
#endif // defined(XR_USE_PLATFORM_ANDROID)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrCompositionLayerDepthInfoKHR* value);
#if defined(XR_USE_GRAPHICS_API_VULKAN)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrVulkanSwapchainFormatListCreateInfoKHR* value);
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrCompositionLayerCylinderKHR* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrCompositionLayerEquirectKHR* value);
#if defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_WIN32)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsBindingOpenGLWin32KHR* value);
#endif // defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_WIN32)
#if defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_XLIB)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsBindingOpenGLXlibKHR* value);
#endif // defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_XLIB)
#if defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_XCB)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsBindingOpenGLXcbKHR* value);
#endif // defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_XCB)
#if defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_WAYLAND)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsBindingOpenGLWaylandKHR* value);
#endif // defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_WAYLAND)
#if defined(XR_USE_GRAPHICS_API_OPENGL)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSwapchainImageOpenGLKHR* value);
#endif // defined(XR_USE_GRAPHICS_API_OPENGL)
#if defined(XR_USE_GRAPHICS_API_OPENGL)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsRequirementsOpenGLKHR* value);
#endif // defined(XR_USE_GRAPHICS_API_OPENGL)
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES) && defined(XR_USE_PLATFORM_ANDROID)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsBindingOpenGLESAndroidKHR* value);
#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES) && defined(XR_USE_PLATFORM_ANDROID)
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSwapchainImageOpenGLESKHR* value);
#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES)
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsRequirementsOpenGLESKHR* value);
#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsBindingVulkanKHR* value);
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSwapchainImageVulkanKHR* value);
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsRequirementsVulkanKHR* value);
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_D3D11)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsBindingD3D11KHR* value);
#endif // defined(XR_USE_GRAPHICS_API_D3D11)
#if defined(XR_USE_GRAPHICS_API_D3D11)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSwapchainImageD3D11KHR* value);
#endif // defined(XR_USE_GRAPHICS_API_D3D11)
#if defined(XR_USE_GRAPHICS_API_D3D11)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsRequirementsD3D11KHR* value);
#endif // defined(XR_USE_GRAPHICS_API_D3D11)
#if defined(XR_USE_GRAPHICS_API_D3D12)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsBindingD3D12KHR* value);
#endif // defined(XR_USE_GRAPHICS_API_D3D12)
#if defined(XR_USE_GRAPHICS_API_D3D12)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSwapchainImageD3D12KHR* value);
#endif // defined(XR_USE_GRAPHICS_API_D3D12)
#if defined(XR_USE_GRAPHICS_API_D3D12)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsRequirementsD3D12KHR* value);
#endif // defined(XR_USE_GRAPHICS_API_D3D12)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrVisibilityMaskKHR* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrEventDataVisibilityMaskChangedKHR* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrEventDataPerfSettingsEXT* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrDebugUtilsObjectNameInfoEXT* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrDebugUtilsLabelEXT* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrDebugUtilsMessengerCallbackDataEXT* value);
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrDebugUtilsMessengerCreateInfoEXT* value);
// Function used to clean up any residual map values that point to an instance prior to that
// instance being deleted.
void GenValidUsageCleanUpMaps(GenValidUsageXrInstanceInfo *instance_info) {
    EraseAllInstanceTableMapElements(instance_info);
    g_session_info.removeHandlesForInstance(instance_info);
    g_space_info.removeHandlesForInstance(instance_info);
    g_action_info.removeHandlesForInstance(instance_info);
    g_swapchain_info.removeHandlesForInstance(instance_info);
    g_actionset_info.removeHandlesForInstance(instance_info);
    g_debugutilsmessengerext_info.removeHandlesForInstance(instance_info);
}

// Function to convert XrObjectType to string
std::string GenValidUsageXrObjectTypeToString(const XrObjectType& type) {
    std::string object_string;
    if (type == XR_OBJECT_TYPE_UNKNOWN) {
        object_string = "Unknown XR Object";
    } else if (type == XR_OBJECT_TYPE_INSTANCE) {
        object_string = "XrInstance";
    } else if (type == XR_OBJECT_TYPE_SESSION) {
        object_string = "XrSession";
    } else if (type == XR_OBJECT_TYPE_SWAPCHAIN) {
        object_string = "XrSwapchain";
    } else if (type == XR_OBJECT_TYPE_SPACE) {
        object_string = "XrSpace";
    } else if (type == XR_OBJECT_TYPE_ACTION_SET) {
        object_string = "XrActionSet";
    } else if (type == XR_OBJECT_TYPE_ACTION) {
        object_string = "XrAction";
    } else if (type == XR_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT) {
        object_string = "XrDebugUtilsMessengerEXT";
    }
    return object_string;
}

// Structure used for state validation.

// Result return value for next chain validation
enum NextChainResult {
    NEXT_CHAIN_RESULT_VALID = 0,
    NEXT_CHAIN_RESULT_ERROR = -1,
    NEXT_CHAIN_RESULT_DUPLICATE_STRUCT = -2,
};

// Prototype for validateNextChain command (it uses the validate structure commands so add it after
NextChainResult ValidateNextChain(GenValidUsageXrInstanceInfo *instance_info,
                                  const std::string &command_name,
                                  std::vector<GenValidUsageXrObjectInfo>& objects_info,
                                  const void* next,
                                  std::vector<XrStructureType>& valid_ext_structs,
                                  std::vector<XrStructureType>& encountered_structs,
                                  std::vector<XrStructureType>& duplicate_structs);

// Function to validate XrInstanceCreateFlags flags
ValidateXrFlagsResult ValidateXrInstanceCreateFlags(const XrFlags64 value) {
    if (0 == value) {
        return VALIDATE_XR_FLAGS_ZERO;
    }
    XrFlags64 int_value = value;
    if (int_value != 0) {
        // Something is left, it must be invalid
        return VALIDATE_XR_FLAGS_INVALID;
    }
    return VALIDATE_XR_FLAGS_SUCCESS;
}

// Function to validate XrSessionCreateFlags flags
ValidateXrFlagsResult ValidateXrSessionCreateFlags(const XrFlags64 value) {
    if (0 == value) {
        return VALIDATE_XR_FLAGS_ZERO;
    }
    XrFlags64 int_value = value;
    if (int_value != 0) {
        // Something is left, it must be invalid
        return VALIDATE_XR_FLAGS_INVALID;
    }
    return VALIDATE_XR_FLAGS_SUCCESS;
}

// Function to validate XrSpaceVelocityFlags flags
ValidateXrFlagsResult ValidateXrSpaceVelocityFlags(const XrFlags64 value) {
    if (0 == value) {
        return VALIDATE_XR_FLAGS_ZERO;
    }
    XrFlags64 int_value = value;
    if ((int_value & XR_SPACE_VELOCITY_LINEAR_VALID_BIT) != 0) {
        // Clear the value XR_SPACE_VELOCITY_LINEAR_VALID_BIT since it is valid
        int_value &= ~XR_SPACE_VELOCITY_LINEAR_VALID_BIT;
    }
    if ((int_value & XR_SPACE_VELOCITY_ANGULAR_VALID_BIT) != 0) {
        // Clear the value XR_SPACE_VELOCITY_ANGULAR_VALID_BIT since it is valid
        int_value &= ~XR_SPACE_VELOCITY_ANGULAR_VALID_BIT;
    }
    if (int_value != 0) {
        // Something is left, it must be invalid
        return VALIDATE_XR_FLAGS_INVALID;
    }
    return VALIDATE_XR_FLAGS_SUCCESS;
}

// Function to validate XrSpaceLocationFlags flags
ValidateXrFlagsResult ValidateXrSpaceLocationFlags(const XrFlags64 value) {
    if (0 == value) {
        return VALIDATE_XR_FLAGS_ZERO;
    }
    XrFlags64 int_value = value;
    if ((int_value & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
        // Clear the value XR_SPACE_LOCATION_ORIENTATION_VALID_BIT since it is valid
        int_value &= ~XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
    }
    if ((int_value & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0) {
        // Clear the value XR_SPACE_LOCATION_POSITION_VALID_BIT since it is valid
        int_value &= ~XR_SPACE_LOCATION_POSITION_VALID_BIT;
    }
    if ((int_value & XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT) != 0) {
        // Clear the value XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT since it is valid
        int_value &= ~XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;
    }
    if ((int_value & XR_SPACE_LOCATION_POSITION_TRACKED_BIT) != 0) {
        // Clear the value XR_SPACE_LOCATION_POSITION_TRACKED_BIT since it is valid
        int_value &= ~XR_SPACE_LOCATION_POSITION_TRACKED_BIT;
    }
    if (int_value != 0) {
        // Something is left, it must be invalid
        return VALIDATE_XR_FLAGS_INVALID;
    }
    return VALIDATE_XR_FLAGS_SUCCESS;
}

// Function to validate XrSwapchainCreateFlags flags
ValidateXrFlagsResult ValidateXrSwapchainCreateFlags(const XrFlags64 value) {
    if (0 == value) {
        return VALIDATE_XR_FLAGS_ZERO;
    }
    XrFlags64 int_value = value;
    if ((int_value & XR_SWAPCHAIN_CREATE_PROTECTED_CONTENT_BIT) != 0) {
        // Clear the value XR_SWAPCHAIN_CREATE_PROTECTED_CONTENT_BIT since it is valid
        int_value &= ~XR_SWAPCHAIN_CREATE_PROTECTED_CONTENT_BIT;
    }
    if ((int_value & XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT) != 0) {
        // Clear the value XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT since it is valid
        int_value &= ~XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT;
    }
    if (int_value != 0) {
        // Something is left, it must be invalid
        return VALIDATE_XR_FLAGS_INVALID;
    }
    return VALIDATE_XR_FLAGS_SUCCESS;
}

// Function to validate XrSwapchainUsageFlags flags
ValidateXrFlagsResult ValidateXrSwapchainUsageFlags(const XrFlags64 value) {
    if (0 == value) {
        return VALIDATE_XR_FLAGS_ZERO;
    }
    XrFlags64 int_value = value;
    if ((int_value & XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT) != 0) {
        // Clear the value XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT since it is valid
        int_value &= ~XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if ((int_value & XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0) {
        // Clear the value XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT since it is valid
        int_value &= ~XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if ((int_value & XR_SWAPCHAIN_USAGE_UNORDERED_ACCESS_BIT) != 0) {
        // Clear the value XR_SWAPCHAIN_USAGE_UNORDERED_ACCESS_BIT since it is valid
        int_value &= ~XR_SWAPCHAIN_USAGE_UNORDERED_ACCESS_BIT;
    }
    if ((int_value & XR_SWAPCHAIN_USAGE_TRANSFER_SRC_BIT) != 0) {
        // Clear the value XR_SWAPCHAIN_USAGE_TRANSFER_SRC_BIT since it is valid
        int_value &= ~XR_SWAPCHAIN_USAGE_TRANSFER_SRC_BIT;
    }
    if ((int_value & XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT) != 0) {
        // Clear the value XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT since it is valid
        int_value &= ~XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
    }
    if ((int_value & XR_SWAPCHAIN_USAGE_SAMPLED_BIT) != 0) {
        // Clear the value XR_SWAPCHAIN_USAGE_SAMPLED_BIT since it is valid
        int_value &= ~XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
    }
    if ((int_value & XR_SWAPCHAIN_USAGE_MUTABLE_FORMAT_BIT) != 0) {
        // Clear the value XR_SWAPCHAIN_USAGE_MUTABLE_FORMAT_BIT since it is valid
        int_value &= ~XR_SWAPCHAIN_USAGE_MUTABLE_FORMAT_BIT;
    }
    if (int_value != 0) {
        // Something is left, it must be invalid
        return VALIDATE_XR_FLAGS_INVALID;
    }
    return VALIDATE_XR_FLAGS_SUCCESS;
}

// Function to validate XrCompositionLayerFlags flags
ValidateXrFlagsResult ValidateXrCompositionLayerFlags(const XrFlags64 value) {
    if (0 == value) {
        return VALIDATE_XR_FLAGS_ZERO;
    }
    XrFlags64 int_value = value;
    if ((int_value & XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT) != 0) {
        // Clear the value XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT since it is valid
        int_value &= ~XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
    }
    if ((int_value & XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT) != 0) {
        // Clear the value XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT since it is valid
        int_value &= ~XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
    }
    if ((int_value & XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT) != 0) {
        // Clear the value XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT since it is valid
        int_value &= ~XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT;
    }
    if (int_value != 0) {
        // Something is left, it must be invalid
        return VALIDATE_XR_FLAGS_INVALID;
    }
    return VALIDATE_XR_FLAGS_SUCCESS;
}

// Function to validate XrViewStateFlags flags
ValidateXrFlagsResult ValidateXrViewStateFlags(const XrFlags64 value) {
    if (0 == value) {
        return VALIDATE_XR_FLAGS_ZERO;
    }
    XrFlags64 int_value = value;
    if ((int_value & XR_VIEW_STATE_ORIENTATION_VALID_BIT) != 0) {
        // Clear the value XR_VIEW_STATE_ORIENTATION_VALID_BIT since it is valid
        int_value &= ~XR_VIEW_STATE_ORIENTATION_VALID_BIT;
    }
    if ((int_value & XR_VIEW_STATE_POSITION_VALID_BIT) != 0) {
        // Clear the value XR_VIEW_STATE_POSITION_VALID_BIT since it is valid
        int_value &= ~XR_VIEW_STATE_POSITION_VALID_BIT;
    }
    if ((int_value & XR_VIEW_STATE_ORIENTATION_TRACKED_BIT) != 0) {
        // Clear the value XR_VIEW_STATE_ORIENTATION_TRACKED_BIT since it is valid
        int_value &= ~XR_VIEW_STATE_ORIENTATION_TRACKED_BIT;
    }
    if ((int_value & XR_VIEW_STATE_POSITION_TRACKED_BIT) != 0) {
        // Clear the value XR_VIEW_STATE_POSITION_TRACKED_BIT since it is valid
        int_value &= ~XR_VIEW_STATE_POSITION_TRACKED_BIT;
    }
    if (int_value != 0) {
        // Something is left, it must be invalid
        return VALIDATE_XR_FLAGS_INVALID;
    }
    return VALIDATE_XR_FLAGS_SUCCESS;
}

// Function to validate XrInputSourceLocalizedNameFlags flags
ValidateXrFlagsResult ValidateXrInputSourceLocalizedNameFlags(const XrFlags64 value) {
    if (0 == value) {
        return VALIDATE_XR_FLAGS_ZERO;
    }
    XrFlags64 int_value = value;
    if ((int_value & XR_INPUT_SOURCE_LOCALIZED_NAME_USER_PATH_BIT) != 0) {
        // Clear the value XR_INPUT_SOURCE_LOCALIZED_NAME_USER_PATH_BIT since it is valid
        int_value &= ~XR_INPUT_SOURCE_LOCALIZED_NAME_USER_PATH_BIT;
    }
    if ((int_value & XR_INPUT_SOURCE_LOCALIZED_NAME_INTERACTION_PROFILE_BIT) != 0) {
        // Clear the value XR_INPUT_SOURCE_LOCALIZED_NAME_INTERACTION_PROFILE_BIT since it is valid
        int_value &= ~XR_INPUT_SOURCE_LOCALIZED_NAME_INTERACTION_PROFILE_BIT;
    }
    if ((int_value & XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT) != 0) {
        // Clear the value XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT since it is valid
        int_value &= ~XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT;
    }
    if (int_value != 0) {
        // Something is left, it must be invalid
        return VALIDATE_XR_FLAGS_INVALID;
    }
    return VALIDATE_XR_FLAGS_SUCCESS;
}

// Function to validate XrDebugUtilsMessageSeverityFlagsEXT flags
ValidateXrFlagsResult ValidateXrDebugUtilsMessageSeverityFlagsEXT(const XrFlags64 value) {
    if (0 == value) {
        return VALIDATE_XR_FLAGS_ZERO;
    }
    XrFlags64 int_value = value;
    if ((int_value & XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) != 0) {
        // Clear the value XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT since it is valid
        int_value &= ~XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    }
    if ((int_value & XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) != 0) {
        // Clear the value XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT since it is valid
        int_value &= ~XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    }
    if ((int_value & XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0) {
        // Clear the value XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT since it is valid
        int_value &= ~XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    }
    if ((int_value & XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0) {
        // Clear the value XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT since it is valid
        int_value &= ~XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    }
    if (int_value != 0) {
        // Something is left, it must be invalid
        return VALIDATE_XR_FLAGS_INVALID;
    }
    return VALIDATE_XR_FLAGS_SUCCESS;
}

// Function to validate XrDebugUtilsMessageTypeFlagsEXT flags
ValidateXrFlagsResult ValidateXrDebugUtilsMessageTypeFlagsEXT(const XrFlags64 value) {
    if (0 == value) {
        return VALIDATE_XR_FLAGS_ZERO;
    }
    XrFlags64 int_value = value;
    if ((int_value & XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) != 0) {
        // Clear the value XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT since it is valid
        int_value &= ~XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
    }
    if ((int_value & XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0) {
        // Clear the value XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT since it is valid
        int_value &= ~XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    }
    if ((int_value & XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) != 0) {
        // Clear the value XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT since it is valid
        int_value &= ~XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    }
    if ((int_value & XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT) != 0) {
        // Clear the value XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT since it is valid
        int_value &= ~XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
    }
    if (int_value != 0) {
        // Something is left, it must be invalid
        return VALIDATE_XR_FLAGS_INVALID;
    }
    return VALIDATE_XR_FLAGS_SUCCESS;
}

// Function to validate XrResult enum
bool ValidateXrEnum(GenValidUsageXrInstanceInfo *instance_info,
                    const std::string &command_name,
                    const std::string &validation_name,
                    const std::string &item_name,
                    std::vector<GenValidUsageXrObjectInfo>& objects_info,
                    const XrResult value) {
    switch (value) {
        case XR_SUCCESS:
            return true;
        case XR_TIMEOUT_EXPIRED:
            return true;
        case XR_SESSION_LOSS_PENDING:
            return true;
        case XR_EVENT_UNAVAILABLE:
            return true;
        case XR_SPACE_BOUNDS_UNAVAILABLE:
            return true;
        case XR_SESSION_NOT_FOCUSED:
            return true;
        case XR_FRAME_DISCARDED:
            return true;
        case XR_ERROR_VALIDATION_FAILURE:
            return true;
        case XR_ERROR_RUNTIME_FAILURE:
            return true;
        case XR_ERROR_OUT_OF_MEMORY:
            return true;
        case XR_ERROR_API_VERSION_UNSUPPORTED:
            return true;
        case XR_ERROR_INITIALIZATION_FAILED:
            return true;
        case XR_ERROR_FUNCTION_UNSUPPORTED:
            return true;
        case XR_ERROR_FEATURE_UNSUPPORTED:
            return true;
        case XR_ERROR_EXTENSION_NOT_PRESENT:
            return true;
        case XR_ERROR_LIMIT_REACHED:
            return true;
        case XR_ERROR_SIZE_INSUFFICIENT:
            return true;
        case XR_ERROR_HANDLE_INVALID:
            return true;
        case XR_ERROR_INSTANCE_LOST:
            return true;
        case XR_ERROR_SESSION_RUNNING:
            return true;
        case XR_ERROR_SESSION_NOT_RUNNING:
            return true;
        case XR_ERROR_SESSION_LOST:
            return true;
        case XR_ERROR_SYSTEM_INVALID:
            return true;
        case XR_ERROR_PATH_INVALID:
            return true;
        case XR_ERROR_PATH_COUNT_EXCEEDED:
            return true;
        case XR_ERROR_PATH_FORMAT_INVALID:
            return true;
        case XR_ERROR_PATH_UNSUPPORTED:
            return true;
        case XR_ERROR_LAYER_INVALID:
            return true;
        case XR_ERROR_LAYER_LIMIT_EXCEEDED:
            return true;
        case XR_ERROR_SWAPCHAIN_RECT_INVALID:
            return true;
        case XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED:
            return true;
        case XR_ERROR_ACTION_TYPE_MISMATCH:
            return true;
        case XR_ERROR_SESSION_NOT_READY:
            return true;
        case XR_ERROR_SESSION_NOT_STOPPING:
            return true;
        case XR_ERROR_TIME_INVALID:
            return true;
        case XR_ERROR_REFERENCE_SPACE_UNSUPPORTED:
            return true;
        case XR_ERROR_FILE_ACCESS_ERROR:
            return true;
        case XR_ERROR_FILE_CONTENTS_INVALID:
            return true;
        case XR_ERROR_FORM_FACTOR_UNSUPPORTED:
            return true;
        case XR_ERROR_FORM_FACTOR_UNAVAILABLE:
            return true;
        case XR_ERROR_API_LAYER_NOT_PRESENT:
            return true;
        case XR_ERROR_CALL_ORDER_INVALID:
            return true;
        case XR_ERROR_GRAPHICS_DEVICE_INVALID:
            return true;
        case XR_ERROR_POSE_INVALID:
            return true;
        case XR_ERROR_INDEX_OUT_OF_RANGE:
            return true;
        case XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED:
            return true;
        case XR_ERROR_ENVIRONMENT_BLEND_MODE_UNSUPPORTED:
            return true;
        case XR_ERROR_NAME_DUPLICATED:
            return true;
        case XR_ERROR_NAME_INVALID:
            return true;
        case XR_ERROR_ACTIONSET_NOT_ATTACHED:
            return true;
        case XR_ERROR_ACTIONSETS_ALREADY_ATTACHED:
            return true;
        case XR_ERROR_LOCALIZED_NAME_DUPLICATED:
            return true;
        case XR_ERROR_LOCALIZED_NAME_INVALID:
            return true;
        case XR_ERROR_ANDROID_THREAD_SETTINGS_ID_INVALID_KHR:
            // Enum value XR_ERROR_ANDROID_THREAD_SETTINGS_ID_INVALID_KHR requires extension XR_KHR_android_thread_settings, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_android_thread_settings")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrResult value \"XR_ERROR_ANDROID_THREAD_SETTINGS_ID_INVALID_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_android_thread_settings\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_ERROR_ANDROID_THREAD_SETTINGS_FAILURE_KHR:
            // Enum value XR_ERROR_ANDROID_THREAD_SETTINGS_FAILURE_KHR requires extension XR_KHR_android_thread_settings, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_android_thread_settings")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrResult value \"XR_ERROR_ANDROID_THREAD_SETTINGS_FAILURE_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_android_thread_settings\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
    default:
        return false;
}
}

// Function to validate XrStructureType enum
bool ValidateXrEnum(GenValidUsageXrInstanceInfo *instance_info,
                    const std::string &command_name,
                    const std::string &validation_name,
                    const std::string &item_name,
                    std::vector<GenValidUsageXrObjectInfo>& objects_info,
                    const XrStructureType value) {
    switch (value) {
        case XR_TYPE_UNKNOWN:
            return false; // Invalid XrStructureType 
        case XR_TYPE_API_LAYER_PROPERTIES:
            return true;
        case XR_TYPE_EXTENSION_PROPERTIES:
            return true;
        case XR_TYPE_INSTANCE_CREATE_INFO:
            return true;
        case XR_TYPE_SYSTEM_GET_INFO:
            return true;
        case XR_TYPE_SYSTEM_PROPERTIES:
            return true;
        case XR_TYPE_VIEW_LOCATE_INFO:
            return true;
        case XR_TYPE_VIEW:
            return true;
        case XR_TYPE_SESSION_CREATE_INFO:
            return true;
        case XR_TYPE_SWAPCHAIN_CREATE_INFO:
            return true;
        case XR_TYPE_SESSION_BEGIN_INFO:
            return true;
        case XR_TYPE_VIEW_STATE:
            return true;
        case XR_TYPE_FRAME_END_INFO:
            return true;
        case XR_TYPE_HAPTIC_VIBRATION:
            return true;
        case XR_TYPE_EVENT_DATA_BUFFER:
            return true;
        case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
            return true;
        case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
            return true;
        case XR_TYPE_ACTION_STATE_BOOLEAN:
            return true;
        case XR_TYPE_ACTION_STATE_FLOAT:
            return true;
        case XR_TYPE_ACTION_STATE_VECTOR2F:
            return true;
        case XR_TYPE_ACTION_STATE_POSE:
            return true;
        case XR_TYPE_ACTION_SET_CREATE_INFO:
            return true;
        case XR_TYPE_ACTION_CREATE_INFO:
            return true;
        case XR_TYPE_INSTANCE_PROPERTIES:
            return true;
        case XR_TYPE_FRAME_WAIT_INFO:
            return true;
        case XR_TYPE_COMPOSITION_LAYER_PROJECTION:
            return true;
        case XR_TYPE_COMPOSITION_LAYER_QUAD:
            return true;
        case XR_TYPE_REFERENCE_SPACE_CREATE_INFO:
            return true;
        case XR_TYPE_ACTION_SPACE_CREATE_INFO:
            return true;
        case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
            return true;
        case XR_TYPE_VIEW_CONFIGURATION_VIEW:
            return true;
        case XR_TYPE_SPACE_LOCATION:
            return true;
        case XR_TYPE_SPACE_VELOCITY:
            return true;
        case XR_TYPE_FRAME_STATE:
            return true;
        case XR_TYPE_VIEW_CONFIGURATION_PROPERTIES:
            return true;
        case XR_TYPE_FRAME_BEGIN_INFO:
            return true;
        case XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW:
            return true;
        case XR_TYPE_EVENT_DATA_EVENTS_LOST:
            return true;
        case XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING:
            return true;
        case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
            return true;
        case XR_TYPE_INTERACTION_PROFILE_STATE:
            return true;
        case XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO:
            return true;
        case XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO:
            return true;
        case XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO:
            return true;
        case XR_TYPE_ACTION_STATE_GET_INFO:
            return true;
        case XR_TYPE_HAPTIC_ACTION_INFO:
            return true;
        case XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO:
            return true;
        case XR_TYPE_ACTIONS_SYNC_INFO:
            return true;
        case XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO:
            return true;
        case XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO:
            return true;
        case XR_TYPE_COMPOSITION_LAYER_CUBE_KHR:
            // Enum value XR_TYPE_COMPOSITION_LAYER_CUBE_KHR requires extension XR_KHR_composition_layer_cube, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_composition_layer_cube")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_COMPOSITION_LAYER_CUBE_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_composition_layer_cube\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR:
            // Enum value XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR requires extension XR_KHR_android_create_instance, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_android_create_instance")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_android_create_instance\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR:
            // Enum value XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR requires extension XR_KHR_composition_layer_depth, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_composition_layer_depth")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_composition_layer_depth\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_VULKAN_SWAPCHAIN_FORMAT_LIST_CREATE_INFO_KHR:
            // Enum value XR_TYPE_VULKAN_SWAPCHAIN_FORMAT_LIST_CREATE_INFO_KHR requires extension XR_KHR_vulkan_swapchain_format_list, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_vulkan_swapchain_format_list")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_VULKAN_SWAPCHAIN_FORMAT_LIST_CREATE_INFO_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_vulkan_swapchain_format_list\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT:
            // Enum value XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT requires extension XR_EXT_performance_settings, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_EXT_performance_settings")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_EXT_performance_settings\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR:
            // Enum value XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR requires extension XR_KHR_composition_layer_cylinder, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_composition_layer_cylinder")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_composition_layer_cylinder\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_COMPOSITION_LAYER_EQUIRECT_KHR:
            // Enum value XR_TYPE_COMPOSITION_LAYER_EQUIRECT_KHR requires extension XR_KHR_composition_layer_equirect, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_composition_layer_equirect")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_COMPOSITION_LAYER_EQUIRECT_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_composition_layer_equirect\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT:
            // Enum value XR_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT requires extension XR_EXT_debug_utils, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_EXT_debug_utils")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_EXT_debug_utils\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT:
            // Enum value XR_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT requires extension XR_EXT_debug_utils, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_EXT_debug_utils")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_EXT_debug_utils\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT:
            // Enum value XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT requires extension XR_EXT_debug_utils, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_EXT_debug_utils")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_EXT_debug_utils\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_DEBUG_UTILS_LABEL_EXT:
            // Enum value XR_TYPE_DEBUG_UTILS_LABEL_EXT requires extension XR_EXT_debug_utils, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_EXT_debug_utils")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_DEBUG_UTILS_LABEL_EXT\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_EXT_debug_utils\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR:
            // Enum value XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR requires extension XR_KHR_opengl_enable, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_opengl_enable")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_opengl_enable\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR:
            // Enum value XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR requires extension XR_KHR_opengl_enable, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_opengl_enable")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_opengl_enable\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_GRAPHICS_BINDING_OPENGL_XCB_KHR:
            // Enum value XR_TYPE_GRAPHICS_BINDING_OPENGL_XCB_KHR requires extension XR_KHR_opengl_enable, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_opengl_enable")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_GRAPHICS_BINDING_OPENGL_XCB_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_opengl_enable\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR:
            // Enum value XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR requires extension XR_KHR_opengl_enable, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_opengl_enable")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_opengl_enable\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR:
            // Enum value XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR requires extension XR_KHR_opengl_enable, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_opengl_enable")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_opengl_enable\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR:
            // Enum value XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR requires extension XR_KHR_opengl_enable, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_opengl_enable")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_opengl_enable\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR:
            // Enum value XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR requires extension XR_KHR_opengl_es_enable, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_opengl_es_enable")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_opengl_es_enable\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR:
            // Enum value XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR requires extension XR_KHR_opengl_es_enable, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_opengl_es_enable")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_opengl_es_enable\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR:
            // Enum value XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR requires extension XR_KHR_opengl_es_enable, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_opengl_es_enable")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_opengl_es_enable\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR:
            // Enum value XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR requires extension XR_KHR_vulkan_enable, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_vulkan_enable")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_vulkan_enable\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR:
            // Enum value XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR requires extension XR_KHR_vulkan_enable, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_vulkan_enable")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_vulkan_enable\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR:
            // Enum value XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR requires extension XR_KHR_vulkan_enable, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_vulkan_enable")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_vulkan_enable\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_GRAPHICS_BINDING_D3D11_KHR:
            // Enum value XR_TYPE_GRAPHICS_BINDING_D3D11_KHR requires extension XR_KHR_D3D11_enable, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_D3D11_enable")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_GRAPHICS_BINDING_D3D11_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_D3D11_enable\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR:
            // Enum value XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR requires extension XR_KHR_D3D11_enable, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_D3D11_enable")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_D3D11_enable\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR:
            // Enum value XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR requires extension XR_KHR_D3D11_enable, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_D3D11_enable")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_D3D11_enable\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_GRAPHICS_BINDING_D3D12_KHR:
            // Enum value XR_TYPE_GRAPHICS_BINDING_D3D12_KHR requires extension XR_KHR_D3D12_enable, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_D3D12_enable")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_GRAPHICS_BINDING_D3D12_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_D3D12_enable\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR:
            // Enum value XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR requires extension XR_KHR_D3D12_enable, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_D3D12_enable")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_D3D12_enable\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR:
            // Enum value XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR requires extension XR_KHR_D3D12_enable, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_D3D12_enable")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_D3D12_enable\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_VISIBILITY_MASK_KHR:
            // Enum value XR_TYPE_VISIBILITY_MASK_KHR requires extension XR_KHR_visibility_mask, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_visibility_mask")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_VISIBILITY_MASK_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_visibility_mask\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
        case XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR:
            // Enum value XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR requires extension XR_KHR_visibility_mask, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_visibility_mask")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrStructureType value \"XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_KHR_visibility_mask\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
    default:
        return false;
}
}

// Function to validate XrFormFactor enum
bool ValidateXrEnum(GenValidUsageXrInstanceInfo *instance_info,
                    const std::string &command_name,
                    const std::string &validation_name,
                    const std::string &item_name,
                    std::vector<GenValidUsageXrObjectInfo>& objects_info,
                    const XrFormFactor value) {
    switch (value) {
        case XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY:
            return true;
        case XR_FORM_FACTOR_HANDHELD_DISPLAY:
            return true;
    default:
        return false;
}
}

// Function to validate XrViewConfigurationType enum
bool ValidateXrEnum(GenValidUsageXrInstanceInfo *instance_info,
                    const std::string &command_name,
                    const std::string &validation_name,
                    const std::string &item_name,
                    std::vector<GenValidUsageXrObjectInfo>& objects_info,
                    const XrViewConfigurationType value) {
    switch (value) {
        case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO:
            return true;
        case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO:
            return true;
        case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO:
            // Enum value XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO requires extension XR_VARJO_quad_views, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_VARJO_quad_views")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrViewConfigurationType value \"XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_VARJO_quad_views\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
    default:
        return false;
}
}

// Function to validate XrEnvironmentBlendMode enum
bool ValidateXrEnum(GenValidUsageXrInstanceInfo *instance_info,
                    const std::string &command_name,
                    const std::string &validation_name,
                    const std::string &item_name,
                    std::vector<GenValidUsageXrObjectInfo>& objects_info,
                    const XrEnvironmentBlendMode value) {
    switch (value) {
        case XR_ENVIRONMENT_BLEND_MODE_OPAQUE:
            return true;
        case XR_ENVIRONMENT_BLEND_MODE_ADDITIVE:
            return true;
        case XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND:
            return true;
    default:
        return false;
}
}

// Function to validate XrReferenceSpaceType enum
bool ValidateXrEnum(GenValidUsageXrInstanceInfo *instance_info,
                    const std::string &command_name,
                    const std::string &validation_name,
                    const std::string &item_name,
                    std::vector<GenValidUsageXrObjectInfo>& objects_info,
                    const XrReferenceSpaceType value) {
    switch (value) {
        case XR_REFERENCE_SPACE_TYPE_VIEW:
            return true;
        case XR_REFERENCE_SPACE_TYPE_LOCAL:
            return true;
        case XR_REFERENCE_SPACE_TYPE_STAGE:
            return true;
    default:
        return false;
}
}

// Function to validate XrActionType enum
bool ValidateXrEnum(GenValidUsageXrInstanceInfo *instance_info,
                    const std::string &command_name,
                    const std::string &validation_name,
                    const std::string &item_name,
                    std::vector<GenValidUsageXrObjectInfo>& objects_info,
                    const XrActionType value) {
    switch (value) {
        case XR_ACTION_TYPE_BOOLEAN_INPUT:
            return true;
        case XR_ACTION_TYPE_FLOAT_INPUT:
            return true;
        case XR_ACTION_TYPE_VECTOR2F_INPUT:
            return true;
        case XR_ACTION_TYPE_POSE_INPUT:
            return true;
        case XR_ACTION_TYPE_VIBRATION_OUTPUT:
            return true;
    default:
        return false;
}
}

// Function to validate XrEyeVisibility enum
bool ValidateXrEnum(GenValidUsageXrInstanceInfo *instance_info,
                    const std::string &command_name,
                    const std::string &validation_name,
                    const std::string &item_name,
                    std::vector<GenValidUsageXrObjectInfo>& objects_info,
                    const XrEyeVisibility value) {
    switch (value) {
        case XR_EYE_VISIBILITY_BOTH:
            return true;
        case XR_EYE_VISIBILITY_LEFT:
            return true;
        case XR_EYE_VISIBILITY_RIGHT:
            return true;
    default:
        return false;
}
}

// Function to validate XrSessionState enum
bool ValidateXrEnum(GenValidUsageXrInstanceInfo *instance_info,
                    const std::string &command_name,
                    const std::string &validation_name,
                    const std::string &item_name,
                    std::vector<GenValidUsageXrObjectInfo>& objects_info,
                    const XrSessionState value) {
    switch (value) {
        case XR_SESSION_STATE_UNKNOWN:
            return true;
        case XR_SESSION_STATE_IDLE:
            return true;
        case XR_SESSION_STATE_READY:
            return true;
        case XR_SESSION_STATE_SYNCHRONIZED:
            return true;
        case XR_SESSION_STATE_VISIBLE:
            return true;
        case XR_SESSION_STATE_FOCUSED:
            return true;
        case XR_SESSION_STATE_STOPPING:
            return true;
        case XR_SESSION_STATE_LOSS_PENDING:
            return true;
        case XR_SESSION_STATE_EXITING:
            return true;
    default:
        return false;
}
}

// Function to validate XrObjectType enum
bool ValidateXrEnum(GenValidUsageXrInstanceInfo *instance_info,
                    const std::string &command_name,
                    const std::string &validation_name,
                    const std::string &item_name,
                    std::vector<GenValidUsageXrObjectInfo>& objects_info,
                    const XrObjectType value) {
    switch (value) {
        case XR_OBJECT_TYPE_UNKNOWN:
            return true;
        case XR_OBJECT_TYPE_INSTANCE:
            return true;
        case XR_OBJECT_TYPE_SESSION:
            return true;
        case XR_OBJECT_TYPE_SWAPCHAIN:
            return true;
        case XR_OBJECT_TYPE_SPACE:
            return true;
        case XR_OBJECT_TYPE_ACTION_SET:
            return true;
        case XR_OBJECT_TYPE_ACTION:
            return true;
        case XR_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT:
            // Enum value XR_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT requires extension XR_EXT_debug_utils, so check that it is enabled
            if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_EXT_debug_utils")) {
                std::string vuid = "VUID-";
                vuid += validation_name;
                vuid += "-";
                vuid += item_name;
                vuid += "-parameter";
                std::string error_str = "XrObjectType value \"XR_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT\"";
                error_str += " being used, which requires extension ";
                error_str += " \"XR_EXT_debug_utils\" to be enabled, but it is not enabled";
                CoreValidLogMessage(instance_info, vuid,
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, error_str);
                return false;
            }
            return true;
    default:
        return false;
}
}

#if defined(XR_USE_PLATFORM_ANDROID)
// Function to validate XrAndroidThreadTypeKHR enum
bool ValidateXrEnum(GenValidUsageXrInstanceInfo *instance_info,
                    const std::string &command_name,
                    const std::string &validation_name,
                    const std::string &item_name,
                    std::vector<GenValidUsageXrObjectInfo>& objects_info,
                    const XrAndroidThreadTypeKHR value) {
    // Enum requires extension XR_KHR_android_thread_settings, so check that it is enabled
    if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_android_thread_settings")) {
        std::string vuid = "VUID-";
        vuid += validation_name;
        vuid += "-";
        vuid += item_name;
        vuid += "-parameter";
        std::string error_str = "XrAndroidThreadTypeKHR requires extension ";
        error_str += " \"XR_KHR_android_thread_settings\" to be enabled, but it is not enabled";
        CoreValidLogMessage(instance_info, vuid,
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, error_str);
        return false;
    }
    switch (value) {
        case XR_ANDROID_THREAD_TYPE_APPLICATION_MAIN_KHR:
            return true;
        case XR_ANDROID_THREAD_TYPE_APPLICATION_WORKER_KHR:
            return true;
        case XR_ANDROID_THREAD_TYPE_RENDERER_MAIN_KHR:
            return true;
        case XR_ANDROID_THREAD_TYPE_RENDERER_WORKER_KHR:
            return true;
    default:
        return false;
}
}

#endif // defined(XR_USE_PLATFORM_ANDROID)
// Function to validate XrVisibilityMaskTypeKHR enum
bool ValidateXrEnum(GenValidUsageXrInstanceInfo *instance_info,
                    const std::string &command_name,
                    const std::string &validation_name,
                    const std::string &item_name,
                    std::vector<GenValidUsageXrObjectInfo>& objects_info,
                    const XrVisibilityMaskTypeKHR value) {
    // Enum requires extension XR_KHR_visibility_mask, so check that it is enabled
    if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_visibility_mask")) {
        std::string vuid = "VUID-";
        vuid += validation_name;
        vuid += "-";
        vuid += item_name;
        vuid += "-parameter";
        std::string error_str = "XrVisibilityMaskTypeKHR requires extension ";
        error_str += " \"XR_KHR_visibility_mask\" to be enabled, but it is not enabled";
        CoreValidLogMessage(instance_info, vuid,
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, error_str);
        return false;
    }
    switch (value) {
        case XR_VISIBILITY_MASK_TYPE_HIDDEN_TRIANGLE_MESH_KHR:
            return true;
        case XR_VISIBILITY_MASK_TYPE_VISIBLE_TRIANGLE_MESH_KHR:
            return true;
        case XR_VISIBILITY_MASK_TYPE_LINE_LOOP_KHR:
            return true;
    default:
        return false;
}
}

// Function to validate XrPerfSettingsDomainEXT enum
bool ValidateXrEnum(GenValidUsageXrInstanceInfo *instance_info,
                    const std::string &command_name,
                    const std::string &validation_name,
                    const std::string &item_name,
                    std::vector<GenValidUsageXrObjectInfo>& objects_info,
                    const XrPerfSettingsDomainEXT value) {
    // Enum requires extension XR_EXT_performance_settings, so check that it is enabled
    if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_EXT_performance_settings")) {
        std::string vuid = "VUID-";
        vuid += validation_name;
        vuid += "-";
        vuid += item_name;
        vuid += "-parameter";
        std::string error_str = "XrPerfSettingsDomainEXT requires extension ";
        error_str += " \"XR_EXT_performance_settings\" to be enabled, but it is not enabled";
        CoreValidLogMessage(instance_info, vuid,
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, error_str);
        return false;
    }
    switch (value) {
        case XR_PERF_SETTINGS_DOMAIN_CPU_EXT:
            return true;
        case XR_PERF_SETTINGS_DOMAIN_GPU_EXT:
            return true;
    default:
        return false;
}
}

// Function to validate XrPerfSettingsSubDomainEXT enum
bool ValidateXrEnum(GenValidUsageXrInstanceInfo *instance_info,
                    const std::string &command_name,
                    const std::string &validation_name,
                    const std::string &item_name,
                    std::vector<GenValidUsageXrObjectInfo>& objects_info,
                    const XrPerfSettingsSubDomainEXT value) {
    // Enum requires extension XR_EXT_performance_settings, so check that it is enabled
    if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_EXT_performance_settings")) {
        std::string vuid = "VUID-";
        vuid += validation_name;
        vuid += "-";
        vuid += item_name;
        vuid += "-parameter";
        std::string error_str = "XrPerfSettingsSubDomainEXT requires extension ";
        error_str += " \"XR_EXT_performance_settings\" to be enabled, but it is not enabled";
        CoreValidLogMessage(instance_info, vuid,
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, error_str);
        return false;
    }
    switch (value) {
        case XR_PERF_SETTINGS_SUB_DOMAIN_COMPOSITING_EXT:
            return true;
        case XR_PERF_SETTINGS_SUB_DOMAIN_RENDERING_EXT:
            return true;
        case XR_PERF_SETTINGS_SUB_DOMAIN_THERMAL_EXT:
            return true;
    default:
        return false;
}
}

// Function to validate XrPerfSettingsLevelEXT enum
bool ValidateXrEnum(GenValidUsageXrInstanceInfo *instance_info,
                    const std::string &command_name,
                    const std::string &validation_name,
                    const std::string &item_name,
                    std::vector<GenValidUsageXrObjectInfo>& objects_info,
                    const XrPerfSettingsLevelEXT value) {
    // Enum requires extension XR_EXT_performance_settings, so check that it is enabled
    if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_EXT_performance_settings")) {
        std::string vuid = "VUID-";
        vuid += validation_name;
        vuid += "-";
        vuid += item_name;
        vuid += "-parameter";
        std::string error_str = "XrPerfSettingsLevelEXT requires extension ";
        error_str += " \"XR_EXT_performance_settings\" to be enabled, but it is not enabled";
        CoreValidLogMessage(instance_info, vuid,
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, error_str);
        return false;
    }
    switch (value) {
        case XR_PERF_SETTINGS_LEVEL_POWER_SAVINGS_EXT:
            return true;
        case XR_PERF_SETTINGS_LEVEL_SUSTAINED_LOW_EXT:
            return true;
        case XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT:
            return true;
        case XR_PERF_SETTINGS_LEVEL_BOOST_EXT:
            return true;
    default:
        return false;
}
}

// Function to validate XrPerfSettingsNotificationLevelEXT enum
bool ValidateXrEnum(GenValidUsageXrInstanceInfo *instance_info,
                    const std::string &command_name,
                    const std::string &validation_name,
                    const std::string &item_name,
                    std::vector<GenValidUsageXrObjectInfo>& objects_info,
                    const XrPerfSettingsNotificationLevelEXT value) {
    // Enum requires extension XR_EXT_performance_settings, so check that it is enabled
    if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_EXT_performance_settings")) {
        std::string vuid = "VUID-";
        vuid += validation_name;
        vuid += "-";
        vuid += item_name;
        vuid += "-parameter";
        std::string error_str = "XrPerfSettingsNotificationLevelEXT requires extension ";
        error_str += " \"XR_EXT_performance_settings\" to be enabled, but it is not enabled";
        CoreValidLogMessage(instance_info, vuid,
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, error_str);
        return false;
    }
    switch (value) {
        case XR_PERF_SETTINGS_NOTIF_LEVEL_NORMAL_EXT:
            return true;
        case XR_PERF_SETTINGS_NOTIF_LEVEL_WARNING_EXT:
            return true;
        case XR_PERF_SETTINGS_NOTIF_LEVEL_IMPAIRED_EXT:
            return true;
    default:
        return false;
}
}

bool ExtensionEnabled(std::vector<std::string> &extensions, const char* const check_extension_name) {
    for (auto enabled_extension: extensions) {
        if (enabled_extension == check_extension_name) {
            return true;
        }
    }
    return false;
}

bool ValidateInstanceExtensionDependencies(GenValidUsageXrInstanceInfo *gen_instance_info,
                                           const std::string &command,
                                           const std::string &struct_name,
                                           std::vector<GenValidUsageXrObjectInfo>& objects_info,
                                           std::vector<std::string> &extensions) {
    for (uint32_t cur_index = 0; cur_index < extensions.size(); ++cur_index) {
        if (extensions[cur_index] == "XR_KHR_vulkan_swapchain_format_list") {
            for (uint32_t check_index = 0; check_index < extensions.size(); ++check_index) {
                if (cur_index == check_index) {
                    continue;
                }
                if (!ExtensionEnabled(extensions, "XR_KHR_vulkan_enable")) {
                    if (nullptr != gen_instance_info) {
                        std::string vuid = "VUID-";
                        vuid += command;
                        vuid += "-";
                        vuid += struct_name;
                        vuid += "-parameter";
                        CoreValidLogMessage(gen_instance_info, vuid, VALID_USAGE_DEBUG_SEVERITY_ERROR,
                                            command, objects_info,
                                            "Missing extension dependency \"XR_KHR_vulkan_enable\" (required by extension" \
                                            "\"XR_KHR_vulkan_swapchain_format_list\") from enabled extension list");
                    }
                    return false;
                }
            }
        }
    }
    return true;
}

bool ValidateSystemExtensionDependencies(GenValidUsageXrInstanceInfo *gen_instance_info,
                                         const std::string &command,
                                         const std::string &struct_name,
                                         std::vector<GenValidUsageXrObjectInfo>& objects_info,
                                         std::vector<std::string> &extensions) {
    // No system extensions to check dependencies for
    return true;
}

ValidateXrHandleResult VerifyXrInstanceHandle(const XrInstance* handle_to_check) {
    return g_instance_info.verifyHandle(handle_to_check);
}

ValidateXrHandleResult VerifyXrSessionHandle(const XrSession* handle_to_check) {
    return g_session_info.verifyHandle(handle_to_check);
}

ValidateXrHandleResult VerifyXrSpaceHandle(const XrSpace* handle_to_check) {
    return g_space_info.verifyHandle(handle_to_check);
}

ValidateXrHandleResult VerifyXrActionHandle(const XrAction* handle_to_check) {
    return g_action_info.verifyHandle(handle_to_check);
}

ValidateXrHandleResult VerifyXrSwapchainHandle(const XrSwapchain* handle_to_check) {
    return g_swapchain_info.verifyHandle(handle_to_check);
}

ValidateXrHandleResult VerifyXrActionSetHandle(const XrActionSet* handle_to_check) {
    return g_actionset_info.verifyHandle(handle_to_check);
}

ValidateXrHandleResult VerifyXrDebugUtilsMessengerEXTHandle(const XrDebugUtilsMessengerEXT* handle_to_check) {
    return g_debugutilsmessengerext_info.verifyHandle(handle_to_check);
}

// Implementation function to get parent handle information
bool GetXrParent(const XrObjectType inhandle_type, const uint64_t inhandle,
                 XrObjectType& outhandle_type, uint64_t& outhandle) {
    if (inhandle_type == XR_OBJECT_TYPE_INSTANCE) {
        return false;
    }
    if (inhandle_type == XR_OBJECT_TYPE_SESSION) {
        // Get the object and parent of the handle
        GenValidUsageXrHandleInfo *handle_info = g_session_info.get(TreatIntegerAsHandle<XrSession>(inhandle));
        outhandle_type = handle_info->direct_parent_type;
        outhandle = handle_info->direct_parent_handle;
        return true;
    }
    if (inhandle_type == XR_OBJECT_TYPE_SPACE) {
        // Get the object and parent of the handle
        GenValidUsageXrHandleInfo *handle_info = g_space_info.get(TreatIntegerAsHandle<XrSpace>(inhandle));
        outhandle_type = handle_info->direct_parent_type;
        outhandle = handle_info->direct_parent_handle;
        return true;
    }
    if (inhandle_type == XR_OBJECT_TYPE_ACTION) {
        // Get the object and parent of the handle
        GenValidUsageXrHandleInfo *handle_info = g_action_info.get(TreatIntegerAsHandle<XrAction>(inhandle));
        outhandle_type = handle_info->direct_parent_type;
        outhandle = handle_info->direct_parent_handle;
        return true;
    }
    if (inhandle_type == XR_OBJECT_TYPE_SWAPCHAIN) {
        // Get the object and parent of the handle
        GenValidUsageXrHandleInfo *handle_info = g_swapchain_info.get(TreatIntegerAsHandle<XrSwapchain>(inhandle));
        outhandle_type = handle_info->direct_parent_type;
        outhandle = handle_info->direct_parent_handle;
        return true;
    }
    if (inhandle_type == XR_OBJECT_TYPE_ACTION_SET) {
        // Get the object and parent of the handle
        GenValidUsageXrHandleInfo *handle_info = g_actionset_info.get(TreatIntegerAsHandle<XrActionSet>(inhandle));
        outhandle_type = handle_info->direct_parent_type;
        outhandle = handle_info->direct_parent_handle;
        return true;
    }
    if (inhandle_type == XR_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT) {
        // Get the object and parent of the handle
        GenValidUsageXrHandleInfo *handle_info = g_debugutilsmessengerext_info.get(TreatIntegerAsHandle<XrDebugUtilsMessengerEXT>(inhandle));
        outhandle_type = handle_info->direct_parent_type;
        outhandle = handle_info->direct_parent_handle;
        return true;
    }
    return false;
}

// Implementation of VerifyXrParent function
bool VerifyXrParent(XrObjectType handle1_type, const uint64_t handle1,
                    XrObjectType handle2_type, const uint64_t handle2,
                    bool check_this) {
    if (IsIntegerNullHandle(handle1) || IsIntegerNullHandle(handle2)) {
        return false;
    } else if (check_this && handle1_type == handle2_type) {
        return (handle1 == handle2);
    }
    if (handle1_type == XR_OBJECT_TYPE_INSTANCE && handle2_type != XR_OBJECT_TYPE_INSTANCE) {
        XrObjectType parent_type;
        uint64_t parent_handle;
        if (!GetXrParent(handle2_type, handle2, parent_type, parent_handle)) {
            return false;
        }
        return VerifyXrParent(handle1_type, handle1, parent_type, parent_handle, true);
    } else if (handle2_type == XR_OBJECT_TYPE_INSTANCE && handle1_type != XR_OBJECT_TYPE_INSTANCE) {
        XrObjectType parent_type;
        uint64_t parent_handle;
        if (!GetXrParent(handle1_type, handle1, parent_type, parent_handle)) {
            return false;
        }
        return VerifyXrParent(parent_type, parent_handle, handle2_type, handle2, true);
    } else {
        XrObjectType parent1_type;
        uint64_t parent1_handle;
                XrObjectType parent2_type;
        uint64_t parent2_handle;
        if (!GetXrParent(handle1_type, handle1, parent1_type, parent1_handle)) {
            return false;
        }
        if (!GetXrParent(handle2_type, handle2, parent2_type, parent2_handle)) {
            return false;
        }
        if (parent1_type == handle2_type) {
            return (parent1_handle == handle2);
        } else if (handle1_type == parent2_type) {
            return (handle1 == parent2_handle);
        } else {
            return VerifyXrParent(parent1_type, parent1_handle, parent2_type, parent2_handle, true);
        }
    }
    return false;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrApiLayerProperties* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_API_LAYER_PROPERTIES) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrApiLayerProperties",
                             value->type, "VUID-XrApiLayerProperties-type-type", XR_TYPE_API_LAYER_PROPERTIES, "XR_TYPE_API_LAYER_PROPERTIES");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrApiLayerProperties-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrApiLayerProperties struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrApiLayerProperties : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrApiLayerProperties-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrApiLayerProperties struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    if (XR_MAX_API_LAYER_NAME_SIZE < std::strlen(value->layerName)) {
        CoreValidLogMessage(instance_info, "VUID-XrApiLayerProperties-layerName-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrApiLayerProperties member layerName length is too long.");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    if (XR_MAX_API_LAYER_DESCRIPTION_SIZE < std::strlen(value->description)) {
        CoreValidLogMessage(instance_info, "VUID-XrApiLayerProperties-description-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrApiLayerProperties member description length is too long.");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrExtensionProperties* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_EXTENSION_PROPERTIES) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrExtensionProperties",
                             value->type, "VUID-XrExtensionProperties-type-type", XR_TYPE_EXTENSION_PROPERTIES, "XR_TYPE_EXTENSION_PROPERTIES");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrExtensionProperties-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrExtensionProperties struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrExtensionProperties : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrExtensionProperties-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrExtensionProperties struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    if (XR_MAX_EXTENSION_NAME_SIZE < std::strlen(value->extensionName)) {
        CoreValidLogMessage(instance_info, "VUID-XrExtensionProperties-extensionName-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrExtensionProperties member extensionName length is too long.");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrApplicationInfo* value) {
    XrResult xr_result = XR_SUCCESS;
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    if (XR_MAX_APPLICATION_NAME_SIZE < std::strlen(value->applicationName)) {
        CoreValidLogMessage(instance_info, "VUID-XrApplicationInfo-applicationName-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrApplicationInfo member applicationName length is too long.");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    if (XR_MAX_ENGINE_NAME_SIZE < std::strlen(value->engineName)) {
        CoreValidLogMessage(instance_info, "VUID-XrApplicationInfo-engineName-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrApplicationInfo member engineName length is too long.");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrInstanceCreateInfo* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_INSTANCE_CREATE_INFO) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrInstanceCreateInfo",
                             value->type, "VUID-XrInstanceCreateInfo-type-type", XR_TYPE_INSTANCE_CREATE_INFO, "XR_TYPE_INSTANCE_CREATE_INFO");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    valid_ext_structs.push_back(XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);
    valid_ext_structs.push_back(XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR);
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrInstanceCreateInfo-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrInstanceCreateInfo struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrInstanceCreateInfo : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrInstanceCreateInfo-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrInstanceCreateInfo struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    ValidateXrFlagsResult instance_create_flags_result = ValidateXrInstanceCreateFlags(value->createFlags);
    // Valid flags available, so it must be invalid to fail.
    if (VALIDATE_XR_FLAGS_INVALID == instance_create_flags_result) {
        // Otherwise, flags must be valid.
        std::ostringstream oss_enum;
        oss_enum << "XrInstanceCreateInfo invalid member XrInstanceCreateFlags \"createFlags\" flag value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->createFlags));
        oss_enum <<" contains illegal bit";
        CoreValidLogMessage(instance_info, "VUID-XrInstanceCreateInfo-createFlags-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Validate that the structure XrApplicationInfo is valid
    xr_result = ValidateXrStruct(instance_info, command_name, objects_info,
                                                    check_members, &value->applicationInfo);
    if (XR_SUCCESS != xr_result) {
        CoreValidLogMessage(instance_info, "VUID-XrInstanceCreateInfo-applicationInfo-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrInstanceCreateInfo member applicationInfo is invalid");
        return xr_result;
    }
    // Pointer/array variable with a length variable.  Make sure that
    // if length variable is non-zero that the pointer is not NULL
    if (nullptr == value->enabledApiLayerNames && 0 != value->enabledApiLayerCount) {
        CoreValidLogMessage(instance_info, "VUID-XrInstanceCreateInfo-enabledApiLayerNames-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name, objects_info,
                            "XrInstanceCreateInfo contains invalid NULL for char \"enabledApiLayerNames\" is which not "
                            "optional since \"enabledApiLayerCount\" is set and must be non-NULL");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // NOTE: Can't validate "VUID-XrInstanceCreateInfo-enabledApiLayerNames-parameter" null-termination
    // Optional array must be non-NULL when value->enabledExtensionCount is non-zero
    if (0 != value->enabledExtensionCount && nullptr == value->enabledExtensionNames) {
        CoreValidLogMessage(instance_info, "VUID-XrInstanceCreateInfo-enabledExtensionNames-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrInstanceCreateInfo member enabledExtensionCount is NULL, but value->enabledExtensionCount is greater than 0");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // Pointer/array variable with a length variable.  Make sure that
    // if length variable is non-zero that the pointer is not NULL
    if (nullptr == value->enabledExtensionNames && 0 != value->enabledExtensionCount) {
        CoreValidLogMessage(instance_info, "VUID-XrInstanceCreateInfo-enabledExtensionNames-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name, objects_info,
                            "XrInstanceCreateInfo contains invalid NULL for char \"enabledExtensionNames\" is which not "
                            "optional since \"enabledExtensionCount\" is set and must be non-NULL");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // NOTE: Can't validate "VUID-XrInstanceCreateInfo-enabledExtensionNames-parameter" null-termination
    std::vector<std::string> enabled_extension_vec;
    for (uint32_t extension = 0; extension < value->enabledExtensionCount; ++extension) {
        enabled_extension_vec.push_back(value->enabledExtensionNames[extension]);
    }
    if (!ValidateInstanceExtensionDependencies(nullptr, command_name, "XrInstanceCreateInfo",
                                               objects_info, enabled_extension_vec)) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrInstanceProperties* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_INSTANCE_PROPERTIES) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrInstanceProperties",
                             value->type, "VUID-XrInstanceProperties-type-type", XR_TYPE_INSTANCE_PROPERTIES, "XR_TYPE_INSTANCE_PROPERTIES");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrInstanceProperties-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrInstanceProperties struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrInstanceProperties : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrInstanceProperties-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrInstanceProperties struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    if (XR_MAX_RUNTIME_NAME_SIZE < std::strlen(value->runtimeName)) {
        CoreValidLogMessage(instance_info, "VUID-XrInstanceProperties-runtimeName-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrInstanceProperties member runtimeName length is too long.");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrEventDataBuffer* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_EVENT_DATA_BUFFER) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrEventDataBuffer",
                             value->type, "VUID-XrEventDataBuffer-type-type", XR_TYPE_EVENT_DATA_BUFFER, "XR_TYPE_EVENT_DATA_BUFFER");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrEventDataBuffer-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrEventDataBuffer struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrEventDataBuffer : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrEventDataBuffer-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrEventDataBuffer struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSystemGetInfo* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_SYSTEM_GET_INFO) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrSystemGetInfo",
                             value->type, "VUID-XrSystemGetInfo-type-type", XR_TYPE_SYSTEM_GET_INFO, "XR_TYPE_SYSTEM_GET_INFO");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrSystemGetInfo-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrSystemGetInfo struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrSystemGetInfo : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrSystemGetInfo-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrSystemGetInfo struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Make sure the enum type XrFormFactor value is valid
    if (!ValidateXrEnum(instance_info, command_name, "XrSystemGetInfo", "formFactor", objects_info, value->formFactor)) {
        std::ostringstream oss_enum;
        oss_enum << "XrSystemGetInfo contains invalid XrFormFactor \"formFactor\" enum value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->formFactor));
        CoreValidLogMessage(instance_info, "VUID-XrSystemGetInfo-formFactor-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSystemGraphicsProperties* value) {
    XrResult xr_result = XR_SUCCESS;
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSystemTrackingProperties* value) {
    XrResult xr_result = XR_SUCCESS;
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSystemProperties* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_SYSTEM_PROPERTIES) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrSystemProperties",
                             value->type, "VUID-XrSystemProperties-type-type", XR_TYPE_SYSTEM_PROPERTIES, "XR_TYPE_SYSTEM_PROPERTIES");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrSystemProperties-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrSystemProperties struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrSystemProperties : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrSystemProperties-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrSystemProperties struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    if (XR_MAX_SYSTEM_NAME_SIZE < std::strlen(value->systemName)) {
        CoreValidLogMessage(instance_info, "VUID-XrSystemProperties-systemName-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrSystemProperties member systemName length is too long.");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Validate that the structure XrSystemTrackingProperties is valid
    xr_result = ValidateXrStruct(instance_info, command_name, objects_info,
                                                    check_members, &value->trackingProperties);
    if (XR_SUCCESS != xr_result) {
        CoreValidLogMessage(instance_info, "VUID-XrSystemProperties-trackingProperties-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrSystemProperties member trackingProperties is invalid");
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSessionCreateInfo* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_SESSION_CREATE_INFO) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrSessionCreateInfo",
                             value->type, "VUID-XrSessionCreateInfo-type-type", XR_TYPE_SESSION_CREATE_INFO, "XR_TYPE_SESSION_CREATE_INFO");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    valid_ext_structs.push_back(XR_TYPE_GRAPHICS_BINDING_D3D11_KHR);
    valid_ext_structs.push_back(XR_TYPE_GRAPHICS_BINDING_D3D12_KHR);
    valid_ext_structs.push_back(XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR);
    valid_ext_structs.push_back(XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR);
    valid_ext_structs.push_back(XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR);
    valid_ext_structs.push_back(XR_TYPE_GRAPHICS_BINDING_OPENGL_XCB_KHR);
    valid_ext_structs.push_back(XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR);
    valid_ext_structs.push_back(XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR);
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrSessionCreateInfo-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrSessionCreateInfo struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrSessionCreateInfo : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrSessionCreateInfo-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrSessionCreateInfo struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    ValidateXrFlagsResult session_create_flags_result = ValidateXrSessionCreateFlags(value->createFlags);
    // Valid flags available, so it must be invalid to fail.
    if (VALIDATE_XR_FLAGS_INVALID == session_create_flags_result) {
        // Otherwise, flags must be valid.
        std::ostringstream oss_enum;
        oss_enum << "XrSessionCreateInfo invalid member XrSessionCreateFlags \"createFlags\" flag value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->createFlags));
        oss_enum <<" contains illegal bit";
        CoreValidLogMessage(instance_info, "VUID-XrSessionCreateInfo-createFlags-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrVector3f* value) {
    XrResult xr_result = XR_SUCCESS;
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSpaceVelocity* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_SPACE_VELOCITY) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrSpaceVelocity",
                             value->type, "VUID-XrSpaceVelocity-type-type", XR_TYPE_SPACE_VELOCITY, "XR_TYPE_SPACE_VELOCITY");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrSpaceVelocity-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrSpaceVelocity struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrSpaceVelocity : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrSpaceVelocity-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrSpaceVelocity struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    ValidateXrFlagsResult space_velocity_flags_result = ValidateXrSpaceVelocityFlags(value->velocityFlags);
    // Valid flags available, so it must be invalid to fail.
    if (VALIDATE_XR_FLAGS_INVALID == space_velocity_flags_result) {
        // Otherwise, flags must be valid.
        std::ostringstream oss_enum;
        oss_enum << "XrSpaceVelocity invalid member XrSpaceVelocityFlags \"velocityFlags\" flag value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->velocityFlags));
        oss_enum <<" contains illegal bit";
        CoreValidLogMessage(instance_info, "VUID-XrSpaceVelocity-velocityFlags-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrQuaternionf* value) {
    XrResult xr_result = XR_SUCCESS;
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrPosef* value) {
    XrResult xr_result = XR_SUCCESS;
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrReferenceSpaceCreateInfo* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_REFERENCE_SPACE_CREATE_INFO) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrReferenceSpaceCreateInfo",
                             value->type, "VUID-XrReferenceSpaceCreateInfo-type-type", XR_TYPE_REFERENCE_SPACE_CREATE_INFO, "XR_TYPE_REFERENCE_SPACE_CREATE_INFO");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrReferenceSpaceCreateInfo-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrReferenceSpaceCreateInfo struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrReferenceSpaceCreateInfo : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrReferenceSpaceCreateInfo-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrReferenceSpaceCreateInfo struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Make sure the enum type XrReferenceSpaceType value is valid
    if (!ValidateXrEnum(instance_info, command_name, "XrReferenceSpaceCreateInfo", "referenceSpaceType", objects_info, value->referenceSpaceType)) {
        std::ostringstream oss_enum;
        oss_enum << "XrReferenceSpaceCreateInfo contains invalid XrReferenceSpaceType \"referenceSpaceType\" enum value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->referenceSpaceType));
        CoreValidLogMessage(instance_info, "VUID-XrReferenceSpaceCreateInfo-referenceSpaceType-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrExtent2Df* value) {
    XrResult xr_result = XR_SUCCESS;
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrActionSpaceCreateInfo* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_ACTION_SPACE_CREATE_INFO) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrActionSpaceCreateInfo",
                             value->type, "VUID-XrActionSpaceCreateInfo-type-type", XR_TYPE_ACTION_SPACE_CREATE_INFO, "XR_TYPE_ACTION_SPACE_CREATE_INFO");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrActionSpaceCreateInfo-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrActionSpaceCreateInfo struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrActionSpaceCreateInfo : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrActionSpaceCreateInfo-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrActionSpaceCreateInfo struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    {
        // writeValidateInlineHandleValidation
        ValidateXrHandleResult handle_result = VerifyXrActionHandle(&value->action);
        if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
            // Not a valid handle or NULL (which is not valid in this case)
            std::ostringstream oss;
            oss << "Invalid XrAction handle \"action\" ";
            oss << HandleToHexString(value->action);
            CoreValidLogMessage(instance_info, "VUID-XrActionSpaceCreateInfo-action-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, oss.str());
            return XR_ERROR_HANDLE_INVALID;
        }
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSpaceLocation* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_SPACE_LOCATION) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrSpaceLocation",
                             value->type, "VUID-XrSpaceLocation-type-type", XR_TYPE_SPACE_LOCATION, "XR_TYPE_SPACE_LOCATION");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    valid_ext_structs.push_back(XR_TYPE_SPACE_VELOCITY);
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrSpaceLocation-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrSpaceLocation struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrSpaceLocation : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrSpaceLocation-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrSpaceLocation struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    ValidateXrFlagsResult space_location_flags_result = ValidateXrSpaceLocationFlags(value->locationFlags);
    // Valid flags available, so it must be invalid to fail.
    if (VALIDATE_XR_FLAGS_INVALID == space_location_flags_result) {
        // Otherwise, flags must be valid.
        std::ostringstream oss_enum;
        oss_enum << "XrSpaceLocation invalid member XrSpaceLocationFlags \"locationFlags\" flag value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->locationFlags));
        oss_enum <<" contains illegal bit";
        CoreValidLogMessage(instance_info, "VUID-XrSpaceLocation-locationFlags-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrViewConfigurationProperties* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_VIEW_CONFIGURATION_PROPERTIES) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrViewConfigurationProperties",
                             value->type, "VUID-XrViewConfigurationProperties-type-type", XR_TYPE_VIEW_CONFIGURATION_PROPERTIES, "XR_TYPE_VIEW_CONFIGURATION_PROPERTIES");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrViewConfigurationProperties-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrViewConfigurationProperties struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrViewConfigurationProperties : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrViewConfigurationProperties-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrViewConfigurationProperties struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Make sure the enum type XrViewConfigurationType value is valid
    if (!ValidateXrEnum(instance_info, command_name, "XrViewConfigurationProperties", "viewConfigurationType", objects_info, value->viewConfigurationType)) {
        std::ostringstream oss_enum;
        oss_enum << "XrViewConfigurationProperties contains invalid XrViewConfigurationType \"viewConfigurationType\" enum value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->viewConfigurationType));
        CoreValidLogMessage(instance_info, "VUID-XrViewConfigurationProperties-viewConfigurationType-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrViewConfigurationView* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_VIEW_CONFIGURATION_VIEW) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrViewConfigurationView",
                             value->type, "VUID-XrViewConfigurationView-type-type", XR_TYPE_VIEW_CONFIGURATION_VIEW, "XR_TYPE_VIEW_CONFIGURATION_VIEW");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrViewConfigurationView-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrViewConfigurationView struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrViewConfigurationView : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrViewConfigurationView-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrViewConfigurationView struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSwapchainCreateInfo* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_SWAPCHAIN_CREATE_INFO) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrSwapchainCreateInfo",
                             value->type, "VUID-XrSwapchainCreateInfo-type-type", XR_TYPE_SWAPCHAIN_CREATE_INFO, "XR_TYPE_SWAPCHAIN_CREATE_INFO");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrSwapchainCreateInfo-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrSwapchainCreateInfo struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrSwapchainCreateInfo : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrSwapchainCreateInfo-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrSwapchainCreateInfo struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    ValidateXrFlagsResult swapchain_create_flags_result = ValidateXrSwapchainCreateFlags(value->createFlags);
    // Valid flags available, so it must be invalid to fail.
    if (VALIDATE_XR_FLAGS_INVALID == swapchain_create_flags_result) {
        // Otherwise, flags must be valid.
        std::ostringstream oss_enum;
        oss_enum << "XrSwapchainCreateInfo invalid member XrSwapchainCreateFlags \"createFlags\" flag value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->createFlags));
        oss_enum <<" contains illegal bit";
        CoreValidLogMessage(instance_info, "VUID-XrSwapchainCreateInfo-createFlags-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    ValidateXrFlagsResult swapchain_usage_flags_result = ValidateXrSwapchainUsageFlags(value->usageFlags);
    // Valid flags available, so it must be invalid to fail.
    if (VALIDATE_XR_FLAGS_INVALID == swapchain_usage_flags_result) {
        // Otherwise, flags must be valid.
        std::ostringstream oss_enum;
        oss_enum << "XrSwapchainCreateInfo invalid member XrSwapchainUsageFlags \"usageFlags\" flag value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->usageFlags));
        oss_enum <<" contains illegal bit";
        CoreValidLogMessage(instance_info, "VUID-XrSwapchainCreateInfo-usageFlags-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSwapchainImageBaseHeader* value) {
    XrResult xr_result = XR_SUCCESS;
    // NOTE: Can't validate "VUID-XrSwapchainImageBaseHeader-type-parameter" because it is a base structure
    // NOTE: Can't validate "VUID-XrSwapchainImageBaseHeader-next-next" because it is a base structure
#if defined(XR_USE_GRAPHICS_API_OPENGL)
    if (value->type == XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR) {
        const XrSwapchainImageOpenGLKHR* new_value = reinterpret_cast<const XrSwapchainImageOpenGLKHR*>(value);
        if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_opengl_enable")) {
            std::string error_str = "XrSwapchainImageBaseHeader being used with child struct type ";
            error_str += "\"XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR\"";
            error_str += " which requires extension \"XR_KHR_opengl_enable\" to be enabled, but it is not enabled";
            CoreValidLogMessage(instance_info, "VUID-XrSwapchainImageBaseHeader-type-type",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, error_str);
            return XR_ERROR_VALIDATION_FAILURE;
        }
        return ValidateXrStruct(instance_info, command_name, objects_info, check_members, new_value);
    }
#endif // defined(XR_USE_GRAPHICS_API_OPENGL)
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
    if (value->type == XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR) {
        const XrSwapchainImageOpenGLESKHR* new_value = reinterpret_cast<const XrSwapchainImageOpenGLESKHR*>(value);
        if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_opengl_es_enable")) {
            std::string error_str = "XrSwapchainImageBaseHeader being used with child struct type ";
            error_str += "\"XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR\"";
            error_str += " which requires extension \"XR_KHR_opengl_es_enable\" to be enabled, but it is not enabled";
            CoreValidLogMessage(instance_info, "VUID-XrSwapchainImageBaseHeader-type-type",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, error_str);
            return XR_ERROR_VALIDATION_FAILURE;
        }
        return ValidateXrStruct(instance_info, command_name, objects_info, check_members, new_value);
    }
#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    if (value->type == XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR) {
        const XrSwapchainImageVulkanKHR* new_value = reinterpret_cast<const XrSwapchainImageVulkanKHR*>(value);
        if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_vulkan_enable")) {
            std::string error_str = "XrSwapchainImageBaseHeader being used with child struct type ";
            error_str += "\"XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR\"";
            error_str += " which requires extension \"XR_KHR_vulkan_enable\" to be enabled, but it is not enabled";
            CoreValidLogMessage(instance_info, "VUID-XrSwapchainImageBaseHeader-type-type",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, error_str);
            return XR_ERROR_VALIDATION_FAILURE;
        }
        return ValidateXrStruct(instance_info, command_name, objects_info, check_members, new_value);
    }
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_D3D11)
    if (value->type == XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR) {
        const XrSwapchainImageD3D11KHR* new_value = reinterpret_cast<const XrSwapchainImageD3D11KHR*>(value);
        if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_D3D11_enable")) {
            std::string error_str = "XrSwapchainImageBaseHeader being used with child struct type ";
            error_str += "\"XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR\"";
            error_str += " which requires extension \"XR_KHR_D3D11_enable\" to be enabled, but it is not enabled";
            CoreValidLogMessage(instance_info, "VUID-XrSwapchainImageBaseHeader-type-type",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, error_str);
            return XR_ERROR_VALIDATION_FAILURE;
        }
        return ValidateXrStruct(instance_info, command_name, objects_info, check_members, new_value);
    }
#endif // defined(XR_USE_GRAPHICS_API_D3D11)
#if defined(XR_USE_GRAPHICS_API_D3D12)
    if (value->type == XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR) {
        const XrSwapchainImageD3D12KHR* new_value = reinterpret_cast<const XrSwapchainImageD3D12KHR*>(value);
        if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_D3D12_enable")) {
            std::string error_str = "XrSwapchainImageBaseHeader being used with child struct type ";
            error_str += "\"XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR\"";
            error_str += " which requires extension \"XR_KHR_D3D12_enable\" to be enabled, but it is not enabled";
            CoreValidLogMessage(instance_info, "VUID-XrSwapchainImageBaseHeader-type-type",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, error_str);
            return XR_ERROR_VALIDATION_FAILURE;
        }
        return ValidateXrStruct(instance_info, command_name, objects_info, check_members, new_value);
    }
#endif // defined(XR_USE_GRAPHICS_API_D3D12)
    InvalidStructureType(instance_info, command_name, objects_info, "XrSwapchainImageBaseHeader",
                         value->type, "VUID-XrSwapchainImageBaseHeader-type-type");
    return XR_ERROR_VALIDATION_FAILURE;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSwapchainImageAcquireInfo* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrSwapchainImageAcquireInfo",
                             value->type, "VUID-XrSwapchainImageAcquireInfo-type-type", XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO, "XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrSwapchainImageAcquireInfo-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrSwapchainImageAcquireInfo struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrSwapchainImageAcquireInfo : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrSwapchainImageAcquireInfo-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrSwapchainImageAcquireInfo struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSwapchainImageWaitInfo* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrSwapchainImageWaitInfo",
                             value->type, "VUID-XrSwapchainImageWaitInfo-type-type", XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO, "XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrSwapchainImageWaitInfo-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrSwapchainImageWaitInfo struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrSwapchainImageWaitInfo : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrSwapchainImageWaitInfo-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrSwapchainImageWaitInfo struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSwapchainImageReleaseInfo* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrSwapchainImageReleaseInfo",
                             value->type, "VUID-XrSwapchainImageReleaseInfo-type-type", XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, "XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrSwapchainImageReleaseInfo-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrSwapchainImageReleaseInfo struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrSwapchainImageReleaseInfo : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrSwapchainImageReleaseInfo-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrSwapchainImageReleaseInfo struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSessionBeginInfo* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_SESSION_BEGIN_INFO) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrSessionBeginInfo",
                             value->type, "VUID-XrSessionBeginInfo-type-type", XR_TYPE_SESSION_BEGIN_INFO, "XR_TYPE_SESSION_BEGIN_INFO");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrSessionBeginInfo-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrSessionBeginInfo struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrSessionBeginInfo : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrSessionBeginInfo-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrSessionBeginInfo struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Make sure the enum type XrViewConfigurationType value is valid
    if (!ValidateXrEnum(instance_info, command_name, "XrSessionBeginInfo", "primaryViewConfigurationType", objects_info, value->primaryViewConfigurationType)) {
        std::ostringstream oss_enum;
        oss_enum << "XrSessionBeginInfo contains invalid XrViewConfigurationType \"primaryViewConfigurationType\" enum value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->primaryViewConfigurationType));
        CoreValidLogMessage(instance_info, "VUID-XrSessionBeginInfo-primaryViewConfigurationType-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrFrameWaitInfo* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_FRAME_WAIT_INFO) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrFrameWaitInfo",
                             value->type, "VUID-XrFrameWaitInfo-type-type", XR_TYPE_FRAME_WAIT_INFO, "XR_TYPE_FRAME_WAIT_INFO");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrFrameWaitInfo-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrFrameWaitInfo struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrFrameWaitInfo : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrFrameWaitInfo-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrFrameWaitInfo struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrFrameState* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_FRAME_STATE) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrFrameState",
                             value->type, "VUID-XrFrameState-type-type", XR_TYPE_FRAME_STATE, "XR_TYPE_FRAME_STATE");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrFrameState-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrFrameState struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrFrameState : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrFrameState-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrFrameState struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrFrameBeginInfo* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_FRAME_BEGIN_INFO) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrFrameBeginInfo",
                             value->type, "VUID-XrFrameBeginInfo-type-type", XR_TYPE_FRAME_BEGIN_INFO, "XR_TYPE_FRAME_BEGIN_INFO");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrFrameBeginInfo-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrFrameBeginInfo struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrFrameBeginInfo : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrFrameBeginInfo-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrFrameBeginInfo struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrCompositionLayerBaseHeader* value) {
    XrResult xr_result = XR_SUCCESS;
    // NOTE: Can't validate "VUID-XrCompositionLayerBaseHeader-type-parameter" because it is a base structure
    // NOTE: Can't validate "VUID-XrCompositionLayerBaseHeader-next-next" because it is a base structure
    // NOTE: Can't validate "VUID-XrCompositionLayerBaseHeader-layerFlags-parameter" because it is a base structure
    // NOTE: Can't validate "VUID-XrCompositionLayerBaseHeader-space-parameter" because it is a base structure
    if (value->type == XR_TYPE_COMPOSITION_LAYER_PROJECTION) {
        const XrCompositionLayerProjection* new_value = reinterpret_cast<const XrCompositionLayerProjection*>(value);
        return ValidateXrStruct(instance_info, command_name, objects_info, check_members, new_value);
    }
    if (value->type == XR_TYPE_COMPOSITION_LAYER_QUAD) {
        const XrCompositionLayerQuad* new_value = reinterpret_cast<const XrCompositionLayerQuad*>(value);
        return ValidateXrStruct(instance_info, command_name, objects_info, check_members, new_value);
    }
    if (value->type == XR_TYPE_COMPOSITION_LAYER_CUBE_KHR) {
        const XrCompositionLayerCubeKHR* new_value = reinterpret_cast<const XrCompositionLayerCubeKHR*>(value);
        if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_composition_layer_cube")) {
            std::string error_str = "XrCompositionLayerBaseHeader being used with child struct type ";
            error_str += "\"XR_TYPE_COMPOSITION_LAYER_CUBE_KHR\"";
            error_str += " which requires extension \"XR_KHR_composition_layer_cube\" to be enabled, but it is not enabled";
            CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerBaseHeader-type-type",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, error_str);
            return XR_ERROR_VALIDATION_FAILURE;
        }
        return ValidateXrStruct(instance_info, command_name, objects_info, check_members, new_value);
    }
    if (value->type == XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR) {
        const XrCompositionLayerCylinderKHR* new_value = reinterpret_cast<const XrCompositionLayerCylinderKHR*>(value);
        if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_composition_layer_cylinder")) {
            std::string error_str = "XrCompositionLayerBaseHeader being used with child struct type ";
            error_str += "\"XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR\"";
            error_str += " which requires extension \"XR_KHR_composition_layer_cylinder\" to be enabled, but it is not enabled";
            CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerBaseHeader-type-type",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, error_str);
            return XR_ERROR_VALIDATION_FAILURE;
        }
        return ValidateXrStruct(instance_info, command_name, objects_info, check_members, new_value);
    }
    if (value->type == XR_TYPE_COMPOSITION_LAYER_EQUIRECT_KHR) {
        const XrCompositionLayerEquirectKHR* new_value = reinterpret_cast<const XrCompositionLayerEquirectKHR*>(value);
        if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_composition_layer_equirect")) {
            std::string error_str = "XrCompositionLayerBaseHeader being used with child struct type ";
            error_str += "\"XR_TYPE_COMPOSITION_LAYER_EQUIRECT_KHR\"";
            error_str += " which requires extension \"XR_KHR_composition_layer_equirect\" to be enabled, but it is not enabled";
            CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerBaseHeader-type-type",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, error_str);
            return XR_ERROR_VALIDATION_FAILURE;
        }
        return ValidateXrStruct(instance_info, command_name, objects_info, check_members, new_value);
    }
    InvalidStructureType(instance_info, command_name, objects_info, "XrCompositionLayerBaseHeader",
                         value->type, "VUID-XrCompositionLayerBaseHeader-type-type");
    return XR_ERROR_VALIDATION_FAILURE;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrFrameEndInfo* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_FRAME_END_INFO) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrFrameEndInfo",
                             value->type, "VUID-XrFrameEndInfo-type-type", XR_TYPE_FRAME_END_INFO, "XR_TYPE_FRAME_END_INFO");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrFrameEndInfo-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrFrameEndInfo struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrFrameEndInfo : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrFrameEndInfo-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrFrameEndInfo struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Make sure the enum type XrEnvironmentBlendMode value is valid
    if (!ValidateXrEnum(instance_info, command_name, "XrFrameEndInfo", "environmentBlendMode", objects_info, value->environmentBlendMode)) {
        std::ostringstream oss_enum;
        oss_enum << "XrFrameEndInfo contains invalid XrEnvironmentBlendMode \"environmentBlendMode\" enum value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->environmentBlendMode));
        CoreValidLogMessage(instance_info, "VUID-XrFrameEndInfo-environmentBlendMode-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Optional array must be non-NULL when value->layerCount is non-zero
    if (0 != value->layerCount && nullptr == value->layers) {
        CoreValidLogMessage(instance_info, "VUID-XrFrameEndInfo-layers-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrFrameEndInfo member layerCount is NULL, but value->layerCount is greater than 0");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    for (uint32_t value_layers_inc = 0; value_layers_inc < value->layerCount; ++value_layers_inc) {
        // Validate if XrCompositionLayerBaseHeader is a child structure of type XrCompositionLayerProjection and it is valid
        const XrCompositionLayerProjection* const* new_compositionlayerprojection_value = reinterpret_cast<const XrCompositionLayerProjection* const*>(value->layers);
        if (new_compositionlayerprojection_value[value_layers_inc]->type == XR_TYPE_COMPOSITION_LAYER_PROJECTION) {
            if (nullptr != new_compositionlayerprojection_value) {
                xr_result = ValidateXrStruct(instance_info, command_name,
                                                                objects_info, check_members, new_compositionlayerprojection_value[value_layers_inc]);
                if (XR_SUCCESS != xr_result) {
                    std::string error_message = "Structure XrFrameEndInfo member layers";
                    error_message += "[";
                    error_message += std::to_string(value_layers_inc);
                    error_message += "]";
                    error_message += " is invalid";
                    CoreValidLogMessage(instance_info, "VUID-XrFrameEndInfo-layers-parameter",
                                        VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                        objects_info,
                                        error_message);
                    return XR_ERROR_VALIDATION_FAILURE;
                    break;
                } else {
                    continue;
                    }
            }
        }
        // Validate if XrCompositionLayerBaseHeader is a child structure of type XrCompositionLayerQuad and it is valid
        const XrCompositionLayerQuad* const* new_compositionlayerquad_value = reinterpret_cast<const XrCompositionLayerQuad* const*>(value->layers);
        if (new_compositionlayerquad_value[value_layers_inc]->type == XR_TYPE_COMPOSITION_LAYER_QUAD) {
            if (nullptr != new_compositionlayerquad_value) {
                xr_result = ValidateXrStruct(instance_info, command_name,
                                                                objects_info, check_members, new_compositionlayerquad_value[value_layers_inc]);
                if (XR_SUCCESS != xr_result) {
                    std::string error_message = "Structure XrFrameEndInfo member layers";
                    error_message += "[";
                    error_message += std::to_string(value_layers_inc);
                    error_message += "]";
                    error_message += " is invalid";
                    CoreValidLogMessage(instance_info, "VUID-XrFrameEndInfo-layers-parameter",
                                        VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                        objects_info,
                                        error_message);
                    return XR_ERROR_VALIDATION_FAILURE;
                    break;
                } else {
                    continue;
                    }
            }
        }
        // Validate if XrCompositionLayerBaseHeader is a child structure of type XrCompositionLayerCubeKHR and it is valid
        const XrCompositionLayerCubeKHR* const* new_compositionlayercubekhr_value = reinterpret_cast<const XrCompositionLayerCubeKHR* const*>(value->layers);
        if (new_compositionlayercubekhr_value[value_layers_inc]->type == XR_TYPE_COMPOSITION_LAYER_CUBE_KHR) {
            if (nullptr != new_compositionlayercubekhr_value) {
                xr_result = ValidateXrStruct(instance_info, command_name,
                                                                objects_info, check_members, new_compositionlayercubekhr_value[value_layers_inc]);
                if (XR_SUCCESS != xr_result) {
                    std::string error_message = "Structure XrFrameEndInfo member layers";
                    error_message += "[";
                    error_message += std::to_string(value_layers_inc);
                    error_message += "]";
                    error_message += " is invalid";
                    CoreValidLogMessage(instance_info, "VUID-XrFrameEndInfo-layers-parameter",
                                        VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                        objects_info,
                                        error_message);
                    return XR_ERROR_VALIDATION_FAILURE;
                    break;
                } else {
                    continue;
                    }
            }
        }
        // Validate if XrCompositionLayerBaseHeader is a child structure of type XrCompositionLayerCylinderKHR and it is valid
        const XrCompositionLayerCylinderKHR* const* new_compositionlayercylinderkhr_value = reinterpret_cast<const XrCompositionLayerCylinderKHR* const*>(value->layers);
        if (new_compositionlayercylinderkhr_value[value_layers_inc]->type == XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR) {
            if (nullptr != new_compositionlayercylinderkhr_value) {
                xr_result = ValidateXrStruct(instance_info, command_name,
                                                                objects_info, check_members, new_compositionlayercylinderkhr_value[value_layers_inc]);
                if (XR_SUCCESS != xr_result) {
                    std::string error_message = "Structure XrFrameEndInfo member layers";
                    error_message += "[";
                    error_message += std::to_string(value_layers_inc);
                    error_message += "]";
                    error_message += " is invalid";
                    CoreValidLogMessage(instance_info, "VUID-XrFrameEndInfo-layers-parameter",
                                        VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                        objects_info,
                                        error_message);
                    return XR_ERROR_VALIDATION_FAILURE;
                    break;
                } else {
                    continue;
                    }
            }
        }
        // Validate if XrCompositionLayerBaseHeader is a child structure of type XrCompositionLayerEquirectKHR and it is valid
        const XrCompositionLayerEquirectKHR* const* new_compositionlayerequirectkhr_value = reinterpret_cast<const XrCompositionLayerEquirectKHR* const*>(value->layers);
        if (new_compositionlayerequirectkhr_value[value_layers_inc]->type == XR_TYPE_COMPOSITION_LAYER_EQUIRECT_KHR) {
            if (nullptr != new_compositionlayerequirectkhr_value) {
                xr_result = ValidateXrStruct(instance_info, command_name,
                                                                objects_info, check_members, new_compositionlayerequirectkhr_value[value_layers_inc]);
                if (XR_SUCCESS != xr_result) {
                    std::string error_message = "Structure XrFrameEndInfo member layers";
                    error_message += "[";
                    error_message += std::to_string(value_layers_inc);
                    error_message += "]";
                    error_message += " is invalid";
                    CoreValidLogMessage(instance_info, "VUID-XrFrameEndInfo-layers-parameter",
                                        VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                        objects_info,
                                        error_message);
                    return XR_ERROR_VALIDATION_FAILURE;
                    break;
                } else {
                    continue;
                    }
            }
        }
        // Validate that the base-structure XrCompositionLayerBaseHeader is valid
        if (nullptr != value->layers[value_layers_inc]) {
            xr_result = ValidateXrStruct(instance_info, command_name,
                                                            objects_info, check_members, value->layers[value_layers_inc]);
            if (XR_SUCCESS != xr_result) {
                CoreValidLogMessage(instance_info, "VUID-XrFrameEndInfo-layers-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info,
                                    "Structure XrFrameEndInfo member layers is invalid");
                return xr_result;
            }
        }
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrViewLocateInfo* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_VIEW_LOCATE_INFO) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrViewLocateInfo",
                             value->type, "VUID-XrViewLocateInfo-type-type", XR_TYPE_VIEW_LOCATE_INFO, "XR_TYPE_VIEW_LOCATE_INFO");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrViewLocateInfo-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrViewLocateInfo struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrViewLocateInfo : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrViewLocateInfo-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrViewLocateInfo struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Make sure the enum type XrViewConfigurationType value is valid
    if (!ValidateXrEnum(instance_info, command_name, "XrViewLocateInfo", "viewConfigurationType", objects_info, value->viewConfigurationType)) {
        std::ostringstream oss_enum;
        oss_enum << "XrViewLocateInfo contains invalid XrViewConfigurationType \"viewConfigurationType\" enum value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->viewConfigurationType));
        CoreValidLogMessage(instance_info, "VUID-XrViewLocateInfo-viewConfigurationType-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    {
        // writeValidateInlineHandleValidation
        ValidateXrHandleResult handle_result = VerifyXrSpaceHandle(&value->space);
        if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
            // Not a valid handle or NULL (which is not valid in this case)
            std::ostringstream oss;
            oss << "Invalid XrSpace handle \"space\" ";
            oss << HandleToHexString(value->space);
            CoreValidLogMessage(instance_info, "VUID-XrViewLocateInfo-space-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, oss.str());
            return XR_ERROR_HANDLE_INVALID;
        }
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrViewState* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_VIEW_STATE) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrViewState",
                             value->type, "VUID-XrViewState-type-type", XR_TYPE_VIEW_STATE, "XR_TYPE_VIEW_STATE");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrViewState-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrViewState struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrViewState : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrViewState-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrViewState struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    ValidateXrFlagsResult view_state_flags_result = ValidateXrViewStateFlags(value->viewStateFlags);
    // Valid flags available, so it must be invalid to fail.
    if (VALIDATE_XR_FLAGS_INVALID == view_state_flags_result) {
        // Otherwise, flags must be valid.
        std::ostringstream oss_enum;
        oss_enum << "XrViewState invalid member XrViewStateFlags \"viewStateFlags\" flag value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->viewStateFlags));
        oss_enum <<" contains illegal bit";
        CoreValidLogMessage(instance_info, "VUID-XrViewState-viewStateFlags-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrFovf* value) {
    XrResult xr_result = XR_SUCCESS;
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrView* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_VIEW) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrView",
                             value->type, "VUID-XrView-type-type", XR_TYPE_VIEW, "XR_TYPE_VIEW");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrView-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrView struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrView : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrView-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrView struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrActionSetCreateInfo* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_ACTION_SET_CREATE_INFO) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrActionSetCreateInfo",
                             value->type, "VUID-XrActionSetCreateInfo-type-type", XR_TYPE_ACTION_SET_CREATE_INFO, "XR_TYPE_ACTION_SET_CREATE_INFO");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrActionSetCreateInfo-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrActionSetCreateInfo struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrActionSetCreateInfo : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrActionSetCreateInfo-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrActionSetCreateInfo struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    if (XR_MAX_ACTION_SET_NAME_SIZE < std::strlen(value->actionSetName)) {
        CoreValidLogMessage(instance_info, "VUID-XrActionSetCreateInfo-actionSetName-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrActionSetCreateInfo member actionSetName length is too long.");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    if (XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE < std::strlen(value->localizedActionSetName)) {
        CoreValidLogMessage(instance_info, "VUID-XrActionSetCreateInfo-localizedActionSetName-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrActionSetCreateInfo member localizedActionSetName length is too long.");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrActionCreateInfo* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_ACTION_CREATE_INFO) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrActionCreateInfo",
                             value->type, "VUID-XrActionCreateInfo-type-type", XR_TYPE_ACTION_CREATE_INFO, "XR_TYPE_ACTION_CREATE_INFO");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrActionCreateInfo-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrActionCreateInfo struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrActionCreateInfo : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrActionCreateInfo-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrActionCreateInfo struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    if (XR_MAX_ACTION_NAME_SIZE < std::strlen(value->actionName)) {
        CoreValidLogMessage(instance_info, "VUID-XrActionCreateInfo-actionName-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrActionCreateInfo member actionName length is too long.");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Make sure the enum type XrActionType value is valid
    if (!ValidateXrEnum(instance_info, command_name, "XrActionCreateInfo", "actionType", objects_info, value->actionType)) {
        std::ostringstream oss_enum;
        oss_enum << "XrActionCreateInfo contains invalid XrActionType \"actionType\" enum value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->actionType));
        CoreValidLogMessage(instance_info, "VUID-XrActionCreateInfo-actionType-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Optional array must be non-NULL when value->countSubactionPaths is non-zero
    if (0 != value->countSubactionPaths && nullptr == value->subactionPaths) {
        CoreValidLogMessage(instance_info, "VUID-XrActionCreateInfo-subactionPaths-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrActionCreateInfo member countSubactionPaths is NULL, but value->countSubactionPaths is greater than 0");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // NOTE: Can't validate "VUID-XrActionCreateInfo-subactionPaths-parameter" type
    if (XR_MAX_LOCALIZED_ACTION_NAME_SIZE < std::strlen(value->localizedActionName)) {
        CoreValidLogMessage(instance_info, "VUID-XrActionCreateInfo-localizedActionName-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrActionCreateInfo member localizedActionName length is too long.");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrActionSuggestedBinding* value) {
    XrResult xr_result = XR_SUCCESS;
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    {
        // writeValidateInlineHandleValidation
        ValidateXrHandleResult handle_result = VerifyXrActionHandle(&value->action);
        if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
            // Not a valid handle or NULL (which is not valid in this case)
            std::ostringstream oss;
            oss << "Invalid XrAction handle \"action\" ";
            oss << HandleToHexString(value->action);
            CoreValidLogMessage(instance_info, "VUID-XrActionSuggestedBinding-action-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, oss.str());
            return XR_ERROR_HANDLE_INVALID;
        }
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrInteractionProfileSuggestedBinding* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrInteractionProfileSuggestedBinding",
                             value->type, "VUID-XrInteractionProfileSuggestedBinding-type-type", XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING, "XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrInteractionProfileSuggestedBinding-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrInteractionProfileSuggestedBinding struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrInteractionProfileSuggestedBinding : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrInteractionProfileSuggestedBinding-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrInteractionProfileSuggestedBinding struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Non-optional array length must be non-zero
    if (0 >= value->countSuggestedBindings && nullptr != value->suggestedBindings) {
        CoreValidLogMessage(instance_info, "VUID-XrInteractionProfileSuggestedBinding-countSuggestedBindings-arraylength",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrInteractionProfileSuggestedBinding member countSuggestedBindings is non-optional and must be greater than 0");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // Pointer/array variable with a length variable.  Make sure that
    // if length variable is non-zero that the pointer is not NULL
    if (nullptr == value->suggestedBindings && 0 != value->countSuggestedBindings) {
        CoreValidLogMessage(instance_info, "VUID-XrInteractionProfileSuggestedBinding-suggestedBindings-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name, objects_info,
                            "XrInteractionProfileSuggestedBinding contains invalid NULL for XrActionSuggestedBinding \"suggestedBindings\" is which not "
                            "optional since \"countSuggestedBindings\" is set and must be non-NULL");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    for (uint32_t value_suggestedbindings_inc = 0; value_suggestedbindings_inc < value->countSuggestedBindings; ++value_suggestedbindings_inc) {
        // Validate that the structure XrActionSuggestedBinding is valid
        xr_result = ValidateXrStruct(instance_info, command_name, objects_info,
                                                        check_members, &value->suggestedBindings[value_suggestedbindings_inc]);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(instance_info, "VUID-XrInteractionProfileSuggestedBinding-suggestedBindings-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info,
                                "Structure XrInteractionProfileSuggestedBinding member suggestedBindings is invalid");
            return xr_result;
        }
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSessionActionSetsAttachInfo* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrSessionActionSetsAttachInfo",
                             value->type, "VUID-XrSessionActionSetsAttachInfo-type-type", XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO, "XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrSessionActionSetsAttachInfo-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrSessionActionSetsAttachInfo struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrSessionActionSetsAttachInfo : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrSessionActionSetsAttachInfo-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrSessionActionSetsAttachInfo struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Non-optional array length must be non-zero
    if (0 >= value->countActionSets && nullptr != value->actionSets) {
        CoreValidLogMessage(instance_info, "VUID-XrSessionActionSetsAttachInfo-countActionSets-arraylength",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrSessionActionSetsAttachInfo member countActionSets is non-optional and must be greater than 0");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // Pointer/array variable with a length variable.  Make sure that
    // if length variable is non-zero that the pointer is not NULL
    if (nullptr == value->actionSets && 0 != value->countActionSets) {
        CoreValidLogMessage(instance_info, "VUID-XrSessionActionSetsAttachInfo-actionSets-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name, objects_info,
                            "XrSessionActionSetsAttachInfo contains invalid NULL for XrActionSet \"actionSets\" is which not "
                            "optional since \"countActionSets\" is set and must be non-NULL");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    for (uint32_t value_actionsets_inc = 0; value_actionsets_inc < value->countActionSets; ++value_actionsets_inc) {
        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrActionSetHandle(&value->actionSets[value_actionsets_inc]);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrActionSet handle \"actionSets\" ";
                oss << HandleToHexString(value->actionSets[value_actionsets_inc]);
                CoreValidLogMessage(instance_info, "VUID-XrSessionActionSetsAttachInfo-actionSets-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrInteractionProfileState* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_INTERACTION_PROFILE_STATE) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrInteractionProfileState",
                             value->type, "VUID-XrInteractionProfileState-type-type", XR_TYPE_INTERACTION_PROFILE_STATE, "XR_TYPE_INTERACTION_PROFILE_STATE");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrInteractionProfileState-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrInteractionProfileState struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrInteractionProfileState : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrInteractionProfileState-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrInteractionProfileState struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrActionStateGetInfo* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_ACTION_STATE_GET_INFO) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrActionStateGetInfo",
                             value->type, "VUID-XrActionStateGetInfo-type-type", XR_TYPE_ACTION_STATE_GET_INFO, "XR_TYPE_ACTION_STATE_GET_INFO");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrActionStateGetInfo-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrActionStateGetInfo struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrActionStateGetInfo : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrActionStateGetInfo-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrActionStateGetInfo struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    {
        // writeValidateInlineHandleValidation
        ValidateXrHandleResult handle_result = VerifyXrActionHandle(&value->action);
        if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
            // Not a valid handle or NULL (which is not valid in this case)
            std::ostringstream oss;
            oss << "Invalid XrAction handle \"action\" ";
            oss << HandleToHexString(value->action);
            CoreValidLogMessage(instance_info, "VUID-XrActionStateGetInfo-action-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, oss.str());
            return XR_ERROR_HANDLE_INVALID;
        }
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrActionStateBoolean* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_ACTION_STATE_BOOLEAN) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrActionStateBoolean",
                             value->type, "VUID-XrActionStateBoolean-type-type", XR_TYPE_ACTION_STATE_BOOLEAN, "XR_TYPE_ACTION_STATE_BOOLEAN");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrActionStateBoolean-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrActionStateBoolean struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrActionStateBoolean : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrActionStateBoolean-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrActionStateBoolean struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrActionStateFloat* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_ACTION_STATE_FLOAT) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrActionStateFloat",
                             value->type, "VUID-XrActionStateFloat-type-type", XR_TYPE_ACTION_STATE_FLOAT, "XR_TYPE_ACTION_STATE_FLOAT");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrActionStateFloat-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrActionStateFloat struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrActionStateFloat : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrActionStateFloat-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrActionStateFloat struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrVector2f* value) {
    XrResult xr_result = XR_SUCCESS;
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrActionStateVector2f* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_ACTION_STATE_VECTOR2F) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrActionStateVector2f",
                             value->type, "VUID-XrActionStateVector2f-type-type", XR_TYPE_ACTION_STATE_VECTOR2F, "XR_TYPE_ACTION_STATE_VECTOR2F");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrActionStateVector2f-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrActionStateVector2f struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrActionStateVector2f : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrActionStateVector2f-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrActionStateVector2f struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrActionStatePose* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_ACTION_STATE_POSE) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrActionStatePose",
                             value->type, "VUID-XrActionStatePose-type-type", XR_TYPE_ACTION_STATE_POSE, "XR_TYPE_ACTION_STATE_POSE");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrActionStatePose-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrActionStatePose struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrActionStatePose : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrActionStatePose-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrActionStatePose struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrActiveActionSet* value) {
    XrResult xr_result = XR_SUCCESS;
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    {
        // writeValidateInlineHandleValidation
        ValidateXrHandleResult handle_result = VerifyXrActionSetHandle(&value->actionSet);
        if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
            // Not a valid handle or NULL (which is not valid in this case)
            std::ostringstream oss;
            oss << "Invalid XrActionSet handle \"actionSet\" ";
            oss << HandleToHexString(value->actionSet);
            CoreValidLogMessage(instance_info, "VUID-XrActiveActionSet-actionSet-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, oss.str());
            return XR_ERROR_HANDLE_INVALID;
        }
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrActionsSyncInfo* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_ACTIONS_SYNC_INFO) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrActionsSyncInfo",
                             value->type, "VUID-XrActionsSyncInfo-type-type", XR_TYPE_ACTIONS_SYNC_INFO, "XR_TYPE_ACTIONS_SYNC_INFO");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrActionsSyncInfo-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrActionsSyncInfo struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrActionsSyncInfo : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrActionsSyncInfo-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrActionsSyncInfo struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Optional array must be non-NULL when value->countActiveActionSets is non-zero
    if (0 != value->countActiveActionSets && nullptr == value->activeActionSets) {
        CoreValidLogMessage(instance_info, "VUID-XrActionsSyncInfo-activeActionSets-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrActionsSyncInfo member countActiveActionSets is NULL, but value->countActiveActionSets is greater than 0");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    for (uint32_t value_activeactionsets_inc = 0; value_activeactionsets_inc < value->countActiveActionSets; ++value_activeactionsets_inc) {
        // Validate that the structure XrActiveActionSet is valid
        xr_result = ValidateXrStruct(instance_info, command_name, objects_info,
                                                        check_members, &value->activeActionSets[value_activeactionsets_inc]);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(instance_info, "VUID-XrActionsSyncInfo-activeActionSets-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info,
                                "Structure XrActionsSyncInfo member activeActionSets is invalid");
            return xr_result;
        }
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrBoundSourcesForActionEnumerateInfo* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrBoundSourcesForActionEnumerateInfo",
                             value->type, "VUID-XrBoundSourcesForActionEnumerateInfo-type-type", XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO, "XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrBoundSourcesForActionEnumerateInfo-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrBoundSourcesForActionEnumerateInfo struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrBoundSourcesForActionEnumerateInfo : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrBoundSourcesForActionEnumerateInfo-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrBoundSourcesForActionEnumerateInfo struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    {
        // writeValidateInlineHandleValidation
        ValidateXrHandleResult handle_result = VerifyXrActionHandle(&value->action);
        if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
            // Not a valid handle or NULL (which is not valid in this case)
            std::ostringstream oss;
            oss << "Invalid XrAction handle \"action\" ";
            oss << HandleToHexString(value->action);
            CoreValidLogMessage(instance_info, "VUID-XrBoundSourcesForActionEnumerateInfo-action-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, oss.str());
            return XR_ERROR_HANDLE_INVALID;
        }
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrInputSourceLocalizedNameGetInfo* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrInputSourceLocalizedNameGetInfo",
                             value->type, "VUID-XrInputSourceLocalizedNameGetInfo-type-type", XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO, "XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrInputSourceLocalizedNameGetInfo-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrInputSourceLocalizedNameGetInfo struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrInputSourceLocalizedNameGetInfo : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrInputSourceLocalizedNameGetInfo-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrInputSourceLocalizedNameGetInfo struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    ValidateXrFlagsResult input_source_localized_name_flags_result = ValidateXrInputSourceLocalizedNameFlags(value->whichComponents);
    // Flags must be non-zero in this case.
    if (VALIDATE_XR_FLAGS_ZERO == input_source_localized_name_flags_result) {
        CoreValidLogMessage(instance_info, "VUID-XrInputSourceLocalizedNameGetInfo-whichComponents-requiredbitmask",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "XrInputSourceLocalizedNameFlags \"whichComponents\" flag must be non-zero");
        return XR_ERROR_VALIDATION_FAILURE;
    } else if (VALIDATE_XR_FLAGS_SUCCESS != input_source_localized_name_flags_result) {
        // Otherwise, flags must be valid.
        std::ostringstream oss_enum;
        oss_enum << "XrInputSourceLocalizedNameGetInfo invalid member XrInputSourceLocalizedNameFlags \"whichComponents\" flag value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->whichComponents));
        oss_enum <<" contains illegal bit";
        CoreValidLogMessage(instance_info, "VUID-XrInputSourceLocalizedNameGetInfo-whichComponents-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrHapticActionInfo* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_HAPTIC_ACTION_INFO) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrHapticActionInfo",
                             value->type, "VUID-XrHapticActionInfo-type-type", XR_TYPE_HAPTIC_ACTION_INFO, "XR_TYPE_HAPTIC_ACTION_INFO");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrHapticActionInfo-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrHapticActionInfo struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrHapticActionInfo : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrHapticActionInfo-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrHapticActionInfo struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    {
        // writeValidateInlineHandleValidation
        ValidateXrHandleResult handle_result = VerifyXrActionHandle(&value->action);
        if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
            // Not a valid handle or NULL (which is not valid in this case)
            std::ostringstream oss;
            oss << "Invalid XrAction handle \"action\" ";
            oss << HandleToHexString(value->action);
            CoreValidLogMessage(instance_info, "VUID-XrHapticActionInfo-action-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, oss.str());
            return XR_ERROR_HANDLE_INVALID;
        }
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrHapticBaseHeader* value) {
    XrResult xr_result = XR_SUCCESS;
    // NOTE: Can't validate "VUID-XrHapticBaseHeader-type-parameter" because it is a base structure
    // NOTE: Can't validate "VUID-XrHapticBaseHeader-next-next" because it is a base structure
    if (value->type == XR_TYPE_HAPTIC_VIBRATION) {
        const XrHapticVibration* new_value = reinterpret_cast<const XrHapticVibration*>(value);
        return ValidateXrStruct(instance_info, command_name, objects_info, check_members, new_value);
    }
    InvalidStructureType(instance_info, command_name, objects_info, "XrHapticBaseHeader",
                         value->type, "VUID-XrHapticBaseHeader-type-type");
    return XR_ERROR_VALIDATION_FAILURE;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrOffset2Di* value) {
    XrResult xr_result = XR_SUCCESS;
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrExtent2Di* value) {
    XrResult xr_result = XR_SUCCESS;
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrRect2Di* value) {
    XrResult xr_result = XR_SUCCESS;
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSwapchainSubImage* value) {
    XrResult xr_result = XR_SUCCESS;
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    {
        // writeValidateInlineHandleValidation
        ValidateXrHandleResult handle_result = VerifyXrSwapchainHandle(&value->swapchain);
        if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
            // Not a valid handle or NULL (which is not valid in this case)
            std::ostringstream oss;
            oss << "Invalid XrSwapchain handle \"swapchain\" ";
            oss << HandleToHexString(value->swapchain);
            CoreValidLogMessage(instance_info, "VUID-XrSwapchainSubImage-swapchain-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, oss.str());
            return XR_ERROR_HANDLE_INVALID;
        }
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrCompositionLayerProjectionView* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrCompositionLayerProjectionView",
                             value->type, "VUID-XrCompositionLayerProjectionView-type-type", XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW, "XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    valid_ext_structs.push_back(XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR);
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerProjectionView-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrCompositionLayerProjectionView struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrCompositionLayerProjectionView : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerProjectionView-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrCompositionLayerProjectionView struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Validate that the structure XrSwapchainSubImage is valid
    xr_result = ValidateXrStruct(instance_info, command_name, objects_info,
                                                    check_members, &value->subImage);
    if (XR_SUCCESS != xr_result) {
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerProjectionView-subImage-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrCompositionLayerProjectionView member subImage is invalid");
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrCompositionLayerProjection* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_COMPOSITION_LAYER_PROJECTION) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrCompositionLayerProjection",
                             value->type, "VUID-XrCompositionLayerProjection-type-type", XR_TYPE_COMPOSITION_LAYER_PROJECTION, "XR_TYPE_COMPOSITION_LAYER_PROJECTION");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerProjection-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrCompositionLayerProjection struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrCompositionLayerProjection : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerProjection-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrCompositionLayerProjection struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    ValidateXrFlagsResult composition_layer_flags_result = ValidateXrCompositionLayerFlags(value->layerFlags);
    // Valid flags available, so it must be invalid to fail.
    if (VALIDATE_XR_FLAGS_INVALID == composition_layer_flags_result) {
        // Otherwise, flags must be valid.
        std::ostringstream oss_enum;
        oss_enum << "XrCompositionLayerProjection invalid member XrCompositionLayerFlags \"layerFlags\" flag value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->layerFlags));
        oss_enum <<" contains illegal bit";
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerProjection-layerFlags-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    {
        // writeValidateInlineHandleValidation
        ValidateXrHandleResult handle_result = VerifyXrSpaceHandle(&value->space);
        if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
            // Not a valid handle or NULL (which is not valid in this case)
            std::ostringstream oss;
            oss << "Invalid XrSpace handle \"space\" ";
            oss << HandleToHexString(value->space);
            CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerProjection-space-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, oss.str());
            return XR_ERROR_HANDLE_INVALID;
        }
    }
    // Non-optional array length must be non-zero
    if (0 >= value->viewCount && nullptr != value->views) {
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerProjection-viewCount-arraylength",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrCompositionLayerProjection member viewCount is non-optional and must be greater than 0");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // Pointer/array variable with a length variable.  Make sure that
    // if length variable is non-zero that the pointer is not NULL
    if (nullptr == value->views && 0 != value->viewCount) {
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerProjection-views-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name, objects_info,
                            "XrCompositionLayerProjection contains invalid NULL for XrCompositionLayerProjectionView \"views\" is which not "
                            "optional since \"viewCount\" is set and must be non-NULL");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    for (uint32_t value_views_inc = 0; value_views_inc < value->viewCount; ++value_views_inc) {
        // Validate that the structure XrCompositionLayerProjectionView is valid
        xr_result = ValidateXrStruct(instance_info, command_name, objects_info,
                                                        check_members, &value->views[value_views_inc]);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerProjection-views-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info,
                                "Structure XrCompositionLayerProjection member views is invalid");
            return xr_result;
        }
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrCompositionLayerQuad* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_COMPOSITION_LAYER_QUAD) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrCompositionLayerQuad",
                             value->type, "VUID-XrCompositionLayerQuad-type-type", XR_TYPE_COMPOSITION_LAYER_QUAD, "XR_TYPE_COMPOSITION_LAYER_QUAD");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerQuad-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrCompositionLayerQuad struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrCompositionLayerQuad : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerQuad-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrCompositionLayerQuad struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    ValidateXrFlagsResult composition_layer_flags_result = ValidateXrCompositionLayerFlags(value->layerFlags);
    // Valid flags available, so it must be invalid to fail.
    if (VALIDATE_XR_FLAGS_INVALID == composition_layer_flags_result) {
        // Otherwise, flags must be valid.
        std::ostringstream oss_enum;
        oss_enum << "XrCompositionLayerQuad invalid member XrCompositionLayerFlags \"layerFlags\" flag value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->layerFlags));
        oss_enum <<" contains illegal bit";
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerQuad-layerFlags-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    {
        // writeValidateInlineHandleValidation
        ValidateXrHandleResult handle_result = VerifyXrSpaceHandle(&value->space);
        if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
            // Not a valid handle or NULL (which is not valid in this case)
            std::ostringstream oss;
            oss << "Invalid XrSpace handle \"space\" ";
            oss << HandleToHexString(value->space);
            CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerQuad-space-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, oss.str());
            return XR_ERROR_HANDLE_INVALID;
        }
    }
    // Make sure the enum type XrEyeVisibility value is valid
    if (!ValidateXrEnum(instance_info, command_name, "XrCompositionLayerQuad", "eyeVisibility", objects_info, value->eyeVisibility)) {
        std::ostringstream oss_enum;
        oss_enum << "XrCompositionLayerQuad contains invalid XrEyeVisibility \"eyeVisibility\" enum value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->eyeVisibility));
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerQuad-eyeVisibility-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Validate that the structure XrSwapchainSubImage is valid
    xr_result = ValidateXrStruct(instance_info, command_name, objects_info,
                                                    check_members, &value->subImage);
    if (XR_SUCCESS != xr_result) {
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerQuad-subImage-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrCompositionLayerQuad member subImage is invalid");
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrEventDataBaseHeader* value) {
    XrResult xr_result = XR_SUCCESS;
    // NOTE: Can't validate "VUID-XrEventDataBaseHeader-type-parameter" because it is a base structure
    // NOTE: Can't validate "VUID-XrEventDataBaseHeader-next-next" because it is a base structure
    if (value->type == XR_TYPE_EVENT_DATA_EVENTS_LOST) {
        const XrEventDataEventsLost* new_value = reinterpret_cast<const XrEventDataEventsLost*>(value);
        return ValidateXrStruct(instance_info, command_name, objects_info, check_members, new_value);
    }
    if (value->type == XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING) {
        const XrEventDataInstanceLossPending* new_value = reinterpret_cast<const XrEventDataInstanceLossPending*>(value);
        return ValidateXrStruct(instance_info, command_name, objects_info, check_members, new_value);
    }
    if (value->type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
        const XrEventDataSessionStateChanged* new_value = reinterpret_cast<const XrEventDataSessionStateChanged*>(value);
        return ValidateXrStruct(instance_info, command_name, objects_info, check_members, new_value);
    }
    if (value->type == XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING) {
        const XrEventDataReferenceSpaceChangePending* new_value = reinterpret_cast<const XrEventDataReferenceSpaceChangePending*>(value);
        return ValidateXrStruct(instance_info, command_name, objects_info, check_members, new_value);
    }
    if (value->type == XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED) {
        const XrEventDataInteractionProfileChanged* new_value = reinterpret_cast<const XrEventDataInteractionProfileChanged*>(value);
        return ValidateXrStruct(instance_info, command_name, objects_info, check_members, new_value);
    }
    if (value->type == XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR) {
        const XrEventDataVisibilityMaskChangedKHR* new_value = reinterpret_cast<const XrEventDataVisibilityMaskChangedKHR*>(value);
        if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_KHR_visibility_mask")) {
            std::string error_str = "XrEventDataBaseHeader being used with child struct type ";
            error_str += "\"XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR\"";
            error_str += " which requires extension \"XR_KHR_visibility_mask\" to be enabled, but it is not enabled";
            CoreValidLogMessage(instance_info, "VUID-XrEventDataBaseHeader-type-type",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, error_str);
            return XR_ERROR_VALIDATION_FAILURE;
        }
        return ValidateXrStruct(instance_info, command_name, objects_info, check_members, new_value);
    }
    if (value->type == XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT) {
        const XrEventDataPerfSettingsEXT* new_value = reinterpret_cast<const XrEventDataPerfSettingsEXT*>(value);
        if (nullptr != instance_info && !ExtensionEnabled(instance_info->enabled_extensions, "XR_EXT_performance_settings")) {
            std::string error_str = "XrEventDataBaseHeader being used with child struct type ";
            error_str += "\"XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT\"";
            error_str += " which requires extension \"XR_EXT_performance_settings\" to be enabled, but it is not enabled";
            CoreValidLogMessage(instance_info, "VUID-XrEventDataBaseHeader-type-type",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, error_str);
            return XR_ERROR_VALIDATION_FAILURE;
        }
        return ValidateXrStruct(instance_info, command_name, objects_info, check_members, new_value);
    }
    InvalidStructureType(instance_info, command_name, objects_info, "XrEventDataBaseHeader",
                         value->type, "VUID-XrEventDataBaseHeader-type-type");
    return XR_ERROR_VALIDATION_FAILURE;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrEventDataEventsLost* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_EVENT_DATA_EVENTS_LOST) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrEventDataEventsLost",
                             value->type, "VUID-XrEventDataEventsLost-type-type", XR_TYPE_EVENT_DATA_EVENTS_LOST, "XR_TYPE_EVENT_DATA_EVENTS_LOST");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrEventDataEventsLost-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrEventDataEventsLost struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrEventDataEventsLost : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrEventDataEventsLost-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrEventDataEventsLost struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrEventDataInstanceLossPending* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrEventDataInstanceLossPending",
                             value->type, "VUID-XrEventDataInstanceLossPending-type-type", XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING, "XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrEventDataInstanceLossPending-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrEventDataInstanceLossPending struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrEventDataInstanceLossPending : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrEventDataInstanceLossPending-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrEventDataInstanceLossPending struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrEventDataSessionStateChanged* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrEventDataSessionStateChanged",
                             value->type, "VUID-XrEventDataSessionStateChanged-type-type", XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED, "XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrEventDataSessionStateChanged-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrEventDataSessionStateChanged struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrEventDataSessionStateChanged : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrEventDataSessionStateChanged-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrEventDataSessionStateChanged struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    {
        // writeValidateInlineHandleValidation
        ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&value->session);
        if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
            // Not a valid handle or NULL (which is not valid in this case)
            std::ostringstream oss;
            oss << "Invalid XrSession handle \"session\" ";
            oss << HandleToHexString(value->session);
            CoreValidLogMessage(instance_info, "VUID-XrEventDataSessionStateChanged-session-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, oss.str());
            return XR_ERROR_HANDLE_INVALID;
        }
    }
    // Make sure the enum type XrSessionState value is valid
    if (!ValidateXrEnum(instance_info, command_name, "XrEventDataSessionStateChanged", "state", objects_info, value->state)) {
        std::ostringstream oss_enum;
        oss_enum << "XrEventDataSessionStateChanged contains invalid XrSessionState \"state\" enum value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->state));
        CoreValidLogMessage(instance_info, "VUID-XrEventDataSessionStateChanged-state-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrEventDataReferenceSpaceChangePending* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrEventDataReferenceSpaceChangePending",
                             value->type, "VUID-XrEventDataReferenceSpaceChangePending-type-type", XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING, "XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrEventDataReferenceSpaceChangePending-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrEventDataReferenceSpaceChangePending struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrEventDataReferenceSpaceChangePending : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrEventDataReferenceSpaceChangePending-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrEventDataReferenceSpaceChangePending struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    {
        // writeValidateInlineHandleValidation
        ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&value->session);
        if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
            // Not a valid handle or NULL (which is not valid in this case)
            std::ostringstream oss;
            oss << "Invalid XrSession handle \"session\" ";
            oss << HandleToHexString(value->session);
            CoreValidLogMessage(instance_info, "VUID-XrEventDataReferenceSpaceChangePending-session-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, oss.str());
            return XR_ERROR_HANDLE_INVALID;
        }
    }
    // Make sure the enum type XrReferenceSpaceType value is valid
    if (!ValidateXrEnum(instance_info, command_name, "XrEventDataReferenceSpaceChangePending", "referenceSpaceType", objects_info, value->referenceSpaceType)) {
        std::ostringstream oss_enum;
        oss_enum << "XrEventDataReferenceSpaceChangePending contains invalid XrReferenceSpaceType \"referenceSpaceType\" enum value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->referenceSpaceType));
        CoreValidLogMessage(instance_info, "VUID-XrEventDataReferenceSpaceChangePending-referenceSpaceType-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrEventDataInteractionProfileChanged* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrEventDataInteractionProfileChanged",
                             value->type, "VUID-XrEventDataInteractionProfileChanged-type-type", XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED, "XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrEventDataInteractionProfileChanged-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrEventDataInteractionProfileChanged struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrEventDataInteractionProfileChanged : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrEventDataInteractionProfileChanged-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrEventDataInteractionProfileChanged struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    {
        // writeValidateInlineHandleValidation
        ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&value->session);
        if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
            // Not a valid handle or NULL (which is not valid in this case)
            std::ostringstream oss;
            oss << "Invalid XrSession handle \"session\" ";
            oss << HandleToHexString(value->session);
            CoreValidLogMessage(instance_info, "VUID-XrEventDataInteractionProfileChanged-session-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, oss.str());
            return XR_ERROR_HANDLE_INVALID;
        }
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrHapticVibration* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_HAPTIC_VIBRATION) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrHapticVibration",
                             value->type, "VUID-XrHapticVibration-type-type", XR_TYPE_HAPTIC_VIBRATION, "XR_TYPE_HAPTIC_VIBRATION");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrHapticVibration-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrHapticVibration struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrHapticVibration : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrHapticVibration-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrHapticVibration struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrOffset2Df* value) {
    XrResult xr_result = XR_SUCCESS;
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrRect2Df* value) {
    XrResult xr_result = XR_SUCCESS;
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrVector4f* value) {
    XrResult xr_result = XR_SUCCESS;
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrColor4f* value) {
    XrResult xr_result = XR_SUCCESS;
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrCompositionLayerCubeKHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_COMPOSITION_LAYER_CUBE_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrCompositionLayerCubeKHR",
                             value->type, "VUID-XrCompositionLayerCubeKHR-type-type", XR_TYPE_COMPOSITION_LAYER_CUBE_KHR, "XR_TYPE_COMPOSITION_LAYER_CUBE_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerCubeKHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrCompositionLayerCubeKHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrCompositionLayerCubeKHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerCubeKHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrCompositionLayerCubeKHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    ValidateXrFlagsResult composition_layer_flags_result = ValidateXrCompositionLayerFlags(value->layerFlags);
    // Valid flags available, so it must be invalid to fail.
    if (VALIDATE_XR_FLAGS_INVALID == composition_layer_flags_result) {
        // Otherwise, flags must be valid.
        std::ostringstream oss_enum;
        oss_enum << "XrCompositionLayerCubeKHR invalid member XrCompositionLayerFlags \"layerFlags\" flag value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->layerFlags));
        oss_enum <<" contains illegal bit";
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerCubeKHR-layerFlags-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    {
        // writeValidateInlineHandleValidation
        ValidateXrHandleResult handle_result = VerifyXrSpaceHandle(&value->space);
        if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
            // Not a valid handle or NULL (which is not valid in this case)
            std::ostringstream oss;
            oss << "Invalid XrSpace handle \"space\" ";
            oss << HandleToHexString(value->space);
            CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerCubeKHR-space-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, oss.str());
            return XR_ERROR_HANDLE_INVALID;
        }
    }
    // Make sure the enum type XrEyeVisibility value is valid
    if (!ValidateXrEnum(instance_info, command_name, "XrCompositionLayerCubeKHR", "eyeVisibility", objects_info, value->eyeVisibility)) {
        std::ostringstream oss_enum;
        oss_enum << "XrCompositionLayerCubeKHR contains invalid XrEyeVisibility \"eyeVisibility\" enum value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->eyeVisibility));
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerCubeKHR-eyeVisibility-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    {
        // writeValidateInlineHandleValidation
        ValidateXrHandleResult handle_result = VerifyXrSwapchainHandle(&value->swapchain);
        if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
            // Not a valid handle or NULL (which is not valid in this case)
            std::ostringstream oss;
            oss << "Invalid XrSwapchain handle \"swapchain\" ";
            oss << HandleToHexString(value->swapchain);
            CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerCubeKHR-swapchain-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, oss.str());
            return XR_ERROR_HANDLE_INVALID;
        }
    }
    // Everything checked out properly
    return xr_result;
}

#if defined(XR_USE_PLATFORM_ANDROID)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrInstanceCreateInfoAndroidKHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrInstanceCreateInfoAndroidKHR",
                             value->type, "VUID-XrInstanceCreateInfoAndroidKHR-type-type", XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR, "XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrInstanceCreateInfoAndroidKHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrInstanceCreateInfoAndroidKHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrInstanceCreateInfoAndroidKHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrInstanceCreateInfoAndroidKHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrInstanceCreateInfoAndroidKHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Non-optional pointer/array variable that needs to not be NULL
    if (nullptr == value->applicationVM) {
        CoreValidLogMessage(instance_info, "VUID-XrInstanceCreateInfoAndroidKHR-applicationVM-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name, objects_info,
                            "XrInstanceCreateInfoAndroidKHR contains invalid NULL for void \"applicationVM\" which is not "
                            "optional and must be non-NULL");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Non-optional pointer/array variable that needs to not be NULL
    if (nullptr == value->applicationActivity) {
        CoreValidLogMessage(instance_info, "VUID-XrInstanceCreateInfoAndroidKHR-applicationActivity-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name, objects_info,
                            "XrInstanceCreateInfoAndroidKHR contains invalid NULL for void \"applicationActivity\" which is not "
                            "optional and must be non-NULL");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

#endif // defined(XR_USE_PLATFORM_ANDROID)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrCompositionLayerDepthInfoKHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrCompositionLayerDepthInfoKHR",
                             value->type, "VUID-XrCompositionLayerDepthInfoKHR-type-type", XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR, "XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerDepthInfoKHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrCompositionLayerDepthInfoKHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrCompositionLayerDepthInfoKHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerDepthInfoKHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrCompositionLayerDepthInfoKHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Validate that the structure XrSwapchainSubImage is valid
    xr_result = ValidateXrStruct(instance_info, command_name, objects_info,
                                                    check_members, &value->subImage);
    if (XR_SUCCESS != xr_result) {
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerDepthInfoKHR-subImage-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrCompositionLayerDepthInfoKHR member subImage is invalid");
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

#if defined(XR_USE_GRAPHICS_API_VULKAN)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrVulkanSwapchainFormatListCreateInfoKHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_VULKAN_SWAPCHAIN_FORMAT_LIST_CREATE_INFO_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrVulkanSwapchainFormatListCreateInfoKHR",
                             value->type, "VUID-XrVulkanSwapchainFormatListCreateInfoKHR-type-type", XR_TYPE_VULKAN_SWAPCHAIN_FORMAT_LIST_CREATE_INFO_KHR, "XR_TYPE_VULKAN_SWAPCHAIN_FORMAT_LIST_CREATE_INFO_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrVulkanSwapchainFormatListCreateInfoKHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrVulkanSwapchainFormatListCreateInfoKHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrVulkanSwapchainFormatListCreateInfoKHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrVulkanSwapchainFormatListCreateInfoKHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrVulkanSwapchainFormatListCreateInfoKHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Optional array must be non-NULL when value->viewFormatCount is non-zero
    if (0 != value->viewFormatCount && nullptr == value->viewFormats) {
        CoreValidLogMessage(instance_info, "VUID-XrVulkanSwapchainFormatListCreateInfoKHR-viewFormats-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrVulkanSwapchainFormatListCreateInfoKHR member viewFormatCount is NULL, but value->viewFormatCount is greater than 0");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // Pointer/array variable with a length variable.  Make sure that
    // if length variable is non-zero that the pointer is not NULL
    if (nullptr == value->viewFormats && 0 != value->viewFormatCount) {
        CoreValidLogMessage(instance_info, "VUID-XrVulkanSwapchainFormatListCreateInfoKHR-viewFormats-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name, objects_info,
                            "XrVulkanSwapchainFormatListCreateInfoKHR contains invalid NULL for VkFormat \"viewFormats\" is which not "
                            "optional since \"viewFormatCount\" is set and must be non-NULL");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // NOTE: Can't validate "VUID-XrVulkanSwapchainFormatListCreateInfoKHR-viewFormats-parameter" type
    // Everything checked out properly
    return xr_result;
}

#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrCompositionLayerCylinderKHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrCompositionLayerCylinderKHR",
                             value->type, "VUID-XrCompositionLayerCylinderKHR-type-type", XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR, "XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerCylinderKHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrCompositionLayerCylinderKHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrCompositionLayerCylinderKHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerCylinderKHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrCompositionLayerCylinderKHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    ValidateXrFlagsResult composition_layer_flags_result = ValidateXrCompositionLayerFlags(value->layerFlags);
    // Valid flags available, so it must be invalid to fail.
    if (VALIDATE_XR_FLAGS_INVALID == composition_layer_flags_result) {
        // Otherwise, flags must be valid.
        std::ostringstream oss_enum;
        oss_enum << "XrCompositionLayerCylinderKHR invalid member XrCompositionLayerFlags \"layerFlags\" flag value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->layerFlags));
        oss_enum <<" contains illegal bit";
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerCylinderKHR-layerFlags-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    {
        // writeValidateInlineHandleValidation
        ValidateXrHandleResult handle_result = VerifyXrSpaceHandle(&value->space);
        if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
            // Not a valid handle or NULL (which is not valid in this case)
            std::ostringstream oss;
            oss << "Invalid XrSpace handle \"space\" ";
            oss << HandleToHexString(value->space);
            CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerCylinderKHR-space-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, oss.str());
            return XR_ERROR_HANDLE_INVALID;
        }
    }
    // Make sure the enum type XrEyeVisibility value is valid
    if (!ValidateXrEnum(instance_info, command_name, "XrCompositionLayerCylinderKHR", "eyeVisibility", objects_info, value->eyeVisibility)) {
        std::ostringstream oss_enum;
        oss_enum << "XrCompositionLayerCylinderKHR contains invalid XrEyeVisibility \"eyeVisibility\" enum value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->eyeVisibility));
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerCylinderKHR-eyeVisibility-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Validate that the structure XrSwapchainSubImage is valid
    xr_result = ValidateXrStruct(instance_info, command_name, objects_info,
                                                    check_members, &value->subImage);
    if (XR_SUCCESS != xr_result) {
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerCylinderKHR-subImage-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrCompositionLayerCylinderKHR member subImage is invalid");
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrCompositionLayerEquirectKHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_COMPOSITION_LAYER_EQUIRECT_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrCompositionLayerEquirectKHR",
                             value->type, "VUID-XrCompositionLayerEquirectKHR-type-type", XR_TYPE_COMPOSITION_LAYER_EQUIRECT_KHR, "XR_TYPE_COMPOSITION_LAYER_EQUIRECT_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerEquirectKHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrCompositionLayerEquirectKHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrCompositionLayerEquirectKHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerEquirectKHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrCompositionLayerEquirectKHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    ValidateXrFlagsResult composition_layer_flags_result = ValidateXrCompositionLayerFlags(value->layerFlags);
    // Valid flags available, so it must be invalid to fail.
    if (VALIDATE_XR_FLAGS_INVALID == composition_layer_flags_result) {
        // Otherwise, flags must be valid.
        std::ostringstream oss_enum;
        oss_enum << "XrCompositionLayerEquirectKHR invalid member XrCompositionLayerFlags \"layerFlags\" flag value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->layerFlags));
        oss_enum <<" contains illegal bit";
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerEquirectKHR-layerFlags-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    {
        // writeValidateInlineHandleValidation
        ValidateXrHandleResult handle_result = VerifyXrSpaceHandle(&value->space);
        if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
            // Not a valid handle or NULL (which is not valid in this case)
            std::ostringstream oss;
            oss << "Invalid XrSpace handle \"space\" ";
            oss << HandleToHexString(value->space);
            CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerEquirectKHR-space-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, oss.str());
            return XR_ERROR_HANDLE_INVALID;
        }
    }
    // Make sure the enum type XrEyeVisibility value is valid
    if (!ValidateXrEnum(instance_info, command_name, "XrCompositionLayerEquirectKHR", "eyeVisibility", objects_info, value->eyeVisibility)) {
        std::ostringstream oss_enum;
        oss_enum << "XrCompositionLayerEquirectKHR contains invalid XrEyeVisibility \"eyeVisibility\" enum value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->eyeVisibility));
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerEquirectKHR-eyeVisibility-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Validate that the structure XrSwapchainSubImage is valid
    xr_result = ValidateXrStruct(instance_info, command_name, objects_info,
                                                    check_members, &value->subImage);
    if (XR_SUCCESS != xr_result) {
        CoreValidLogMessage(instance_info, "VUID-XrCompositionLayerEquirectKHR-subImage-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrCompositionLayerEquirectKHR member subImage is invalid");
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

#if defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_WIN32)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsBindingOpenGLWin32KHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrGraphicsBindingOpenGLWin32KHR",
                             value->type, "VUID-XrGraphicsBindingOpenGLWin32KHR-type-type", XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR, "XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsBindingOpenGLWin32KHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrGraphicsBindingOpenGLWin32KHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrGraphicsBindingOpenGLWin32KHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsBindingOpenGLWin32KHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrGraphicsBindingOpenGLWin32KHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

#endif // defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_WIN32)
#if defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_XLIB)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsBindingOpenGLXlibKHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrGraphicsBindingOpenGLXlibKHR",
                             value->type, "VUID-XrGraphicsBindingOpenGLXlibKHR-type-type", XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR, "XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsBindingOpenGLXlibKHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrGraphicsBindingOpenGLXlibKHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrGraphicsBindingOpenGLXlibKHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsBindingOpenGLXlibKHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrGraphicsBindingOpenGLXlibKHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Non-optional pointer/array variable that needs to not be NULL
    if (nullptr == value->xDisplay) {
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsBindingOpenGLXlibKHR-xDisplay-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name, objects_info,
                            "XrGraphicsBindingOpenGLXlibKHR contains invalid NULL for Display \"xDisplay\" which is not "
                            "optional and must be non-NULL");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // NOTE: Can't validate "VUID-XrGraphicsBindingOpenGLXlibKHR-xDisplay-parameter" type
    // Everything checked out properly
    return xr_result;
}

#endif // defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_XLIB)
#if defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_XCB)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsBindingOpenGLXcbKHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_GRAPHICS_BINDING_OPENGL_XCB_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrGraphicsBindingOpenGLXcbKHR",
                             value->type, "VUID-XrGraphicsBindingOpenGLXcbKHR-type-type", XR_TYPE_GRAPHICS_BINDING_OPENGL_XCB_KHR, "XR_TYPE_GRAPHICS_BINDING_OPENGL_XCB_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsBindingOpenGLXcbKHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrGraphicsBindingOpenGLXcbKHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrGraphicsBindingOpenGLXcbKHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsBindingOpenGLXcbKHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrGraphicsBindingOpenGLXcbKHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

#endif // defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_XCB)
#if defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_WAYLAND)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsBindingOpenGLWaylandKHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrGraphicsBindingOpenGLWaylandKHR",
                             value->type, "VUID-XrGraphicsBindingOpenGLWaylandKHR-type-type", XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR, "XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsBindingOpenGLWaylandKHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrGraphicsBindingOpenGLWaylandKHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrGraphicsBindingOpenGLWaylandKHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsBindingOpenGLWaylandKHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrGraphicsBindingOpenGLWaylandKHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

#endif // defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_WAYLAND)
#if defined(XR_USE_GRAPHICS_API_OPENGL)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSwapchainImageOpenGLKHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrSwapchainImageOpenGLKHR",
                             value->type, "VUID-XrSwapchainImageOpenGLKHR-type-type", XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR, "XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrSwapchainImageOpenGLKHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrSwapchainImageOpenGLKHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrSwapchainImageOpenGLKHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrSwapchainImageOpenGLKHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrSwapchainImageOpenGLKHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

#endif // defined(XR_USE_GRAPHICS_API_OPENGL)
#if defined(XR_USE_GRAPHICS_API_OPENGL)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsRequirementsOpenGLKHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrGraphicsRequirementsOpenGLKHR",
                             value->type, "VUID-XrGraphicsRequirementsOpenGLKHR-type-type", XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR, "XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsRequirementsOpenGLKHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrGraphicsRequirementsOpenGLKHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrGraphicsRequirementsOpenGLKHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsRequirementsOpenGLKHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrGraphicsRequirementsOpenGLKHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

#endif // defined(XR_USE_GRAPHICS_API_OPENGL)
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES) && defined(XR_USE_PLATFORM_ANDROID)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsBindingOpenGLESAndroidKHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrGraphicsBindingOpenGLESAndroidKHR",
                             value->type, "VUID-XrGraphicsBindingOpenGLESAndroidKHR-type-type", XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR, "XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsBindingOpenGLESAndroidKHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrGraphicsBindingOpenGLESAndroidKHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrGraphicsBindingOpenGLESAndroidKHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsBindingOpenGLESAndroidKHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrGraphicsBindingOpenGLESAndroidKHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES) && defined(XR_USE_PLATFORM_ANDROID)
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSwapchainImageOpenGLESKHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrSwapchainImageOpenGLESKHR",
                             value->type, "VUID-XrSwapchainImageOpenGLESKHR-type-type", XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR, "XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrSwapchainImageOpenGLESKHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrSwapchainImageOpenGLESKHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrSwapchainImageOpenGLESKHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrSwapchainImageOpenGLESKHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrSwapchainImageOpenGLESKHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES)
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsRequirementsOpenGLESKHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrGraphicsRequirementsOpenGLESKHR",
                             value->type, "VUID-XrGraphicsRequirementsOpenGLESKHR-type-type", XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR, "XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsRequirementsOpenGLESKHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrGraphicsRequirementsOpenGLESKHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrGraphicsRequirementsOpenGLESKHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsRequirementsOpenGLESKHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrGraphicsRequirementsOpenGLESKHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsBindingVulkanKHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrGraphicsBindingVulkanKHR",
                             value->type, "VUID-XrGraphicsBindingVulkanKHR-type-type", XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR, "XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsBindingVulkanKHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrGraphicsBindingVulkanKHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrGraphicsBindingVulkanKHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsBindingVulkanKHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrGraphicsBindingVulkanKHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSwapchainImageVulkanKHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrSwapchainImageVulkanKHR",
                             value->type, "VUID-XrSwapchainImageVulkanKHR-type-type", XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR, "XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrSwapchainImageVulkanKHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrSwapchainImageVulkanKHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrSwapchainImageVulkanKHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrSwapchainImageVulkanKHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrSwapchainImageVulkanKHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsRequirementsVulkanKHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrGraphicsRequirementsVulkanKHR",
                             value->type, "VUID-XrGraphicsRequirementsVulkanKHR-type-type", XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR, "XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsRequirementsVulkanKHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrGraphicsRequirementsVulkanKHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrGraphicsRequirementsVulkanKHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsRequirementsVulkanKHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrGraphicsRequirementsVulkanKHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_D3D11)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsBindingD3D11KHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_GRAPHICS_BINDING_D3D11_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrGraphicsBindingD3D11KHR",
                             value->type, "VUID-XrGraphicsBindingD3D11KHR-type-type", XR_TYPE_GRAPHICS_BINDING_D3D11_KHR, "XR_TYPE_GRAPHICS_BINDING_D3D11_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsBindingD3D11KHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrGraphicsBindingD3D11KHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrGraphicsBindingD3D11KHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsBindingD3D11KHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrGraphicsBindingD3D11KHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Non-optional pointer/array variable that needs to not be NULL
    if (nullptr == value->device) {
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsBindingD3D11KHR-device-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name, objects_info,
                            "XrGraphicsBindingD3D11KHR contains invalid NULL for ID3D11Device \"device\" which is not "
                            "optional and must be non-NULL");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // NOTE: Can't validate "VUID-XrGraphicsBindingD3D11KHR-device-parameter" type
    // Everything checked out properly
    return xr_result;
}

#endif // defined(XR_USE_GRAPHICS_API_D3D11)
#if defined(XR_USE_GRAPHICS_API_D3D11)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSwapchainImageD3D11KHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrSwapchainImageD3D11KHR",
                             value->type, "VUID-XrSwapchainImageD3D11KHR-type-type", XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR, "XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrSwapchainImageD3D11KHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrSwapchainImageD3D11KHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrSwapchainImageD3D11KHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrSwapchainImageD3D11KHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrSwapchainImageD3D11KHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Non-optional pointer/array variable that needs to not be NULL
    if (nullptr == value->texture) {
        CoreValidLogMessage(instance_info, "VUID-XrSwapchainImageD3D11KHR-texture-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name, objects_info,
                            "XrSwapchainImageD3D11KHR contains invalid NULL for ID3D11Texture2D \"texture\" which is not "
                            "optional and must be non-NULL");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // NOTE: Can't validate "VUID-XrSwapchainImageD3D11KHR-texture-parameter" type
    // Everything checked out properly
    return xr_result;
}

#endif // defined(XR_USE_GRAPHICS_API_D3D11)
#if defined(XR_USE_GRAPHICS_API_D3D11)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsRequirementsD3D11KHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrGraphicsRequirementsD3D11KHR",
                             value->type, "VUID-XrGraphicsRequirementsD3D11KHR-type-type", XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR, "XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsRequirementsD3D11KHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrGraphicsRequirementsD3D11KHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrGraphicsRequirementsD3D11KHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsRequirementsD3D11KHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrGraphicsRequirementsD3D11KHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

#endif // defined(XR_USE_GRAPHICS_API_D3D11)
#if defined(XR_USE_GRAPHICS_API_D3D12)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsBindingD3D12KHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_GRAPHICS_BINDING_D3D12_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrGraphicsBindingD3D12KHR",
                             value->type, "VUID-XrGraphicsBindingD3D12KHR-type-type", XR_TYPE_GRAPHICS_BINDING_D3D12_KHR, "XR_TYPE_GRAPHICS_BINDING_D3D12_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsBindingD3D12KHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrGraphicsBindingD3D12KHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrGraphicsBindingD3D12KHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsBindingD3D12KHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrGraphicsBindingD3D12KHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Non-optional pointer/array variable that needs to not be NULL
    if (nullptr == value->device) {
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsBindingD3D12KHR-device-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name, objects_info,
                            "XrGraphicsBindingD3D12KHR contains invalid NULL for ID3D12Device \"device\" which is not "
                            "optional and must be non-NULL");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // NOTE: Can't validate "VUID-XrGraphicsBindingD3D12KHR-device-parameter" type
    // Non-optional pointer/array variable that needs to not be NULL
    if (nullptr == value->queue) {
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsBindingD3D12KHR-queue-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name, objects_info,
                            "XrGraphicsBindingD3D12KHR contains invalid NULL for ID3D12CommandQueue \"queue\" which is not "
                            "optional and must be non-NULL");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // NOTE: Can't validate "VUID-XrGraphicsBindingD3D12KHR-queue-parameter" type
    // Everything checked out properly
    return xr_result;
}

#endif // defined(XR_USE_GRAPHICS_API_D3D12)
#if defined(XR_USE_GRAPHICS_API_D3D12)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrSwapchainImageD3D12KHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrSwapchainImageD3D12KHR",
                             value->type, "VUID-XrSwapchainImageD3D12KHR-type-type", XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR, "XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrSwapchainImageD3D12KHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrSwapchainImageD3D12KHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrSwapchainImageD3D12KHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrSwapchainImageD3D12KHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrSwapchainImageD3D12KHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Non-optional pointer/array variable that needs to not be NULL
    if (nullptr == value->texture) {
        CoreValidLogMessage(instance_info, "VUID-XrSwapchainImageD3D12KHR-texture-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name, objects_info,
                            "XrSwapchainImageD3D12KHR contains invalid NULL for ID3D12Resource \"texture\" which is not "
                            "optional and must be non-NULL");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // NOTE: Can't validate "VUID-XrSwapchainImageD3D12KHR-texture-parameter" type
    // Everything checked out properly
    return xr_result;
}

#endif // defined(XR_USE_GRAPHICS_API_D3D12)
#if defined(XR_USE_GRAPHICS_API_D3D12)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrGraphicsRequirementsD3D12KHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrGraphicsRequirementsD3D12KHR",
                             value->type, "VUID-XrGraphicsRequirementsD3D12KHR-type-type", XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR, "XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsRequirementsD3D12KHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrGraphicsRequirementsD3D12KHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrGraphicsRequirementsD3D12KHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrGraphicsRequirementsD3D12KHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrGraphicsRequirementsD3D12KHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Everything checked out properly
    return xr_result;
}

#endif // defined(XR_USE_GRAPHICS_API_D3D12)
XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrVisibilityMaskKHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_VISIBILITY_MASK_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrVisibilityMaskKHR",
                             value->type, "VUID-XrVisibilityMaskKHR-type-type", XR_TYPE_VISIBILITY_MASK_KHR, "XR_TYPE_VISIBILITY_MASK_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrVisibilityMaskKHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrVisibilityMaskKHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrVisibilityMaskKHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrVisibilityMaskKHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrVisibilityMaskKHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // NOTE: Can't validate "VUID-XrVisibilityMaskKHR-vertices-parameter" type
    // Optional array must be non-NULL when value->indexCapacityInput is non-zero
    if (0 != value->indexCapacityInput && nullptr == value->indices) {
        CoreValidLogMessage(instance_info, "VUID-XrVisibilityMaskKHR-indices-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrVisibilityMaskKHR member indexCapacityInput is NULL, but value->indexCapacityInput is greater than 0");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // NOTE: Can't validate "VUID-XrVisibilityMaskKHR-indices-parameter" type
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrEventDataVisibilityMaskChangedKHR* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrEventDataVisibilityMaskChangedKHR",
                             value->type, "VUID-XrEventDataVisibilityMaskChangedKHR-type-type", XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR, "XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrEventDataVisibilityMaskChangedKHR-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrEventDataVisibilityMaskChangedKHR struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrEventDataVisibilityMaskChangedKHR : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrEventDataVisibilityMaskChangedKHR-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrEventDataVisibilityMaskChangedKHR struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    {
        // writeValidateInlineHandleValidation
        ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&value->session);
        if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
            // Not a valid handle or NULL (which is not valid in this case)
            std::ostringstream oss;
            oss << "Invalid XrSession handle \"session\" ";
            oss << HandleToHexString(value->session);
            CoreValidLogMessage(instance_info, "VUID-XrEventDataVisibilityMaskChangedKHR-session-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                                objects_info, oss.str());
            return XR_ERROR_HANDLE_INVALID;
        }
    }
    // Make sure the enum type XrViewConfigurationType value is valid
    if (!ValidateXrEnum(instance_info, command_name, "XrEventDataVisibilityMaskChangedKHR", "viewConfigurationType", objects_info, value->viewConfigurationType)) {
        std::ostringstream oss_enum;
        oss_enum << "XrEventDataVisibilityMaskChangedKHR contains invalid XrViewConfigurationType \"viewConfigurationType\" enum value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->viewConfigurationType));
        CoreValidLogMessage(instance_info, "VUID-XrEventDataVisibilityMaskChangedKHR-viewConfigurationType-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrEventDataPerfSettingsEXT* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrEventDataPerfSettingsEXT",
                             value->type, "VUID-XrEventDataPerfSettingsEXT-type-type", XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT, "XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrEventDataPerfSettingsEXT-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrEventDataPerfSettingsEXT struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrEventDataPerfSettingsEXT : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrEventDataPerfSettingsEXT-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrEventDataPerfSettingsEXT struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Make sure the enum type XrPerfSettingsDomainEXT value is valid
    if (!ValidateXrEnum(instance_info, command_name, "XrEventDataPerfSettingsEXT", "domain", objects_info, value->domain)) {
        std::ostringstream oss_enum;
        oss_enum << "XrEventDataPerfSettingsEXT contains invalid XrPerfSettingsDomainEXT \"domain\" enum value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->domain));
        CoreValidLogMessage(instance_info, "VUID-XrEventDataPerfSettingsEXT-domain-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Make sure the enum type XrPerfSettingsSubDomainEXT value is valid
    if (!ValidateXrEnum(instance_info, command_name, "XrEventDataPerfSettingsEXT", "subDomain", objects_info, value->subDomain)) {
        std::ostringstream oss_enum;
        oss_enum << "XrEventDataPerfSettingsEXT contains invalid XrPerfSettingsSubDomainEXT \"subDomain\" enum value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->subDomain));
        CoreValidLogMessage(instance_info, "VUID-XrEventDataPerfSettingsEXT-subDomain-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Make sure the enum type XrPerfSettingsNotificationLevelEXT value is valid
    if (!ValidateXrEnum(instance_info, command_name, "XrEventDataPerfSettingsEXT", "fromLevel", objects_info, value->fromLevel)) {
        std::ostringstream oss_enum;
        oss_enum << "XrEventDataPerfSettingsEXT contains invalid XrPerfSettingsNotificationLevelEXT \"fromLevel\" enum value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->fromLevel));
        CoreValidLogMessage(instance_info, "VUID-XrEventDataPerfSettingsEXT-fromLevel-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Make sure the enum type XrPerfSettingsNotificationLevelEXT value is valid
    if (!ValidateXrEnum(instance_info, command_name, "XrEventDataPerfSettingsEXT", "toLevel", objects_info, value->toLevel)) {
        std::ostringstream oss_enum;
        oss_enum << "XrEventDataPerfSettingsEXT contains invalid XrPerfSettingsNotificationLevelEXT \"toLevel\" enum value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->toLevel));
        CoreValidLogMessage(instance_info, "VUID-XrEventDataPerfSettingsEXT-toLevel-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrDebugUtilsObjectNameInfoEXT* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrDebugUtilsObjectNameInfoEXT",
                             value->type, "VUID-XrDebugUtilsObjectNameInfoEXT-type-type", XR_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, "XR_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrDebugUtilsObjectNameInfoEXT-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrDebugUtilsObjectNameInfoEXT struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrDebugUtilsObjectNameInfoEXT : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrDebugUtilsObjectNameInfoEXT-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrDebugUtilsObjectNameInfoEXT struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Make sure the enum type XrObjectType value is valid
    if (!ValidateXrEnum(instance_info, command_name, "XrDebugUtilsObjectNameInfoEXT", "objectType", objects_info, value->objectType)) {
        std::ostringstream oss_enum;
        oss_enum << "XrDebugUtilsObjectNameInfoEXT contains invalid XrObjectType \"objectType\" enum value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->objectType));
        CoreValidLogMessage(instance_info, "VUID-XrDebugUtilsObjectNameInfoEXT-objectType-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // NOTE: Can't validate "VUID-XrDebugUtilsObjectNameInfoEXT-objectName-parameter" null-termination
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrDebugUtilsLabelEXT* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_DEBUG_UTILS_LABEL_EXT) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrDebugUtilsLabelEXT",
                             value->type, "VUID-XrDebugUtilsLabelEXT-type-type", XR_TYPE_DEBUG_UTILS_LABEL_EXT, "XR_TYPE_DEBUG_UTILS_LABEL_EXT");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrDebugUtilsLabelEXT-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrDebugUtilsLabelEXT struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrDebugUtilsLabelEXT : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrDebugUtilsLabelEXT-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrDebugUtilsLabelEXT struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Non-optional pointer/array variable that needs to not be NULL
    if (nullptr == value->labelName) {
        CoreValidLogMessage(instance_info, "VUID-XrDebugUtilsLabelEXT-labelName-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name, objects_info,
                            "XrDebugUtilsLabelEXT contains invalid NULL for char \"labelName\" which is not "
                            "optional and must be non-NULL");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // NOTE: Can't validate "VUID-XrDebugUtilsLabelEXT-labelName-parameter" null-termination
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrDebugUtilsMessengerCallbackDataEXT* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrDebugUtilsMessengerCallbackDataEXT",
                             value->type, "VUID-XrDebugUtilsMessengerCallbackDataEXT-type-type", XR_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT, "XR_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrDebugUtilsMessengerCallbackDataEXT-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrDebugUtilsMessengerCallbackDataEXT struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrDebugUtilsMessengerCallbackDataEXT : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrDebugUtilsMessengerCallbackDataEXT-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrDebugUtilsMessengerCallbackDataEXT struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    // Non-optional pointer/array variable that needs to not be NULL
    if (nullptr == value->messageId) {
        CoreValidLogMessage(instance_info, "VUID-XrDebugUtilsMessengerCallbackDataEXT-messageId-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name, objects_info,
                            "XrDebugUtilsMessengerCallbackDataEXT contains invalid NULL for char \"messageId\" which is not "
                            "optional and must be non-NULL");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // NOTE: Can't validate "VUID-XrDebugUtilsMessengerCallbackDataEXT-messageId-parameter" null-termination
    // Non-optional pointer/array variable that needs to not be NULL
    if (nullptr == value->functionName) {
        CoreValidLogMessage(instance_info, "VUID-XrDebugUtilsMessengerCallbackDataEXT-functionName-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name, objects_info,
                            "XrDebugUtilsMessengerCallbackDataEXT contains invalid NULL for char \"functionName\" which is not "
                            "optional and must be non-NULL");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // NOTE: Can't validate "VUID-XrDebugUtilsMessengerCallbackDataEXT-functionName-parameter" null-termination
    // Non-optional pointer/array variable that needs to not be NULL
    if (nullptr == value->message) {
        CoreValidLogMessage(instance_info, "VUID-XrDebugUtilsMessengerCallbackDataEXT-message-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name, objects_info,
                            "XrDebugUtilsMessengerCallbackDataEXT contains invalid NULL for char \"message\" which is not "
                            "optional and must be non-NULL");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // NOTE: Can't validate "VUID-XrDebugUtilsMessengerCallbackDataEXT-message-parameter" null-termination
    // Optional array must be non-NULL when value->sessionLabelCount is non-zero
    if (0 != value->sessionLabelCount && nullptr == value->sessionLabels) {
        CoreValidLogMessage(instance_info, "VUID-XrDebugUtilsMessengerCallbackDataEXT-sessionLabels-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
                            "Structure XrDebugUtilsMessengerCallbackDataEXT member sessionLabelCount is NULL, but value->sessionLabelCount is greater than 0");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // Everything checked out properly
    return xr_result;
}

XrResult ValidateXrStruct(GenValidUsageXrInstanceInfo *instance_info, const std::string &command_name,
                          std::vector<GenValidUsageXrObjectInfo>& objects_info, bool check_members,
                          const XrDebugUtilsMessengerCreateInfoEXT* value) {
    XrResult xr_result = XR_SUCCESS;
    // Make sure the structure type is correct
    if (value->type != XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT) {
        InvalidStructureType(instance_info, command_name, objects_info, "XrDebugUtilsMessengerCreateInfoEXT",
                             value->type, "VUID-XrDebugUtilsMessengerCreateInfoEXT-type-type", XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT, "XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    std::vector<XrStructureType> valid_ext_structs;
    std::vector<XrStructureType> duplicate_ext_structs;
    std::vector<XrStructureType> encountered_structs;
    NextChainResult next_result = ValidateNextChain(instance_info, command_name, objects_info,
                                                     value->next, valid_ext_structs,
                                                     encountered_structs,
                                                     duplicate_ext_structs);
    // No valid extension structs for this 'next'.  Therefore, must be NULL
    // or only contain a list of valid extension structures.
    if (NEXT_CHAIN_RESULT_ERROR == next_result) {
        CoreValidLogMessage(instance_info, "VUID-XrDebugUtilsMessengerCreateInfoEXT-next-next",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "Invalid structure(s) in \"next\" chain for XrDebugUtilsMessengerCreateInfoEXT struct \"next\"");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    } else if (NEXT_CHAIN_RESULT_DUPLICATE_STRUCT == next_result) {
        char struct_type_buffer[XR_MAX_STRUCTURE_NAME_SIZE];
        std::string error_message = "Multiple structures of the same type(s) in \"next\" chain for ";
        error_message += "XrDebugUtilsMessengerCreateInfoEXT : ";
        if (nullptr != instance_info) {
            bool wrote_struct = false;
            for (uint32_t dup = 0; dup < duplicate_ext_structs.size(); ++dup) {
                if (XR_SUCCESS == instance_info->dispatch_table->StructureTypeToString(instance_info->instance,
                                                                                       duplicate_ext_structs[dup],
                                                                                       struct_type_buffer)) {
                    if (wrote_struct) {
                        error_message += ", ";
                    } else {
                        wrote_struct = true;
                    }
                    error_message += struct_type_buffer;
                }
            }
        }
        CoreValidLogMessage(instance_info, "VUID-XrDebugUtilsMessengerCreateInfoEXT-next-unique",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info,
        "Multiple structures of the same type(s) in \"next\" chain for XrDebugUtilsMessengerCreateInfoEXT struct");
        xr_result = XR_ERROR_VALIDATION_FAILURE;
    }
    // If we are not to check the rest of the members, just return here.
    if (!check_members || XR_SUCCESS != xr_result) {
        return xr_result;
    }
    ValidateXrFlagsResult debug_utils_message_severity_flags_ext_result = ValidateXrDebugUtilsMessageSeverityFlagsEXT(value->messageSeverities);
    // Flags must be non-zero in this case.
    if (VALIDATE_XR_FLAGS_ZERO == debug_utils_message_severity_flags_ext_result) {
        CoreValidLogMessage(instance_info, "VUID-XrDebugUtilsMessengerCreateInfoEXT-messageSeverities-requiredbitmask",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "XrDebugUtilsMessageSeverityFlagsEXT \"messageSeverities\" flag must be non-zero");
        return XR_ERROR_VALIDATION_FAILURE;
    } else if (VALIDATE_XR_FLAGS_SUCCESS != debug_utils_message_severity_flags_ext_result) {
        // Otherwise, flags must be valid.
        std::ostringstream oss_enum;
        oss_enum << "XrDebugUtilsMessengerCreateInfoEXT invalid member XrDebugUtilsMessageSeverityFlagsEXT \"messageSeverities\" flag value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->messageSeverities));
        oss_enum <<" contains illegal bit";
        CoreValidLogMessage(instance_info, "VUID-XrDebugUtilsMessengerCreateInfoEXT-messageSeverities-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    ValidateXrFlagsResult debug_utils_message_type_flags_ext_result = ValidateXrDebugUtilsMessageTypeFlagsEXT(value->messageTypes);
    // Flags must be non-zero in this case.
    if (VALIDATE_XR_FLAGS_ZERO == debug_utils_message_type_flags_ext_result) {
        CoreValidLogMessage(instance_info, "VUID-XrDebugUtilsMessengerCreateInfoEXT-messageTypes-requiredbitmask",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, "XrDebugUtilsMessageTypeFlagsEXT \"messageTypes\" flag must be non-zero");
        return XR_ERROR_VALIDATION_FAILURE;
    } else if (VALIDATE_XR_FLAGS_SUCCESS != debug_utils_message_type_flags_ext_result) {
        // Otherwise, flags must be valid.
        std::ostringstream oss_enum;
        oss_enum << "XrDebugUtilsMessengerCreateInfoEXT invalid member XrDebugUtilsMessageTypeFlagsEXT \"messageTypes\" flag value ";
        oss_enum << Uint32ToHexString(static_cast<uint32_t>(value->messageTypes));
        oss_enum <<" contains illegal bit";
        CoreValidLogMessage(instance_info, "VUID-XrDebugUtilsMessengerCreateInfoEXT-messageTypes-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name,
                            objects_info, oss_enum.str());
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Non-optional pointer/array variable that needs to not be NULL
    if (nullptr == value->userCallback) {
        CoreValidLogMessage(instance_info, "VUID-XrDebugUtilsMessengerCreateInfoEXT-userCallback-parameter",
                            VALID_USAGE_DEBUG_SEVERITY_ERROR, command_name, objects_info,
                            "XrDebugUtilsMessengerCreateInfoEXT contains invalid NULL for PFN_xrDebugUtilsMessengerCallbackEXT \"userCallback\" which is not "
                            "optional and must be non-NULL");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // NOTE: Can't validate "VUID-XrDebugUtilsMessengerCreateInfoEXT-userCallback-parameter" type
    // Everything checked out properly
    return xr_result;
}


NextChainResult ValidateNextChain(GenValidUsageXrInstanceInfo *instance_info,
                                  const std::string &command_name,
                                  std::vector<GenValidUsageXrObjectInfo>& objects_info,
                                  const void* next,
                                  std::vector<XrStructureType>& valid_ext_structs,
                                  std::vector<XrStructureType>& encountered_structs,
                                  std::vector<XrStructureType>& duplicate_structs) {
    NextChainResult return_result = NEXT_CHAIN_RESULT_VALID;
    // NULL is valid
    if (nullptr == next) {
        return return_result;
    }
    // Non-NULL is not valid if there is no valid extension structs
    if (nullptr != next && 0 == valid_ext_structs.size()) {
        return NEXT_CHAIN_RESULT_ERROR;
    }
    const XrBaseInStructure* next_header = reinterpret_cast<const XrBaseInStructure*>(next);
    auto valid_ext = std::find(valid_ext_structs.begin(), valid_ext_structs.end(), next_header->type);
    if (valid_ext == valid_ext_structs.end()) {
        // Not a valid extension structure type for this next chain.
        return NEXT_CHAIN_RESULT_ERROR;
    } else {
        // Check to see if we've already encountered this structure.
        auto already_encountered_ext = std::find(encountered_structs.begin(), encountered_structs.end(), next_header->type);
        if (already_encountered_ext != encountered_structs.end()) {
            // Make sure we only put in unique types into our duplicate list.
            auto already_duplicate = std::find(duplicate_structs.begin(), duplicate_structs.end(), next_header->type);
            if (already_duplicate == duplicate_structs.end()) {
                duplicate_structs.push_back(next_header->type);
            }
            return_result = NEXT_CHAIN_RESULT_DUPLICATE_STRUCT;
        }
    }
    switch (next_header->type) {
        case XR_TYPE_API_LAYER_PROPERTIES:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrApiLayerProperties*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_EXTENSION_PROPERTIES:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrExtensionProperties*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_INSTANCE_CREATE_INFO:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrInstanceCreateInfo*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_SYSTEM_GET_INFO:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrSystemGetInfo*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_SYSTEM_PROPERTIES:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrSystemProperties*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_VIEW_LOCATE_INFO:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrViewLocateInfo*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_VIEW:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrView*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_SESSION_CREATE_INFO:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrSessionCreateInfo*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_SWAPCHAIN_CREATE_INFO:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrSwapchainCreateInfo*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_SESSION_BEGIN_INFO:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrSessionBeginInfo*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_VIEW_STATE:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrViewState*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_FRAME_END_INFO:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrFrameEndInfo*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_HAPTIC_VIBRATION:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrHapticVibration*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_EVENT_DATA_BUFFER:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrEventDataBuffer*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrEventDataInstanceLossPending*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrEventDataSessionStateChanged*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_ACTION_STATE_BOOLEAN:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrActionStateBoolean*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_ACTION_STATE_FLOAT:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrActionStateFloat*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_ACTION_STATE_VECTOR2F:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrActionStateVector2f*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_ACTION_STATE_POSE:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrActionStatePose*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_ACTION_SET_CREATE_INFO:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrActionSetCreateInfo*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_ACTION_CREATE_INFO:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrActionCreateInfo*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_INSTANCE_PROPERTIES:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrInstanceProperties*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_FRAME_WAIT_INFO:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrFrameWaitInfo*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_COMPOSITION_LAYER_PROJECTION:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrCompositionLayerProjection*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_COMPOSITION_LAYER_QUAD:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrCompositionLayerQuad*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_REFERENCE_SPACE_CREATE_INFO:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrReferenceSpaceCreateInfo*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_ACTION_SPACE_CREATE_INFO:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrActionSpaceCreateInfo*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrEventDataReferenceSpaceChangePending*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_VIEW_CONFIGURATION_VIEW:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrViewConfigurationView*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_SPACE_LOCATION:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrSpaceLocation*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_SPACE_VELOCITY:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrSpaceVelocity*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_FRAME_STATE:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrFrameState*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_VIEW_CONFIGURATION_PROPERTIES:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrViewConfigurationProperties*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_FRAME_BEGIN_INFO:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrFrameBeginInfo*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrCompositionLayerProjectionView*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_EVENT_DATA_EVENTS_LOST:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrEventDataEventsLost*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrInteractionProfileSuggestedBinding*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrEventDataInteractionProfileChanged*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_INTERACTION_PROFILE_STATE:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrInteractionProfileState*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrSwapchainImageAcquireInfo*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrSwapchainImageWaitInfo*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrSwapchainImageReleaseInfo*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_ACTION_STATE_GET_INFO:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrActionStateGetInfo*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_HAPTIC_ACTION_INFO:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrHapticActionInfo*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrSessionActionSetsAttachInfo*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_ACTIONS_SYNC_INFO:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrActionsSyncInfo*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrBoundSourcesForActionEnumerateInfo*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrInputSourceLocalizedNameGetInfo*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_COMPOSITION_LAYER_CUBE_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrCompositionLayerCubeKHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
#if defined(XR_USE_PLATFORM_ANDROID)
        case XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrInstanceCreateInfoAndroidKHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
#endif // defined(XR_USE_PLATFORM_ANDROID)
        case XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrCompositionLayerDepthInfoKHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
#if defined(XR_USE_GRAPHICS_API_VULKAN)
        case XR_TYPE_VULKAN_SWAPCHAIN_FORMAT_LIST_CREATE_INFO_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrVulkanSwapchainFormatListCreateInfoKHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
        case XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrEventDataPerfSettingsEXT*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrCompositionLayerCylinderKHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_COMPOSITION_LAYER_EQUIRECT_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrCompositionLayerEquirectKHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrDebugUtilsObjectNameInfoEXT*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrDebugUtilsMessengerCallbackDataEXT*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrDebugUtilsMessengerCreateInfoEXT*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_DEBUG_UTILS_LABEL_EXT:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrDebugUtilsLabelEXT*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
#if defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_WIN32)
        case XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrGraphicsBindingOpenGLWin32KHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
#endif // defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_WIN32)
#if defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_XLIB)
        case XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrGraphicsBindingOpenGLXlibKHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
#endif // defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_XLIB)
#if defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_XCB)
        case XR_TYPE_GRAPHICS_BINDING_OPENGL_XCB_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrGraphicsBindingOpenGLXcbKHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
#endif // defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_XCB)
#if defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_WAYLAND)
        case XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrGraphicsBindingOpenGLWaylandKHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
#endif // defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_WAYLAND)
#if defined(XR_USE_GRAPHICS_API_OPENGL)
        case XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrSwapchainImageOpenGLKHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
#endif // defined(XR_USE_GRAPHICS_API_OPENGL)
#if defined(XR_USE_GRAPHICS_API_OPENGL)
        case XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrGraphicsRequirementsOpenGLKHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
#endif // defined(XR_USE_GRAPHICS_API_OPENGL)
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES) && defined(XR_USE_PLATFORM_ANDROID)
        case XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrGraphicsBindingOpenGLESAndroidKHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES) && defined(XR_USE_PLATFORM_ANDROID)
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
        case XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrSwapchainImageOpenGLESKHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES)
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
        case XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrGraphicsRequirementsOpenGLESKHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
        case XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrGraphicsBindingVulkanKHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
        case XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrSwapchainImageVulkanKHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
        case XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrGraphicsRequirementsVulkanKHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_D3D11)
        case XR_TYPE_GRAPHICS_BINDING_D3D11_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrGraphicsBindingD3D11KHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
#endif // defined(XR_USE_GRAPHICS_API_D3D11)
#if defined(XR_USE_GRAPHICS_API_D3D11)
        case XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrSwapchainImageD3D11KHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
#endif // defined(XR_USE_GRAPHICS_API_D3D11)
#if defined(XR_USE_GRAPHICS_API_D3D11)
        case XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrGraphicsRequirementsD3D11KHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
#endif // defined(XR_USE_GRAPHICS_API_D3D11)
#if defined(XR_USE_GRAPHICS_API_D3D12)
        case XR_TYPE_GRAPHICS_BINDING_D3D12_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrGraphicsBindingD3D12KHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
#endif // defined(XR_USE_GRAPHICS_API_D3D12)
#if defined(XR_USE_GRAPHICS_API_D3D12)
        case XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrSwapchainImageD3D12KHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
#endif // defined(XR_USE_GRAPHICS_API_D3D12)
#if defined(XR_USE_GRAPHICS_API_D3D12)
        case XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrGraphicsRequirementsD3D12KHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
#endif // defined(XR_USE_GRAPHICS_API_D3D12)
        case XR_TYPE_VISIBILITY_MASK_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrVisibilityMaskKHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        case XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR:
            if (XR_SUCCESS != ValidateXrStruct(instance_info, command_name, objects_info, false,
                                               reinterpret_cast<const XrEventDataVisibilityMaskChangedKHR*>(next))) {
                return NEXT_CHAIN_RESULT_ERROR;
            }
            break;
        default:
            return NEXT_CHAIN_RESULT_ERROR;
    }
    NextChainResult next_result = ValidateNextChain(instance_info, command_name,
                                                    objects_info, next_header->next,
                                                    valid_ext_structs,
                                                    encountered_structs,
                                                    duplicate_structs);
    if (NEXT_CHAIN_RESULT_VALID == next_result && NEXT_CHAIN_RESULT_VALID != return_result) {
        return return_result;
    } else {
        return next_result;
    }
}


// ---- Core 1.0 commands
XrResult GenValidUsageInputsXrCreateInstance(
const XrInstanceCreateInfo* createInfo,
XrInstance* instance) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == createInfo) {
            CoreValidLogMessage(nullptr, "VUID-xrCreateInstance-createInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateInstance", objects_info,
                                "Invalid NULL for XrInstanceCreateInfo \"createInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrInstanceCreateInfo is valid
        xr_result = ValidateXrStruct(nullptr, "xrCreateInstance", objects_info,
                                                        true, createInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(nullptr, "VUID-xrCreateInstance-createInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateInstance",
                                objects_info,
                                "Command xrCreateInstance param createInfo is invalid");
            return xr_result;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == instance) {
            CoreValidLogMessage(nullptr, "VUID-xrCreateInstance-instance-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateInstance", objects_info,
                                "Invalid NULL for XrInstance \"instance\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageInputsXrDestroyInstance(
XrInstance instance) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrDestroyInstance-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrDestroyInstance",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrDestroyInstance(
    XrInstance instance) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->DestroyInstance(instance);
        if (XR_SUCCEEDED(result)) {
            g_instance_info.erase(instance);
        }
        GenValidUsageCleanUpMaps(gen_instance_info);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageInputsXrGetInstanceProperties(
XrInstance instance,
XrInstanceProperties* instanceProperties) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrGetInstanceProperties-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetInstanceProperties",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == instanceProperties) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetInstanceProperties-instanceProperties-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetInstanceProperties", objects_info,
                                "Invalid NULL for XrInstanceProperties \"instanceProperties\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrInstanceProperties is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrGetInstanceProperties", objects_info,
                                                        false, instanceProperties);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetInstanceProperties-instanceProperties-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetInstanceProperties",
                                objects_info,
                                "Command xrGetInstanceProperties param instanceProperties is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrGetInstanceProperties(
    XrInstance instance,
    XrInstanceProperties* instanceProperties) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->GetInstanceProperties(instance, instanceProperties);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrGetInstanceProperties(
    XrInstance instance,
    XrInstanceProperties* instanceProperties) {
    XrResult test_result = GenValidUsageInputsXrGetInstanceProperties(instance, instanceProperties);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrGetInstanceProperties(instance, instanceProperties);
}

XrResult GenValidUsageInputsXrPollEvent(
XrInstance instance,
XrEventDataBuffer* eventData) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrPollEvent-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrPollEvent",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == eventData) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrPollEvent-eventData-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrPollEvent", objects_info,
                                "Invalid NULL for XrEventDataBuffer \"eventData\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrEventDataBuffer is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrPollEvent", objects_info,
                                                        false, eventData);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrPollEvent-eventData-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrPollEvent",
                                objects_info,
                                "Command xrPollEvent param eventData is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrPollEvent(
    XrInstance instance,
    XrEventDataBuffer* eventData) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->PollEvent(instance, eventData);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrPollEvent(
    XrInstance instance,
    XrEventDataBuffer* eventData) {
    XrResult test_result = GenValidUsageInputsXrPollEvent(instance, eventData);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrPollEvent(instance, eventData);
}

XrResult GenValidUsageInputsXrResultToString(
XrInstance instance,
XrResult value,
char buffer[XR_MAX_RESULT_STRING_SIZE]) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrResultToString-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrResultToString",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Make sure the enum type XrResult value is valid
        if (!ValidateXrEnum(gen_instance_info, "xrResultToString", "xrResultToString", "value", objects_info, value)) {
            std::ostringstream oss_enum;
            oss_enum << "Invalid XrResult \"value\" enum value ";
            oss_enum << Uint32ToHexString(static_cast<uint32_t>(value));
            CoreValidLogMessage(gen_instance_info, "VUID-xrResultToString-value-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrResultToString",
                                objects_info, oss_enum.str());
            return XR_ERROR_VALIDATION_FAILURE;
        }
        if (XR_MAX_RESULT_STRING_SIZE < std::strlen(buffer)) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrResultToString-buffer-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrResultToString",
                                objects_info,
                                "Command xrResultToString param buffer length is too long.");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrResultToString(
    XrInstance instance,
    XrResult value,
    char buffer[XR_MAX_RESULT_STRING_SIZE]) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->ResultToString(instance, value, buffer);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrResultToString(
    XrInstance instance,
    XrResult value,
    char buffer[XR_MAX_RESULT_STRING_SIZE]) {
    XrResult test_result = GenValidUsageInputsXrResultToString(instance, value, buffer);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrResultToString(instance, value, buffer);
}

XrResult GenValidUsageInputsXrStructureTypeToString(
XrInstance instance,
XrStructureType value,
char buffer[XR_MAX_STRUCTURE_NAME_SIZE]) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrStructureTypeToString-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrStructureTypeToString",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Make sure the enum type XrStructureType value is valid
        if (!ValidateXrEnum(gen_instance_info, "xrStructureTypeToString", "xrStructureTypeToString", "value", objects_info, value)) {
            std::ostringstream oss_enum;
            oss_enum << "Invalid XrStructureType \"value\" enum value ";
            oss_enum << Uint32ToHexString(static_cast<uint32_t>(value));
            CoreValidLogMessage(gen_instance_info, "VUID-xrStructureTypeToString-value-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrStructureTypeToString",
                                objects_info, oss_enum.str());
            return XR_ERROR_VALIDATION_FAILURE;
        }
        if (XR_MAX_STRUCTURE_NAME_SIZE < std::strlen(buffer)) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrStructureTypeToString-buffer-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrStructureTypeToString",
                                objects_info,
                                "Command xrStructureTypeToString param buffer length is too long.");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrStructureTypeToString(
    XrInstance instance,
    XrStructureType value,
    char buffer[XR_MAX_STRUCTURE_NAME_SIZE]) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->StructureTypeToString(instance, value, buffer);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrStructureTypeToString(
    XrInstance instance,
    XrStructureType value,
    char buffer[XR_MAX_STRUCTURE_NAME_SIZE]) {
    XrResult test_result = GenValidUsageInputsXrStructureTypeToString(instance, value, buffer);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrStructureTypeToString(instance, value, buffer);
}

XrResult GenValidUsageInputsXrGetSystem(
XrInstance instance,
const XrSystemGetInfo* getInfo,
XrSystemId* systemId) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrGetSystem-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetSystem",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == getInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetSystem-getInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetSystem", objects_info,
                                "Invalid NULL for XrSystemGetInfo \"getInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrSystemGetInfo is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrGetSystem", objects_info,
                                                        true, getInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetSystem-getInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetSystem",
                                objects_info,
                                "Command xrGetSystem param getInfo is invalid");
            return xr_result;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == systemId) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetSystem-systemId-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetSystem", objects_info,
                                "Invalid NULL for XrSystemId \"systemId\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrGetSystem-systemId-parameter" type
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrGetSystem(
    XrInstance instance,
    const XrSystemGetInfo* getInfo,
    XrSystemId* systemId) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->GetSystem(instance, getInfo, systemId);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrGetSystem(
    XrInstance instance,
    const XrSystemGetInfo* getInfo,
    XrSystemId* systemId) {
    XrResult test_result = GenValidUsageInputsXrGetSystem(instance, getInfo, systemId);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrGetSystem(instance, getInfo, systemId);
}

XrResult GenValidUsageInputsXrGetSystemProperties(
XrInstance instance,
XrSystemId systemId,
XrSystemProperties* properties) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrGetSystemProperties-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetSystemProperties",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == properties) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetSystemProperties-properties-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetSystemProperties", objects_info,
                                "Invalid NULL for XrSystemProperties \"properties\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrSystemProperties is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrGetSystemProperties", objects_info,
                                                        false, properties);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetSystemProperties-properties-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetSystemProperties",
                                objects_info,
                                "Command xrGetSystemProperties param properties is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrGetSystemProperties(
    XrInstance instance,
    XrSystemId systemId,
    XrSystemProperties* properties) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->GetSystemProperties(instance, systemId, properties);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrGetSystemProperties(
    XrInstance instance,
    XrSystemId systemId,
    XrSystemProperties* properties) {
    XrResult test_result = GenValidUsageInputsXrGetSystemProperties(instance, systemId, properties);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrGetSystemProperties(instance, systemId, properties);
}

XrResult GenValidUsageInputsXrEnumerateEnvironmentBlendModes(
XrInstance instance,
XrSystemId systemId,
XrViewConfigurationType viewConfigurationType,
uint32_t environmentBlendModeCapacityInput,
uint32_t* environmentBlendModeCountOutput,
XrEnvironmentBlendMode* environmentBlendModes) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrEnumerateEnvironmentBlendModes-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateEnvironmentBlendModes",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Make sure the enum type XrViewConfigurationType value is valid
        if (!ValidateXrEnum(gen_instance_info, "xrEnumerateEnvironmentBlendModes", "xrEnumerateEnvironmentBlendModes", "viewConfigurationType", objects_info, viewConfigurationType)) {
            std::ostringstream oss_enum;
            oss_enum << "Invalid XrViewConfigurationType \"viewConfigurationType\" enum value ";
            oss_enum << Uint32ToHexString(static_cast<uint32_t>(viewConfigurationType));
            CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateEnvironmentBlendModes-viewConfigurationType-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateEnvironmentBlendModes",
                                objects_info, oss_enum.str());
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Optional array must be non-NULL when environmentBlendModeCapacityInput is non-zero
        if (0 != environmentBlendModeCapacityInput && nullptr == environmentBlendModes) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateEnvironmentBlendModes-environmentBlendModes-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateEnvironmentBlendModes",
                                objects_info,
                                "Command xrEnumerateEnvironmentBlendModes param environmentBlendModes is NULL, but environmentBlendModeCapacityInput is greater than 0");
            xr_result = XR_ERROR_VALIDATION_FAILURE;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == environmentBlendModeCountOutput) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateEnvironmentBlendModes-environmentBlendModeCountOutput-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateEnvironmentBlendModes", objects_info,
                                "Invalid NULL for uint32_t \"environmentBlendModeCountOutput\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrEnumerateEnvironmentBlendModes-environmentBlendModeCountOutput-parameter" type
        for (uint32_t value_environmentblendmodes_inc = 0; value_environmentblendmodes_inc < environmentBlendModeCapacityInput; ++value_environmentblendmodes_inc) {
            // Make sure the enum type XrEnvironmentBlendMode value is valid
            if (!ValidateXrEnum(gen_instance_info, "xrEnumerateEnvironmentBlendModes", "xrEnumerateEnvironmentBlendModes", "environmentBlendModes", objects_info, environmentBlendModes[value_environmentblendmodes_inc])) {
                std::ostringstream oss_enum;
                oss_enum << "Invalid XrEnvironmentBlendMode \"environmentBlendModes\" enum value ";
                oss_enum << Uint32ToHexString(static_cast<uint32_t>(environmentBlendModes[value_environmentblendmodes_inc]));
                CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateEnvironmentBlendModes-environmentBlendModes-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateEnvironmentBlendModes",
                                    objects_info, oss_enum.str());
                return XR_ERROR_VALIDATION_FAILURE;
            }
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrEnumerateEnvironmentBlendModes(
    XrInstance instance,
    XrSystemId systemId,
    XrViewConfigurationType viewConfigurationType,
    uint32_t environmentBlendModeCapacityInput,
    uint32_t* environmentBlendModeCountOutput,
    XrEnvironmentBlendMode* environmentBlendModes) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->EnumerateEnvironmentBlendModes(instance, systemId, viewConfigurationType, environmentBlendModeCapacityInput, environmentBlendModeCountOutput, environmentBlendModes);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrEnumerateEnvironmentBlendModes(
    XrInstance instance,
    XrSystemId systemId,
    XrViewConfigurationType viewConfigurationType,
    uint32_t environmentBlendModeCapacityInput,
    uint32_t* environmentBlendModeCountOutput,
    XrEnvironmentBlendMode* environmentBlendModes) {
    XrResult test_result = GenValidUsageInputsXrEnumerateEnvironmentBlendModes(instance, systemId, viewConfigurationType, environmentBlendModeCapacityInput, environmentBlendModeCountOutput, environmentBlendModes);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrEnumerateEnvironmentBlendModes(instance, systemId, viewConfigurationType, environmentBlendModeCapacityInput, environmentBlendModeCountOutput, environmentBlendModes);
}

XrResult GenValidUsageInputsXrCreateSession(
XrInstance instance,
const XrSessionCreateInfo* createInfo,
XrSession* session) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrCreateSession-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateSession",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == createInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateSession-createInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateSession", objects_info,
                                "Invalid NULL for XrSessionCreateInfo \"createInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrSessionCreateInfo is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrCreateSession", objects_info,
                                                        true, createInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateSession-createInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateSession",
                                objects_info,
                                "Command xrCreateSession param createInfo is invalid");
            return xr_result;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == session) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateSession-session-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateSession", objects_info,
                                "Invalid NULL for XrSession \"session\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrCreateSession(
    XrInstance instance,
    const XrSessionCreateInfo* createInfo,
    XrSession* session) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->CreateSession(instance, createInfo, session);
        if (XR_SUCCESS == result && nullptr != session) {
            std::unique_ptr<GenValidUsageXrHandleInfo> handle_info(new GenValidUsageXrHandleInfo());
            handle_info->instance_info = gen_instance_info;
            handle_info->direct_parent_type = XR_OBJECT_TYPE_INSTANCE;
            handle_info->direct_parent_handle = MakeHandleGeneric(instance);
            g_session_info.insert(*session, std::move(handle_info));
        }
    } catch (std::bad_alloc&) {
        result = XR_ERROR_OUT_OF_MEMORY;
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageInputsXrDestroySession(
XrSession session) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrDestroySession-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrDestroySession",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrDestroySession(
    XrSession session) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->DestroySession(session);

        // Clean up any labels associated with this session
        CoreValidationDeleteSessionLabels(session);

        if (XR_SUCCEEDED(result)) {
            g_session_info.erase(session);
        }
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrDestroySession(
    XrSession session) {
    XrResult test_result = GenValidUsageInputsXrDestroySession(session);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrDestroySession(session);
}

XrResult GenValidUsageInputsXrEnumerateReferenceSpaces(
XrSession session,
uint32_t spaceCapacityInput,
uint32_t* spaceCountOutput,
XrReferenceSpaceType* spaces) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrEnumerateReferenceSpaces-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateReferenceSpaces",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Optional array must be non-NULL when spaceCapacityInput is non-zero
        if (0 != spaceCapacityInput && nullptr == spaces) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateReferenceSpaces-spaces-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateReferenceSpaces",
                                objects_info,
                                "Command xrEnumerateReferenceSpaces param spaces is NULL, but spaceCapacityInput is greater than 0");
            xr_result = XR_ERROR_VALIDATION_FAILURE;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == spaceCountOutput) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateReferenceSpaces-spaceCountOutput-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateReferenceSpaces", objects_info,
                                "Invalid NULL for uint32_t \"spaceCountOutput\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrEnumerateReferenceSpaces-spaceCountOutput-parameter" type
        for (uint32_t value_spaces_inc = 0; value_spaces_inc < spaceCapacityInput; ++value_spaces_inc) {
            // Make sure the enum type XrReferenceSpaceType value is valid
            if (!ValidateXrEnum(gen_instance_info, "xrEnumerateReferenceSpaces", "xrEnumerateReferenceSpaces", "spaces", objects_info, spaces[value_spaces_inc])) {
                std::ostringstream oss_enum;
                oss_enum << "Invalid XrReferenceSpaceType \"spaces\" enum value ";
                oss_enum << Uint32ToHexString(static_cast<uint32_t>(spaces[value_spaces_inc]));
                CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateReferenceSpaces-spaces-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateReferenceSpaces",
                                    objects_info, oss_enum.str());
                return XR_ERROR_VALIDATION_FAILURE;
            }
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrEnumerateReferenceSpaces(
    XrSession session,
    uint32_t spaceCapacityInput,
    uint32_t* spaceCountOutput,
    XrReferenceSpaceType* spaces) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->EnumerateReferenceSpaces(session, spaceCapacityInput, spaceCountOutput, spaces);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrEnumerateReferenceSpaces(
    XrSession session,
    uint32_t spaceCapacityInput,
    uint32_t* spaceCountOutput,
    XrReferenceSpaceType* spaces) {
    XrResult test_result = GenValidUsageInputsXrEnumerateReferenceSpaces(session, spaceCapacityInput, spaceCountOutput, spaces);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrEnumerateReferenceSpaces(session, spaceCapacityInput, spaceCountOutput, spaces);
}

XrResult GenValidUsageInputsXrCreateReferenceSpace(
XrSession session,
const XrReferenceSpaceCreateInfo* createInfo,
XrSpace* space) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrCreateReferenceSpace-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateReferenceSpace",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == createInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateReferenceSpace-createInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateReferenceSpace", objects_info,
                                "Invalid NULL for XrReferenceSpaceCreateInfo \"createInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrReferenceSpaceCreateInfo is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrCreateReferenceSpace", objects_info,
                                                        true, createInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateReferenceSpace-createInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateReferenceSpace",
                                objects_info,
                                "Command xrCreateReferenceSpace param createInfo is invalid");
            return xr_result;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == space) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateReferenceSpace-space-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateReferenceSpace", objects_info,
                                "Invalid NULL for XrSpace \"space\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrCreateReferenceSpace(
    XrSession session,
    const XrReferenceSpaceCreateInfo* createInfo,
    XrSpace* space) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->CreateReferenceSpace(session, createInfo, space);
        if (XR_SUCCESS == result && nullptr != space) {
            std::unique_ptr<GenValidUsageXrHandleInfo> handle_info(new GenValidUsageXrHandleInfo());
            handle_info->instance_info = gen_instance_info;
            handle_info->direct_parent_type = XR_OBJECT_TYPE_SESSION;
            handle_info->direct_parent_handle = MakeHandleGeneric(session);
            g_space_info.insert(*space, std::move(handle_info));
        }
    } catch (std::bad_alloc&) {
        result = XR_ERROR_OUT_OF_MEMORY;
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrCreateReferenceSpace(
    XrSession session,
    const XrReferenceSpaceCreateInfo* createInfo,
    XrSpace* space) {
    XrResult test_result = GenValidUsageInputsXrCreateReferenceSpace(session, createInfo, space);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrCreateReferenceSpace(session, createInfo, space);
}

XrResult GenValidUsageInputsXrGetReferenceSpaceBoundsRect(
XrSession session,
XrReferenceSpaceType referenceSpaceType,
XrExtent2Df* bounds) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrGetReferenceSpaceBoundsRect-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetReferenceSpaceBoundsRect",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Make sure the enum type XrReferenceSpaceType value is valid
        if (!ValidateXrEnum(gen_instance_info, "xrGetReferenceSpaceBoundsRect", "xrGetReferenceSpaceBoundsRect", "referenceSpaceType", objects_info, referenceSpaceType)) {
            std::ostringstream oss_enum;
            oss_enum << "Invalid XrReferenceSpaceType \"referenceSpaceType\" enum value ";
            oss_enum << Uint32ToHexString(static_cast<uint32_t>(referenceSpaceType));
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetReferenceSpaceBoundsRect-referenceSpaceType-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetReferenceSpaceBoundsRect",
                                objects_info, oss_enum.str());
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == bounds) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetReferenceSpaceBoundsRect-bounds-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetReferenceSpaceBoundsRect", objects_info,
                                "Invalid NULL for XrExtent2Df \"bounds\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrGetReferenceSpaceBoundsRect-bounds-parameter" type
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrGetReferenceSpaceBoundsRect(
    XrSession session,
    XrReferenceSpaceType referenceSpaceType,
    XrExtent2Df* bounds) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->GetReferenceSpaceBoundsRect(session, referenceSpaceType, bounds);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrGetReferenceSpaceBoundsRect(
    XrSession session,
    XrReferenceSpaceType referenceSpaceType,
    XrExtent2Df* bounds) {
    XrResult test_result = GenValidUsageInputsXrGetReferenceSpaceBoundsRect(session, referenceSpaceType, bounds);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrGetReferenceSpaceBoundsRect(session, referenceSpaceType, bounds);
}

XrResult GenValidUsageInputsXrCreateActionSpace(
XrSession session,
const XrActionSpaceCreateInfo* createInfo,
XrSpace* space) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrCreateActionSpace-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateActionSpace",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == createInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateActionSpace-createInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateActionSpace", objects_info,
                                "Invalid NULL for XrActionSpaceCreateInfo \"createInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrActionSpaceCreateInfo is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrCreateActionSpace", objects_info,
                                                        true, createInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateActionSpace-createInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateActionSpace",
                                objects_info,
                                "Command xrCreateActionSpace param createInfo is invalid");
            return xr_result;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == space) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateActionSpace-space-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateActionSpace", objects_info,
                                "Invalid NULL for XrSpace \"space\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrCreateActionSpace(
    XrSession session,
    const XrActionSpaceCreateInfo* createInfo,
    XrSpace* space) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->CreateActionSpace(session, createInfo, space);
        if (XR_SUCCESS == result && nullptr != space) {
            std::unique_ptr<GenValidUsageXrHandleInfo> handle_info(new GenValidUsageXrHandleInfo());
            handle_info->instance_info = gen_instance_info;
            handle_info->direct_parent_type = XR_OBJECT_TYPE_SESSION;
            handle_info->direct_parent_handle = MakeHandleGeneric(session);
            g_space_info.insert(*space, std::move(handle_info));
        }
    } catch (std::bad_alloc&) {
        result = XR_ERROR_OUT_OF_MEMORY;
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrCreateActionSpace(
    XrSession session,
    const XrActionSpaceCreateInfo* createInfo,
    XrSpace* space) {
    XrResult test_result = GenValidUsageInputsXrCreateActionSpace(session, createInfo, space);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrCreateActionSpace(session, createInfo, space);
}

XrResult GenValidUsageInputsXrLocateSpace(
XrSpace space,
XrSpace baseSpace,
XrTime   time,
XrSpaceLocation* location) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(space, XR_OBJECT_TYPE_SPACE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSpaceHandle(&space);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSpace handle \"space\" ";
                oss << HandleToHexString(space);
                CoreValidLogMessage(nullptr, "VUID-xrLocateSpace-space-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrLocateSpace",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_space_info.getWithInstanceInfo(space);
        GenValidUsageXrHandleInfo *gen_space_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        objects_info.emplace_back(baseSpace, XR_OBJECT_TYPE_SPACE);
        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSpaceHandle(&baseSpace);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSpace handle \"baseSpace\" ";
                oss << HandleToHexString(baseSpace);
                CoreValidLogMessage(gen_instance_info, "VUID-xrLocateSpace-baseSpace-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrLocateSpace",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        // Verify that the handles share a common ancestry
        if (!VerifyXrParent(XR_OBJECT_TYPE_SPACE,  MakeHandleGeneric(space),
                    XR_OBJECT_TYPE_SPACE,  MakeHandleGeneric(baseSpace), false)) {
            std::ostringstream oss_error;
            oss_error << "XrSpace " << HandleToHexString(space);
            oss_error <<  " and XrSpace ";
            oss_error << HandleToHexString(baseSpace);
            oss_error <<  " must share a parent";
            CoreValidLogMessage(gen_instance_info, "VUID-xrLocateSpace-commonparent",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrLocateSpace",
                                objects_info, oss_error.str());
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == location) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrLocateSpace-location-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrLocateSpace", objects_info,
                                "Invalid NULL for XrSpaceLocation \"location\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrSpaceLocation is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrLocateSpace", objects_info,
                                                        false, location);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrLocateSpace-location-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrLocateSpace",
                                objects_info,
                                "Command xrLocateSpace param location is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrLocateSpace(
    XrSpace space,
    XrSpace baseSpace,
    XrTime   time,
    XrSpaceLocation* location) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_space_info.getWithInstanceInfo(space);
        GenValidUsageXrHandleInfo *gen_space_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->LocateSpace(space, baseSpace, time, location);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrLocateSpace(
    XrSpace space,
    XrSpace baseSpace,
    XrTime   time,
    XrSpaceLocation* location) {
    XrResult test_result = GenValidUsageInputsXrLocateSpace(space, baseSpace, time, location);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrLocateSpace(space, baseSpace, time, location);
}

XrResult GenValidUsageInputsXrDestroySpace(
XrSpace space) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(space, XR_OBJECT_TYPE_SPACE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSpaceHandle(&space);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSpace handle \"space\" ";
                oss << HandleToHexString(space);
                CoreValidLogMessage(nullptr, "VUID-xrDestroySpace-space-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrDestroySpace",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_space_info.getWithInstanceInfo(space);
        GenValidUsageXrHandleInfo *gen_space_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrDestroySpace(
    XrSpace space) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_space_info.getWithInstanceInfo(space);
        GenValidUsageXrHandleInfo *gen_space_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->DestroySpace(space);
        if (XR_SUCCEEDED(result)) {
            g_space_info.erase(space);
        }
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrDestroySpace(
    XrSpace space) {
    XrResult test_result = GenValidUsageInputsXrDestroySpace(space);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrDestroySpace(space);
}

XrResult GenValidUsageInputsXrEnumerateViewConfigurations(
XrInstance instance,
XrSystemId systemId,
uint32_t viewConfigurationTypeCapacityInput,
uint32_t* viewConfigurationTypeCountOutput,
XrViewConfigurationType* viewConfigurationTypes) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrEnumerateViewConfigurations-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateViewConfigurations",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Optional array must be non-NULL when viewConfigurationTypeCapacityInput is non-zero
        if (0 != viewConfigurationTypeCapacityInput && nullptr == viewConfigurationTypes) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateViewConfigurations-viewConfigurationTypes-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateViewConfigurations",
                                objects_info,
                                "Command xrEnumerateViewConfigurations param viewConfigurationTypes is NULL, but viewConfigurationTypeCapacityInput is greater than 0");
            xr_result = XR_ERROR_VALIDATION_FAILURE;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == viewConfigurationTypeCountOutput) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateViewConfigurations-viewConfigurationTypeCountOutput-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateViewConfigurations", objects_info,
                                "Invalid NULL for uint32_t \"viewConfigurationTypeCountOutput\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrEnumerateViewConfigurations-viewConfigurationTypeCountOutput-parameter" type
        for (uint32_t value_viewconfigurationtypes_inc = 0; value_viewconfigurationtypes_inc < viewConfigurationTypeCapacityInput; ++value_viewconfigurationtypes_inc) {
            // Make sure the enum type XrViewConfigurationType value is valid
            if (!ValidateXrEnum(gen_instance_info, "xrEnumerateViewConfigurations", "xrEnumerateViewConfigurations", "viewConfigurationTypes", objects_info, viewConfigurationTypes[value_viewconfigurationtypes_inc])) {
                std::ostringstream oss_enum;
                oss_enum << "Invalid XrViewConfigurationType \"viewConfigurationTypes\" enum value ";
                oss_enum << Uint32ToHexString(static_cast<uint32_t>(viewConfigurationTypes[value_viewconfigurationtypes_inc]));
                CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateViewConfigurations-viewConfigurationTypes-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateViewConfigurations",
                                    objects_info, oss_enum.str());
                return XR_ERROR_VALIDATION_FAILURE;
            }
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrEnumerateViewConfigurations(
    XrInstance instance,
    XrSystemId systemId,
    uint32_t viewConfigurationTypeCapacityInput,
    uint32_t* viewConfigurationTypeCountOutput,
    XrViewConfigurationType* viewConfigurationTypes) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->EnumerateViewConfigurations(instance, systemId, viewConfigurationTypeCapacityInput, viewConfigurationTypeCountOutput, viewConfigurationTypes);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrEnumerateViewConfigurations(
    XrInstance instance,
    XrSystemId systemId,
    uint32_t viewConfigurationTypeCapacityInput,
    uint32_t* viewConfigurationTypeCountOutput,
    XrViewConfigurationType* viewConfigurationTypes) {
    XrResult test_result = GenValidUsageInputsXrEnumerateViewConfigurations(instance, systemId, viewConfigurationTypeCapacityInput, viewConfigurationTypeCountOutput, viewConfigurationTypes);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrEnumerateViewConfigurations(instance, systemId, viewConfigurationTypeCapacityInput, viewConfigurationTypeCountOutput, viewConfigurationTypes);
}

XrResult GenValidUsageInputsXrGetViewConfigurationProperties(
XrInstance instance,
XrSystemId systemId,
XrViewConfigurationType viewConfigurationType,
XrViewConfigurationProperties* configurationProperties) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrGetViewConfigurationProperties-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetViewConfigurationProperties",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Make sure the enum type XrViewConfigurationType value is valid
        if (!ValidateXrEnum(gen_instance_info, "xrGetViewConfigurationProperties", "xrGetViewConfigurationProperties", "viewConfigurationType", objects_info, viewConfigurationType)) {
            std::ostringstream oss_enum;
            oss_enum << "Invalid XrViewConfigurationType \"viewConfigurationType\" enum value ";
            oss_enum << Uint32ToHexString(static_cast<uint32_t>(viewConfigurationType));
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetViewConfigurationProperties-viewConfigurationType-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetViewConfigurationProperties",
                                objects_info, oss_enum.str());
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == configurationProperties) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetViewConfigurationProperties-configurationProperties-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetViewConfigurationProperties", objects_info,
                                "Invalid NULL for XrViewConfigurationProperties \"configurationProperties\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrViewConfigurationProperties is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrGetViewConfigurationProperties", objects_info,
                                                        false, configurationProperties);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetViewConfigurationProperties-configurationProperties-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetViewConfigurationProperties",
                                objects_info,
                                "Command xrGetViewConfigurationProperties param configurationProperties is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrGetViewConfigurationProperties(
    XrInstance instance,
    XrSystemId systemId,
    XrViewConfigurationType viewConfigurationType,
    XrViewConfigurationProperties* configurationProperties) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->GetViewConfigurationProperties(instance, systemId, viewConfigurationType, configurationProperties);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrGetViewConfigurationProperties(
    XrInstance instance,
    XrSystemId systemId,
    XrViewConfigurationType viewConfigurationType,
    XrViewConfigurationProperties* configurationProperties) {
    XrResult test_result = GenValidUsageInputsXrGetViewConfigurationProperties(instance, systemId, viewConfigurationType, configurationProperties);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrGetViewConfigurationProperties(instance, systemId, viewConfigurationType, configurationProperties);
}

XrResult GenValidUsageInputsXrEnumerateViewConfigurationViews(
XrInstance instance,
XrSystemId systemId,
XrViewConfigurationType viewConfigurationType,
uint32_t viewCapacityInput,
uint32_t* viewCountOutput,
XrViewConfigurationView* views) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrEnumerateViewConfigurationViews-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateViewConfigurationViews",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Make sure the enum type XrViewConfigurationType value is valid
        if (!ValidateXrEnum(gen_instance_info, "xrEnumerateViewConfigurationViews", "xrEnumerateViewConfigurationViews", "viewConfigurationType", objects_info, viewConfigurationType)) {
            std::ostringstream oss_enum;
            oss_enum << "Invalid XrViewConfigurationType \"viewConfigurationType\" enum value ";
            oss_enum << Uint32ToHexString(static_cast<uint32_t>(viewConfigurationType));
            CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateViewConfigurationViews-viewConfigurationType-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateViewConfigurationViews",
                                objects_info, oss_enum.str());
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Optional array must be non-NULL when viewCapacityInput is non-zero
        if (0 != viewCapacityInput && nullptr == views) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateViewConfigurationViews-views-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateViewConfigurationViews",
                                objects_info,
                                "Command xrEnumerateViewConfigurationViews param views is NULL, but viewCapacityInput is greater than 0");
            xr_result = XR_ERROR_VALIDATION_FAILURE;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == viewCountOutput) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateViewConfigurationViews-viewCountOutput-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateViewConfigurationViews", objects_info,
                                "Invalid NULL for uint32_t \"viewCountOutput\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrEnumerateViewConfigurationViews-viewCountOutput-parameter" type
        for (uint32_t value_views_inc = 0; value_views_inc < viewCapacityInput; ++value_views_inc) {
            // Validate that the structure XrViewConfigurationView is valid
            xr_result = ValidateXrStruct(gen_instance_info, "xrEnumerateViewConfigurationViews", objects_info,
                                                            true, &views[value_views_inc]);
            if (XR_SUCCESS != xr_result) {
                CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateViewConfigurationViews-views-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateViewConfigurationViews",
                                    objects_info,
                                    "Command xrEnumerateViewConfigurationViews param views is invalid");
                return xr_result;
            }
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrEnumerateViewConfigurationViews(
    XrInstance instance,
    XrSystemId systemId,
    XrViewConfigurationType viewConfigurationType,
    uint32_t viewCapacityInput,
    uint32_t* viewCountOutput,
    XrViewConfigurationView* views) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->EnumerateViewConfigurationViews(instance, systemId, viewConfigurationType, viewCapacityInput, viewCountOutput, views);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrEnumerateViewConfigurationViews(
    XrInstance instance,
    XrSystemId systemId,
    XrViewConfigurationType viewConfigurationType,
    uint32_t viewCapacityInput,
    uint32_t* viewCountOutput,
    XrViewConfigurationView* views) {
    XrResult test_result = GenValidUsageInputsXrEnumerateViewConfigurationViews(instance, systemId, viewConfigurationType, viewCapacityInput, viewCountOutput, views);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrEnumerateViewConfigurationViews(instance, systemId, viewConfigurationType, viewCapacityInput, viewCountOutput, views);
}

XrResult GenValidUsageInputsXrEnumerateSwapchainFormats(
XrSession session,
uint32_t formatCapacityInput,
uint32_t* formatCountOutput,
int64_t* formats) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrEnumerateSwapchainFormats-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateSwapchainFormats",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Optional array must be non-NULL when formatCapacityInput is non-zero
        if (0 != formatCapacityInput && nullptr == formats) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateSwapchainFormats-formats-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateSwapchainFormats",
                                objects_info,
                                "Command xrEnumerateSwapchainFormats param formats is NULL, but formatCapacityInput is greater than 0");
            xr_result = XR_ERROR_VALIDATION_FAILURE;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == formatCountOutput) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateSwapchainFormats-formatCountOutput-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateSwapchainFormats", objects_info,
                                "Invalid NULL for uint32_t \"formatCountOutput\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrEnumerateSwapchainFormats-formatCountOutput-parameter" type
        // NOTE: Can't validate "VUID-xrEnumerateSwapchainFormats-formats-parameter" type
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrEnumerateSwapchainFormats(
    XrSession session,
    uint32_t formatCapacityInput,
    uint32_t* formatCountOutput,
    int64_t* formats) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->EnumerateSwapchainFormats(session, formatCapacityInput, formatCountOutput, formats);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrEnumerateSwapchainFormats(
    XrSession session,
    uint32_t formatCapacityInput,
    uint32_t* formatCountOutput,
    int64_t* formats) {
    XrResult test_result = GenValidUsageInputsXrEnumerateSwapchainFormats(session, formatCapacityInput, formatCountOutput, formats);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrEnumerateSwapchainFormats(session, formatCapacityInput, formatCountOutput, formats);
}

XrResult GenValidUsageInputsXrCreateSwapchain(
XrSession session,
const XrSwapchainCreateInfo* createInfo,
XrSwapchain* swapchain) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrCreateSwapchain-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateSwapchain",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == createInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateSwapchain-createInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateSwapchain", objects_info,
                                "Invalid NULL for XrSwapchainCreateInfo \"createInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrSwapchainCreateInfo is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrCreateSwapchain", objects_info,
                                                        true, createInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateSwapchain-createInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateSwapchain",
                                objects_info,
                                "Command xrCreateSwapchain param createInfo is invalid");
            return xr_result;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == swapchain) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateSwapchain-swapchain-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateSwapchain", objects_info,
                                "Invalid NULL for XrSwapchain \"swapchain\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrCreateSwapchain(
    XrSession session,
    const XrSwapchainCreateInfo* createInfo,
    XrSwapchain* swapchain) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->CreateSwapchain(session, createInfo, swapchain);
        if (XR_SUCCESS == result && nullptr != swapchain) {
            std::unique_ptr<GenValidUsageXrHandleInfo> handle_info(new GenValidUsageXrHandleInfo());
            handle_info->instance_info = gen_instance_info;
            handle_info->direct_parent_type = XR_OBJECT_TYPE_SESSION;
            handle_info->direct_parent_handle = MakeHandleGeneric(session);
            g_swapchain_info.insert(*swapchain, std::move(handle_info));
        }
    } catch (std::bad_alloc&) {
        result = XR_ERROR_OUT_OF_MEMORY;
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrCreateSwapchain(
    XrSession session,
    const XrSwapchainCreateInfo* createInfo,
    XrSwapchain* swapchain) {
    XrResult test_result = GenValidUsageInputsXrCreateSwapchain(session, createInfo, swapchain);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrCreateSwapchain(session, createInfo, swapchain);
}

XrResult GenValidUsageInputsXrDestroySwapchain(
XrSwapchain swapchain) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(swapchain, XR_OBJECT_TYPE_SWAPCHAIN);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSwapchainHandle(&swapchain);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSwapchain handle \"swapchain\" ";
                oss << HandleToHexString(swapchain);
                CoreValidLogMessage(nullptr, "VUID-xrDestroySwapchain-swapchain-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrDestroySwapchain",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_swapchain_info.getWithInstanceInfo(swapchain);
        GenValidUsageXrHandleInfo *gen_swapchain_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrDestroySwapchain(
    XrSwapchain swapchain) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_swapchain_info.getWithInstanceInfo(swapchain);
        GenValidUsageXrHandleInfo *gen_swapchain_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->DestroySwapchain(swapchain);
        if (XR_SUCCEEDED(result)) {
            g_swapchain_info.erase(swapchain);
        }
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrDestroySwapchain(
    XrSwapchain swapchain) {
    XrResult test_result = GenValidUsageInputsXrDestroySwapchain(swapchain);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrDestroySwapchain(swapchain);
}

XrResult GenValidUsageInputsXrEnumerateSwapchainImages(
XrSwapchain swapchain,
uint32_t imageCapacityInput,
uint32_t* imageCountOutput,
XrSwapchainImageBaseHeader* images) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(swapchain, XR_OBJECT_TYPE_SWAPCHAIN);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSwapchainHandle(&swapchain);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSwapchain handle \"swapchain\" ";
                oss << HandleToHexString(swapchain);
                CoreValidLogMessage(nullptr, "VUID-xrEnumerateSwapchainImages-swapchain-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateSwapchainImages",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_swapchain_info.getWithInstanceInfo(swapchain);
        GenValidUsageXrHandleInfo *gen_swapchain_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Optional array must be non-NULL when imageCapacityInput is non-zero
        if (0 != imageCapacityInput && nullptr == images) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateSwapchainImages-images-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateSwapchainImages",
                                objects_info,
                                "Command xrEnumerateSwapchainImages param images is NULL, but imageCapacityInput is greater than 0");
            xr_result = XR_ERROR_VALIDATION_FAILURE;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == imageCountOutput) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateSwapchainImages-imageCountOutput-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateSwapchainImages", objects_info,
                                "Invalid NULL for uint32_t \"imageCountOutput\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrEnumerateSwapchainImages-imageCountOutput-parameter" type
        for (uint32_t value_images_inc = 0; value_images_inc < imageCapacityInput; ++value_images_inc) {
#if defined(XR_USE_GRAPHICS_API_OPENGL)
            // Validate if XrSwapchainImageBaseHeader is a child structure of type XrSwapchainImageOpenGLKHR and it is valid
            XrSwapchainImageOpenGLKHR* new_swapchainimageopenglkhr_value = reinterpret_cast<XrSwapchainImageOpenGLKHR*>(images);
            if (new_swapchainimageopenglkhr_value[value_images_inc].type == XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR) {
                if (nullptr != new_swapchainimageopenglkhr_value) {
                    xr_result = ValidateXrStruct(gen_instance_info, "xrEnumerateSwapchainImages",
                                                                    objects_info, false, &new_swapchainimageopenglkhr_value[value_images_inc]);
                    if (XR_SUCCESS != xr_result) {
                        std::string error_message = "Command xrEnumerateSwapchainImages param images";
                        error_message += "[";
                        error_message += std::to_string(value_images_inc);
                        error_message += "]";
                        error_message += " is invalid";
                        CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateSwapchainImages-images-parameter",
                                            VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateSwapchainImages",
                                            objects_info,
                                            error_message);
                        return XR_ERROR_VALIDATION_FAILURE;
                        break;
                    } else {
                        continue;
                        }
                }
            }
#endif // defined(XR_USE_GRAPHICS_API_OPENGL)
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
            // Validate if XrSwapchainImageBaseHeader is a child structure of type XrSwapchainImageOpenGLESKHR and it is valid
            XrSwapchainImageOpenGLESKHR* new_swapchainimageopengleskhr_value = reinterpret_cast<XrSwapchainImageOpenGLESKHR*>(images);
            if (new_swapchainimageopengleskhr_value[value_images_inc].type == XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR) {
                if (nullptr != new_swapchainimageopengleskhr_value) {
                    xr_result = ValidateXrStruct(gen_instance_info, "xrEnumerateSwapchainImages",
                                                                    objects_info, false, &new_swapchainimageopengleskhr_value[value_images_inc]);
                    if (XR_SUCCESS != xr_result) {
                        std::string error_message = "Command xrEnumerateSwapchainImages param images";
                        error_message += "[";
                        error_message += std::to_string(value_images_inc);
                        error_message += "]";
                        error_message += " is invalid";
                        CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateSwapchainImages-images-parameter",
                                            VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateSwapchainImages",
                                            objects_info,
                                            error_message);
                        return XR_ERROR_VALIDATION_FAILURE;
                        break;
                    } else {
                        continue;
                        }
                }
            }
#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
            // Validate if XrSwapchainImageBaseHeader is a child structure of type XrSwapchainImageVulkanKHR and it is valid
            XrSwapchainImageVulkanKHR* new_swapchainimagevulkankhr_value = reinterpret_cast<XrSwapchainImageVulkanKHR*>(images);
            if (new_swapchainimagevulkankhr_value[value_images_inc].type == XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR) {
                if (nullptr != new_swapchainimagevulkankhr_value) {
                    xr_result = ValidateXrStruct(gen_instance_info, "xrEnumerateSwapchainImages",
                                                                    objects_info, false, &new_swapchainimagevulkankhr_value[value_images_inc]);
                    if (XR_SUCCESS != xr_result) {
                        std::string error_message = "Command xrEnumerateSwapchainImages param images";
                        error_message += "[";
                        error_message += std::to_string(value_images_inc);
                        error_message += "]";
                        error_message += " is invalid";
                        CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateSwapchainImages-images-parameter",
                                            VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateSwapchainImages",
                                            objects_info,
                                            error_message);
                        return XR_ERROR_VALIDATION_FAILURE;
                        break;
                    } else {
                        continue;
                        }
                }
            }
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_D3D11)
            // Validate if XrSwapchainImageBaseHeader is a child structure of type XrSwapchainImageD3D11KHR and it is valid
            XrSwapchainImageD3D11KHR* new_swapchainimaged3d11khr_value = reinterpret_cast<XrSwapchainImageD3D11KHR*>(images);
            if (new_swapchainimaged3d11khr_value[value_images_inc].type == XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR) {
                if (nullptr != new_swapchainimaged3d11khr_value) {
                    xr_result = ValidateXrStruct(gen_instance_info, "xrEnumerateSwapchainImages",
                                                                    objects_info, false, &new_swapchainimaged3d11khr_value[value_images_inc]);
                    if (XR_SUCCESS != xr_result) {
                        std::string error_message = "Command xrEnumerateSwapchainImages param images";
                        error_message += "[";
                        error_message += std::to_string(value_images_inc);
                        error_message += "]";
                        error_message += " is invalid";
                        CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateSwapchainImages-images-parameter",
                                            VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateSwapchainImages",
                                            objects_info,
                                            error_message);
                        return XR_ERROR_VALIDATION_FAILURE;
                        break;
                    } else {
                        continue;
                        }
                }
            }
#endif // defined(XR_USE_GRAPHICS_API_D3D11)
#if defined(XR_USE_GRAPHICS_API_D3D12)
            // Validate if XrSwapchainImageBaseHeader is a child structure of type XrSwapchainImageD3D12KHR and it is valid
            XrSwapchainImageD3D12KHR* new_swapchainimaged3d12khr_value = reinterpret_cast<XrSwapchainImageD3D12KHR*>(images);
            if (new_swapchainimaged3d12khr_value[value_images_inc].type == XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR) {
                if (nullptr != new_swapchainimaged3d12khr_value) {
                    xr_result = ValidateXrStruct(gen_instance_info, "xrEnumerateSwapchainImages",
                                                                    objects_info, false, &new_swapchainimaged3d12khr_value[value_images_inc]);
                    if (XR_SUCCESS != xr_result) {
                        std::string error_message = "Command xrEnumerateSwapchainImages param images";
                        error_message += "[";
                        error_message += std::to_string(value_images_inc);
                        error_message += "]";
                        error_message += " is invalid";
                        CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateSwapchainImages-images-parameter",
                                            VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateSwapchainImages",
                                            objects_info,
                                            error_message);
                        return XR_ERROR_VALIDATION_FAILURE;
                        break;
                    } else {
                        continue;
                        }
                }
            }
#endif // defined(XR_USE_GRAPHICS_API_D3D12)
            // Validate that the base-structure XrSwapchainImageBaseHeader is valid
            xr_result = ValidateXrStruct(gen_instance_info, "xrEnumerateSwapchainImages", objects_info,
                                                            true, &images[value_images_inc]);
            if (XR_SUCCESS != xr_result) {
                CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateSwapchainImages-images-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateSwapchainImages",
                                    objects_info,
                                    "Command xrEnumerateSwapchainImages param images is invalid");
                return xr_result;
            }
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrEnumerateSwapchainImages(
    XrSwapchain swapchain,
    uint32_t imageCapacityInput,
    uint32_t* imageCountOutput,
    XrSwapchainImageBaseHeader* images) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_swapchain_info.getWithInstanceInfo(swapchain);
        GenValidUsageXrHandleInfo *gen_swapchain_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->EnumerateSwapchainImages(swapchain, imageCapacityInput, imageCountOutput, images);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrEnumerateSwapchainImages(
    XrSwapchain swapchain,
    uint32_t imageCapacityInput,
    uint32_t* imageCountOutput,
    XrSwapchainImageBaseHeader* images) {
    XrResult test_result = GenValidUsageInputsXrEnumerateSwapchainImages(swapchain, imageCapacityInput, imageCountOutput, images);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrEnumerateSwapchainImages(swapchain, imageCapacityInput, imageCountOutput, images);
}

XrResult GenValidUsageInputsXrAcquireSwapchainImage(
XrSwapchain         swapchain,
const XrSwapchainImageAcquireInfo* acquireInfo,
uint32_t* index) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(swapchain, XR_OBJECT_TYPE_SWAPCHAIN);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSwapchainHandle(&swapchain);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSwapchain handle \"swapchain\" ";
                oss << HandleToHexString(swapchain);
                CoreValidLogMessage(nullptr, "VUID-xrAcquireSwapchainImage-swapchain-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrAcquireSwapchainImage",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_swapchain_info.getWithInstanceInfo(swapchain);
        GenValidUsageXrHandleInfo *gen_swapchain_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Validate that the structure XrSwapchainImageAcquireInfo is valid
        if (nullptr != acquireInfo) {
            xr_result = ValidateXrStruct(gen_instance_info, "xrAcquireSwapchainImage",
                                                            objects_info, false, acquireInfo);
            if (XR_SUCCESS != xr_result) {
                CoreValidLogMessage(gen_instance_info, "VUID-xrAcquireSwapchainImage-acquireInfo-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrAcquireSwapchainImage",
                                    objects_info,
                                    "Command xrAcquireSwapchainImage param acquireInfo is invalid");
                return xr_result;
            }
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == index) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrAcquireSwapchainImage-index-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrAcquireSwapchainImage", objects_info,
                                "Invalid NULL for uint32_t \"index\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrAcquireSwapchainImage-index-parameter" type
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrAcquireSwapchainImage(
    XrSwapchain         swapchain,
    const XrSwapchainImageAcquireInfo* acquireInfo,
    uint32_t* index) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_swapchain_info.getWithInstanceInfo(swapchain);
        GenValidUsageXrHandleInfo *gen_swapchain_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->AcquireSwapchainImage(swapchain, acquireInfo, index);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrAcquireSwapchainImage(
    XrSwapchain         swapchain,
    const XrSwapchainImageAcquireInfo* acquireInfo,
    uint32_t* index) {
    XrResult test_result = GenValidUsageInputsXrAcquireSwapchainImage(swapchain, acquireInfo, index);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrAcquireSwapchainImage(swapchain, acquireInfo, index);
}

XrResult GenValidUsageInputsXrWaitSwapchainImage(
XrSwapchain swapchain,
const XrSwapchainImageWaitInfo* waitInfo) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(swapchain, XR_OBJECT_TYPE_SWAPCHAIN);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSwapchainHandle(&swapchain);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSwapchain handle \"swapchain\" ";
                oss << HandleToHexString(swapchain);
                CoreValidLogMessage(nullptr, "VUID-xrWaitSwapchainImage-swapchain-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrWaitSwapchainImage",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_swapchain_info.getWithInstanceInfo(swapchain);
        GenValidUsageXrHandleInfo *gen_swapchain_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == waitInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrWaitSwapchainImage-waitInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrWaitSwapchainImage", objects_info,
                                "Invalid NULL for XrSwapchainImageWaitInfo \"waitInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrSwapchainImageWaitInfo is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrWaitSwapchainImage", objects_info,
                                                        true, waitInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrWaitSwapchainImage-waitInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrWaitSwapchainImage",
                                objects_info,
                                "Command xrWaitSwapchainImage param waitInfo is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrWaitSwapchainImage(
    XrSwapchain swapchain,
    const XrSwapchainImageWaitInfo* waitInfo) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_swapchain_info.getWithInstanceInfo(swapchain);
        GenValidUsageXrHandleInfo *gen_swapchain_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->WaitSwapchainImage(swapchain, waitInfo);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrWaitSwapchainImage(
    XrSwapchain swapchain,
    const XrSwapchainImageWaitInfo* waitInfo) {
    XrResult test_result = GenValidUsageInputsXrWaitSwapchainImage(swapchain, waitInfo);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrWaitSwapchainImage(swapchain, waitInfo);
}

XrResult GenValidUsageInputsXrReleaseSwapchainImage(
XrSwapchain         swapchain,
const XrSwapchainImageReleaseInfo* releaseInfo) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(swapchain, XR_OBJECT_TYPE_SWAPCHAIN);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSwapchainHandle(&swapchain);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSwapchain handle \"swapchain\" ";
                oss << HandleToHexString(swapchain);
                CoreValidLogMessage(nullptr, "VUID-xrReleaseSwapchainImage-swapchain-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrReleaseSwapchainImage",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_swapchain_info.getWithInstanceInfo(swapchain);
        GenValidUsageXrHandleInfo *gen_swapchain_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Validate that the structure XrSwapchainImageReleaseInfo is valid
        if (nullptr != releaseInfo) {
            xr_result = ValidateXrStruct(gen_instance_info, "xrReleaseSwapchainImage",
                                                            objects_info, false, releaseInfo);
            if (XR_SUCCESS != xr_result) {
                CoreValidLogMessage(gen_instance_info, "VUID-xrReleaseSwapchainImage-releaseInfo-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrReleaseSwapchainImage",
                                    objects_info,
                                    "Command xrReleaseSwapchainImage param releaseInfo is invalid");
                return xr_result;
            }
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrReleaseSwapchainImage(
    XrSwapchain         swapchain,
    const XrSwapchainImageReleaseInfo* releaseInfo) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_swapchain_info.getWithInstanceInfo(swapchain);
        GenValidUsageXrHandleInfo *gen_swapchain_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->ReleaseSwapchainImage(swapchain, releaseInfo);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrReleaseSwapchainImage(
    XrSwapchain         swapchain,
    const XrSwapchainImageReleaseInfo* releaseInfo) {
    XrResult test_result = GenValidUsageInputsXrReleaseSwapchainImage(swapchain, releaseInfo);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrReleaseSwapchainImage(swapchain, releaseInfo);
}

XrResult GenValidUsageInputsXrBeginSession(
XrSession session,
const XrSessionBeginInfo* beginInfo) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrBeginSession-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrBeginSession",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == beginInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrBeginSession-beginInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrBeginSession", objects_info,
                                "Invalid NULL for XrSessionBeginInfo \"beginInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrSessionBeginInfo is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrBeginSession", objects_info,
                                                        true, beginInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrBeginSession-beginInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrBeginSession",
                                objects_info,
                                "Command xrBeginSession param beginInfo is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrBeginSession(
    XrSession session,
    const XrSessionBeginInfo* beginInfo) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->BeginSession(session, beginInfo);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrBeginSession(
    XrSession session,
    const XrSessionBeginInfo* beginInfo) {
    XrResult test_result = GenValidUsageInputsXrBeginSession(session, beginInfo);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrBeginSession(session, beginInfo);
}

XrResult GenValidUsageInputsXrEndSession(
XrSession session) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrEndSession-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEndSession",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrEndSession(
    XrSession session) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->EndSession(session);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrEndSession(
    XrSession session) {
    XrResult test_result = GenValidUsageInputsXrEndSession(session);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrEndSession(session);
}

XrResult GenValidUsageInputsXrRequestExitSession(
XrSession session) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrRequestExitSession-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrRequestExitSession",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrRequestExitSession(
    XrSession session) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->RequestExitSession(session);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrRequestExitSession(
    XrSession session) {
    XrResult test_result = GenValidUsageInputsXrRequestExitSession(session);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrRequestExitSession(session);
}

XrResult GenValidUsageInputsXrWaitFrame(
XrSession session,
const XrFrameWaitInfo* frameWaitInfo,
XrFrameState* frameState) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrWaitFrame-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrWaitFrame",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Validate that the structure XrFrameWaitInfo is valid
        if (nullptr != frameWaitInfo) {
            xr_result = ValidateXrStruct(gen_instance_info, "xrWaitFrame",
                                                            objects_info, false, frameWaitInfo);
            if (XR_SUCCESS != xr_result) {
                CoreValidLogMessage(gen_instance_info, "VUID-xrWaitFrame-frameWaitInfo-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrWaitFrame",
                                    objects_info,
                                    "Command xrWaitFrame param frameWaitInfo is invalid");
                return xr_result;
            }
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == frameState) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrWaitFrame-frameState-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrWaitFrame", objects_info,
                                "Invalid NULL for XrFrameState \"frameState\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrFrameState is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrWaitFrame", objects_info,
                                                        false, frameState);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrWaitFrame-frameState-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrWaitFrame",
                                objects_info,
                                "Command xrWaitFrame param frameState is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrWaitFrame(
    XrSession session,
    const XrFrameWaitInfo* frameWaitInfo,
    XrFrameState* frameState) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->WaitFrame(session, frameWaitInfo, frameState);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrWaitFrame(
    XrSession session,
    const XrFrameWaitInfo* frameWaitInfo,
    XrFrameState* frameState) {
    XrResult test_result = GenValidUsageInputsXrWaitFrame(session, frameWaitInfo, frameState);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrWaitFrame(session, frameWaitInfo, frameState);
}

XrResult GenValidUsageInputsXrBeginFrame(
XrSession session,
const XrFrameBeginInfo* frameBeginInfo) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrBeginFrame-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrBeginFrame",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Validate that the structure XrFrameBeginInfo is valid
        if (nullptr != frameBeginInfo) {
            xr_result = ValidateXrStruct(gen_instance_info, "xrBeginFrame",
                                                            objects_info, false, frameBeginInfo);
            if (XR_SUCCESS != xr_result) {
                CoreValidLogMessage(gen_instance_info, "VUID-xrBeginFrame-frameBeginInfo-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrBeginFrame",
                                    objects_info,
                                    "Command xrBeginFrame param frameBeginInfo is invalid");
                return xr_result;
            }
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrBeginFrame(
    XrSession session,
    const XrFrameBeginInfo* frameBeginInfo) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->BeginFrame(session, frameBeginInfo);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrBeginFrame(
    XrSession session,
    const XrFrameBeginInfo* frameBeginInfo) {
    XrResult test_result = GenValidUsageInputsXrBeginFrame(session, frameBeginInfo);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrBeginFrame(session, frameBeginInfo);
}

XrResult GenValidUsageInputsXrEndFrame(
XrSession session,
const XrFrameEndInfo* frameEndInfo) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrEndFrame-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEndFrame",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == frameEndInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrEndFrame-frameEndInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEndFrame", objects_info,
                                "Invalid NULL for XrFrameEndInfo \"frameEndInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrFrameEndInfo is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrEndFrame", objects_info,
                                                        true, frameEndInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrEndFrame-frameEndInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEndFrame",
                                objects_info,
                                "Command xrEndFrame param frameEndInfo is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrEndFrame(
    XrSession session,
    const XrFrameEndInfo* frameEndInfo) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->EndFrame(session, frameEndInfo);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrEndFrame(
    XrSession session,
    const XrFrameEndInfo* frameEndInfo) {
    XrResult test_result = GenValidUsageInputsXrEndFrame(session, frameEndInfo);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrEndFrame(session, frameEndInfo);
}

XrResult GenValidUsageInputsXrLocateViews(
XrSession session,
const XrViewLocateInfo* viewLocateInfo,
XrViewState* viewState,
uint32_t viewCapacityInput,
uint32_t* viewCountOutput,
XrView* views) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrLocateViews-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrLocateViews",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == viewLocateInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrLocateViews-viewLocateInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrLocateViews", objects_info,
                                "Invalid NULL for XrViewLocateInfo \"viewLocateInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrViewLocateInfo is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrLocateViews", objects_info,
                                                        true, viewLocateInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrLocateViews-viewLocateInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrLocateViews",
                                objects_info,
                                "Command xrLocateViews param viewLocateInfo is invalid");
            return xr_result;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == viewState) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrLocateViews-viewState-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrLocateViews", objects_info,
                                "Invalid NULL for XrViewState \"viewState\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrViewState is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrLocateViews", objects_info,
                                                        false, viewState);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrLocateViews-viewState-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrLocateViews",
                                objects_info,
                                "Command xrLocateViews param viewState is invalid");
            return xr_result;
        }
        // Optional array must be non-NULL when viewCapacityInput is non-zero
        if (0 != viewCapacityInput && nullptr == views) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrLocateViews-views-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrLocateViews",
                                objects_info,
                                "Command xrLocateViews param views is NULL, but viewCapacityInput is greater than 0");
            xr_result = XR_ERROR_VALIDATION_FAILURE;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == viewCountOutput) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrLocateViews-viewCountOutput-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrLocateViews", objects_info,
                                "Invalid NULL for uint32_t \"viewCountOutput\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrLocateViews-viewCountOutput-parameter" type
        for (uint32_t value_views_inc = 0; value_views_inc < viewCapacityInput; ++value_views_inc) {
            // Validate that the structure XrView is valid
            xr_result = ValidateXrStruct(gen_instance_info, "xrLocateViews", objects_info,
                                                            true, &views[value_views_inc]);
            if (XR_SUCCESS != xr_result) {
                CoreValidLogMessage(gen_instance_info, "VUID-xrLocateViews-views-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrLocateViews",
                                    objects_info,
                                    "Command xrLocateViews param views is invalid");
                return xr_result;
            }
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrLocateViews(
    XrSession session,
    const XrViewLocateInfo* viewLocateInfo,
    XrViewState* viewState,
    uint32_t viewCapacityInput,
    uint32_t* viewCountOutput,
    XrView* views) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->LocateViews(session, viewLocateInfo, viewState, viewCapacityInput, viewCountOutput, views);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrLocateViews(
    XrSession session,
    const XrViewLocateInfo* viewLocateInfo,
    XrViewState* viewState,
    uint32_t viewCapacityInput,
    uint32_t* viewCountOutput,
    XrView* views) {
    XrResult test_result = GenValidUsageInputsXrLocateViews(session, viewLocateInfo, viewState, viewCapacityInput, viewCountOutput, views);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrLocateViews(session, viewLocateInfo, viewState, viewCapacityInput, viewCountOutput, views);
}

XrResult GenValidUsageInputsXrStringToPath(
XrInstance instance,
const char* pathString,
XrPath* path) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrStringToPath-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrStringToPath",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == pathString) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrStringToPath-pathString-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrStringToPath", objects_info,
                                "Invalid NULL for char \"pathString\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrStringToPath-pathString-parameter" null-termination
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == path) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrStringToPath-path-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrStringToPath", objects_info,
                                "Invalid NULL for XrPath \"path\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrStringToPath-path-parameter" type
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrStringToPath(
    XrInstance instance,
    const char* pathString,
    XrPath* path) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->StringToPath(instance, pathString, path);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrStringToPath(
    XrInstance instance,
    const char* pathString,
    XrPath* path) {
    XrResult test_result = GenValidUsageInputsXrStringToPath(instance, pathString, path);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrStringToPath(instance, pathString, path);
}

XrResult GenValidUsageInputsXrPathToString(
XrInstance instance,
XrPath path,
uint32_t bufferCapacityInput,
uint32_t* bufferCountOutput,
char* buffer) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrPathToString-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrPathToString",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Optional array must be non-NULL when bufferCapacityInput is non-zero
        if (0 != bufferCapacityInput && nullptr == buffer) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrPathToString-buffer-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrPathToString",
                                objects_info,
                                "Command xrPathToString param buffer is NULL, but bufferCapacityInput is greater than 0");
            xr_result = XR_ERROR_VALIDATION_FAILURE;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == bufferCountOutput) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrPathToString-bufferCountOutput-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrPathToString", objects_info,
                                "Invalid NULL for uint32_t \"bufferCountOutput\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrPathToString-bufferCountOutput-parameter" type
        // NOTE: Can't validate "VUID-xrPathToString-buffer-parameter" type
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrPathToString(
    XrInstance instance,
    XrPath path,
    uint32_t bufferCapacityInput,
    uint32_t* bufferCountOutput,
    char* buffer) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->PathToString(instance, path, bufferCapacityInput, bufferCountOutput, buffer);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrPathToString(
    XrInstance instance,
    XrPath path,
    uint32_t bufferCapacityInput,
    uint32_t* bufferCountOutput,
    char* buffer) {
    XrResult test_result = GenValidUsageInputsXrPathToString(instance, path, bufferCapacityInput, bufferCountOutput, buffer);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrPathToString(instance, path, bufferCapacityInput, bufferCountOutput, buffer);
}

XrResult GenValidUsageInputsXrCreateActionSet(
XrInstance instance,
const XrActionSetCreateInfo* createInfo,
XrActionSet* actionSet) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrCreateActionSet-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateActionSet",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == createInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateActionSet-createInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateActionSet", objects_info,
                                "Invalid NULL for XrActionSetCreateInfo \"createInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrActionSetCreateInfo is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrCreateActionSet", objects_info,
                                                        true, createInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateActionSet-createInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateActionSet",
                                objects_info,
                                "Command xrCreateActionSet param createInfo is invalid");
            return xr_result;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == actionSet) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateActionSet-actionSet-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateActionSet", objects_info,
                                "Invalid NULL for XrActionSet \"actionSet\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrCreateActionSet(
    XrInstance instance,
    const XrActionSetCreateInfo* createInfo,
    XrActionSet* actionSet) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->CreateActionSet(instance, createInfo, actionSet);
        if (XR_SUCCESS == result && nullptr != actionSet) {
            std::unique_ptr<GenValidUsageXrHandleInfo> handle_info(new GenValidUsageXrHandleInfo());
            handle_info->instance_info = gen_instance_info;
            handle_info->direct_parent_type = XR_OBJECT_TYPE_INSTANCE;
            handle_info->direct_parent_handle = MakeHandleGeneric(instance);
            g_actionset_info.insert(*actionSet, std::move(handle_info));
        }
    } catch (std::bad_alloc&) {
        result = XR_ERROR_OUT_OF_MEMORY;
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrCreateActionSet(
    XrInstance instance,
    const XrActionSetCreateInfo* createInfo,
    XrActionSet* actionSet) {
    XrResult test_result = GenValidUsageInputsXrCreateActionSet(instance, createInfo, actionSet);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrCreateActionSet(instance, createInfo, actionSet);
}

XrResult GenValidUsageInputsXrDestroyActionSet(
XrActionSet actionSet) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(actionSet, XR_OBJECT_TYPE_ACTION_SET);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrActionSetHandle(&actionSet);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrActionSet handle \"actionSet\" ";
                oss << HandleToHexString(actionSet);
                CoreValidLogMessage(nullptr, "VUID-xrDestroyActionSet-actionSet-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrDestroyActionSet",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_actionset_info.getWithInstanceInfo(actionSet);
        GenValidUsageXrHandleInfo *gen_actionset_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrDestroyActionSet(
    XrActionSet actionSet) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_actionset_info.getWithInstanceInfo(actionSet);
        GenValidUsageXrHandleInfo *gen_actionset_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->DestroyActionSet(actionSet);
        if (XR_SUCCEEDED(result)) {
            g_actionset_info.erase(actionSet);
        }
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrDestroyActionSet(
    XrActionSet actionSet) {
    XrResult test_result = GenValidUsageInputsXrDestroyActionSet(actionSet);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrDestroyActionSet(actionSet);
}

XrResult GenValidUsageInputsXrCreateAction(
XrActionSet actionSet,
const XrActionCreateInfo* createInfo,
XrAction* action) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(actionSet, XR_OBJECT_TYPE_ACTION_SET);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrActionSetHandle(&actionSet);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrActionSet handle \"actionSet\" ";
                oss << HandleToHexString(actionSet);
                CoreValidLogMessage(nullptr, "VUID-xrCreateAction-actionSet-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateAction",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_actionset_info.getWithInstanceInfo(actionSet);
        GenValidUsageXrHandleInfo *gen_actionset_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == createInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateAction-createInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateAction", objects_info,
                                "Invalid NULL for XrActionCreateInfo \"createInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrActionCreateInfo is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrCreateAction", objects_info,
                                                        true, createInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateAction-createInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateAction",
                                objects_info,
                                "Command xrCreateAction param createInfo is invalid");
            return xr_result;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == action) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateAction-action-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateAction", objects_info,
                                "Invalid NULL for XrAction \"action\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrCreateAction(
    XrActionSet actionSet,
    const XrActionCreateInfo* createInfo,
    XrAction* action) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_actionset_info.getWithInstanceInfo(actionSet);
        GenValidUsageXrHandleInfo *gen_actionset_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->CreateAction(actionSet, createInfo, action);
        if (XR_SUCCESS == result && nullptr != action) {
            std::unique_ptr<GenValidUsageXrHandleInfo> handle_info(new GenValidUsageXrHandleInfo());
            handle_info->instance_info = gen_instance_info;
            handle_info->direct_parent_type = XR_OBJECT_TYPE_ACTION_SET;
            handle_info->direct_parent_handle = MakeHandleGeneric(actionSet);
            g_action_info.insert(*action, std::move(handle_info));
        }
    } catch (std::bad_alloc&) {
        result = XR_ERROR_OUT_OF_MEMORY;
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrCreateAction(
    XrActionSet actionSet,
    const XrActionCreateInfo* createInfo,
    XrAction* action) {
    XrResult test_result = GenValidUsageInputsXrCreateAction(actionSet, createInfo, action);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrCreateAction(actionSet, createInfo, action);
}

XrResult GenValidUsageInputsXrDestroyAction(
XrAction action) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(action, XR_OBJECT_TYPE_ACTION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrActionHandle(&action);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrAction handle \"action\" ";
                oss << HandleToHexString(action);
                CoreValidLogMessage(nullptr, "VUID-xrDestroyAction-action-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrDestroyAction",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_action_info.getWithInstanceInfo(action);
        GenValidUsageXrHandleInfo *gen_action_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrDestroyAction(
    XrAction action) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_action_info.getWithInstanceInfo(action);
        GenValidUsageXrHandleInfo *gen_action_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->DestroyAction(action);
        if (XR_SUCCEEDED(result)) {
            g_action_info.erase(action);
        }
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrDestroyAction(
    XrAction action) {
    XrResult test_result = GenValidUsageInputsXrDestroyAction(action);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrDestroyAction(action);
}

XrResult GenValidUsageInputsXrSuggestInteractionProfileBindings(
XrInstance instance,
const XrInteractionProfileSuggestedBinding* suggestedBindings) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrSuggestInteractionProfileBindings-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSuggestInteractionProfileBindings",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == suggestedBindings) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrSuggestInteractionProfileBindings-suggestedBindings-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSuggestInteractionProfileBindings", objects_info,
                                "Invalid NULL for XrInteractionProfileSuggestedBinding \"suggestedBindings\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrInteractionProfileSuggestedBinding is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrSuggestInteractionProfileBindings", objects_info,
                                                        true, suggestedBindings);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrSuggestInteractionProfileBindings-suggestedBindings-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSuggestInteractionProfileBindings",
                                objects_info,
                                "Command xrSuggestInteractionProfileBindings param suggestedBindings is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrSuggestInteractionProfileBindings(
    XrInstance instance,
    const XrInteractionProfileSuggestedBinding* suggestedBindings) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->SuggestInteractionProfileBindings(instance, suggestedBindings);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrSuggestInteractionProfileBindings(
    XrInstance instance,
    const XrInteractionProfileSuggestedBinding* suggestedBindings) {
    XrResult test_result = GenValidUsageInputsXrSuggestInteractionProfileBindings(instance, suggestedBindings);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrSuggestInteractionProfileBindings(instance, suggestedBindings);
}

XrResult GenValidUsageInputsXrAttachSessionActionSets(
XrSession session,
const XrSessionActionSetsAttachInfo* attachInfo) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrAttachSessionActionSets-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrAttachSessionActionSets",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == attachInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrAttachSessionActionSets-attachInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrAttachSessionActionSets", objects_info,
                                "Invalid NULL for XrSessionActionSetsAttachInfo \"attachInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrSessionActionSetsAttachInfo is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrAttachSessionActionSets", objects_info,
                                                        true, attachInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrAttachSessionActionSets-attachInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrAttachSessionActionSets",
                                objects_info,
                                "Command xrAttachSessionActionSets param attachInfo is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrAttachSessionActionSets(
    XrSession session,
    const XrSessionActionSetsAttachInfo* attachInfo) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->AttachSessionActionSets(session, attachInfo);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrAttachSessionActionSets(
    XrSession session,
    const XrSessionActionSetsAttachInfo* attachInfo) {
    XrResult test_result = GenValidUsageInputsXrAttachSessionActionSets(session, attachInfo);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrAttachSessionActionSets(session, attachInfo);
}

XrResult GenValidUsageInputsXrGetCurrentInteractionProfile(
XrSession session,
XrPath topLevelUserPath,
XrInteractionProfileState* interactionProfile) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrGetCurrentInteractionProfile-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetCurrentInteractionProfile",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == interactionProfile) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetCurrentInteractionProfile-interactionProfile-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetCurrentInteractionProfile", objects_info,
                                "Invalid NULL for XrInteractionProfileState \"interactionProfile\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrInteractionProfileState is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrGetCurrentInteractionProfile", objects_info,
                                                        false, interactionProfile);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetCurrentInteractionProfile-interactionProfile-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetCurrentInteractionProfile",
                                objects_info,
                                "Command xrGetCurrentInteractionProfile param interactionProfile is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrGetCurrentInteractionProfile(
    XrSession session,
    XrPath topLevelUserPath,
    XrInteractionProfileState* interactionProfile) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->GetCurrentInteractionProfile(session, topLevelUserPath, interactionProfile);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrGetCurrentInteractionProfile(
    XrSession session,
    XrPath topLevelUserPath,
    XrInteractionProfileState* interactionProfile) {
    XrResult test_result = GenValidUsageInputsXrGetCurrentInteractionProfile(session, topLevelUserPath, interactionProfile);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrGetCurrentInteractionProfile(session, topLevelUserPath, interactionProfile);
}

XrResult GenValidUsageInputsXrGetActionStateBoolean(
XrSession session,
const XrActionStateGetInfo* getInfo,
XrActionStateBoolean* state) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrGetActionStateBoolean-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetActionStateBoolean",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == getInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetActionStateBoolean-getInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetActionStateBoolean", objects_info,
                                "Invalid NULL for XrActionStateGetInfo \"getInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrActionStateGetInfo is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrGetActionStateBoolean", objects_info,
                                                        true, getInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetActionStateBoolean-getInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetActionStateBoolean",
                                objects_info,
                                "Command xrGetActionStateBoolean param getInfo is invalid");
            return xr_result;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == state) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetActionStateBoolean-state-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetActionStateBoolean", objects_info,
                                "Invalid NULL for XrActionStateBoolean \"state\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrActionStateBoolean is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrGetActionStateBoolean", objects_info,
                                                        false, state);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetActionStateBoolean-state-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetActionStateBoolean",
                                objects_info,
                                "Command xrGetActionStateBoolean param state is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrGetActionStateBoolean(
    XrSession session,
    const XrActionStateGetInfo* getInfo,
    XrActionStateBoolean* state) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->GetActionStateBoolean(session, getInfo, state);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrGetActionStateBoolean(
    XrSession session,
    const XrActionStateGetInfo* getInfo,
    XrActionStateBoolean* state) {
    XrResult test_result = GenValidUsageInputsXrGetActionStateBoolean(session, getInfo, state);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrGetActionStateBoolean(session, getInfo, state);
}

XrResult GenValidUsageInputsXrGetActionStateFloat(
XrSession session,
const XrActionStateGetInfo* getInfo,
XrActionStateFloat* state) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrGetActionStateFloat-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetActionStateFloat",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == getInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetActionStateFloat-getInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetActionStateFloat", objects_info,
                                "Invalid NULL for XrActionStateGetInfo \"getInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrActionStateGetInfo is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrGetActionStateFloat", objects_info,
                                                        true, getInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetActionStateFloat-getInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetActionStateFloat",
                                objects_info,
                                "Command xrGetActionStateFloat param getInfo is invalid");
            return xr_result;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == state) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetActionStateFloat-state-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetActionStateFloat", objects_info,
                                "Invalid NULL for XrActionStateFloat \"state\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrActionStateFloat is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrGetActionStateFloat", objects_info,
                                                        false, state);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetActionStateFloat-state-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetActionStateFloat",
                                objects_info,
                                "Command xrGetActionStateFloat param state is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrGetActionStateFloat(
    XrSession session,
    const XrActionStateGetInfo* getInfo,
    XrActionStateFloat* state) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->GetActionStateFloat(session, getInfo, state);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrGetActionStateFloat(
    XrSession session,
    const XrActionStateGetInfo* getInfo,
    XrActionStateFloat* state) {
    XrResult test_result = GenValidUsageInputsXrGetActionStateFloat(session, getInfo, state);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrGetActionStateFloat(session, getInfo, state);
}

XrResult GenValidUsageInputsXrGetActionStateVector2f(
XrSession session,
const XrActionStateGetInfo* getInfo,
XrActionStateVector2f* state) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrGetActionStateVector2f-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetActionStateVector2f",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == getInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetActionStateVector2f-getInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetActionStateVector2f", objects_info,
                                "Invalid NULL for XrActionStateGetInfo \"getInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrActionStateGetInfo is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrGetActionStateVector2f", objects_info,
                                                        true, getInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetActionStateVector2f-getInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetActionStateVector2f",
                                objects_info,
                                "Command xrGetActionStateVector2f param getInfo is invalid");
            return xr_result;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == state) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetActionStateVector2f-state-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetActionStateVector2f", objects_info,
                                "Invalid NULL for XrActionStateVector2f \"state\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrActionStateVector2f is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrGetActionStateVector2f", objects_info,
                                                        false, state);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetActionStateVector2f-state-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetActionStateVector2f",
                                objects_info,
                                "Command xrGetActionStateVector2f param state is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrGetActionStateVector2f(
    XrSession session,
    const XrActionStateGetInfo* getInfo,
    XrActionStateVector2f* state) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->GetActionStateVector2f(session, getInfo, state);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrGetActionStateVector2f(
    XrSession session,
    const XrActionStateGetInfo* getInfo,
    XrActionStateVector2f* state) {
    XrResult test_result = GenValidUsageInputsXrGetActionStateVector2f(session, getInfo, state);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrGetActionStateVector2f(session, getInfo, state);
}

XrResult GenValidUsageInputsXrGetActionStatePose(
XrSession session,
const XrActionStateGetInfo* getInfo,
XrActionStatePose* state) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrGetActionStatePose-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetActionStatePose",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == getInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetActionStatePose-getInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetActionStatePose", objects_info,
                                "Invalid NULL for XrActionStateGetInfo \"getInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrActionStateGetInfo is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrGetActionStatePose", objects_info,
                                                        true, getInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetActionStatePose-getInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetActionStatePose",
                                objects_info,
                                "Command xrGetActionStatePose param getInfo is invalid");
            return xr_result;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == state) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetActionStatePose-state-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetActionStatePose", objects_info,
                                "Invalid NULL for XrActionStatePose \"state\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrActionStatePose is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrGetActionStatePose", objects_info,
                                                        false, state);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetActionStatePose-state-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetActionStatePose",
                                objects_info,
                                "Command xrGetActionStatePose param state is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrGetActionStatePose(
    XrSession session,
    const XrActionStateGetInfo* getInfo,
    XrActionStatePose* state) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->GetActionStatePose(session, getInfo, state);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrGetActionStatePose(
    XrSession session,
    const XrActionStateGetInfo* getInfo,
    XrActionStatePose* state) {
    XrResult test_result = GenValidUsageInputsXrGetActionStatePose(session, getInfo, state);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrGetActionStatePose(session, getInfo, state);
}

XrResult GenValidUsageInputsXrSyncActions(
XrSession session,
const XrActionsSyncInfo* syncInfo) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrSyncActions-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSyncActions",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == syncInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrSyncActions-syncInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSyncActions", objects_info,
                                "Invalid NULL for XrActionsSyncInfo \"syncInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrActionsSyncInfo is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrSyncActions", objects_info,
                                                        true, syncInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrSyncActions-syncInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSyncActions",
                                objects_info,
                                "Command xrSyncActions param syncInfo is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrSyncActions(
    XrSession session,
    const XrActionsSyncInfo* syncInfo) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->SyncActions(session, syncInfo);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrSyncActions(
    XrSession session,
    const XrActionsSyncInfo* syncInfo) {
    XrResult test_result = GenValidUsageInputsXrSyncActions(session, syncInfo);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrSyncActions(session, syncInfo);
}

XrResult GenValidUsageInputsXrEnumerateBoundSourcesForAction(
XrSession session,
const XrBoundSourcesForActionEnumerateInfo* enumerateInfo,
uint32_t sourceCapacityInput,
uint32_t* sourceCountOutput,
XrPath* sources) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrEnumerateBoundSourcesForAction-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateBoundSourcesForAction",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == enumerateInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateBoundSourcesForAction-enumerateInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateBoundSourcesForAction", objects_info,
                                "Invalid NULL for XrBoundSourcesForActionEnumerateInfo \"enumerateInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrBoundSourcesForActionEnumerateInfo is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrEnumerateBoundSourcesForAction", objects_info,
                                                        true, enumerateInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateBoundSourcesForAction-enumerateInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateBoundSourcesForAction",
                                objects_info,
                                "Command xrEnumerateBoundSourcesForAction param enumerateInfo is invalid");
            return xr_result;
        }
        // Optional array must be non-NULL when sourceCapacityInput is non-zero
        if (0 != sourceCapacityInput && nullptr == sources) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateBoundSourcesForAction-sources-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateBoundSourcesForAction",
                                objects_info,
                                "Command xrEnumerateBoundSourcesForAction param sources is NULL, but sourceCapacityInput is greater than 0");
            xr_result = XR_ERROR_VALIDATION_FAILURE;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == sourceCountOutput) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrEnumerateBoundSourcesForAction-sourceCountOutput-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrEnumerateBoundSourcesForAction", objects_info,
                                "Invalid NULL for uint32_t \"sourceCountOutput\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrEnumerateBoundSourcesForAction-sourceCountOutput-parameter" type
        // NOTE: Can't validate "VUID-xrEnumerateBoundSourcesForAction-sources-parameter" type
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrEnumerateBoundSourcesForAction(
    XrSession session,
    const XrBoundSourcesForActionEnumerateInfo* enumerateInfo,
    uint32_t sourceCapacityInput,
    uint32_t* sourceCountOutput,
    XrPath* sources) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->EnumerateBoundSourcesForAction(session, enumerateInfo, sourceCapacityInput, sourceCountOutput, sources);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrEnumerateBoundSourcesForAction(
    XrSession session,
    const XrBoundSourcesForActionEnumerateInfo* enumerateInfo,
    uint32_t sourceCapacityInput,
    uint32_t* sourceCountOutput,
    XrPath* sources) {
    XrResult test_result = GenValidUsageInputsXrEnumerateBoundSourcesForAction(session, enumerateInfo, sourceCapacityInput, sourceCountOutput, sources);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrEnumerateBoundSourcesForAction(session, enumerateInfo, sourceCapacityInput, sourceCountOutput, sources);
}

XrResult GenValidUsageInputsXrGetInputSourceLocalizedName(
XrSession session,
const XrInputSourceLocalizedNameGetInfo* getInfo,
uint32_t bufferCapacityInput,
uint32_t* bufferCountOutput,
char* buffer) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrGetInputSourceLocalizedName-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetInputSourceLocalizedName",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == getInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetInputSourceLocalizedName-getInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetInputSourceLocalizedName", objects_info,
                                "Invalid NULL for XrInputSourceLocalizedNameGetInfo \"getInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrInputSourceLocalizedNameGetInfo is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrGetInputSourceLocalizedName", objects_info,
                                                        true, getInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetInputSourceLocalizedName-getInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetInputSourceLocalizedName",
                                objects_info,
                                "Command xrGetInputSourceLocalizedName param getInfo is invalid");
            return xr_result;
        }
        // Optional array must be non-NULL when bufferCapacityInput is non-zero
        if (0 != bufferCapacityInput && nullptr == buffer) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetInputSourceLocalizedName-buffer-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetInputSourceLocalizedName",
                                objects_info,
                                "Command xrGetInputSourceLocalizedName param buffer is NULL, but bufferCapacityInput is greater than 0");
            xr_result = XR_ERROR_VALIDATION_FAILURE;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == bufferCountOutput) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetInputSourceLocalizedName-bufferCountOutput-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetInputSourceLocalizedName", objects_info,
                                "Invalid NULL for uint32_t \"bufferCountOutput\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrGetInputSourceLocalizedName-bufferCountOutput-parameter" type
        // NOTE: Can't validate "VUID-xrGetInputSourceLocalizedName-buffer-parameter" type
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrGetInputSourceLocalizedName(
    XrSession session,
    const XrInputSourceLocalizedNameGetInfo* getInfo,
    uint32_t bufferCapacityInput,
    uint32_t* bufferCountOutput,
    char* buffer) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->GetInputSourceLocalizedName(session, getInfo, bufferCapacityInput, bufferCountOutput, buffer);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrGetInputSourceLocalizedName(
    XrSession session,
    const XrInputSourceLocalizedNameGetInfo* getInfo,
    uint32_t bufferCapacityInput,
    uint32_t* bufferCountOutput,
    char* buffer) {
    XrResult test_result = GenValidUsageInputsXrGetInputSourceLocalizedName(session, getInfo, bufferCapacityInput, bufferCountOutput, buffer);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrGetInputSourceLocalizedName(session, getInfo, bufferCapacityInput, bufferCountOutput, buffer);
}

XrResult GenValidUsageInputsXrApplyHapticFeedback(
XrSession session,
const XrHapticActionInfo* hapticActionInfo,
const XrHapticBaseHeader* hapticFeedback) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrApplyHapticFeedback-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrApplyHapticFeedback",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == hapticActionInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrApplyHapticFeedback-hapticActionInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrApplyHapticFeedback", objects_info,
                                "Invalid NULL for XrHapticActionInfo \"hapticActionInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrHapticActionInfo is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrApplyHapticFeedback", objects_info,
                                                        true, hapticActionInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrApplyHapticFeedback-hapticActionInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrApplyHapticFeedback",
                                objects_info,
                                "Command xrApplyHapticFeedback param hapticActionInfo is invalid");
            return xr_result;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == hapticFeedback) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrApplyHapticFeedback-hapticFeedback-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrApplyHapticFeedback", objects_info,
                                "Invalid NULL for XrHapticBaseHeader \"hapticFeedback\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate if XrHapticBaseHeader is a child structure of type XrHapticVibration and it is valid
        const XrHapticVibration* new_hapticvibration_value = reinterpret_cast<const XrHapticVibration*>(hapticFeedback);
        if (new_hapticvibration_value->type == XR_TYPE_HAPTIC_VIBRATION) {
            xr_result = ValidateXrStruct(gen_instance_info, "xrApplyHapticFeedback",
                                                            objects_info,false, new_hapticvibration_value);
            if (XR_SUCCESS != xr_result) {
                std::string error_message = "Command xrApplyHapticFeedback param hapticFeedback";
                error_message += " is invalid";
                CoreValidLogMessage(gen_instance_info, "VUID-xrApplyHapticFeedback-hapticFeedback-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrApplyHapticFeedback",
                                    objects_info,
                                    error_message);
                return XR_ERROR_VALIDATION_FAILURE;
            }
        }
        // Validate that the base-structure XrHapticBaseHeader is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrApplyHapticFeedback", objects_info,
                                                        true, hapticFeedback);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrApplyHapticFeedback-hapticFeedback-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrApplyHapticFeedback",
                                objects_info,
                                "Command xrApplyHapticFeedback param hapticFeedback is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrApplyHapticFeedback(
    XrSession session,
    const XrHapticActionInfo* hapticActionInfo,
    const XrHapticBaseHeader* hapticFeedback) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->ApplyHapticFeedback(session, hapticActionInfo, hapticFeedback);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrApplyHapticFeedback(
    XrSession session,
    const XrHapticActionInfo* hapticActionInfo,
    const XrHapticBaseHeader* hapticFeedback) {
    XrResult test_result = GenValidUsageInputsXrApplyHapticFeedback(session, hapticActionInfo, hapticFeedback);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrApplyHapticFeedback(session, hapticActionInfo, hapticFeedback);
}

XrResult GenValidUsageInputsXrStopHapticFeedback(
XrSession session,
const XrHapticActionInfo* hapticActionInfo) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrStopHapticFeedback-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrStopHapticFeedback",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == hapticActionInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrStopHapticFeedback-hapticActionInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrStopHapticFeedback", objects_info,
                                "Invalid NULL for XrHapticActionInfo \"hapticActionInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrHapticActionInfo is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrStopHapticFeedback", objects_info,
                                                        true, hapticActionInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrStopHapticFeedback-hapticActionInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrStopHapticFeedback",
                                objects_info,
                                "Command xrStopHapticFeedback param hapticActionInfo is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrStopHapticFeedback(
    XrSession session,
    const XrHapticActionInfo* hapticActionInfo) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->StopHapticFeedback(session, hapticActionInfo);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrStopHapticFeedback(
    XrSession session,
    const XrHapticActionInfo* hapticActionInfo) {
    XrResult test_result = GenValidUsageInputsXrStopHapticFeedback(session, hapticActionInfo);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrStopHapticFeedback(session, hapticActionInfo);
}


// ---- XR_KHR_android_thread_settings extension commands
#if defined(XR_USE_PLATFORM_ANDROID)

XrResult GenValidUsageInputsXrSetAndroidApplicationThreadKHR(
XrSession session,
XrAndroidThreadTypeKHR threadType,
uint32_t threadId) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrSetAndroidApplicationThreadKHR-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSetAndroidApplicationThreadKHR",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Make sure the enum type XrAndroidThreadTypeKHR value is valid
        if (!ValidateXrEnum(gen_instance_info, "xrSetAndroidApplicationThreadKHR", "xrSetAndroidApplicationThreadKHR", "threadType", objects_info, threadType)) {
            std::ostringstream oss_enum;
            oss_enum << "Invalid XrAndroidThreadTypeKHR \"threadType\" enum value ";
            oss_enum << Uint32ToHexString(static_cast<uint32_t>(threadType));
            CoreValidLogMessage(gen_instance_info, "VUID-xrSetAndroidApplicationThreadKHR-threadType-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSetAndroidApplicationThreadKHR",
                                objects_info, oss_enum.str());
            return XR_ERROR_VALIDATION_FAILURE;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrSetAndroidApplicationThreadKHR(
    XrSession session,
    XrAndroidThreadTypeKHR threadType,
    uint32_t threadId) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->SetAndroidApplicationThreadKHR(session, threadType, threadId);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrSetAndroidApplicationThreadKHR(
    XrSession session,
    XrAndroidThreadTypeKHR threadType,
    uint32_t threadId) {
    XrResult test_result = GenValidUsageInputsXrSetAndroidApplicationThreadKHR(session, threadType, threadId);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrSetAndroidApplicationThreadKHR(session, threadType, threadId);
}

#endif // defined(XR_USE_PLATFORM_ANDROID)


// ---- XR_KHR_android_surface_swapchain extension commands
#if defined(XR_USE_PLATFORM_ANDROID)

XrResult GenValidUsageInputsXrCreateSwapchainAndroidSurfaceKHR(
XrSession session,
const XrSwapchainCreateInfo* info,
XrSwapchain* swapchain,
jobject* surface) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrCreateSwapchainAndroidSurfaceKHR-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateSwapchainAndroidSurfaceKHR",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == info) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateSwapchainAndroidSurfaceKHR-info-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateSwapchainAndroidSurfaceKHR", objects_info,
                                "Invalid NULL for XrSwapchainCreateInfo \"info\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrSwapchainCreateInfo is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrCreateSwapchainAndroidSurfaceKHR", objects_info,
                                                        true, info);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateSwapchainAndroidSurfaceKHR-info-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateSwapchainAndroidSurfaceKHR",
                                objects_info,
                                "Command xrCreateSwapchainAndroidSurfaceKHR param info is invalid");
            return xr_result;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == swapchain) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateSwapchainAndroidSurfaceKHR-swapchain-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateSwapchainAndroidSurfaceKHR", objects_info,
                                "Invalid NULL for XrSwapchain \"swapchain\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == surface) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateSwapchainAndroidSurfaceKHR-surface-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateSwapchainAndroidSurfaceKHR", objects_info,
                                "Invalid NULL for jobject \"surface\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrCreateSwapchainAndroidSurfaceKHR-surface-parameter" type
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrCreateSwapchainAndroidSurfaceKHR(
    XrSession session,
    const XrSwapchainCreateInfo* info,
    XrSwapchain* swapchain,
    jobject* surface) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->CreateSwapchainAndroidSurfaceKHR(session, info, swapchain, surface);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrCreateSwapchainAndroidSurfaceKHR(
    XrSession session,
    const XrSwapchainCreateInfo* info,
    XrSwapchain* swapchain,
    jobject* surface) {
    XrResult test_result = GenValidUsageInputsXrCreateSwapchainAndroidSurfaceKHR(session, info, swapchain, surface);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrCreateSwapchainAndroidSurfaceKHR(session, info, swapchain, surface);
}

#endif // defined(XR_USE_PLATFORM_ANDROID)


// ---- XR_KHR_opengl_enable extension commands
#if defined(XR_USE_GRAPHICS_API_OPENGL)

XrResult GenValidUsageInputsXrGetOpenGLGraphicsRequirementsKHR(
XrInstance instance,
XrSystemId systemId,
XrGraphicsRequirementsOpenGLKHR* graphicsRequirements) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrGetOpenGLGraphicsRequirementsKHR-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetOpenGLGraphicsRequirementsKHR",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == graphicsRequirements) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetOpenGLGraphicsRequirementsKHR-graphicsRequirements-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetOpenGLGraphicsRequirementsKHR", objects_info,
                                "Invalid NULL for XrGraphicsRequirementsOpenGLKHR \"graphicsRequirements\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrGraphicsRequirementsOpenGLKHR is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrGetOpenGLGraphicsRequirementsKHR", objects_info,
                                                        false, graphicsRequirements);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetOpenGLGraphicsRequirementsKHR-graphicsRequirements-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetOpenGLGraphicsRequirementsKHR",
                                objects_info,
                                "Command xrGetOpenGLGraphicsRequirementsKHR param graphicsRequirements is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrGetOpenGLGraphicsRequirementsKHR(
    XrInstance instance,
    XrSystemId systemId,
    XrGraphicsRequirementsOpenGLKHR* graphicsRequirements) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->GetOpenGLGraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrGetOpenGLGraphicsRequirementsKHR(
    XrInstance instance,
    XrSystemId systemId,
    XrGraphicsRequirementsOpenGLKHR* graphicsRequirements) {
    XrResult test_result = GenValidUsageInputsXrGetOpenGLGraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrGetOpenGLGraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
}

#endif // defined(XR_USE_GRAPHICS_API_OPENGL)


// ---- XR_KHR_opengl_es_enable extension commands
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)

XrResult GenValidUsageInputsXrGetOpenGLESGraphicsRequirementsKHR(
XrInstance instance,
XrSystemId systemId,
XrGraphicsRequirementsOpenGLESKHR* graphicsRequirements) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrGetOpenGLESGraphicsRequirementsKHR-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetOpenGLESGraphicsRequirementsKHR",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == graphicsRequirements) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetOpenGLESGraphicsRequirementsKHR-graphicsRequirements-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetOpenGLESGraphicsRequirementsKHR", objects_info,
                                "Invalid NULL for XrGraphicsRequirementsOpenGLESKHR \"graphicsRequirements\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrGraphicsRequirementsOpenGLESKHR is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrGetOpenGLESGraphicsRequirementsKHR", objects_info,
                                                        false, graphicsRequirements);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetOpenGLESGraphicsRequirementsKHR-graphicsRequirements-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetOpenGLESGraphicsRequirementsKHR",
                                objects_info,
                                "Command xrGetOpenGLESGraphicsRequirementsKHR param graphicsRequirements is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrGetOpenGLESGraphicsRequirementsKHR(
    XrInstance instance,
    XrSystemId systemId,
    XrGraphicsRequirementsOpenGLESKHR* graphicsRequirements) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->GetOpenGLESGraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrGetOpenGLESGraphicsRequirementsKHR(
    XrInstance instance,
    XrSystemId systemId,
    XrGraphicsRequirementsOpenGLESKHR* graphicsRequirements) {
    XrResult test_result = GenValidUsageInputsXrGetOpenGLESGraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrGetOpenGLESGraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
}

#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES)


// ---- XR_KHR_vulkan_enable extension commands
#if defined(XR_USE_GRAPHICS_API_VULKAN)

XrResult GenValidUsageInputsXrGetVulkanInstanceExtensionsKHR(
XrInstance instance,
XrSystemId systemId,
uint32_t bufferCapacityInput,
uint32_t* bufferCountOutput,
char* buffer) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrGetVulkanInstanceExtensionsKHR-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetVulkanInstanceExtensionsKHR",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Optional array must be non-NULL when bufferCapacityInput is non-zero
        if (0 != bufferCapacityInput && nullptr == buffer) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetVulkanInstanceExtensionsKHR-buffer-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetVulkanInstanceExtensionsKHR",
                                objects_info,
                                "Command xrGetVulkanInstanceExtensionsKHR param buffer is NULL, but bufferCapacityInput is greater than 0");
            xr_result = XR_ERROR_VALIDATION_FAILURE;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == bufferCountOutput) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetVulkanInstanceExtensionsKHR-bufferCountOutput-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetVulkanInstanceExtensionsKHR", objects_info,
                                "Invalid NULL for uint32_t \"bufferCountOutput\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrGetVulkanInstanceExtensionsKHR-bufferCountOutput-parameter" type
        // NOTE: Can't validate "VUID-xrGetVulkanInstanceExtensionsKHR-buffer-parameter" null-termination
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrGetVulkanInstanceExtensionsKHR(
    XrInstance instance,
    XrSystemId systemId,
    uint32_t bufferCapacityInput,
    uint32_t* bufferCountOutput,
    char* buffer) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->GetVulkanInstanceExtensionsKHR(instance, systemId, bufferCapacityInput, bufferCountOutput, buffer);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrGetVulkanInstanceExtensionsKHR(
    XrInstance instance,
    XrSystemId systemId,
    uint32_t bufferCapacityInput,
    uint32_t* bufferCountOutput,
    char* buffer) {
    XrResult test_result = GenValidUsageInputsXrGetVulkanInstanceExtensionsKHR(instance, systemId, bufferCapacityInput, bufferCountOutput, buffer);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrGetVulkanInstanceExtensionsKHR(instance, systemId, bufferCapacityInput, bufferCountOutput, buffer);
}

#endif // defined(XR_USE_GRAPHICS_API_VULKAN)

#if defined(XR_USE_GRAPHICS_API_VULKAN)

XrResult GenValidUsageInputsXrGetVulkanDeviceExtensionsKHR(
XrInstance instance,
XrSystemId systemId,
uint32_t bufferCapacityInput,
uint32_t* bufferCountOutput,
char* buffer) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrGetVulkanDeviceExtensionsKHR-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetVulkanDeviceExtensionsKHR",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Optional array must be non-NULL when bufferCapacityInput is non-zero
        if (0 != bufferCapacityInput && nullptr == buffer) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetVulkanDeviceExtensionsKHR-buffer-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetVulkanDeviceExtensionsKHR",
                                objects_info,
                                "Command xrGetVulkanDeviceExtensionsKHR param buffer is NULL, but bufferCapacityInput is greater than 0");
            xr_result = XR_ERROR_VALIDATION_FAILURE;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == bufferCountOutput) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetVulkanDeviceExtensionsKHR-bufferCountOutput-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetVulkanDeviceExtensionsKHR", objects_info,
                                "Invalid NULL for uint32_t \"bufferCountOutput\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrGetVulkanDeviceExtensionsKHR-bufferCountOutput-parameter" type
        // NOTE: Can't validate "VUID-xrGetVulkanDeviceExtensionsKHR-buffer-parameter" null-termination
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrGetVulkanDeviceExtensionsKHR(
    XrInstance instance,
    XrSystemId systemId,
    uint32_t bufferCapacityInput,
    uint32_t* bufferCountOutput,
    char* buffer) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->GetVulkanDeviceExtensionsKHR(instance, systemId, bufferCapacityInput, bufferCountOutput, buffer);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrGetVulkanDeviceExtensionsKHR(
    XrInstance instance,
    XrSystemId systemId,
    uint32_t bufferCapacityInput,
    uint32_t* bufferCountOutput,
    char* buffer) {
    XrResult test_result = GenValidUsageInputsXrGetVulkanDeviceExtensionsKHR(instance, systemId, bufferCapacityInput, bufferCountOutput, buffer);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrGetVulkanDeviceExtensionsKHR(instance, systemId, bufferCapacityInput, bufferCountOutput, buffer);
}

#endif // defined(XR_USE_GRAPHICS_API_VULKAN)

#if defined(XR_USE_GRAPHICS_API_VULKAN)

XrResult GenValidUsageInputsXrGetVulkanGraphicsDeviceKHR(
XrInstance instance,
XrSystemId systemId,
VkInstance vkInstance,
VkPhysicalDevice* vkPhysicalDevice) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrGetVulkanGraphicsDeviceKHR-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetVulkanGraphicsDeviceKHR",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == vkPhysicalDevice) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetVulkanGraphicsDeviceKHR-vkPhysicalDevice-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetVulkanGraphicsDeviceKHR", objects_info,
                                "Invalid NULL for VkPhysicalDevice \"vkPhysicalDevice\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrGetVulkanGraphicsDeviceKHR-vkPhysicalDevice-parameter" type
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrGetVulkanGraphicsDeviceKHR(
    XrInstance instance,
    XrSystemId systemId,
    VkInstance vkInstance,
    VkPhysicalDevice* vkPhysicalDevice) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->GetVulkanGraphicsDeviceKHR(instance, systemId, vkInstance, vkPhysicalDevice);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrGetVulkanGraphicsDeviceKHR(
    XrInstance instance,
    XrSystemId systemId,
    VkInstance vkInstance,
    VkPhysicalDevice* vkPhysicalDevice) {
    XrResult test_result = GenValidUsageInputsXrGetVulkanGraphicsDeviceKHR(instance, systemId, vkInstance, vkPhysicalDevice);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrGetVulkanGraphicsDeviceKHR(instance, systemId, vkInstance, vkPhysicalDevice);
}

#endif // defined(XR_USE_GRAPHICS_API_VULKAN)

#if defined(XR_USE_GRAPHICS_API_VULKAN)

XrResult GenValidUsageInputsXrGetVulkanGraphicsRequirementsKHR(
XrInstance instance,
XrSystemId systemId,
XrGraphicsRequirementsVulkanKHR* graphicsRequirements) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrGetVulkanGraphicsRequirementsKHR-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetVulkanGraphicsRequirementsKHR",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == graphicsRequirements) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetVulkanGraphicsRequirementsKHR-graphicsRequirements-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetVulkanGraphicsRequirementsKHR", objects_info,
                                "Invalid NULL for XrGraphicsRequirementsVulkanKHR \"graphicsRequirements\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrGraphicsRequirementsVulkanKHR is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrGetVulkanGraphicsRequirementsKHR", objects_info,
                                                        false, graphicsRequirements);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetVulkanGraphicsRequirementsKHR-graphicsRequirements-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetVulkanGraphicsRequirementsKHR",
                                objects_info,
                                "Command xrGetVulkanGraphicsRequirementsKHR param graphicsRequirements is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrGetVulkanGraphicsRequirementsKHR(
    XrInstance instance,
    XrSystemId systemId,
    XrGraphicsRequirementsVulkanKHR* graphicsRequirements) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->GetVulkanGraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrGetVulkanGraphicsRequirementsKHR(
    XrInstance instance,
    XrSystemId systemId,
    XrGraphicsRequirementsVulkanKHR* graphicsRequirements) {
    XrResult test_result = GenValidUsageInputsXrGetVulkanGraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrGetVulkanGraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
}

#endif // defined(XR_USE_GRAPHICS_API_VULKAN)


// ---- XR_KHR_D3D11_enable extension commands
#if defined(XR_USE_GRAPHICS_API_D3D11)

XrResult GenValidUsageInputsXrGetD3D11GraphicsRequirementsKHR(
XrInstance instance,
XrSystemId systemId,
XrGraphicsRequirementsD3D11KHR* graphicsRequirements) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrGetD3D11GraphicsRequirementsKHR-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetD3D11GraphicsRequirementsKHR",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == graphicsRequirements) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetD3D11GraphicsRequirementsKHR-graphicsRequirements-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetD3D11GraphicsRequirementsKHR", objects_info,
                                "Invalid NULL for XrGraphicsRequirementsD3D11KHR \"graphicsRequirements\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrGraphicsRequirementsD3D11KHR is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrGetD3D11GraphicsRequirementsKHR", objects_info,
                                                        false, graphicsRequirements);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetD3D11GraphicsRequirementsKHR-graphicsRequirements-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetD3D11GraphicsRequirementsKHR",
                                objects_info,
                                "Command xrGetD3D11GraphicsRequirementsKHR param graphicsRequirements is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrGetD3D11GraphicsRequirementsKHR(
    XrInstance instance,
    XrSystemId systemId,
    XrGraphicsRequirementsD3D11KHR* graphicsRequirements) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->GetD3D11GraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrGetD3D11GraphicsRequirementsKHR(
    XrInstance instance,
    XrSystemId systemId,
    XrGraphicsRequirementsD3D11KHR* graphicsRequirements) {
    XrResult test_result = GenValidUsageInputsXrGetD3D11GraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrGetD3D11GraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
}

#endif // defined(XR_USE_GRAPHICS_API_D3D11)


// ---- XR_KHR_D3D12_enable extension commands
#if defined(XR_USE_GRAPHICS_API_D3D12)

XrResult GenValidUsageInputsXrGetD3D12GraphicsRequirementsKHR(
XrInstance instance,
XrSystemId systemId,
XrGraphicsRequirementsD3D12KHR* graphicsRequirements) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrGetD3D12GraphicsRequirementsKHR-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetD3D12GraphicsRequirementsKHR",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == graphicsRequirements) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetD3D12GraphicsRequirementsKHR-graphicsRequirements-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetD3D12GraphicsRequirementsKHR", objects_info,
                                "Invalid NULL for XrGraphicsRequirementsD3D12KHR \"graphicsRequirements\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrGraphicsRequirementsD3D12KHR is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrGetD3D12GraphicsRequirementsKHR", objects_info,
                                                        false, graphicsRequirements);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetD3D12GraphicsRequirementsKHR-graphicsRequirements-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetD3D12GraphicsRequirementsKHR",
                                objects_info,
                                "Command xrGetD3D12GraphicsRequirementsKHR param graphicsRequirements is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrGetD3D12GraphicsRequirementsKHR(
    XrInstance instance,
    XrSystemId systemId,
    XrGraphicsRequirementsD3D12KHR* graphicsRequirements) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->GetD3D12GraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrGetD3D12GraphicsRequirementsKHR(
    XrInstance instance,
    XrSystemId systemId,
    XrGraphicsRequirementsD3D12KHR* graphicsRequirements) {
    XrResult test_result = GenValidUsageInputsXrGetD3D12GraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrGetD3D12GraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
}

#endif // defined(XR_USE_GRAPHICS_API_D3D12)


// ---- XR_KHR_visibility_mask extension commands
XrResult GenValidUsageInputsXrGetVisibilityMaskKHR(
XrSession session,
XrViewConfigurationType viewConfigurationType,
uint32_t viewIndex,
XrVisibilityMaskTypeKHR visibilityMaskType,
XrVisibilityMaskKHR* visibilityMask) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrGetVisibilityMaskKHR-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetVisibilityMaskKHR",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Make sure the enum type XrViewConfigurationType value is valid
        if (!ValidateXrEnum(gen_instance_info, "xrGetVisibilityMaskKHR", "xrGetVisibilityMaskKHR", "viewConfigurationType", objects_info, viewConfigurationType)) {
            std::ostringstream oss_enum;
            oss_enum << "Invalid XrViewConfigurationType \"viewConfigurationType\" enum value ";
            oss_enum << Uint32ToHexString(static_cast<uint32_t>(viewConfigurationType));
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetVisibilityMaskKHR-viewConfigurationType-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetVisibilityMaskKHR",
                                objects_info, oss_enum.str());
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Make sure the enum type XrVisibilityMaskTypeKHR value is valid
        if (!ValidateXrEnum(gen_instance_info, "xrGetVisibilityMaskKHR", "xrGetVisibilityMaskKHR", "visibilityMaskType", objects_info, visibilityMaskType)) {
            std::ostringstream oss_enum;
            oss_enum << "Invalid XrVisibilityMaskTypeKHR \"visibilityMaskType\" enum value ";
            oss_enum << Uint32ToHexString(static_cast<uint32_t>(visibilityMaskType));
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetVisibilityMaskKHR-visibilityMaskType-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetVisibilityMaskKHR",
                                objects_info, oss_enum.str());
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == visibilityMask) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetVisibilityMaskKHR-visibilityMask-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetVisibilityMaskKHR", objects_info,
                                "Invalid NULL for XrVisibilityMaskKHR \"visibilityMask\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrVisibilityMaskKHR is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrGetVisibilityMaskKHR", objects_info,
                                                        false, visibilityMask);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrGetVisibilityMaskKHR-visibilityMask-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetVisibilityMaskKHR",
                                objects_info,
                                "Command xrGetVisibilityMaskKHR param visibilityMask is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrGetVisibilityMaskKHR(
    XrSession session,
    XrViewConfigurationType viewConfigurationType,
    uint32_t viewIndex,
    XrVisibilityMaskTypeKHR visibilityMaskType,
    XrVisibilityMaskKHR* visibilityMask) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->GetVisibilityMaskKHR(session, viewConfigurationType, viewIndex, visibilityMaskType, visibilityMask);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrGetVisibilityMaskKHR(
    XrSession session,
    XrViewConfigurationType viewConfigurationType,
    uint32_t viewIndex,
    XrVisibilityMaskTypeKHR visibilityMaskType,
    XrVisibilityMaskKHR* visibilityMask) {
    XrResult test_result = GenValidUsageInputsXrGetVisibilityMaskKHR(session, viewConfigurationType, viewIndex, visibilityMaskType, visibilityMask);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrGetVisibilityMaskKHR(session, viewConfigurationType, viewIndex, visibilityMaskType, visibilityMask);
}


// ---- XR_KHR_win32_convert_performance_counter_time extension commands
#if defined(XR_USE_PLATFORM_WIN32)

XrResult GenValidUsageInputsXrConvertWin32PerformanceCounterToTimeKHR(
XrInstance instance,
const LARGE_INTEGER* performanceCounter,
XrTime* time) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrConvertWin32PerformanceCounterToTimeKHR-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrConvertWin32PerformanceCounterToTimeKHR",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == performanceCounter) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrConvertWin32PerformanceCounterToTimeKHR-performanceCounter-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrConvertWin32PerformanceCounterToTimeKHR", objects_info,
                                "Invalid NULL for LARGE_INTEGER \"performanceCounter\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrConvertWin32PerformanceCounterToTimeKHR-performanceCounter-parameter" type
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == time) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrConvertWin32PerformanceCounterToTimeKHR-time-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrConvertWin32PerformanceCounterToTimeKHR", objects_info,
                                "Invalid NULL for XrTime \"time\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrConvertWin32PerformanceCounterToTimeKHR-time-parameter" type
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrConvertWin32PerformanceCounterToTimeKHR(
    XrInstance instance,
    const LARGE_INTEGER* performanceCounter,
    XrTime* time) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->ConvertWin32PerformanceCounterToTimeKHR(instance, performanceCounter, time);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrConvertWin32PerformanceCounterToTimeKHR(
    XrInstance instance,
    const LARGE_INTEGER* performanceCounter,
    XrTime* time) {
    XrResult test_result = GenValidUsageInputsXrConvertWin32PerformanceCounterToTimeKHR(instance, performanceCounter, time);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrConvertWin32PerformanceCounterToTimeKHR(instance, performanceCounter, time);
}

#endif // defined(XR_USE_PLATFORM_WIN32)

#if defined(XR_USE_PLATFORM_WIN32)

XrResult GenValidUsageInputsXrConvertTimeToWin32PerformanceCounterKHR(
XrInstance instance,
XrTime   time,
LARGE_INTEGER* performanceCounter) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrConvertTimeToWin32PerformanceCounterKHR-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrConvertTimeToWin32PerformanceCounterKHR",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == performanceCounter) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrConvertTimeToWin32PerformanceCounterKHR-performanceCounter-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrConvertTimeToWin32PerformanceCounterKHR", objects_info,
                                "Invalid NULL for LARGE_INTEGER \"performanceCounter\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrConvertTimeToWin32PerformanceCounterKHR-performanceCounter-parameter" type
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrConvertTimeToWin32PerformanceCounterKHR(
    XrInstance instance,
    XrTime   time,
    LARGE_INTEGER* performanceCounter) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->ConvertTimeToWin32PerformanceCounterKHR(instance, time, performanceCounter);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrConvertTimeToWin32PerformanceCounterKHR(
    XrInstance instance,
    XrTime   time,
    LARGE_INTEGER* performanceCounter) {
    XrResult test_result = GenValidUsageInputsXrConvertTimeToWin32PerformanceCounterKHR(instance, time, performanceCounter);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrConvertTimeToWin32PerformanceCounterKHR(instance, time, performanceCounter);
}

#endif // defined(XR_USE_PLATFORM_WIN32)


// ---- XR_KHR_convert_timespec_time extension commands
#if defined(XR_USE_TIMESPEC)

XrResult GenValidUsageInputsXrConvertTimespecTimeToTimeKHR(
XrInstance instance,
const struct timespec* timespecTime,
XrTime* time) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrConvertTimespecTimeToTimeKHR-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrConvertTimespecTimeToTimeKHR",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == timespecTime) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrConvertTimespecTimeToTimeKHR-timespecTime-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrConvertTimespecTimeToTimeKHR", objects_info,
                                "Invalid NULL for timespec \"timespecTime\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrConvertTimespecTimeToTimeKHR-timespecTime-parameter" type
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == time) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrConvertTimespecTimeToTimeKHR-time-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrConvertTimespecTimeToTimeKHR", objects_info,
                                "Invalid NULL for XrTime \"time\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrConvertTimespecTimeToTimeKHR-time-parameter" type
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrConvertTimespecTimeToTimeKHR(
    XrInstance instance,
    const struct timespec* timespecTime,
    XrTime* time) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->ConvertTimespecTimeToTimeKHR(instance, timespecTime, time);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrConvertTimespecTimeToTimeKHR(
    XrInstance instance,
    const struct timespec* timespecTime,
    XrTime* time) {
    XrResult test_result = GenValidUsageInputsXrConvertTimespecTimeToTimeKHR(instance, timespecTime, time);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrConvertTimespecTimeToTimeKHR(instance, timespecTime, time);
}

#endif // defined(XR_USE_TIMESPEC)

#if defined(XR_USE_TIMESPEC)

XrResult GenValidUsageInputsXrConvertTimeToTimespecTimeKHR(
XrInstance instance,
XrTime   time,
struct timespec* timespecTime) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrConvertTimeToTimespecTimeKHR-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrConvertTimeToTimespecTimeKHR",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == timespecTime) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrConvertTimeToTimespecTimeKHR-timespecTime-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrConvertTimeToTimespecTimeKHR", objects_info,
                                "Invalid NULL for timespec \"timespecTime\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrConvertTimeToTimespecTimeKHR-timespecTime-parameter" type
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrConvertTimeToTimespecTimeKHR(
    XrInstance instance,
    XrTime   time,
    struct timespec* timespecTime) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->ConvertTimeToTimespecTimeKHR(instance, time, timespecTime);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrConvertTimeToTimespecTimeKHR(
    XrInstance instance,
    XrTime   time,
    struct timespec* timespecTime) {
    XrResult test_result = GenValidUsageInputsXrConvertTimeToTimespecTimeKHR(instance, time, timespecTime);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrConvertTimeToTimespecTimeKHR(instance, time, timespecTime);
}

#endif // defined(XR_USE_TIMESPEC)


// ---- XR_EXT_performance_settings extension commands
XrResult GenValidUsageInputsXrPerfSettingsSetPerformanceLevelEXT(
XrSession session,
XrPerfSettingsDomainEXT domain,
XrPerfSettingsLevelEXT level) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrPerfSettingsSetPerformanceLevelEXT-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrPerfSettingsSetPerformanceLevelEXT",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Make sure the enum type XrPerfSettingsDomainEXT value is valid
        if (!ValidateXrEnum(gen_instance_info, "xrPerfSettingsSetPerformanceLevelEXT", "xrPerfSettingsSetPerformanceLevelEXT", "domain", objects_info, domain)) {
            std::ostringstream oss_enum;
            oss_enum << "Invalid XrPerfSettingsDomainEXT \"domain\" enum value ";
            oss_enum << Uint32ToHexString(static_cast<uint32_t>(domain));
            CoreValidLogMessage(gen_instance_info, "VUID-xrPerfSettingsSetPerformanceLevelEXT-domain-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrPerfSettingsSetPerformanceLevelEXT",
                                objects_info, oss_enum.str());
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Make sure the enum type XrPerfSettingsLevelEXT value is valid
        if (!ValidateXrEnum(gen_instance_info, "xrPerfSettingsSetPerformanceLevelEXT", "xrPerfSettingsSetPerformanceLevelEXT", "level", objects_info, level)) {
            std::ostringstream oss_enum;
            oss_enum << "Invalid XrPerfSettingsLevelEXT \"level\" enum value ";
            oss_enum << Uint32ToHexString(static_cast<uint32_t>(level));
            CoreValidLogMessage(gen_instance_info, "VUID-xrPerfSettingsSetPerformanceLevelEXT-level-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrPerfSettingsSetPerformanceLevelEXT",
                                objects_info, oss_enum.str());
            return XR_ERROR_VALIDATION_FAILURE;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrPerfSettingsSetPerformanceLevelEXT(
    XrSession session,
    XrPerfSettingsDomainEXT domain,
    XrPerfSettingsLevelEXT level) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->PerfSettingsSetPerformanceLevelEXT(session, domain, level);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrPerfSettingsSetPerformanceLevelEXT(
    XrSession session,
    XrPerfSettingsDomainEXT domain,
    XrPerfSettingsLevelEXT level) {
    XrResult test_result = GenValidUsageInputsXrPerfSettingsSetPerformanceLevelEXT(session, domain, level);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrPerfSettingsSetPerformanceLevelEXT(session, domain, level);
}


// ---- XR_EXT_thermal_query extension commands
XrResult GenValidUsageInputsXrThermalGetTemperatureTrendEXT(
XrSession session,
XrPerfSettingsDomainEXT domain,
XrPerfSettingsNotificationLevelEXT* notificationLevel,
float* tempHeadroom,
float* tempSlope) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrThermalGetTemperatureTrendEXT-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrThermalGetTemperatureTrendEXT",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Make sure the enum type XrPerfSettingsDomainEXT value is valid
        if (!ValidateXrEnum(gen_instance_info, "xrThermalGetTemperatureTrendEXT", "xrThermalGetTemperatureTrendEXT", "domain", objects_info, domain)) {
            std::ostringstream oss_enum;
            oss_enum << "Invalid XrPerfSettingsDomainEXT \"domain\" enum value ";
            oss_enum << Uint32ToHexString(static_cast<uint32_t>(domain));
            CoreValidLogMessage(gen_instance_info, "VUID-xrThermalGetTemperatureTrendEXT-domain-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrThermalGetTemperatureTrendEXT",
                                objects_info, oss_enum.str());
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == notificationLevel) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrThermalGetTemperatureTrendEXT-notificationLevel-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrThermalGetTemperatureTrendEXT", objects_info,
                                "Invalid NULL for XrPerfSettingsNotificationLevelEXT \"notificationLevel\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Make sure the enum type XrPerfSettingsNotificationLevelEXT value is valid
        if (!ValidateXrEnum(gen_instance_info, "xrThermalGetTemperatureTrendEXT", "xrThermalGetTemperatureTrendEXT", "notificationLevel", objects_info, *notificationLevel)) {
            std::ostringstream oss_enum;
            oss_enum << "Invalid XrPerfSettingsNotificationLevelEXT \"notificationLevel\" enum value ";
            oss_enum << Uint32ToHexString(static_cast<uint32_t>(*notificationLevel));
            CoreValidLogMessage(gen_instance_info, "VUID-xrThermalGetTemperatureTrendEXT-notificationLevel-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrThermalGetTemperatureTrendEXT",
                                objects_info, oss_enum.str());
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == tempHeadroom) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrThermalGetTemperatureTrendEXT-tempHeadroom-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrThermalGetTemperatureTrendEXT", objects_info,
                                "Invalid NULL for float \"tempHeadroom\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrThermalGetTemperatureTrendEXT-tempHeadroom-parameter" type
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == tempSlope) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrThermalGetTemperatureTrendEXT-tempSlope-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrThermalGetTemperatureTrendEXT", objects_info,
                                "Invalid NULL for float \"tempSlope\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // NOTE: Can't validate "VUID-xrThermalGetTemperatureTrendEXT-tempSlope-parameter" type
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrThermalGetTemperatureTrendEXT(
    XrSession session,
    XrPerfSettingsDomainEXT domain,
    XrPerfSettingsNotificationLevelEXT* notificationLevel,
    float* tempHeadroom,
    float* tempSlope) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->ThermalGetTemperatureTrendEXT(session, domain, notificationLevel, tempHeadroom, tempSlope);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageXrThermalGetTemperatureTrendEXT(
    XrSession session,
    XrPerfSettingsDomainEXT domain,
    XrPerfSettingsNotificationLevelEXT* notificationLevel,
    float* tempHeadroom,
    float* tempSlope) {
    XrResult test_result = GenValidUsageInputsXrThermalGetTemperatureTrendEXT(session, domain, notificationLevel, tempHeadroom, tempSlope);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrThermalGetTemperatureTrendEXT(session, domain, notificationLevel, tempHeadroom, tempSlope);
}


// ---- XR_EXT_debug_utils extension commands
XrResult GenValidUsageInputsXrSetDebugUtilsObjectNameEXT(
XrInstance instance,
const XrDebugUtilsObjectNameInfoEXT* nameInfo) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrSetDebugUtilsObjectNameEXT-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSetDebugUtilsObjectNameEXT",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == nameInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrSetDebugUtilsObjectNameEXT-nameInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSetDebugUtilsObjectNameEXT", objects_info,
                                "Invalid NULL for XrDebugUtilsObjectNameInfoEXT \"nameInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrDebugUtilsObjectNameInfoEXT is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrSetDebugUtilsObjectNameEXT", objects_info,
                                                        true, nameInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrSetDebugUtilsObjectNameEXT-nameInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSetDebugUtilsObjectNameEXT",
                                objects_info,
                                "Command xrSetDebugUtilsObjectNameEXT param nameInfo is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrSetDebugUtilsObjectNameEXT(
    XrInstance instance,
    const XrDebugUtilsObjectNameInfoEXT* nameInfo) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->SetDebugUtilsObjectNameEXT(instance, nameInfo);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageInputsXrCreateDebugUtilsMessengerEXT(
XrInstance instance,
const XrDebugUtilsMessengerCreateInfoEXT* createInfo,
XrDebugUtilsMessengerEXT* messenger) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrCreateDebugUtilsMessengerEXT-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateDebugUtilsMessengerEXT",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == createInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateDebugUtilsMessengerEXT-createInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateDebugUtilsMessengerEXT", objects_info,
                                "Invalid NULL for XrDebugUtilsMessengerCreateInfoEXT \"createInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrDebugUtilsMessengerCreateInfoEXT is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrCreateDebugUtilsMessengerEXT", objects_info,
                                                        true, createInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateDebugUtilsMessengerEXT-createInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateDebugUtilsMessengerEXT",
                                objects_info,
                                "Command xrCreateDebugUtilsMessengerEXT param createInfo is invalid");
            return xr_result;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == messenger) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrCreateDebugUtilsMessengerEXT-messenger-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrCreateDebugUtilsMessengerEXT", objects_info,
                                "Invalid NULL for XrDebugUtilsMessengerEXT \"messenger\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrCreateDebugUtilsMessengerEXT(
    XrInstance instance,
    const XrDebugUtilsMessengerCreateInfoEXT* createInfo,
    XrDebugUtilsMessengerEXT* messenger) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->CreateDebugUtilsMessengerEXT(instance, createInfo, messenger);
        if (XR_SUCCESS == result && nullptr != messenger) {
            std::unique_ptr<GenValidUsageXrHandleInfo> handle_info(new GenValidUsageXrHandleInfo());
            handle_info->instance_info = gen_instance_info;
            handle_info->direct_parent_type = XR_OBJECT_TYPE_INSTANCE;
            handle_info->direct_parent_handle = MakeHandleGeneric(instance);
            g_debugutilsmessengerext_info.insert(*messenger, std::move(handle_info));
        }
    } catch (std::bad_alloc&) {
        result = XR_ERROR_OUT_OF_MEMORY;
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageInputsXrDestroyDebugUtilsMessengerEXT(
XrDebugUtilsMessengerEXT messenger) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(messenger, XR_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrDebugUtilsMessengerEXTHandle(&messenger);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrDebugUtilsMessengerEXT handle \"messenger\" ";
                oss << HandleToHexString(messenger);
                CoreValidLogMessage(nullptr, "VUID-xrDestroyDebugUtilsMessengerEXT-messenger-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrDestroyDebugUtilsMessengerEXT",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_debugutilsmessengerext_info.getWithInstanceInfo(messenger);
        GenValidUsageXrHandleInfo *gen_debugutilsmessengerext_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrDestroyDebugUtilsMessengerEXT(
    XrDebugUtilsMessengerEXT messenger) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_debugutilsmessengerext_info.getWithInstanceInfo(messenger);
        GenValidUsageXrHandleInfo *gen_debugutilsmessengerext_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->DestroyDebugUtilsMessengerEXT(messenger);
        if (XR_SUCCEEDED(result)) {
            g_debugutilsmessengerext_info.erase(messenger);
        }
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageInputsXrSubmitDebugUtilsMessageEXT(
XrInstance                                  instance,
XrDebugUtilsMessageSeverityFlagsEXT         messageSeverity,
XrDebugUtilsMessageTypeFlagsEXT             messageTypes,
const XrDebugUtilsMessengerCallbackDataEXT* callbackData) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(instance, XR_OBJECT_TYPE_INSTANCE);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrInstanceHandle(&instance);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrInstance handle \"instance\" ";
                oss << HandleToHexString(instance);
                CoreValidLogMessage(nullptr, "VUID-xrSubmitDebugUtilsMessageEXT-instance-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSubmitDebugUtilsMessageEXT",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        ValidateXrFlagsResult debug_utils_message_severity_flags_ext_result = ValidateXrDebugUtilsMessageSeverityFlagsEXT(messageSeverity);
        // Flags must be non-zero in this case.
        if (VALIDATE_XR_FLAGS_ZERO == debug_utils_message_severity_flags_ext_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrSubmitDebugUtilsMessageEXT-messageSeverity-requiredbitmask",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSubmitDebugUtilsMessageEXT",
                                objects_info, "XrDebugUtilsMessageSeverityFlagsEXT \"messageSeverity\" flag must be non-zero");
            return XR_ERROR_VALIDATION_FAILURE;
        } else if (VALIDATE_XR_FLAGS_SUCCESS != debug_utils_message_severity_flags_ext_result) {
            // Otherwise, flags must be valid.
            std::ostringstream oss_enum;
            oss_enum << "Invalid XrDebugUtilsMessageSeverityFlagsEXT \"messageSeverity\" flag value ";
            oss_enum << Uint32ToHexString(static_cast<uint32_t>(messageSeverity));
            oss_enum <<" contains illegal bit";
            CoreValidLogMessage(gen_instance_info, "VUID-xrSubmitDebugUtilsMessageEXT-messageSeverity-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSubmitDebugUtilsMessageEXT",
                                objects_info, oss_enum.str());
            return XR_ERROR_VALIDATION_FAILURE;
        }
        ValidateXrFlagsResult debug_utils_message_type_flags_ext_result = ValidateXrDebugUtilsMessageTypeFlagsEXT(messageTypes);
        // Flags must be non-zero in this case.
        if (VALIDATE_XR_FLAGS_ZERO == debug_utils_message_type_flags_ext_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrSubmitDebugUtilsMessageEXT-messageTypes-requiredbitmask",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSubmitDebugUtilsMessageEXT",
                                objects_info, "XrDebugUtilsMessageTypeFlagsEXT \"messageTypes\" flag must be non-zero");
            return XR_ERROR_VALIDATION_FAILURE;
        } else if (VALIDATE_XR_FLAGS_SUCCESS != debug_utils_message_type_flags_ext_result) {
            // Otherwise, flags must be valid.
            std::ostringstream oss_enum;
            oss_enum << "Invalid XrDebugUtilsMessageTypeFlagsEXT \"messageTypes\" flag value ";
            oss_enum << Uint32ToHexString(static_cast<uint32_t>(messageTypes));
            oss_enum <<" contains illegal bit";
            CoreValidLogMessage(gen_instance_info, "VUID-xrSubmitDebugUtilsMessageEXT-messageTypes-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSubmitDebugUtilsMessageEXT",
                                objects_info, oss_enum.str());
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == callbackData) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrSubmitDebugUtilsMessageEXT-callbackData-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSubmitDebugUtilsMessageEXT", objects_info,
                                "Invalid NULL for XrDebugUtilsMessengerCallbackDataEXT \"callbackData\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrDebugUtilsMessengerCallbackDataEXT is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrSubmitDebugUtilsMessageEXT", objects_info,
                                                        true, callbackData);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrSubmitDebugUtilsMessageEXT-callbackData-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSubmitDebugUtilsMessageEXT",
                                objects_info,
                                "Command xrSubmitDebugUtilsMessageEXT param callbackData is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult                                    GenValidUsageNextXrSubmitDebugUtilsMessageEXT(
    XrInstance                                  instance,
    XrDebugUtilsMessageSeverityFlagsEXT         messageSeverity,
    XrDebugUtilsMessageTypeFlagsEXT             messageTypes,
    const XrDebugUtilsMessengerCallbackDataEXT* callbackData) {
    XrResult result = XR_SUCCESS;
    try {
        GenValidUsageXrInstanceInfo *gen_instance_info = g_instance_info.get(instance);
        result = gen_instance_info->dispatch_table->SubmitDebugUtilsMessageEXT(instance, messageSeverity, messageTypes, callbackData);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult                                    GenValidUsageXrSubmitDebugUtilsMessageEXT(
    XrInstance                                  instance,
    XrDebugUtilsMessageSeverityFlagsEXT         messageSeverity,
    XrDebugUtilsMessageTypeFlagsEXT             messageTypes,
    const XrDebugUtilsMessengerCallbackDataEXT* callbackData) {
    XrResult test_result = GenValidUsageInputsXrSubmitDebugUtilsMessageEXT(instance, messageSeverity, messageTypes, callbackData);
    if (XR_SUCCESS != test_result) {
        return test_result;
    }
    return GenValidUsageNextXrSubmitDebugUtilsMessageEXT(instance, messageSeverity, messageTypes, callbackData);
}

XrResult GenValidUsageInputsXrSessionBeginDebugUtilsLabelRegionEXT(
XrSession session,
const XrDebugUtilsLabelEXT* labelInfo) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrSessionBeginDebugUtilsLabelRegionEXT-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSessionBeginDebugUtilsLabelRegionEXT",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == labelInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrSessionBeginDebugUtilsLabelRegionEXT-labelInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSessionBeginDebugUtilsLabelRegionEXT", objects_info,
                                "Invalid NULL for XrDebugUtilsLabelEXT \"labelInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrDebugUtilsLabelEXT is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrSessionBeginDebugUtilsLabelRegionEXT", objects_info,
                                                        true, labelInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrSessionBeginDebugUtilsLabelRegionEXT-labelInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSessionBeginDebugUtilsLabelRegionEXT",
                                objects_info,
                                "Command xrSessionBeginDebugUtilsLabelRegionEXT param labelInfo is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrSessionBeginDebugUtilsLabelRegionEXT(
    XrSession session,
    const XrDebugUtilsLabelEXT* labelInfo) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->SessionBeginDebugUtilsLabelRegionEXT(session, labelInfo);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageInputsXrSessionEndDebugUtilsLabelRegionEXT(
XrSession session) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrSessionEndDebugUtilsLabelRegionEXT-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSessionEndDebugUtilsLabelRegionEXT",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrSessionEndDebugUtilsLabelRegionEXT(
    XrSession session) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->SessionEndDebugUtilsLabelRegionEXT(session);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}

XrResult GenValidUsageInputsXrSessionInsertDebugUtilsLabelEXT(
XrSession session,
const XrDebugUtilsLabelEXT* labelInfo) {
    try {
        XrResult xr_result = XR_SUCCESS;
        std::vector<GenValidUsageXrObjectInfo> objects_info;
        objects_info.emplace_back(session, XR_OBJECT_TYPE_SESSION);

        {
            // writeValidateInlineHandleValidation
            ValidateXrHandleResult handle_result = VerifyXrSessionHandle(&session);
            if (handle_result != VALIDATE_XR_HANDLE_SUCCESS) {
                // Not a valid handle or NULL (which is not valid in this case)
                std::ostringstream oss;
                oss << "Invalid XrSession handle \"session\" ";
                oss << HandleToHexString(session);
                CoreValidLogMessage(nullptr, "VUID-xrSessionInsertDebugUtilsLabelEXT-session-parameter",
                                    VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSessionInsertDebugUtilsLabelEXT",
                                    objects_info, oss.str());
                return XR_ERROR_HANDLE_INVALID;
            }
        }
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        // Non-optional pointer/array variable that needs to not be NULL
        if (nullptr == labelInfo) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrSessionInsertDebugUtilsLabelEXT-labelInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSessionInsertDebugUtilsLabelEXT", objects_info,
                                "Invalid NULL for XrDebugUtilsLabelEXT \"labelInfo\" which is not "
                                "optional and must be non-NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // Validate that the structure XrDebugUtilsLabelEXT is valid
        xr_result = ValidateXrStruct(gen_instance_info, "xrSessionInsertDebugUtilsLabelEXT", objects_info,
                                                        true, labelInfo);
        if (XR_SUCCESS != xr_result) {
            CoreValidLogMessage(gen_instance_info, "VUID-xrSessionInsertDebugUtilsLabelEXT-labelInfo-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrSessionInsertDebugUtilsLabelEXT",
                                objects_info,
                                "Command xrSessionInsertDebugUtilsLabelEXT param labelInfo is invalid");
            return xr_result;
        }
        return XR_SUCCESS;
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

XrResult GenValidUsageNextXrSessionInsertDebugUtilsLabelEXT(
    XrSession session,
    const XrDebugUtilsLabelEXT* labelInfo) {
    XrResult result = XR_SUCCESS;
    try {
        auto info_with_instance = g_session_info.getWithInstanceInfo(session);
        GenValidUsageXrHandleInfo *gen_session_info = info_with_instance.first;
        GenValidUsageXrInstanceInfo *gen_instance_info = info_with_instance.second;
        result = gen_instance_info->dispatch_table->SessionInsertDebugUtilsLabelEXT(session, labelInfo);
    } catch (...) {
        result = XR_ERROR_VALIDATION_FAILURE;
    }
    return result;
}


// API Layer's xrGetInstanceProcAddr
XrResult GenValidUsageXrGetInstanceProcAddr(
    XrInstance          instance,
    const char*         name,
    PFN_xrVoidFunction* function) {
    try {
        std::string func_name = name;
        std::vector<GenValidUsageXrObjectInfo> objects;
        if (g_instance_info.verifyHandle(&instance) == VALIDATE_XR_HANDLE_INVALID) {
            // Make sure the instance is valid if it is not XR_NULL_HANDLE
            std::vector<GenValidUsageXrObjectInfo> objects;
            objects.resize(1);
            objects[0].handle = MakeHandleGeneric(instance);
            objects[0].type = XR_OBJECT_TYPE_INSTANCE;
            CoreValidLogMessage(nullptr, "VUID-xrGetInstanceProcAddr-instance-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetInstanceProcAddr", objects,
                                "Invalid instance handle provided.");
        }
        // NOTE: Can't validate "VUID-xrGetInstanceProcAddr-name-parameter" null-termination
        // If we setup the function, just return
        if (function == nullptr) {
            CoreValidLogMessage(nullptr, "VUID-xrGetInstanceProcAddr-function-parameter",
                                VALID_USAGE_DEBUG_SEVERITY_ERROR, "xrGetInstanceProcAddr", objects,
                                "function is NULL");
            return XR_ERROR_VALIDATION_FAILURE;
        }

        // ---- Core 1.0 commands
        if (func_name == "xrGetInstanceProcAddr") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrGetInstanceProcAddr);
        } else if (func_name == "xrCreateInstance") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(CoreValidationXrCreateInstance);
        } else if (func_name == "xrDestroyInstance") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(CoreValidationXrDestroyInstance);
        } else if (func_name == "xrGetInstanceProperties") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrGetInstanceProperties);
        } else if (func_name == "xrPollEvent") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrPollEvent);
        } else if (func_name == "xrResultToString") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrResultToString);
        } else if (func_name == "xrStructureTypeToString") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrStructureTypeToString);
        } else if (func_name == "xrGetSystem") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrGetSystem);
        } else if (func_name == "xrGetSystemProperties") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrGetSystemProperties);
        } else if (func_name == "xrEnumerateEnvironmentBlendModes") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrEnumerateEnvironmentBlendModes);
        } else if (func_name == "xrCreateSession") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(CoreValidationXrCreateSession);
        } else if (func_name == "xrDestroySession") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrDestroySession);
        } else if (func_name == "xrEnumerateReferenceSpaces") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrEnumerateReferenceSpaces);
        } else if (func_name == "xrCreateReferenceSpace") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrCreateReferenceSpace);
        } else if (func_name == "xrGetReferenceSpaceBoundsRect") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrGetReferenceSpaceBoundsRect);
        } else if (func_name == "xrCreateActionSpace") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrCreateActionSpace);
        } else if (func_name == "xrLocateSpace") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrLocateSpace);
        } else if (func_name == "xrDestroySpace") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrDestroySpace);
        } else if (func_name == "xrEnumerateViewConfigurations") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrEnumerateViewConfigurations);
        } else if (func_name == "xrGetViewConfigurationProperties") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrGetViewConfigurationProperties);
        } else if (func_name == "xrEnumerateViewConfigurationViews") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrEnumerateViewConfigurationViews);
        } else if (func_name == "xrEnumerateSwapchainFormats") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrEnumerateSwapchainFormats);
        } else if (func_name == "xrCreateSwapchain") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrCreateSwapchain);
        } else if (func_name == "xrDestroySwapchain") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrDestroySwapchain);
        } else if (func_name == "xrEnumerateSwapchainImages") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrEnumerateSwapchainImages);
        } else if (func_name == "xrAcquireSwapchainImage") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrAcquireSwapchainImage);
        } else if (func_name == "xrWaitSwapchainImage") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrWaitSwapchainImage);
        } else if (func_name == "xrReleaseSwapchainImage") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrReleaseSwapchainImage);
        } else if (func_name == "xrBeginSession") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrBeginSession);
        } else if (func_name == "xrEndSession") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrEndSession);
        } else if (func_name == "xrRequestExitSession") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrRequestExitSession);
        } else if (func_name == "xrWaitFrame") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrWaitFrame);
        } else if (func_name == "xrBeginFrame") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrBeginFrame);
        } else if (func_name == "xrEndFrame") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrEndFrame);
        } else if (func_name == "xrLocateViews") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrLocateViews);
        } else if (func_name == "xrStringToPath") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrStringToPath);
        } else if (func_name == "xrPathToString") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrPathToString);
        } else if (func_name == "xrCreateActionSet") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrCreateActionSet);
        } else if (func_name == "xrDestroyActionSet") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrDestroyActionSet);
        } else if (func_name == "xrCreateAction") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrCreateAction);
        } else if (func_name == "xrDestroyAction") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrDestroyAction);
        } else if (func_name == "xrSuggestInteractionProfileBindings") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrSuggestInteractionProfileBindings);
        } else if (func_name == "xrAttachSessionActionSets") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrAttachSessionActionSets);
        } else if (func_name == "xrGetCurrentInteractionProfile") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrGetCurrentInteractionProfile);
        } else if (func_name == "xrGetActionStateBoolean") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrGetActionStateBoolean);
        } else if (func_name == "xrGetActionStateFloat") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrGetActionStateFloat);
        } else if (func_name == "xrGetActionStateVector2f") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrGetActionStateVector2f);
        } else if (func_name == "xrGetActionStatePose") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrGetActionStatePose);
        } else if (func_name == "xrSyncActions") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrSyncActions);
        } else if (func_name == "xrEnumerateBoundSourcesForAction") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrEnumerateBoundSourcesForAction);
        } else if (func_name == "xrGetInputSourceLocalizedName") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrGetInputSourceLocalizedName);
        } else if (func_name == "xrApplyHapticFeedback") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrApplyHapticFeedback);
        } else if (func_name == "xrStopHapticFeedback") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrStopHapticFeedback);

        // ---- XR_KHR_android_thread_settings extension commands
#if defined(XR_USE_PLATFORM_ANDROID)
        } else if (func_name == "xrSetAndroidApplicationThreadKHR") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrSetAndroidApplicationThreadKHR);
#endif // defined(XR_USE_PLATFORM_ANDROID)

        // ---- XR_KHR_android_surface_swapchain extension commands
#if defined(XR_USE_PLATFORM_ANDROID)
        } else if (func_name == "xrCreateSwapchainAndroidSurfaceKHR") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrCreateSwapchainAndroidSurfaceKHR);
#endif // defined(XR_USE_PLATFORM_ANDROID)

        // ---- XR_KHR_opengl_enable extension commands
#if defined(XR_USE_GRAPHICS_API_OPENGL)
        } else if (func_name == "xrGetOpenGLGraphicsRequirementsKHR") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrGetOpenGLGraphicsRequirementsKHR);
#endif // defined(XR_USE_GRAPHICS_API_OPENGL)

        // ---- XR_KHR_opengl_es_enable extension commands
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
        } else if (func_name == "xrGetOpenGLESGraphicsRequirementsKHR") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrGetOpenGLESGraphicsRequirementsKHR);
#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES)

        // ---- XR_KHR_vulkan_enable extension commands
#if defined(XR_USE_GRAPHICS_API_VULKAN)
        } else if (func_name == "xrGetVulkanInstanceExtensionsKHR") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrGetVulkanInstanceExtensionsKHR);
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
        } else if (func_name == "xrGetVulkanDeviceExtensionsKHR") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrGetVulkanDeviceExtensionsKHR);
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
        } else if (func_name == "xrGetVulkanGraphicsDeviceKHR") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrGetVulkanGraphicsDeviceKHR);
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
        } else if (func_name == "xrGetVulkanGraphicsRequirementsKHR") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrGetVulkanGraphicsRequirementsKHR);
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)

        // ---- XR_KHR_D3D11_enable extension commands
#if defined(XR_USE_GRAPHICS_API_D3D11)
        } else if (func_name == "xrGetD3D11GraphicsRequirementsKHR") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrGetD3D11GraphicsRequirementsKHR);
#endif // defined(XR_USE_GRAPHICS_API_D3D11)

        // ---- XR_KHR_D3D12_enable extension commands
#if defined(XR_USE_GRAPHICS_API_D3D12)
        } else if (func_name == "xrGetD3D12GraphicsRequirementsKHR") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrGetD3D12GraphicsRequirementsKHR);
#endif // defined(XR_USE_GRAPHICS_API_D3D12)

        // ---- XR_KHR_visibility_mask extension commands
        } else if (func_name == "xrGetVisibilityMaskKHR") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrGetVisibilityMaskKHR);

        // ---- XR_KHR_win32_convert_performance_counter_time extension commands
#if defined(XR_USE_PLATFORM_WIN32)
        } else if (func_name == "xrConvertWin32PerformanceCounterToTimeKHR") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrConvertWin32PerformanceCounterToTimeKHR);
#endif // defined(XR_USE_PLATFORM_WIN32)
#if defined(XR_USE_PLATFORM_WIN32)
        } else if (func_name == "xrConvertTimeToWin32PerformanceCounterKHR") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrConvertTimeToWin32PerformanceCounterKHR);
#endif // defined(XR_USE_PLATFORM_WIN32)

        // ---- XR_KHR_convert_timespec_time extension commands
#if defined(XR_USE_TIMESPEC)
        } else if (func_name == "xrConvertTimespecTimeToTimeKHR") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrConvertTimespecTimeToTimeKHR);
#endif // defined(XR_USE_TIMESPEC)
#if defined(XR_USE_TIMESPEC)
        } else if (func_name == "xrConvertTimeToTimespecTimeKHR") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrConvertTimeToTimespecTimeKHR);
#endif // defined(XR_USE_TIMESPEC)

        // ---- XR_EXT_performance_settings extension commands
        } else if (func_name == "xrPerfSettingsSetPerformanceLevelEXT") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrPerfSettingsSetPerformanceLevelEXT);

        // ---- XR_EXT_thermal_query extension commands
        } else if (func_name == "xrThermalGetTemperatureTrendEXT") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrThermalGetTemperatureTrendEXT);

        // ---- XR_EXT_debug_utils extension commands
        } else if (func_name == "xrSetDebugUtilsObjectNameEXT") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(CoreValidationXrSetDebugUtilsObjectNameEXT);
        } else if (func_name == "xrCreateDebugUtilsMessengerEXT") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(CoreValidationXrCreateDebugUtilsMessengerEXT);
        } else if (func_name == "xrDestroyDebugUtilsMessengerEXT") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(CoreValidationXrDestroyDebugUtilsMessengerEXT);
        } else if (func_name == "xrSubmitDebugUtilsMessageEXT") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(GenValidUsageXrSubmitDebugUtilsMessageEXT);
        } else if (func_name == "xrSessionBeginDebugUtilsLabelRegionEXT") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(CoreValidationXrSessionBeginDebugUtilsLabelRegionEXT);
        } else if (func_name == "xrSessionEndDebugUtilsLabelRegionEXT") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(CoreValidationXrSessionEndDebugUtilsLabelRegionEXT);
        } else if (func_name == "xrSessionInsertDebugUtilsLabelEXT") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(CoreValidationXrSessionInsertDebugUtilsLabelEXT);
        }
        // If we setup the function, just return
        if (*function != nullptr) {
            return XR_SUCCESS;
        }
        // We have not found it, so pass it down to the next layer/runtime
        GenValidUsageXrInstanceInfo* instance_valid_usage_info = g_instance_info.get(instance);
        if (nullptr == instance_valid_usage_info) {
            return XR_ERROR_HANDLE_INVALID;
        }
        return instance_valid_usage_info->dispatch_table->GetInstanceProcAddr(instance, name, function);
    } catch (...) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
}

