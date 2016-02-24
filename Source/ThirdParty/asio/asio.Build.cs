using System.IO;
using UnrealBuildTool;

public class asio : ModuleRules
{
	public asio(TargetInfo Target)
	{
		Type = ModuleType.External;

		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "asio", "include"));
	}
}
