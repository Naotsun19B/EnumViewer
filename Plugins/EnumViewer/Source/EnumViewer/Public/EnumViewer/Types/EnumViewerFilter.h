// Copyright 2022 Naotsun. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace EnumViewer
{
	class FEnumViewerInitializationOptions;
	
	/**
	 * Interface class for creating filters for the Enum Viewer.
	 */
	class IEnumViewerFilter
	{
	public:
		virtual ~IEnumViewerFilter() = default;

		// Checks if a enum is allowed by this filter.
		virtual bool IsEnumAllowed(const FEnumViewerInitializationOptions& InInitOptions, const UEnum* InEnum) = 0;

		// Checks if a unloaded enum is allowed by this filter.
		virtual bool IsUnloadedEnumAllowed(const FEnumViewerInitializationOptions& InInitOptions, const FName InEnumPath) = 0;
	};
}
