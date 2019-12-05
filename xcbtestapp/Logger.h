/********************************************************************************
*
* WARNING This file is subject to the terms and conditions defined in the file
* 'LICENSE.txt', which is part of this source code package. Please ensure you
* have read the 'LICENSE.txt' file before using this software.
*
*********************************************************************************/

#pragma once

#include <string>
#include <functional>

namespace Emperor
{
	class LogCallback
	{
	public:
		LogCallback(
			std::function<void(const std::string&)> logDebug,
			std::function<void(const std::string&)> logMessage,
			std::function<void(const std::string&)> logWarning,
			std::function<void(const std::string&)> logError);

		void LogDebug(const std::string& msg) const;
		void LogMessage(const std::string& msg) const;
		void LogWarning(const std::string& msg) const;
		void LogError(const std::string& msg) const;

	private:
		std::function<void(const std::string&)> m_LogDebug;
		std::function<void(const std::string&)> m_LogMessage;
		std::function<void(const std::string&)> m_LogWarning;
		std::function<void(const std::string&)> m_LogError;
	};

	class Logger
	{
	public:
		Logger(LogCallback& logger);

		void LogDebug(const std::string& msg) const;
		void LogMessage(const std::string& msg) const;
		void LogWarning(const std::string& msg) const;
		void LogError(const std::string& msg) const;

	protected:
		void SetLoggingName(const std::string& name);

		LogCallback& GetLogger() const;

	private:
		std::string m_LoggingName;
		const LogCallback& m_LogCallback;
	};
}
