param (
    $AppName,
    $Publisher,
    $MsixPath,
    $SigntoolPath
)

$ErrorActionPreference = "Stop"

try {
    $cert = Get-ChildItem Cert:\CurrentUser\My |
        Where-Object Subject -eq $Publisher |
        Select-Object -First 1

    if (-not $cert) {
        Write-Host ">>> Generating new certificate..."

        $cert = New-SelfSignedCertificate `
            -Type Custom `
            -Subject $Publisher `
            -FriendlyName "$AppName Dev Cert" `
            -KeyUsage DigitalSignature `
            -CertStoreLocation Cert:\CurrentUser\My `
            -NotAfter (Get-Date).AddYears(10) `
            -TextExtension @(
                '2.5.29.37={text}1.3.6.1.5.5.7.3.3',
                '2.5.29.19={text}'
            )
    }

    $certPath = "$MsixPath.cer"
    Export-Certificate -Cert $cert -FilePath $certPath | Out-Null

    Write-Host ">>> Exported $certPath"
    Write-Host ">>> Import it to 'Local Machine' -> 'Trusted Root Certification Authorities' if you want to install the MSIX package"

    & $SigntoolPath sign /fd SHA256 /sha1 $cert.Thumbprint $MsixPath
    if ($LASTEXITCODE) {
        throw "Signtool failed"
    }
} catch {
    Write-Host ">>> Error: $($_.Exception.Message)"
    exit 1
}
