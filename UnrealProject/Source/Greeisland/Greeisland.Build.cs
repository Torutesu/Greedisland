using UnrealBuildTool;

public class Greeisland : ModuleRules
{
    public Greeisland(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "DeveloperSettings",
            "Engine",
            "InputCore",
            "Json",
            "JsonUtilities",
            "UMG"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Slate",
            "SlateCore"
        });
    }
}
