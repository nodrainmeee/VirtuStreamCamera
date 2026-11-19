$ErrorActionPreference = 'SilentlyContinue'
$Report = Join-Path $PSScriptRoot 'virtustream-pc-diagnostics.txt'
$os = Get-CimInstance Win32_OperatingSystem
$cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
$gpu = Get-CimInstance Win32_VideoController | Select-Object Name, DriverVersion, DriverDate, VideoModeDescription, AdapterRAM
$disk = Get-CimInstance Win32_DiskDrive | Select-Object Model, MediaType, Size
$ram = Get-CimInstance Win32_ComputerSystem
@(
  'VirtuStream Camera - PC diagnostics',
  "Date: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')",
  "OS: $($os.Caption) | Version: $($os.Version) | Build: $($os.BuildNumber) | Architecture: $($os.OSArchitecture)",
  "CPU: $($cpu.Name) | Cores: $($cpu.NumberOfCores) | Logical: $($cpu.NumberOfLogicalProcessors)",
  "RAM total bytes: $($ram.TotalPhysicalMemory)",
  'GPU:'
) | Out-File -Encoding utf8 $Report
$gpu | Format-List | Out-File -Append -Encoding utf8 $Report
'Disks:' | Out-File -Append -Encoding utf8 $Report
$disk | Format-Table -AutoSize | Out-File -Append -Encoding utf8 $Report
Write-Host "Diagnostic saved to: $Report"
