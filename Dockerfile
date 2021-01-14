# escape=`

# Use the latest Windows Server Core image with .NET Framework 4.8.
FROM mcr.microsoft.com/dotnet/framework/sdk:4.8-windowsservercore-ltsc2019

# Restore the default Windows shell for correct batch processing.
SHELL ["cmd", "/S", "/C"]

# Download the Build Tools bootstrapper.
ADD https://aka.ms/vs/16/release/vs_buildtools.exe C:\TEMP\vs_buildtools.exe

# Install Build Tools with the Microsoft.VisualStudio.Workload.AzureBuildTools workload, excluding workloads and components with known issues.
# RUN C:\TEMP\vs_buildtools.exe --quiet --wait --norestart --nocache `
RUN C:\TEMP\vs_buildtools.exe --wait --norestart --nocache `
  --installPath C:\BuildTools `
  --all `
  --remove Microsoft.VisualStudio.Component.Windows10SDK.10240 `
  --remove Microsoft.VisualStudio.Component.Windows10SDK.10586 `
  --remove Microsoft.VisualStudio.Component.Windows10SDK.14393 `
  --remove Microsoft.VisualStudio.Component.Windows81SDK
# --add Microsoft.VisualStudio.Workload.AzureBuildTools `
# --remove Microsoft.VisualStudio.Component.Windows10SDK.10240 `
# --remove Microsoft.VisualStudio.Component.Windows10SDK.10586 `
# --remove Microsoft.VisualStudio.Component.Windows10SDK.14393 `
# --remove Microsoft.VisualStudio.Component.Windows81SDK `
# || IF "%ERRORLEVEL%"=="3010" EXIT 0

# FROM mcr.microsoft.com/windows/servercore:ltsc2019
#FROM lukaslansky/visualstudio-netwebworkload:v16.3.2
SHELL ["powershell", "-Command", "$ErrorActionPreference = 'Stop'; $ProgressPreference = 'SilentlyContinue';"]

WORKDIR /work

# Install Chocolatey
# RUN Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))

# RUN choco install --yes 7zip cmake unxutils
# RUN choco install --yes dotnetfx; echo skip
# RUN choco install --yes visualstudio2019community
# RUN choco install --yes visualstudio2019-workload-nativedesktop

RUN Invoke-Expression (New-Object System.Net.WebClient).DownloadString('https://get.scoop.sh')
# RUN scoop install 7zip cmake unxutils
RUN scoop install 7zip unxutils
# RUN scoop install cmake unxutils
# RUN scoop install cmake

COPY ci /work/ci

RUN ci/install.ps1 -OBS_VER 25.0.8

CMD ./build.ps1 -OBS_BIN_DIR32 ../OBS-Studio-25.0.8-Full-x86 -OBS_BIN_DIR64 ../OBS-Studio-25.0.8-Full-x64 -OBS_SRC_DIR ../obs-studio-25.0.8

