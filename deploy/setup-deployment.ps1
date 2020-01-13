param (
    [string]$TargetPath = ".\build",
    [string]$BuildPath = "..\src\build-ui",
    [string]$RootPath = "..",
    [string]$QtPath = ".\build-qt\qtbase"
)

echo "Setting up deployment files."

$json = Get-Content ".\deployment.json" -Raw | ConvertFrom-Json

$targetDirs = @($json.files.psobject.Properties.name)

for ($i = 0; $i -lt $targetDirs.length; $i++) {

    $targetDir = $targetDirs[$i]
    $jsonTargetDir = $json.files."$targetDir"

    $sections = @($jsonTargetDir.psobject.Properties.name)

    $targetDir = $targetDir -replace "/",  "\"
    $targetDir = $targetDir -replace '\${TARGET}',  "$TargetPath"
    echo "target: $targetDir"

    New-Item $targetDir -ItemType directory -Force | Out-Null
    
    for ($j = 0; $j -lt $sections.length; $j++) {
        $sectionName = $sections[$j]
        echo "  $sectionName"

        $files = @($jsonTargetDir."$sectionName")

        for ($k = 0; $k -lt $files.Length; $k++) {
            $file = $files[$k]
            $file = $file -replace "/", "\"
            $file = $file -replace '\${BUILD}', "$BuildPath"
            $file = $file -replace '\${ROOT}', "$RootPath"
            $file = $file -replace '\${QTPATH}', "$QtPath"

            if ($file.Contains('|')) {
                $fileParts = $file.Split('|')

                $include = $fileParts[1].Trim().Split(' ')
                $exclude = $fileParts[2].Trim().Split(' ')

                $file = $fileParts[0].Trim()
                $dirPath = (Resolve-Path $file).path
                $dirName = $dirPath.Split('\')[-1]
                echo "      $file ($include | $exclude)"

                Get-ChildItem -Path $dirPath\* -Include $include -Exclude $exclude | Foreach-Object {

                    $dest = $_.FullName -replace [regex]::Escape($dirPath),"$targetDir\$dirName"
                    $destDir = $_.Directory.FullName -replace [regex]::Escape($dirPath),"$targetDir\$dirName"

                    New-Item "$destDir" -ItemType directory -Force | Out-Null
                    Copy-Item -Force -path $_ -destination $dest
                }
            }
            else {
                echo "      $file"
                Copy-Item -Force "$file" $targetDir
            }
        }
    }
}
