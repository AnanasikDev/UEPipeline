#include "NamingValidator.h"
#include "NamingConventionSettings.h"

UNamingValidator::UNamingValidator()
{
    bIsEnabled = true;
}

bool UNamingValidator::CanValidateAsset_Implementation(
    const FAssetData& InAssetData,
    UObject* InObject,
    FDataValidationContext& InContext) const
{
    if (!InObject)
    {
        return false;
    }

    if (InObject->IsA(UObjectRedirector::StaticClass()))
    {
        // object cannot be created of a static class
        return false;
    }

    const FString PackagePath = InAssetData.PackagePath.ToString();
    const UNamingConventionSettings* Settings = UNamingConventionSettings::Get();
    for (const FDirectoryPath& Dir : Settings->ExcludedPaths)
    {
        if (!Dir.Path.IsEmpty() && PackagePath.StartsWith(Dir.Path))
        {
            // ignore asset if it is inside an excluded path
            return false;
        }
    }

    return true;
}

EDataValidationResult UNamingValidator::ValidateLoadedAsset_Implementation(
    const FAssetData& InAssetData,
    UObject* InAsset,
    FDataValidationContext& Context)
{
    if (!InAsset)
    {
        AssetPasses(InAsset);
        return EDataValidationResult::Valid;
    }

    const UNamingConventionSettings* const Settings = UNamingConventionSettings::Get();
    const FString AssetName = InAsset->GetName();

    const UClass* BestMatch = nullptr;
    const FNamingRuleEntry* MatchedEntry = nullptr;

    for (const FNamingRuleEntry& Entry : Settings->Rules)
    {
        const UClass* const RuleClass = Entry.AssetClass.LoadSynchronous();

        if (!RuleClass)
        {
            continue;
        }

        if (InAsset->GetClass()->IsChildOf(RuleClass))
        {
            if (!BestMatch || RuleClass->IsChildOf(BestMatch))
            {
                BestMatch = RuleClass;
                MatchedEntry = &Entry;
            }
        }
    }

    // skip unrecognized type
    if (!BestMatch || !MatchedEntry)
    {
        AssetPasses(InAsset);
        return EDataValidationResult::Valid;
    }

    // pass if ANY rule for this type is satisfied
    bool bAnyPassed = false;
    for (const FNamingRule& Rule : MatchedEntry->Rules)
    {
        const bool bPrefixOk = Rule.Prefix.IsEmpty() || AssetName.StartsWith(Rule.Prefix);

        const bool bSuffixOk = Rule.Suffix.IsEmpty() || AssetName.EndsWith(Rule.Suffix);

        if (bPrefixOk && bSuffixOk)
        {
            bAnyPassed = true;
            break;
        }
    }

    if (!bAnyPassed)
    {
        // validation for this asset failed, print to console

        // build a list of accepted prefixes/suffixes for the error message
        TArray<FString> Expected;
        for (const FNamingRule& Rule : MatchedEntry->Rules)
        {
            FString Desc;
            if (!Rule.Prefix.IsEmpty())
            {
                Desc += FString::Printf(TEXT("prefix \"%s\""), *Rule.Prefix);
            }
            if (!Rule.Suffix.IsEmpty())
            {
                if (!Desc.IsEmpty())
                {
                    Desc += TEXT(" + ");
                }
                Desc += FString::Printf(TEXT("suffix \"%s\""), *Rule.Suffix);
            }
            Expected.Add(Desc);
        }

        AssetFails(InAsset, FText::Format(
            INVTEXT("{0} (type: {1}) does not match any naming rule: {2}"),
            FText::FromString(AssetName),
            FText::FromString(BestMatch->GetName()),
            FText::FromString(FString::Join(Expected, TEXT(" | ")))
        ));
        return EDataValidationResult::Invalid;
    }

    AssetPasses(InAsset);
    return EDataValidationResult::Valid;
}