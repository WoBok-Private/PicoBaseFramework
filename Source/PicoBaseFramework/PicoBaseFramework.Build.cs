using UnrealBuildTool;

public class PicoBaseFramework : ModuleRules
{
    public PicoBaseFramework(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "XRBase", "HeadMountedDisplay" });
    }
}
