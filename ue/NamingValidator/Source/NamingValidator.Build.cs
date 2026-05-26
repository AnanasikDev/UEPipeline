using UnrealBuildTool;

public class NamingValidator : ModuleRules
{
	public NamingValidator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UMG",
            "DeveloperSettings"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "DataValidation" });
	}
}
