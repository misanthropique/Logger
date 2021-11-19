/**
 * Copyright ©2021. Brent Weichel. All Rights Reserved.
 * Permission to use, copy, modify, and/or distribute this software, in whole
 * or part by any means, without express prior written agreement is prohibited.
 */
#pragma once

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iterator>
#include <map>
#include <string>
#include <vector>

/**
 * A class for logging messages either out to stderr, or
 * out to a user defined log file.
 *
 * TODO
 * [ ] Add time since process start
 * [ ] Add Windows support
 */
class Logger
{
	std::string mLoggerName;
	Logger::Level mLoggingLevel;
	FILE* mLoggingFile;
	Logger::TimePrefix mTimePrefix;
	std::string mUserDefinedTimeFormatting;

	// Check if the requested log message
	// is greater than or equal to the set level.
	bool _canWriteLogMessage(
		Logger::Level logMessageLevel )
	{
		static const Logger::Level LOGGER_LEVEL_ORDER[] =
		{
			Logger::Level::ALL,
			Logger::Level::DEBUG,
			Logger::Level::INFO,
			Logger::Level::WARNING,
			Logger::Level::ERROR,
			Logger::Level::CRITICAL
		};
		static const size_t LOGGER_LEVEL_ORDER_ARRAY_SIZE =
			sizeof( LOGGER_LEVEL_ORDER ) / sizeof( LOGGER_LEVEL_ORDER[ 0 ] );

		if ( Logger::Level::None == mLoggingLevel )
		{
			return false;
		}

		auto messageLevel = std::find( LOGGER_LEVEL_ORDER, LOGGER_LEVEL_ORDER + LOGGER_LEVEL_ORDER_ARRAY_SIZE, logMessageLevel );
		auto loggerLevel = std::find( LOGGER_LEVEL_ORDER, LOGGER_LEVEL_ORDER + LOGGER_LEVEL_ORDER_ARRAY_SIZE, mLoggingLevel );

		return ( std::distance( LOGGER_LEVEL_ORDER, loggerLevel ) <= std::distance( LOGGER_LEVEL_ORDER, messageLevel ) );
	}

	// Get the timestamp for the next message.
	// The timestamp will be left empty if we fail.
	void _getTimestamp(
		std::string& timestamp )
	{
		std::time_t currentTime = std::time( nullptr );
		std::tm localTime = *std::localtime( &currentTime );
		std::stringstream timeBuffer;

		switch ( mTimePrefix )
		{
		case Logger::TimePrefix::NONE:
			// Do nothing
			break;

		case Logger::TimePrefix::ISO_8601:
			std::tm utcTime = *std::gmtime( &currentTime );
			timeBuffer << std::put_time( utcTime, "%Y-%m-%dT%H:%M:%SZ" );
			break;

		case Logger::TimePrefix::LOCAL_DEFAULT:
			timeBuffer << std::put_time( localTime, "%Y-%m-%d %H:%M:%S" );
			break;

		case Logger::TimePrefix::SECONDS_SINCE_PROCESS_START:
			// This is going to be a bit involved.
			// I'll come back to this later.
			break;

		case Logger::TimePrefix::USER_DEFINED:
			if ( not mUserDefinedTimeFormatting.empty() )
			{
				timeBuffer << std::put_time( localTime, mUserDefinedTimeFormatting );
			}
			break;
		}

		timestamp = timeBuffer.str();
	}

	// Write out the log message per the configured
	// requirements. If the message level is less than the
	// configured, then the message will be ignored.
	void _writeMessage(
		Logger::Level logMessageLevel,
		const char* format,
		va_list arguments )
	{
		static const std::map< Logger::Level, std::string > MESSAGE_LEVEL_TO_STRING
		{
			{ Logger::Level::DEBUG, "DEBUG" },
			{ Logger::Level::INFO, "INFO" },
			{ Logger::Level::WARNING, "WARNING" },
			{ Logger::Level::ERROR, "ERROR" },
			{ Logger::Level::CRITICAL, "CRITICAL" }
		};

		if ( _canWriteLogMessage( logMessageLevel ) )
		{
			std::string timestamp;
			_getTimestamp( timestamp );

			const auto& levelString = MESSAGE_LEVEL_TO_STRING[ logMessageLevel ];

			va_list argumentsCopy;
			va_copy( argumentsCopy, arguments );
			std::vector< char > messageBuffer( 1 + vsnprintf( nullptr, 0, format, argumentsCopy ) );
			va_end( argumentsCopy );

			vsnprintf( messageBuffer.data(), messageBuffer.size(), format, arguments );

			// timestamp logger_name level message
			fprintf( mLoggingFile, "%s%s%s%s%s%s\n",
				timestamp.c_str(), ( 0 == timestamp.length() ) ? "" : " ",
				mLoggerName.c_str(), ( 0 == mLoggerName.length() ) ? "" : " ",
				levelString.c_str(), messageBuffer );
			fflush( mLoggingFile );
		}
	}

public:
	enum class Level
	{
		ALL,      ///< Everything shall be logged regardless of level.
		DEBUG,    ///< Everything from debug and above shall be logged.
		INFO,     ///< Everything from info and above shall be logged.
		WARNING,  ///< Everything from warning and above shall be logged.
		ERROR,    ///< Everything from error and above shall be logged.
		CRITICAL, ///< Critical is the highest level of logging.
		NONE      ///< Nothing is to be logged.
	};

