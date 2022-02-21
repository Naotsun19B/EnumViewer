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
		virtual bool IsEnumAllowed(const FEnumViewerInitializationOptions& InInitOptions, const UEnum* InEnum, TSharedRef<class FEnumViewerFilterFunctions> InFilterFuncs) = 0;

		// Checks if a unloaded enum is allowed by this filter.
		virtual bool IsUnloadedEnumAllowed(const FEnumViewerInitializationOptions& InInitOptions, const FName InEnumPath, TSharedRef<class FEnumViewerFilterFunctions> InFilterFuncs) = 0;
	};

	/**
	 * Execution result of the function that performs the processing related to enum.
	 */
	enum class EEnumFilterReturn : uint8
	{
		Failed = 0, 
		Passed, 
		NoItems,
	};

	/**
	 * A class that defines a function that performs processing related to enum.
	 */
	class ENUMVIEWER_API FEnumViewerFilterFunctions
	{
	public:
		virtual ~FEnumViewerFilterFunctions() = default;

		// Checks if the given Enum is a child-of any of the enums in a set.
		virtual EEnumFilterReturn IfInChildOfEnumsSet(TSet<const UEnum*>& InSet, const UEnum* InEnum);

		// Checks if the given enum is a child-of ALL of the classes in a set.
		virtual EEnumFilterReturn IfMatchesAllInChildOfEnumsSet(TSet<const UEnum*>& InSet, const UEnum* InEnum);

		// Checks if the enum is in the enums set.
		virtual EEnumFilterReturn IfInEnumsSet(TSet<const UEnum*>& InSet, const UEnum* InEnum);
	};
}
