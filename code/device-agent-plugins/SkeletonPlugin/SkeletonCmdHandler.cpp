// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "stdafx.h"
#include "PluginJsonConstants.h"
#include "..\..\AzureDeviceManagementCommon\DMConstants.h"
#include "..\..\AzureDeviceManagementCommon\Plugins\PluginConstants.h"
#include "SkeletonCmdHandler.h"

using namespace DMCommon;
using namespace DMUtils;
using namespace std;

constexpr char InterfaceVersion[] = "1.0.0";

namespace Microsoft { namespace Azure { namespace DeviceManagement { namespace SkeletonPlugin {

    SkeletonCmdHandler::SkeletonCmdHandler() :
        BaseHandler(SkeletonCmdHandlerId, ReportedSchema(JsonDeviceSchemasTypeRaw, JsonDeviceSchemasTagDM, InterfaceVersion))
    {
    }

    void SkeletonCmdHandler::Start(
        const Json::Value& handlerConfig,
        bool& active)
    {
        TRACELINE(LoggingLevel::Verbose, __FUNCTION__);

        SetConfig(handlerConfig);

        Json::Value logFilesPath = handlerConfig[JsonTextLogFilesPath];
        if (!logFilesPath.isNull() && logFilesPath.isString())
        {
            wstring wideLogFileName = MultibyteToWide(logFilesPath.asString().c_str());
            wstring wideLogFileNamePrefix = MultibyteToWide(GetId().c_str());
            gLogger.SetLogFilePath(wideLogFileName.c_str(), wideLogFileNamePrefix.c_str());
            gLogger.EnableConsole(true);

            TRACELINE(LoggingLevel::Verbose, "Logging configured.");
        }

        active = true;
    }

    void SkeletonCmdHandler::OnConnectionStatusChanged(
        DMCommon::ConnectionStatus status)
    {
        TRACELINE(LoggingLevel::Verbose, __FUNCTION__);
        if (status == ConnectionStatus::eOffline)
        {
            TRACELINE(LoggingLevel::Verbose, "Connection Status: Offline.");
        }
        else
        {
            TRACELINE(LoggingLevel::Verbose, "Connection Status: Online.");

            Json::Value handlerConfig = GetConfig();

            // Report when connected to cloud and Json handler is set "reportOnConnect": true....
            Json::Value reportOnConnect = handlerConfig[JsonHandlerConfigReportOnConnect];
            if (!reportOnConnect.isNull() && reportOnConnect.isBool())
            {
                if (reportOnConnect.asBool())
                {
                    TRACELINE(LoggingLevel::Verbose, "Report on connect is on...");

                    Json::Value reportedObject(Json::objectValue);
                    std::shared_ptr<ReportedErrorList> errorList = make_shared<ReportedErrorList>();
                    FinalizeAndReport(reportedObject, errorList);
                }
                else
                {
                    TRACELINE(LoggingLevel::Verbose, "Report on connect is off...");
                }
            }
        }
    }

    InvokeResult SkeletonCmdHandler::Invoke(
        const Json::Value& desiredConfig) noexcept
    {
        TRACELINE(LoggingLevel::Verbose, __FUNCTION__);

        (void)desiredConfig;

        // Returned objects (if InvokeContext::eDirectMethod, it is returned to the cloud direct method caller).
        InvokeResult invokeResult(InvokeContext::eDirectMethod, JsonDirectMethodSuccessCode, JsonDirectMethodEmptyPayload);

        // Twin reported objects
        Json::Value reportedObject(Json::objectValue);
        std::shared_ptr<ReportedErrorList> errorList = make_shared<ReportedErrorList>();

        RunOperation(GetId(), errorList,
            [&]()
        {
            // Processing Meta Data
            _metaData->FromJsonParentObject(desiredConfig);

            string serviceInterfaceVersion = _metaData->GetServiceInterfaceVersion();

            //Compare interface version with the interface version sent by service
            if (MajorVersionCompare(InterfaceVersion, serviceInterfaceVersion) == 0)
            {
                // Get required parameters first...
                // string param0 = GetSinglePropertyOpStringParameter(desiredConfig, "param0");

                // Get optional parameters...
                // OperationModelT<bool> param1 = TryGetOptionalSinglePropertyOpBoolParameter(desiredConfig, "param1");

                // Start processing...
                // Do the actual work here.

                // if (returnCode != 0)
                // {
                //    throw DMException(returnCode, "Error: ApplyUpdate.exe returned an error code.");
                // }
                _metaData->SetOutputInterfaceVersion(InterfaceVersion);
            }
            else
            {
                throw DMException(DMSubsystem::DeviceAgentPlugin, DM_PLUGIN_ERROR_INVALID_INTERFACE_VERSION, "Service solution is trying to talk with Interface Version that is not supported.");
            }
        });

        // Update device twin
        FinalizeAndReport(reportedObject, errorList);

        // Pack return payload
        if (errorList->Count() != 0)
        {
            invokeResult.code = JsonDirectMethodFailureCode;
            invokeResult.payload = errorList->ToJsonObject()[GetId()].toStyledString();
        }
        return invokeResult;
    }

}}}}