#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "NamingConventionSettings.generated.h"

USTRUCT(BlueprintType)
struct FNamingRule
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Naming")
    FString Prefix;

    UPROPERTY(EditAnywhere, Category = "Naming")
    FString Suffix;
};

USTRUCT(BlueprintType)
struct FNamingRuleEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Naming")
    TSoftClassPtr<UObject> AssetClass;

    UPROPERTY(EditAnywhere, Category = "Naming")
    TArray<FNamingRule> Rules;
};

UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "Naming Conventions"))
class UNamingConventionSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UNamingConventionSettings();

    UPROPERTY(config, EditAnywhere, Category = "Naming", meta = (TitleProperty = "AssetClass"))
    TArray<FNamingRuleEntry> Rules;

    UPROPERTY(config, EditAnywhere, Category = "Naming", meta = (ContentDir, LongPackageName))
    TArray<FDirectoryPath> ExcludedPaths;

    static const UNamingConventionSettings* Get()
    {
        return GetDefault<UNamingConventionSettings>();
    }

    // UDeveloperSettings overrides
    virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
    virtual FName GetSectionName() const override { return TEXT("NamingConventions"); }
};