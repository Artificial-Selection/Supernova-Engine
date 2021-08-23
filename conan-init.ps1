$env:CONAN_USER_HOME=$(Get-Location)

conan remote add -f gitlab "https://gitlab.com/api/v4/projects/28364651/packages/conan"
conan config set general.revisions_enabled=1
PAUSE