param(
    [Parameter(Mandatory = $true)]
    [string]$NewRoot
)

$tempFile = Join-Path $env:TEMP "p4_client_spec.txt"

try
{
    # Export current workspace spec
    p4 client -o | Out-File -Encoding utf8 $tempFile

    $content = Get-Content $tempFile
    $updated = $content | ForEach-Object {
        if ($_ -match '^Root:\s+') # if line starts with Root: and any amount of spaces
        {
            "Root:`t$NewRoot" # then replace it with the new root
        }
        else
        {
            $_ # otherwise no changes
        }
    }

    # save to the temp file
    $updated | Out-File -Encoding utf8 $tempFile

    # import the temp file into p4 client temp file (saved automatically on exit)
    Get-Content $tempFile | p4 client -i

    Write-Host "Perforce root updated to:"
    Write-Host $NewRoot
}
finally
{
    if (Test-Path $tempFile)
    {
        Remove-Item $tempFile -Force # cleanup
    }
}