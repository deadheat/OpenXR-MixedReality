try {
    Push-Location $PSScriptRoot\shared\ext\bgfx
    ..\bin\genie --with-tools --with-dynamic-runtime --vs=winstore100 vs2019
    ..\bin\genie --with-tools --with-dynamic-runtime vs2019

    Get-ChildItem .build\projects\vs2019 | ForEach-Object {
        $content = Get-Content $_.FullName -Raw
        $content = $content -replace '8.1', '10.0'
        $content | Set-Content $_.FullName
    }

    Get-ChildItem .build\projects\vs2019-winstore100 | ForEach-Object {
        $content = Get-Content $_.FullName -Raw
        $content = $content -replace '8.1', '10.0'
        $content | Set-Content $_.FullName
    }
}
finally {
    Pop-Location
}