	enum class TimePrefix
	{
		NONE,                        ///< Don't prefix the messages with any time.
		ISO_8601,                    ///< Output the time in ISO-8601 UTC time, e.g. 2021-11-18T06:18:23Z.
		LOCAL_DEFAULT,               ///< Using local time, formatted to YYYY-MM-DD HH:MM::SS.
		SECONDS_SINCE_PROCESS_START, ///< Seconds since the process started.
		USER_DEFINED                 ///< Prefix messages with a user defined time stamp.
	};

	/**
	 * Default constructor for the Logger class.
	 * @param name Name of this Logger instance. [default: ""]
	 * @param filename Name of the log file to write to. [default: stderr]
	 * @param loggingLevel Lowest level to log to file. [default: Level::WARNING]
	 * @param timePrefix Time prefix to use for messages. [default: TimePrefix::LOCAL_DEFAULT]
	 * @param userDefinedTimeFormatting A user defined time formatting string. [default: ""]
	 */
	Logger(
		const std::string& name = std::string(),
		const std::string& filename = std::string(),
		Logger::Level loggingLevel = Logger::Level::WARNING,
		Logger::TimePrefix timePrefix = Logger::TimePrefix::LOCAL_DEFAULT,
		const std::string& userDefinedTimeFormatting = std::string() )
	{
		mLoggerName = name;
		mLoggingFile = stderr;
		mLoggingLevel = loggingLevel;
		mTimePrefix = timePrefix;
		mUserDefinedTimeFormatting = userDefinedTimeFormatting;

		if ( not mLoggerName.empty() )
		{
			FILE* filePointer;
			// If we fail to open the file, then
			// leave the logging file as stderr.
			if ( nullptr != ( filePointer = fopen( mLoggerName.c_str(), "w" ) ) )
			{
				mLoggingFile = filePointer;
			}
		}
	}

	/**
	 * Close any open file handles and release any held resources.
	 */
	~Logger()
	{
		if ( ( stderr != mLoggingFile )
			&& ( nullptr != mLoggingFile ) )
		{
			fclose( mLoggingFile );
			mLoggingFile = nullptr;
		}

		mLoggingLevel = Logger::Level::NONE;
		mTimePrefix = Logger::Level::NONE;
		mUserDefinedTimeFormatting.clear();
		mLoggerName.clear();
	}

	/**
	 * Log a debug message.
	 * @param format Format string for the debug message.
	 */
	void debug(
		const char* format, ... )
	{
		va_list arguments;
		va_start( arguments, format );
		_writeMessage( Logger::Level::DEBUG, format, arguments );
		va_end( arguments );
	}

	/**
	 * Log a info message.
	 * @param format Format string for the info message.
	 */
	void info(
		const char* format, ... )
	{
		va_list arguments;
		va_start( arguments, format );
		_writeMessage( Logger::Level::INFO, format, arguments );
		va_end( arguments );
	}

	/**
	 * Log a warning message.
	 * @param format Format string for the warning message.
	 */
	void warning(
		const char* format, ... )
	{
		va_list arguments;
		va_start( arguments, format );
		_writeMessage( Logger::Level::WARNING, format, arguments );
		va_end( arguments );
	}

	/**
	 * Log a error message.
	 * @param format Format string for the error message.
	 */
	void error(
		const char* format, ... )
	{
		va_list arguments;
		va_start( arguments, format );
		_writeMessage( Logger::Level::ERROR, format, arguments );
		va_end( arguments );
	}

	/**
	 * Log a critical message.
	 * @param format Format string for the critical message.
	 */
	void critical(
		const char* format, ... )
	{
		va_list arguments;
		va_start( arguments, format );
		_writeMessage( Logger::Level::CRITICAL, format, arguments );
		va_end( arguments );
	}
};
