#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorBase.h"
#include "NamingValidator.generated.h"

UCLASS()
class UNamingValidator : public UEditorValidatorBase
{
    GENERATED_BODY()

public:
    UNamingValidator();

protected:
    /// <summary>
    /// Determines if the validation process should be applied to a specific asset. This allows to exclude directories, file types, metadata patterns, from being analyzed.
    /// </summary>
    virtual bool CanValidateAsset_Implementation(
        const FAssetData& InAssetData,
        UObject* InObject,
        FDataValidationContext& InContext) const override;

    /// <summary>
    /// The main validation process. For the given asset in the given context, returns result of the validation.
    /// </summary>
    virtual EDataValidationResult ValidateLoadedAsset_Implementation(
        const FAssetData& InAssetData,
        UObject* InAsset,
        FDataValidationContext& Context) override;
};