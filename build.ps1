param([string]$Aqua='aqua',[string[]]$Targets=@())
$ErrorActionPreference='Stop'
$arguments=@('library','build',$PSScriptRoot)
if($Targets.Count){$arguments+=@('--targets',($Targets -join ','))}
& $Aqua @arguments
if($LASTEXITCODE -ne 0){throw 'Jolt SDK build failed'}
