/********************************************************************************
*
* WARNING This file is subject to the termsand conditions defined in the file
* 'LICENSE.txt', which is part of this source code package.Please ensure you
* have read the 'LICENSE.txt' file before using this software.
*
*********************************************************************************/

#include "Logger.h"

using namespace std;
using namespace Emperor;

//--------------------------------------------------------------------------------
// LogCallback
//--------------------------------------------------------------------------------

LogCallback::LogCallback(
	function<void(const string&)> logDebug,
	function<void(const string&)> logMessage,
	function<void(const string&)> logWarning,
	function<void(const string&)> logError)
	: m_LogDebug(logDebug)
	, m_LogMessage(logMessage)
	, m_LogWarning(logWarning)
	, m_LogError(logError)
{

}

void LogCallback::LogDebug(const string& msg) const
{
	m_LogDebug(msg);
}

void LogCallback::LogMessage(const string& msg) const
{
	m_LogMessage(msg);
}

void LogCallback::LogWarning(const string& msg) const
{
	m_LogWarning(msg);
}

void LogCallback::LogError(const string& msg) const
{
	m_LogError(msg);
}

//--------------------------------------------------------------------------------
// Logger
//--------------------------------------------------------------------------------

Logger::Logger(LogCallback& logger)
	: m_LogCallback(logger)
{

}

LogCallback& Logger::GetLogger() const
{
	return const_cast<LogCallback&>(m_LogCallback);
}

void Logger::SetLoggingName(const string& name)
{
	m_LoggingName = name;
}

void Logger::LogDebug(const string& msg) const
{
	m_LogCallback.LogDebug("[" + m_LoggingName + "] " + msg);
}

void Logger::LogMessage(const string& msg) const
{
	m_LogCallback.LogMessage("[" + m_LoggingName + "] " + msg);
}

void Logger::LogWarning(const string& msg) const
{
	m_LogCallback.LogWarning("[" + m_LoggingName + "] " + msg);
}

void Logger::LogError(const string& msg) const
{
	m_LogCallback.LogError("[" + m_LoggingName + "] " + msg);
}
